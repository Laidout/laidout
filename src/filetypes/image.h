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
#include "../document.h"
#include "filefilters.h"




namespace Laidout {


void installImageFilter();


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
	
