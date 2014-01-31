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
// Copyright (C) 2010,2012-2013 by Tom Lechner
//
#ifndef DRAWABLEOBJECT_H
#define DRAWABLEOBJECT_H


#include <lax/tagged.h>
#include <lax/refptrstack.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/pathinterface.h>
#include "objectcontainer.h"
#include "../guides.h"
#include "../calculator/values.h"
//#include "objectfilter.h"



namespace Laidout {



class DrawableObject;
class PointAnchor;


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

//----------------------------------- AlignmentRule ---------------------------

//! Types for how a child DrawableObject fits in a parent. See AlignmentRule.
enum AlignmentRuleTypes
{
	ALIGNMENTRULE_None=0,
	ALIGNMENTRULE_Matrix,
	ALIGNMENTRULE_Align,    //!<Align particular bounding box point to particular parent bounding box point
	ALIGNMENTRULE_Move,     //!<Align to specific anchor in parent
	ALIGNMENTRULE_ScaleRotate,
	ALIGNMENTRULE_Scale,
	ALIGNMENTRULE_ScaleFree,
	ALIGNMENTRULE_ScaleX,
	ALIGNMENTRULE_ScaleY,
	ALIGNMENTRULE_Rotate,
	ALIGNMENTRULE_Stretch,
	ALIGNMENTRULE_Shear,
	ALIGNMENTRULE_EdgeMagnet,
	ALIGNMENTRULE_Code,       //!<Obtain matrix through code
	ALIGNMENTRULE_MAX
};

const char *AlignmentRuleName(int type);

class AlignmentRule
{
  public:
	int type; //straight matrix, align points, code, various in AlignmentRuleTypes
	

	int object_anchor; //an anchor in object that gets moved toward target
	int constraintype; //vector is object coords, or parent, or page, or none
	flatpoint constraindir;

	int invariant1, invariant2; //anchors in object

	 // too many types in one class!!
	 //1.
	double magnet_attachement; //0..1 usually
	double magnet_scale; //0 for natural size, 1 for total fill

	 //2.
	PointAnchor *target;
	int offset_units; //page, object, or parent
	flatpoint offset; //displacement off the target anchor

	 //3.
	flatpoint align1; //alignment to parent bounding box
	flatpoint align2; //where in bounding box to align to parent point from align1

	 //4.
	int code_id; //id of a TextObject with runnable code that returns an offset

	AlignmentRule *next;

	AlignmentRule();
	virtual ~AlignmentRule();
};


//----------------------------- DrawableObject ---------------------------------

/*! Used in DrawableObject::GetAnchor().
 * Custom anchors must have a value of ANCHOR_MAX or greater.
 */
enum BBoxAnchorTypes {
	BBOXANCHOR_ul=1, //devs: do not change this value!!
	BBOXANCHOR_um,
	BBOXANCHOR_ur,
	BBOXANCHOR_ml,
	BBOXANCHOR_mm,
	BBOXANCHOR_mr,
	BBOXANCHOR_ll,
	BBOXANCHOR_lm,
	BBOXANCHOR_lr,
	BBOXANCHOR_MAX
};

/*! for DrawableObject::locks */
enum DrawableObjectLockTypes {
	OBJLOCK_Contents   = (1<<0),
	OBJLOCK_Position   = (1<<1),
	OBJLOCK_Rotation   = (1<<2),
	OBJLOCK_Scale      = (1<<3),
	OBJLOCK_Shear      = (1<<4),
	OBJLOCK_Kids       = (1<<5),
	OBJLOCK_Selectable = (1<<6)
};

class DrawableObject :  virtual public ObjectContainer,
						virtual public Laxkit::Tagged,
						virtual public LaxInterfaces::SomeData,
					    virtual public FunctionEvaluator,
					    virtual public Value
{
 protected:
 public:
	DrawableObject *parent;
	AlignmentRule *parent_link;

	int locks; //lock object contents|matrix|rotation|shear|scale|kids|selectable
	char locked, visible, prints, selectable;

	SomeData *clip; //If not a PathsData, then is an object for a softmask
	LaxInterfaces::PathsData *clip_path;
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

	ValueHash properties;
	LaxFiles::Attribute metadata;
	LaxFiles::Attribute iohints;


	DrawableObject(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~DrawableObject();
	virtual const char *whattype() { return "Group"; }
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);
	virtual const char *Id();

	 //sub classes MUST redefine pointin() and FindBBox() to point to the proper things.
	 //default is point to things particular to Groups.
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void FindBBox();
	virtual int AddAlignmentRule(AlignmentRule *newlink, bool replace=false, int where=-1);
	virtual int RemoveAlignmentRule(int index);
	virtual void UpdateFromRules();
	virtual LaxInterfaces::SomeData *GetParent();
	virtual Laxkit::Affine GetTransformToContext(bool invert, int partial);

	virtual int Selectable();
	virtual int Visible();
	virtual int IsLocked(int which);
	virtual void Lock(int which);
	virtual void Unlock(int which);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_out_group(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void dump_in_group_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	
	 //new functions for DrawableObject
	virtual LaxInterfaces::PathsData *GetAreaPath();
	virtual LaxInterfaces::PathsData *GetInsetPath(); //return an inset path, may or may not be inset_path, where streams are laid into
	virtual LaxInterfaces::PathsData *GetWrapPath(); //path inside which external streams can't go

	Laxkit::RefPtrStack<PointAnchor> anchors;
	virtual int NumAnchors();
	virtual int GetAnchorInfoI(int anchor_index, int *id, const char **name, flatpoint *p, bool transform_to_parent);
	virtual int GetAnchorInfo(int anchor_id, const char **name, flatpoint *p, bool transform_to_parent);
	virtual int GetAnchorI(int anchor_index, PointAnchor **anchor);
	virtual int GetAnchor(int anchor_id, PointAnchor **anchor);
	virtual int AddAnchor(const char *name, flatpoint pos, int type, int nid);
	virtual int RemoveAnchor(int anchor_id);
	virtual int RemoveAnchorI(int index);


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
	virtual Value *duplicate();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log);
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
	virtual Value *dereference(int index);
	//virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log);
};


//------------------------------------ BBoxValue ------------------------------------------------
ObjectDef *makeBBoxObjectDef();
class BBoxValue : virtual public Value, virtual public Laxkit::DoubleBBox, virtual public FunctionEvaluator
{
  public:
	BBoxValue();
	BBoxValue(double mix,double max,double miy,double may);
	virtual ObjectDef *makeObjectDef();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *duplicate();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log);
};

} //namespace Laidout

#endif

