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
// Copyright (C) 2019 by Tom Lechner
//


#include <lax/language.h>
#include <lax/interfaces/svgcoord.h>

#include "../laidout.h"

#include "nodeinterface.h"
#include "../dataobjects/lcaptiondata.h"
#include "../dataobjects/limagedata.h"
#include "../dataobjects/lsomedataref.h"
#include "../dataobjects/lpathsdata.h"
#include "../dataobjects/bboxvalue.h"
#include "../dataobjects/affinevalue.h"
#include "../dataobjects/pointsetvalue.h"
#include "../dataobjects/lvoronoidata.h"
#include "../dataobjects/imagevalue.h"
#include "../dataobjects/fontvalue.h"
#include "../core/objectiterator.h"

#include <unistd.h>


//template implementation
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace LaxInterfaces;

namespace Laidout {


//------------------------ DuplicateDrawableNode ------------------------

/*! 
 * Node to create an array of duplicates or clones of a drawable
 */

class DuplicateDrawableNode : public NodeBase
{
  public:
	DuplicateDrawableNode();
	virtual ~DuplicateDrawableNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new DuplicateDrawableNode(); }
};


DuplicateDrawableNode::DuplicateDrawableNode()
{
	makestr(Name, _("Duplicate Drawable"));
	makestr(type, "Drawables/DuplicateDrawable");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "clone", new BooleanValue(false),1, _("As clones"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "n", new IntValue(1),1, _("How many"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

DuplicateDrawableNode::~DuplicateDrawableNode()
{
}

NodeBase *DuplicateDrawableNode::Duplicate()
{
	DuplicateDrawableNode *node = new DuplicateDrawableNode();
	node->DuplicateBase(this);
	return node;
}

int DuplicateDrawableNode::GetStatus()
{
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) return -1;

	int isnum;
	getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	
	return NodeBase::GetStatus();
}

int DuplicateDrawableNode::Update()
{
	Error(nullptr);

	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) return -1;

	int isnum;
	bool clone = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	int n = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	
	if (n <= 0) {
		Error(_("How many must be > 0"));
		return -1;
	}

	SetValue *set = dynamic_cast<SetValue*>(properties.e[3]->GetData());
	if (!set) {
		set = new SetValue();
		properties.e[3]->SetData(set, 1);
	} else {
		// while (set->n() > n) set->remove(set->n()-1);
		set->values.flush();
	}
	
	DBG Utf8String str;
	if (clone) {
		for (int c=0; c<n; c++) {
			LSomeDataRef *oo = new LSomeDataRef(o);
			DBG str.Sprintf("%s_%d", o->Id(), c);
			DBG dynamic_cast<anObject*>(oo)->Id(str.c_str());
			oo->FindBBox();
			set->Push(oo, 1);
		}
	} else {
		for (int c=0; c<n; c++) {
			DrawableObject *oo = dynamic_cast<DrawableObject*>(o->duplicate());
			DBG str.Sprintf("%s_%d", o->Id(), c);
			DBG oo->Id(str.c_str());
			oo->FindBBox();
			set->Push(oo, 1);
		}
	}

	// UpdatePreview();
	// Wrap();

	properties.e[3]->Touch();
	return NodeBase::Update();
}

int DuplicateDrawableNode::UpdatePreview()
{
	return 1;
}


//------------------------ DuplicateSingleNode ------------------------

/*! 
 * Node to create an array of duplicates or clones of a drawable
 */

class DuplicateSingleNode : public NodeBase
{
  public:
	DuplicateSingleNode();
	virtual ~DuplicateSingleNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new DuplicateSingleNode(); }
};


DuplicateSingleNode::DuplicateSingleNode()
{
	makestr(Name, _("Duplicate single"));
	makestr(type, "Drawables/DuplicateSingle");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

DuplicateSingleNode::~DuplicateSingleNode()
{
}

NodeBase *DuplicateSingleNode::Duplicate()
{
	DuplicateSingleNode *node = new DuplicateSingleNode();
	node->DuplicateBase(this);
	return node;
}

int DuplicateSingleNode::GetStatus()
{
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) return -1;
	return NodeBase::GetStatus();
}

int DuplicateSingleNode::Update()
{
	Error(nullptr);

	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) {
		Error(_("In must be a Drawable"));
		return -1;
	}

	anObject *filter = o->filter;
	if (filter) o->filter = nullptr;
	DrawableObject *dup = dynamic_cast<DrawableObject*>(o->duplicate(NULL));
	// *** duplicate filter separately for safety... otherwise needs a ton of overloaded funcs to deal with.. needs more thought
	// *** just ignore filter for now
	// if (filter) {
	// 	ObjectFilter *nfilter = dynamic_cast<ObjectFilter*>(dynamic_cast<ObjectFilter*>(filter)->Duplicate());
	// 	dup->filter = nfilter;
	// 	nfilter->SetParent(dup);
	// }
	if (filter) o->filter = filter; //put filter back on
	dup->FindBBox();
	properties.e[1]->SetData(dup,1);
	return NodeBase::Update();
}

int DuplicateSingleNode::UpdatePreview()
{
	return 1;
}


//----------------------- KidsToSetNode ------------------------

/*! \class KidsToSetNode
 *
 * Return a set of children of a DrawableObject.
 */
class KidsToSetNode : public NodeBase
{
  public:
	KidsToSetNode();
	virtual ~KidsToSetNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new KidsToSetNode(); }
};

KidsToSetNode::KidsToSetNode()
{
	makestr(type, "Drawable/KidsToSet");
	makestr(Name, _("Set of kids"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",  NULL,1, _("Parent"), _("A drawable")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new SetValue(),1, _("Set of kids"), NULL,0, false));
}

KidsToSetNode::~KidsToSetNode()
{
}

NodeBase *KidsToSetNode::Duplicate()
{
	KidsToSetNode *node = new KidsToSetNode();
	node->DuplicateBase(this);
	return node;
}

int KidsToSetNode::GetStatus()
{
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) {
		Error(_("Parent must be a DrawableObject"));
		return -1;
	}
	return NodeBase::GetStatus();
}

int KidsToSetNode::Update()
{
	Error(nullptr);
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) {
		Error(_("Parent must be a DrawableObject"));
		return -1;
	}

	SetValue *out = dynamic_cast<SetValue*>(properties.e[1]->GetData());

	for (int c=0; c<o->NumKids(); c++) {
		DrawableObject *oo = dynamic_cast<DrawableObject*>(o->Child(c));
		if (c > out->n()) {
			out->Push(oo,0);
		} else {
			if (out->e(c) != oo) out->Set(c, oo, 0);
		}
	}

	while (out->n() > o->NumKids()) out->Remove(out->n()-1);
	properties.e[1]->Touch();
	return NodeBase::Update();
}



//------------------------ SetParentNode ------------------------

/*! 
 * Node to set parent of a DrawableObject or array of them.
 */

class SetParentNode : public NodeBase
{
  public:
	SetParentNode();
	virtual ~SetParentNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SetParentNode(); }
};


SetParentNode::SetParentNode()
{
	makestr(Name, _("Set parent"));
	makestr(type, "Drawables/SetParent");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "parent", nullptr,1, _("Parent"), 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "kids", nullptr,1, _("Children"), 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

SetParentNode::~SetParentNode()
{
}

NodeBase *SetParentNode::Duplicate()
{
	SetParentNode *node = new SetParentNode();
	node->DuplicateBase(this);
	return node;
}

int SetParentNode::GetStatus()
{
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!o) return -1;
	o = dynamic_cast<DrawableObject*>(properties.e[1]->GetData());
	if (!o) {
		SetValue *s = dynamic_cast<SetValue*>(properties.e[1]->GetData());
		if (!s) return -1;
	}
	return NodeBase::GetStatus();
}

int SetParentNode::Update()
{
	Error(nullptr);

	DrawableObject *pnt = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!pnt) {
		Error(_("Parent must be a Drawable"));
		return -1;
	}

	DrawableObject *newpnt = nullptr;
	DrawableObject *k = dynamic_cast<DrawableObject*>(properties.e[1]->GetData());

	if (k) { //simple single child to parent
		newpnt = dynamic_cast<DrawableObject*>(pnt->duplicate());
		DrawableObject *kk = dynamic_cast<DrawableObject*>(k->duplicate());
		newpnt->push(kk);
		kk->dec_count();

	} else { // set of children to parent
		SetValue *set = dynamic_cast<SetValue*>(properties.e[1]->GetData());
		if (!set) {
			Error(_("Children must be a single Drawable or set of Drawables"));
			return -1;
		}
		for (int c=0; c<set->n(); c++) {
			if (!dynamic_cast<DrawableObject*>(set->e(c))) {
				Error(_("Children must be a single Drawable or set of Drawables"));
				return -1;
			}
		}

		newpnt = dynamic_cast<DrawableObject*>(pnt->duplicate());
		for (int c=0; c<set->n(); c++) {
			DrawableObject *kk = dynamic_cast<DrawableObject*>(set->e(c)->duplicate());
			newpnt->push(kk);
			kk->dec_count();
		}
	}

	newpnt->FindBBox();
	properties.e[2]->SetData(newpnt,1);
	return NodeBase::Update();
}

int SetParentNode::UpdatePreview()
{
	return 1;
}


//------------------------ SetPositionsNode ------------------------

/*! 
 * Node to Set positions of Drawables from a PointSet
 */

class SetPositionsNode : public NodeBase
{
  public:
	SetPositionsNode();
	virtual ~SetPositionsNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual Value *PreviewFrom() { return properties.e[2]->GetData(); }

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SetPositionsNode(); }
};


SetPositionsNode::SetPositionsNode()
{
	makestr(Name, _("Set positions"));
	makestr(type, "Drawable/SetPositions");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "objects", nullptr,1, _("Objects"), nullptr,0,false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "positions", nullptr,1, _("Positions"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "override", new BooleanValue(false),1, _("Modify original"), _("Dangerous!! Modifies original objects. May affect upstream connections."),0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

SetPositionsNode::~SetPositionsNode()
{
}

NodeBase *SetPositionsNode::Duplicate()
{
	SetPositionsNode *node = new SetPositionsNode();
	node->DuplicateBase(this);
	return node;
}

int SetPositionsNode::GetStatus()
{
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData()); 
	if (!o) {
		SetValue *ss = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!ss) return -1;
	}

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData()); 
	if (!fv) {
		PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[1]->GetData());
		if (!set) {
			SetValue *ss = dynamic_cast<SetValue*>(properties.e[1]->GetData());
			if (!ss) return -1;
		}
	}

	if (!isNumberType(properties.e[2]->GetData(), nullptr)) return -1;

	return NodeBase::GetStatus();
}

int SetPositionsNode::Update()
{
	Error(nullptr);

	// determine input object(s)
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData()); 
	SetValue *oset = nullptr;
	if (!o) {
		oset = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!oset) {
			Error(_("Objects must be Drawable or set of Drawables"));
			return -1;
		}
	}

	// determine position(s)
	FlatvectorValue *pos = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData()); 
	PointSetValue *pset = nullptr;
	SetValue *set = nullptr;
	if (!pos) {
		pset = dynamic_cast<PointSetValue*>(properties.e[1]->GetData());
		if (!pset) {
			set = dynamic_cast<SetValue*>(properties.e[1]->GetData());
			if (!set) {
				Error(_("Positions must be a vector2, PointSet, or Set of Vector2"));
				return -1;
			}
		} //else if (pset->NumPoints() == 0) return -1;
	}	

	int isnum = 0;
	bool override = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;

	// apply into output
	if (o) { //single object, easy!
		if (!override) o = dynamic_cast<DrawableObject*>(o->duplicate());
		else o->inc_count();
		properties.e[3]->SetData(o, 1);

		if (pos) o->origin(pos->v);
		else if (pset) {
			if (pset->NumPoints() > 0) {
				o->origin(pset->points.e[0]->p);
			}
		} else { //set
			if (set->n() > 0) {
				pos = dynamic_cast<FlatvectorValue*>(set->e(0));
				if (pos) o->origin(pos->v);
				else {
					Error(_("Set must only contain Vector2"));
					return -1;
				}
			}
		}

	} else { //oset of input drawables
		SetValue *out = dynamic_cast<SetValue *>(properties.e[3]->GetData());
		if (!out) {
			if (override) {
				out = oset;
				out->inc_count();
			} else out = new SetValue();
			properties.e[3]->SetData(out, 1);
		} else {
			if (override && out != oset) {
				properties.e[3]->SetData(out, 0);
			} else if (!override) out->values.flush();
			properties.e[3]->Touch();
		}

		int i = 0;
		flatvector p;
		for (int c=0; c<oset->n(); c++) {
			DrawableObject *oo = dynamic_cast<DrawableObject*>(oset->e(c));
			if (!oo) continue;

			AffineValue *aff = nullptr;
			if (pos) p = pos->v;
			else if (pset) {
				if (i < pset->NumPoints()) {
					p = pset->points.e[i]->p;
					aff = dynamic_cast<AffineValue*>(pset->points.e[i]->info);
					i++;
				}
			} else { //set
				if (i < set->n()) {
					pos = dynamic_cast<FlatvectorValue*>(set->e(i));
					if (pos) p = pos->v;
					i++; // *** this is poor.. should track to next flatvector or ensure set is all vectors above
				}
			}

			if (!override) {
				oo = dynamic_cast<DrawableObject*>(oo->duplicate());
				oo->FindBBox();
				out->Push(oo, 1);
			} //else oo is already in out, since out == oset
			oo->origin(p);
			if (aff) {
				oo->xaxis(aff->xaxis());
				oo->yaxis(aff->yaxis());
			}
			// DBG cerr << "SetPositions: duped obj: "<<endl;
			// DBG oo->dump_out(stderr, 2, 0, nullptr);
		}
	}

	return NodeBase::Update();
}

