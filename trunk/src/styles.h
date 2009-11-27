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
// Copyright (C) 2004-2009 by Tom Lechner
//
#ifndef STYLES_H
#define STYLES_H

#include <lax/anobject.h>
#include <lax/lists.h>
#include <lax/strmanip.h>
#include <cstdio>
#include <lax/dump.h>
#include <lax/refcounted.h>
#include "calculator/values.h"

//------------------------------ FieldMask -------------------------------------------

class FieldPlace : protected Laxkit::NumStack<int>
{
 public:
	FieldPlace() {}
	FieldPlace(const FieldPlace &place);
	virtual ~FieldPlace() {}
	virtual int n() const { return Laxkit::NumStack<int>::n; }
	virtual int e(int i) const { if (i>=0 && i<Laxkit::NumStack<int>::n) 
		return Laxkit::NumStack<int>::e[i];  return -1; }
	virtual int e(int i,int val) { if (i>=0 && i<Laxkit::NumStack<int>::n) 
		{ return Laxkit::NumStack<int>::e[i]=val; }  return -1; }
	virtual int operator==(const FieldPlace &place) const;
	virtual FieldPlace &operator=(const FieldPlace &place);
	virtual int push(int nd,int where=-1) { return Laxkit::NumStack<int>::push(nd,where); }
	virtual int pop(int which=-1) { return Laxkit::NumStack<int>::pop(which); }
	virtual void flush() { Laxkit::NumStack<int>::flush(); }
	virtual const int *list() { return (const int *)Laxkit::NumStack<int>::e; }
	virtual void out(const char *str);//for debugging
};

//------------------------------ FieldMask -------------------------------------------

class FieldMask : public Laxkit::PtrStack<FieldPlace>
{
 protected:
	int *chartoint(const char *ext,const char **next_ret);
 public:
	FieldMask();
	FieldMask(const char *ext);
	FieldMask(const FieldMask &mask);
	FieldMask &operator=(FieldMask &mask);
	FieldMask &operator=(FieldPlace &place);
	virtual int operator==(int what);
	virtual int operator==(const FieldMask &mask);
	virtual FieldMask &operator=(int f); // shorthand for single level fields
	//virtual int push(FieldMask &f);
	virtual int push(int where,int n, ...);
	virtual int push(int n,int *list,int where=-1);
	virtual int has(const char *ext,int *index=NULL); // does ext correspond to any in mask?
	virtual int value(int f,int where);
	virtual int value(int f,int where,int newval);
};


//------------------------------ StyleDef --------------------------------------------
#define STYLEDEF_CAPPED 1

 // for StyleDef::format
enum ElementType {
	Element_Any,
	Element_None,
	Element_Int,
	Element_Real,
	Element_String,
	Element_Fields, 
	Element_Boolean,
	Element_Date,
	Element_File,
	Element_3bit,
	Element_Flag,
	Element_Enum,
	Element_DynamicEnum,
	Element_EnumVal,
	Element_Function,
	Element_Color,
	Element_MaxBuiltinFormatValue
};
extern const char *element_TypeNames[14];
		

#define STYLEDEF_DUPLICATE 1
#define STYLEDEF_ORPHAN    2

class StyleDef;
class Style;
typedef Style *(*NewStyleFunc)(StyleDef *def);
typedef int (*StyleFunc)(ValueHash *context, ValueHash *parameters,
							 Value **value_ret, char **message_ret);
 
class StyleDef : public Laxkit::anObject, public LaxFiles::DumpUtility, public Laxkit::RefCounted
{
 public:
	char *extends;
	StyleDef *extendsdef;
	NewStyleFunc newfunc;
	StyleFunc stylefunc;
	Style *newStyle(StyleDef *def) { if (newfunc) return newfunc(this); return NULL; }
	char *name; //name for interpreter (basically class name)
	char *Name; // Name for dialog label
	char *description; // description
	char *range;
	char *defaultvalue;
	
	 // STYLEDEF_ORIGINAL
	 // STYLEDEF_DUPLICATE
	 // STYLEDEF_ORPHAN  =  is a representation of a composite style, not stored in any manager, and only 1 reference to it exists
	 // STYLEDEF_CAPPED = cannot push/pop fields
	 // STYLEDEF_READONLY = cannot modify parts of the styledef
	unsigned int flags;

	ElementType format; // int,real,string,fields,...
	int fieldsformat;  //dynamically assigned to new object types
	Laxkit::PtrStack<StyleDef> *fields; //might be NULL, any fields are assumed to not be local to the stack.
	
	StyleDef();
	StyleDef(const char *nextends,const char *nname,const char *nName,
			const char *ndesc,ElementType fmt,const char *nrange, const char *newdefval,
			Laxkit::PtrStack<StyleDef>  *nfields=NULL,unsigned int fflags=STYLEDEF_CAPPED,
			NewStyleFunc nnewfunc=0,StyleFunc nstylefunc=0);
	virtual ~StyleDef();

