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
#include "../styles.h"
#include <lax/refptrstack.cc>
#include <lax/strmanip.h>
#include <cstdio>
#include <iostream>

#define DBG
using namespace std;

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
{
	DBG values.flush(); //this should happen automatically anyway
}

int ValueHash::push(const char *name,int i)
{
	keys.push(newstr(name));
	Value *v=new IntValue(i);
	int c=values.push(v);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,double d)
{
	keys.push(newstr(name));
	Value *v=new DoubleValue(d);
	int c=values.push(v);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,const char *value)
{
	keys.push(newstr(name));
	Value *v=new StringValue(value);
	int c=values.push(v);
	v->dec_count();
	return c;
}

//! Create an ObjectValue with obj, and push.
/*! Increments obj count. */
int ValueHash::pushObject(const char *name,Laxkit::RefCounted *obj)
{
	keys.push(newstr(name));
	Value *v=new ObjectValue(obj);
	int c=values.push(v);
	v->dec_count();
	return c;
}

/*! Increments count on v. */
int ValueHash::push(const char *name,Value *v)
{
	keys.push(newstr(name));
	return values.push(v);
}

int ValueHash::n()
{ return keys.n; }

//! Return the Value with index i, or NULL if i is out of bounds.
Value *ValueHash::e(int i)
{
	if (i<0 || i>keys.n) return NULL;
	return values.e[i];
}

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

//! Return the value at index i.
Value *ValueHash::value(int i)
{
	if (i<0 || i>=keys.n) return NULL;
	return values.e[i];
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

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not an IntValue, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * No cast conversion is done between int and real.
 */
long ValueHash::findInt(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	IntValue *i=dynamic_cast<IntValue*>(values.e[which]);
	if (!i) { if (error_ret) *error_ret=2; return 0; }
	if (error_ret) *error_ret=0;
	return i->i;
}

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a DoubleValue, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * No cast conversion is done between int and real. Use findIntOrDouble() if
 * you don't care about the difference.
 */
double ValueHash::findDouble(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	DoubleValue *d=dynamic_cast<DoubleValue*>(values.e[which]);
	if (!d) { if (error_ret) *error_ret=2; return 0; }
	if (error_ret) *error_ret=0;
	return d->d;
}

//! Return a double value from an IntValue or a DoubleValue.
/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a DoubleValue, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * No cast conversion is done between int and real.
 */
double ValueHash::findIntOrDouble(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }

	DoubleValue *d=dynamic_cast<DoubleValue*>(values.e[which]);
	if (d) {
		if (error_ret) *error_ret=0;
		return d->d;
	}
	IntValue *i=dynamic_cast<IntValue*>(values.e[which]);
	if (i) {
		if (error_ret) *error_ret=0;
		return i->i;
	}

	if (error_ret) *error_ret=2; //for not found
	return 0;
}

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a StringValue, then sets *error_ret=2.
 * Otherwise set to 0.
 */
const char *ValueHash::findString(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	StringValue *s=dynamic_cast<StringValue*>(values.e[which]);
	if (!s) { if (error_ret) *error_ret=2; return NULL; }
	if (error_ret) *error_ret=0;
	return s->str;
}

/*! Does not increment count of the object.
 *  If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not an ObjectValue, then sets *error_ret=2.
 * Otherwise set to 0.
 */
Laxkit::RefCounted *ValueHash::findObject(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	ObjectValue *o=dynamic_cast<ObjectValue*>(values.e[which]);
	if (!o) { if (error_ret) *error_ret=2; return NULL; }
	if (error_ret) *error_ret=0;
	return o->object;
}

//------------------------------------- Value ---------------------------------------
/*! \class Value
 * \brief Base class of internal scripting objects.
 *
 * \todo WARNING: This is a temporary implementation. It needs vast improvements to handle
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

//----------------------------- SetValue ----------------------------------
/*! \class SetValue
 */

//! Push val, which increments its count.
/*! Return 0 for success or nonzero for error.
 */
int SetValue::Push(Value *v)
{
	if (!v) return 1;
	if (values.push(v)>=0) return 0;
	return 1;
}

const char *SetValue::toCChar()
{
	makestr(tempstr,"{");
	for (int c=0; c<values.n; c++) {
		appendstr(tempstr,values.e[c]->toCChar());
		if (c!=values.n-1) appendstr(tempstr,",");
	}
	appendstr(tempstr,"}");
	return tempstr;
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
//! Create a string value with the first len characters of s.
/*! If len<=0, then use strlen(s).
 */
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
	DBG cerr <<"ObjectValue creation.."<<endl;
	object=obj; 
	if (object) object->inc_count();
}

/*! Objects gets count decremented.
 */
ObjectValue::~ObjectValue()
{
	DBG cerr <<"ObjectValue destructor.."<<endl;
	if (object) object->dec_count();
}

const char *ObjectValue::toCChar()
{
	if (!object) return NULL;
	if (dynamic_cast<Style*>(object)) {
		Style *s=dynamic_cast<Style*>(object);
		if (s->Stylename()) return s->Stylename();
		return s->whattype();
	}
	return "object(TODO!!)";
}


