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
// Copyright (C) 2004-2013 by Tom Lechner
//

#ifndef PLAINTEXT_H
#define PLAINTEXT_H


#include <lax/anobject.h>
#include <lax/dump.h>

#include <cstdlib>


namespace Laidout {


//------------------------------ FileRef -------------------------------
class FileRef : public Laxkit::anObject
{
 public:
	char *filename;
	FileRef(const char *file);
	virtual ~FileRef() { if (filename) delete[] filename; }
	virtual const char *whattype() { return "FileRef"; }
};

//------------------------------ PlainText -------------------------------

enum PlainTextType {
	TEXT_Note,
	TEXT_Component,
	TEXT_Script,
	TEXT_Temporary
};

class PlainText : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	Laxkit::anObject *owner;
	int texttype, textsubtype;
	clock_t lastmodtime;
	clock_t lastfiletime;
	char *thetext;
	char *name;
	char *filename;

	PlainText();
	PlainText(const char *newtext);
	virtual ~PlainText();
	virtual const char *whattype() { return "PlainText"; }
	virtual const char *Filename();
	virtual const char *Filename(const char *newfile);
	virtual int SaveText();
	virtual int SetText(const char *newtext);
	virtual const char *GetText();
	virtual int LoadFromFile(const char *fname);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


} //namespace Laidout

#endif



