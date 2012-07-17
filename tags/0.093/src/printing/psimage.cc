//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2007 by Tom Lechner
//

#include <X11/Xlib.h>
#include <lax/laximages-imlib.h>
#include "psimage.h"
#include "psfilters.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

void psImage_masked2(FILE *f,LaxInterfaces::ImageData *img);
	
//! Output postscript for a Laxkit::ImageData. 
/*! \ingroup postscript
 * 
 * In the distant future, could chop this down to use Postscript LL3 -or- LL2?
 * Does LL3 only. this is a distant todo only if there is interest. Otherwise,
 * people can use ps2ps -dLanguageLevel=2? (did one test with transparent image,
 * and gv slows way the hell down)
 * 
 * \todo *** the output should be tailored to the specified psDpi(), otherwise the output
 *   file will sometimes be quite enormous. Also, repeat images are outputted for each
 *   instance, which also potentially increases size a lot
 * \todo *** this still assumes a LaxImlibImage.
 */
void psImage(FILE *f,LaxInterfaces::ImageData *img)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	if (!img || !img->image) return;

	LaxImlibImage *imlibimg=dynamic_cast<LaxImlibImage *>(img->image);
	if (!imlibimg) return;
	
	imlib_context_set_image(imlibimg->Image(1));
	
	//if (imlib_image_has_alpha()) { psImage_masked_interleave1(f,img); return; }
	if (imlib_image_has_alpha()) { psImage_masked_interleave1(f,img); return; }
	//if (imlib_image_has_alpha()) { psImage_103(f,img); return; }
	
	char *bname=NULL;
	if (img->filename) bname=strrchr(img->filename,'/'); 
	if (!bname) bname=img->filename;
	if (bname) fprintf(f," %% image %s\n",bname);
	////DBG cerr <<"*** image "<<(bname?bname:"(unknown)")<<" has no alpha"<<endl;
			
	fprintf(f,"[%.10g 0 0 %.10g 0 0] concat\n",
			 img->maxx,img->maxy);
	
	 // image out
	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
	int width,height;
	width=imlib_image_get_width();
	height=imlib_image_get_height();
	int len=3*width*height;
	 //*** for some reason if I just do uchar rgbbuf[len], it will crash for large images
	unsigned char *rgbbuf=new unsigned char[len]; //***this could be redone to not need new huge array like this
	unsigned char r,g,b;
	//DATA32 bt;
	////DBG cerr <<"rgbbug"<<rgbbuf[0]<<endl;//***
	for (int x=0; x<width; x++) {
		for (int y=0; y<height; y++) {
			//bt=buf[y*width+x];
			r=(buf[y*width+x]&((unsigned long)255<<16))>>16;
			g=(buf[y*width+x]&((unsigned long)255<<8))>>8;
			b=(buf[y*width+x]&(unsigned long)255);
			rgbbuf[y*width*3+x*3  ]=r;
			rgbbuf[y*width*3+x*3+1]=g;
			rgbbuf[y*width*3+x*3+2]=b;
			//printf("%x=%x,%x,%x ",bt,r,g,b);
		}
	}
	//int width=(int)img->maxx,
	//    height=(int)img->maxy;
	//double w,h;
	//w=200;
	//h=height*200./width;
	fprintf(f,
			"/DeviceRGB setcolorspace\n"
			"<<\n"
			"  /ImageType 1\n"
			"  /Width %d\n"
			"  /Height %d\n"
			"  /BitsPerComponent 8\n"
			"  /Decode [0 1 0 1 0 1]\n"
			"  /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"  /DataSource currentfile\n"
			"  /ASCII85Decode filter \n"
			">> image\n", width, height, width, height, height);

	 // do the Ascii85Encode filter
	Ascii85_out(f,rgbbuf,len,1,75);

	delete[] rgbbuf;
	imlibimg->doneForNow();
}



//! Output postscript for a Laxkit::ImageData, making a mask from its transparency.
/*! \ingroup postscript
 * Does simple 50 percent threshhold image mask for trasparent images.
 *
 * \todo *** fixme!!! Trying to do InterleaveType 2
 */
