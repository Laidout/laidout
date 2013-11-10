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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef LAIDOUTIMPORT_H
#define LAIDOUTIMPORT_H

#include <lax/interfaces/somedata.h>
#include "../version.h"
#include "../document.h"
#include "filefilters.h"



namespace Laidout {


void installLaidoutFilter();

//------------------------------------- LaidoutOutFilter -----------------------------------
class LaidoutOutFilter : public ExportFilter
{
 protected:
 public:
	LaidoutOutFilter();
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Laidout"; }
	virtual const char *DefaultExtension() { return "laidout"; }
	virtual const char *Version() { return LAIDOUT_VERSION; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();
	
	virtual int Out(const char *filename, Laxkit::anObject *context, ErrorLog &log);
};


//------------------------------------- LaidoutInFilter -----------------------------------
class LaidoutInFilter : public ImportFilter
{
 protected:
 public:
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Laidout"; }
	virtual const char *DefaultExtension() { return "laidout"; }
	virtual const char *Version() { return LAIDOUT_VERSION; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();
	
	virtual const char *FileType(const char *first100bytes);
	virtual int In(const char *file, Laxkit::anObject *context, ErrorLog &log);
};


} // namespace Laidout

#endif
	
