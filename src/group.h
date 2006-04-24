//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef GROUP_H
#define GROUP_H

#include <lax/interfaces/somedata.h>
#include "objectcontainer.h"

//------------------------------ Group ----------------------------

class Group : public ObjectContainer,
			  virtual public LaxInterfaces::SomeData 
			  
{
 protected:
	Laxkit::PtrStack<LaxInterfaces::SomeData> objs;
 public:
	int locked, visible, prints;
	Group() { locked=0; visible=prints=1; }
	virtual ~Group();
	virtual const char *whattype() { return "Group"; }
	virtual LaxInterfaces::SomeData *findobj(LaxInterfaces::SomeData *d,int *n=NULL);
	virtual int findindex(LaxInterfaces::SomeData *d) { return objs.findindex(d); }
	virtual int push(LaxInterfaces::SomeData *obj,int local);
	virtual int pushnodup(LaxInterfaces::SomeData *obj,int local);
	virtual int remove(int i);
	virtual int popp(LaxInterfaces::SomeData *d,int *local=NULL);
	virtual void flush();
	virtual void swap(int i1,int i2) { objs.swap(i1,i2); }
	virtual int slide(int i1,int i2);
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void FindBBox();
	//virtual int contains(SomeData *d,FieldPlace &place);
	//virtual LaxInterfaces::SomeData *getObject(FieldPlace &place,int offset=0);
	//virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, LaxInterfaces::SomeData **d=NULL);
	virtual LaxInterfaces::SomeData *e(int i);
	virtual int n() { return objs.n; }
	virtual Laxkit::anObject *object_e(int i) { return e(i); }
};


#endif

