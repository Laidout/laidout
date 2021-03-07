//
//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2009-2014 by Tom Lechner
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

#include "../core/fieldplace.h"

#include <ctime>


namespace Laidout {


typedef int Unit;

/*! For Value and ObjectDef objects */
enum ValueTypes {
	VALUE_None=0,       //!< as return value for type checking
	VALUE_Any,        //!< as tag for function parameters

	VALUE_Set,        //!< zero or more of various objects
	VALUE_Object,     //!< opaque object for passing around non-Value objects
	VALUE_Int,        //!< integers
	VALUE_Real,       //!< real numbers (doubles)
	VALUE_Number,     //!< Special tag meaning allow int or real.
	VALUE_String,     //!< strings, utf8 based
	VALUE_Bytes,      //!< raw data
	VALUE_Fields,     //!< collection of subfields
	VALUE_Flatvector, //!< two dimensional vector
	VALUE_Spacevector,//!< three dimensional vector
	VALUE_Quaternion, //!< four dimensional vector
	VALUE_File,       //!< string like object refering to a file on disk
	VALUE_FileSave,   //!< Same as VALUE_File, but hinted to be for saving a file
	VALUE_FileLoad,   //!< Same as VALUE_File, but hinted to be for loading a file
	VALUE_Enum,       //!< One of a list of string like labels, with associated integer value
	VALUE_EnumVal,    //!< these do not exist independently of a VALUE_Enum's ObjectDef
	VALUE_Boolean,    //!< Translatable as 1 for true, or 0 for false
	VALUE_Array,      //!< Mathematical matrices, or sets of fixed sizes
	VALUE_Hash,       //!< Basically a set with named values

	VALUE_Image,      //!< see ImageValue in imagevalue.h
	VALUE_Flags,      //!< *** unimplemented!
	VALUE_Color,      //!< A Color
	VALUE_Date,       //!< *** unimplemented!
	VALUE_Time,       //!< *** unimplemented!
	VALUE_Complex,    //!< *** unimplemented!
	VALUE_Font,       //!< see FontValue in fontvalue.h

	VALUE_Variable,   //!< for use in an ObjectDef
	VALUE_Operator,   //!< for use in an ObjectDef
	VALUE_Class,      //!< for use in an ObjectDef
	VALUE_Function,   //!< for use in an ObjectDef
	VALUE_Namespace,  //!< for use in an ObjectDef

	VALUE_Alias,      //!< For BlockInfo, tag to use a name in place of another
	VALUE_Overloaded, //!< For Entry, stores names that happen to be overloaded
	VALUE_Dummy,      //!< Internal Entry type to aid diagnostic parsing

	VALUE_LValue,     //!< A name value that you can assign things to.

	 //field action hints
	VALUE_Button,     //!< Hint for a field action

	VALUE_MaxBuiltIn
};

const char *element_TypeNames(int type);

class ObjectDef;
class Value;
class ValueHash;
class CalcSettings;

typedef Value *(*NewObjectFunc)();

//return 0 for success, -1 incompatible params, 1 error with params
typedef int (*ObjectFunc)(ValueHash *context, ValueHash *parameters, Value **value_ret, Laxkit::ErrorLog &log);
//typedef int (*ObjectFunc)(const char *func,int len, ValueHash *context, ValueHash *parameters,  CalcSettings *settings,
//						  Value **value_ret, Laxkit::ErrorLog *log);

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
				   Value **value_ret, Laxkit::ErrorLog *log) = 0;
};


//--------------------------- FunctionEvaluator ------------------------------------
class FunctionEvaluator
{
  public:
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log) = 0;
};

class SimpleFunctionEvaluator : public FunctionEvaluator
{
  public:
	NewObjectFunc newfunc;
	ObjectFunc function;
	SimpleFunctionEvaluator(ObjectFunc func);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};


//------------------------------ ObjectDef --------------------------------------------


enum ObjectDefOut {
	DEFOUT_Indented     = 0,
	DEFOUT_Script       = (-2),
	DEFOUT_HumanSummary = (-3),
	DEFOUT_CPP          = (-4),
	DEFOUT_JSON         = (-5),
	DEFOUT_MAX
};

enum ObjectDefFlags {
	OBJECTDEF_CAPPED    = (1<<0), //!< Forbid adding new fields to a variable
	OBJECTDEF_DUPLICATE = (1<<1),
	OBJECTDEF_ORPHAN    = (1<<2),
	OBJECTDEF_ISSET     = (1<<3),
	OBJECTDEF_READONLY  = (1<<4), //!< An override making something from a baseclass readonly in the derived class
	OBJECTDEF_CONST     = (1<<5), //!< Function does not change anything internal
 // OBJECTDEF_OPTIONAL
 // OBJECTDEF_FORCE_NUM_PARAMS
 // OBJECTDEF_FORCE_TYPES
	OBJECTDEF_MAX
};