//------------------------ SetScalesNode ------------------------

/*! 
 * Node to Set scales of Affines from a number or vector, or set thereof.
 */

class SetScalesNode : public NodeBase
{
	bool GetScale(Value *v, flatvector &scalev);

  public:
	SetScalesNode();
	virtual ~SetScalesNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual Value *PreviewFrom() { return properties.e[2]->GetData(); }

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SetScalesNode(); }
};


SetScalesNode::SetScalesNode()
{
	makestr(Name, _("Set scales"));
	makestr(type, "Drawable/SetScales");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "objects", nullptr,1, _("Objects"), nullptr,0,false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scales", nullptr,1, _("Scales"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "override", new BooleanValue(false),1, _("Modify original"), _("Dangerous!! Modifies original objects. May affect upstream connections."),0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

SetScalesNode::~SetScalesNode()
{
}

NodeBase *SetScalesNode::Duplicate()
{
	SetScalesNode *node = new SetScalesNode();
	node->DuplicateBase(this);
	return node;
}

int SetScalesNode::GetStatus()
{
	Affine *o = dynamic_cast<Affine*>(properties.e[0]->GetData()); 
	if (!o) {
		SetValue *ss = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!ss) return -1;
	}

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData()); 
	if (!fv) {
		PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[1]->GetData());
		if (!set) {
			SetValue *ss = dynamic_cast<SetValue*>(properties.e[1]->GetData());
			if (!ss) return -1;
		}
	}

	if (!isNumberType(properties.e[2]->GetData(), nullptr)) return -1;

	return NodeBase::GetStatus();
}

bool SetScalesNode::GetScale(Value *v, flatvector &scalev)
{
	if (!v) return false;
	if (isNumberType(v, &(scalev.x))) { scalev.y = scalev.x; return true; }
	if (v->type() == VALUE_Flatvector) {
		scalev = dynamic_cast<FlatvectorValue*>(v)->v;
		return true;
	}
	if (v->type() == VALUE_Spacevector) {
		SpacevectorValue *vvv = dynamic_cast<SpacevectorValue*>(v);
		scalev.set(vvv->v.x, vvv->v.y);
		return true;
	}
	return false;
}

int SetScalesNode::Update()
{
	Error(nullptr);

	// determine input object(s)
	Value *ov = properties.e[0]->GetData();
	Affine *o = dynamic_cast<Affine*>(ov); 
	SetValue *iset = nullptr;
	if (!o) {
		iset = dynamic_cast<SetValue*>(ov);
		if (!iset) {
			Error(_("Objects must be Affine derived or set of Affine derived"));
			return -1;
		}
	}

	// determine scales(s)
	flatvector scalev(1,1);
	
	Value *v = properties.e[1]->GetData();
	if (!v) return -1;

	PointSetValue *pset = nullptr;
	SetValue *set = nullptr;
	if (!GetScale(v, scalev)) {
		pset = dynamic_cast<PointSetValue*>(v);
		if (!pset) set = dynamic_cast<SetValue*>(v);
		if (!set && !pset) {
			Error(_("Scales must be a number, vector2, PointSet, or Set of Vector2 or number"));
			return -1;
		}
	}
	
	int isnum = 0;
	bool override = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;

	// apply into output
	if (o) { //single object, easy!
		if (!override) {
			ov = ov->duplicate();
			o = dynamic_cast<Affine*>(ov);
		} else ov->inc_count();
		properties.e[3]->SetData(ov, 1);

		if (pset) { //pointset, grab first
			if (pset->NumPoints() > 0) {
				Affine *af = dynamic_cast<Affine*>(pset->points.e[0]->info);
				if (!af) {
					Error(_("Point set missing scale information"));
					return -1;
				}
				scalev.set(af->xaxis().norm(), af->yaxis().norm());
			}
		} else if (set) { //set, grab first
			if (set->n() > 0) {
				if (!GetScale(set->e(0), scalev)) {
					Error(_("Set must only contain numbers or Vector2"));
					return -1;
				}
			}
		}
		o->setScale(scalev.x, scalev.y);

	} else { //iset of inputs
		SetValue *out = dynamic_cast<SetValue *>(properties.e[3]->GetData());
		if (!out) {
			if (override) {
				out = iset;
				out->inc_count();
			} else out = new SetValue();
			properties.e[3]->SetData(out, 1);
		} else {
			if (override) {
				if (out != iset) {
					properties.e[3]->SetData(iset, 0);
					out = iset;
				}
			} else out->values.flush();
			properties.e[3]->Touch();
		}

		for (int c=0; c<iset->n(); c++) {
			//object to transform
			ov = iset->e(c);
			o = dynamic_cast<Affine*>(ov);
			if (!o) {
				Error(_("Bad input type"));
				return -1;
			}

			//get scale value
			if (pset) { //pointset
				if (c < pset->NumPoints()) {
					Affine *af = dynamic_cast<Affine*>(pset->points.e[c]->info);
					if (!af) {
						Error(_("Point set missing scale information"));
						return -1;
					}
					scalev.x = af->xaxis().norm();
					scalev.y = af->yaxis().norm();
				}
			} else if (set) {
				if (c < set->n()) {
					if (!GetScale(set->e(c), scalev)) {
						Error(_("Scales must be a number, vector2, PointSet, or Set of Vector2 or number"));
						return -1;
					}
				}
			} //else just go with current scalev

			if (!override) {
				ov = ov->duplicate();
				o = dynamic_cast<Affine*>(ov);
				if (dynamic_cast<DrawableObject*>(ov)) dynamic_cast<DrawableObject*>(ov)->FindBBox();
				out->Push(ov, 1);
			} //else oo is already in out, since out == oset
			o->setScale(scalev.x, scalev.y);
		}
	}

	return NodeBase::Update();
}


//------------------------ RotateDrawablesNode ------------------------

/*! 
 * Rotate a DrawableObject or list thereof.
 */

class RotateDrawablesNode : public NodeBase
{
  public:
	RotateDrawablesNode();
	virtual ~RotateDrawablesNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual Value *PreviewFrom() { return properties.e[2]->GetData(); }

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new RotateDrawablesNode(); }
};


RotateDrawablesNode::RotateDrawablesNode()
{
	makestr(Name, _("Rotate Objs"));
	makestr(type, "Drawable/RotateDrawables");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "objects", nullptr,1, _("Objects"), nullptr,0,false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "angle", new DoubleValue(0),1, _("Angle"), nullptr,0,true));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "override", new BooleanValue(false),1, _("Override in"), _("Modifies in. Dangerous!!"),0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

RotateDrawablesNode::~RotateDrawablesNode()
{
}

NodeBase *RotateDrawablesNode::Duplicate()
{
	RotateDrawablesNode *node = new RotateDrawablesNode();
	node->DuplicateBase(this);
	return node;
}

int RotateDrawablesNode::GetStatus()
{
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData()); 
	if (!o) {
		SetValue *ss = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!ss) return -1;
	}

	if (!isNumberType(properties.e[1]->GetData(), nullptr)) return -1;
	return NodeBase::GetStatus();
}

int RotateDrawablesNode::Update()
{
	Error(nullptr);
	bool override = false;

	// determine input object(s)
	DrawableObject *o = dynamic_cast<DrawableObject*>(properties.e[0]->GetData()); 
	SetValue *oset = nullptr;
	if (!o) {
		oset = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!oset) return -1;
	}

	// determine position(s)
	int isnum;
	double angle = getNumberValue(properties.e[1]->GetData(), &isnum);
	SetValue *set = nullptr;
	if (!isnum) {
		set = dynamic_cast<SetValue*>(properties.e[1]->GetData());
		if (!set) {
			Error(_("Angle must be a number or a set of numbers"));
			return -1;
		}
	}
	

	// apply into output
	if (o) { //single object, easy!
		if (!override) o = dynamic_cast<DrawableObject*>(o->duplicate());
		else o->inc_count();
		properties.e[2]->SetData(o, 1);

		if (set) {
			if (set->n() > 0) {
				angle = getNumberValue(set->e(0), &isnum);
				if (!isnum) {
					o->dec_count();
					Error(_("Set must contain only numbers"));
					return -1;
				}
			}
		}
		o->Rotate(angle);

	} else { //oset of input drawables
		SetValue *out = dynamic_cast<SetValue *>(properties.e[2]->GetData());
		if (!out) {
			if (override) {
				out = oset;
				out->inc_count();
			} else out = new SetValue();
			properties.e[2]->SetData(out, 1);
		} else {
			if (override && out != oset) {
				properties.e[2]->SetData(out, 0);
			} else if (!override) out->values.flush();
			properties.e[2]->Touch();
		}

		int i = 0;
		for (int c=0; c<oset->n(); c++) {
			DrawableObject *oo = dynamic_cast<DrawableObject*>(oset->e(c));
			if (!oo) continue;

			if (set) {
				if (i < set->n()) {
					angle = getNumberValue(set->e(c), &isnum);
					if (isnum) i++;
				} else angle = 0;
			}

			if (!override) {
				oo = dynamic_cast<DrawableObject*>(oo->duplicate());
				oo->FindBBox();
				out->Push(oo, 1);
			} //else oo is already in out, since out == oset
			oo->Rotate(angle);
		}
	}

	return NodeBase::Update();
}


//------------------------ LCaptionDataNode ------------------------

/*! 
 * Node for LImageData.
 */

class LCaptionDataNode : public NodeBase
{
  public:
	LCaptionDataNode();
	virtual ~LCaptionDataNode();

	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual Value *PreviewFrom() { return properties.e[2]->GetData(); }

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new LCaptionDataNode(); }
};


LCaptionDataNode::LCaptionDataNode()
{
	makestr(Name, _("Caption"));
	makestr(type, "Drawable/CaptionData");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "text", new StringValue(),1, _("Text")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "escaped", new BooleanValue(false),1, _("Escaped text"), _("Unescape '\\n', '\\t', and unicode chars: '\\U003F'")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "transform",  nullptr,1, _("Transform")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "font", nullptr,1, _("Font")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "color", new ColorValue(0.,0.,0.,1.),1, _("Color")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", nullptr,1, _("out"), nullptr, 0, false)); 
}

LCaptionDataNode::~LCaptionDataNode()
{
}

NodeBase *LCaptionDataNode::Duplicate()
{
	LCaptionDataNode *node = new LCaptionDataNode();
	node->DuplicateBase(this);
	return node;
}

int LCaptionDataNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v || (v->type() != VALUE_String && v->type() != VALUE_Set)) return -1;

	v = properties.e[1]->GetData();
	if (!isNumberType(v, nullptr)) return -1;

	v = properties.e[2]->GetData();
	if (v && !dynamic_cast<Affine*>(v) && v->type() != VALUE_Set) return -1;

	v = properties.e[3]->GetData();
	if (v && v->type() != VALUE_Font && v->type() != VALUE_Set) return -1;

	v = properties.e[4]->GetData();
	if (v && v->type() != VALUE_Color && v->type() != VALUE_Set) return -1;
	
	return NodeBase::GetStatus();
}

