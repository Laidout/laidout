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
#ifndef VALUES_H
#define VALUES_H

#include <lax/refptrstack.h>
#include <lax/refcounted.h>


typedef int Unit;

enum ValueTypes {
	VALUE_None,
	VALUE_Int,
	VALUE_Double,
	VALUE_String,
	VALUE_Object,
	VALUE_Set
};

//----------------------------- Value ----------------------------------
class Value : public Laxkit::RefCounted
{
 protected:
	char *tempstr;
 public:
	Unit units;
	Value();
	virtual ~Value();
	virtual const char *toCChar() = 0;
	const char *whattype() { return "Value"; }
	virtual int type() = 0;
};

//----------------------------- SetValue ----------------------------------
//class SetValue : public Value
//{
// public:
//	RefPtrStack<Value> values;
//	IntValue(Value *v) { values.push(v); }
//	virtual const char *toCChar();
//	virtual int type() { return VALUE_Set; }
//};

//----------------------------- IntValue ----------------------------------
class IntValue : public Value
{
 public:
	long i;
	IntValue(long ii=0) { i=ii; }
	virtual const char *toCChar();
	virtual int type() { return VALUE_Int; }
};

//----------------------------- DoubleValue ----------------------------------
class DoubleValue : public Value
{
 public:
	double d;
	DoubleValue(double dd=0) { d=dd; }
	virtual const char *toCChar();
	virtual int type() { return VALUE_Double; }
};

//----------------------------- StringValue ----------------------------------
class StringValue : public Value
{
 public:
	char *str;
	StringValue(const char *s=NULL, int len=-1);
	virtual ~StringValue() { if (str) delete[] str; }
	virtual const char *toCChar();
	virtual int type() { return VALUE_String; }
};

//----------------------------- ObjectValue ----------------------------------
class ObjectValue : public Value
{
 public:
	Laxkit::RefCounted *object;
	ObjectValue(RefCounted *obj=NULL);
	virtual ~ObjectValue();
	virtual const char *toCChar();
	virtual int type() { return VALUE_Object; }
};

//----------------------------- ValueHash ----------------------------------
class ValueHash
{
	Laxkit::PtrStack<char> keys;
	Laxkit::RefPtrStack<Value> values;

 public:
	ValueHash();
	~ValueHash();

	int push(const char *name,int i);
	int push(const char *name,double d);
	int push(const char *name,const char *string);
	int push(const char *name,Laxkit::RefCounted *obj);
	void swap(int i1, int i2);
	void renameKey(int i,const char *newname);
	const char *key(int i);

	int n();
	Value *find(const char *name);
	int findIndex(const char *name);
	long findInt(const char *name, int which=-1);
	double findDouble(const char *name, int which=-1);
	const char *findString(const char *name, int which=-1);
	Laxkit::RefCounted *findObject(const char *name, int which=-1);
};


#endif

