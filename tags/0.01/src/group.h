//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
			  virtual public Laxkit::SomeData 
			  
{
 protected:
	Laxkit::PtrStack<Laxkit::SomeData> objs;
 public:
	int locked, visible, prints;
	Group() { locked=0; visible=prints=1; }
	virtual ~Group();
	virtual const char *whattype() { return "Group"; }
	virtual Laxkit::SomeData *findobj(Laxkit::SomeData *d,int *n=NULL);
	virtual int findindex(Laxkit::SomeData *d) { return objs.findindex(d); }
	virtual int push(Laxkit::SomeData *obj,int local);
	virtual int pushnodup(Laxkit::SomeData *obj,int local);
	virtual int remove(int i);
	virtual int popp(Laxkit::SomeData *d,int *local=NULL);
	virtual void flush();
	virtual void swap(int i1,int i2) { objs.swap(i1,i2); }
	virtual int slide(int i1,int i2);
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void FindBBox();
	//virtual int contains(SomeData *d,FieldPlace &place);
	//virtual Laxkit::SomeData *getObject(FieldPlace &place,int offset=0);
	//virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, Laxkit::SomeData **d=NULL);
	virtual Laxkit::SomeData *e(int i);
	virtual int n() { return objs.n; }
	virtual Laxkit::anObject *object_e(int i) { return e(i); }
};


#endif
