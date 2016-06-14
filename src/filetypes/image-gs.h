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
// Copyright (C) 2007 by Tom Lechner
//
#ifndef FILETYPES_IMAGE_GS_H
#define FILETYPES_IMAGE_GS_H

#include "../version.h"
#include "../document.h"
#include "filefilters.h"




namespace Laidout {


void installImageFilter();


//------------------------------------ ImageGsExportFilter ----------------------------------
class ImageGsExportFilter : public ExportFilter
{
 protected:
 public:
	ImageGsExportFilter();
	virtual ~ImageGsExportFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension();
	virtual const char *Format() { return "Image via gs"; }
	virtual const char *Version() { return ""; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();

	virtual int Out(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	//virtual int Verify(Laxkit::anObject *context); //preflight checker
};


} // namespace Laidout

#endif
	
