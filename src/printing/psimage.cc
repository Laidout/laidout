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

#include <lax/laximages.h>
#include "psimage.h"
#include "psfilters.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


namespace Laidout {


void psImage_masked2(FILE *f,LaxInterfaces::ImageData *img);

/*! Return true for any 4th char not 255. Else false. len is pixel length, assumed ordered BGRA.
 */
bool has_alpha(unsigned char *buf, long len)
{
	len*=4;
	for (int c=3; c<len; c+=4) if (buf[c]!=255) return true;
	return false;
}

//! Output postscript for a Laxkit::ImageData. 
/*! \ingroup postscript
 * 
 * In the distant future, could chop this down to use Postscript LL3 -or- LL2?
 * Does LL3 only. this is a distant todo only if there is interest. Otherwise,
 * people can use ps2ps -dLanguageLevel=2? (did one test with transparent image,
 * and gv slows way the hell down)
 *
 * Return 0 for success, or nonzero for could not output image.
 * 
 * \todo *** the output should be tailored to the specified psDpi(), otherwise the output
 *   file will sometimes be quite enormous. Also, repeat images are outputted for each
 *   instance, which also potentially increases size a lot
 */
int psImage(FILE *f,LaxInterfaces::ImageData *img)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	if (!img || !img->image) return 1;

	unsigned char *buf=img->image->getImageBuffer(); // ARGB
	if (!buf) return 2;

	int width,height;
	width =img->image->w();
	height=img->image->h();

	if (has_alpha(buf, width*height)) { return psImage_masked_interleave1(f, buf,width,height); }
	//if (has_alpha(buf, width*height)) { psImage_103(f,img); return; }
	

	 //so image has no transparency....
	char *bname=NULL;
	if (img->filename) bname=strrchr(img->filename,'/'); 
	if (!bname) bname=img->filename;
	if (bname) fprintf(f," %% image %s\n",bname);
			
	fprintf(f,"[%.10g 0 0 %.10g 0 0] concat\n",
			 img->maxx,img->maxy);
	
	 // image out
	 //for some reason if I just do uchar rgbbuf[len], it will crash for large images
	int len=3*width*height;
	unsigned char *rgbbuf=new unsigned char[len]; //***this could be redone to not need new huge array like this
	unsigned char r,g,b;
	for (int x=0; x<width; x++) {
		for (int y=0; y<height; y++) {
			r=buf[(y*width+x)*4 + 2];
			g=buf[(y*width+x)*4 + 1];
			b=buf[(y*width+x)*4    ];

			rgbbuf[y*width*3+x*3  ]=r;
			rgbbuf[y*width*3+x*3+1]=g;
			rgbbuf[y*width*3+x*3+2]=b;
		}
	}

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
	img->image->doneWithBuffer(buf);

	return 0;
}



//! Output postscript for a Laxkit::ImageData, making a mask from its transparency.
/*! \ingroup postscript
 * Does simple 50 percent threshhold image mask for trasparent images.
 *
 * \todo *** fixme!!! Trying to do InterleaveType 2
 */
//int psImage_masked_interleave2(FILE *f, unsigned char *buf, int width, int height)
//{
//	 // the image gets put in a postscript box with sides 1x1, and the matrix
//	 // in the image is ??? so must set
//	 // up proper transforms
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
//
//	 //----------- write out DataSource
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
//
//			r=buf[(y*width+x)*4 + 2];
//			g=buf[(y*width+x)*4 + 1];
//			b=buf[(y*width+x)*4    ];
//			a=buf[(y*width+x)*4 + 3];
//			
//			row[pos++]=r;
//			row[pos++]=g;
//			row[pos++]=b;
//			
//			if (a>127) alpha[apos]|=1<<abit;
//			abit--;
//			if (abit<0) abit=7;
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
int psImage_masked_interleave1(FILE *f, unsigned char *buf, int width, int height)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
			
	fprintf(f,"[%d 0 0 %d 0 0] concat\n",
			 width, height);
	
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

	unsigned char row[width*4+5];
	unsigned char a,r,g,b;

	int w=0,pos=0;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			r=buf[(y*width+x)*4 + 2];
			g=buf[(y*width+x)*4 + 1];
			b=buf[(y*width+x)*4    ];
			a=buf[(y*width+x)*4 + 3];
			
			row[pos++]=(a>127?0:1);
			row[pos++]=r;
			row[pos++]=g;
			row[pos++]=b; 
		}

		Ascii85_out(f, row, pos/4*4, 0, 75, &w);
		if (pos%4) memmove(row,row+pos/4*4,pos%4);
		pos%=4;
	}
	fprintf(f,"~>\n");
	
	return 0;
}

//! Output a Ghostscript ready type 103 image dictionary.
/*! \ingroup postscript
 *
 * \todo *** does type 103 even work? have no idea if this is correct code.
 *    it produces image but no transparency... maybe some other switch to turn on?
 */
int psImage_103(FILE *f, unsigned char *buf, int width, int height)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	
	fprintf(f,"[%d 0 0 %d 0 0] concat\n",
			 width, height);
	
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
	unsigned char row[width*5+5];
	unsigned char a,r,g,b;

	int w=0,pos=0;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			r=buf[(y*width+x)*4 + 2];
			g=buf[(y*width+x)*4 + 1];
			b=buf[(y*width+x)*4    ];
			a=buf[(y*width+x)*4 + 3];
			
			row[pos++]=a;
			row[pos++]=a;
			row[pos++]=r;
			row[pos++]=g;
			row[pos++]=b;
		}
		Ascii85_out(f, row, pos/4*4, 0, 75, &w);
		if (pos%4) memmove(row,row+pos/4*4,pos%4);
		pos%=4;
	}
	fprintf(f,"~>\n");
	
	return 0;
}


} // namespace Laidout

