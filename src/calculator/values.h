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
// Copyright (C) 2009-2013 by Tom Lechner
//
#ifndef VALUES_H
#define VALUES_H

#include <lax/anobject.h>
#include <lax/refptrstack.h>
#include <lax/vectors.h>
#include <lax/dump.h>
#include <lax/errorlog.h>
#include <lax/colorbase.h>

#include <lax/shortcuts.h>

#include "../fieldplace.h"

namespace Laidout {


typedef int Unit;

/*! For Value and ObjectDef objects */
enum ValueTypes {
	VALUE_Any,        //!< as tag for function parameters
	VALUE_None,       //!< as return value for type checking

	VALUE_Set,        //!< zero or more of various objects
	VALUE_Object,     //!< opaque object for passing around non-Value objects
	VALUE_Int,        //!< integers
	VALUE_Real,       //!< real numbers (doubles)
	VALUE_Number,     //!< Special tag meaning allow int or real.
	VALUE_String,     //!< strings, utf8 based
	VALUE_Fields,     //!< collection of subfields 
	VALUE_Flatvector, //!< two dimensional vector
	VALUE_Spacevector,//!< three dimensional vector
	VALUE_File,       //!< string like object refering to a file on disk
	VALUE_Enum,       //!< One of a list of string like labels, with associated integer value
	VALUE_EnumVal,    //!< these do not exist independently of a VALUE_Enum's ObjectDef
	VALUE_Boolean,    //!< Translatable as 1 for true, or 0 for false
	VALUE_Flags,      //!< *** unimplemented!
	VALUE_Color,      //!< A Color
	VALUE_Date,       //!< *** unimplemented!
	VALUE_Time,       //!< *** unimplemented!
	VALUE_Complex,    //!< *** unimplemented!
	VALUE_Array,      //!< Mathematical matrices, or sets of fixed sizes
	VALUE_Hash,       //!< Basically a set with named values

	VALUE_Variable,   //!< for use in an ObjectDef
	VALUE_Operator,   //!< for use in an ObjectDef
	VALUE_Class,      //!< for use in an ObjectDef
	VALUE_Function,   //!< for use in an ObjectDef
	VALUE_Namespace,  //!< for use in an ObjectDef

	VALUE_Alias,      //!< For BlockInfo, tag to use a name in place of another
	VALUE_Overloaded, //!< For Entry, stores names that happen to be overloaded

	VALUE_LValue,     //!< A name value that you can assign things to.

	VALUE_MaxBuiltIn
};

const char *element_TypeNames(int type);

class ObjectDef;
class Value;
class ValueHash;
class CalcSettings;

typedef Value *(*NewObjectFunc)(ObjectDef *def);
typedef int (*ObjectFunc)(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log);
//typedef int (*ObjectFunc)(const char *func,int len, ValueHash *context, ValueHash *parameters,  CalcSettings *settings,
//						  Value **value_ret, ErrorLog *log);

//--------------------------- OpFuncEvaluator ------------------------------------
enum OperatorDirectionType
{
	OPS_None,
	OPS_LtoR,
	OPS_RtoL,
	OPS_Left,
	OPS_Right,
	OPS_MAX
};
enum OperatorFlags
{
	OPS_Assignment=(1<<0)
};

class OpFuncEvaluator
{
  public:
	virtual int Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings,
				   Value **value_ret, ErrorLog *log) = 0;
};


//--------------------------- FunctionEvaluator ------------------------------------
class FunctionEvaluator
{
  public:
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log) = 0;
};

class SimpleFunctionEvaluator : public FunctionEvaluator
{
  public:
	NewObjectFunc newfunc;
	ObjectFunc function;
	SimpleFunctionEvaluator(ObjectFunc func);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log);
};

//------------------------------ ObjectDef --------------------------------------------


 
//ObjectDef::flags:
#define OBJECTDEF_CAPPED    (1<<0)
#define OBJECTDEF_DUPLICATE (1<<1)
#define OBJECTDEF_ORPHAN    (1<<2)


class ObjectDef : public Laxkit::anObject, public LaxFiles::DumpUtility
{
  public:
	ObjectDef *parent_namespace;
	Laxkit::RefPtrStack<ObjectDef> extendsdefs;

	 //Default callers
	NewObjectFunc newfunc;
	ObjectFunc stylefunc;
	FunctionEvaluator *evaluator;
	OpFuncEvaluator *opevaluator;
	Value *newObject(ObjectDef *def);


	 //descriptive elements
	char *name; //name for interpreter (basically class name)
	char *Name; // Name for dialog label
	char *description; // description
	char *range;
	Laxkit::PtrStack<char> suggestions;
	char *defaultvalue;
	Value *defaultValue; //this has more overhead than just a string, but can be more convenient in some cases.

	 // OBJECTDEF_ORIGINAL
	 // OBJECTDEF_DUPLICATE
	 // OBJECTDEF_ORPHAN  =  is a representation of a composite style, not stored in any manager, and only 1 reference to it exists
	 // OBJECTDEF_CAPPED = cannot push/pop fields
	 // OBJECTDEF_READONLY = cannot modify parts
	 // OBJECTDEF_OPTIONAL
	 // OBJECTDEF_FORCE_NUM_PARAMS
	 // OBJECTDEF_FORCE_TYPES
	unsigned int flags;

