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

#include "document.h"

//------------------------- Project ------------------------------------

class Project : public LaxFiles::DumpUtility
{
 public:
	char *name,*filename;
	Laxkit::PtrStack<Document> docs;
	Group limbos;
	
	//StyleManager styles;
	//Page scratchboard;
	//Laxkit::PtrStack<char> project_notes;

	Project();
	virtual ~Project();

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
	virtual int Load(const char *file,char **error_ret);
	virtual int Save(char **error_ret);
	virtual int clear();
};

#endif

