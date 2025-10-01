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
// Copyright (C) 2010,2012-2013 by Tom Lechner
//
#ifndef DRAWABLEOBJECT_H
#define DRAWABLEOBJECT_H


#include <lax/tagged.h>
#include <lax/refptrstack.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/groupdata.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/somedataref.h>
#include "objectcontainer.h"
//#include "../filetypes/objectio.h"
#include "../core/guides.h"
#include "../calculator/values.h"
//#include "objectfilter.h"
// #include "../text/streams.h"


namespace Laidout {



class StreamAttachment;
class DrawableObject;
class PointAnchor;
class Document;
class Page;
class ObjectIO;
//class ObjectFilter;


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
	int invariant1, invariant2; //anchors in object

	enum TargetType { UNKNOWN, NONE, PARENT, OBJECT, PAGE, OTHER_OBJECT };

	TargetType constraintype; //vector is object coords, or parent, or page, or none
	Laxkit::flatpoint constraindir;

	 // too many types in one class!!
	 //1.
	double magnet_attachement; //0..1 usually
	double magnet_scale; //0 for natural size, 1 for total fill

	 //2.
	PointAnchor *target;
	TargetType target_location; //stub info for loading
	char *target_object; //stub info for loading
	char *target_anchor; //stub info for loading

	TargetType offset_units; //page, object, or parent
	Laxkit::flatpoint offset; //displacement off the target anchor

	 //3.
	Laxkit::flatpoint align1; //alignment to parent bounding box
	Laxkit::flatpoint align2; //where in bounding box to align to parent point from align1

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

enum ClipTypes {
	CLIP_None,
	CLIP_From_Parent_Area,
	CLIP_Custom_Path,
	CLIP_Custom_Softmask,
	CLIP_MAX
};

class DrawableObject :  virtual public ObjectContainer,
						virtual public Laxkit::Tagged,
						virtual public LaxInterfaces::GroupData,
					    virtual public FunctionEvaluator,
					    virtual public Value
{
 protected:
 public:
	AlignmentRule *parent_link;

	ObjectIO *importer;
	Laxkit::anObject *importer_data;

	ClipTypes clip_type;
	ClipTypes child_clip_type;
	SomeData *soft_mask; //whatever it is, take copy to use as alpha
	LaxInterfaces::PathsData *clip_path; //its m is from main object coordinate space
	virtual LaxInterfaces::PathsData *ClipPath(const double **extra_m);
	virtual void RebuildClipPath();

	double autowrap;
	double autoinset; //distance away from default to put the paths when auto generated
	LaxInterfaces::PathsData *inset_path;
	LaxInterfaces::PathsData *wrap_path; //area path + offset, or custom

	enum WrapType {
		NoWrap,
		BlockLeft,
		BlockRight,
		BlockAround,
		Bounds,
		VisualBounds, //page level aligned bounds
		Offset,
		Custom
	};
	WrapType wraptype, wrapblock;

	Laxkit::RefPtrStack<DrawObjectChain> chains; //for linked objects

	Laxkit::RefPtrStack<StreamAttachment> streams;

	//--filters:
	double opacity;
	//double blur;   //TODO! opacity and blur: built in filters?
	Laxkit::anObject *filter; // *** can't declare as ObjectFilter directly due to absurd filefilter.h definitions.. need to fix this!
	virtual DrawableObject *FinalObject();
	virtual int SetFilter(Laxkit::anObject *nfilter, int absorb);
	virtual void ForceFilterUpdates(bool onlythis);

	//Laxkit::RefPtrStack<anObject *> refs; //what other resources this objects depends on?

	ValueHash properties;
	Laxkit::AttributeObject *metadata;
	Laxkit::Attribute iohints;


	DrawableObject();
	virtual ~DrawableObject();
	virtual const char *whattype() { return "Group"; }
	virtual const char *IconKey() { return "Group"; }
	virtual LaxInterfaces::SomeData *duplicateData(LaxInterfaces::SomeData *dup);
	virtual const char *Id();
	virtual const char *Id(const char *newid);
	virtual int dec_count();

	virtual int Selectable();

	// sub classes MUST redefine pointin() and FindBBox() to point to the proper things.
	// default is point to things particular to Groups.
	//virtual int pointin(Laxkit::flatpoint pp,int pin=1);
	virtual void FindBBox();
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual int AddAlignmentRule(AlignmentRule *newlink, bool replace=false, int where=-1);
	virtual int RemoveAlignmentRule(int index);
	virtual void UpdateFromRules();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	//virtual void dump_out_group(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	//virtual void dump_in_group_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	static DrawableObject* CreateFromAttribute(Laxkit::Attribute *att, const char *type, Laxkit::DumpContext *context);
	
	 //new functions for DrawableObject
	virtual LaxInterfaces::SomeData *EquivalentObject();
	virtual LaxInterfaces::PathsData *GetAreaPath();
	virtual LaxInterfaces::PathsData *GetInsetPath(); //return an inset path, may or may not be inset_path, where streams are laid into
	virtual LaxInterfaces::PathsData *GetWrapPath(); //path inside which external streams can't go

	virtual int InstallClip(LaxInterfaces::PathsData *pathsdata);

	// for snapping
	Laxkit::RefPtrStack<PointAnchor> anchors;
	virtual int NumAnchors();
	virtual int GetAnchorInfoI(int anchor_index, int *id, const char **name, Laxkit::flatpoint *p, int *anchor_type, bool transform_to_parent);
	virtual int GetAnchorInfo(int anchor_id, const char **name, Laxkit::flatpoint *p, int *anchor_type, bool transform_to_parent);
	virtual int GetAnchorI(int anchor_index, PointAnchor **anchor);
	virtual int GetAnchor(int anchor_id, PointAnchor **anchor);
	virtual int FindAnchorId(const char *name, int *index_ret);
	virtual int AddAnchor(const char *name, Laxkit::flatpoint pos, int type, int nid);
	virtual int RemoveAnchor(int anchor_id);
	virtual int RemoveAnchorI(int index);
	virtual int ResolveAnchorRefs(Document *doc, Page *page, DrawableObject *g, Laxkit::ErrorLog &log);

	// custom properties
	virtual Laxkit::anObject *GetProperty(const char *name);
	virtual int SetProperty(const char *name, Laxkit::anObject *prop, bool absorb);

	 //Group specific functions:
	//virtual int contains(SomeData *d,FieldPlace &place);
	//virtual LaxInterfaces::SomeData *getObject(FieldPlace &place,int offset=0);
	//virtual int nextObject(FieldPlace &place, FieldPlace &first, int curlevel, LaxInterfaces::SomeData **d=NULL);

	virtual int GroupObjs(int n, int *which, int *newgroupindex);
	virtual int UnGroup(int which);
	virtual int UnGroup(int n,const int *which);
	
	 //for ObjectContainer
	virtual int n();
	virtual LaxInterfaces::SomeData *e(int i);
	virtual Laxkit::anObject *object_e(int i);
	virtual const char *object_e_name(int i);
	virtual const double *object_transform(int i);

	virtual LaxInterfaces::SomeData *FindObject(const char *id);
	virtual LaxInterfaces::SomeData *FindObjectRegex(const char *pattern, LaxInterfaces::SomeData *after);
	virtual DrawableObject *GetDrawableParent();

	 //for Value
	virtual ObjectDef *makeObjectDef();
	virtual anObject *duplicate() { return duplicateValue(); }
	virtual Value *duplicateValue();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);
};

typedef DrawableObject Group;



} //namespace Laidout

#endif

