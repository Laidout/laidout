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
// Copyright (C) 2010-2013 by Tom Lechner
//


#include <lax/transformmath.h>
#include "drawableobject.h"
#include "../laidout.h"
#include "../drawdata.h"
#include "../language.h"
#include "../stylemanager.h"
#include "objectfilter.h"

#include <lax/refptrstack.cc>

#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {


//----------------------------- AlignmentRuleTypes ---------------------------------
const char *AlignmentRuleName(int type)
{
	if (type==ALIGNMENTRULE_None)       return "";
	if (type==ALIGNMENTRULE_Matrix)      return _("Matrix");
	if (type==ALIGNMENTRULE_Align)       return _("Align");
	if (type==ALIGNMENTRULE_Move)        return _("Move");
	if (type==ALIGNMENTRULE_Scale)       return _("Scale");
	if (type==ALIGNMENTRULE_ScaleFree)   return _("Scale Free");
	if (type==ALIGNMENTRULE_ScaleX)      return _("Scale X");
	if (type==ALIGNMENTRULE_ScaleY)      return _("Scale Y");
	if (type==ALIGNMENTRULE_ScaleRotate) return _("Scale Rotate");
	if (type==ALIGNMENTRULE_Rotate)      return _("Rotate");
	if (type==ALIGNMENTRULE_Stretch)     return _("Stretch");
	if (type==ALIGNMENTRULE_Shear)       return _("Shear");
	if (type==ALIGNMENTRULE_EdgeMagnet)  return _("Edge Magnet");
	if (type==ALIGNMENTRULE_Code)        return _("Code");
	return "";
}

/*! This is used for rule saving and loading.
 */
const char *AlignmentRulePlainName(int type)
{
	if (type==ALIGNMENTRULE_None)        return "";
	if (type==ALIGNMENTRULE_Matrix)      return "Matrix";
	if (type==ALIGNMENTRULE_Align)       return "Align";
	if (type==ALIGNMENTRULE_Move)        return "Move";
	if (type==ALIGNMENTRULE_Scale)       return "Scale";
	if (type==ALIGNMENTRULE_ScaleFree)   return "ScaleFree";
	if (type==ALIGNMENTRULE_ScaleX)      return "ScaleX";
	if (type==ALIGNMENTRULE_ScaleY)      return "ScaleY";
	if (type==ALIGNMENTRULE_ScaleRotate) return "ScaleRotate";
	if (type==ALIGNMENTRULE_Rotate)      return "Rotate";
	if (type==ALIGNMENTRULE_Stretch)     return "Stretch";
	if (type==ALIGNMENTRULE_Shear)       return "Shear";
	if (type==ALIGNMENTRULE_EdgeMagnet)  return "EdgeMagnet";
	if (type==ALIGNMENTRULE_Code)        return "Code";
	return "";
}

int AlignmentRuleFromPlainName(const char *rule)
{
	if (!strcmp(rule,""))            return ALIGNMENTRULE_None;
	if (!strcmp(rule,"Matrix"))      return ALIGNMENTRULE_Matrix;
	if (!strcmp(rule,"Align"))       return ALIGNMENTRULE_Align;
	if (!strcmp(rule,"Move"))        return ALIGNMENTRULE_Move;
	if (!strcmp(rule,"Scale"))       return ALIGNMENTRULE_Scale;
	if (!strcmp(rule,"ScaleFree"))   return ALIGNMENTRULE_ScaleFree;
	if (!strcmp(rule,"ScaleX"))      return ALIGNMENTRULE_ScaleX;
	if (!strcmp(rule,"ScaleY"))      return ALIGNMENTRULE_ScaleY;
	if (!strcmp(rule,"ScaleRotate")) return ALIGNMENTRULE_ScaleRotate;
	if (!strcmp(rule,"Rotate"))      return ALIGNMENTRULE_Rotate;
	if (!strcmp(rule,"Stretch"))     return ALIGNMENTRULE_Stretch;
	if (!strcmp(rule,"Shear"))       return ALIGNMENTRULE_Shear;
	if (!strcmp(rule,"EdgeMagnet"))  return ALIGNMENTRULE_EdgeMagnet;
	if (!strcmp(rule,"Code"))        return ALIGNMENTRULE_Code;
	return ALIGNMENTRULE_None;
}


//----------------------------------- AlignmentRule ---------------------------
/*! \class AlignmentRule
 * \brief Define a particular relationship to a parent object.
 *
 * This might be a straight affine matrix, or alignment to the parent's bounding box,
 * or alignment to a parent anchor point, or code.
 *
 * The affine matrix found with drawableobject->m() is the actual visual appearance
 * at any given time. It is computed based on settings found in 
 * DrawableObject::parent_link.
 *
 * If next!=NULL, then apply any anchors in order. Typically, this will be a position
 * anchor first, then resize anchors for edges of bounding boxes.
 */

AlignmentRule::AlignmentRule()
{
	type=ALIGNMENTRULE_Matrix;

	object_anchor=0; //an anchor in object that gets moved toward target
	constraintype=UNKNOWN; //vector is 0 for parent, 1 for object, 2 for page

	invariant1=0;
	invariant2=0;

	magnet_attachement=.5; //0..1 usually
	magnet_scale=0; //0 for natural size, 1 for total fill

	target=NULL;
	target_object=NULL; //if not null and target is null, then need to search for it..
	target_anchor=NULL;
	offset_units=UNKNOWN; //0 for parent, 1 for object, 2 for page

	code_id=0;
	next=NULL;
}

AlignmentRule::~AlignmentRule()
{
	delete[] target_object;
	delete[] target_anchor;
	if (target) target->dec_count();

	if (next) delete next;
}


//---------------------------------- DrawObjectChain ---------------------------------
/*! \class DrawObjectChain
 * \brief Class to link objects together.
 *
 * Chains can be used in various ways. The most common is linking text frames. Also,
 * they can be used to link imported images into a single chain. Say you import pages of a multipage pdf.
 * A chain can preserve the order of imported pages, and if you reimport later, you can
 * automatically replace that chain with the new one. This can be useful for updating externally defined content.
 *
 * Further food for thought is using chains to define connector networks, but that might be overkill.
 * More food for thought would be using chains for special filter application, or for skeleton manipulation.
 *
 * Please note that DrawableObjects increment and decrement DrawObjectChains, but NOT the other way around.
 * The chain only has a plain pointer to the objects. Thus the objects are really parents, and the chain is a child.
 */


DrawObjectChain::DrawObjectChain(int id)
{
	chain_id=id;
	object=NULL;
	next=prev=NULL;
}

DrawObjectChain::~DrawObjectChain()
{
	if (next) next->prev=prev;
	if (prev) prev->next=next;
}


//----------------------------- DrawableObject ---------------------------------
/*! \class DrawableObject
 * \brief base of all drawable Laidout objects.
 *
 * The Laxkit interfaces get the elements of DrawableObject
 *
 * The object may or may not have its own clip path or mask, or a wraparound
 * or inset path. The inset path is the area it defines in which streams may be
 * laid inside. If these things are not specified, then they are generated 
 * automatically.
 */


DrawableObject::DrawableObject()
{
	clip = NULL;
	clip_path = wrap_path = inset_path=NULL;
	autowrap = autoinset = 0;

	alpha  = 1;
	blur   = 0;
	filter = NULL;

	parent_link=NULL;

	//Id(); //makes this->nameid (of SomeData) be something like `whattype()`12343
}

/*! Will detach this object from any object chain. It is assumed that other objects in
 * the chain are referenced elsewhere, so the other objects in the chain are NOT deleted here.
 */
DrawableObject::~DrawableObject()
{
	if (clip)       clip      ->dec_count();
	if (clip_path)  clip_path ->dec_count();
	if (wrap_path)  wrap_path ->dec_count();
	if (inset_path) inset_path->dec_count();
	if (filter)     filter    ->dec_count();

	if (chains.n)   chains.flush();

	if (parent_link) delete parent_link; //don't delete parent itself.. that is a one way reference
}

int DrawableObject::Selectable()
{
	return (!(obj_flags&OBJ_Unselectable)) | SomeData::Selectable();
}

/*! Return object primitives that represent this object.
 *
 * For objects that belong to hot new tools, it is very difficult
 * to update exporters for these specific new requirements. This way,
 * you need only define this function to return the equivalent
 * rendered in Laidout "primitives".
 *
 * Default is to return NULL. If there is no equivalent, NULL MUST be returned.
 * Calling code must call dec_count() on the returned object.
 */