int LCaptionDataNode::Update()
{
	Error(nullptr);

	StringValue *strin = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	SetValue *strsetin = nullptr;
	if (!strin) {
		strsetin = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!strsetin) {
			Error(_("Text must be a string or set of strings"));
			return -1;
		}
	}

	int isnum = 0;
	bool escaped_text = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;

	// *** should be able to parse from PointSet, or SetValue of positions
	Affine af;
	SetValue *afset = nullptr;
	Value *v = properties.e[2]->GetData();
	Affine *a = dynamic_cast<Affine*>(v);
	if (a) {
		af.m(a->m());
	} else if (v) { //assume identity if null
		afset = dynamic_cast<SetValue*>(v);
		if (!afset) {
			Error(_("Transforms must be Affine or set of Affine"));
			return -1;
		}
	}

	v = properties.e[3]->GetData();
	FontValue *fv = dynamic_cast<FontValue*>(v);
	SetValue *fontset = nullptr;
	if (!fv && v) {
		fontset = dynamic_cast<SetValue*>(v);
		if (!fontset) {
			Error(_("Font must be a font or set of fonts")); //note it's ok if null
			return -1;
		}
	}

	v = properties.e[4]->GetData();
	ColorValue *color = dynamic_cast<ColorValue*>(v);
	SetValue *colorset = nullptr;
	if (!color && v) {
		colorset = dynamic_cast<SetValue*>(v);
		if (!colorset) {
			Error(_("Color must be a color or set of colors"));
			return -1;
		}
	}

	int max = 1;
	if (strsetin && strsetin->n() > max) max = strsetin->n();
	if (afset && afset->n() > max)       max = afset->n();
	if (fontset && fontset->n() > max)   max = fontset->n();
	if (colorset && colorset->n() > max) max = colorset->n();

	SetValue *outset = nullptr;
	LCaptionData *out = nullptr;
	if (strsetin || afset || fontset || colorset) {
		//out needs to be a set
		outset = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
		if (!outset) {
			outset = new SetValue();
			properties.e[properties.n-1]->SetData(outset, 1);
		} else properties.e[properties.n-1]->Touch();
	} else {
		//out is just a single object
		out = dynamic_cast<LCaptionData*>(properties.e[properties.n-1]->GetData());
		if (!out) {
			out = new LCaptionData();
			out->yaxis(-out->yaxis());
			properties.e[properties.n-1]->SetData(out, 1);
		} else {
			out->m(1,0,0,-1,0,0);
			properties.e[properties.n-1]->Touch();
		}
	}

	for (int c=0; c<max; c++) {
		if (afset && c < afset->n()) {
			a = dynamic_cast<Affine*>(afset->e(c));
			if (!a) {
				Error(_("Transforms must be Affine or set of Affine"));
				return -1;
			}
			af.m(a->m());
		}

		if (fontset && c < fontset->n()) {
			fv = dynamic_cast<FontValue*>(fontset->e(c));
			if (!fv) {
				Error(_("Font must be a font or set of fonts")); //note it's ok if null
				return -1;
			}
		}

		if (colorset && c < colorset->n()) {
			color = dynamic_cast<ColorValue*>(colorset->e(c));
			if (!color) {
				Error(_("Color must be a color or set of colors"));
				return -1;
			}
		}

		if (strsetin && c < strsetin->n()) {
			strin = dynamic_cast<StringValue*>(strsetin->e(c));
			if (!strin) {
				Error(_("Text must be a string or set of strings"));
				return -1;
			}	
		}
		if (!strin) {
			Error(_("Text must be a string or set of strings"));
			return -1;
		}

		if (outset) {
			out = nullptr;
			if (c < outset->n()) out = dynamic_cast<LCaptionData*>(outset->e(c));
			if (!out) {
				out = new LCaptionData();
				out->yaxis(-out->yaxis());
				if (c < outset->n()) outset->Set(c, out, 1);
				else outset->Push(out, 1);
			} else out->m(1,0,0,-1,0,0);
		}

		out->Multiply(af);
		out->xcentering = out->ycentering = 50;
		if (fv && fv->font) out->Font(fv->font);
		if (escaped_text) out->SetTextEscaped(strin->str);
		else out->SetText(strin->str);
		if (color) out->ColorRGB(color->color.Red(), color->color.Green(), color->color.Blue(), color->color.Alpha());
	}
	if (outset) {
		while (outset->n() > max) outset->Remove(max);
	}

	return NodeBase::Update();
}


//------------------------ LImageDataNode ------------------------

/*! 
 * Node for LImageData.
 */

class LImageDataNode : public NodeBase
{
  public:
	LImageDataNode();
	virtual ~LImageDataNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual Value *PreviewFrom() { return properties.e[2]->GetData(); }

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new LImageDataNode(); }
};


LImageDataNode::LImageDataNode()
{
	makestr(Name, _("Image Data"));
	makestr(type, "Drawable/ImageData");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "transform",  new AffineValue(),1, _("Transform")));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "image", new ColorValue(1.,1.,1.,1.),1, _("Image")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "image", nullptr,1, _("Image")));

	LImageData *imagedata = new LImageData();
	AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", imagedata,1, nullptr, nullptr, 0, false)); 
}

LImageDataNode::~LImageDataNode()
{
}

NodeBase *LImageDataNode::Duplicate()
{
	LImageDataNode *node = new LImageDataNode();
	node->DuplicateBase(this);
	return node;
}

int LImageDataNode::Update()
{
	Affine *a = dynamic_cast<Affine*>(properties.e[0]->GetData());
	if (!a) return -1;
	//ColorValue *col = dynamic_cast<ColorValue*>(properties.e[1]->GetData());
	ImageValue *image = dynamic_cast<ImageValue*>(properties.e[1]->GetData());
	//if (!col && !image) return -1;
	if (!image) return -1;

	LImageData *imagedata = dynamic_cast<LImageData*>(properties.e[2]->GetData());
	imagedata->m(a->m());
	//if (col) imagedata->SetImageAsColor(col->color.Red(), col->color.Green(), col->color.Blue(), col->color.Alpha());
	//else
	imagedata->SetImage(image->image, nullptr);

	return NodeBase::Update();
}

int LImageDataNode::GetStatus()
{
	if (!dynamic_cast<Affine*>(properties.e[0]->GetData())) return -1;
	if (!dynamic_cast<ColorValue*>(properties.e[1]->GetData())
		&& !dynamic_cast<ImageValue*>(properties.e[1]->GetData())) return -1;
	if (!properties.e[2]->data) return 1;

	return NodeBase::GetStatus();
}



//------------------------ LImageDataInfoNode ------------------------

/*! \class LImageDataInfoNode
 * Node for LImageDataInfo.
 */

class LImageDataInfoNode : public NodeBase
{
  public:
	LImageDataInfoNode();
	virtual ~LImageDataInfoNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();
};


LImageDataInfoNode::LImageDataInfoNode()
{
	makestr(Name, _("Image Data Info"));
	makestr(type, "Drawable/ImageDataInfo");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,true, "in", nullptr,1, NULL, 0, false)); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "transform",  new AffineValue(),1, _("Transform"),nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "image", new ImageValue(),1, _("Image"),nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "file", new StringValue(""),1, _("File"), nullptr,0,false)); 

}

LImageDataInfoNode::~LImageDataInfoNode()
{
}

NodeBase *LImageDataInfoNode::Duplicate()
{
	LImageDataInfoNode *node = new LImageDataInfoNode();
	node->DuplicateBase(this);
	return node;
}

int LImageDataInfoNode::Update()
{
	LImageData *imgdata = dynamic_cast<LImageData*>(properties.e[0]->GetData());
	if (!imgdata) return -1;

	AffineValue *a = dynamic_cast<AffineValue*>(properties.e[1]->GetData());
	a->set(imgdata->m());

	ImageValue *image = dynamic_cast<ImageValue*>(properties.e[2]->GetData());
	if (image->image != imgdata->image) {
		if (image->image) image->image->dec_count();
		image->image = imgdata->image;
		if (image->image) image->image->inc_count();
	}

	dynamic_cast<StringValue*>(properties.e[3]->GetData())->Set(imgdata->filename ? imgdata->filename : "");

	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}

int LImageDataInfoNode::GetStatus()
{
	LImageData *imgdata = dynamic_cast<LImageData*>(properties.e[0]->GetData());
	if (!imgdata) return -1;
	return NodeBase::GetStatus();
}

int LImageDataInfoNode::UpdatePreview()
{
	LImageData *imagedata = dynamic_cast<LImageData*>(properties.e[0]->GetData());
	LaxImage *img = imagedata ? imagedata->GetPreview() : nullptr;
	if (img == nullptr) img = imagedata->image;
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	} else {
		if (total_preview) total_preview->dec_count();
		total_preview = nullptr;
	}
	return 1;
}


Laxkit::anObject *newLImageDataInfoNode(int p, Laxkit::anObject *ref)
{
	return new LImageDataInfoNode();
}


//------------------------ PathsDataNode ------------------------

/*! \class PathsDataNode
 * Node for constructing PathsData objects..
 *
 * todo:
 *   points
 *   weight nodes
 *   fillstyle
 *   linestyle
 */

class PathsDataNode : public NodeBase
{
  public:
	LPathsData *pathsdata;

	PathsDataNode(LPathsData *path, int absorb);
	virtual ~PathsDataNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();
};


PathsDataNode::PathsDataNode(LPathsData *path, int absorb)
{
	makestr(Name, "PathsData");
	makestr(type, "Paths/PathsData");
	pathsdata = path;
	if (pathsdata && !absorb) pathsdata->inc_count();
	if (!pathsdata) pathsdata = new LPathsData();

	// just output a new, empty LPathsData object.
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Path", pathsdata,1, _("Path"), NULL,0, false));
}

PathsDataNode::~PathsDataNode()
{
	//if (pathsdata) pathsdata->dec_count();
}

NodeBase *PathsDataNode::Duplicate()
{
	PathsDataNode *node = new PathsDataNode(pathsdata, false);
	node->DuplicateBase(this);
	return node;
}

int PathsDataNode::Update()
{
	return NodeBase::Update();
}

int PathsDataNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int PathsDataNode::UpdatePreview()
{
	LaxImage *img = pathsdata->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	}
	return 1;
}

Laxkit::anObject *newPathsDataNode(int p, Laxkit::anObject *ref)
{
	return new PathsDataNode(nullptr, false);
}


//------------------------ SetOriginBBoxNode ------------------------

/*! 
 * Node for setting a PathsData origin to a bbox point.
 */

class SetOriginBBoxNode : public NodeBase
{
  public:
	SetOriginBBoxNode();
	virtual ~SetOriginBBoxNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SetOriginBBoxNode(); }
};


SetOriginBBoxNode::SetOriginBBoxNode()
{
	makestr(Name, "Set Origin at bbox");
	makestr(type, "Paths/SetOriginToBBox");
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "x", new DoubleValue(.5),1, _("BBox X"), _("0..1, 0 is minimum, 1 is maximum"),0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "y", new DoubleValue(.5),1, _("BBox Y"), _("0..1, 0 is minimum, 1 is maximum"),0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

SetOriginBBoxNode::~SetOriginBBoxNode()
{
	//if (pathsdata) pathsdata->dec_count();
}

NodeBase *SetOriginBBoxNode::Duplicate()
{
	SetOriginBBoxNode *node = new SetOriginBBoxNode();
	node->DuplicateBase(this);
	return node;
}

int SetOriginBBoxNode::GetStatus()
{
	LPathsData *o = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!o) {
		Error(_("Only works on PathsData"));
		return -1;
	}

	if (!isNumberType(properties.e[1]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), nullptr)) return -1;
	
	return NodeBase::GetStatus();
}

int SetOriginBBoxNode::Update()
{
	Error(nullptr);

	LPathsData *o = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!o) return -1;

	int isnum;
	double x = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	double y = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	
	// *** safe duplicating when an object has a filter, but it also doesn't copy the filter.. need to sort this out
	DrawableObject *d = dynamic_cast<DrawableObject*>(o);
	anObject *filter = d->filter;
	d->filter = NULL;
	DrawableObject *copy = dynamic_cast<DrawableObject*>(d->duplicate());
	copy->FindBBox();
	d->filter = filter;
		
	LPathsData *out = dynamic_cast<LPathsData*>(copy);
	out->SetOriginToBBoxPoint(flatpoint(x,y));
	properties.e[3]->SetData(out,1);
	return NodeBase::Update();
}


//------------------------ EndPointsNode ------------------------

/*! 
 * Node for extracting end points of paths.
 */

class EndPointsNode : public NodeBase
{
  public:
	EndPointsNode();
	virtual ~EndPointsNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new EndPointsNode(); }
};


