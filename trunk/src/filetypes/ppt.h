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
// Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef SAVEPPT_H
#define SAVEPPT_H

#include <lax/interfaces/somedata.h>
#include "../version.h"
#include "../document.h"
#include "filefilters.h"


void installPptFilter();

//------------------------------------- PptoutFilter -----------------------------------
class PptoutFilter : public ExportFilter
{
 protected:
 public:
	PptoutFilter();
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Passepartout"; }
	virtual const char *DefaultExtension() { return "ppt"; }
	virtual const char *Version() { return "0.6"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	
	virtual int Out(const char *filename, Laxkit::anObject *context, char **error_ret);
};


//------------------------------------- PptinFilter -----------------------------------
class PptinFilter : public ImportFilter
{
 protected:
	virtual int pptDumpInGroup(LaxFiles::Attribute *att, Group *group);
 public:
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Passepartout"; }
	virtual const char *DefaultExtension() { return "ppt"; }
	virtual const char *Version() { return "0.6"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	
	virtual const char *FileType(const char *first100bytes);
	virtual int In(const char *file, Laxkit::anObject *context, char **error_ret);
};



#endif
	
