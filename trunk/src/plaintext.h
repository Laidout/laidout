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
// Copyright (C) 2004-2009 by Tom Lechner
//

#ifndef PLAINTEXT_H
#define PLAINTEXT_H


#include <lax/anobject.h>
#include <lax/refcounted.h>
#include <lax/dump.h>

#include <cstdlib>

//------------------------------ FileRef -------------------------------
class FileRef : public Laxkit::anObject, public Laxkit::RefCounted
{
 public:
	char *filename;
	FileRef(const char *file);
	~FileRef() { if (filename) delete[] filename; }
	virtual const char *whattype() { return "FileRef"; }
};

//------------------------------ PlainText -------------------------------

class PlainText : public Laxkit::anObject, public Laxkit::RefCounted, public LaxFiles::DumpUtility
{
 public:
	Laxkit::anObject *owner;
	clock_t lastmodtime;
	clock_t lastfiletime;
	char *thetext;
	char *name;

	PlainText();
	virtual ~PlainText();
	virtual const char *whattype() { return "PlainText"; }
	const char *filename();
	const char *GetText();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


#endif



