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

#include <lax/transformmath.h>
#include <lax/laximages-imlib.h>
#include "psout.h"
#include "psimagepatch.h"
#include "psimage.h"
#include "psfilters.h"

#include <Imlib2.h>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxInterfaces;

//! Output postscript for an ImagePatchData. 
/*! \ingroup postscript
 * \todo *** this is in the serious hack stage
 */
void psImagePatch(FILE *f,LaxInterfaces::ImagePatchData *i)
{
	 // make an ImageData covering the bounding box

	 //find bounds of the patch in paper coordinates
	flatpoint ul=transform_point(psCTM(),flatpoint(i->minx,i->miny)),
			  ur=transform_point(psCTM(),flatpoint(i->maxx,i->miny)),
			  ll=transform_point(psCTM(),flatpoint(i->minx,i->maxy)),
			  lr=transform_point(psCTM(),flatpoint(i->maxx,i->maxy));
	DBG cerr <<"  ul: "<<ul.x<<','<<ul.y<<endl;
	DBG cerr <<"  ur: "<<ur.x<<','<<ur.y<<endl;
	DBG cerr <<"  ll: "<<ll.x<<','<<ll.y<<endl;
	DBG cerr <<"  lr: "<<lr.x<<','<<lr.y<<endl;

	 //bounds could be any parallelogram, width and height are lengths along edges
	int width,height;
	width= (int)(sqrt((ul-ur)*(ul-ur))/72*psDpi());
	height=(int)(sqrt((ul-ll)*(ul-ll))/72*psDpi());
	
	Imlib_Image image;
	image=imlib_create_image(width,height);
	imlib_context_set_image(image);
	imlib_image_set_has_alpha(1);
	DATA32 *buf=imlib_image_get_data();
	memset(buf,0,width*height*4); // make whole transparent/black
	//memset(buf,0xff,width*height*4); // makes whole non-transparent/white
	
	 // create an image that spans the bounding box of the patch
	unsigned char *buffer;
	buffer=(unsigned char *) buf;
	double a=(i->maxx-i->minx)/width,
		   d=(i->miny-i->maxy)/height;
	double m[6]; //takes points from i to buffer
	m[0]=1/a;
	m[1]=0;
	m[2]=0;
	m[3]=1/d;
	m[4]=-i->minx/a;
	m[5]=-i->maxy/d;

	i->renderToBuffer(buffer,width,height, 0,8,4);
	imlib_image_put_back_data(buf);
	imlib_image_flip_vertical();
	ImageData img;
	LaxImage *limg=new LaxImlibImage(NULL,image);
	img.SetImage(limg);
	limg->dec_count();

	 // set image transform
	double mm[6];
	transform_invert(mm,m);
	img.m(mm);

	 // push axes
	psPushCtm();
	psConcat(img.m());
	fprintf(f,"gsave\n"
			  "[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				img.m(0), img.m(1), img.m(2), img.m(3), img.m(4), img.m(5)); 
	
	psImage(f,&img);
	
	 // pop axes
	fprintf(f,"grestore\n\n");
	psPopCtm();
}

