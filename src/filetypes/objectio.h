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
// Copyright (C) 2018 by Tom Lechner
//
//
#ifndef IMAGEIMPORTER_H
#define IMAGEIMPORTER_H

#include <lax/anobject.h>
#include <lax/dump.h>

#include "filefilters.h"


namespace Laidout {


//----------------------------- ImageImporter -------------------------------

class ObjectIO : public FileFilter
{
  protected:
  public:
	ObjectIO() {}
	virtual ~ObjectIO() {}
	virtual const char *FilterClass() { return "object"; }

	 //default laidout format i/o
	//virtual int CanImportAtt() { return false; }
	//virtual int CanExportAtt() { return false; }
	//virtual LaxFiles::Attribute * dump_out_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context) { return att; }
	//virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context) {}
	virtual int Serializable(int what) = 0; //if can serialize to what format. 0 is default Laidout style

	virtual int CanImport(const char *file) = 0; //if null, then return if in theory it can import
	virtual int CanExport(anObject *object) = 0; //if null, then return if in theory it can export
	virtual int Import(const char *file, anObject **object_ret, anObject *context, Laxkit::ErrorLog &log) = 0; //ret # of failing errors
	virtual int Export(const char *file, anObject *object,      anObject *context, Laxkit::ErrorLog &log) = 0; //ret # of failing errors
};


} //namespace Laidout

#endif