LaxInterfaces::SomeData *DrawableObject::EquivalentObject()
{ return NULL; }

/*! Return an object representing *this transformed by filter.
 * If no filter, return this.
 */
DrawableObject *DrawableObject::FinalObject()
{
	if (!filter)return this;

	ObjectFilter *ofilter = dynamic_cast<ObjectFilter*>(filter);
	DrawableObject *fobj = (ofilter ? dynamic_cast<DrawableObject*>(ofilter->FinalObject()) : NULL);
	if (fobj) return fobj;

	DBG cerr << " *** Warning! filter did not return a valid object for "<<Id()<<"!"<<endl;
	return this;
}

/*! If index out or range, remove top.
 * Return 0 for success, nonzero error.
 */
int DrawableObject::RemoveAlignmentRule(int index)
{
	if (!parent_link) return 0;

	AlignmentRule *ruleprev=NULL, *rule=parent_link;
	while (rule && rule->next && index) { ruleprev=rule; rule=rule->next; index--; }

	if (ruleprev) ruleprev->next=rule->next;
	else parent_link=rule->next;
	rule->next=NULL;
	delete rule;

	return 0;
}

/*! Return 0 for success.
 * Return 1 for improper link, nothing changed.
 *
 * If newlink==NULL, then remove all rules.
 * If where>=0, the install at that position, else tack onto end.
 * If replace, then replace link at position where, else insert at that spot.
 */
int DrawableObject::AddAlignmentRule(AlignmentRule *newlink, bool replace, int where)
{
	if (!newlink) {
		if (parent_link) delete parent_link;
		parent_link=NULL;
		return 0;
	}
	
	if (where<0) where=10000000; //effectively the end
	AlignmentRule *ruleprev=NULL, *rule=parent_link;
	while (rule && where) { ruleprev=rule; rule=rule->next; where--; }

	if (!rule) {
		 //append on end
		if (ruleprev) ruleprev->next=newlink;
		else parent_link=newlink;
	} else {
		if (replace) {
			AlignmentRule *old=rule;
			ruleprev->next=newlink;
			while (newlink->next) newlink=newlink->next;
			newlink->next=(rule?rule->next:NULL);
			old->next=NULL;
			delete old;

		} else {
			ruleprev->next=newlink;
			while (newlink->next) newlink=newlink->next;
			newlink->next=rule;
		}
	}

	UpdateFromRules();

	return 0;
}

//! Set this->m() to be an approprate value based on parent_link. Does nothing if no rules.
void DrawableObject::UpdateFromRules()
{
	 //parent link can be a straight matrix off the parent coordinate origin,
	 //or align to an anchor, then offset
	if (!parent_link) return;
	
	AlignmentRule *link=parent_link;
	while (link) {
		if (link->type==ALIGNMENTRULE_Matrix) {
			//a do nothing rule

		} else if (link->type==ALIGNMENTRULE_EdgeMagnet) {
			DBG cerr <<" *** need to implement ALIGNMENTRULE_EdgeMagnet"<<endl;
		} else if (link->type==ALIGNMENTRULE_Code) {
			DBG cerr <<" *** need to implement ALIGNMENTRULE_Code"<<endl;

		} else if (link->type==ALIGNMENTRULE_Align) {
			 // Align particular bounding box point to particular parent bounding box point
			if (!parent) { link=link->next; continue; }

			flatpoint p;
			p.x = parent->minx + link->align1.x*(parent->maxx-parent->minx);
			p.y = parent->miny + link->align1.y*(parent->maxy-parent->miny);

			flatpoint p2;
			p2.x = minx + link->align2.x*(maxx-minx);
			p2.y = miny + link->align2.y*(maxy-miny);
			p2=transformPoint(p2);

			origin(origin()+p-p2);

		} else {
			if (!link->target) {
				DBG cerr << " warning: null rule link->target, skipping for now"<<endl;
				break;
			}

			 //get target point
			flatpoint np=link->target->p;
			if (dynamic_cast<Page*>(link->target->owner)) {
				if (link->target->anchor_type==PANCHOR_BBox) {
					Page *page=dynamic_cast<Page*>(link->target->owner);
					if (page->pagestyle->outline) np=page->pagestyle->outline->BBoxPoint(np.x,np.y, false);
				}
			} else {
				GroupData *d=dynamic_cast<GroupData*>(link->target->owner);

				if (d && link->target->anchor_type==PANCHOR_BBox) {
					np=d->BBoxPoint(np.x,np.y,false);
				}

				while (d) {
					np=d->transformPoint(np);
					d=dynamic_cast<GroupData*>(d->parent);
				}
			}
			Affine a=GetTransformToContext(true,1); //invert, and get transform to parent context
			np=a.transformPoint(np); //now np is the anchor point mapped to object parent space


			flatpoint op; //op will be object anchor in parent space
			GetAnchorInfo(link->object_anchor, NULL, &op,NULL, true);

			flatpoint i1, i2;
			if (link->invariant1>=0) GetAnchorInfo(link->invariant1, NULL, &i1,NULL, true);
			if (link->invariant2>=0) GetAnchorInfo(link->invariant2, NULL, &i2,NULL, true);


			if (link->type==ALIGNMENTRULE_Move) {
				origin(origin()+np-op);

			} else if (link->type==ALIGNMENTRULE_Scale) {
				double scale=norm(op-i1);
				if (scale!=0) {
					scale=norm(np-i1)/scale;
					if (scale!=0) Scale(i1,scale);
				}
			} else if (link->type==ALIGNMENTRULE_ScaleFree) {
				double scalex=op.x-i1.x;
				if (scalex!=0) scalex=(np.x-i1.x)/scalex; else scalex=1;
				double scaley=op.y-i1.y;
				if (scaley!=0) scaley=(np.y-i1.y)/scaley; else scaley=1;
				if (scalex!=0 && scaley!=0) Scale(i1,scalex,scaley);

			} else if (link->type==ALIGNMENTRULE_ScaleX) {
				double scale=op.x-i1.x;
				if (scale!=0) {
					scale=(np.x-i1.x)/scale;
					if (scale!=0 && scale!=1) Scale(i1,scale,1);
				}

			} else if (link->type==ALIGNMENTRULE_ScaleY) {
				double scale=op.y-i1.y;
				if (scale!=0) {
					scale=(np.y-i1.y)/scale;
					if (scale!=0 && scale!=1) Scale(i1,1,scale);
				}

			} else if (link->type==ALIGNMENTRULE_ScaleRotate) {
				RotateScale(i1,op,np);

			} else if (link->type==ALIGNMENTRULE_Rotate) {
				RotatePointed(i1,op,np);

			} else if (link->type==ALIGNMENTRULE_Stretch) {
				Stretch(i1,op,np);

			} else if (link->type==ALIGNMENTRULE_Shear) {
				AnchorShear(i1,i2,op,np);
			}
		}

		link=link->next;
	}
}

///*! Return concatenation of parent transforms.
// * Note this is not valid beyond containing page.
// *
// * If partial>0, then do not use that many upper transforms. For instance, partial==1
// * means get the transform to the parent space, not the object space.
// */
//Laxkit::Affine DrawableObject::GetTransformToContext(bool invert, int partial)
//{
//	DrawableObject *d=this;
//	while (d && partial>0) { d=d->parent; partial--; }
//
//	Affine a;
//	while (d) { 
//		a.Multiply(*dynamic_cast<Affine*>(d));
//		d=d->parent;
//	}
//
//	if (invert) a.Invert();
//	return a;
//}

/*! Returns copy with copies of kids.
 */
LaxInterfaces::SomeData *DrawableObject::duplicate(LaxInterfaces::SomeData *dup)
{
	DrawableObject *d=dynamic_cast<DrawableObject*>(dup);
	if (dup && !d) return NULL; //not a drawableobject!
	if (!dup) {
		dup=newObject("Group");
		d=dynamic_cast<DrawableObject*>(dup);
	}

	//assign new id
	//parent??
	//clip
	//wrap
	//inset
	//ignore chains
	//meta? and tags
	//iohints

	 //filter
	d->alpha = alpha;
	d->blur  = blur;
	if (filter) {
		ObjectFilter *ofilter = dynamic_cast<ObjectFilter*>(filter);
		d->filter = ofilter->Duplicate();
	}

	 //kids
	SomeData *obj;
	DrawableObject *dobj;
	for (int c=0; c<kids.n; c++) {
		obj=kids.e[c]->duplicate(NULL);
		dobj=dynamic_cast<DrawableObject*>(obj);
		d->push(obj);
		dobj->parent=d;
		obj->dec_count();
	}

	return dup;
}