EndPointsNode::EndPointsNode()
{
	makestr(Name, "End points");
	makestr(type, "Paths/EndPoints");
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "ends", new BooleanValue(true),1, _("Ends"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "starts", new BooleanValue(true),1, _("Starts"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "transforms", new BooleanValue(false),1, _("As transforms"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "n", new IntValue(0),1, _("Num points"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

EndPointsNode::~EndPointsNode()
{
	//if (pathsdata) pathsdata->dec_count();
}

NodeBase *EndPointsNode::Duplicate()
{
	EndPointsNode *node = new EndPointsNode();
	node->DuplicateBase(this);
	return node;
}

int EndPointsNode::GetStatus()
{
	LPathsData *o = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!o) return -1;

	if (!isNumberType(properties.e[1]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[3]->GetData(), nullptr)) return -1;
	
	return NodeBase::GetStatus();
}

int EndPointsNode::Update()
{
	LPathsData *o = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!o) return -1;

	int isnum;
	bool ends = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	bool starts = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	bool astransform = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;

	PointSetValue *out = dynamic_cast<PointSetValue*>(properties.e[5]->GetData());
	if (!out) {
		out = new PointSetValue();
		properties.e[5]->SetData(out, 1);
	} else {
		out->Flush();
	}

	for (int c=0; c<o->paths.n; c++) {
		LaxInterfaces::Coordinate *p = o->paths.e[c]->path;
		if (!p) continue;
		if (starts) {
			LaxInterfaces::Coordinate *pp = p->previousVertex(0);
			if (!pp) {
				AffineValue *a = nullptr;
				if (astransform) {
					a = new AffineValue();
					a->origin(p->p());
					flatpoint dir = p->direction(1);
					a->xaxis(dir);
					a->yaxis(transpose(dir));
				}
				out->AddPoint(p->p(), a, 1);
			}
		}
		if (ends) {
			LaxInterfaces::Coordinate *pp = p->lastPoint(1);
			if (pp && pp != p) {
				AffineValue *a = nullptr;
				if (astransform) {
					a = new AffineValue();
					a->origin(pp->p());
					flatpoint dir = -pp->direction(0);
					a->xaxis(dir);
					a->yaxis(transpose(dir));
				}
				out->AddPoint(pp->p(), a,1);
			}
		}
	}
	
	IntValue *iv = dynamic_cast<IntValue*>(properties.e[4]->GetData());
	iv->i = out->NumPoints();
	properties.e[4]->Touch();
	properties.e[5]->Touch();
	return NodeBase::Update();
}

int EndPointsNode::UpdatePreview()
{
	return 1;
}


//------------------------ ExtremaNode ------------------------

/*! 
 * Node for extracting end points of paths.
 */

class ExtremaNode : public NodeBase
{
  public:
	ExtremaNode();
	virtual ~ExtremaNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ExtremaNode(); }
};


ExtremaNode::ExtremaNode()
{
	makestr(Name, "Extrema");
	makestr(type, "Paths/Extrema");
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "leftmost",   new BooleanValue(true),1, _("Leftmost"),   nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "rightmost",  new BooleanValue(true),1, _("Rightmost"),  nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "topmost",    new BooleanValue(true),1, _("Topmost"),    nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "bottommost", new BooleanValue(true),1, _("Bottommost"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "n", new IntValue(0),1, _("Num points"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "paths", nullptr,1, _("Path indices"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "t", nullptr,1, _("t"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "points", nullptr,1, _("Points"), nullptr,0,false));
}

ExtremaNode::~ExtremaNode()
{
	//if (pathsdata) pathsdata->dec_count();
}

NodeBase *ExtremaNode::Duplicate()
{
	ExtremaNode *node = new ExtremaNode();
	node->DuplicateBase(this);
	return node;
}

int ExtremaNode::GetStatus()
{
	LPathsData *o = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!o) return -1;

	if (!isNumberType(properties.e[1]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[3]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[4]->GetData(), nullptr)) return -1;
	
	return NodeBase::GetStatus();
}

int ExtremaNode::Update()
{
	LPathsData *pdata = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!pdata) return -1;

	int isnum;
	bool lefts = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	bool rights = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	bool tops = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;
	bool bottoms = getNumberValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) return -1;

	SetValue *pindices = dynamic_cast<SetValue*>(properties.e[6]->GetData());
	if (!pindices) {
		pindices = new SetValue();
		properties.e[6]->SetData(pindices, 1);
	} else {
		pindices->Flush();
	}
	SetValue *t = dynamic_cast<SetValue*>(properties.e[7]->GetData());
	if (!t) {
		t = new SetValue();
		properties.e[7]->SetData(t, 1);
	} else {
		t->Flush();
	}
	PointSetValue *points = dynamic_cast<PointSetValue*>(properties.e[8]->GetData());
	if (!points) {
		points = new PointSetValue();
		properties.e[8]->SetData(points, 1);
	} else {
		points->Flush();
	}

	NumStack<double> t_vals;
	NumStack<flatpoint> p_rets;

	for (int c=0; c<pdata->paths.n; c++) {
		t_vals.flush();
		p_rets.flush();
		int n = pdata->FindExtrema(c, &p_rets, &t_vals);
		for (int c2=0; c2<n; c2++) {
			if (    (lefts   && p_rets[c2].info == LAX_RIGHT)
				 || (rights  && p_rets[c2].info == LAX_LEFT)
				 || (tops    && p_rets[c2].info == LAX_BOTTOM)
				 || (bottoms && p_rets[c2].info == LAX_TOP)
			   ) {
			   	AffineValue *af = new AffineValue;
			    af->origin(p_rets[c2]);
			    flatvector v;
			    pdata->PointAlongPath(c, t_vals[c2], 0, nullptr, &v);
			    v.normalize();
			    af->xaxis(v);
			    af->yaxis(transpose(v));
				points->AddPoint(p_rets[c2], af, true);
				t->Push(new DoubleValue(t_vals[c2]), 1);
				pindices->Push(new IntValue(c), 1);
			}
		}
	}
	
	IntValue *iv = dynamic_cast<IntValue*>(properties.e[5]->GetData());
	iv->i = points->NumPoints();
	properties.e[5]->Touch();
	properties.e[6]->Touch();
	properties.e[7]->Touch();
	properties.e[8]->Touch();
	return NodeBase::Update();
}


//------------------------ CornersNode ------------------------

/*! 
 * Node for extracting corner points of paths.
 */

class CornersNode : public NodeBase
{
  public:
	CornersNode();
	virtual ~CornersNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new CornersNode(); }
};

CornersNode::CornersNode()
{
	makestr(Name, "Corners");
	makestr(type, "Paths/Corners");
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "threshhold", new DoubleValue(1e-5),1, _("Threshhold"), _("Angle in degrees"),0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "n", new IntValue(0),1, _("Num points"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "paths", nullptr,1, _("Path indices"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "t", nullptr,1, _("t"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "points", nullptr,1, _("Points"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "inv",  nullptr,1, _("In v"),  nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "outv", nullptr,1, _("Out v"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "angles", nullptr,1, _("Angles"), nullptr,0,false));
}

CornersNode::~CornersNode()
{
}

NodeBase *CornersNode::Duplicate()
{
	CornersNode *node = new CornersNode();
	node->DuplicateBase(this);
	return node;
}

int CornersNode::GetStatus()
{
	LPathsData *o = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!o) return -1;

	if (!isNumberType(properties.e[1]->GetData(), nullptr)) return -1;
	
	return NodeBase::GetStatus();
}

int CornersNode::Update()
{
	LPathsData *pdata = dynamic_cast<LPathsData*>(properties.e[0]->GetData());
	if (!pdata) return -1;

	int isnum;
	double threshhold = getNumberValue(properties.e[1]->GetData(), &isnum) * M_PI/180;
	if (!isnum) return -1;

	SetValue *pindices = dynamic_cast<SetValue*>(properties.e[3]->GetData());
	if (!pindices) {
		pindices = new SetValue();
		properties.e[3]->SetData(pindices, 1);
	} else {
		pindices->Flush();
	}

	SetValue *t = dynamic_cast<SetValue*>(properties.e[4]->GetData());
	if (!t) {
		t = new SetValue();
		properties.e[4]->SetData(t, 1);
	} else {
		t->Flush();
	}

	PointSetValue *points = dynamic_cast<PointSetValue*>(properties.e[5]->GetData());
	if (!points) {
		points = new PointSetValue();
		properties.e[5]->SetData(points, 1);
	} else {
		points->Flush();
	}

	PointSetValue *ins = dynamic_cast<PointSetValue*>(properties.e[6]->GetData());
	if (!ins) {
		ins = new PointSetValue();
		properties.e[6]->SetData(ins, 1);
	} else {
		ins->Flush();
	}

	PointSetValue *outs = dynamic_cast<PointSetValue*>(properties.e[7]->GetData());
	if (!outs) {
		outs = new PointSetValue();
		properties.e[7]->SetData(outs, 1);
	} else {
		outs->Flush();
	}

	SetValue *angles = dynamic_cast<SetValue*>(properties.e[8]->GetData());
	if (!angles) {
		angles = new SetValue();
		properties.e[8]->SetData(angles, 1);
	} else {
		angles->Flush();
	}

	LaxInterfaces::Coordinate *p, *start, *pnext;
	for (int c=0; c<pdata->paths.n; c++) {
		p = pdata->paths.e[c]->path;
		if (!p) continue;

		start = p;
		int i = 0;

		do {
			pnext = p->nextVertex(0);
			if (!pnext || (p == start && p->previousVertex(0))) {
				p = p->nextVertex(0);
				i++;
				continue;
			}
			flatvector vbefore = p->direction(0);
			flatvector vafter = p->direction(1);
			if (-vbefore * vafter < cos(threshhold)) {	
				points->AddPoint(p->p(), nullptr, true);
				t->Push(new DoubleValue(i), 1);
				pindices->Push(new IntValue(c), 1);
				ins     ->AddPoint(vbefore);
				outs    ->AddPoint(vafter);

				double a1 = vbefore.angle();
				if (a1 < 0) a1 += 2*M_PI;
				double a2 = vafter.angle();
				if (a2 < 0) a2 += 2*M_PI;
				a2 -= a1;
				if (a2 < 0) a2 += 2*M_PI;
				angles  ->Push(new DoubleValue(a2), 1);
			}
			p = p->nextVertex(0);
			i++;
		} while (p && p != start);
	}
	
	IntValue *iv = dynamic_cast<IntValue*>(properties.e[2]->GetData());
	iv->i = points->NumPoints();
	properties.e[2]->Touch();
	properties.e[3]->Touch();
	properties.e[4]->Touch();
	properties.e[5]->Touch();
	properties.e[6]->Touch();
	properties.e[7]->Touch();
	properties.e[8]->Touch();
	return NodeBase::Update();
}


//----------------------- ExtrudeNode ------------------------

/*! \class ExtrudeNode
 *
 * Extrude a path. Only works on open paths. Add control points to the end points according to the extrude direction (or path).
 */
class ExtrudeNode : public NodeBase
{
  public:
	ExtrudeNode();
	virtual ~ExtrudeNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ExtrudeNode(); }
};

ExtrudeNode::ExtrudeNode()
{
	makestr(type, "Paths/Extrude");
	makestr(Name, _("Extrude"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",  nullptr,1,    _("Input"), _("A path")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "dir", new FlatvectorValue(1,0),1,  _("Vector"),  _("Vector or path")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", nullptr,1, _("Out"), nullptr,0, false));
}

ExtrudeNode::~ExtrudeNode()
{
}

NodeBase *ExtrudeNode::Duplicate()
{
	ExtrudeNode *node = new ExtrudeNode();
	node->DuplicateBase(this);
	return node;
}

/*! 0 for ok, -1 for bad ins. */
int ExtrudeNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v || !v->istype("PathsData")) return -1;

	v = properties.e[1]->GetData();
	if (v->type() != VALUE_Flatvector && !v->istype("PathsData")) return -1;

	return NodeBase::GetStatus();
}

