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
#ifndef GENERICSTYLE_H
#define GENERICSTYLE_H

#include "styles.h"

class GenericStyle : public Style
{
 protected:
	 Laxkit::PtrStack<FieldNode> fields; // this usually holds the actual values
 public:
	GenericStyle() { basedon=NULL; styledef=NULL; stylename=NULL; }
	GenericStyle(StyleDef *sdef,Style *bsdon,const char *nstn);
	virtual ~GenericStyle();
	virtual Style *duplicate(Style *s=NULL);

	virtual void *dereference(const char *extstr,int copy);
	virtual int set(FieldMask *field,Value *val,FieldMask *mask_ret)=0; 
	virtual int set(const char *ext,Value *val,FieldMask *mask_ret)=0;
	
	virtual FieldMask set(FieldMask *field,Value *val); 
	virtual FieldMask set(const char *ext, Value *val);
	virtual FieldMask set(Fieldmask *field,const char *val);
	virtual FieldMask set(const char *ext, const char *val);
	virtual FieldMask set(Fieldmask *field,int val);
	virtual FieldMask set(const char *ext, int val);
	virtual FieldMask set(Fieldmask *field,double val);
	virtual FieldMask set(const char *ext, double val);
	virtual Value *getvalue(FieldMask *field);
	virtual Value *getvalue(const cahr *ext);
	virtual int getint(FieldMask *field);
	virtual int getint(const char *ext);
	virtual double getdouble(FieldMask *field);
	virtual double getdouble(const char *ext);
	virtual char *getstring(FieldMask *field);
	virtual char *getstring(const char *ext);
};

#endif