/*! Default is to return clip_path if it exists, or a bounding box path.
 */
LaxInterfaces::PathsData *DrawableObject::GetAreaPath()
{
	if (clip_path) return clip_path;

	 //contsruct bounding box path
	clip_path=new PathsData;
	clip_path->appendRect(minx,miny, maxx-minx,maxy-miny);
	return clip_path;
}

/*! Return an inset path, may or may not be inset_path, where streams are laid into.
 *
 * if no inset_path, then return GetAreaPath().
 */
LaxInterfaces::PathsData *DrawableObject::GetInsetPath()
{
	if (inset_path) return inset_path;
	return GetAreaPath();
}

/*! Path inside which external streams can't go.
 *
 * Default is to return wrap_path. Returns NULL if no wrap_path.
 */
LaxInterfaces::PathsData *DrawableObject::GetWrapPath()
{
	if (wrap_path) return wrap_path;
	return NULL;
}


/*! Default is just the 9 points of corners, midpoints, and the middle of bounding boxes.
 * There will always be at least 9 anchors. These are read only. Any more are
 * found on the anchors stack.
 */
int DrawableObject::NumAnchors()
{
	return 9+anchors.n;
	//int num=9;
	//for (int c=0; c<anchors.n; c++) if (anchors.e[c]->id>=BBOXANCHOR_MAX) num++;
	//return num;
}

/*! If nid<0 then create a random id for it. Otherwise, currently nid MUST be
 * larger than BBOXANCHOR_MAX or it will not be added, and 1 returned.
 *
 * type must currently be PANCHOR_BBox or PANCHOR_Absolute, or error and 1 returned.
 *
 * Return 0 for success, or nonzero for unable to add.
 */
int DrawableObject::AddAnchor(const char *name, flatpoint pos, int type, int nid)
{
	if (nid<0) nid=getUniqueNumber();
	if (nid<=BBOXANCHOR_MAX) return 1;
	if (type!=PANCHOR_BBox && type!=PANCHOR_Absolute) return 1;

	PointAnchor *a=new PointAnchor(name,type,pos,pos,nid);
	anchors.push(a);
	a->dec_count();

	return 0;
}

/*! Return 1 for error (such as trying to remove a standard bbox anchor) or 0 for success.
 */
int DrawableObject::RemoveAnchor(int anchor_id)
{
	if (anchor_id<BBOXANCHOR_MAX) return 1;

	for (int c=0; c<anchors.n; c++) {
		if (anchors.e[c]->id==anchor_id) {
			return RemoveAnchorI(c+9);
		}
	}

	return 1;
}

int DrawableObject::RemoveAnchorI(int index)
{
	if (index<9 || index>=anchors.n) return 1;
	anchors.remove(index);
	return 0;
}

/*! Return the id and optionally the index for the anchor with given id.
 */
int DrawableObject::FindAnchorId(const char *name, int *index_ret)
{
	const char *nm=NULL;
	int id=-1;

	for (int c=0; c<NumAnchors(); c++) {
		GetAnchorInfoI(c,&id, &nm, NULL,NULL,false);
		if (nm && !strcmp(nm,name)) {
			if (index_ret) *index_ret=c;
			return id;
		}
	}

	if (index_ret) *index_ret=-1;
	return -1;
}

/*! Like GetAnchorInfo(), but by index rather than id.
 * anchor_index must be in [0..NumAnchors()).
 *
 * If transform_to_parent, then returned point is in object's parent space regardless of anchor_type.
 * In this case, anchor_type is set to PANCHOR_Absolute.
 * Else is either object coordinates, or object bbox, according to the anchor's type
 * (PANCHOR_Absolute or PANCHOR_BBox).
 *
 * Return 1 for found, or 0 for not found.
 */
int DrawableObject::GetAnchorInfoI(int anchor_index, int *id, const char **name, flatpoint *p, int *anchor_type, bool transform_to_parent)
{
	if (anchor_index>=0 && anchor_index<9) {
		int anchor_id=anchor_index+1;

		const char *nname=NULL;
		flatpoint pp;

		if (anchor_id==BBOXANCHOR_ul)      { nname="top left";     pp=flatpoint( 0, 1); }
		else if (anchor_id==BBOXANCHOR_um) { nname="top";          pp=flatpoint(.5, 1); }
		else if (anchor_id==BBOXANCHOR_ur) { nname="top right";    pp=flatpoint( 1, 1); }
		else if (anchor_id==BBOXANCHOR_ml) { nname="left";         pp=flatpoint( 0,.5); }
		else if (anchor_id==BBOXANCHOR_mm) { nname="middle";       pp=flatpoint(.5,.5); }
		else if (anchor_id==BBOXANCHOR_mr) { nname="right";        pp=flatpoint( 1,.5); }
		else if (anchor_id==BBOXANCHOR_ll) { nname="bottom left";  pp=flatpoint( 0, 0); }
		else if (anchor_id==BBOXANCHOR_lm) { nname="bottom";       pp=flatpoint(.5, 0); }
		else if (anchor_id==BBOXANCHOR_lr) { nname="bottom right"; pp=flatpoint( 1, 0); }

		if (anchor_type) *anchor_type=PANCHOR_BBox;

		if (transform_to_parent) {
			if (anchor_type) *anchor_type=PANCHOR_Absolute;
			pp.x=pp.x*(maxx-minx)+minx;
			pp.y=pp.y*(maxy-miny)+miny;
			pp=transformPoint(pp);
		}

		if (p) *p=pp;
		if (id) *id=anchor_id;
		if (name) *name=nname;

		return 1;

	}
	
	anchor_index-=9;
	if (anchor_index<0 || anchor_index>=anchors.n) return 0;

	PointAnchor *a=anchors.e[anchor_index];

	if (id) *id=a->id;
	if (name) *name=a->name;
	if (p) {
		flatpoint pp=a->p;
		if (transform_to_parent) {
			if (a->anchor_type==PANCHOR_BBox) {
				pp.x=pp.x*(maxx-minx)+minx;
				pp.y=pp.y*(maxy-miny)+miny;
			}
			pp=transformPoint(pp);
			if (anchor_type) *anchor_type=PANCHOR_Absolute;
		} else if (anchor_type) *anchor_type=a->anchor_type;

		*p=pp;
	}

	return 1;
}

/*! Return 0 for anchor found, else 1. 
 * If transform_to_parent, then returned point is in object's parent space, else is object coordinates,
 * regardless if it is type PANCHOR_BBox.
 */
int DrawableObject::GetAnchorInfo(int anchor_id, const char **name, flatpoint *p, int *anchor_type, bool transform_to_parent)
{
	int index=-1;

	if (anchor_id>=BBOXANCHOR_ul && anchor_id<BBOXANCHOR_MAX) index=anchor_id-1;
	else if (anchor_id>=BBOXANCHOR_MAX) {
		for (int c=0; c<anchors.n; c++) {
			if (anchors.e[c]->id==anchor_id) index=c+9;
		}
	}
	if (index==-1) return 0;

	return GetAnchorInfoI(index, NULL,name,p,anchor_type,transform_to_parent);
}

/*! Returns pointer to actual anchor info, if possible. Return 0 if actual anchor returned.
 * Return -1 if a copy of the anchor is returned (for the default bbox anchors).
 * Return 1 for anchor not found.
 *
 * Calling code must call dec_count() on the anchor when done.
 */
int DrawableObject::GetAnchorI(int anchor_index, PointAnchor **anchor)
{
	if (anchor_index>=0 && anchor_index<9) {
		return GetAnchor(anchor_index+1, anchor);
	}

	if (anchor_index<0 || anchor_index>=anchors.n) { *anchor=NULL; return 1; }

	*anchor=anchors.e[anchor_index];
	(*anchor)->inc_count();
	return 0;
}

/*! Returns pointer to actual anchor info, if possible. Return 0 if actual anchor returned.
 * Return -1 if a copy of the anchor is returned (for the default bbox anchors).
 * Return 1 for anchor not found.
 *
 * Calling code must call dec_count() on the anchor when done.
 */
