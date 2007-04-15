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
#ifndef EXTRAS_H
#define EXTRAS_H

#include "laidout.h"
#include <lax/interfaces/imageinterface.h>

//------------------------------------- class ImagePlopInfo ------------------------------------
class ImagePlopInfo
{
 public:
	LaxInterfaces::ImageData *image;
	int error;
	int dpi;
	int page;
	double *xywh;
	ImagePlopInfo *next;
	ImagePlopInfo(LaxInterfaces::ImageData *img, int ndpi, int npage, double *d);
	~ImagePlopInfo();
	void add(LaxInterfaces::ImageData *img, int ndpi, int npage, double *d);
};


//------------------------------------- functions -------------------------------
int dumpInImageList(Document *doc,const char *file, int startpage, int defaultdpi, int perpage);
int dumpInImageList(Document *doc,LaxFiles::Attribute *att, int startpage, int defaultdpi, int perpage);
int dumpInImages(Document *doc, int startpage, const char *pathtoimagedir, int perpage=1, int ddpi=150);
int dumpInImages(Document *doc, int startpage, const char **imagefiles, const char **previewfiles, 
				 int nimages, int perpage=1, int ddpi=150);
//int dumpInImages(Document *doc, int startpage, LaxInterfaces::ImageData **images, int nimages, int perpage=1, int ddpi=150);
int dumpInImages(Document *doc, ImagePlopInfo *images, int startpage);

#endif