//void psImage_masked_interleave2(FILE *f,LaxInterfaces::ImageData *img)
//{
//	 // the image gets put in a postscript box with sides 1x1, and the matrix
//	 // in the image is ??? so must set
//	 // up proper transforms
//	
//	if (!img || !img->image) return;
//
//	LaxImlibImage *imlibimg=dynamic_cast<LaxImlibImage *>(img->image);
//	if (!imlibimg) return;
//	
//	imlib_context_set_image(imlibimg->Image(1));
//	int width,height;
//	width=imlib_image_get_width();
//	height=imlib_image_get_height();
//	
//			
//	fprintf(f,"[%.10g 0 0 %.10g 0 0] concat\n",
//			 img->maxx,img->maxy);
//	
//	 // image out
//	fprintf(f,
//			"/DeviceRGB setcolorspace\n"
//			"<<\n"
//			"  /ImageType 3\n"
//			"  /InterleaveType 2"
//			"  /DataDict <<\n"
//			"    /ImageType 1\n"
//			"    /Width %d\n"
//			"    /Height %d\n"
//			"    /BitsPerComponent 8\n"
//			"    /Decode [0 1 0 1 0 1]\n"
//			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
//			"    /DataSource currentfile\n"
//			"    /ASCII85Decode filter \n"
//			"  >>\n", width, height, width, height, height);
//
//	 //---------------write out MaskDict
//	fprintf(f,
//			"  /MaskDict <<\n"
//			"    /ImageType 1\n"
//			"    /Width %d\n"
//			"    /Height %d\n"
//			"    /BitsPerComponent 1\n"
//			"    /Decode [0 1]\n"
//			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
//			"  >>\n", width, height, width, height, height);
//	
//	fprintf(f,">> image\n\n");
//	
//	 //----------- write out DataSource
//	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
//	DATA32 color;
//
//	int apos,abit,alen,pos;
//
//	alen=width/8+1;
//	unsigned char row[(width*3+alen)/4*4+5],alpha[alen];
//	unsigned char a,r,g,b;
//
//	int w=0;
//	pos=0;
//	apos=-1;
//	for (int y=0; y<height; y++) {
//		apos=-1;
//		abit=7;
//		memset(alpha,width/8+1,0);
//		for (int x=0; x<width; x++) {
//			if (abit==7) apos++;//this is so pos gets updated correctly below
//			color=buf[y*width+x];
//
//			a=(color&((unsigned long)255<<24))>>24;
//			r=(color&((unsigned long)255<<16))>>16;
//			g=(color&((unsigned long)255<<8))>>8;
//			b=(color& (unsigned long)255);
//			
//			row[pos++]=r;
//			row[pos++]=g;
//			row[pos++]=b;
//			
//			if (a>127) alpha[apos]|=1<<abit;
//			abit--;
//			if (abit<0) abit=7;
//			//DBG fprintf(stderr,"%x=%x,%x,%x ",bt,r,g,b);
//		}
//		memcpy(row+pos,alpha,apos+1);
//		pos+=apos+1; //pos is now how many bytes to consider
//		Ascii85_out(f, row, pos/4*4, 0, 75, &w);
//		if (pos%4) memmove(row,row+pos/4*4,pos%4);
//		pos%=4;
//	}
//	fprintf(f,"~>\n");
//	
//}

//! Output postscript for a Laxkit::ImageData, making a mask from its transparency.
/*! \ingroup postscript
 * Does simple 50 percent threshhold image mask for trasparent images.
 */
