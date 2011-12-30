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
// Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef IMPORT_IMAGE_H
#define IMPORT_IMAGE_H

#include "laidout.h"
#include <lax/interfaces/imageinterface.h>

//------------------------------------- ImagePlopInfo ------------------------------------
class ImagePlopInfo
{
 public:
	LaxInterfaces::ImageData *image; //if image==NULL, then is settings object
	int scaleflag; //0=scale by dpi, 3=scale to fit always, 1=scale up if necessary 2=scale down to fit if necessary
	double alignx; //0=full left, 100=full right
	double aligny; //0=full top, 100=full bottom
	Laxkit::NumStack<flatpoint> *alignment; //one per imposition page type
	int error;
	double dpi;
	int page;
	double *xywh;
	ImagePlopInfo *next;
	ImagePlopInfo(LaxInterfaces::ImageData *img, double ndpi, int npage, int sflag, double *d);
	~ImagePlopInfo();
	void add(LaxInterfaces::ImageData *img, double ndpi, int npage, int sflag, double *d);
};


//------------------------------------- ImportImageSettings ------------------------------------
class ImportImageSettings : public Laxkit::RefCounted, public LaxFiles::DumpUtility
{
 public:
	char *settingsname, *filename;
	double defaultdpi; //overrideable per image
	int scaleup, scaledown; //overrideable per image
	Laxkit::NumStack<flatpoint> alignment; //one per imposition page type

	int perpage; //number per page, or as will fit (-1), or all in 1 page (-2)

	//PtrStack<ImageAlternateInfo*> alternatesettings; //overrides laidout default settings

	int startpage; //which page (or area) to start dumping images into
	int destination;
	Laxkit::RefCounted *destobject;
//	Arrangement *arrangement;

	ImportImageSettings();
	virtual ~ImportImageSettings();
	virtual ImportImageSettings *duplicate();

	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
};


//------------------------------------- functions -------------------------------
int dumpOutImageListFormat(FILE *f);
int dumpInImageList(ImportImageSettings *settings, Document *doc,const char *file);
int dumpInImageList(ImportImageSettings *settings, Document *doc,LaxFiles::Attribute *att);
int dumpInImages(ImportImageSettings *settings, Document *doc, const char *pathtoimagedir);
int dumpInImages(ImportImageSettings *settings, Document *doc,
				 const char **imagefiles, const char **previewfiles, int nimages);

int dumpInImages(ImportImageSettings *settings, Document *doc, ImagePlopInfo *images, int startpage);

#endif