int DrawableObject::GetAnchor(int anchor_id, PointAnchor **anchor)
{
	if (anchor_id>=BBOXANCHOR_ul && anchor_id<BBOXANCHOR_MAX) {
		const char *name;
		flatpoint p;
		int type;
		GetAnchorInfo(anchor_id, &name, &p,&type, false);
		*anchor=new PointAnchor(name,type,p,p,anchor_id);
		(*anchor)->owner=this;
		return -1;
	}
	for (int c=0; c<anchors.n; c++) {
		if (anchors.e[c]->id==anchor_id) {
			*anchor=anchors.e[c];
			(*anchor)->owner=this;
			(*anchor)->inc_count();
			return 0;
		}
	}
	*anchor=NULL;
	return 1;
}

//! Append all the bboxes of the objects.
void DrawableObject::FindBBox()
{
	//if (flags&DRAWABLEOBJ_Fixed_Bounds) return;

	maxx=minx-1;
	maxy=miny-1;
	if (!kids.n) return;

	DrawableObject *o;
	for (int c=0; c<kids.n; c++) {
		o=dynamic_cast<DrawableObject*>(kids.e[c]);
		if (o && o->parent_link) continue; //skip when there are alignment rules
		addtobounds(kids.e[c]->m(),kids.e[c]);
	}
}

////! Check the point against all objs.
///*! \todo *** this is broken! ignores the obj transform
// */
//int DrawableObject::pointin(flatpoint pp,int pin)
//{ 
//	if (!kids.n) return 0;
//	if (!selectable) return 0;
//	flatpoint p(((pp-origin())*xaxis())/(xaxis()*xaxis()), 
//		        ((pp-origin())*yaxis())/(yaxis()*yaxis()));
//	for (int c=0; c<kids.n; c++) {
//		if (kids.e[c]->pointin(p,pin)) return 1;
//	}
//	return 0;
//}


//! Normally return kids.n, but return 0 if the object has locked kids(???).
int DrawableObject::n()
{
	//if (SomeData::flags&(SOMEDATA_LOCK_KIDS|SOMEDATA_LOCK_CONTENTS)) return 0;
	return kids.n;
}

//! Return object with index i in stack.
/*! Note that the object's count is not changed. If the calling code wants to hang
 * on to the object they should quickly inc_count the object.
 */
LaxInterfaces::SomeData *DrawableObject::e(int i)
{
	if (i<0 || i>=kids.n) return NULL;
	return kids.e[i];
}

Laxkit::anObject *DrawableObject::object_e(int i)
{
	if (i>=0 && i<kids.n) return kids.e[i];
	return NULL;
}

const char *DrawableObject::object_e_name(int i)
{
	if (i>=0 && i<kids.n) return kids.e[i]->Id();
	return NULL;
}

const double *DrawableObject::object_transform(int i)
{
	if (i<0 || i>=kids.n) return NULL;
	return kids.e[i]->m();
}

//! Take all the elements in the list which, and put them in a new group at the smallest index.
/*! If any of which are not in kids, then nothing is changed. If ne<=0 then the which list
 * is assumed to be terminated by a -1.
 *
 * Return 0 for success, or nonzero error.
 */
int DrawableObject::GroupObjs(int ne, int *which, int *newgroupindex)
{
	int status = GroupData::GroupObjs(ne,which,newgroupindex);
	if (status!=0) return status;
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

//! If element which is a Group, then make its elements direct elements of this, and remove the group.
/*! Return 0 for success, or nonzero error.
 */
int DrawableObject::UnGroup(int which)
{
	int status = GroupData::UnGroup(which);
	if (status!=0) return status;
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

//! Ungroup some descendent of this Group.
/*! which is list of indices of subgroup. So say which=={1,3,6},
 * then ungroup the element this->1->3->6, which is a great
 * grandchild of this. All intervening elements must be Group objects
 * as must be the final object. Then the other UnGroup() is called.
 *
 * If n<=0, then which is assumed to be a -1 terminated list.
 *
 *  Return 0 for success, or nonzero error.
 */
int DrawableObject::UnGroup(int n,const int *which)
{
	int status = GroupData::UnGroup(n,which);
	if (status!=0) return status;
	laidout->notifyDocTreeChanged(NULL,TreeObjectReorder,0,0);
	return 0;
}

const char *DrawableObject::Id()
{ return SomeData::Id(); }

const char *DrawableObject::Id(const char *newid)
{ return SomeData::Id(newid); }

//! Dump out iohints and metadata, if any.
void DrawableObject::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sid nameofobject\n",spc);
		fprintf(f,"%siohints ...       #(optional) object level i/o leftovers from importing\n",spc);
		fprintf(f,"%smetadata ...      #(optional) object level metadata\n",spc);
		fprintf(f,"%stags tag1 \"tag 2\" #(optional) list of string tags\n",spc);
		fprintf(f,"%sfilter            #(optional) Nodes defining filter transformationss\n",spc);
		fprintf(f,"%salignmentrule align (a1x,a1y) (a2x,a2y)  #(optional) if different than simple matrix\n",spc);
		fprintf(f,"%s  ...\n",spc);
		fprintf(f,"%skids          #child object list...\n",spc);
		//fprintf(f,"%s    ...\n",spc);
		return;
	}

	fprintf(f,"%sid %s\n",spc,Id());

	 //for plain group objects only, dumps out matrix, visible, selectable, locks, min/max
	if (!strcmp(whattype(),"Group")) {
		SomeData::dump_out(f,indent,what,context);
		//otherwise, some of the the base somedata stuff will be in the config section

	} else { 
		 //output just the locks.. most Laxkit objects dump out their own matrix. ignoring min/max for now
		if (visible)    fprintf(f,"%svisible\n",spc);
		if (selectable) fprintf(f,"%sselectable\n",spc);
		fprintf(f,"%slocks ",spc);
		if (locks & OBJLOCK_Contents  ) fprintf(f,"contents ");
		if (locks & OBJLOCK_Position  ) fprintf(f,"position ");
		if (locks & OBJLOCK_Rotation  ) fprintf(f,"rotation ");
		if (locks & OBJLOCK_Scale     ) fprintf(f,"scale ");
		if (locks & OBJLOCK_Shear     ) fprintf(f,"shear ");
		if (locks & OBJLOCK_Kids      ) fprintf(f,"kids ");
		if (locks & OBJLOCK_Selectable) fprintf(f,"selectable ");
		fprintf(f,"\n");
	}


	 // dump notes/meta data
	if (metadata.attributes.n) {
		fprintf(f,"%smetadata\n",spc);
		metadata.dump_out(f,indent+2);
	}
	
	 // dump iohints if any
	if (iohints.attributes.n) {
		fprintf(f,"%siohints\n",spc);
		iohints.dump_out(f,indent+2);
	}

	if (NumberOfTags()) {
		char *str=GetAllTags();
		fprintf(f,"%stags %s\n",spc, str);
		delete[] str;
	}

	if (filter) {
		ObjectFilter *ofilter = dynamic_cast<ObjectFilter*>(filter);
		fprintf(f,"%sfilter\n",spc);
		ofilter->dump_out(f, indent+4,what,context);
	}

	if (anchors.n) {
		PointAnchor *a;
		for (int c=0; c<anchors.n; c++) {
			a=anchors.e[c];
			if (a->name && strchr(a->name,'"')) {
				 // *** HACK! lazy dev...
				cerr << " *** WARNING! unescaped \" character in anchor name.. devs need to implement protection!!"<<endl;
			}
			if (a->anchor_type==PANCHOR_BBox) {
				fprintf(f,"%sanchor \"%s\" bbox %.10g,%.10g\n", spc, a->name, a->p.x,a->p.y);
			} else if (a->anchor_type==PANCHOR_Absolute) {
				fprintf(f,"%sanchor \"%s\" point %.10g,%.10g\n", spc, a->name, a->p.x,a->p.y);
			}
		}
	}

	if (parent_link) {
		AlignmentRule *link=parent_link;
		flatpoint p;
		//PointAnchor *anchor=NULL;

		while (link) {
			if (link->type==ALIGNMENTRULE_Matrix) {
				//ignore matrix ones, they are no-op currently

			} if (link->type==ALIGNMENTRULE_Align) {
				fprintf(f,"%salignmentrule  align (%.10g,%.10g) (%.10g,%.10g)\n", spc,
						link->align1.x, link->align1.y,
						link->align2.x, link->align2.y);

			} if (link->type==ALIGNMENTRULE_EdgeMagnet) {
				cerr << " *** WARNING! alignment rule edgemagnet save not implemented!"<<endl;

			} if (link->type==ALIGNMENTRULE_Code) {
				cerr << " *** WARNING! alignment rule code save not implemented!"<<endl;

			} else {
				fprintf(f,"%salignmentrule %s\n", spc, AlignmentRulePlainName(link->type));
				const char *nm=NULL;
				if (link->type!=ALIGNMENTRULE_Move) {
					GetAnchorInfo(link->invariant1, &nm, NULL,NULL, false);
					if (link->type==ALIGNMENTRULE_Shear) {
						const char *nm2=NULL;
						GetAnchorInfo(link->invariant2, &nm2, NULL,NULL, false);
						fprintf(f,"%s  constant \"%s\" \"%s\"\n",spc, nm,nm2);
					} else {
						fprintf(f,"%s  constant \"%s\"\n",spc, nm);
					}
				}
				GetAnchorInfo(link->object_anchor, &nm, NULL,NULL, false);
				fprintf(f,"%s  point \"%s\"\n",spc, nm);

				if (link->target) {
					if (link->target->owner==parent) {
						fprintf(f,"%s  target parent.\"%s\"\n", spc, link->target->name);
					} else if (link->target->owner==NULL || link->target_location==AlignmentRule::PAGE) {
						fprintf(f,"%s  target page.\"%s\"\n", spc, link->target->name);
					} else {
						if (dynamic_cast<DrawableObject*>(link->target->owner))
						fprintf(f,"%s  target object(\"%s\").\"%s\"\n", spc,
								dynamic_cast<DrawableObject*>(link->target->owner)->Id(),link->target->name);
					}
				}

				if (link->constraintype==AlignmentRule::PARENT) 
					fprintf(f,"%s  constrain parent (%.10g,%.10g)\n", spc, link->constraindir.x,link->constraindir.y);
				else if (link->constraintype==AlignmentRule::OBJECT) 
					fprintf(f,"%s  constrain object (%.10g,%.10g)\n", spc, link->constraindir.x,link->constraindir.y);
				else if (link->constraintype==AlignmentRule::PAGE) 
					fprintf(f,"%s  constrain page (%.10g,%.10g)\n", spc, link->constraindir.x,link->constraindir.y);

				if (link->offset_units==AlignmentRule::PARENT) 
					fprintf(f,"%s  offset parent (%.10g,%.10g)\n", spc, link->offset.x,link->offset.y);
				else if (link->offset_units==AlignmentRule::OBJECT) 
					fprintf(f,"%s  offset object (%.10g,%.10g)\n", spc, link->offset.x,link->offset.y);
				else if (link->offset_units==AlignmentRule::PAGE) 
					fprintf(f,"%s  offset page (%.10g,%.10g)\n", spc, link->offset.x,link->offset.y);
			}

			link=link->next;
		}
	}

	if (properties.n()) {
		fprintf(f, "%sproperties\n",spc);
		for (int c=0; c<properties.n(); c++) {
			properties.e(c)->dump_out(f,indent+2, what,context);
		}
	}

	if (kids.n) {
		fprintf(f,"%skids\n",spc);
		dump_out_group(f,indent+2,what,context, true);
	}
}