int ExtrudeNode::Update()
{
	PathsData *in = dynamic_cast<PathsData*>(properties.e[0]->GetData());
	if (!in) return -1;

	FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
	PathsData *pdir = (fv ? nullptr : dynamic_cast<PathsData*>(properties.e[1]->GetData()));
	if (!fv && !pdir) return -1;
	if (pdir && (pdir->paths.n == 0 || pdir->paths.e[0]->path == nullptr)) return -1;

	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[2]->GetData());
	if (!out) {
		out = new LPathsData();
		properties.e[2]->SetData(out, 1);
	} else properties.e[2]->Touch();

	out->clear();
	out->m(in->m());
	if (in->linestyle) out->InstallLineStyle(in->linestyle);
	
	Affine af;
	flatvector v_diff;
	Path *dir = nullptr;
	Coordinate *p1, *p2;
	if (fv) {
		v_diff = fv->v;
	} else {
		if (pdir->paths.e[0]->path->NumPoints(1) < 2) {
			Error(_("Must have more than 1 point extrusion"));
			return -1;
		}
		dir = pdir->paths.e[0]->duplicate();
		dir->openAt(nullptr,0);
		
		flatpoint dir_start = dir->path->p();
		p1 = dir->path->lastPoint(1);
		flatpoint dir_end = p1->p();
		v_diff = dir_end - dir_start;

		if (dir->path->prev) { //remove initial controls if any
			p2 = dir->path->firstPoint(0);
			dir->path->disconnect(false);
			delete p2;
		}
		if (p1->next) { //remove trailing controls, if any
			p2 = p1->next;
			p1->disconnect(true);
			delete p2;
		}
	}

	for (int c=0; c<in->paths.n; c++) {
		Path *path1 = in->paths.e[c];
		if (!path1 || !path1->path) continue;

		path1 = path1->duplicate();
		path1->openAt(nullptr,0);

		//remove initial/trailing controls if any
		if (path1->path->prev) {
			p1 = path1->path->firstPoint(0);
			path1->path->disconnect(false);
			delete p1;
		}
		p1 = path1->path->lastPoint(1);
		if (p1->next) { //remove trailing controls, if any
			p2 = p1->next;
			p1->disconnect(true);
			delete p2;
		}

		// offset new edge
		Path *path2 = path1->duplicate();
		path2->Reverse();
		af.origin(v_diff);
		path2->Transform(af.m());

		if (fv) {
			 //easy just offset and connect
			p1 = path1->path->lastPoint(1);
			p2 = path2->path->firstPoint(1);
			flatvector vv = (p2->p() - p1->p())/3;

			p1->next = new Coordinate(p1->p() + vv, POINT_TOPREV, nullptr);
			p1->next->prev = p1;
			p1 = p1->next;
			
			p2->prev = new Coordinate(p2->p() - vv, POINT_TONEXT, nullptr);
			p2->prev->next = p2;
			p2 = p2->prev;
			
			p1->next = p2;
			p2->prev = p1;
			path2->path = nullptr;
			delete path2;

			p1 = path1->path->lastPoint(1);
			p2 = path1->path->firstPoint(1);
			vv = (p2->p() - p1->p())/3;

			p1->next = new Coordinate(p1->p() + vv, POINT_TOPREV, nullptr);
			p1->next->prev = p1;
			p1 = p1->next;
		
			p2->prev = new Coordinate(p2->p() - vv, POINT_TONEXT, nullptr);
			p2->prev->next = p2;
			p2 = p2->prev;
			
			p1->next = p2;
			p2->prev = p1;

			out->paths.push(path1);

		} else {
			//we need to install dups of a path between end points
			Path *dir1 = dir->duplicate();
			Path *dir2 = dir->duplicate();

			// so now we have vertex....vertex to add as new edges, minus the initial/final vertices
			
			// transform dir1 paths to proper places
			p1 = path1->path;
			p2 = path1->path->lastPoint(1);
			flatvector v = p2->p() - dir1->path->p();
			af.origin(v);
			dir1->Transform(af.m());
			v = p1->p() - dir2->path->p();
			af.origin(v);
			dir2->Transform(af.m());
			dir2->Reverse();

			// connect path1 to dir1
			p1 = path1->path->lastPoint(1);
			p1->next = dir1->path->next;
			p1->next->prev = p1;
			dir1->path->next = nullptr;
			delete dir1;

			// connect to path2
			p1 = p1->lastPoint(1);
			p1->next = path2->path->next;
			path2->path->next->prev = p1;
			path2->path->next = nullptr;
			delete path2;

			// connect to dir2
			p1 = p1->lastPoint(1);
			p1->next = dir2->path->next;
			p1->next->prev = p1;
			dir2->path->next = nullptr;
			delete dir2;

			// connect to path1
			p1 = p1->lastPoint(1);
			p2 = p1;
			p1 = p1->prev;
			p1->disconnect(true);
			delete p2;
			p1->next = path1->path;
			path1->path->prev = p1;
			
			out->paths.push(path1);
		}
	}

	//clean up
	DBG out->dump_out(stdout, 2, 0, nullptr);
	out->FindBBox();
	delete dir;
	return NodeBase::Update();
}


//------------------------ PointsToDelaunayNode ------------------------

/*! 
 * Node to convert a PointSet to a VoronoiData.
 */

class PointsToDelaunayNode : public NodeBase
{
  public:
	PointsToDelaunayNode();
	virtual ~PointsToDelaunayNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PointsToDelaunayNode(); }
};


PointsToDelaunayNode::PointsToDelaunayNode()
{
	makestr(Name, _("Points to Delaunay"));
	makestr(type, "Points/PointsToDelaunay");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "points", new BooleanValue(true),1, _("Points"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "tris", new BooleanValue(true),1, _("Triangles"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "voronoi", new BooleanValue(true),1, _("Voronoi"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

PointsToDelaunayNode::~PointsToDelaunayNode()
{
}

NodeBase *PointsToDelaunayNode::Duplicate()
{
	PointsToDelaunayNode *node = new PointsToDelaunayNode();
	node->DuplicateBase(this);
	return node;
}

int PointsToDelaunayNode::GetStatus()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;
	int isnum;
	for (int c=1; c<4; c++) {
		getNumberValue(properties.e[c]->GetData(), &isnum);
		if (!isnum) return -1;
	}

	return NodeBase::GetStatus();
}

int PointsToDelaunayNode::Update()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;

	int isnum;
	bool points = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	bool tris = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	bool voronoi = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;

	LVoronoiData *out = dynamic_cast<LVoronoiData*>(properties.e[4]->GetData());
	if (!out) {
		out = new LVoronoiData();
		properties.e[4]->SetData(out, 1);
	}

	out->Flush();
	out->show_points = points;
	out->show_delaunay = tris;
	out->show_voronoi = voronoi;
	out->CopyFrom(set, false);
	out->RebuildVoronoi(true);
	out->FindBBox();
	out->touchContents();

	properties.e[4]->Touch();
	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}

int PointsToDelaunayNode::UpdatePreview()
{
	DrawableObject *out = dynamic_cast<DrawableObject*>(properties.e[properties.n-1]->GetData());
	if (!out) return 1;

	LaxImage *img = out->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	} else {
		if (total_preview) total_preview->dec_count();
		total_preview = nullptr;
	}
	return 1;
}


//------------------------ PointsToDotsNode ------------------------

/*! 
 * Node to convert a PointSet to a dots in a PathsData.
 */

class PointsToDotsNode : public NodeBase
{
  public:
	PointsToDotsNode();
	virtual ~PointsToDotsNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PointsToDotsNode(); }
};


PointsToDotsNode::PointsToDotsNode()
{
	makestr(Name, _("Points to dots"));
	makestr(type, "Points/PointsToDots");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "radius", new DoubleValue(1),1, _("Point radius"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

PointsToDotsNode::~PointsToDotsNode()
{
}

NodeBase *PointsToDotsNode::Duplicate()
{
	PointsToDotsNode *node = new PointsToDotsNode();
	node->DuplicateBase(this);
	return node;
}

int PointsToDotsNode::GetStatus()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;
	int isnum;
	getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;

	return NodeBase::GetStatus();
}

int PointsToDotsNode::Update()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;

	int isnum;
	double radius = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;

	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[2]->GetData());
	if (!out) {
		out = new LPathsData();
		properties.e[2]->SetData(out, 1);
	}

	out->clear();
	for (int c=0; c<set->NumPoints(); c++) {
		out->pushEmpty();
		out->appendEllipse(set->points.e[c]->p, radius, radius, 0, 0, 4, 1);
	}
	out->FindBBox();

	properties.e[2]->Touch();
	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}

int PointsToDotsNode::UpdatePreview()
{
	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[2]->GetData());
	if (!out) return 1;

	LaxImage *img = out->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	} else {
		if (total_preview) total_preview->dec_count();
		total_preview = nullptr;
	}
	return 1;
}


//------------------------ PointsToBarChartNode ------------------------

/*! 
 * Convert PointSet to a bar chart, sorted by x.
 */

class PointsToBarChartNode : public NodeBase
{
  public:
	PointsToBarChartNode();
	virtual ~PointsToBarChartNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PointsToBarChartNode(); }
};


PointsToBarChartNode::PointsToBarChartNode()
{
	makestr(Name, _("Points to bar chart"));
	makestr(type, "Points/PointsToBarChart");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "x", new DoubleValue(0),1, _("X"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "y", new DoubleValue(0),1, _("Y"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "w", new DoubleValue(1),1, _("Width"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "h", new DoubleValue(1),1, _("Height"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "gap", new DoubleValue(.1),1, _("Gap"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "bounds", nullptr,1, _("Data bounds"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

PointsToBarChartNode::~PointsToBarChartNode()
{
}

NodeBase *PointsToBarChartNode::Duplicate()
{
	PointsToBarChartNode *node = new PointsToBarChartNode();
	node->DuplicateBase(this);
	return node;
}

int PointsToBarChartNode::GetStatus()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;
	for (int c=1; c<6; c++) {
		int isnum;
		getNumberValue(properties.e[c]->GetData(), &isnum);
		if (!isnum) return -1;
	}

	return NodeBase::GetStatus();
}

int PointsToBarChartNode::Update()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;

	int isnum;
	double x = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	double y = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	double w = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;
	double h = getNumberValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) return -1;
	double gap = getNumberValue(properties.e[5]->GetData(), &isnum);
	if (!isnum) return -1;

	BBoxValue *databounds = dynamic_cast<BBoxValue*>(properties.e[6]->GetData());
	if (!databounds) {
		databounds = new BBoxValue();
		properties.e[6]->SetData(databounds, 1);
	}
	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[7]->GetData());
	if (!out) {
		out = new LPathsData();
		properties.e[7]->SetData(out, 1);
	}

	out->clear();
	PointSet dup;
	dup.CopyFrom(set, false);
	dup.SortX(true);
	double slotwidth = w / dup.NumPoints();
	double barwidth = slotwidth * (1-gap);
	dup.GetBBox(*databounds);
	databounds->addtobounds(flatpoint(dup.points.e[0]->p.x, 0));

	for (int c=0; c<dup.NumPoints(); c++) {
		out->pushEmpty();
		// double v = (dup.points.e[c]->p.y - databounds->miny) / databounds->boxheight();
		double v = (dup.points.e[c]->p.y) / databounds->boxheight(); //always use actual origin
		if (fabs(v) < .001) v = .001; //prevent path line construction from wigging out
		out->appendRect(x + (c+.5)*slotwidth-barwidth/2, y, barwidth, v * h);
	}

	properties.e[6]->Touch();
	properties.e[7]->Touch();
	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}

int PointsToBarChartNode::UpdatePreview()
{
	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[7]->GetData());
	if (!out) return 1;

	LaxImage *img = out->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	} else {
		if (total_preview) total_preview->dec_count();
		total_preview = nullptr;
	}
	return 1;
}


//------------------------ PointsToBarsNode ------------------------

/*! 
 * Node to convert a PointSet to a bars in a PathsData, sorted by point index order.
 */

class PointsToBarsNode : public NodeBase
{
  public:
	PointsToBarsNode();
	virtual ~PointsToBarsNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PointsToBarsNode(); }
};


PointsToBarsNode::PointsToBarsNode()
{
	makestr(Name, _("Points to bars"));
	makestr(type, "Points/PointsToBars");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, NULL, 0, false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "slotwidth", new DoubleValue(1),1, _("Slot width"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "barwidth", new DoubleValue(.8),1, _("Bar Width"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "scale", new DoubleValue(1),1, _("Scale"), nullptr,0,true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}

PointsToBarsNode::~PointsToBarsNode()
{
}

NodeBase *PointsToBarsNode::Duplicate()
{
	PointsToBarsNode *node = new PointsToBarsNode();
	node->DuplicateBase(this);
	return node;
}

int PointsToBarsNode::GetStatus()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;
	int isnum;
	getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;

	return NodeBase::GetStatus();
}

int PointsToBarsNode::Update()
{
	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[0]->GetData());
	if (!set) return -1;

	int isnum;
	double slotwidth = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	double barwidth = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	double vscale = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;

	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[4]->GetData());
	if (!out) {
		out = new LPathsData();
		properties.e[4]->SetData(out, 1);
	} else {
		properties.e[4]->Touch();
	}

	out->clear();
	for (int c=0; c<set->NumPoints(); c++) {
		out->pushEmpty();
		out->appendRect((c+.5)*slotwidth-barwidth/2, 0, barwidth, vscale * set->points.e[c]->p.y);
	}

	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}

int PointsToBarsNode::UpdatePreview()
{
	LPathsData *out = dynamic_cast<LPathsData*>(properties.e[4]->GetData());
	if (!out) return 1;

	LaxImage *img = out->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	} else {
		if (total_preview) total_preview->dec_count();
		total_preview = nullptr;
	}
	return 1;
}



//------------------------  PointSetNode -----------------------------