class ObjectDef : public Laxkit::anObject, public LaxFiles::DumpUtility
{
  public:
	ObjectDef *parent_namespace;
	Laxkit::RefPtrStack<ObjectDef> extendsdefs;

	 //Default callers
	NewObjectFunc newfunc; //bare creation func
	ObjectFunc stylefunc; //constructor func with parameters
	FunctionEvaluator *evaluator;
	OpFuncEvaluator *opevaluator;
	Value *newObject(ObjectDef *def);

	 //descriptive elements
	char *name; //name for interpreter (basically class name)
	char *Name; // Name for dialog label
	char *description; // description
	char *range;
	Laxkit::PtrStack<char> suggestions;
	Laxkit::PtrStack<char> aliases; //TODO: any listed here are synonymous with *this
	char *defaultvalue;
	Value *defaultValue; //this has more overhead than just a string, but can be more convenient in some cases.

	unsigned int flags; //!< see ObjectDefFlags

	ValueTypes format;    //built in type: int,real,string,... fields means it is not really a builtin type, implies use of fieldsformat
	char *format_str;     //for convenience, this is a string name of this->format
	int islist;           //if this variable is actually an array of the given type
	int fieldsformat;     //dynamically assigned to new object types
	ObjectDef *fieldsdef; //a def associated with fieldsformat. Overrides fields.
	Laxkit::RefPtrStack<ObjectDef> *fields;

	char *uihint;
	Laxkit::anObject *extrainfo;

	ObjectDef();
	ObjectDef(const char *nname,const char *nName, const char *ndesc, Value *newval, const char *type, unsigned int fflags);
	ObjectDef(ObjectDef *nextends,const char *nname,const char *nName, const char *ndesc,
			const char *fmt,const char *nrange, const char *newdefval,
			Laxkit::RefPtrStack<ObjectDef>  *nfields=NULL,unsigned int fflags=OBJECTDEF_CAPPED,
			NewObjectFunc nnewfunc=0,ObjectFunc nstylefunc=0);
	ObjectDef(ObjectDef *nextends,const char *nname,const char *nName, const char *ndesc,
			NewObjectFunc nnewfunc, ObjectFunc nstylefunc);
	virtual ~ObjectDef();
	virtual const char *whattype() { return "ObjectDef"; }
	virtual void Clear(int which=~0);


	 // namespace helpers
	virtual Value *newValue(const char *objectdef);
	virtual ObjectDef *FindDef(const char *objectdef, int len=-1, int which=0);
	virtual int AddObjectDef(ObjectDef *def, int absorb);
	virtual int SetVariable(const char *name,Value *v,int absorb);
	virtual int pushVariable(const char *name,const char *nName, const char *ndesc, const char *type, unsigned int fflags, Value *v,int absorb);

	 // helpers to locate fields by name, "blah.3.x"
	virtual int getNumFieldsOfThis();
	virtual ObjectDef *getFieldOfThis(int index);
	virtual int findFieldOfThis(const char *fname,char **next);
	virtual int getNumFields();
	virtual int getNumEnumFields();
	virtual int isData();
	virtual ObjectDef *getField(int index);
	virtual int findfield(const char *fname,char **next); // return index value of fname. assumed top level field
	virtual int findActualDef(int index,ObjectDef **def);
	virtual int getEnumInfo(int index,
						const char **nm=NULL,
						const char **Nm=NULL,
						const char **desc=NULL,
						int *id=NULL);
	virtual int getInfo(int index,
						const char **nm=NULL,
						const char **Nm=NULL,
						const char **desc=NULL,
						const char **rng=NULL,
						const char **defv=NULL,
						ValueTypes *fmt=NULL,
						const char **objtype=NULL,
						ObjectDef **def_ret=NULL);


	 //-------- ObjectDef creation helper functions ------
	 // The following (push/pop/cap) are convenience functions
	 // to construct a styledef on the fly
	virtual int Extend(ObjectDef *def);
	virtual int SetType(const char *type);
	virtual ObjectDef *last();
	virtual int pop(int fieldindex);
	virtual int push(ObjectDef *newfield, int absorb=1);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 const char *fformat,const char *nrange, const char *newdefval,
					 unsigned int fflags,
					 NewObjectFunc nnewfunc,
					 ObjectFunc nstylefunc=NULL);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 const char *fformat,const char *nrange, const char *newdefval,
					 Laxkit::RefPtrStack<ObjectDef> *nfields,unsigned int fflags,
					 NewObjectFunc nnewfunc,
					 ObjectFunc nstylefunc);
	virtual int pushEnum(const char *nname,const char *nName,const char *ndesc,
					 bool is_class, //false means this enum works ONLY for this field. else it can be used elsewhere as a type
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
					const char *ptype, const char *nrange, const char *newdefval, Value *defvalue);
	virtual void Sort();