void DrawableObject::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	char *name,*value;
	int foundconfig=0;
	if (!strcmp(whattype(),"Group")) foundconfig=-1;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			SomeData::Id(value);

		} else if (!strcmp(name,"visible")) {
            visible = BooleanAttribute(value);
        } else if (!strcmp(name,"selectable")) {
            selectable = BooleanAttribute(value);

        } else if (!strcmp(name,"locks")) {
			if (isblank(value)) continue;

            int n=0;
            char **strs = splitspace(value, &n);
            for (int c=0; c<n; c++) {
                if      (!strcmp(strs[c],"contents"  )) locks |= OBJLOCK_Contents  ;
                else if (!strcmp(strs[c],"position"  )) locks |= OBJLOCK_Position  ;
                else if (!strcmp(strs[c],"rotation"  )) locks |= OBJLOCK_Rotation  ;
                else if (!strcmp(strs[c],"scale"     )) locks |= OBJLOCK_Scale     ;
                else if (!strcmp(strs[c],"shear"     )) locks |= OBJLOCK_Shear     ;
                else if (!strcmp(strs[c],"kids"      )) locks |= OBJLOCK_Kids      ;
                else if (!strcmp(strs[c],"selectable")) locks |= OBJLOCK_Selectable;
            }
            deletestrs(strs, n);

		} else if (!strcmp(name,"iohints")) {
			if (iohints.attributes.n) iohints.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				iohints.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		} else if (!strcmp(name,"metadata")) {
			if (metadata.attributes.n) metadata.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				metadata.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		} else if (!strcmp(name,"anchor")) {
			 //anchor value will be something like: "name" bbox 1.5,3.5
			char *ptr=NULL;
			char *name=QuotedAttribute(value,&ptr);
			if (!name) continue;
			value=ptr;
			char *type=QuotedAttribute(value,&ptr);
			if (!type) { delete[] name; continue; }

			flatvector p;
			value=ptr;
			if (FlatvectorAttribute(value,&p,&ptr)==0) { delete[] name; delete[] type; continue; }//was error in reading!
			
			AddAnchor(name,p,(!strcmp(type,"bbox") ? PANCHOR_BBox : PANCHOR_Absolute), -1);

			delete[] name;
			delete[] type;

		} else if (!strcmp(name,"alignmentrule")) {
			if (!strncmp(value,"matrix",6)) continue; //we assume matrix anyway

			if (!strncmp(value,"align",5)) {
				flatvector a1,a2;
				char *end;
				if (FlatvectorAttribute(value,&a1,&end)==0) continue; //was error in reading!
				value=end;
				if (FlatvectorAttribute(value,&a2,&end)==0) continue; //was error in reading!

				AlignmentRule *rule=new AlignmentRule;
				rule->type=ALIGNMENTRULE_Align;
				rule->align1=a1;
				rule->align2=a2;
				AddAlignmentRule(rule, false, -1);

			} else if (!strcmp(value,"edgemagnet")) {
				cerr << " *** WARNING! alignment rule edgemagnet load not implemented!"<<endl;

			} else if (!strcmp(value,"code")) {
				cerr << " *** WARNING! alignment rule code load not implemented!"<<endl;

			} else {
				int type=AlignmentRuleFromPlainName(value);
				if (type==ALIGNMENTRULE_None) {
					cerr <<" WARNING! unknown alignment type "<<value<<endl;
					continue;
				}

				int error=0;
				int i1=-1, i2=-1;
				int anchor=-1;

				flatpoint offset;
				AlignmentRule::TargetType offsetunits=AlignmentRule::UNKNOWN;

				flatpoint constraint;
				AlignmentRule::TargetType constraintunits=AlignmentRule::UNKNOWN;

				AlignmentRule::TargetType target_location=AlignmentRule::UNKNOWN; //0 for parent, 1 for object, 2 for page
				char *target_object_name=NULL;
				char *target_anchor_name=NULL;

				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++)  {
					name= att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;

					if (!strcmp(name,"constant")) {
						char *end_ptr=NULL;
						char *nm=QuotedAttribute(value, &end_ptr);

						if (nm) {
							i1=FindAnchorId(nm,NULL);
							delete[] nm;

							value=end_ptr;
							nm=QuotedAttribute(value, &end_ptr);
							if (nm) {
								i2=FindAnchorId(nm,NULL);
								delete[] nm;
							}
						}

					} else if (!strcmp(name,"point")) {
						anchor=FindAnchorId(value,NULL);

					} else if (!strcmp(name,"target")) {
						if (strncmp(value,"object",6)==0) {
							 //refers to some random object somewhere on the page
							value+=6;
							while (isspace(*value)) value++;
							if (*value=='(') {
								value++;
								char *end_ptr=NULL;
								delete[] target_object_name;
								target_object_name=QuotedAttribute(value, &end_ptr);
								value=end_ptr;
								if (target_object_name && *value==')') {
									value++;
									if (*value=='.') value++;
									delete[] target_anchor_name;
									target_anchor_name=QuotedAttribute(value, &end_ptr);
									target_location=AlignmentRule::OTHER_OBJECT;
								}
							}

						} else if (strncmp(value,"parent",6)==0) {
							value+=6;
							while (isspace(*value)) value++;
							if (*value=='.') value++;
							char *end_ptr=NULL;
							target_anchor_name=QuotedAttribute(value, &end_ptr);
							if (target_anchor_name) target_location=AlignmentRule::PARENT;

						} else if (strncmp(value,"page",4)==0) {
							value+=4;
							while (isspace(*value)) value++;
							if (*value=='.') value++;
							while (isspace(*value)) value++;
							char *end_ptr=NULL;
							delete[] target_anchor_name;
							target_anchor_name=QuotedAttribute(value, &end_ptr);
							if (target_anchor_name) target_location=AlignmentRule::PAGE;
						}

					} else if (!strcmp(name,"offset")) {
						 //units 0 for parent, 1 for object, 2 for page
						if (strncmp(value,"object",6)==0)      { value+=6;  offsetunits=AlignmentRule::OBJECT; }
						else if (strncmp(value,"parent",6)==0) { value+=6;  offsetunits=AlignmentRule::PARENT; }
						else if (strncmp(value,"page",4)==0)   { value+=4;  offsetunits=AlignmentRule::PAGE;   }
						while (isspace(*value)) value++;

						if (FlatvectorAttribute(value,&offset,NULL)==0) {
							error=1;
						}

					} else if (!strcmp(name,"constrain")) {
						if (strncmp(value,"object",6)==0)      { value+=6;  constraintunits=AlignmentRule::OBJECT; }
						else if (strncmp(value,"parent",6)==0) { value+=6;  constraintunits=AlignmentRule::PARENT; }
						else if (strncmp(value,"page",4)==0)   { value+=4;  constraintunits=AlignmentRule::PAGE;   }
						while (isspace(*value)) value++;

						if (FlatvectorAttribute(value,&constraint,NULL)==0) {
							error=1;
						}
					}
				}
				
				PointAnchor *target=NULL;
				if (!error) {
					if (target_location==AlignmentRule::PARENT) {
						DrawableObject *d = dynamic_cast<DrawableObject*>(parent);

						int target_id = d->FindAnchorId(target_anchor_name,NULL);
						if (target_id>=0) d->GetAnchor(target_id, &target);
						if (target==NULL) error=1;
						else {
							delete[] target_anchor_name;
							delete[] target_object_name;
						}
					}
				}


				if (!error) {
							// *** create anchor stub to fill in later?
					AlignmentRule *rule=new AlignmentRule;
					rule->type=type;
					rule->object_anchor=anchor;
					rule->invariant1=i1;
					rule->invariant2=i2;
					rule->offset=offset;
					rule->offset_units=offsetunits;
					rule->constraindir=constraint;
					rule->constraintype=constraintunits;
					rule->target=NULL; //target.. delay because parent bbox not fully formed yet
					rule->target_location=target_location;
					rule->target_anchor=target_anchor_name;
					rule->target_object=target_object_name;
					AddAlignmentRule(rule, false, -1);
				} else {
					delete[] target_anchor_name;
					delete[] target_object_name;
				}

			}
			continue;

		} else if (!strcmp(name,"tags")) {
			InsertTags(value,0);

		} else if (!strcmp(name,"kids")) {
			//dump_in_group_atts(att->attributes.e[c], flag,context, false);
			dump_in_group_atts(att->attributes.e[c], flag,context, true); //for backwards compatibility

		} else if (foundconfig==0 && !strcmp(name,"config")) {
			foundconfig=1;

		} else if (!strcmp(name,"filter")) {
			ObjectFilter *ofilter = new ObjectFilter(this);
			ofilter->dump_in_atts(att->attributes.e[c], 0, context);
		}
	}

	 //is plain group, need to grab the base somedata stuff
	if (foundconfig==-1) SomeData::dump_in_atts(att, flag,context);
}