class PointSetNode : public NodeBase
{
  public:
	enum SetTypes {
		Empty,
		Grid,    // x,y
		HexGrid, // num on edge    
		RandomSquare, // n
		RandomCircle, // 
		MAX
	};
	SetTypes settype;
	
	PointSetNode(SetTypes ntype);
	virtual ~PointSetNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();
};


PointSetNode::PointSetNode(PointSetNode::SetTypes ntype)
{
	settype = ntype;

	if (settype == Grid) {
		makestr(type, "Points/Grid");
		makestr(Name, _("Point Grid"));

		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "x", new DoubleValue(0),1,  _("X"),     NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "y", new DoubleValue(0),1,  _("Y"),     NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "w", new DoubleValue(1),1,  _("Width"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "h", new DoubleValue(1),1, _("Height"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "nx", new IntValue(2),1,  _("Num X"),    NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "ny", new IntValue(2),1,  _("Num Y"),    NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "centers", new BooleanValue(false),1, _("Centers"), NULL));
		
	} else if (settype == HexGrid) {
		makestr(type, "Points/HexGrid");
		makestr(Name, _("Hex Point Grid"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "o", new FlatvectorValue(),1, _("Center"),   NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "length", new DoubleValue(1),1,  _("Edge"),  NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "num", new IntValue(4),1,  _("Num on edge"), NULL));
		
	} else if (settype == RandomSquare) {
		makestr(type, "Points/RandomRectangle");
		makestr(Name, _("Random Rectangle"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "o", new FlatvectorValue(),1, _("Center"),   NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "w", new DoubleValue(1),1,  _("Width"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "h", new DoubleValue(1),1, _("Height"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "num", new IntValue(4),1,  _("Num points"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "seed", new IntValue(0),1,  _("Seed"), NULL));

	} else if (settype == RandomCircle) {
		makestr(type, "Points/RandomCircle");
		makestr(Name, _("Random Circle"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "o", new FlatvectorValue(),1, _("Center"),   NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "radius", new DoubleValue(1),1,  _("Radius"),  NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "num", new IntValue(4),1,  _("Num points"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "seed", new IntValue(0),1,  _("Seed"), NULL));
	}

	PointSetValue *set = new PointSetValue();
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", set,1, _("Out"), NULL,0, false));

	Update();
	Wrap();
}

PointSetNode::~PointSetNode()
{
}

int PointSetNode::UpdatePreview()
{
	return 0;
	// LPathsData *pathsdata = dynamic_cast<LPathsData*>(properties.e[properties.n-1]->GetData());
	// if (!pathsdata) return 0;
	// LaxImage *img = pathsdata->GetPreview();
	// if (img) {
	// 	if (img != total_preview) {
	// 		if (total_preview) total_preview->dec_count();
	// 		total_preview = img;
	// 		total_preview->inc_count();
	// 	}
	// }
	// return 1;
}

NodeBase *PointSetNode::Duplicate()
{
	PointSetNode *newnode = new PointSetNode(settype);

	for (int c=0; c<properties.n-1; c++) {
		Value *v = properties.e[c]->GetData();
		if (v) newnode->properties.e[c]->SetData(v->duplicate(), 1);
	}

	return newnode;
}

//0 ok, -1 bad ins, 1 just needs updating
int PointSetNode::GetStatus()
{
	char types[7];
	const char *sig = "nnnnnnnvnn    vnnnn  vnnn   ";
	int sigoff = 0;

		// Grid,    // x,y
		// HexGrid, // num on edge    
		// RandomSquare, // n
		// RandomCircle, // 
		
#define OFFGRID     0
#define OFFHEX      7
#define OFFRSQUARE  14
#define OFFRCIRCLE  21

	if      (settype == Grid)         sigoff = OFFGRID   ;
	else if (settype == HexGrid)      sigoff = OFFHEX    ;
	else if (settype == RandomSquare) sigoff = OFFRSQUARE;
	else if (settype == RandomCircle) sigoff = OFFRCIRCLE;
	
	for (int c=0; c<properties.n-1; c++) {
		Value *data = properties.e[c]->GetData();
		if (!data) { types[c] = ' '; continue; }

		// stype = data->whattype();
		if (isNumberType(data, nullptr)) types[c] = 'n';
		// else if (!strcmp(stype, "StringValue")) types[c] = 's';
		else if (data->type() == VALUE_Flatvector) types[c] = 'v';
		else types[c] = ' ';
	}
	for (int c=properties.n-1; c<6; c++) types[c] = ' ';
	types[6] = '\0';

	if (strncmp(sig+sigoff, types, 6)) return -1;

	return NodeBase::GetStatus();
}

//0 ok, -1 bad ins, 1 just needs updating
int PointSetNode::Update()
{
	Error(nullptr);
	if (GetStatus() == -1) return -1;

	PointSetValue *set = dynamic_cast<PointSetValue*>(properties.e[properties.n-1]->GetData());
	if (!set) {
		set = new PointSetValue();
		properties.e[properties.n-1]->SetData(set, 1);
	} else {
		set->Flush();
		properties.e[properties.n-1]->Touch();
	}

	if (settype == Grid) {
		int isnum;
		double x = getNumberValue(properties.e[0]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad x")); return -1; }
		double y = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad y")); return -1; }
		double w = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad width")); return -1; }
		double h = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad height")); return -1; }
		int nx = getNumberValue(properties.e[4]->GetData(), &isnum);
		if (!isnum || nx <= 0) { makestr(error_message, _("Bad num x")); return -1; }
		int ny = getNumberValue(properties.e[5]->GetData(), &isnum);
		if (!isnum || ny <= 0) { makestr(error_message, _("Bad num y")); return -1; }
		bool centers = getNumberValue(properties.e[6]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Expected boolean for centers")); return -1; }

		if (centers) {
			double dw = w / nx;
			x += dw/2;
			double dh = h / ny;
			y += dh/2;
			w -= dw;
			h -= dh;
		}
		set->CreateGrid(nx,ny,x,y,w,h, LAX_LRTB);

	} else if (settype == HexGrid) {
		flatvector o;
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[0]->GetData());
		if (fv) o = fv->v;
		else {
			AffineValue *av = dynamic_cast<AffineValue*>(properties.e[0]->GetData());
			if (av) o = av->origin();
			else { 
				Error(_("Expected vector"));
				return -1;
			}
		}
		int isnum = 0;
		double l = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad edge length")); return -1; }

		int n = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad num on edge")); return -1; }

		set->CreateHexChunk(l, n);
		for (int c=0; c<set->points.n; c++) set->points.e[c]->p += o;
	
	} else if (settype == RandomSquare) {
		flatvector o;
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[0]->GetData());
		if (fv) o = fv->v;
		else {
			AffineValue *av = dynamic_cast<AffineValue*>(properties.e[0]->GetData());
			if (av) o = av->origin();
			else { 
				Error(_("Expected vector"));
				return -1;
			}
		}
		int isnum = 0;
		double w = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad width")); return -1; }
		double h = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad height")); return -1; }

		int n = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad num points")); return -1; }

		int seed = getNumberValue(properties.e[4]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad seed")); return -1; }

		set->CreateRandomPoints(n, seed, o.x-w/2,o.x+w/2, o.y-h/2, o.y+h/2);

	} else if (settype == RandomCircle) {
		flatvector o;
		FlatvectorValue *fv = dynamic_cast<FlatvectorValue*>(properties.e[0]->GetData());
		if (fv) o = fv->v;
		else {
			AffineValue *av = dynamic_cast<AffineValue*>(properties.e[0]->GetData());
			if (av) o = av->origin();
			else { 
				Error(_("Expected vector"));
				return -1;
			}
		}
		int isnum = 0;
		double r = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad radius")); return -1; }
		
		int n = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad num points")); return -1; }

		int seed = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad seed")); return -1; }

		set->CreateRandomRadial(n, seed, o.x, o.y, r);
	}

	// set->FindBBox();
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}

Laxkit::anObject *newPointsGridNode(int p, Laxkit::anObject *ref)
{
	return new PointSetNode(PointSetNode::Grid);
}

Laxkit::anObject *newPointsHexNode(int p, Laxkit::anObject *ref)
{
	return new PointSetNode(PointSetNode::HexGrid);
}

Laxkit::anObject *newPointsRandomSquareNode(int p, Laxkit::anObject *ref)
{
	return new PointSetNode(PointSetNode::RandomSquare);
}

Laxkit::anObject *newPointsRandomRadialNode(int p, Laxkit::anObject *ref)
{
	return new PointSetNode(PointSetNode::RandomCircle);
}


//------------------------  PathGeneratorNode -----------------------------

class PathGeneratorNode : public NodeBase
{
  public:
	enum PathTypes {
		Square,
		Circle,     //num points, is bez, start, end
		Rectangle, //x,y,w,h, roundh, roundv
		Polygon, // vertices, winds
		Svgd, // svg style d string
		Function, //y=f(x), x range, step
		FunctionT, //p=(x(t), y(t)), t range, step
		FunctionRofT, //polar coordinate = r(theta)
		FunctionPolarT, //polar coord = r(t), theta(t)
		MAX
	};
	PathTypes pathtype;
	LPathsData *path;

	PathGeneratorNode(PathTypes ntype);
	virtual ~PathGeneratorNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();
};

PathGeneratorNode::PathGeneratorNode(PathGeneratorNode::PathTypes ntype)
{
	pathtype = ntype;

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "pointset", new BooleanValue(false),1,  _("As pointset"), _("If true, output a PointSet. If false, output a PathsData")));

	if (pathtype == Square) {
		makestr(type, "Paths/Square");
		makestr(Name, _("Square"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "edge", new DoubleValue(1),1,  _("Edge"),  NULL));
		//no inputs! always square 0..1

	} else if (pathtype == Rectangle) {
		makestr(type, "Paths/Rectangle");
		makestr(Name, _("Rectangle"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "x", new DoubleValue(0),1,  _("X"),     NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "y", new DoubleValue(0),1,  _("Y"),     NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "w", new DoubleValue(1),1,  _("Width"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "h", new DoubleValue(1),1, _("Height"), NULL));
		// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "r1", new DoubleValue(0),1, _("Round 1"), NULL));
		// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "r2", new DoubleValue(0),1, _("Round 2"), NULL));
		// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "r3", new DoubleValue(0),1, _("Round 3"), NULL));
		// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "r4", new DoubleValue(0),1, _("Round 4"), NULL));

	} else if (pathtype == Circle) {
		makestr(type, "Paths/Circle");
		makestr(Name, _("Circle"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "n",      new IntValue(4),1,     _("Points"), _("Number of points")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "radius", new DoubleValue(1),1,  _("Radius"),  NULL));
		// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Start",  new DoubleValue(0),1,  _("Start"),  _("Start angle")));
		// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "End",    new DoubleValue(0),1,  _("End"),    _("End angle. Same as start means full circle.")));
		//AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Smooth", new BooleanValue(1),1, _("Smooth"), _("Is a bezier curve"));
		// if you don't want smooth, use polygon

	} else if (pathtype == Polygon) {
		makestr(type, "Paths/Polygon");
		makestr(Name, _("Polygon"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "n",      new IntValue(4),1,     _("Points"), _("Number of points")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Radius",new DoubleValue(1),1,   _("Radius"),_("Radius")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Winding",new DoubleValue(1),1,  _("Winding"),_("Angle between points is winding*360/n")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Offset", new DoubleValue(0),1,  _("Offset"), _("Rotate all points by this many degrees")));

	} else if (pathtype == Function) {
		makestr(type, "Paths/FunctionX");
		makestr(Name, _("Function of x"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Y", new StringValue("x"),1, _("y(x)"),_("A function of x")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Minx", new DoubleValue(0),1,  _("Min x"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Maxx", new DoubleValue(1),1,  _("Max x"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step", new DoubleValue(.1),1, _("Step"), NULL));

	} else if (pathtype == FunctionT) {
		makestr(type, "Paths/FunctionT");
		makestr(Name, _("Function of t"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "X", new StringValue("t"),1, _("x(t)")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Y", new StringValue("t"),1, _("y(t)")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Mint", new DoubleValue(0),1,  _("Min t"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Maxt", new DoubleValue(1),1,  _("Max t"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step", new DoubleValue(.1),1, _("Step"), NULL));

	} else if (pathtype == FunctionRofT) {
		makestr(type, "Paths/PolarR");
		makestr(Name, _("Polar function r(theta)"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "r", new StringValue("theta"),1, _("r(theta)")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Mint", new DoubleValue(0),1,  _("Min theta"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Maxt", new DoubleValue(1),1,  _("Max theta"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step", new DoubleValue(.1),1, _("Step"), NULL));

	} else if (pathtype == FunctionPolarT) {
		makestr(type, "Paths/PolarT");
		makestr(Name, _("Polar function r(t), theta(t)"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "r",     new StringValue("t"),1, _("r(t)")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "theta", new StringValue("t"),1, _("theta(t)")));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Mint", new DoubleValue(0),1,  _("Min t"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Maxt", new DoubleValue(1),1,  _("Max t"), NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step", new DoubleValue(.1),1, _("Step"), NULL));

	} else if (pathtype == Svgd) {
		makestr(type, "Paths/Svgd");
		makestr(Name, _("Svg d"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "d",      new StringValue(""),1, _("d"), _("Svg style d path string")));
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "line",   new DoubleValue(.1),1,         _("Line width"), NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "stroke", new ColorValue(1.,0.,0.,1.),1, _("Stroke"), NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "fill",   new ColorValue(1.,0.,1.,0.),1, _("Fill"), NULL));

	path = new LPathsData();
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", path,1, _("Out"), NULL,0, false));

	Update();
	Wrap();
}

PathGeneratorNode::~PathGeneratorNode()
{
}

int PathGeneratorNode::UpdatePreview()
{
	Previewable *obj = dynamic_cast<Previewable*>(properties.e[properties.n-1]->GetData());
	if (!obj) return 0;
	LaxImage *img = obj->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	}
	return 1;
}

NodeBase *PathGeneratorNode::Duplicate()
{
	PathGeneratorNode *newnode = new PathGeneratorNode(pathtype);

	for (int c=0; c<properties.n-1; c++) {
		Value *v = properties.e[c]->GetData();
		if (v) newnode->properties.e[c]->SetData(v->duplicate(), 1);
	}

	return newnode;
}

//0 ok, -1 bad ins, 1 just needs updating
int PathGeneratorNode::GetStatus()
{
	char types[6];
	const char *stype;
	const char *sig = "n    nnnn nn   nnnn snnn ssnnns    snnn ssnnn";
	int sigoff = 0;

	if (!dynamic_cast<BooleanValue*>(properties.e[0]->GetData())) {
		return -1;
	}

#define OFFSQUARE     0
#define OFFRECTANGLE  5
#define OFFCIRCLE     10
#define OFFPOLYGON    15
#define OFFFUNCTION   20
#define OFFFUNCTIONT  25
#define OFFSVGD       30
#define OFFPOLARRofT  35
#define OFFPOLART     40

	if      (pathtype == Square)         sigoff = OFFSQUARE;
	else if (pathtype == Circle)         sigoff = OFFCIRCLE;
	else if (pathtype == Rectangle)      sigoff = OFFRECTANGLE;
	else if (pathtype == Polygon)        sigoff = OFFPOLYGON;
	else if (pathtype == Svgd)           sigoff = OFFSVGD;
	else if (pathtype == Function)       sigoff = OFFFUNCTION;
	else if (pathtype == FunctionT)      sigoff = OFFFUNCTIONT;
	else if (pathtype == FunctionRofT)   sigoff = OFFPOLARRofT;
	else if (pathtype == FunctionPolarT) sigoff = OFFPOLART;

	Value *data = nullptr;
	for (int c=0; c<properties.n-1-3; c++) {
		data = properties.e[c+1]->GetData();
		if (!data) { types[c] = ' '; continue; }
		stype = data->whattype();
		if (isNumberType(data, nullptr)) types[c] = 'n';
		else if (!strcmp(stype, "StringValue")) types[c] = 's';
		else types[c] = ' ';
	}
	for (int c=properties.n-2-3; c<5; c++) types[c] = ' ';
	types[5] = '\0';

	if (strncmp(sig+sigoff, types, 5)) return -1;

	double d;
	if (!isNumberType(properties.e[properties.n-4]->GetData(), &d) || d < 0) {
		Error(_("Line width must be >= 0"));
		return -1;
	}
	data = properties.e[properties.n-3]->GetData();
	if (data->type() != VALUE_Color) return -1;
	data = properties.e[properties.n-2]->GetData();
	if (data->type() != VALUE_Color) return -1;

	return NodeBase::GetStatus();
}

//0 ok, -1 bad ins, 1 just needs updating
int PathGeneratorNode::Update()
{
	Error(nullptr);
	if (GetStatus() == -1) return -1;

	PointSetValue *set = nullptr;
	LPathsData *path = nullptr;
	bool doset = dynamic_cast<BooleanValue*>(properties.e[0]->GetData())->i;
	if (doset) {
		set = dynamic_cast<PointSetValue*>(properties.e[properties.n-1]->GetData());
		if (!set) {
			set = new PointSetValue();
			properties.e[properties.n-1]->SetData(set, 1);

		} else {
			set->Flush();
		}
	} else {
		path = dynamic_cast<LPathsData*>(properties.e[properties.n-1]->GetData());
		if (!path) {
			path = new LPathsData();
			properties.e[properties.n-1]->SetData(path, 1);

		} else {
			path->clear();
		}
	}

	int isnum;
	double linewidth = getNumberValue(properties.e[properties.n-4]->GetData(), &isnum);
	if (!isnum) { makestr(error_message, _("Bad line width")); return -1; }
	Value *v;
	v = properties.e[properties.n-3]->GetData();
	if (v->type() != VALUE_Color) return -1;
	ColorValue *stroke = static_cast<ColorValue*>(v);
	v = properties.e[properties.n-2]->GetData();
	if (v->type() != VALUE_Color) return -1;
	ColorValue *fill = static_cast<ColorValue*>(v);

	if (pathtype == Square) {
		int isnum;
		double e = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (path) path->appendRect(0,0,e,e);
		else {
			set->AddPoint(flatpoint(0,0));
			set->AddPoint(flatpoint(e,0));
			set->AddPoint(flatpoint(e,e));
			set->AddPoint(flatpoint(0,e));
		}

	} else if (pathtype == Rectangle) {
		// path->clear();
		double x = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad x")); return -1; }
		double y = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad y")); return -1; }
		double w = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad width")); return -1; }
		double h = getNumberValue(properties.e[4]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad height")); return -1; }

		if (path) path->appendRect(x,y,w,h);
		else {
			set->AddPoint(flatpoint(x,y));
			set->AddPoint(flatpoint(x+w,y));
			set->AddPoint(flatpoint(x+w,y+h));
			set->AddPoint(flatpoint(x,y+h));
		}

	} else if (pathtype == Circle) {
		// path->clear();
		int n = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum || n <= 0) {makestr(error_message, _("Bad number of points")); return -1; }
		double r = getNumberValue(properties.e[2]->GetData(), &isnum);

		if (path) path->appendEllipse(flatpoint(), r,r, 2*M_PI, 0, n, 1);
		else set->CreateCircle(n, 0,0, r);

	} else if (pathtype == Polygon) {
		// make an n sided polygon
		// path->clear();
		
		int n = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum || n <= 0) {makestr(error_message, _("Bad number of points")); return -1; }
		double radius = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) {makestr(error_message, _("Bad radius number")); return -1; }
		double winding = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) {makestr(error_message, _("Bad winding number")); return -1; }
		double offset = getNumberValue(properties.e[4]->GetData(), &isnum);
		if (!isnum) {makestr(error_message, _("Bad offset number")); return -1; }

		double anglediff = winding * 2*M_PI / n;
		for (int c=0; c<n; c++) {
			double angle = offset + c * anglediff;
			if (path) path->append(radius * cos(angle), radius * sin(angle));
			else set->AddPoint(flatpoint(radius * cos(angle), radius * sin(angle)));
		}
		if (path) path->close();		

	} else if (pathtype == Function || pathtype == FunctionRofT) {
		int isnum;
		StringValue *expr = dynamic_cast<StringValue*>(properties.e[1]->GetData());
		if (!expr) { makestr(error_message, _("Expression must be a string")); return -1; }
		const char *expression = expr->str;

		double start = getNumberValue(properties.e[2]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad min")); return -1; }
		double end = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad max")); return -1; }
		double step = getNumberValue(properties.e[4]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad step")); return -1; }

		if ((start < end && step <= 0) || (start > end && step >= 0)) {
			makestr(error_message, _("Bad step value"));
			return -1;
		}
		if (start == end) {
			makestr(error_message, _("Start can't equal end"));
			return -1;
		}

		// path->clear();

		DoubleValue *xx = new DoubleValue();
		ValueHash hash;
		const char *param = "x";
		if (pathtype == FunctionRofT) param = "theta";
		hash.push(param, xx);
		xx->dec_count();
		int status;
		// int pointsadded = 0;
		ErrorLog log;

		// *** need to construct a reasonable context
		for (double x = start; (start < end && x <= end) || (start > end && x>=end); x += step) {
			Value *ret = nullptr;
			xx->d = x;
			status = laidout->calculator->EvaluateWithParams(expression,-1, nullptr, &hash, &ret, &log);
			if (status != 0 || !ret) {
				char *er = log.FullMessageStr();
				if (er) {
					makestr(error_message, er);
					delete[] er;
				}
				if (ret) ret->dec_count();
				return -1;
			}

			double ret_valx = getNumberValue(ret, &isnum);
			ret->dec_count();
			if (!isnum) {
				makestr(error_message, _("Function returned non-number."));
				return -1;
			}

			if (path) {
				if (pathtype == FunctionRofT) path->append(ret_valx * cos(x), ret_valx * sin(x));
				else path->append(x, ret_valx);
			} else {
				if (pathtype == FunctionRofT) set->AddPoint(flatpoint(ret_valx * cos(x), ret_valx * sin(x)));
				else set->AddPoint(flatpoint(x, ret_valx));
			}
		}
		// hash.flush();

	} else if (pathtype == FunctionT || pathtype == FunctionPolarT) {
		int isnum;
		StringValue *expr = dynamic_cast<StringValue*>(properties.e[1]->GetData());
		if (!expr) { makestr(error_message, _("Expression must be a string")); return -1; }
		const char *expressionx = expr->str;

		expr = dynamic_cast<StringValue*>(properties.e[2]->GetData());
		if (!expr) { makestr(error_message, _("Expression must be a string")); return -1; }
		const char *expressiony = expr->str;

		double start = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad min")); return -1; }
		double end = getNumberValue(properties.e[4]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad max")); return -1; }
		double step = getNumberValue(properties.e[5]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad step")); return -1; }
		

		if ((start < end && step <= 0) || (start > end && step >= 0)) {
			makestr(error_message, _("Bad step value"));
			return -1;
		}
		if (start == end) {
			makestr(error_message, _("Start can't equal end"));
			return -1;
		}

		DoubleValue *xx = new DoubleValue();
		ValueHash hash;
		hash.push("t", xx);
		xx->dec_count();
		int status;
		// int pointsadded = 0;
		ErrorLog log;

		// *** need to construct a reasonable context
		for (double x = start; (start < end && x <= end) || (start > end && x>=end); x += step) {
			xx->d = x;
			Value *ret = nullptr;
			status = laidout->calculator->EvaluateWithParams(expressionx,-1, nullptr, &hash, &ret, &log);
			if (status != 0 || !ret) {
				char *er = log.FullMessageStr();
				if (er) {
					makestr(error_message, er);
					delete[] er;
				}
				if (ret) ret->dec_count();
				return -1;
			}

			double ret_valx = getNumberValue(ret, &isnum);
			ret->dec_count();
			ret = nullptr;
			if (!isnum) {
				makestr(error_message, _("X function returned non-number."));
				return -1;
			}

			status = laidout->calculator->EvaluateWithParams(expressiony,-1, nullptr, &hash, &ret, &log);
			if (status != 0 || !ret) {
				char *er = log.FullMessageStr();
				if (er) {
					makestr(error_message, er);
					delete[] er;
				}
				if (ret) ret->dec_count();
				return -1;
			}

			double ret_valy = getNumberValue(ret, &isnum);
			ret->dec_count();
			if (!isnum) {
				makestr(error_message, _("Y function returned non-number."));
				return -1;
			}

			if (path) {
				if (pathtype == FunctionPolarT) path->append(ret_valx * cos(ret_valy), ret_valx * sin(ret_valy));
				else path->append(ret_valx, ret_valy);
			} else {
				if (pathtype == FunctionPolarT) set->AddPoint(flatpoint(ret_valx * cos(ret_valy), ret_valx * sin(ret_valy)));
				else set->AddPoint(flatpoint(ret_valx, ret_valy));
			}
		}

	} else if (pathtype == Svgd) {
		StringValue *v = dynamic_cast<StringValue*>(properties.e[1]->GetData());
		if (path) path->appendSvg(v->str);
		else {
			//only adds vertices, no interpolated bez segments
			LaxInterfaces::Coordinate *coords = LaxInterfaces::SvgToCoordinate(v->str, 1, nullptr, nullptr);
			if (coords) {
				LaxInterfaces::Coordinate *p = coords, *start = coords;
				do {
					if (p->flags & POINT_VERTEX) set->AddPoint(p->p());
					p = p->next;
				} while (p && p != start);
			}
		}
	}

	if (path) {
		path->FindBBox();
		ScreenColor col(stroke->color.Red(), stroke->color.Green(), stroke->color.Blue(), stroke->color.Alpha());
		path->line(linewidth, -1, -1, &col);
		col.rgbf(fill->color.Red(), fill->color.Green(), fill->color.Blue(), fill->color.Alpha());
		path->fill(&col);
	}
	properties.e[properties.n-1]->Touch();
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}


Laxkit::anObject *newPathCircleNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Circle);
}

Laxkit::anObject *newPathRectangleNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Rectangle);
}

Laxkit::anObject *newPathSquareNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Square);
}

Laxkit::anObject *newPathPolygonNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Polygon);
}

Laxkit::anObject *newPathFunctionNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Function);
}