	ValueTypes format; // int,real,string,fields,...
	int fieldsformat;  //dynamically assigned to new object types
	Laxkit::RefPtrStack<ObjectDef> *fields;

	ObjectDef();
	ObjectDef(const char *nname,const char *nName, const char *ndesc, Value *newval);
	ObjectDef(ObjectDef *nextends,const char *nname,const char *nName, const char *ndesc,
			ValueTypes fmt,const char *nrange, const char *newdefval,
			Laxkit::RefPtrStack<ObjectDef>  *nfields=NULL,unsigned int fflags=OBJECTDEF_CAPPED,
			NewObjectFunc nnewfunc=0,ObjectFunc nstylefunc=0);
	virtual ~ObjectDef();
	virtual const char *whattype() { return "ObjectDef"; }
	virtual void Clear(int which=~0);


	 // namespace helpers
	virtual Value *newValue(const char *objectdef);
	virtual ObjectDef *FindDef(const char *objectdef, int len=-1, int which=7);
	virtual int AddObjectDef(ObjectDef *def, int absorb);
	virtual int SetVariable(const char *name,Value *v,int absorb);
	virtual int pushVariable(const char *name,const char *nName, const char *ndesc, Value *v,int absorb);
	
	 // helpers to locate fields by name, "blah.3.x"
	virtual int getNumFieldsOfThis();
	virtual ObjectDef *getFieldOfThis(int index);
	virtual int findFieldOfThis(const char *fname,char **next);
	virtual int getNumFields();
	virtual ObjectDef *getField(int index);
	virtual int findfield(const char *fname,char **next); // return index value of fname. assumed top level field
	virtual int findActualDef(int index,ObjectDef **def);
	virtual int getInfo(int index,
						const char **nm=NULL,
						const char **Nm=NULL,
						const char **desc=NULL,
						const char **rng=NULL,
						const char **defv=NULL,
						ValueTypes *fmt=NULL,
						int *objtype=NULL,
						ObjectDef **def_ret=NULL);


	 //-------- ObjectDef creation helper functions ------
	 // The following (push/pop/cap) are convenience functions 
	 // to construct a styledef on the fly
	virtual int Extend(ObjectDef *def);
	virtual ObjectDef *last();
	virtual int pop(int fieldindex);
	virtual int push(ObjectDef *newfield, int absorb=1);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 ValueTypes fformat,const char *nrange, const char *newdefval,
					 unsigned int fflags,
					 NewObjectFunc nnewfunc,
					 ObjectFunc nstylefunc=NULL);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 ValueTypes fformat,const char *nrange, const char *newdefval,
					 Laxkit::RefPtrStack<ObjectDef> *nfields,unsigned int fflags,
					 NewObjectFunc nnewfunc,
					 ObjectFunc nstylefunc);
	virtual int pushEnum(const char *nname,const char *nName,const char *ndesc,
					 const char *newdefval,
					 NewObjectFunc nnewfunc,
					 ObjectFunc nstylefunc,
					 ...);
	virtual int pushEnumValue(const char *str, const char *Str, const char *dsc, int id=-10000000);
	virtual int pushFunction(const char *nname,const char *nName,const char *ndesc,
					 FunctionEvaluator *nfunc,
					 ...);
	virtual int pushOperator(const char *op,int dir,int priority, const char *desc, OpFuncEvaluator *evaluator, int nflags=0);
	virtual int pushParameter(const char *nname,const char *nName,const char *ndesc,
					ValueTypes fformat,const char *nrange, const char *newdefval, Value *defvalue=NULL);

//	virtual int callFunction(const char *field, ValueHash *context, ValueHash *parameters,
//							 Value **value_ret, CalcSettings *settings, ErrorLog *log);

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

typedef ObjectDef StyleDef;


//----------------------------- Value ----------------------------------

class Value : virtual public Laxkit::anObject
{
  protected:
	int modified;
	ObjectDef *objectdef;
  public:
	Value();
	virtual ~Value();
	virtual const char *whattype() { return "Value"; }

	virtual int type() = 0;
	virtual Value *duplicate() = 0;
	virtual int getValueStr(char **buffer,int *len, int oktoreallocate);//subclasses should NOT redefine this
	virtual int getValueStr(char *buffer,int len);//subclasses SHOULD redefine this
	virtual const char *Id();