/*! Recursively map any unmapped anchors. Assume we are on the given page.
 * Returns the number of items adjusted.
 *
 * \todo use targets in any attached page bleeds
 */
int DrawableObject::ResolveAnchorRefs(Document *doc, Page *page, Group *g, Laxkit::ErrorLog &log)
{
	int adjusted=0;
	DrawableObject *oo;
	anObject *own;

	for (int c=0; c<g->n(); c++) {
        oo=dynamic_cast<DrawableObject*>(g->e(c));
		if (!oo) continue;

        if (oo->parent_link) {
             //check for unresolved anchor linkages
            AlignmentRule *rule=oo->parent_link;
            bool needtoupdate=false;
			own=NULL;

            while (rule) {
                 // *** a bit hacky here.. need more reasonable system of dependency checking
                if (  rule->type!=ALIGNMENTRULE_Matrix &&
                      rule->type!=ALIGNMENTRULE_EdgeMagnet &&
                      rule->type!=ALIGNMENTRULE_Code &&
                      rule->type!=ALIGNMENTRULE_Align &&
                      rule->target==NULL && rule->target_anchor!=NULL) {
    
                    DrawableObject *ooo=NULL;
                    if (rule->target_location==AlignmentRule::PARENT) {
                        ooo=dynamic_cast<DrawableObject*>(oo->parent);
						own=ooo;
                    } else if (rule->target_location==AlignmentRule::OTHER_OBJECT) {
                        ooo=dynamic_cast<DrawableObject*>(FindChild(rule->target_object));
						own=ooo;
                    } else if (rule->target_location==AlignmentRule::PAGE) {
						ooo=&page->anchors;
						ooo->setbounds(page->pagestyle->outline);
						own=page;
                    }

                    if (ooo) {
                        PointAnchor *anchor=NULL;
                        int index=-1;
                        ooo->FindAnchorId(rule->target_anchor,&index);
                        ooo->GetAnchorI(index, &anchor);
						anchor->owner=own;
                        if (anchor!=NULL) {
                            rule->target=anchor;
                            needtoupdate=true;
							adjusted++; 
                        } else {
                            log.AddMessage(_("Missing anchor in anchor target object!"),ERROR_Warning);
                        }
                        
                    } else {
                        log.AddMessage(_("Missing anchor target object!"),ERROR_Warning);
                    }

                }

                rule=rule->next;
            }
            if (needtoupdate) oo->UpdateFromRules();
		} //if oo->parent_link

		if (oo->kids.n) adjusted+=ResolveAnchorRefs(doc,page,oo, log);
	} //each object in g

	return adjusted;
}



//-------------- Value functions:

Value *NewDrawableObject(ObjectDef *)
{ return NULL; }
//{ return new DrawableObject; }

//! Return an ObjectDef for a "Group" object.
ObjectDef *DrawableObject::makeObjectDef()
{
	// *** objectdef is in Value...
	ObjectDef *objectdef=stylemanager.FindDef("Group");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef=makeAffineObjectDef(); //always makes a new def
		makestr(objectdef->name,"Group");
		makestr(objectdef->Name,_("Group"));
		makestr(objectdef->description,_("Group of drawable objects, and base of all drawable objects"));
		
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}
	return objectdef;
}

Value *DrawableObject::duplicate()
{
	DrawableObject *o=dynamic_cast<DrawableObject*>(duplicate(NULL));
	return o;
}

Value *DrawableObject::dereference(const char *extstring, int len)
{
	return NULL;
}

int DrawableObject::assign(FieldExtPlace *ext,Value *v)
{
	return -1;
}

int DrawableObject::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, ErrorLog *log)
{
	return -1;
}






//--------------------------------------- AffineValue ---------------------------------------


/* \class AffineValue
 * \brief Adds scripting functions for a Laxkit::Affine object.
 */

AffineValue::AffineValue()
{}

AffineValue::AffineValue(const double *m)
  : Affine(m)
{}

Value *AffineValue::dereference(int index)
{
	if (index<0 || index>=6) return NULL;
	return new DoubleValue(m(index));
}

int AffineValue::getValueStr(char *buffer,int len)
{
    int needed=6*30;
    if (!buffer || len<needed) return needed;

	sprintf(buffer,"[%.10g,%.10g,%.10g,%.10g,%.10g,%.10g]",m(0),m(1),m(2),m(3),m(4),m(5));
    modified=0;
    return 0;
}

Value *AffineValue::duplicate()
{
	AffineValue *dup=new AffineValue(m());
	return dup;
}

