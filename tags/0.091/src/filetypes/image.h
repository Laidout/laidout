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
// Copyright (C) 2007 by Tom Lechner
//
#ifndef FILETYPES_IMAGE_H
#define FILETYPES_IMAGE_H

#include "../version.h"
#include "../document.h"
#include "filefilters.h"



void installImageFilter();


//------------------------------------ SvgOutputFilter ----------------------------------
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
	virtual StyleDef *GetStyleDef();

	virtual int Out(const char *filename, Laxkit::anObject *context, char **error_ret);

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	//virtual int Verify(Laxkit::anObject *context); //preflight checker
};


#endif
	
