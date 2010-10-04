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
#ifndef MYSTERYDATA_H
#define MYSTERYDATA_H

#include <lax/interfaces/rectinterface.h>
#include <lax/attributes.h>


//--------------------------------------- MysteryData ---------------------------------
class MysteryData : public LaxInterfaces::SomeData
{
 public:
	char *importer;
	char *name;
	long nativeid;
	int numpoints;
	flatpoint *outline;
	LaxFiles::Attribute *attributes;
	MysteryData(const char *gen=NULL);
	virtual ~MysteryData();
	virtual const char *whattype() { return "MysteryData"; }
	virtual int installAtts(LaxFiles::Attribute *att);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

#endif