ObjectDef *AffineValue::makeObjectDef()
{
	objectdef=stylemanager.FindDef("Affine");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef=makeAffineObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}
	return objectdef;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int AffineValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (len==6 && !strncmp(function,"rotate",6)) {
		int err=0;
		double angle;
		flatpoint p;
		try {
			angle=pp->findIntOrDouble("angle",-1,&err);
			if (err) throw _("Missing angle.");
			err=0;

			FlatvectorValue *fpv=dynamic_cast<FlatvectorValue*>(pp->find("point"));
			if (fpv) p=fpv->v;

			Rotate(angle,p);

		} catch (const char *str) {
			if (log) log->AddMessage(str,ERROR_Fail);
			err=1;
		}
		 
		if (value_ret) *value_ret=NULL;
		return err;

//	} else if (len==8 && !strncmp(function,"array3x3",8)) {
//		ArrayValue *v=new ArrayValue;
//		v->push(new ArrayValue(m(0), m(1), 0));
//		v->push(new ArrayValue(m(2), m(3), 0));
//		v->push(new ArrayValue(m(4), m(5), 1));
//		return v;

	} else if (len==9 && !strncmp(function,"translate",9)) {
		int err=0;
		flatpoint p;
		p.x=pp->findIntOrDouble("x",-1,&err); 
		p.y=pp->findIntOrDouble("y",-1,&err);
		int i=pp->findIndex("p",1);
		if (i>=0 && dynamic_cast<FlatvectorValue*>(pp->e(i))) p=dynamic_cast<FlatvectorValue*>(pp->e(i))->v;
		origin(origin()+p);
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (len==11 && !strncmp(function,"scalerotate",11)) {
		int err=0;
		flatpoint p1,p2,p3;
		p1=pp->findFlatvector("p1",-1,&err);
		p2=pp->findFlatvector("p2",-1,&err);
		p3=pp->findFlatvector("p3",-1,&err);

		if (p1==p2) {
			if (log) log->AddMessage(_("p1 and p2 must be different points"),ERROR_Fail);
			return 1;
		}

		RotateScale(p1,p2,p3);
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (len==11 && !strncmp(function,"anchorshear",11)) {
		int err=0;
		flatpoint p1,p2,p3,p4;
		p1=pp->findFlatvector("p1",-1,&err);
		p2=pp->findFlatvector("p2",-1,&err);
		p3=pp->findFlatvector("p3",-1,&err);
		p4=pp->findFlatvector("p4",-1,&err);

		if (p1==p2) {
			if (log) log->AddMessage(_("p1 and p2 must be different points"),ERROR_Fail);
			return 1;
		}

		AnchorShear(p1,p2,p3,p4);
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (len==4 &&  !strncmp(function,"flip",4)) {
		int err1=0,err2=0;
		flatpoint p1,p2;
		p1=pp->findFlatvector("p1",-1,&err1);
		p2=pp->findFlatvector("p2",-1,&err2);
		if (err1!=0 && err2!=0) p2=flatpoint(1,0);

		if (p1==p2) {
			if (log) log->AddMessage(_("p1 and p2 must be different points"),ERROR_Fail);
			return 1;
		}

		Flip(p1,p2);
		if (value_ret) *value_ret=NULL;
		return 0;


	} else if (len==12 && !strncmp(function,"settransform",12)) {
		int err=0;
		double mm[6];
		mm[0]=pp->findDouble("a",-1,&err);
		mm[1]=pp->findDouble("b",-1,&err);
		mm[2]=pp->findDouble("c",-1,&err);
		mm[3]=pp->findDouble("d",-1,&err);
		mm[4]=pp->findDouble("x",-1,&err);
		mm[5]=pp->findDouble("y",-1,&err);

		if (is_degenerate_transform(mm)) {
			if (log) log->AddMessage(_("Bad matrix!"),ERROR_Fail);
			return 1;
		}

		m(mm);
		if (value_ret) *value_ret=NULL;
		return 0;
	}

	return 1;
}

//! Contructor for AffineValue objects.
int NewAffineObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	Value *v=new AffineValue();
	*value_ret=v;

	if (!parameters || !parameters->n()) return 0;

//	Value *matrix=parameters->find("matrix");
//	if (matrix) {
//		SetValue *set=dynamic_cast<SetValue*>(matrix);
//		if (set && set->GetNumFields()==6) {
//		}
//	}

	return 0;
}

//! Create a new ObjectDef with Affine characteristics. Always creates new one, does not search for Affine globally.
ObjectDef *makeAffineObjectDef()
{
	ObjectDef *sd=new ObjectDef(NULL,"Affine",
			_("Affine"),
			_("Affine transform defined by 6 real numbers."),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NULL,NewAffineObject);


	sd->pushFunction("translate", _("Translate"), _("Move by a certain amount"),
					 NULL, //evaluator
					 "x",_("X"),_("The amount to move in the x direction"),"number", NULL,NULL,
					 "y",_("Y"),_("The amount to move in the y direction"),"number", NULL,NULL,
					 NULL);

	sd->pushFunction("rotate", _("Rotate"), _("Rotate the object, optionally around a point"),
					 NULL, //evaluator
					 "angle",_("Angle"),_("The angle to rotate"),"number", NULL,NULL,
					 "point",_("Point"),_("The point around which to rotate. Default is the origin."),"flatvector", NULL,"(0,0)",
					 NULL);


	sd->pushFunction("scalerotate", _("ScaleRotate"), _("Rotate and scale the object, keeping one point fixed"),
					 NULL,
					 "p1",_("P1"),_("A constant point"),       "flatvector", NULL,NULL,
					 "p2",_("P2"),_("The point to move."),     "flatvector", NULL,NULL,
					 "p3",_("P3"),_("The new position of p2."),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("anchorshear", _("Anchor Shear"), _("Transform so that p1 and p2 stay fixed, but p3 is shifted to newp3."),
					 NULL,
					 "p1",_("P1"),_("A constant point"),       "flatvector", NULL,NULL,
					 "p2",_("P2"),_("Another constant point"), "flatvector", NULL,NULL,
					 "p3",_("P3"),_("The point to move."),     "flatvector", NULL,NULL,
					 "p4",_("P4"),_("The new position of p3."),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("flip", _("Flip"), _("Flip around an axis defined by two points."),
					 NULL,
					 "p1",_("P1"),_("A constant point"),      "flatvector", NULL,NULL,
					 "p2",_("P2"),_("Another constant point"),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("settransform", _("Set Transform"), _("Set the object's affine transform, with a set of 6 real numbers: a,b,c,d,x,y."),
					 NULL,
					 "a",_("A"),_("A"),"real", NULL,NULL,
					 "b",_("B"),_("B"),"real", NULL,NULL,
					 "c",_("C"),_("C"),"real", NULL,NULL,
					 "d",_("D"),_("D"),"real", NULL,NULL,
					 "x",_("X"),_("X"),"real", NULL,NULL,
					 "y",_("Y"),_("Y"),"real", NULL,NULL,
					 NULL);


	return sd;
}


//--------------------------------------- BBoxValue ---------------------------------------


/* \class BBoxValue
 * \brief Adds scripting functions for a Laxkit::DoubleBBox object.
 */

BBoxValue::BBoxValue()
{}

BBoxValue::BBoxValue(double mix,double max,double miy,double may)
  : DoubleBBox(mix,max,miy,may)
{}

Value *BBoxValue::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "minx")) return new DoubleValue(minx);
	if (extequal(extstring,len, "maxx")) return new DoubleValue(maxx);
	if (extequal(extstring,len, "miny")) return new DoubleValue(miny);
	if (extequal(extstring,len, "maxy")) return new DoubleValue(maxy);

	if (extequal(extstring,len, "x"   )) return new DoubleValue(minx);
	if (extequal(extstring,len, "y"   )) return new DoubleValue(miny);
	if (extequal(extstring,len, "width")) return new DoubleValue(maxx-minx);
	if (extequal(extstring,len, "height")) return new DoubleValue(maxy-miny);
	return NULL;
}

int BBoxValue::assign(FieldExtPlace *ext,Value *v)
{
	if (ext->n()!=1) return -1;
	const char *str=ext->e(0);

	int isnum=0;
    double d=getNumberValue(v,&isnum);
    if (!isnum) return 0;

	if (!strcmp(str,"minx")) minx=d;
	else if (!strcmp(str,"maxx")) maxx=d;
	else if (!strcmp(str,"miny")) miny=d;
	else if (!strcmp(str,"maxy")) maxy=d;
	else if (!strcmp(str,"x")) { maxx+=d-minx; minx=d; }
	else if (!strcmp(str,"y")) { maxy+=d-miny; miny=d; }
	else if (!strcmp(str,"width")) maxx=minx+d;
	else if (!strcmp(str,"height")) maxy=miny+d;
	else return -1;

	return 1;
}

//! Return something like [[.5,.5], [1,0]], which is [min,max] for [x,y].
int BBoxValue::getValueStr(char *buffer,int len)
{
    int needed=4*30;
    if (!buffer || len<needed) return needed;

	sprintf(buffer,"[[%.10g,%.10g],[%.10g,%.10g]]",minx,maxx,miny,maxy);
    modified=0;
    return 0;
}

Value *BBoxValue::duplicate()
{
	BBoxValue *dup=new BBoxValue(minx,maxx,miny,maxy);
	return dup;
}

ObjectDef *BBoxValue::makeObjectDef()
{
	objectdef=stylemanager.FindDef("BBox");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef=makeBBoxObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}
	return objectdef;
}

