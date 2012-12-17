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
#include <lax/vectors.h>
#include <lax/dump.h>
#include <lax/errorlog.h>



namespace Laidout {


typedef int Unit;

enum ValueTypes {
	VALUE_None,
	VALUE_Set,
	VALUE_Object,
	VALUE_Fields,
	VALUE_Int,
	VALUE_Real,
	VALUE_String,
	VALUE_Flatvector,
	VALUE_Spacevector,
	VALUE_Array, // *** unimplemented!
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

//------------------------------ ObjectDef --------------------------------------------

 // for ObjectDef::format
enum ElementType {
	Element_Any,
	Element_None,
	Element_Set, //for sets, the range value in the ObjectDef restricts the set to that type
	Element_Object,
	Element_Int,
	Element_Real,
	Element_String,
	Element_Fields, 
	Element_Flatvector, 
	Element_Spacevector, 
	Element_File,
	Element_Flag,
	Element_Enum, //if the def has a function, then it is a dynamic enum
	Element_EnumVal,
	Element_Color,
	Element_Date,
	Element_Time,
	Element_Boolean,
	Element_Complex,
	Element_Function,

	Element_MaxBuiltinFormatValue
};
const char *element_TypeNames(int type);
		

class ObjectDef;
class Style;
class Value;
class ValueHash;
typedef Style *(*NewStyleFunc)(ObjectDef *def);
typedef int (*StyleFunc)(ValueHash *context, ValueHash *parameters,
							 Value **value_ret, ErrorLog &log);
 

#define OBJECTDEF_CAPPED    1
#define OBJECTDEF_DUPLICATE 2
#define OBJECTDEF_ORPHAN    4


class ObjectDef : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	char *extends;
	ObjectDef *extendsdef;
	NewStyleFunc newfunc;
	StyleFunc stylefunc;
	Style *newStyle(ObjectDef *def) { if (newfunc) return newfunc(this); return NULL; }

	char *name; //name for interpreter (basically class name)
	char *Name; // Name for dialog label
	char *description; // description
	char *range;
	char *defaultvalue;
	Laxkit::PtrStack<char> suggestions;
	
	 // OBJECTDEF_ORIGINAL
	 // OBJECTDEF_DUPLICATE
	 // OBJECTDEF_ORPHAN  =  is a representation of a composite style, not stored in any manager, and only 1 reference to it exists
	 // OBJECTDEF_CAPPED = cannot push/pop fields
	 // OBJECTDEF_READONLY = cannot modify parts of the styledef
	unsigned int flags;

	ElementType format; // int,real,string,fields,...
	int fieldsformat;  //dynamically assigned to new object types
	Laxkit::RefPtrStack<ObjectDef> *fields; //might be NULL, any fields are assumed to not be local to the stack.
	
	ObjectDef();
	ObjectDef(const char *nextends,const char *nname,const char *nName, const char *ndesc,
			ElementType fmt,const char *nrange, const char *newdefval,
			Laxkit::RefPtrStack<ObjectDef>  *nfields=NULL,unsigned int fflags=OBJECTDEF_CAPPED,
			NewStyleFunc nnewfunc=0,StyleFunc nstylefunc=0);
	virtual ~ObjectDef();
	virtual const char *whattype() { return "ObjectDef"; }

	 // helpers to locate fields by name, "blah.3.x"
	virtual int getNumFields();
	virtual int findfield(char *fname,char **next); // return index value of fname. assumed top level field
	virtual int findActualDef(int index,ObjectDef **def);
	virtual ObjectDef *getField(int index);
	virtual int getInfo(int index,
						const char **nm=NULL,
						const char **Nm=NULL,
						const char **desc=NULL,
						const char **rng=NULL,
						const char **defv=NULL,
						ElementType *fmt=NULL,
						int *objtype=NULL,
						ObjectDef **def_ret=NULL);
 	//int *getfields(const char *extstr); // returns 0 terminated list of indices: "1.4.23+ -> { 1,4,23,0 }

