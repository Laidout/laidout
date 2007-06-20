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
#ifndef FILEFILTERS_H
#define FILEFILTERS_H

#include <lax/anxapp.h>
#include <lax/refcounted.h>
#include "../document.h"



class Document;

//------------------------------- DocumentExportConfig ----------------------------------
class DocumentExportConfig : public Laxkit::anObject, public Laxkit::RefCounted
{
 public:
	int start,end;
	int layout;
	Document *doc;
	char *filename;
	char *tofiles;
	DocumentExportConfig();
	DocumentExportConfig(Document *ndoc, const char *file, const char *to, int l,int s,int e);
	virtual ~DocumentExportConfig();
};

//------------------------------------- FileFilter -----------------------------------
typedef void Plugin; //******must implement plugins!!
class FileFilter : public Laxkit::anObject
{
 public:
	Plugin *plugin; //***which plugin, if any, the filter came from.
	FileFilter() { plugin=NULL; }
	virtual ~FileFilter() {}
	virtual const char *Author() = 0;
	virtual const char *FilterVersion() = 0;
	
	virtual const char *Format() = 0;
	virtual const char *DefaultExtension() = 0;
	virtual const char *Version() = 0;
	virtual const char *VersionName() = 0;
	virtual const char *FilterClass() = 0;

	virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
};


//------------------------------------- FileInputFilter -----------------------------------
class ImportFilter : public FileFilter
{
 public:
	virtual const char *whattype() { return "FileInputFilter"; }
	virtual const char *FileType(const char *first100bytes) = 0;
	virtual int In(const char *file, Laxkit::anObject *context, char **error_ret) = 0;
};


//------------------------------------- FileOutputFilter -----------------------------------
class ExportFilter : public FileFilter
{
 public:
	virtual const char *whattype() { return "FileOutputFilter"; }
	virtual int Out(const char *file, Laxkit::anObject *context, char **error_ret) = 0;
	virtual int Verify(Laxkit::anObject *context) { return 1; } //= 0; //preflight checker
};



#endif