void psImage_masked_interleave1(FILE *f,LaxInterfaces::ImageData *img)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	if (!img || !img->image) return;
	
	LaxImlibImage *imlibimg=dynamic_cast<LaxImlibImage *>(img->image);
	if (!imlibimg) return;
	
	imlib_context_set_image(imlibimg->Image(1));
	int width,height;
	width=imlib_image_get_width();
	height=imlib_image_get_height();
	
			
	fprintf(f,"[%.10g 0 0 %.10g 0 0] concat\n",
			 img->maxx,img->maxy);
	
	 // image out
	fprintf(f,
			"/DeviceRGB setcolorspace\n"
			"<<\n"
			"  /ImageType 3\n"
			"  /InterleaveType 1"
			"  /DataDict <<\n"
			"    /ImageType 1\n"
			"    /Width %d\n"
			"    /Height %d\n"
			"    /BitsPerComponent 8\n"
			"    /Decode [0 1 0 1 0 1]\n"
			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"    /DataSource currentfile\n"
			"    /ASCII85Decode filter \n"
			"  >>\n", width, height, width, height, height);

	 //---------------write out MaskDict
	fprintf(f,
			"  /MaskDict <<\n"
			"    /ImageType 1\n"
			"    /Width %d\n"
			"    /Height %d\n"
			"    /BitsPerComponent 8\n"
			"    /Decode [0 1]\n"
			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"  >>\n", width, height, width, height, height);
	
	fprintf(f,
			">> image\n\n");
	
	
	 //----------- write out DataSource
	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
	DATA32 color;

	unsigned char row[width*4+5];
	unsigned char a,r,g,b;

	int w=0,pos=0;
	////DBG int numout=0;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			color=buf[y*width+x];

			a=(color&((unsigned long)255<<24))>>24;
			r=(color&((unsigned long)255<<16))>>16;
			g=(color&((unsigned long)255<<8))>>8;
			b=(color& (unsigned long)255);
			
			row[pos++]=(a>127?0:1);
			row[pos++]=r;
			row[pos++]=g;
			row[pos++]=b;
			
			////DBG fprintf(stderr,"%x=%a,%x,%x,%x ",bt,a,r,g,b);
		}
		////DBG numout+=pos/4+4;
		Ascii85_out(f, row, pos/4*4, 0, 75, &w);
		if (pos%4) memmove(row,row+pos/4*4,pos%4);
		pos%=4;
	}
	fprintf(f,"~>\n");
	////DBG cerr <<"*****done writing DataDict:"<<numout<<"  w,h:"<<width<<','<<height<<"  *4="<<width*height*4<<endl;
	
}

//! Output a Ghostscript ready type 103 image dictionary for a Laxkit::ImageData. 
/*! \ingroup postscript
 *
 * \todo *** does type 103 even work? have no idea if this is correct code.
 *    it produces image but no transparency... maybe some other switch to turn on?
 */
void psImage_103(FILE *f,LaxInterfaces::ImageData *img)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	if (!img || !img->image) return;
	
	LaxImlibImage *imlibimg=dynamic_cast<LaxImlibImage *>(img->image);
	if (!imlibimg) return;
	
	imlib_context_set_image(imlibimg->Image(1));
	int width,height;
	width=imlib_image_get_width();
	height=imlib_image_get_height();
	
			
	fprintf(f,"[%.10g 0 0 %.10g 0 0] concat\n",
			 img->maxx,img->maxy);
	
	 // image out
	fprintf(f,
			"/DeviceRGB setcolorspace\n"
			"<<\n"
			"  /ImageType 103\n"
			"  /InterleaveType 1"
			"  /DataDict <<\n"
			"    /ImageType 1\n"
			"    /Width %d\n"
			"    /Height %d\n"
			"    /BitsPerComponent 8\n"
			"    /Decode [0 1 0 1 0 1]\n"
			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"    /DataSource currentfile\n"
			"    /ASCII85Decode filter \n"
			"  >> \n", width, height, width, height, height);

	 //---------------write out MaskDict
	fprintf(f,
			"  /ShapeMaskDict <<\n"
			"    /ImageType 1\n"
			"    /Width %d\n"
			"    /Height %d\n"
			"    /BitsPerComponent 8\n"
			"    /Decode [0 1]\n"
			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"    /InterleaveType 1"
			"  >> \n", width, height, width, height, height);
	fprintf(f,
			"  /OpacityMaskDict <<\n"
			"    /ImageType 1\n"
			"    /Width %d\n"
			"    /Height %d\n"
			"    /BitsPerComponent 8\n"
			"    /Decode [0 1]\n"
			"    /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"    /InterleaveType 1"
			"  >> \n", width, height, width, height, height);
	
	fprintf(f,">> image\n\n");
	

	 //----------- write out DataSource
	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
	DATA32 color;

	unsigned char row[width*5+5];
	unsigned char a,r,g,b;

	int w=0,pos=0;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			color=buf[y*width+x];

			a=(color&((unsigned long)255<<24))>>24;
			r=(color&((unsigned long)255<<16))>>16;
			g=(color&((unsigned long)255<<8))>>8;
			b=(color& (unsigned long)255);
			
			row[pos++]=a;
			row[pos++]=a;
			row[pos++]=r;
			row[pos++]=g;
			row[pos++]=b;
			
			////DBG fprintf(stderr,"%x=%a,%x,%x,%x ",bt,a,r,g,b);
		}
		Ascii85_out(f, row, pos/4*4, 0, 75, &w);
		if (pos%4) memmove(row,row+pos/4*4,pos%4);
		pos%=4;
	}
	fprintf(f,"~>\n");
	
}