//	virtual int callFunction(const char *field, ValueHash *context, ValueHash *parameters,
//							 Value **value_ret, CalcSettings *settings, Laxkit::ErrorLog *log);

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//----------------------------- Value ----------------------------------

class Value : virtual public Laxkit::anObject, virtual public LaxFiles::DumpUtility
{
  protected:
	int modified;
	ObjectDef *objectdef;

  public:
	Value();
	virtual ~Value();
	virtual const char *whattype() { return "Value"; }

	virtual int type();
	virtual Value *duplicate() = 0;
	virtual int getValueStr(char **buffer,int *len, int oktoreallocate);//subclasses should NOT redefine this
	virtual int getValueStr(char *buffer,int len);//subclasses SHOULD redefine this
	virtual const char *Id();
	virtual const char *Id(const char *str);

	//virtual int isValidExt(const char *extstring, FieldPlace *place_ret) = 0; //for assignment checking
	virtual int assign(Value *v, const char *extstring); //1 success, 0 fail, 2 for success but other contents changed too, -1 for unknown
	virtual int assign(FieldExtPlace *ext,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown

	virtual Value *dereference(const char *extstring, int len);
	virtual Value *dereference(int index);

 	virtual ObjectDef *makeObjectDef() = 0; //calling code responsible for ref
    virtual ObjectDef *GetObjectDef();
    virtual int getNumFields();
    virtual  ObjectDef *FieldInfo(int i);
    virtual const char *FieldName(int i);
    virtual int FieldIndex(const char *name);

    //from DumpUtility:
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//----------------------------- ValueHash ----------------------------------
class ValueHash : virtual public Laxkit::anObject, virtual public Value, virtual public FunctionEvaluator
{
	Laxkit::PtrStack<char> keys;
	Laxkit::RefPtrStack<Value> values;

  public:
	ValueHash();
	virtual ~ValueHash();
	virtual const char *whattype() { return "ValueHash"; }
	int sorted;

	const char *key(int i);
	Value *value(int i);
	int flush();
	int push(const char *name,int i,int where=-1);
	int push(const char *name,double d,int where=-1);
	int push(const char *name,const char *string,int where=-1);
	int pushObject(const char *name,Laxkit::anObject *obj,int where=-1);
	int push(const char *name,Value *v,int where=-1, bool absorb = false);
	int push(const char *name,int len,Value *v,int where=-1);
	int remove(int i);
	void swap(int i1, int i2);

	void renameKey(int i,const char *newname);
	int set(const char *key, Value *newv, bool absorb = false);
	int set(int which, Value *newv, bool absorb = false);
	int CopyFrom(ValueHash *hash, bool linked);

	int n();
	Value *e(int i);
	Value *find(const char *name);
	int         findIndex(const char *name,int len=-1);
	long        findInt(const char *name, int which=-1, int *error_ret=NULL);
	int         findBoolean(const char *name, int which=-1, int *error_ret=NULL);
	double      findDouble(const char *name, int which=-1, int *error_ret=NULL);
	double      findIntOrDouble(const char *name, int which=-1, int *error_ret=NULL);
	const char *findString(const char *name, int which=-1, int *error_ret=NULL);
	flatvector  findFlatvector(const char *name, int which, int *error_ret=NULL);
	Laxkit::anObject *findObject(const char *name, int which=-1, int *error_ret=NULL);

	 //from Value:
	virtual int     type();
	virtual Value    *duplicate();
	virtual int       getValueStr(char *buffer,int len);
 	virtual ObjectDef *makeObjectDef();
	virtual Value     *dereference(int index);
	virtual Value     *dereference(const char *extstring, int len);
	virtual int        assign(FieldExtPlace *ext,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown
    virtual int        getNumFields();
    virtual ObjectDef  *FieldInfo(int i);
    virtual const char *FieldName(int i);

	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

//----------------------------- GenericValue ----------------------------------
class GenericValue : public Value
{
  public:
	ValueHash elements;

	GenericValue(ObjectDef *def);
	virtual ~GenericValue();
	virtual const char *whattype() { return "GenericValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();

 	virtual ObjectDef *makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v); //return 1 for success, 2 for success, but other contents changed too, -1 for unknown
};

//----------------------------- SetValue ----------------------------------
class SetValue : public Value, virtual public FunctionEvaluator
{
  public:
	char *restrictto;
	Laxkit::RefPtrStack<Value> values;

	SetValue(const char *restricted=NULL);
	virtual ~SetValue();
	virtual const char *whattype() { return "SetValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Set; }

	virtual int Push(Value *v,int absorb, int where=-1);
	virtual int Remove(int index);
	virtual int n();
	virtual Value *e(int i);
	virtual Value *FindID(const char *id);
	virtual int Set(int i, Value *v, int absorb);
	virtual void Flush();

 	virtual ObjectDef *makeObjectDef();
    virtual int getNumFields();
    virtual  ObjectDef *FieldInfo(int i);
    virtual const char *FieldName(int i);
	virtual Value *dereference(int index);

	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

// //----------------------------- MatrixValue ----------------------------------
// class MatrixValue : public SetValue
// {
//   public:
// 	int fixed_size;
// 	char *element_type;

// 	MatrixValue(const char *elementtype=NULL, int size=0);
// 	virtual ~MatrixValue();
// 	virtual const char *whattype() { return "MatrixValue"; }
// 	virtual int getValueStr(char *buffer,int len);
// 	virtual Value *duplicate();
// 	virtual int type() { return VALUE_Array; }

//  	virtual ObjectDef *makeObjectDef();

// 	virtual int Dimensions();
// };

//----------------------------- NullValue ----------------------------------
class NullValue : public Value
{
  public:
	NullValue() {}
	virtual const char *whattype() { return "NullValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_None; }
 	virtual ObjectDef *makeObjectDef() { return NULL; }
};

//----------------------------- BooleanValue ----------------------------------
class BooleanValue : public Value
{
  public:
	int i;
	BooleanValue(int ii) { i=ii; }
	BooleanValue(const char *val);
	virtual const char *whattype() { return "BooleanValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Boolean; }
 	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
};

//----------------------------- IntValue ----------------------------------
class IntValue : public Value
{
  public:
	Unit units;
	long i;
	IntValue(long ii=0) { i=ii; }
	IntValue(const char *val, int base);
	virtual const char *whattype() { return "IntValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Int; }
 	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
};

//----------------------------- DoubleValue ----------------------------------
class DoubleValue : public Value, virtual public FunctionEvaluator
{
  public:
	Unit units;
	double d;
	DoubleValue(double dd=0) { d=dd; }
	virtual void Set(const char *val);
	virtual const char *whattype() { return "DoubleValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Real; }
 	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- FlatvectorValue ----------------------------------
class FlatvectorValue : public Value, virtual public FunctionEvaluator
{
  public:
	Unit units;
	flatvector v;
	FlatvectorValue() { }
	FlatvectorValue(double x, double y) { v.set(x,y); }
	FlatvectorValue(flatvector vv) { v=vv; }
	virtual const char *whattype() { return "FlatvectorValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Flatvector; }
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
 	virtual ObjectDef *makeObjectDef();
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- SpacevectorValue ----------------------------------
class SpacevectorValue : public Value, virtual public FunctionEvaluator
{
  public:
	Unit units;
	spacevector v;
	SpacevectorValue() { }
	SpacevectorValue(spacevector vv) { v=vv; }
	virtual const char *whattype() { return "SpacevectorValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Spacevector; }
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
 	virtual ObjectDef *makeObjectDef();
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- QuaternionValue ----------------------------------
class QuaternionValue : public Value, virtual public FunctionEvaluator
{
  public:
	Unit units;
	Quaternion v;
	QuaternionValue() { }
	QuaternionValue(Quaternion vv) { v=vv; }
	virtual const char *whattype() { return "QuaternionValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Quaternion; }
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
 	virtual ObjectDef *makeObjectDef();
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- StringValue ----------------------------------
class StringValue : public Value, virtual public FunctionEvaluator
{
  public:
	char *str;
	StringValue(const char *s=NULL, int len=-1);
	virtual ~StringValue() { if (str) delete[] str; }
	virtual const char *whattype() { return "StringValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_String; }
 	virtual ObjectDef *makeObjectDef();
	virtual void Set(const char *nstr, int n=-1);
	virtual void InstallString(char *nstr);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- BytesValue ----------------------------------
class BytesValue : public Value, virtual public FunctionEvaluator
{
  public:
	char *str;
	int len;
	BytesValue(const char *s=NULL, int len=0);
	virtual ~BytesValue();
	virtual const char *whattype() { return "BytesValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Bytes; }
 	virtual ObjectDef *makeObjectDef();
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- DateValue ----------------------------------
class DateValue : public Value
{
  public:
  	struct tm date;

	DateValue() {}
	virtual const char *whattype() { return "DateValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Date; }
 	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
};

//----------------------------- EnumValue ----------------------------------
class EnumValue : public Value
{
  public:
	int value;
	char *tempkey; //hack for ease of i/o.. to be removed if MenuValue implemented instead

	EnumValue(ObjectDef *baseenum, int which);
	EnumValue(ObjectDef *baseenum, const char *which);
	virtual ~EnumValue();
	virtual const char *whattype() { return "EnumValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Enum; }
 	virtual ObjectDef *makeObjectDef();
	virtual int EnumId();
	virtual const char *EnumLabel();
	virtual const char *EnumName();
	virtual int SetFromId(int id);
};

//----------------------------- FunctionValue ----------------------------------
class FunctionValue : public Value
{
  public:
	char *code;
	FunctionEvaluator *function;

	FunctionValue(const char *ncode, int len);
	virtual ~FunctionValue();
	virtual const char *whattype() { return "FunctionValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Function; }
 	virtual ObjectDef *makeObjectDef();
};

//----------------------------- FileValue ----------------------------------
class FileValue : public Value, virtual public FunctionEvaluator
{
  public:
	char seperator;
	Laxkit::PtrStack<char> parts;
	char *filename;
	bool hint_is_dir;
	bool hint_for_saving;
	
	FileValue(const char *f=NULL,int len=-1);
	virtual ~FileValue();
	virtual const char *whattype() { return "FileValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_File; }
 	virtual ObjectDef *makeObjectDef();

	virtual int fileType(); //file link, dir link, file, dir, block
	virtual int isLink();
	virtual int Exists();
	virtual int Depth();
	virtual const char *Part(int i);
	virtual void Set(const char *nstr);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 Laxkit::ErrorLog *log);
};

//----------------------------- ColorValue ----------------------------------
class ColorValue : public Value
{
  public:
	Laxkit::ColorBase color;
	ColorValue();
	ColorValue(const char *color);
	ColorValue(double r, double g, double b, double a);
	ColorValue(const Laxkit::ScreenColor &scolor);
	//ColorValue(Laxkit::ColorBase &color);
	virtual ~ColorValue();
	virtual bool Parse(const char *str);
	virtual const char *whattype() { return "ColorValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Color; }
 	virtual ObjectDef *makeObjectDef();
};

//----------------------------- ObjectValue ----------------------------------
class ObjectValue : public Value
{
  public:
	Laxkit::anObject *object;
	ObjectValue(anObject *obj=NULL);
	virtual ~ObjectValue();
	virtual const char *whattype() { return "ObjectValue"; }
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return VALUE_Object; }
 	virtual ObjectDef *makeObjectDef();
	virtual void SetObject(anObject *nobj, bool absorb_count);
};

//------------------------------- parsing helpers ------------------------------------
ValueHash *MapParameters(ObjectDef *def,ValueHash *rawparams);
double getNumberValue(Value *v, int *isnum);
bool setNumberValue(double *d, Value *v);
int getIntValue(Value *v, int *isnum);
int isNumberType(Value *v, double *number_ret);
int isVectorType(Value *v, double *values);
int extequal(const char *str, int len, const char *field, char **next_ret=NULL);
int isName(const char *longstr,int len, const char *null_terminated_str);

Value *AttributeToValue(LaxFiles::Attribute *att);
Value *NewSimpleType(int type);

Value *JsonToValue(const char *str, const char **end_ptr);
Value *ValueToJson(const char *str);

Value *CSVStringToValue(const char *str, const char *delimiter, bool has_headers, bool prefer_cols, int *error_ret);

//-------------------------- Default ObjectDefs for builtin types ---------------------
ObjectDef *Get_BooleanValue_ObjectDef();
ObjectDef *Get_BytesValue_ObjectDef();
ObjectDef *Get_ColorValue_ObjectDef();
ObjectDef *Get_DateValue_ObjectDef();
ObjectDef *Get_DoubleValue_ObjectDef();
ObjectDef *Get_FileValue_ObjectDef();
ObjectDef *Get_FlatvectorValue_ObjectDef();
ObjectDef *Get_IntValue_ObjectDef();
ObjectDef *Get_QuaternionValue_ObjectDef();
ObjectDef *Get_SetValue_ObjectDef();
ObjectDef *Get_SpacevectorValue_ObjectDef();
ObjectDef *Get_StringValue_ObjectDef();
ObjectDef *Get_ValueHash_ObjectDef();


} // namespace Laidout

#endif

