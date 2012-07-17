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
// Copyright (C) 2004-2009,2012 by Tom Lechner
//
#ifndef FILETYPES_EPS_H
#define FILETYPES_EPS_H

#include <lax/interfaces/somedata.h>
#include "../version.h"
#include "../document.h"
#include "filefilters.h"
#include "../printing/psout.h"


void installPostscriptFilters();

//------------------------------------- EpsOutFilter -----------------------------------
class EpsOutFilter : public ExportFilter
{
 protected:
 public:
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "EPS"; }
	virtual const char *DefaultExtension() { return "eps"; }
	virtual const char *Version() { return "3.0"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual StyleDef *GetStyleDef();
	
	virtual int Out(const char *filename, Laxkit::anObject *context, ErrorLog &log);
};


//------------------------------------- PsOutFilter -----------------------------------
class PsOutFilter : public ExportFilter
{
 protected:
 public:
	PsOutFilter();
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Postscript"; }
	virtual const char *DefaultExtension() { return "ps"; }
	virtual const char *Version() { return "LL3"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual StyleDef *GetStyleDef();
	
	virtual int Out(const char *filename, Laxkit::anObject *context, ErrorLog &log);
};





#endif
	