//! Contructor for BBoxValue objects.
int NewBBoxObject(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log)
{
	BBoxValue *v=new BBoxValue();
	*value_ret=v;

	if (!parameters || !parameters->n()) return 0;

	int err=0;
	double d=parameters->findIntOrDouble("maxx",-1,&err);
	if (err==0) v->maxx=d;

	d=parameters->findIntOrDouble("minx",-1,&err);
	if (err==0) v->minx=d;

	d=parameters->findIntOrDouble("maxy",-1,&err);
	if (err==0) v->maxy=d;

	d=parameters->findIntOrDouble("miny",-1,&err);
	if (err==0) v->miny=d;

	d=parameters->findIntOrDouble("y",-1,&err);
	if (err==0) v->miny=d;

	d=parameters->findIntOrDouble("x",-1,&err);
	if (err==0) v->minx=d;

	d=parameters->findIntOrDouble("width",-1,&err);
	if (err==0) v->maxx=v->minx+d;

	d=parameters->findIntOrDouble("height",-1,&err);
	if (err==0) v->maxy=v->miny+d;

	return 0;
}

//! Create a new ObjectDef with BBox characteristics. Always creates new one, does not search for BBox globally.
ObjectDef *makeBBoxObjectDef()
{
	ObjectDef *sd=new ObjectDef(NULL,"BBox",
			_("BBox"),
			_("Bounding box"),
			"class",
			NULL,NULL, //range, default value
			NULL,0, //fields, flags
			NULL,NewBBoxObject);


	 //Contstructor
	sd->pushFunction("BBox", _("Bounding Box"), _("Bounding Box"),
					 NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);

	sd->push("minx",_("Minx"),_("Minimium x"),"real",NULL,NULL,0,0);
	sd->push("maxx",_("Maxx"),_("Maximium x"),"real",NULL,NULL,0,0);
	sd->push("miny",_("Miny"),_("Minimium y"),"real",NULL,NULL,0,0);
	sd->push("maxy",_("Maxy"),_("Maximium y"),"real",NULL,NULL,0,0);

	sd->pushFunction("clear", _("Clear"), _("Clear bounds."),
					 NULL, //evaluator
					 NULL);

	sd->pushFunction("IsValid", _("Is Valid"), _("True if bounds are valid, meaning max values are greater than min values."),
					 NULL, //evaluator
					 NULL);


	sd->pushFunction("Add", _("Add To Bounds"), _("Add a point to bounds"),
					 NULL,
					 "x",_("x"),_("An x coordinate"),"real", NULL,NULL,
					 "y",_("y"),_("A y coordinate"), "real""real", NULL,NULL,
					 "p",_("p"),_("A point"),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("AddBox", _("Add box"), _("Add another bbox to bounds, so that old and new bounds contain both."),
					 NULL,
					 "box", _("box"), _("box"), "BBox", NULL,NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);


	sd->pushFunction("Contains", _("Contains"), _("True if bounds contain point (inside or right on edge)."),
					 NULL,
					 "x",_("x"),_("An x coordinate"),"real", NULL,NULL,
					 "y",_("y"),_("A y coordinate"), "real", NULL,NULL,
					 "p",_("p"),_("A point"),"flatvector", NULL,NULL,
					 NULL);


	sd->pushFunction("Intersects", _("Intersects"), _("Return whether a box intersects."),
					NULL,
					 "box", _("box"), _("box"), "BBox", NULL,NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);

	sd->pushFunction("Intersection", _("Intersection"), _("Return a new box that is the intersection with current."),
					 NULL,
					 "box", _("box"), _("box"), "BBox", NULL,NULL,
					 "minx",_("minx"),_("minx"),"real", NULL,NULL,
					 "maxx",_("maxx"),_("maxx"),"real", NULL,NULL,
					 "miny",_("miny"),_("miny"),"real", NULL,NULL,
					 "maxy",_("maxy"),_("maxy"),"real", NULL,NULL,
					 NULL);

	return sd;
}

/*! Return 0 success, -1 incompatible values, 1 for error.
 */
int BBoxValue::Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
			             Value **value_ret, ErrorLog *log)
{
	if (isName(function,len,"clear")) {
		clear();		 
		if (value_ret) *value_ret=NULL;
		return 0;

	} else if (isName(function,len,"Add")) {
		int err=0;
		flatpoint p;
		p.x=pp->findIntOrDouble("x",-1,&err); 
		p.y=pp->findIntOrDouble("y",-1,&err);
		int i=pp->findIndex("p",1);
		if (i>=0 && dynamic_cast<FlatvectorValue*>(pp->e(i))) p=dynamic_cast<FlatvectorValue*>(pp->e(i))->v;
		addtobounds(p);
		if (value_ret) *value_ret=NULL;
		return 0;

	} if (isName(function,len,"x")) {
		*value_ret=new DoubleValue(minx);
		return 0;

	} if (isName(function,len,"y")) {
		*value_ret=new DoubleValue(miny);
		return 0;

	} if (isName(function,len,"width")) {
		*value_ret=new DoubleValue(maxx-minx);
		return 0;

	} if (isName(function,len,"height")) {
		*value_ret=new DoubleValue(maxy-miny);
		return 0;

	} if (isName(function,len,"IsValid")) {
		*value_ret=new BooleanValue(validbounds());
		return 0;

	} if (isName(function,len,"Contains")) {
		int err=0;
		flatpoint p;
		p.x=pp->findIntOrDouble("x",-1,&err); 
		p.y=pp->findIntOrDouble("y",-1,&err);
		int i=pp->findIndex("p",1);
		if (i>=0 && dynamic_cast<FlatvectorValue*>(pp->e(i))) p=dynamic_cast<FlatvectorValue*>(pp->e(i))->v;
		*value_ret=new BooleanValue(boxcontains(p.x,p.y));
		return 0;

	} if (isName(function,len,"AddBox")) {
		DoubleBBox box;
		Value *v=pp->find("box");
		if (v && dynamic_cast<BBoxValue*>(v)) box=*dynamic_cast<DoubleBBox*>(v);
		double d;
		int err=0;
		d=pp->findIntOrDouble("minx",-1,&err); if (err==0) box.minx=d;
		d=pp->findIntOrDouble("maxx",-1,&err); if (err==0) box.maxx=d;
		d=pp->findIntOrDouble("miny",-1,&err); if (err==0) box.miny=d;
		d=pp->findIntOrDouble("maxy",-1,&err); if (err==0) box.maxy=d;
		addtobounds(&box);

	} if (isName(function,len,"Intersects")) {
		DoubleBBox box;
		Value *v=pp->find("box");
		if (v && dynamic_cast<BBoxValue*>(v)) box=*dynamic_cast<DoubleBBox*>(v);
		double d;
		int err=0;
		d=pp->findIntOrDouble("minx",-1,&err); if (err==0) box.minx=d;
		d=pp->findIntOrDouble("maxx",-1,&err); if (err==0) box.maxx=d;
		d=pp->findIntOrDouble("miny",-1,&err); if (err==0) box.miny=d;
		d=pp->findIntOrDouble("maxy",-1,&err); if (err==0) box.maxy=d;
		*value_ret=new BooleanValue(intersect(&box,0));
		return 0;

	} if (isName(function,len,"Intersection")) {
		BBoxValue *box=new BBoxValue;
		Value *v=pp->find("box");
		if (v && dynamic_cast<BBoxValue*>(v)) *box=*dynamic_cast<BBoxValue*>(v);
		double d;
		int err=0;
		d=pp->findIntOrDouble("minx",-1,&err); if (err==0) box->minx=d;
		d=pp->findIntOrDouble("maxx",-1,&err); if (err==0) box->maxx=d;
		d=pp->findIntOrDouble("miny",-1,&err); if (err==0) box->miny=d;
		d=pp->findIntOrDouble("maxy",-1,&err); if (err==0) box->maxy=d;
		box->intersect(this,1);
		*value_ret=box;
		return 0;
	}

	return 1;
}



} //namespace Laidout