	 //-------- ObjectDef creation helper functions ------
	 // The following (push/pop/cap) are convenience functions 
	 // to construct a styledef on the fly
	virtual int pushEnumValue(const char *str, const char *Str, const char *dsc);
	virtual int pushEnum(const char *nname,const char *nName,const char *ndesc,
					 const char *newdefval,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc,
					 ...);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 ElementType fformat,const char *nrange, const char *newdefval,
					 unsigned int fflags,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc=NULL);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 ElementType fformat,const char *nrange, const char *newdefval,
					 Laxkit::RefPtrStack<ObjectDef> *nfields,unsigned int fflags,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc=NULL);
	virtual int pushParameter(const char *nname,const char *nName,const char *ndesc,
					ElementType fformat,const char *nrange, const char *newdefval);
	virtual int push(ObjectDef *newfield);
	virtual int pop(int fieldindex);

//	virtual int callFunction(const char *field, ValueHash *context, ValueHash *parameters,
//							 Value **value_ret, char **message_ret);

	 // cap prevents accidental further adding/removing fields to a styledef
	 // that is being constructed
	virtual void cap(int y=1) { if (y) flags|=OBJECTDEF_CAPPED; else flags&=~OBJECTDEF_CAPPED; }
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

typedef ObjectDef StyleDef;


//----------------------------- Value ----------------------------------

class Value : virtual public Laxkit::anObject
{
 protected:
	char *tempstr; //cached string representation
	int modified; //whether tempstr needs to be updated
	ObjectDef *objectdef;
 public:
	Value();
	virtual ~Value();
	virtual const char *whattype() { return "Value"; }
	virtual const char *CChar();

	virtual const char *toCChar() = 0;
	virtual Value *duplicate() = 0;
	virtual int type() = 0;
	virtual const char *Id();

	//virtual int isValidExt(const char *extstring, FieldPlace *place_ret) = 0; //for assignment checking
	//virtual int assign(const char *extstring,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown
	//virtual int assign(FieldPlace &ext,Value *v) = 0; //return 1 for success, 2 for success, but other contents changed too, -1 for unknown
	//virtual Value *dereference(const char *extstring) = 0; // returns a reference if possible, or new. Calling code MUST decrement count.

 	virtual ObjectDef *makeObjectDef() = 0;
    virtual ObjectDef *GetObjectDef();
    virtual int getNumFields();
    virtual  ObjectDef *FieldInfo(int i);
    virtual const char *FieldName(int i);
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

 	virtual ObjectDef *makeObjectDef();
    virtual int getNumFields();
    virtual  ObjectDef *FieldInfo(int i);
    virtual const char *FieldName(int i);
};

//----------------------------- IntValue ----------------------------------
class IntValue : public Value
{
 public:
	Unit units;
	long i;
	IntValue(long ii=0) { i=ii; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Int; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- DoubleValue ----------------------------------
class DoubleValue : public Value
{
 public:
	Unit units;
	double d;
	DoubleValue(double dd=0) { d=dd; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Real; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- FlatvectorValue ----------------------------------
class FlatvectorValue : public Value
{
 public:
	Unit units;
	flatvector v;
	FlatvectorValue() { }
	FlatvectorValue(flatvector vv) { v=vv; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Flatvector; }
	virtual Value *dereference(const char *extstring);
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- SpacevectorValue ----------------------------------
class SpacevectorValue : public Value
{
 public:
	Unit units;
	spacevector v;
	SpacevectorValue() { }
	SpacevectorValue(spacevector vv) { v=vv; }
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Spacevector; }
	virtual Value *dereference(const char *extstring);
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
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
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- ObjectValue ----------------------------------
class ObjectValue : public Value
{
 public:
	Laxkit::anObject *object;
	ObjectValue(anObject *obj=NULL);
	virtual ~ObjectValue();
	virtual const char *toCChar();
	virtual Value *duplicate();
	virtual int type() { return VALUE_Object; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- ValueHash ----------------------------------
class ValueHash : public Laxkit::anObject
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
	int pushObject(const char *name,Laxkit::anObject *obj);
	int push(const char *name,Value *v);
	int remove(int i);
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
	Laxkit::anObject *findObject(const char *name, int which=-1, int *error_ret=NULL);

};


} // namespace Laidout

#endif