	//virtual int isValidExt(const char *extstring, FieldPlace *place_ret) = 0; //for assignment checking
	//virtual int assign(const char *extstring,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown
	virtual int assign(FieldExtPlace *ext,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown

	//virtual Value *dereference(const FieldPlace &ext) = 0; // returns a reference if possible, or new. Calling code MUST decrement count.
	virtual Value *dereference(const char *extstring, int len);
	virtual Value *dereference(int index);

 	virtual ObjectDef *makeObjectDef() = 0;
    virtual ObjectDef *GetObjectDef();
    virtual int getNumFields();
    virtual  ObjectDef *FieldInfo(int i);
    virtual const char *FieldName(int i);
    virtual int FieldIndex(const char *name);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//----------------------------- SetValue ----------------------------------
class SetValue : public Value, virtual public FunctionEvaluator
{
  public:
	char *restrictto;
	Laxkit::RefPtrStack<Value> values;

	SetValue(const char *restricted=NULL);
	virtual ~SetValue();
	virtual int Push(Value *v,int absorb, int where=-1);
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Set; }

 	virtual ObjectDef *makeObjectDef();
    virtual int getNumFields();
    virtual  ObjectDef *FieldInfo(int i);
    virtual const char *FieldName(int i);
	virtual Value *dereference(int index);

	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log);
};

//----------------------------- ArrayValue ----------------------------------
class ArrayValue : public SetValue
{
  public:
	int fixed_size;
	char *element_type;

	ArrayValue(const char *elementtype=NULL, int size=0);
	virtual ~ArrayValue();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Array; }

 	virtual ObjectDef *makeObjectDef();

	virtual int Dimensions();
};

//----------------------------- BooleanValue ----------------------------------
class BooleanValue : public Value
{
  public:
	int i;
	BooleanValue(int ii) { i=ii; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Boolean; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
	virtual int assign(FieldExtPlace *ext,Value *v);
};

//----------------------------- IntValue ----------------------------------
class IntValue : public Value
{
  public:
	Unit units;
	long i;
	IntValue(long ii=0) { i=ii; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Int; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
	virtual int assign(FieldExtPlace *ext,Value *v);
};

//----------------------------- DoubleValue ----------------------------------
class DoubleValue : public Value
{
  public:
	Unit units;
	double d;
	DoubleValue(double dd=0) { d=dd; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Real; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
	virtual int assign(FieldExtPlace *ext,Value *v);
};

//----------------------------- FlatvectorValue ----------------------------------
class FlatvectorValue : public Value
{
  public:
	Unit units;
	flatvector v;
	FlatvectorValue() { }
	FlatvectorValue(flatvector vv) { v=vv; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Flatvector; }
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
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
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Spacevector; }
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- StringValue ----------------------------------
class StringValue : public Value
{
  public:
	char *str;
	StringValue(const char *s=NULL, int len=-1);
	virtual ~StringValue() { if (str) delete[] str; }
	virtual int getValueStr(char *buffer,int len);
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
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Object; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- ColorValue ----------------------------------
class ColorValue : public Value
{
  public:
	Laxkit::ColorBase color;
	ColorValue(const char *color);
	//ColorValue(Laxkit::ColorBase &color);
	virtual ~ColorValue();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Color; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet
};

//----------------------------- EnumValue ----------------------------------
class EnumValue : public Value
{
  public:
	int value;
	ObjectDef *enumdef;
	EnumValue(ObjectDef *baseenum, int which);
	virtual ~EnumValue();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Enum; }
 	virtual ObjectDef *makeObjectDef();
};

//----------------------------- FunctionValue ----------------------------------
class FunctionValue : public Value
{
  public:
	char *code;
	FunctionEvaluator *function;

	FunctionValue(const char *ncode, int len);
	virtual ~FunctionValue();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Function; }
 	virtual ObjectDef *makeObjectDef();
};

//----------------------------- FileValue ----------------------------------
class FileValue : public Value
{
  public:
	char *filename;
	FileValue(const char *f=NULL,int len=-1);
	virtual ~FileValue();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_File; }
 	virtual ObjectDef *makeObjectDef() { return NULL; } //built ins do not return a def yet

	virtual int fileType(); //file link, dir link, file, dir, block
	virtual int isLink();
	virtual int Exists();
};

//----------------------------- ValueHash ----------------------------------
class ValueHash : virtual public Laxkit::anObject, virtual public Value, virtual public FunctionEvaluator
{
	Laxkit::PtrStack<char> keys;
	Laxkit::RefPtrStack<Value> values;

  public:
	ValueHash();
	~ValueHash();
	int sorted;

	const char *key(int i);
	Value *value(int i);
	int push(const char *name,int i);
	int push(const char *name,double d);
	int push(const char *name,const char *string);
	int pushObject(const char *name,Laxkit::anObject *obj);
	int push(const char *name,Value *v);
	int push(const char *name,int len,Value *v);
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
	flatvector findFlatvector(const char *name, int which, int *error_ret=NULL);
	Laxkit::anObject *findObject(const char *name, int which=-1, int *error_ret=NULL);

	 //from Value:
	virtual int     type();
	virtual Value    *duplicate();
	virtual int       getValueStr(char *buffer,int len);
 	virtual ObjectDef *makeObjectDef();
	virtual Value     *dereference(int index);
    virtual int        getNumFields();
    virtual ObjectDef  *FieldInfo(int i);
    virtual const char *FieldName(int i);

	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log);
};

//------------------------------- parsing helpers ------------------------------------
ValueHash *MapParameters(ObjectDef *def,ValueHash *rawparams);
double getNumberValue(Value *v, int *isnum);
int extequal(const char *str, int len, const char *field, char **next_ret=NULL);


} // namespace Laidout

#endif

