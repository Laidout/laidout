//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// functions to output postscript images
// at the moment does Imlib_Images
//
// the stuff here is not actually used in Laidout.. bits of it have been
// transported elsewhere

#include <X11/Xlib.h>
#include <Imlib2.h>
#include <libintl.h>

#include "psfilters.h"


#include <iostream>
using namespace std;
#define DBG 


//-------------------------------- Imlib_Image to ps ------------------------------------

//! Output an Imlib_Image as postscript with Ascii85 encoding.
/*! \ingroup postscript
 */
void Imlib_ImageToPS(FILE *psout,Imlib_Image image)
{
	if (!psout || !image) return;
	imlib_context_set_image(image);
	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
	DATA32 bt;
	int width,height;
	width=imlib_image_get_width();
	height=imlib_image_get_height();
	int len=3*width*height;
	unsigned char rgbbuf[15];
	unsigned char r,g,b;
	DBG cerr <<endl;
	fprintf(psout,
			"/DeviceRGB setcolorspace\n"
			"<<\n"
			"  /ImageType 1  /Width %d  /Height %d\n"
			"  /BitsPerComponent 8\n"
			"  /Decode [0 1 0 1 0 1]\n"
			"  /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"  /DataSource currentfile\n"
			"  /ASCII85Decode filter \n"
			">> image\n", width, height, width, height, height);

	 // do the Ascii85Encode filter
	int index=0,i2,linelen=0;
	while (index<len) {
		i2=0;
		bt=buf[index++];
		rgbbuf[0]=(bt&0xff0000)>>16;
		rgbbuf[1]=(bt&0xff00)>>8;
		rgbbuf[2]=(bt&0xff);
		i2=3;

		if (index<len) {
			bt=buf[index++];
			rgbbuf[3]=(bt&0xff0000)>>16;
			rgbbuf[4]=(bt&0xff00)>>8;
			rgbbuf[5]=(bt&0xff);
			i2=6;
		}
		if (index<len) {
			bt=buf[index++];
			rgbbuf[6]=(bt&0xff0000)>>16;
			rgbbuf[7]=(bt&0xff00)>>8;
			rgbbuf[8]=(bt&0xff);
			i2=9;
		} 
		if (index<len) {
			bt=buf[index++];
			rgbbuf[9]=(bt&0xff0000)>>16;
			rgbbuf[10]=(bt&0xff00)>>8;
			rgbbuf[11]=(bt&0xff);
			i2=12;
		} 
		if (index<len) {
			bt=buf[index++];
			rgbbuf[12]=(bt&0xff0000)>>16;
			rgbbuf[13]=(bt&0xff00)>>8;
			rgbbuf[14]=(bt&0xff);
			i2=15;
		} 
		 
		
		Ascii85_out(psout,rgbbuf,i2,0,75,&linelen);
	}
	fprintf(psout,"~>\n");

}

//------------------------------- output a full postscript file with an image -----------------------

//! Output a full postscript file with an Imlib_Image in it. 
/*! \ingroup postscript
 * outputs to temp.ps
 */
void ps_Imlib_Image_out(Imlib_Image image)
{
	setlocale(LC_ALL,"C");
	FILE *f=fopen("temp.ps","w");
	if (!f) {
		DBG cerr <<"Failed to open temp.ps."<<endl;
		setlocale(LC_ALL,"");
		return;
	}

	 // write out header
	fprintf (f,
			"%%!PS-Adobe-3.0\n"
			"%%%%Orientation: Portrait\n"
			"%%%%Pages: 1\n"
			"%%%%PageOrder: Ascend\n"
			"%%%%CreationDate: Sat Feb 11 21:12:07 2006\n"
			"%%%%Creator: Test program\n"
			"%%%%For: tom <tom@doublian> (i686, Linux 2.6.15-1-k7)\n"
			"%%%%EndComments\n"
			"\n"
			"%%%%BeginProlog\n"
			"%%%%EndProlog\n"
			"\n"
			"%%%%BeginSetup\n"
			"%%%%EndSetup\n"
			"\n"
			"%%%%Page: \"1\" 1\n"
			"gsave  %% group\n"
			"[1 0 0 1 0 0] concat\n"
			"%% Raster image: blah.jpg\n"
			"save\n");

	 // write out image data
	double w,h;
	w=200;
	h=height*200./width;
	fprintf (f,"[%f 0 0 %f 165.786 325.676] concat\n", w,h);
	Imlib_ImageToPS(f,image);

	 // draw image border
	fprintf(f,"\nnewpath\n"
			"-.1 -.1 moveto 1.1 -.1 lineto 1.1 1.1 lineto -.1 1.1 lineto -.1 -.1 lineto closepath\n"
			".1 setlinewidth\n"
			"stroke\n");
	
	 // write out footer
	fprintf(f,
			"\n"
			"restore\n"
			"grestore %% /group\n"
			"\n"
			"showpage\n"
			"\n"
			"%%%%EOF\n");

	fclose(f);
	setlocale(LC_ALL,"");
}

