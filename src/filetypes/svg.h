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
#ifndef FILETYPES_SVG_H
#define FILETYPES_SVG_H

#include "../document.h"

int svgout(const char *version, Document *doc);
Document *svgin(const char *file,Document *doc,int startpage);

//------------------------------------ SvgOutputFilter ----------------------------------
class SvgOutputFilter
{
 protected:
 public:
	virtual ~SvgOutputFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension() { return "svg"; }
	virtual const char *Format() { return "Svg"; }
	virtual const char *Version() { return "1.0"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	
	
	virtual int Out(const char *file, Laxkit::anObject *context, char **error_ret) = 0;
	virtual int Verify(Laxkit::anObject *context) = 0; //preflight checker
};


#endif
	
