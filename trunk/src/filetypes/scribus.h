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
// Copyright (C) 2007,2010-2012 by Tom Lechner
//
#ifndef FILETYPES_SCRIBUS_H
#define FILETYPES_SCRIBUS_H


#include "../version.h"
#include "../document.h"
#include "filefilters.h"




namespace Laidout {


void installScribusFilter();

//------------------------ Scribus in/reimpose/out helpers -------------------------------------
int addScribusDocument(const char *file, Document *existingdoc=NULL);
int exportImposedScribus(Document *doc,const char *imposeout);

//------------------------------------ ScribusExportFilter ----------------------------------
class ScribusExportFilter : public ExportFilter
{
 protected:
 public:
	ScribusExportFilter();
	virtual ~ScribusExportFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension() { return "sla"; }
	virtual const char *Format() { return "Scribus"; }
	virtual const char *Version() { return "1.3.3.8"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();

	virtual int Out(const char *filename, Laxkit::anObject *context, ErrorLog &log);

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	//virtual int Verify(Laxkit::anObject *context); //preflight checker
};


//------------------------------------ ScribusImportFilter ----------------------------------
class ScribusImportFilter : public ImportFilter
{
 public:
	virtual ~ScribusImportFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension() { return "sla"; }
	virtual const char *Format() { return "Scribus"; }
	virtual const char *Version() { return "1.3.3.*"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();

	virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	
	
	virtual const char *FileType(const char *first100bytes);
	virtual int In(const char *file, Laxkit::anObject *context, ErrorLog &log);
};


} // namespace Laidout

#endif




