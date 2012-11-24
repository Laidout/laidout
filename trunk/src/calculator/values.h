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
// Copyright (C) 2009-2012 by Tom Lechner
//
#ifndef VALUES_H
#define VALUES_H

#include <lax/anobject.h>
#include <lax/refptrstack.h>
#include <lax/refcounted.h>
#include <lax/vectors.h>


typedef int Unit;

enum ValueTypes {
	VALUE_None,
	VALUE_Set,
	VALUE_Object,
	VALUE_Int,
	VALUE_Double,
	VALUE_String,
	VALUE_Flatvector,
	VALUE_Spacevector,
	VALUE_File, // *** unimplemented!
	VALUE_Flags, // *** unimplemented!
	VALUE_Enum, // *** unimplemented!
	VALUE_EnumVal, // *** unimplemented!
	VALUE_Color, // *** unimplemented!
	VALUE_Date, // *** unimplemented!
	VALUE_Time, // *** unimplemented!
	VALUE_Boolean, // *** unimplemented!
	VALUE_Complex, // *** unimplemented!
	VALUE_Function, // *** unimplemented!

	VALUE_MaxBuiltIn
};

//----------------------------- Value ----------------------------------
class Value : public Laxkit::RefCounted
{
 protected:
	char *tempstr; //cached string representation
	int modified; //whether tempstr needs to be updated
 public:
	Unit units;
	Value();
	virtual ~Value();
	virtual const char *whattype() { return "Value"; }
	virtual const char *CChar();

	virtual const char *toCChar() = 0;
	virtual Value *duplicate() = 0;
	virtual int type() = 0;
};

//----------------------------- SetValue ----------------------------------
class SetValue : public Value
{
 public:
	Laxkit::RefPtrStack<Value> values;
	virtual int Push(Value *v);
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Set; }
};

//----------------------------- IntValue ----------------------------------
class IntValue : public Value
{
 public:
	long i;
	IntValue(long ii=0) { i=ii; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Int; }
};

//----------------------------- DoubleValue ----------------------------------
class DoubleValue : public Value
{
 public:
	double d;
	DoubleValue(double dd=0) { d=dd; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Double; }
};

//----------------------------- FlatvectorValue ----------------------------------
class FlatvectorValue : public Value
{
 public:
	flatvector v;
	FlatvectorValue() { }
	FlatvectorValue(flatvector vv) { v=vv; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Flatvector; }
};

//----------------------------- SpacevectorValue ----------------------------------
class SpacevectorValue : public Value
{
 public:
	spacevector v;
	SpacevectorValue() { }
	SpacevectorValue(spacevector vv) { v=vv; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Spacevector; }
};

//----------------------------- StringValue ----------------------------------
class StringValue : public Value
{
 public:
	char *str;
	StringValue(const char *s=NULL, int len=-1);
	virtual ~StringValue() { if (str) delete[] str; }
	virtual const char *toCChar();
	virtual Value *duplicate();
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
	virtual Value *duplicate();
	virtual int type() { return VALUE_Object; }
};

//----------------------------- ValueHash ----------------------------------
class ValueHash : public Laxkit::anObject, public Laxkit::RefCounted
{
	Laxkit::PtrStack<char> keys;
	Laxkit::RefPtrStack<Value> values;

 public:
	ValueHash();
	~ValueHash();

	const char *key(int i);
	Value *value(int i);
	int push(const char *name,int i);
	int push(const char *name,double d);
	int push(const char *name,const char *string);
	int pushObject(const char *name,Laxkit::RefCounted *obj);
	int push(const char *name,Value *v);
	void swap(int i1, int i2);

	void renameKey(int i,const char *newname);
	int set(const char *key, Value *newv);
	int set(int which, Value *newv);

	int n();
	Value *e(int i);
	Value *find(const char *name);
	int findIndex(const char *name,int len=-1);
	long findInt(const char *name, int which=-1, int *error_ret=NULL);
	double findDouble(const char *name, int which=-1, int *error_ret=NULL);
	double findIntOrDouble(const char *name, int which=-1, int *error_ret=NULL);
	const char *findString(const char *name, int which=-1, int *error_ret=NULL);
	Laxkit::RefCounted *findObject(const char *name, int which=-1, int *error_ret=NULL);

};


#endif

