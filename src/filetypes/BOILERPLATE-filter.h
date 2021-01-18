//
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
// Copyright (C) 2020 by Tom Lechner
//
#ifndef FILETYPES_BOILERPLATE_H
#define FILETYPES_BOILERPLATE_H


#include "../version.h"
#include "../core/document.h"
#include "filefilters.h"




namespace Laidout {


void installBoilerPlateFilter();

//------------------------------------ BoilerPlateExportFilter ----------------------------------
class BoilerPlateExportFilter : public ExportFilter
{
 protected:
 public:
	BoilerPlateExportFilter();
	virtual ~BoilerPlateExportFilter() {}
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *DefaultExtension() { return "jpg"; }
	virtual const char *Format() { return "BoilerPlate"; }
	virtual const char *Version();
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "image"; }
	virtual ObjectDef *GetObjectDef();

	virtual int Out(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);

	//virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	//virtual int Verify(Laxkit::anObject *context); //preflight checker
};


// //------------------------------------ BoilerPlateImportFilter ----------------------------------
// class BoilerPlateImportFilter : public ImportFilter
// {
//  public:
// 	virtual ~BoilerPlateImportFilter() {}
// 	virtual const char *Author() { return "Laidout"; }
// 	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
// 	virtual const char *DefaultExtension() { return "jpg"; }
// 	virtual const char *Format() { return "BoilerPlate"; }
// 	virtual const char *Version() { return "1.0"; }
// 	virtual const char *VersionName();
// 	virtual const char *FilterClass() { return "document"; }
// 	virtual ObjectDef *GetObjectDef();

// 	virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
	
	
// 	virtual const char *FileType(const char *first100bytes);
// 	virtual int In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents,int contentslen);
// };


} // namespace Laidout

#endif