Laxkit::anObject *newPathFunctionTNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::FunctionT);
}

Laxkit::anObject *newPathFunctionRofTNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::FunctionRofT);
}

Laxkit::anObject *newPathFunctionPolarTNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::FunctionPolarT);
}

Laxkit::anObject *newPathSvgdNode(int p, Laxkit::anObject *ref)
{
	return new PathGeneratorNode(PathGeneratorNode::Svgd);
}


//------------------------ FindDrawableNode ------------------------

/*! 
 * Node to get a reference to other DrawableObjects in same page.
 */

class FindDrawableNode : public NodeBase
{
  public:
	FindDrawableNode();
	virtual ~FindDrawableNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref);

	// static SingletonKeeper keeper; //the def for domain enum
	// static ObjectDef *GetDef() { return dynamic_cast<ObjectDef*>(keeper.GetObject()); }
};

Laxkit::anObject *FindDrawableNode::NewNode(int p, Laxkit::anObject *ref)
{
	return new FindDrawableNode();
}

FindDrawableNode::FindDrawableNode()
{
	makestr(Name, _("Find Drawable"));
	makestr(type, "Drawable/FindDrawable");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "base",    NULL,1, _("Base"), _("Object from which to begin searching")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "pattern", new StringValue(),1, _("Pattern"), _("Pattern to search for"), 0, true));

	// AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "exact",    new BooleanValue(false),1,_("Exact"),   _("Match must be exact")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "regex",    new BooleanValue(false),1,_("regex"),   _("Pattern is a regular expression")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "above",    new BooleanValue(true),1, _("Above"),   _("Look in parents and parents other children. Else looks only in children.")));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "siblings", new BooleanValue(true),1, _("Siblings"),_("Look adjacent to base")));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "kids",     new BooleanValue(true),1, _("Kids"),    _("Look under base")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  nullptr,1, _("Found"), nullptr, 0, false));
}

