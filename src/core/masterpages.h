//
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
// Copyright (C) 2022 by Tom Lechner
//
//
#ifndef MASTERPAGE_H
#define MASTERPAGE_H


#include <lax/anobject.h>
#include <lax/dump.h>


namespace Laidout {


//----------------------------- MasterPage -------------------------------

class MasterPage : public Page
{
  protected:
	MasterPageSet *set;
	int pagetype;

  public:
	MasterPage();
	virtual ~MasterPage();

	 //i/o
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//----------------------------- MasterPageSet -------------------------------

class MasterPageSet : public Laxkit::anObject
{
  public:
	char *name;
	Impostion *imposition; //base imposition type, determines how many pagetypes

	Laxkit::NumStack<int> pagetypes;
	Laxkit::RefPtrStack<MasterPage> pages;
};


} //namespace Laidout

#endif

