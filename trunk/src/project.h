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
#ifndef PROJECT_H
#define PROJECT_H

#include <lax/refptrstack.h>
#include "document.h"
#include "papersizes.h"
#include "plaintext.h"


namespace Laidout {


//------------------------- ProjDocument ------------------------------------
class ProjDocument
{
 public:
	Document *doc;
	char *filename;
	char *name;
	int is_in_project;
	ProjDocument(Document *ndoc,char *file,char *nme);
	~ProjDocument();
};

//------------------------- Project ------------------------------------
class Project : public LaxFiles::DumpUtility
{
 public:
	char *name,*filename,*dir;
	double defaultdpi;
	//default units
	//default color mode

	Laxkit::PtrStack<ProjDocument> docs;
	Group limbos;
	Laxkit::RefPtrStack<PaperGroup> papergroups;
	Laxkit::RefPtrStack<PlainText> textobjects;

	LaxFiles::Attribute iohints;

	//StyleManager styles;

	Project();
	virtual ~Project();

	virtual int initDirs();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual int Load(const char *file,ErrorLog &log);
	virtual int Save(ErrorLog &log);
	virtual int clear();

	virtual int Push(Document *doc);
	virtual int Pop(Document *doc);

	virtual Document *Find(const char *name, int howmatch);
	virtual int valid();
};


} // namespace Laidout

#endif