	 // helpers to locate fields by name, "blah.3.x"
	virtual int getNumFields();
	virtual int findfield(char *fname,char **next); // return index value of fname. assumed top level field
	virtual int findDef(int index,StyleDef **def);
	virtual int getInfo(int index,
						const char **nm=NULL,
						const char **Nm=NULL,
						const char **desc=NULL,
						const char **rng=NULL,
						const char **defv=NULL,
						ElementType *fmt=NULL,
						int *objtype=NULL);
 	//int *getfields(const char *extstr); // returns 0 terminated list of indices: "1.4.23+ -> { 1,4,23,0 }

	 //-------- StyleDef creation helper functions ------
	 // The following (push/pop/cap) are convenience functions 
	 // to construct a styledef on the fly
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 ElementType fformat,const char *nrange, const char *newdefval,
					 unsigned int fflags,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc=NULL);
	virtual int push(const char *nname,const char *nName,const char *ndesc,
					 ElementType fformat,const char *nrange, const char *newdefval,
					 Laxkit::PtrStack<StyleDef> *nfields,unsigned int fflags,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc=NULL);
	virtual int push(StyleDef *newfield);
	virtual int pop(int fieldindex);

//	virtual int callFunction(const char *field, ValueHash *context, ValueHash *parameters,
//							 Value **value_ret, char **message_ret);

	 // cap prevents accidental further adding/removing fields to a styledef
	 // that is being constructed
	virtual void cap(int y=1) { if (y) flags|=STYLEDEF_CAPPED; else flags&=~STYLEDEF_CAPPED; }
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//------------------------------------- FieldNode -------------------------------------------
class Style;

class FieldNode
{
 public:
	int index;
	int format;
	unsigned int flags;
	union { 
		unsigned int f;    // format=0, for (non-exclusive) flags
		int e;             // format=1, for enums
		int i;             // format=2
		double d;          // format=3
		char *str;         // format=4
		FieldNode *fields; // format=5
		Style *style;      // format=6, this is useful for variable sized sets
		//Value *v;          // format=7
		void *vd;          // format=8
	} value;
	FieldNode() { value.d=0; } //*** does this really wipe out all?
};

void deleteFieldNode(FieldNode *fn);
	
//------------------------------------- Style -------------------------------------------

#define STYLE_READONLY
#define STYLE_NO_EDIT

class Style : virtual public Laxkit::anObject, 
			  virtual public LaxFiles::DumpUtility,
			  virtual public Laxkit::RefCounted
{
 protected:
	unsigned long style;
	char *stylename; // note this is not a variable name, but it is an instance, 
					 // it would be in a list of styles, like "Bold Body", and the
					 // StyleDef name/Name might be charstyle/"Character Style"
 public:
	StyleDef *styledef;
	Style *basedon;
	FieldMask fieldmask; // is mask of which values are defined in this Style, and would
						 // preempt fields from basedon
	Style();
	Style(StyleDef *sdef,Style *bsdon,const char *nstn);
	virtual ~Style();
	virtual StyleDef *makeStyleDef() = 0;
	virtual StyleDef *GetStyleDef() { return styledef; }
	virtual const char *Stylename() { return stylename; }
	virtual int Stylename(const char *nname) { makestr(stylename,nname); return 1; }
	virtual int getNumFields();
	virtual Style *duplicate(Style *s=NULL)=0;

	 // these return a mask of what changes when you set the specified value.
	 // set must ask the styledef if it can really set that field, 
	 // 	the StyleDef returns a mask of what else changes?????***
	 // set must create the field in *this if it does not exist
	 // get should indicate whether the found value is in *this or a basedon
	//maybe:
//	virtual void *dereference(const char *extstr,int copy) { return NULL; }//***
//	virtual int set(FieldMask *field,Value *val,FieldMask *mask_ret) { return 1; } //***=0
//	virtual int set(const char *ext,Value *val,FieldMask *mask_ret) { return 1; } //***=0
	
//	virtual FieldMask set(FieldMask *field,Value *val); 
//	virtual FieldMask set(Fieldmask *field,const char *val);
//	virtual FieldMask set(const char *ext, const char *val);
//	virtual FieldMask set(Fieldmask *field,int val);
//	virtual FieldMask set(const char *ext, int val);
//	virtual FieldMask set(Fieldmask *field,double val);
//	virtual FieldMask set(const char *ext, double val);
//	virtual Value *getvalue(FieldMask *field);
//	virtual Value *getvalue(const cahr *ext);
//	virtual int getint(FieldMask *field);
//	virtual int getint(const char *ext);
//	virtual double getdouble(FieldMask *field);
//	virtual double getdouble(const char *ext);
//	virtual char *getstring(FieldMask *field);
//	virtual char *getstring(const char *ext);
};

class EnumStyle : public Style
{
 public:
	Laxkit::NumStack<int> ids;
	Laxkit::PtrStack<char> names;

	EnumStyle();
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	virtual int add(const char *nname,int nid=-1);
	virtual const char *name(int Id);
	virtual int id(const char *Name);
	virtual int num() { return names.n; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context) {}
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context) {}
};







#endif