FindDrawableNode::~FindDrawableNode()
{
}

NodeBase *FindDrawableNode::Duplicate()
{
	FindDrawableNode *node = new FindDrawableNode();
	node->DuplicateBase(this);
	return node;
}

int FindDrawableNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;
	StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!s) return -1;
	const char *pattern = s->str;
	if (isblank(pattern)) return -1;

	return NodeBase::GetStatus();
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int FindDrawableNode::Update()
{
	DrawableObject *base = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!base) {
		UpdatePreview();
		Wrap();
		return -1;
	}

	StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!s) return -1;
	const char *pattern = s->str;
	if (isblank(pattern)) return -1;

	bool regex    = dynamic_cast<BooleanValue*>(properties.e[2]->GetData())->i;
	bool above    = dynamic_cast<BooleanValue*>(properties.e[3]->GetData())->i;
	// bool exact = dynamic_cast<BooleanValue*>(properties.e[2]->GetData())->i;
	// bool siblings = dynamic_cast<BooleanValue*>(properties.e[4]->GetData())->i;
	// bool kids     = dynamic_cast<BooleanValue*>(properties.e[4]->GetData())->i;


	if (above) {
		while (base->GetDrawableParent()) base = base->GetDrawableParent();
	}

	ObjectIterator itr;
	DrawableObject *pnt = base;
	itr.SearchIn(pnt);
	itr.Pattern(pattern, regex, true, true, false, false);
	FieldPlace place;
	DrawableObject *found = dynamic_cast<DrawableObject*>(itr.Start(&place));
	
	properties.e[4]->SetData(found, 0);
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}

int FindDrawableNode::UpdatePreview()
{
	Previewable *obj = dynamic_cast<Previewable*>(properties.e[properties.n-1]->GetData());
	LaxImage *img = nullptr;
	if (obj) img = obj->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	} else {
		if (total_preview) total_preview->dec_count();
		total_preview = nullptr;
	}
	return 1;
}

//------------------------ DrawableInfoNode ------------------------

/*! 
 * Node for basic DrawableObject information, like name, parent, transform, and bounds..
 */

class DrawableInfoNode : public NodeBase
{
  public:
	DrawableInfoNode();
	virtual ~DrawableInfoNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new DrawableInfoNode(); }
};

DrawableInfoNode::DrawableInfoNode()
{
	makestr(Name, _("Drawable Info"));
	makestr(type, "Drawable/DrawableInfo");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",  NULL,1, _("In")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "name",      new StringValue(),1, _("Name"),      nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "parent",    nullptr,1,           _("Parent"),    nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "transform", new AffineValue(),1, _("Transform"), nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "fulltransform", new AffineValue(),1, _("Full Transform"),nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "bounds",    new BBoxValue(),1,   _("Bounds"),    nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "page",      new StringValue(),1, _("Page"),      nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "pagetype",  new StringValue(),1, _("Page type"), nullptr, 0, false));
	//AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "clippath", nullptr,1, _("ClipPath"), nullptr,0,false));
}

DrawableInfoNode::~DrawableInfoNode()
{
}

NodeBase *DrawableInfoNode::Duplicate()
{
	DrawableInfoNode *node = new DrawableInfoNode();
	node->DuplicateBase(this);
	return node;
}

int DrawableInfoNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;

	return NodeBase::GetStatus();
}

int DrawableInfoNode::Update()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;

	StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	DrawableObject *parent = dynamic_cast<DrawableObject*>(dr->GetParent());
	//if (dr && !dr->Selectable()) dr = nullptr;
	properties.e[2]->SetData(parent, 0);
	AffineValue *a = dynamic_cast<AffineValue*>(properties.e[3]->GetData());
	AffineValue *f = dynamic_cast<AffineValue*>(properties.e[4]->GetData());
	BBoxValue *b = dynamic_cast<BBoxValue*>(properties.e[5]->GetData());

	s->Set(dr->Id());
	a->m(dr->m());
	Affine ff = dr->GetTransformToContext(false, 0);
	f->set(ff);
	b->setbounds(dr);

	for (int c=1; c<properties.n; c++) properties.e[c]->modtime = times(NULL);

	//find page
	StringValue *pg    = dynamic_cast<StringValue*>(properties.e[6]->GetData());
	StringValue *ptype = dynamic_cast<StringValue*>(properties.e[7]->GetData());
	LaxInterfaces::SomeData *pnt = dr;
	while (pnt->GetParent()) pnt = pnt->GetParent();
	Page *page = dynamic_cast<Page*>(pnt->ResourceOwner());
	Document *doc = nullptr;
	int i = (page ? laidout->project->LocatePage(page, &doc) : -1);
	if (page) {
		if (page->label) {
			pg->Set(page->label);
		} else {
			if (i == -1) {
				pg->Set("??");
			} else {
				char str[20];
				sprintf(str, "%d", i+1);
				pg->Set(str);
			}
		}
		ptype->Set(doc ? doc->imposition->PageTypeName(page->pagestyle->pagetype) : "");
	} else {
		pg->Set("none");
		ptype->Set("");
	}

	return NodeBase::Update();
}


//------------------------ PageInfoNode ------------------------

/*! 
 * Node for getting info about the page a DrawableObject is on.
 */

class PageInfoNode : public NodeBase
{
  public:
	PageInfoNode();
	virtual ~PageInfoNode();

	virtual int GetStatus();
	virtual int Update();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new PageInfoNode(); }
};

PageInfoNode::PageInfoNode()
{
	makestr(Name, _("Page Info"));
	makestr(type, "Drawable/PageInfo");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",  NULL,1, _("In")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "pagelabel", new StringValue(),1, _("Page label"),nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "pagetype",  new StringValue(),1, _("Page type"), nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "bounds",    new BBoxValue(),1,   _("Bounds"),    nullptr, 0, false));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "outline",   nullptr,1,           _("Outline"),   nullptr, 0, false));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "margin",    nullptr,1,           _("Margin"),    nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "style",     nullptr,1,           _("Style"),     nullptr, 0, false));
}

PageInfoNode::~PageInfoNode()
{
}

NodeBase *PageInfoNode::Duplicate()
{
	PageInfoNode *node = new PageInfoNode();
	node->DuplicateBase(this);
	return node;
}

int PageInfoNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;

	return NodeBase::GetStatus();
}

int PageInfoNode::Update()
{
	Error(nullptr);

	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) {
		Error(_("In must be a drawable object"));
		return -1;
	}

	StringValue    *plabel  = dynamic_cast<StringValue *>(properties.e[1]->GetData());
	StringValue    *ptype   = dynamic_cast<StringValue *>(properties.e[2]->GetData());
	BBoxValue      *bounds  = dynamic_cast<BBoxValue *>(properties.e[3]->GetData());
	
	for (int c=1; c<properties.n; c++) properties.e[c]->Touch();

	//find page
	LaxInterfaces::SomeData *pnt = dr;
	while (pnt->GetParent()) pnt = pnt->GetParent();
	Page *page = dynamic_cast<Page*>(pnt->ResourceOwner());
	Document *doc = nullptr;
	int i = (page ? laidout->project->LocatePage(page, &doc) : -1);
	if (page) {
		if (page->label) {
			plabel->Set(page->label);
		} else {
			if (i == -1) {
				plabel->Set("??");
			} else {
				char str[20];
				sprintf(str, "%d", i+1);
				plabel->Set(str);
			}
		}
		ptype->Set(doc ? doc->imposition->PageTypeName(page->pagestyle->pagetype) : "");

		bounds->setbounds(page->pagestyle->outline);
		properties.e[4]->SetData(page->pagestyle, 0);

	} else {
		plabel->Set("none");
		ptype->Set("");
		Error(_("Missing page!"));
	}

	return NodeBase::Update();
}


//--------------------------- SetupDataObjectNodes() -----------------------------------------

/*! Install default built in node types to factory.
 * This is called when the NodeGroup::NodeFactory singleton is created.
 */
int SetupDataObjectNodes(Laxkit::ObjectFactory *factory)
{
	factory->DefineNewObject(getUniqueNumber(), "Drawable/CaptionData",       LCaptionDataNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageData",         LImageDataNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageDataInfo",     newLImageDataInfoNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/DrawableInfo",      DrawableInfoNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/PageInfo",          PageInfoNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/FindDrawable",      FindDrawableNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/DuplicateDrawable", DuplicateDrawableNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/DuplicateSingle",   DuplicateSingleNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/SetParent",         SetParentNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/KidsToSet",         KidsToSetNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/SetPositions",      SetPositionsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/SetScales",         SetScalesNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/RotateDrawables",   RotateDrawablesNode::NewNode,  NULL, 0);

	factory->DefineNewObject(getUniqueNumber(), "Paths/PathsData",         newPathsDataNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Circle",            newPathCircleNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Square",            newPathSquareNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/RectanglePath",     newPathRectangleNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Polygon",           newPathPolygonNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/PathFunctionX",     newPathFunctionNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/PathFunctionT",     newPathFunctionTNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/PathFunctionRofT",  newPathFunctionRofTNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/PathFunctionPolarT",newPathFunctionPolarTNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Svgd",              newPathSvgdNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/EndPoints",         EndPointsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Extrema",           ExtremaNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Corners",           CornersNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/SetOriginToBBox",   SetOriginBBoxNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Paths/Extrude",           ExtrudeNode::NewNode,  NULL, 0);


	factory->DefineNewObject(getUniqueNumber(), "Points/Grid",             newPointsGridNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/HexGrid",          newPointsHexNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/RandomRectangle",  newPointsRandomSquareNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/RandomCircle",     newPointsRandomRadialNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToDots",     PointsToDotsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToBars",     PointsToBarsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToBarChart", PointsToBarChartNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToDelaunay", PointsToDelaunayNode::NewNode,  NULL, 0);

	return 0;
}

} //namespace Laidout

