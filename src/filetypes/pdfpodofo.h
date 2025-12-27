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
// Copyright (C) 2007-2015 by Tom Lechner
//
#ifndef FILETYPES_PDFPODOFO_H
#define FILETYPES_PDFPODOFO_H

#include "filefilters.h"
#include "../version.h"




namespace Laidout {

void installPodofoFilters();


//------------------------------------- PdfinFilter -----------------------------------
class PodofoImportFilter : public ImportFilter
{
 protected:
 public:
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Pdf Podofo"; }
	virtual const char *DefaultExtension() { return "pdf"; }
	virtual const char *Version() { return "*"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();
	
	virtual const char *FileType(const char *first100bytes);
	virtual int In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents,int contentslen);
};


//------------------------------------ PdfExportFilter ----------------------------------
class PodofoExportFilter : public ExportFilter
{
 protected:
	Laxkit::Utf8String pdf_version;

 public:
	PodofoExportFilter();
	virtual ~PodofoExportFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension() { return "pdf"; }
	virtual const char *Format() { return "Pdf Podofo"; }
	virtual const char *Version();
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();

	virtual int Out(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	//virtual int Verify(Laxkit::anObject *context); //preflight checker
};


} // namespace Laidout

#endif

