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
// Copyright (C) 2004-2009 by Tom Lechner
//
#ifndef IMPORT_IMAGE_H
#define IMPORT_IMAGE_H

#include "laidout.h"
#include <lax/interfaces/imageinterface.h>

//------------------------------------- class ImagePlopInfo ------------------------------------
class ImagePlopInfo
{
 public:
	LaxInterfaces::ImageData *image; //if image==NULL, then is settings object
	int scaleflag; //0=scale by dpi, 1=scale to fit always, 2=scale down to fit if necessary
	double alignx; //0=full left, 100=full right
	double aligny; //0=full top, 100=full bottom
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
int dumpOutImageListFormat(FILE *f);
int dumpInImageList(Document *doc,const char *file, int startpage, int defaultdpi, int perpage);
int dumpInImageList(Document *doc,LaxFiles::Attribute *att, int startpage, int defaultdpi, int perpage);
int dumpInImages(Document *doc, int startpage, const char *pathtoimagedir, int perpage=1, int ddpi=150);
int dumpInImages(Document *doc, int startpage, const char **imagefiles, const char **previewfiles, 
				 int nimages, int perpage=1, int ddpi=150);
//int dumpInImages(Document *doc, int startpage, LaxInterfaces::ImageData **images, int nimages, int perpage=1, int ddpi=150);
int dumpInImages(Document *doc, ImagePlopInfo *images, int startpage);

#endif

