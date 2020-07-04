//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2016 by Tom Lechner
//
#ifndef FILETYPES_IMAGE_H
#define FILETYPES_IMAGE_H

#include "../version.h"
#include "../core/document.h"
#include "filefilters.h"




namespace Laidout {


void installImageFilter();


//----------------------------- ImageExportConfig -----------------------------
class ImageExportConfig : public DocumentExportConfig
{
 public:
	char *format;
	int use_transparent_bg;
	Laxkit::Color *background;
	int width, height; //px of output

	ImageExportConfig();
	ImageExportConfig(DocumentExportConfig *config);
	virtual ~ImageExportConfig();
	virtual const char *whattype() { return "ImageExportConfig"; }
	virtual ObjectDef* makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute * dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
};


//------------------------------------ ImageExportFilter ----------------------------------
class ImageExportFilter : public ExportFilter
{
 protected:
 public:
	ImageExportFilter();
	virtual ~ImageExportFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension();
	virtual const char *Format() { return "Image"; }
	virtual const char *Version() { return ""; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();
	virtual DocumentExportConfig *CreateConfig(DocumentExportConfig *fromconfig);

	virtual int Out(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	//virtual int Verify(Laxkit::anObject *context); //preflight checker
};


} // namespace Laidout

#endif
	
