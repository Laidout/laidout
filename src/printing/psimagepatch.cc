//
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include <lax/transformmath.h>
#include <lax/laximages.h>
#include "psout.h"
#include "psimagepatch.h"
#include "psimage.h"
#include "psfilters.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//! Output postscript for an ImagePatchData. 
/*! \ingroup postscript
 * \todo *** this is in the serious hack stage
 */
void psImagePatch(FILE *f,LaxInterfaces::ImagePatchData *i)
{
	 // make an ImageData covering the bounding box

	 //find bounds of the patch in paper coordinates
	flatpoint ul=transform_point(psCTM(),flatpoint(i->minx,i->miny));
	flatpoint ur=transform_point(psCTM(),flatpoint(i->maxx,i->miny));
	flatpoint ll=transform_point(psCTM(),flatpoint(i->minx,i->maxy));

	DBG flatpoint lr=transform_point(psCTM(),flatpoint(i->maxx,i->maxy));
	DBG cerr <<"  ul: "<<ul.x<<','<<ul.y<<endl;
	DBG cerr <<"  ur: "<<ur.x<<','<<ur.y<<endl;
	DBG cerr <<"  ll: "<<ll.x<<','<<ll.y<<endl;
	DBG cerr <<"  lr: "<<lr.x<<','<<lr.y<<endl;

	 //bounds could be any parallelogram, width and height are lengths along edges
	int width,height;
	width= (int)(sqrt((ul-ur)*(ul-ur))/72*psDpi());
	height=(int)(sqrt((ul-ll)*(ul-ll))/72*psDpi());
	
	 // create an image that spans the bounding box of the patch
	LaxImage *limg = ImageLoader::NewImage(width,height);
	unsigned char *buffer = limg->getImageBuffer();
	memset(buffer, 0, width*height*4); // make whole transparent/black
	
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
	 //we need to flip vertically
	unsigned char line[width*4];
	for (int y=0; y<height/2; y++) {
		memcpy(line, buffer + (y*width*4), width*4);
		memcpy(buffer + (y*width*4), buffer + ((height-1-y)*width*4), width*4);
		memcpy(buffer + ((height-1-y)*width*4), line, width*4);
	}
	limg->doneWithBuffer(buffer);

	ImageData img;
	img.SetImage(limg,NULL);
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
				img.m((int)0), img.m(1), img.m(2), img.m(3), img.m(4), img.m(5)); 
	
	psImage(f,&img);
	
	 // pop axes
	fprintf(f,"grestore\n\n");
	psPopCtm();
}

} // namespace Laidout

