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
// Copyright (C) 2010,2012 by Tom Lechner
//
#ifndef DRAWABLEOBJECT_H
#define DRAWABLEOBJECT_H


#include <lax/tagged.h>
#include <lax/refptrstack.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/pathinterface.h>
#include "objectcontainer.h"
//#include "objectfilter.h"



namespace Laidout {



class DrawableObject;


//---------------------------------- DrawObjectChain ---------------------------------
class DrawObjectChain
{
  public:
	int chain_id;
	DrawableObject *object;
	DrawObjectChain *next, *prev;

	DrawObjectChain(int id=0);
	virtual ~DrawObjectChain();
};

//----------------------------------- DrawableParentLink ---------------------------

//! Types for how a child DrawableObject fits in a parent. See DrawableParentLink.
enum ParentLinkTypes
{
	PARENTLINK_Matrix,
	PARENTLINK_Align,
	PARENTLINK_Anchor,
	PARENTLINK_Code,
	PARENTLINK_MAX
};

class DrawableParentLink
{
  public:
	int type; //straight matrix, align points, align by parent anchor, code
	flatpoint anchor1,anchor2;
	int parent_anchor_id;
	int code_id;

	DrawableParentLink();
	virtual ~DrawableParentLink();
};


//----------------------------- DrawableObject ---------------------------------
class DrawableObject :  virtual public ObjectContainer,
						virtual public LaxInterfaces::SomeData,
						virtual public Laxkit::Tagged
{
 protected:
 public:
	DrawableObject *parent;
	DrawableParentLink *parent_link;
	int locks; //lock object contents|matrix|rotation|shear|scale|kids|selectable
	char locked, visible, prints, selectable;

	SomeData *clip; //If not a PathsData, then is an object for a softmask
	LaxInterfaces::PathsData *wrap_path;
	LaxInterfaces::PathsData *inset_path;
	double autowrap, autoinset; //distance away from default to put the paths when auto generated
	int wraptype;

	Laxkit::RefPtrStack<DrawObjectChain> chains; //for linked objects

	//Laxkit::RefPtrStack<ObjectStream> path_streams; //applied to areapath outline
	//Laxkit::RefPtrStack<ObjectStream> area_streams; //applied into areapath area

	//Laxkit::RefPtrStack<ObjectFilter> filters;
	double alpha; //object alpha applied to anything drawn by this and kids
	double blur; //one built in filter?

	//Laxkit::RefPtrStack<anObject *> refs; //what other resources this objects depends on?

	LaxFiles::Attribute metadata;
	LaxFiles::Attribute iohints;


	DrawableObject(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~DrawableObject();
	virtual const char *whattype() { return "Group"; }
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);

	 //sub classes MUST redefine pointin() and FindBBox() to point to the proper things.
	 //default is point to things particular to Groups.
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void FindBBox();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_out_group(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void dump_in_group_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	
	 //new functions for DrawableObject
	LaxInterfaces::PathsData *GetAreaPath();
	LaxInterfaces::PathsData *GetInsetPath(); //return an inset path, may or may not be inset_path, where streams are laid into
	LaxInterfaces::PathsData *GetWrapPath(); //path inside which external streams can't go


	 //Group specific functions:
	Laxkit::RefPtrStack<LaxInterfaces::SomeData> kids;
	virtual LaxInterfaces::SomeData *findobj(LaxInterfaces::SomeData *d,int *n=NULL);
	virtual int findindex(LaxInterfaces::SomeData *d) { return kids.findindex(d); }
	virtual int push(LaxInterfaces::SomeData *obj);
	virtual int pushnodup(LaxInterfaces::SomeData *obj);
	virtual int remove(int i);
	virtual LaxInterfaces::SomeData *pop(int which);
	virtual int popp(LaxInterfaces::SomeData *d);
	virtual void flush();
	virtual void swap(int i1,int i2) { kids.swap(i1,i2); }
	virtual int slide(int i1,int i2);

	//virtual int contains(SomeData *d,FieldPlace &place);
	//virtual LaxInterfaces::SomeData *getObject(FieldPlace &place,int offset=0);
	//virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, LaxInterfaces::SomeData **d=NULL);

	virtual int GroupObjs(int n, int *which);
	virtual int UnGroup(int which);
	virtual int UnGroup(int n,const int *which);
	
	 //for ObjectContainer
	virtual int n();
	virtual LaxInterfaces::SomeData *e(int i);
	virtual Laxkit::anObject *object_e(int i);
	virtual const char *object_e_name(int i);
	virtual const double *object_transform(int i);

	 //for Value
	virtual ObjectDef *makeObjectDef();
};

//------------------------------------ AffineValue ------------------------------------------------
ObjectDef *makeAffineObjectDef();
class AffineValue : virtual public Value, virtual public Laxkit::Affine, virtual public FunctionEvaluator
{
  public:
	AffineValue();
	AffineValue(const double *m);
	virtual ObjectDef *makeObjectDef();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual int type() { return GetObjectDef()->fieldsformat; }
	virtual Value *dereference(int index);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log);
};


} //namespace Laidout

#endif

