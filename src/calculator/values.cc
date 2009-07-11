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
// Copyright (C) 2009 by Tom Lechner
//


#include "values.h"
#include <lax/refptrstack.cc>
#include <lax/strmanip.h>
#include <cstdio>

//---------------------------------------- Values --------------------------------------
/*! \class ValueHash
 * \brief Class to aid parsing of functions.
 *
 * Used in LaidoutCalculator.
 */


ValueHash::ValueHash()
	: keys(2)
{}

ValueHash::~ValueHash()
{}

int ValueHash::push(const char *name,int i)
{
	keys.push(newstr(name));
	return values.push(new IntValue(i));
}

int ValueHash::push(const char *name,double d)
{
	keys.push(newstr(name));
	return values.push(new DoubleValue(d));
}

int ValueHash::push(const char *name,const char *value)
{
	keys.push(newstr(name));
	return values.push(new StringValue(value));
}

/*! Increments obj count. */
int ValueHash::push(const char *name,Laxkit::RefCounted *obj)
{
	keys.push(newstr(name));
	return values.push(new ObjectValue(obj));
}

int ValueHash::n()
{ return keys.n; }

void ValueHash::swap(int i1, int i2)
{
	if (i1<0 || i1>keys.n || i2<0 || i2>keys.n) return;
	keys.swap(i1,i2);
	values.swap(i1,i2);
}

//! Return name of key at index i.
const char *ValueHash::key(int i)
{
	if (i<0 || i>=keys.n) return NULL;
	return keys.e[i];
}

//! Rename key at index i.
void ValueHash::renameKey(int i,const char *newname)
{
	if (i<0 || i>=keys.n) return;
	makestr(keys.e[i],newname);
}

//! Return the index corresponding to name, or -1 if not found.
int ValueHash::findIndex(const char *name)
{
	for (int c=0; c<keys.n; c++) {
		if (!strcmp(name,keys.e[c])) return c;
	}
	return -1;
}

Value *ValueHash::find(const char *name)
{
	for (int c=0; c<keys.n; c++) {
		if (!strcmp(name,keys.e[c])) return values.e[c];
	}
	return NULL;
}

long ValueHash::findInt(const char *name, int which)
{
	if (which<0) which=findIndex(name);
	if (which<0) return 0;
	IntValue *i=dynamic_cast<IntValue*>(values.e[which]);
	if (!i) return 0;
	return i->i;
}

double ValueHash::findDouble(const char *name, int which)
{
	if (which<0) which=findIndex(name);
	if (which<0) return 0;
	DoubleValue *d=dynamic_cast<DoubleValue*>(values.e[which]);
	if (!d) return 0;
	return d->d;
}

const char *ValueHash::findString(const char *name, int which)
{
	if (which<0) which=findIndex(name);
	if (which<0) return 0;
	StringValue *s=dynamic_cast<StringValue*>(values.e[which]);
	if (!s) return NULL;
	return s->str;
}

/*! Does not increment count of the object. */
Laxkit::RefCounted *ValueHash::findObject(const char *name, int which)
{
	if (which<0) which=findIndex(name);
	if (which<0) return 0;
	ObjectValue *o=dynamic_cast<ObjectValue*>(values.e[which]);
	if (!o) return NULL;
	return o->object;
}

//------------------------------------- Value ---------------------------------------
/*! \class Value
 * \brief Base class of internal scripting objects.
 *
 * WARNING: This is a temporary implementation. It needs vast improvements to handle
 * units, other object types and object operators in LaidoutCalculator. You should NOT write code that 
 * depends on Value or Value subclasses as they are currently. It is very possible that Value
 * will take over the role of Style, and something like ValueDef will likely replace StyleDef.
 *
 * Used in LaidoutCalculator.
 */

Value::Value()
	: tempstr(NULL),
	  units(0)
{
}

Value::~Value()
{
	if (tempstr) delete[] tempstr;
}

//--------------------------------- IntValue -----------------------------
const char *IntValue::toCChar()
{
	if (!tempstr) tempstr=new char[20];
	sprintf(tempstr,"%ld",i);
	return tempstr;
}

//--------------------------------- DoubleValue -----------------------------
const char *DoubleValue::toCChar()
{
	if (!tempstr) tempstr=new char[30];
	sprintf(tempstr,"%g",d);
	return tempstr;
}

//--------------------------------- StringValue -----------------------------
StringValue::StringValue(const char *s, int len)
{ str=newnstr(s,len); }

const char *StringValue::toCChar()
{
	return str;
}


//--------------------------------- ObjectValue -----------------------------
/*! Will inc count of obj.
 */
ObjectValue::ObjectValue(RefCounted *obj)
{
	object=obj; 
	if (object) object->inc_count();
}

/*! Objects gets count decremented.
 */
ObjectValue::~ObjectValue()
{
	if (object) object->dec_count();
}

const char *ObjectValue::toCChar()
{
	return "(object:TODO!!)";
}


