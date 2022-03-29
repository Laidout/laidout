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
// Copyright (C) 2018 by Tom Lechner
//


#include <lax/language.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/fileutils.h>
#include <lax/popupmenu.h>
#include <lax/units.h>
#include "nodes.h"
#include "nodeinterface.h"
#include "nodes-dataobjects.h"
#include "../calculator/calculator.h"
#include "../calculator/curvevalue.h"
#include "../dataobjects/lsomedataref.h"
#include "../dataobjects/objectfilter.h"
#include "../dataobjects/bboxvalue.h"
#include "../dataobjects/affinevalue.h"
#include "../dataobjects/imagevalue.h"
#include "../dataobjects/pointsetvalue.h"

//needs calculator... some other way to abstract this so we don't depend directly on laidout??
#include "../laidout.h"

#include <unistd.h>
#include <iostream>

//template implementation
#include <lax/lists.cc>
#include <lax/refptrstack.cc>

using namespace std;
#define DBG

using namespace Laxkit;
using namespace LaxInterfaces;

namespace Laidout {



//-------------------------------------- Define the built in nodes types --------------------------


//------------ DoubleNode

class DoubleNode : public NodeBase
{
  public:
	DoubleNode(double d);
	virtual ~DoubleNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};


DoubleNode::DoubleNode(double d)
{
	makestr(Name, _("Value"));
	makestr(type, "Value");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", new DoubleValue(d), 1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "V", NULL, 1, _("V"),NULL,0, false));
	properties.e[1]->SetData(properties.e[0]->GetData(), 0);
}

DoubleNode::~DoubleNode()
{
}

int DoubleNode::Update()
{
	 //just copy reference to out
	properties.e[1]->SetData(properties.e[0]->GetData(), 0);
	Touch();
	return NodeBase::Update();
}

int DoubleNode::GetStatus()
{
	return NodeBase::GetStatus();
}

NodeBase *DoubleNode::Duplicate()
{
	int isnum;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	DoubleNode *newnode = new DoubleNode(d);
	newnode->DuplicateBase(this);
	return newnode;
}

Laxkit::anObject *newDoubleNode(int p, Laxkit::anObject *ref)
{
	return new DoubleNode(0);
}


//------------ ExpandVectorNode

class ExpandVectorNode : public NodeBase
{
  public:
	int dims;
	ExpandVectorNode(int dimensions, Value *v, int absorb); //up to 4
	virtual ~ExpandVectorNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

ExpandVectorNode::ExpandVectorNode(int dimensions, Value *v, int absorb)
{
	makestr(Name, _("Expand Vector"));
	dims = dimensions;
	makestr(type, "ExpandVector");
//	if (dimensions == 2) {
//		makestr(type, "Expand2");
//	} else if (dimensions == 3) {
//		makestr(type, "Expand3");
//	} else { //if (dimensions == 4) {
//		makestr(type, "Expand4");
//	}

	const char *labels[] = { _("x"), _("y"), _("z"), _("w") };
	const char *names[]  = {   "x" ,   "y" ,   "z" ,   "w"  };

	if (!v) v = new FlatvectorValue();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("V"), v, absorb)); 

	for (int c=0; c<dims; c++) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, names[c],
					new DoubleValue(), 1, labels[c])); 
	}
}

ExpandVectorNode::~ExpandVectorNode()
{
}

NodeBase *ExpandVectorNode::Duplicate()
{
	Value *v = properties.e[0]->GetData();
	if (v) {
		int vtype = v->type();
		if (   vtype == VALUE_Flatvector
			|| vtype == VALUE_Spacevector
			|| vtype == VALUE_Quaternion
			|| vtype == VALUE_Real
			|| vtype == VALUE_Color)
		  v = v->duplicate();
		else v = NULL;
	}
	ExpandVectorNode *newnode = new ExpandVectorNode(dims, v, 1);
	newnode->DuplicateBase(this);
	return newnode;
}

int ExpandVectorNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (v) {
		int vtype = v->type();
		if (!( vtype == VALUE_Flatvector
			|| vtype == VALUE_Spacevector
			|| vtype == VALUE_Quaternion
			|| vtype == VALUE_Real
			|| vtype == VALUE_Color))
		  return -1;
	} else return -1;

	if (!properties.e[dims]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ExpandVectorNode::Update()
{
	double vs[4];
	for (int c=0; c<dims; c++) { vs[c] = 0; }

	Value *v = properties.e[0]->GetData();
	if (!v) return -1;

	int vtype = v->type();
	if      (vtype == VALUE_Real)        vs[0] = dynamic_cast<DoubleValue*>(v)->d;
	else if (vtype == VALUE_Flatvector)  dynamic_cast<FlatvectorValue*>(v) ->v.get(vs);
	else if (vtype == VALUE_Spacevector) dynamic_cast<SpacevectorValue*>(v)->v.get(vs);
	else if (vtype == VALUE_Quaternion)  dynamic_cast<QuaternionValue*>(v)->v.get(vs);
	//else if (vtype == VALUE_Color) {
		//dynamic_cast<QuaternionValue*>(v)->v.get(vs);
	//}
	else return -1;

	for (int c=0; c<4; c++) {
		dynamic_cast<DoubleValue* >(properties.e[c+1]->data)->d = vs[c];
		properties.e[c+1]->modtime = times(NULL);
	}

	return NodeBase::Update();
}

Laxkit::anObject *newExpandVectorNode(int p, Laxkit::anObject *ref)
{
	return new ExpandVectorNode(4, NULL, 0);
}

//------------ FlatvectorNode, SpacevectorNode, QuaternionNode

class VectorNode : public NodeBase
{
  public:
	int dims; //must be 2,3, or 4
	VectorNode(int dimensions, double *initialvalues); //up to 4
	virtual ~VectorNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

VectorNode::VectorNode(int dimensions, double *initialvalues)
{
	dims = dimensions;
	Value *out = NULL;

	if (dimensions == 2) {
		makestr(Name, _("Vector2"));
		makestr(type, "Vector2");
		out = new FlatvectorValue(initialvalues ? flatvector(initialvalues) : flatvector());

	} else if (dimensions == 3) {
		makestr(Name, _("Vector3"));
		makestr(type, "Vector3");
		out = new SpacevectorValue(initialvalues ? spacevector(initialvalues) : spacevector());

	} else { //if (dimensions == 4) {
		makestr(Name, _("Quaternion"));
		makestr(type, "Vector4");
		out = new QuaternionValue(initialvalues ? Quaternion(initialvalues) : Quaternion());
	}

	const char *labels[] = { _("x"), _("y"), _("z"), _("w") };
	const char *names[]  = {   "x" ,   "y" ,   "z" ,   "w"  };

	for (int c=0; c<dims; c++) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, names[c],
					new DoubleValue(initialvalues ? initialvalues[c] : 0), 1, labels[c])); 
	}
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "V", out,1, _("V"),nullptr,0,false));
}

VectorNode::~VectorNode()
{
}

NodeBase *VectorNode::Duplicate()
{
	double vals[4];
	int isnum;
	for (int c=0; c<dims; c++) {
		vals[c] = getNumberValue(properties.e[c]->GetData(), &isnum);
	}
	VectorNode *newnode = new VectorNode(dims, vals);
	newnode->DuplicateBase(this);
	return newnode;
}

int VectorNode::GetStatus()
{
	int isnum = 0;
	for (int c=0; c<dims; c++) {
		Value *v = properties.e[c]->GetData();
		if (!v) return -1;
		getNumberValue(v,&isnum);
		if (!isnum && v->type() != VALUE_Set) return -1;
	}

	if (!properties.e[dims]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int VectorNode::Update()
{
	ClearError();

	int num_ins = dims;
	Value *ins[num_ins];
	for (int c=0; c<num_ins; c++) ins[c] = properties.e[c]->GetData();
	
	SetValue *setins[num_ins];
	SetValue *outset = nullptr;

	int max = 0;
	bool dosets = false;
	if (DetermineSetIns(num_ins, ins, setins, max, dosets) == -1) { //does not check contents of sets.
		//had a null input
		return -1;
	}

	FlatvectorValue *fv = nullptr;
	SpacevectorValue *sv = nullptr;
	QuaternionValue *qv = nullptr;

	if      (dims == 2) fv = UpdatePropType<FlatvectorValue> (properties.e[dims], dosets, max, outset);
    else if (dims == 3) sv = UpdatePropType<SpacevectorValue>(properties.e[dims], dosets, max, outset);
    else if (dims == 4) qv = UpdatePropType<QuaternionValue> (properties.e[dims], dosets, max, outset);

	DoubleValue *in[dims];
	for (int c=0; c<dims; c++) in[c] = nullptr;

	for (int c=0; c<max; c++) {
		int isnum;
		double vs[4];

		for (int c2=0; c2<dims; c2++) {
			in[c2] = GetInValue<DoubleValue>(c, dosets, in[c2], ins[c2], setins[c2]);
			vs[c2] = getNumberValue(in[c2],&isnum);
			if (!isnum) {
				Error(_("Bad number!"));
				return -1;
			}
		}

		if      (dims == 2) GetOutValue<FlatvectorValue> (c, dosets, fv, outset);
		else if (dims == 3) GetOutValue<SpacevectorValue>(c, dosets, sv, outset);
		else if (dims == 4) GetOutValue<QuaternionValue> (c, dosets, qv, outset);
		
		if      (dims == 2) fv->v = flatvector (vs);
		else if (dims == 3) sv->v = spacevector(vs);
		else if (dims == 4) qv->v = Quaternion (vs);
	}
	
	properties.e[dims]->Touch();

	return NodeBase::Update();
}

Laxkit::anObject *newFlatvectorNode(int p, Laxkit::anObject *ref)
{
	return new VectorNode(2, NULL);
}

Laxkit::anObject *newSpacevectorNode(int p, Laxkit::anObject *ref)
{
	return new VectorNode(3, NULL);
}

Laxkit::anObject *newQuaternionNode(int p, Laxkit::anObject *ref)
{
	return new VectorNode(4, NULL);
}


//------------ RectangleNode

class RectangleNode : public NodeBase
{
  public:
  	bool isbbox;
	RectangleNode(int as_bbox);
	virtual ~RectangleNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new RectangleNode(p); }
};

RectangleNode::RectangleNode(int as_bbox)
{
	isbbox = as_bbox;

	Id(isbbox ? "BBox" : "Rectangle");
	Label(isbbox ? _("BBox") : _("Rectangle"));
	makestr(type, isbbox ? "BBox" : "Rectangle");

	const char *names[]  = {   "x" ,   "y" ,   "width" ,   "height",   "minx" ,   "maxx" ,   "miny" ,   "maxy"  };
	const char *labels[] = { _("x"), _("y"), _("width"), _("height"),  _("Min x"), _("Max x"), _("Min y"), _("Max y") };

	for (int c=0; c<4; c++) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, names[(isbbox ? 4 : 0) + c], new DoubleValue(0), 1, labels[(isbbox ? 4 : 0) + c])); 
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "box", new BBoxValue(), 1, isbbox ? _("BBox") : _("Rect")));
}

RectangleNode::~RectangleNode()
{
}

NodeBase *RectangleNode::Duplicate()
{
	RectangleNode *newnode = new RectangleNode(isbbox);
	int isnum;
	for (int c=0; c<4; c++) {
		double v = getNumberValue(properties.e[c]->GetData(), &isnum);		
		newnode->properties.e[c]->SetData(new DoubleValue(v), 1);
	}
	newnode->DuplicateBase(this);
	return newnode;
}

int RectangleNode::GetStatus()
{
	int isnum = 0;
	for (int c=0; c<4; c++) {
		getNumberValue(properties.e[c]->GetData(),&isnum);
		if (!isnum) return -1;
	}

	if (!properties.e[4]->data) return 1; //needs updating to establish result

	return NodeBase::GetStatus(); //default checks mod times
}

int RectangleNode::Update()
{
	int isnum;
	double vs[4];
	for (int c=0; c<4; c++) {
		vs[c] = getNumberValue(properties.e[c]->GetData(),&isnum);
		if (!isnum) return -1;
	}

	if (isbbox) {
		if (!properties.e[4]->data) properties.e[4]->data = new BBoxValue(vs[0], vs[1], vs[2], vs[3]);
		else {
			BBoxValue *v = dynamic_cast<BBoxValue*>(properties.e[4]->data);
			v->setbounds(vs[0], vs[1], vs[2], vs[3]);
		}
	} else {
		if (!properties.e[4]->data) properties.e[4]->data = new BBoxValue(vs[0], vs[0]+vs[2], vs[1], vs[1]+vs[3]);
		else {
			BBoxValue *v = dynamic_cast<BBoxValue*>(properties.e[4]->data);
			v->setbounds(vs[0], vs[0]+vs[2], vs[1], vs[1]+vs[3]);
		}
	}

	properties.e[4]->Touch();

	return NodeBase::Update();
}


//------------ BBoxInfoNode

class BBoxInfoNode : public NodeBase
{
  public:
  	BBoxInfoNode();
	virtual ~BBoxInfoNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new BBoxInfoNode(); }
};

BBoxInfoNode::BBoxInfoNode()
{

	Id("BBoxInfo");
	Label(_("BBox Info"));
	makestr(type, "BBoxInfo");
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, _("In"), nullptr,0,false)); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "minx",   new DoubleValue(0),1, _("Minx"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "maxx",   new DoubleValue(0),1, _("Maxx"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "miny",   new DoubleValue(0),1, _("Miny"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "maxy",   new DoubleValue(0),1, _("Maxy"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "width",  new DoubleValue(0),1, _("Width"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "height", new DoubleValue(0),1, _("Height"), nullptr,0,false));
}

BBoxInfoNode::~BBoxInfoNode()
{
}

NodeBase *BBoxInfoNode::Duplicate()
{
	BBoxInfoNode *newnode = new BBoxInfoNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int BBoxInfoNode::GetStatus()
{
	if (!dynamic_cast<DoubleBBox*>(properties.e[0]->GetData())
		&& !dynamic_cast<SetValue*>(properties.e[0]->GetData())
		) return -1;
	return NodeBase::GetStatus(); //default checks mod times
}

int BBoxInfoNode::Update()
{
	DoubleBBox *bbox = dynamic_cast<DoubleBBox*>(properties.e[0]->GetData());
	SetValue *inset = nullptr;
	if (!bbox) {
		inset = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!inset) return -1;
	}

	DoubleValue *dv;
	SetValue *set[6]; 

	if (inset) {
		for (int c=1; c<7; c++) {
			set[c-1] = dynamic_cast<SetValue*>(properties.e[c]->GetData());
			if (!set[c-1]) {
				set[c-1] = new SetValue();
				properties.e[c]->SetData(set[c-1], 1);
			}
			while (set[c-1]->n() > inset->n()) set[c-1]->Remove(set[c-1]->n()-1);
			while (set[c-1]->n() < inset->n()) set[c-1]->Push(new DoubleValue(),1);
		}
	} else {
		for (int c=1; c<7; c++) {
			dv = dynamic_cast<DoubleValue*>(properties.e[c]->GetData());
			if (!dv) {
				dv = new DoubleValue();
				properties.e[c]->SetData(dv, 1);
			}
		}
	}

	for (int c=0; c<(inset ? inset->n() : 1); c++) {

		dv = dynamic_cast<DoubleValue*>(inset ? set[0]->e(c) : properties.e[1]->GetData());
		dv->d = bbox->minx;
		dv = dynamic_cast<DoubleValue*>(inset ? set[1]->e(c) : properties.e[2]->GetData());
		dv->d = bbox->maxx;
		dv = dynamic_cast<DoubleValue*>(inset ? set[2]->e(c) : properties.e[3]->GetData());
		dv->d = bbox->miny;
		dv = dynamic_cast<DoubleValue*>(inset ? set[3]->e(c) : properties.e[4]->GetData());
		dv->d = bbox->maxy;
		dv = dynamic_cast<DoubleValue*>(inset ? set[4]->e(c) : properties.e[5]->GetData());
		dv->d = bbox->boxwidth();
		dv = dynamic_cast<DoubleValue*>(inset ? set[5]->e(c) : properties.e[6]->GetData());
		dv->d = bbox->boxheight();
	}

	for (int c=1; c<7; c++) properties.e[c]->Touch();

	return NodeBase::Update();
}


//----------------------- BBoxPointNode ------------------------

/*! \class BBoxPointNode
 *
 * Do stuff.
 */
class BBoxPointNode : public NodeBase
{
  public:
	BBoxPointNode();
	virtual ~BBoxPointNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new BBoxPointNode(); }
};

BBoxPointNode::BBoxPointNode()
{
	makestr(type, "Drawable/BBoxPoint");
	makestr(Name, _("BBox Point"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,     _("Object"), _("A bounded object")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "x",   new DoubleValue(.5),1,  _("X"),  _("minx is 0, maxx is 1")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "y",   new DoubleValue(.5),1,  _("Y"),  _("miny is 0, maxy is 1")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

BBoxPointNode::~BBoxPointNode()
{
}

NodeBase *BBoxPointNode::Duplicate()
{
	BBoxPointNode *newnode = new BBoxPointNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int BBoxPointNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int BBoxPointNode::Update() //possible set ins
{
	ClearError();

	int num_ins = 3;
	Value *ins[3];
	ins[0] = properties.e[0]->GetData();
	ins[1] = properties.e[1]->GetData();
	ins[2] = properties.e[2]->GetData();

	SetValue *setins[3];
	SetValue *setouts[1];

	//int num_outs = 1;
	int outprops[1];
	outprops[0] = 3;

	int max = 0;
	//const char *err = nullptr;
	bool dosets = false;
	if (DetermineSetIns(num_ins, ins, setins, max, dosets) == -1) {; //does not check contents of sets.
		//had a null input
		return -1;
	}

	//establish outprop: make it either type, or set. do prop->Touch(). clamp to max. makes setouts[*] null or the out set
	FlatvectorValue *out1 = UpdatePropType<FlatvectorValue>(properties.e[outprops[0]], dosets, max, setouts[0]);
	
	DoubleBBox *in1 = nullptr; //dynamic_cast<DoubleValue*>(ins[0]);
	//DoubleValue *in2 = nullptr; //dynamic_cast<IntValue*>(ins[1]);
	//DoubleValue *in3 = nullptr; //dynamic_cast<LPathsData*>(ins[2]);
	double in2 = 0;
	double in3 = 0;

	for (int c=0; c<max; c++) {
		in1 = GetInValue<DoubleBBox> (c, dosets, in1, ins[0], setins[0]);
		//in2 = GetInValue<DoubleValue>(c, dosets, in2, ins[1], setins[1]);
		//in3 = GetInValue<DoubleValue>(c, dosets, in3, ins[2], setins[2]);

		// error check ins
		if (!in1) {
			Error(_("Missing in bbox"));
			return -1;
		}
		if (GetInNumber(c, dosets, in2, ins[1], setins[1]) != 0) {
			Error(_("Expected number"));
			return -1;
		}
		if (GetInNumber(c, dosets, in3, ins[2], setins[2]) != 0) {
			Error(_("Expected number"));
			return -1;
		}


		GetOutValue<FlatvectorValue>(c, dosets, out1, setouts[0]);
			

		// based on ins, update outs
		out1->v = in1->BBoxPoint(in2,in3);
	}


	return NodeBase::Update();
}


//------------ ColorNode

Laxkit::anObject *newColorNode(int p, Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	makestr(node->Name, _("Color"));
	makestr(node->type, "Color");

	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Color", new ColorValue("#ffffff"), 1, _("Color")));
	//----------
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Red"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Green"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Blue"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Alpha"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, false, _("Out"), new ColorValue("#ffffffff"), 1)); 
	return node;
}


//------------ IntNode

Laxkit::anObject *newIntNode(int p, Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	makestr(node->Name, _("Int"));
	makestr(node->type, "Basics/Integer");

	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Int", new IntValue(0), 1, _("Int")));
	return node;
}


//------------ BooleanNode

Laxkit::anObject *newBooleanNode(int p, Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	makestr(node->Name, _("Boolean"));
	makestr(node->type, "Basics/Boolean");

	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Bool", new BooleanValue(0), 1, _("Bool")));
	return node;
}

//------------ AffineNode

class AffineNode : public NodeBase
{
  public:
	enum NodeType { Abcdef, XYSxyAxy, Expand };
	NodeType atype;
	//int atype; //0 == a,b,c,d,e,f, 1= posx, posy, scalex, scaley, anglex, angley_off_90, 2 = xv, yv, pv, 

	AffineNode(NodeType ntype, const double *values);
	virtual ~AffineNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

AffineNode::AffineNode(AffineNode::NodeType ntype, const double *values)
{
	atype = ntype;

	const char **labels;
	const char *labels1[] = { _("xx"), _("xy"), _("yx"), _("yy"), _("x0"), _("y0") };
	const char *labels2[] = { _("X Scale"), _("Y Scale"), _("X Angle"), _("Y Angle"), _("x0"), _("y0") };
	const char **names;
	const char *names1[]  = {   "xx" ,   "xy" ,   "yx" ,   "yy" ,   "x0" ,   "y0"  };
	const char *names2[]  = { "xscale", "yscale", "xangle", "yangle", "x0",  "y0"  };
	const double v1[] = { 1,0, 0,1, 0,0 };
	const double v2[] = { 1,1, 0,0, 0,0 };

	bool isinput = false;
	if (atype == Abcdef) {
		makestr(Name, _("Affine"));
		makestr(type,   "Math/Affine");
		labels = labels1;
		names  = names1;
		if (!values) values = v1;

	} else if (atype == XYSxyAxy) {
		makestr(Name, _("Affine2"));
		makestr(type,   "Math/Affine2");
		labels = labels2;
		names  = names2;
		if (!values) values = v2;

	} else if (atype == Expand) {
		isinput = true;
		makestr(Name, _("Affine Expand"));
		makestr(type,   "Math/AffineExpand");
		labels = labels1;
		names  = names1;
		if (!values) values = v1;

		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Affine", new AffineValue(),1, _("Affine"))); 

//	} else {
//		makestr(Name, _("Affine Vectors"));
//		makestr(type,   "AffineV" );
//		labels = { _("X"), _("Y"), _("Position"), NULL, NULL, NULL };
	}

	for (int c=0; c<6; c++) {
		if (!labels[c]) break;
		AddProperty(new NodeProperty(isinput ? NodeProperty::PROP_Output : NodeProperty::PROP_Input,
									 true,
									 names[c],
									 new DoubleValue(values ? values[c] : 0), 1, 
									 labels[c],
									 nullptr, 0,
									 isinput ? false : true)); 
	}

	Value *v = new AffineValue();
	if (!isinput) AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Affine"), v,1)); 

}

AffineNode::~AffineNode()
{
}

NodeBase *AffineNode::Duplicate()
{
	double vs[6];
	int isnum;
	if (atype != Expand) {
		for (int c=0; c<6; c++) {
			vs[c] = getNumberValue(properties.e[c]->GetData(), &isnum);
		}
	}

	AffineNode *newnode = new AffineNode(atype, atype == Expand ? nullptr : vs);
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int AffineNode::GetStatus()
{
	if (atype == Expand) {
		if (!dynamic_cast<Affine*>(properties.e[0]->GetData())) return -1;

	} else {

		int isnum;
		//double v[6];
		for (int c=0; c<6; c++) {
			getNumberValue(properties.e[c]->GetData(), &isnum);
			if (!isnum) return -1;
		}
		if (!properties.e[6]->data) return 1;
	}

	// maybe check this is invertible
//	if (m[0]*m[3]-m[1]*m[2] == 0) {
//		 //degenerate matrix!
//		if (!error_message) makestr(error_message, _("Bad matrix"));
//		return -1;
//	} else if (error_message) makestr(error_message, NULL);

	return NodeBase::GetStatus(); //default checks mod times
}

int AffineNode::Update()
{
	double vs[6];

	if (atype == Expand) {
		Affine *affine = dynamic_cast<Affine*>(properties.e[0]->GetData());
		if (!affine) return -1;
		for (int c=0; c<6; c++) {
			dynamic_cast<DoubleValue*>(properties.e[c+1]->GetData())->d = affine->m(c);
			properties.e[c+1]->Touch();
		}

	} else {
		int isnum;
		for (int c=0; c<6; c++) {
			vs[c] = getNumberValue(properties.e[c]->GetData(), &isnum);
			if (!isnum) return -1;
		}

		AffineValue *v = dynamic_cast<AffineValue*>(properties.e[6]->GetData());
		if (!v) {
			v = new AffineValue;
		} else v->inc_count();

		if (atype == XYSxyAxy) {
			//convert scale, angle, pos to vectors
			v->setBasics(vs[4], vs[5], vs[0], vs[1], vs[2]*M_PI/180, vs[3]*M_PI/180);

		} else v->m(vs);

		properties.e[6]->SetData(v, 1);
	}

	return NodeBase::Update();
}

Laxkit::anObject *newAffineNode(int p, Laxkit::anObject *ref)
{
	return new AffineNode(AffineNode::Abcdef, NULL);
}

Laxkit::anObject *newAffineNode2(int p, Laxkit::anObject *ref)
{
	return new AffineNode(AffineNode::XYSxyAxy, NULL);
}

Laxkit::anObject *newAffineExpandNode(int p, Laxkit::anObject *ref)
{
	return new AffineNode(AffineNode::Expand, NULL);
}


//------------ InvertNode

class InvertNode : public NodeBase
{
  public:
	InvertNode();
	virtual ~InvertNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new InvertNode(); }
};

InvertNode::InvertNode()
{
	type = newstr("Invert");
	makestr(Name, _("Invert"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, _("In"), _("Invert a number, vector, quaternion, or affine"),0,false)); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Out"), nullptr,0,false));
}


InvertNode::~InvertNode()
{
}

NodeBase *InvertNode::Duplicate()
{
	InvertNode *newnode = new InvertNode();
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int InvertNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return -1;
	return NodeBase::GetStatus(); //default checks mod times
}

int InvertNode::Update()
{
	Error(nullptr);

	Value *v = properties.e[0]->GetData();
	if (!v) return -1;

	double d;
	if (isNumberType(v, &d)) {
		if (d == 0) {
			Error(_("Cannot invert"));
			return -1;
		}
		DoubleValue *vv = dynamic_cast<DoubleValue*>(properties.e[1]->GetData());
		if (!vv) {
			vv = new DoubleValue();
			properties.e[1]->SetData(vv, 1);
		}
		vv->d = 1/d;

	} else if (v->type() == VALUE_Flatvector) {
		FlatvectorValue *vv = dynamic_cast<FlatvectorValue*>(properties.e[1]->GetData());
		if (!vv) {
			vv = new FlatvectorValue();
			properties.e[1]->SetData(vv, 1);
		}
		vv->v = -dynamic_cast<FlatvectorValue*>(v)->v;

	} else if (v->type() == VALUE_Spacevector) {
		SpacevectorValue *vv = dynamic_cast<SpacevectorValue*>(properties.e[1]->GetData());
		if (!vv) {
			vv = new SpacevectorValue();
			properties.e[1]->SetData(vv, 1);
		}
		vv->v = -dynamic_cast<SpacevectorValue*>(v)->v;

	} else if (v->type() == VALUE_Quaternion) {
		QuaternionValue *vv = dynamic_cast<QuaternionValue*>(properties.e[1]->GetData());
		if (!vv) {
			vv = new QuaternionValue();
			properties.e[1]->SetData(vv, 1);
		}
		vv->v = -dynamic_cast<QuaternionValue*>(v)->v.conjugate();

	//} else if (v->type() == AffineValue::TypeNumber()) {
	} else if (dynamic_cast<Affine*>(v)) {
		AffineValue *vv = dynamic_cast<AffineValue*>(properties.e[1]->GetData());
		if (!vv) {
			vv = new AffineValue();
			properties.e[1]->SetData(vv, 1);
		}
		vv->m(dynamic_cast<Affine*>(v)->m());
		vv->Invert();

	} else {
		Error(_("Cannot invert"));
		return -1;
	}

	properties.e[1]->Touch();
	return NodeBase::Update();
}


//------------ MathNode


enum MathNodeOps {
	OP_None = 0,

	 //1 argument:
	OP_FIRST_1_ARG,
	OP_AbsoluteValue,
	OP_Negative,
	OP_Sqrt,
	OP_Sgn,
	OP_Not,
	OP_Radians_To_Degrees,
	OP_Degrees_To_Radians,
	OP_Sin,
	OP_Cos,
	OP_Tan,
	OP_Asin,
	OP_Acos,
	OP_Atan,
	OP_Sinh,
	OP_Cosh,
	OP_Tanh,
	OP_Asinh,
	OP_Acosh,
	OP_Atanh,
	OP_Ceiling,
	OP_Floor,
	OP_Clamp_To_1, // [0..1]
	OP_Clamp_To_pm_1, // [-1..1]
	 //vector specific
	OP_VECTOR_1_ARG,
	OP_Norm,
	OP_Norm2,
	OP_Flip,
	OP_Normalize,
	OP_Transpose,
	OP_Angle,
	OP_Angle2,
	OP_LAST_1_ARG,


	 //2 arguments:
	OP_FIRST_2_ARG,
	OP_Add,
	OP_Subtract,
	OP_Multiply,
	OP_Divide,
	OP_Mod,
	OP_Power,
	OP_Greater_Than,
	OP_Greater_Than_Or_Equal,
	OP_Less_Than,
	OP_Less_Than_Or_Equal,
	OP_Equals,
	OP_Not_Equal,
	OP_Minimum,
	OP_Maximum,
	OP_Average,
	OP_Atan2,
	OP_RandomRange,    //seed, [0..max]
	//OP_RandomRangeInt, //seed, [0..max]
	OP_Clamp_Max,
	OP_Clamp_Min,
	OP_And,
	OP_Or,
	OP_Xor,
	OP_ShiftLeft,
	OP_ShiftRight,
	 //vector math:
	//OP_Vector_Add,     //use normal add
	//Op_Vector_Subtract,//use normal subtract
	OP_VECTOR_2_ARG,
	OP_Dot,
	OP_Cross,
	OP_Perpendicular,
	OP_Parallel,
	OP_Angle_Between,
	OP_Angle2_Between,
	OP_LAST_2_ARG,


	 //3 args:
	OP_FIRST_3_ARG,
	OP_Lerp,  // r*a+(1-r)*b, do with numbers or vectors, or sets thereof
	OP_Clamp, // in, [min..max]
	OP_LAST_3_ARG,

	 //other:
	OP_Linear_Map, // (5 args) in with [min,max] to [newmin, newmax] out

	OP_Swizzle_YXZ,
	OP_Swizzle_XZY,
	OP_Swizzle_YZX,
	OP_Swizzle_ZXY,
	OP_Swizzle_ZYX,
	OP_Swizzle_1234, //make custom node for this? 1 input, 1 output, drag and drop links

	OP_MAX
};

/*! Create and return a fresh instance of the def for a 1 arg MathNode2 op.
 */
ObjectDef *DefineMathNode1Def()
{
	ObjectDef *def = new ObjectDef("MathNode1Def", _("Math Node Def for 1 argument"), NULL,NULL,"enum", 0);

	 //1 argument
	def->pushEnumValue("AbsoluteValue"  ,_("AbsoluteValue"), _("AbsoluteValue"), OP_AbsoluteValue );
	def->pushEnumValue("Negative"       ,_("Negative"),      _("Negative"),      OP_Negative      );
	def->pushEnumValue("Not"            ,_("Not"),           _("Not"),           OP_Not           );
	def->pushEnumValue("Sqrt"           ,_("Square root"),   _("Square root"),   OP_Sqrt          );
	def->pushEnumValue("Sqn"            ,_("Sign"),          _("Sign: 1, -1 or 0"),OP_Sgn         );
	def->pushEnumValue("Clamp_To_1"     ,_("Clamp To 1"),    _("Clamp To 1"),    OP_Clamp_To_1    );
	def->pushEnumValue("Clamp_To_pm_1"  ,_("Clamp To [-1,1]"),    _("Clamp To [-1,1]"),    OP_Clamp_To_pm_1      );
	def->pushEnumValue("Rad_To_Deg"     ,_("Radians To Degrees"), _("Radians To Degrees"), OP_Radians_To_Degrees );
	def->pushEnumValue("Deg_To_Rad"     ,_("Degrees To Radians"), _("Degrees To Radians"), OP_Degrees_To_Radians );
	def->pushEnumValue("Sin"            ,_("Sin"),           _("Sin"),           OP_Sin           );
	def->pushEnumValue("Cos"            ,_("Cos"),           _("Cos"),           OP_Cos           );
	def->pushEnumValue("Tan"            ,_("Tan"),           _("Tan"),           OP_Tan           );
	def->pushEnumValue("Asin"           ,_("Asin"),          _("Asin"),          OP_Asin          );
	def->pushEnumValue("Acos"           ,_("Acos"),          _("Acos"),          OP_Acos          );
	def->pushEnumValue("Atan"           ,_("Atan"),          _("Atan"),          OP_Atan          );
	def->pushEnumValue("Sinh"           ,_("Sinh"),          _("Sinh"),          OP_Sinh          );
	def->pushEnumValue("Cosh"           ,_("Cosh"),          _("Cosh"),          OP_Cosh          );
	def->pushEnumValue("Tanh"           ,_("Tanh"),          _("Tanh"),          OP_Tanh          );
	def->pushEnumValue("Asinh"          ,_("Asinh"),         _("Asinh"),         OP_Asinh         );
	def->pushEnumValue("Acosh"          ,_("Acosh"),         _("Acosh"),         OP_Acosh         );
	def->pushEnumValue("Atanh"          ,_("Atanh"),         _("Atanh"),         OP_Atanh         );
	def->pushEnumValue("Ceiling"        ,_("Ceiling"),       _("Least integer greater than"), OP_Ceiling );
	def->pushEnumValue("Floor"          ,_("Floor"),         _("Greatest integer less than"), OP_Floor   );
	 //vector math, 1 arg
	def->pushEnumValue("Norm"           ,_("Norm"),          _("Length of vector"),OP_Norm        );
	def->pushEnumValue("Norm2"          ,_("Norm2"),         _("Square of length"),OP_Norm2       );
	def->pushEnumValue("Flip"           ,_("Flip"),          _("Flip"),          OP_Flip          );
	def->pushEnumValue("Normalize"      ,_("Normalize"),     _("Normalize"),     OP_Normalize     );
	def->pushEnumValue("Transpose"      ,_("Transpose"),     _("Transpose"),     OP_Transpose     );
	def->pushEnumValue("Angle"          ,_("Angle"),        _("Angle, -pi to pi. 2d only"),   OP_Angle);


	return def;
}

/*! Create and return a fresh instance of the def for a 2 arg MathNode2 op.
 */
ObjectDef *DefineMathNode2Def()
{
	ObjectDef *def = new ObjectDef("MathNode2Def", _("Math Node Def"), NULL,NULL,"enum", 0);

	 //2 arguments
	def->pushEnumValue("Add",        _("Add"),          _("Add"),         OP_Add         );
	def->pushEnumValue("Subtract",   _("Subtract"),     _("Subtract"),    OP_Subtract    );
	def->pushEnumValue("Multiply",   _("Multiply"),     _("Multiply"),    OP_Multiply    );
	def->pushEnumValue("Divide",     _("Divide"),       _("Divide"),      OP_Divide      );
	def->pushEnumValue("Mod",        _("Mod"),          _("Mod"),         OP_Mod         );
	def->pushEnumValue("Power",      _("Power"),        _("Power"),       OP_Power       );
	def->pushEnumValue("GreaterThan",_("Greater than"), _("Greater than"),OP_Greater_Than);
	def->pushEnumValue("GreaterEqual",_("Greater or equal"),_("Greater or equal"),OP_Greater_Than_Or_Equal);
	def->pushEnumValue("LessThan",   _("Less than"),    _("Less than"),   OP_Less_Than   );
	def->pushEnumValue("LessEqual",  _("Less or equal"),_("Less or equal"),OP_Less_Than_Or_Equal);
	def->pushEnumValue("Equals",     _("Equals"),       _("Equals"),      OP_Equals      );
	def->pushEnumValue("NotEqual",   _("Not Equal"),    _("Not Equal"),   OP_Not_Equal   );
	def->pushEnumValue("Minimum",    _("Minimum"),      _("Minimum"),     OP_Minimum     );
	def->pushEnumValue("Maximum",    _("Maximum"),      _("Maximum"),     OP_Maximum     );
	def->pushEnumValue("Average",    _("Average"),      _("Average"),     OP_Average     );
	def->pushEnumValue("Atan2",      _("Atan2"),        _("Arctangent 2"),OP_Atan2       );

	def->pushEnumValue("And"        ,_("And"       ),   _("And"       ),  OP_And         );
	def->pushEnumValue("Or"         ,_("Or"        ),   _("Or"        ),  OP_Or          );
	def->pushEnumValue("Xor"        ,_("Xor"       ),   _("Xor"       ),  OP_Xor         );
	def->pushEnumValue("ShiftLeft"  ,_("ShiftLeft" ),   _("ShiftLeft" ),  OP_ShiftLeft   );
	def->pushEnumValue("ShiftRight" ,_("ShiftRight"),   _("ShiftRight"),  OP_ShiftRight  );

	 //vector math, 2 args
	def->pushEnumValue("Dot"            ,_("Dot"),            _("Dot"),                     OP_Dot            );
	def->pushEnumValue("Cross"          ,_("Cross"),          _("Cross product"),           OP_Cross          );
	def->pushEnumValue("Perpendicular"  ,_("Perpendicular"),  _("Part of A perpendicular to B"),OP_Perpendicular);
	def->pushEnumValue("Parallel"       ,_("Parallel"),       _("Part of A parallel to B"), OP_Parallel);
	def->pushEnumValue("Angle_Between"  ,_("Angle Between"),  _("Angle Between, 0..pi"),    OP_Angle_Between  );
	def->pushEnumValue("Angle2_Between" ,_("Angle2 Between"), _("Angle2 Between, 0..2*pi"), OP_Angle2_Between );


	return def;
}


class MathNode1 : public NodeBase
{
  public:
	static SingletonKeeper mathnodekeeper; //the def for the op enum
	static ObjectDef *GetMathNode1Def() { return dynamic_cast<ObjectDef*>(mathnodekeeper.GetObject()); }

	int last_status;
	clock_t status_time;

	MathNode1(int op=0, double aa=0); //see MathNodeOps for op
	virtual ~MathNode1();
	virtual int UpdateThisOnly();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual const char *Label();
};

SingletonKeeper MathNode1::mathnodekeeper(DefineMathNode1Def(), true);

//------MathNode1

MathNode1::MathNode1(int op, double aa)
{
	type = newstr("Math1");

	last_status = 1;
	status_time = 0;

	ObjectDef *enumdef = GetMathNode1Def();
	enumdef->inc_count();

	EnumValue *e = new EnumValue(enumdef, 0);
	e->SetFromId(op);
	enumdef->dec_count();
	//const char *Nm = e->EnumLabel();
	//if (Nm) makestr(Name, Nm);

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "Op", e, 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "A", new DoubleValue(aa), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Result", NULL,0, _("Result"), NULL, 0, false));

	last_status = Update();
	status_time = MostRecentIn(NULL);
}

MathNode1::~MathNode1()
{
}

const char *MathNode1::Label()
{
	if (Name) return Name;

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	const char *Nm = ev->EnumLabel();

	return Nm;
}

NodeBase *MathNode1::Duplicate()
{
	int operation = -1;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *edef = ev->GetObjectDef();
	edef->getEnumInfo(ev->value, NULL,NULL,NULL, &operation);

	MathNode1 *newnode = new MathNode1(operation);

	Value *a = properties.e[1]->GetData();
	if (isNumberType(a,nullptr) || a->type() == VALUE_Flatvector || a->type() == VALUE_Spacevector || a->type() == VALUE_Quaternion) {
		a = a->duplicate();
		newnode->properties.e[1]->SetData(a, 1);
	}
	newnode->DuplicateBase(this);
	return newnode;
}

/*! Default is to return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int MathNode1::GetStatus()
{
	if (!properties.e[2]->data) return 1;
	int status = NodeBase::GetStatus(); //checks mod times
	if (status == 1) return 1; //just simple update

	clock_t proptime = MostRecentIn(NULL);
	if (proptime > status_time) {
		last_status = UpdateThisOnly();
		status_time = proptime;
	}
	return last_status;
}

int MathNode1::Update()
{
	int status = UpdateThisOnly();
	if (status == 0) {
		Touch();
		// PropagateUpdate();
		return NodeBase::Update();
	}
	return status;
}

int MathNode1::UpdateThisOnly()
{
	ClearError();
	
	int max = 0;
	bool dosets = false;
	SetValue *setin = nullptr;;
	Value *valuea = properties.e[1]->GetData();

	if (DetermineSetIns(1, &valuea, &setin, max, dosets) == -1) return -1;

	Value *out = properties.e[2]->GetData();
	SetValue *outset = nullptr;
	if (dosets) {
		if (out && out->type() == VALUE_Set) {
			outset = dynamic_cast<SetValue*>(out);
		} else {
			outset = new SetValue();
			properties.e[2]->SetData(outset,1);
		}
		out = nullptr;
	}

	//clamp outset to max
	if (outset) while (outset->n() > max) outset->Remove(outset->n()-1);

	//get operation
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();
	const char *nm = NULL, *Nm = NULL;
	int operation = OP_None;
	def->getEnumInfo(ev->value, &nm, &Nm, NULL, &operation);
	
	for (int c=0; c<max; c++) {
		double result=0;
		if (dosets) {
			if (c < setin->n()) valuea = setin->e(c); 
			if (c < outset->n()) out = outset->e(c); else out = nullptr;
		}
	

		int aisnum=0; //number of elements of result
		double a = getNumberValue(valuea, &aisnum);
		if (aisnum) aisnum = 1; //else ints and booleans return 2 and 3

		if (!aisnum && dynamic_cast<FlatvectorValue *>(valuea)) aisnum = 2;
		if (!aisnum && dynamic_cast<SpacevectorValue*>(valuea)) aisnum = 3;
		if (!aisnum && dynamic_cast<QuaternionValue *>(valuea)) aisnum = 4;

		if (!aisnum) {
			Error(_("Operation can't use that argument"));
			return -1;
		}


		if (aisnum == 1) { //scalar
			if        (operation == OP_AbsoluteValue  || operation == OP_Norm) { result = fabs(a);
			} else if (operation == OP_Norm2            ) { result = a*a;
			} else if (operation == OP_Negative || operation == OP_Flip) { result = -a;
			} else if (operation == OP_Sqrt             ) {
				if (a>=0) result = sqrt(a);
				else {
					Error(_("Sqrt needs nonnegative number"));
					return -1;
				}
			} else if (operation == OP_Sgn              )  { result = a > 0 ? 1 : a < 0 ? -1 : 0;
			} else if (operation == OP_Not              )  { result = !(int)a;
			} else if (operation == OP_Radians_To_Degrees) { result = a *180/M_PI;
			} else if (operation == OP_Degrees_To_Radians) { result = a * M_PI/180;
			} else if (operation == OP_Sin              )  { result = sin(a);
			} else if (operation == OP_Cos              )  { result = cos(a);
			} else if (operation == OP_Tan              )  { result = tan(a);
			} else if (operation == OP_Asin             )  {
				if (a <= 1 && a >= -1) result = asin(a);
				else {
					Error(_("Argument needs to be in range [-1, 1]"));
					return -1;
				}
			} else if (operation == OP_Acos             )  {
				if (a <= 1 && a >= -1) result = acos(a);
				else {
					Error(_("Argument needs to be in range [-1, 1]"));
					return -1;
				}
			} else if (operation == OP_Atan             )  { result = atan(a);
			} else if (operation == OP_Sinh             )  { result = sinh(a);
			} else if (operation == OP_Cosh             )  { result = cosh(a);
			} else if (operation == OP_Tanh             )  { result = tanh(a);
			} else if (operation == OP_Asinh            )  { result = asinh(a);
			} else if (operation == OP_Acosh            )  {
				if (a >= 1) result = acosh(a);
				else {
					Error(_("Argument needs to be >= 1"));
					return -1;
				}
			} else if (operation == OP_Atanh            )  {
				if (a > -1 && a < 1) result = atanh(a);
				else {
					Error(_("Argument needs to be -1 > a > 1"));
					return -1;
				}
			} else if (operation == OP_Ceiling          )  { result = ceil(a);
			} else if (operation == OP_Floor            )  { result = floor(a);
			} else if (operation == OP_Clamp_To_1       )  { result = a > 1 ? 1 : a < 0 ? 0 : a;
			} else if (operation == OP_Clamp_To_pm_1    )  { result = a > 1 ? 1 : a < -1 ? -1 : a;
			} else {
				Error(_("Operation can't use that argument"));
				return -1;
			}

			if (!dynamic_cast<DoubleValue*>(out)) {
				out = new DoubleValue(result);
				if (dosets) {
					if (c >= outset->n()) outset->Push(out, 1);
					else outset->Set(c, out, 1);
				} else properties.e[2]->SetData(out, 1);
			} else dynamic_cast<DoubleValue*>(out)->d = result;

		} else { //else involves vectors...

			int resulttype = VALUE_None;

			double va[4], rv[4];
			for (int c=0; c<4; c++) { va[c] = rv[c] = 0; }

			if (aisnum==2) {
				dynamic_cast<FlatvectorValue*>(valuea)->v.get(va);
				resulttype = VALUE_Flatvector;
			} else if (aisnum==3) {
				dynamic_cast<SpacevectorValue*>(valuea)->v.get(va);
				resulttype = VALUE_Spacevector;
			} else if (aisnum==4) {
				dynamic_cast<SpacevectorValue*>(valuea)->v.get(va);
				resulttype = VALUE_Quaternion;
			}
			

			if (operation == OP_AbsoluteValue || operation == OP_Norm) {
				for (int c=0; c<aisnum; c++) result += va[c]*va[c];
				result = sqrt(result);
				resulttype = VALUE_Real;

			} else if (operation == OP_Norm2         ) {
				for (int c=0; c<aisnum; c++) result += va[c]*va[c];
				resulttype = VALUE_Real;

			} else if (operation == OP_Negative      ) { for (int c=0; c<aisnum; c++) rv[c] = -va[c];
			} else if (operation == OP_Clamp_To_1    ) { for (int c=0; c<aisnum; c++) rv[c] = va[c] > 1 ? 1 : va[c] <  0 ?  0 : va[c];
			} else if (operation == OP_Clamp_To_pm_1 ) { for (int c=0; c<aisnum; c++) rv[c] = va[c] > 1 ? 1 : va[c] < -1 ? -1 : va[c];
			} else if (operation == OP_Flip          ) { for (int c=0; c<aisnum; c++) rv[c] = -va[c];
			} else if (operation == OP_Normalize     ) {
				result = 0;
				for (int c=0; c<aisnum; c++) result += va[c]*va[c];
				result = sqrt(result);
				if (result == 0) {
					Error(_("Can't normalize a null vector"));
					return -1;
				}
				for (int c=0; c<aisnum; c++) rv[c] = va[c]/result;

			} else if (operation == OP_Transpose) {
				result = 0;
				if (aisnum != 2) {
					Error(_("Transpose only works on Vector2"));
					return -1;
				}
				//double vv = rv[0];
				rv[0] = -va[1];
				rv[1] = va[0];

			} else if (operation == OP_Angle) {
				if (aisnum != 2) {
					Error(_("Only for Vector2"));
					return -1;
				}
				resulttype = VALUE_Real;
				result = atan2(va[1], va[0]);

			} else resulttype = VALUE_None;

			if (resulttype == VALUE_None) {
				Error(_("Operation can't use that argument"));
				return -1;
			}

			if (resulttype == VALUE_Real) {
				if (!dynamic_cast<DoubleValue*>(out)) {
					out = new DoubleValue(result);
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[2]->SetData(out, 1);
				} else dynamic_cast<DoubleValue*>(out)->d = result;

			} else if (resulttype == VALUE_Flatvector) {
				if (!out || out->type() != VALUE_Flatvector) {
					out = new FlatvectorValue(flatvector(rv));
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[2]->SetData(out, 1);
				} else dynamic_cast<FlatvectorValue*>(out)->v.set(rv);
				
			} else if (resulttype == VALUE_Spacevector) {
				if (!out || out->type() != VALUE_Spacevector) {
					out = new SpacevectorValue(spacevector(rv));
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[2]->SetData(out, 1);
				} else dynamic_cast<SpacevectorValue*>(out)->v.set(rv);
				
			} else if (resulttype == VALUE_Quaternion) {
				if (!out || out->type() != VALUE_Quaternion) {
					out = new QuaternionValue(Quaternion(rv));
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[2]->SetData(out, 1);
				} else dynamic_cast<QuaternionValue*>(out)->v.set(rv);
			}
		}
	}

	properties.e[2]->Touch();
	return 0;
}

Laxkit::anObject *newMathNode1(int p, Laxkit::anObject *ref)
{
	return new MathNode1();
}




//------MathNode2

class MathNode2 : public NodeBase
{
  public:
	static SingletonKeeper mathnodekeeper; //the def for the op enum
	static ObjectDef *GetMathNode2Def() { return dynamic_cast<ObjectDef*>(mathnodekeeper.GetObject()); }

	int last_status;
	clock_t status_time;

	// int numargs;
	// double a,b,result;

	MathNode2(int op=0, double aa=0, double bb=0); //see 2 arg MathNodeOps for op
	virtual ~MathNode2();
	virtual int UpdateThisOnly();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual const char *Label();
};

SingletonKeeper MathNode2::mathnodekeeper(DefineMathNode2Def(), true);

Laxkit::anObject *newMathNode2(int p, Laxkit::anObject *ref)
{
	return new MathNode2();
}

MathNode2::MathNode2(int op, double aa, double bb)
{
	type = newstr("Math2");
	// Name = newstr(_("Math 2"));

	last_status = 1;
	status_time = 0;

	// a=aa;
	// b=bb;
	// numargs = 2;

	ObjectDef *enumdef = GetMathNode2Def();
	enumdef->inc_count();

	EnumValue *e = new EnumValue(enumdef, 0);
	e->SetFromId(op);
	enumdef->dec_count();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "Op", e, 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "A", new DoubleValue(aa), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "B", new DoubleValue(bb), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Result", NULL,0, _("Result"), NULL, 0, false));

	last_status = Update();
	status_time = MostRecentIn(NULL);
}

MathNode2::~MathNode2()
{
//	if (mathnodedef) {
//		if (mathnodedef->dec_count()<=0) mathnodedef=NULL;
//	}
}

const char *MathNode2::Label()
{
	if (!isblank(Name)) return Name;

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	const char *Nm = ev->EnumLabel();

	return Nm;
}

NodeBase *MathNode2::Duplicate()
{
	int operation = -1;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *edef = ev->GetObjectDef();
	edef->getEnumInfo(ev->value, NULL,NULL,NULL, &operation);

	MathNode2 *newnode = new MathNode2(operation);
	Value *a = properties.e[1]->GetData();
	if (isNumberType(a,nullptr) || a->type() == VALUE_Flatvector || a->type() == VALUE_Spacevector || a->type() == VALUE_Quaternion) {
		a = a->duplicate();
		newnode->properties.e[1]->SetData(a, 1);
	}
	a = properties.e[2]->GetData();
	if (isNumberType(a,nullptr) || a->type() == VALUE_Flatvector || a->type() == VALUE_Spacevector || a->type() == VALUE_Quaternion) {
		a = a->duplicate();
		newnode->properties.e[2]->SetData(a, 1);
	}
	newnode->DuplicateBase(this);
	return newnode;
}

// OPARG_vn
// OPARG_nv
// OPARG_nn
// OPARG_vv
// OPARG_ss
// OPARG_vs sv ns sn

/*! Default is to return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int MathNode2::GetStatus()
{
	if (!properties.e[3]->data) return 1;
	int status = NodeBase::GetStatus(); //checks mod times
	if (status == 1) return 1; //just simple update

	clock_t proptime = MostRecentIn(NULL);
	if (proptime > status_time) {
		last_status = UpdateThisOnly();
		status_time = proptime;
	}
	return last_status;
}

int MathNode2::Update()
{
	last_status = UpdateThisOnly();
	status_time = times(nullptr);
	if (last_status == 0) {
		modtime = times(NULL);
		return last_status;
	}
	return NodeBase::Update();
}

int MathNode2::UpdateThisOnly()
{
	Error(nullptr);

	int max = 0;
	bool dosets = false;
	Value *valuea = properties.e[1]->GetData();
	Value *valueb = properties.e[2]->GetData();

	Value *ins[2];
	ins[0] = valuea;
	ins[1] = valueb;
	SetValue *setins[2];
	setins[0] = setins[1] = nullptr;

	if (DetermineSetIns(2, ins, setins, max, dosets) == -1) return -1;

	Value *out = properties.e[3]->GetData();
	SetValue *outset = nullptr;
	if (dosets) {
		if (out && out->type() == VALUE_Set) {
			outset = dynamic_cast<SetValue*>(out);
		} else {
			outset = new SetValue();
			properties.e[3]->SetData(outset,1);
		}
		out = nullptr;
	}

	//clamp outset to max
	if (outset) while (outset->n() > max) outset->Remove(outset->n()-1);


	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();
	const char *nm = NULL, *Nm = NULL;
	int operation = OP_None;
	def->getEnumInfo(ev->value, &nm, &Nm, NULL, &operation);

	double a,b,result;

	for (int c=0; c<max; c++) {
		if (dosets) {
			if (c < outset->n()) out = outset->e(c);
			else out = nullptr;
			if (setins[0] && c < setins[0]->n()) valuea = setins[0]->e(c);
			if (setins[1] && c < setins[1]->n()) valueb = setins[1]->e(c);
		}

		int aisnum=0, bisnum=0;
		a = getNumberValue(valuea, &aisnum);
		b = getNumberValue(valueb, &bisnum);
		if (aisnum) aisnum = 1; //else ints and booleans return 2 and 3
		if (bisnum) bisnum = 1;

		if (!aisnum && dynamic_cast<FlatvectorValue *>(valuea)) aisnum = 2;
		if (!aisnum && dynamic_cast<SpacevectorValue*>(valuea)) aisnum = 3;
		if (!aisnum && dynamic_cast<QuaternionValue *>(valuea)) aisnum = 4;
		if (!bisnum && dynamic_cast<FlatvectorValue *>(valueb)) bisnum = 2;
		if (!bisnum && dynamic_cast<SpacevectorValue*>(valueb)) bisnum = 3;
		if (!bisnum && dynamic_cast<QuaternionValue *>(valueb)) bisnum = 4;

		if (aisnum == 1 && bisnum == 1) {
			if      (operation==OP_Add)      result = a+b;
			else if (operation==OP_Subtract) result = a-b;
			else if (operation==OP_Multiply) result = a*b;
			else if (operation==OP_Divide) {
				if (b!=0) result = a/b;
				else {
					result=0;
					Error(_("Can't divide by 0"));
					return -1;
				}

			} else if (operation==OP_Mod) {
				if (b!=0) result = a-b*int(a/b);
				else {
					result=0;
					Error(_("Can't divide by 0"));
					return -1;
				}
			} else if (operation==OP_Power) {
				if (a==0 || (a<0 && fabs(b)-fabs(int(b))<1e-10)) {
					 //0 to a power fails, as does negative numbers raised to non-integer powers
					result=0;
					Error(_("Power must be positive"));
					return -1;
				}
				result = pow(a,b);

			} else if (operation==OP_Greater_Than_Or_Equal) { result = (a>=b);
			} else if (operation==OP_Greater_Than)     { result = (a>b);
			} else if (operation==OP_Less_Than)        { result = (a<b);
			} else if (operation==OP_Less_Than_Or_Equal) { result = (a<=b);
			} else if (operation==OP_Equals)           { result = (a==b);
			} else if (operation==OP_Not_Equal)        { result = (a!=b);
			} else if (operation==OP_Minimum)          { result = (a<b ? a : b);
			} else if (operation==OP_Maximum)          { result = (a>b ? a : b);
			} else if (operation==OP_Average)          { result = (a+b)/2;
			} else if (operation==OP_Atan2)            { result = atan2(a,b);

			} else if (operation==OP_And       )       { result = (int(a) & int(b));
			} else if (operation==OP_Or        )       { result = (int(a) | int(b));
			} else if (operation==OP_Xor       )       { result = (int(a) ^ int(b));
			} else if (operation==OP_ShiftLeft )       { result = (int(a) << int(b));
			} else if (operation==OP_ShiftRight)       { result = (int(a) >> int(b));

			} else if (operation==OP_RandomRange)      {
				srandom(a);
				result = b * (double)random()/RAND_MAX;
			} else {
				Error(_("Operation can't use those arguments"));
				return -1;
			}

			if (!dynamic_cast<DoubleValue*>(out)) {
				out = new DoubleValue(result);
				if (dosets) {
					if (c >= outset->n()) outset->Push(out, 1);
					else outset->Set(c, out, 1);
				} else properties.e[3]->SetData(out, 1);
			} else dynamic_cast<DoubleValue*>(out)->d = result;

		} else {
			//else involves vectors...
			int resulttype = VALUE_None;

			double va[4], vb[4], rv[4], *vv;
			for (int c=0; c<4; c++) { va[c] = vb[c] = rv[c] = 0; }

			if (aisnum==2) {
				dynamic_cast<FlatvectorValue*>(valuea)->v.get(va);
				resulttype = VALUE_Flatvector;
			} else if (aisnum==3) {
				dynamic_cast<SpacevectorValue*>(valuea)->v.get(va);
				resulttype = VALUE_Spacevector;
			} else if (aisnum==4) {
				dynamic_cast<SpacevectorValue*>(valuea)->v.get(va);
				resulttype = VALUE_Quaternion;
			}
			
			if (bisnum==2) {
				dynamic_cast<FlatvectorValue*>(valueb)->v.get(vb);
				resulttype = VALUE_Flatvector;
			} else if (bisnum==3) {
				dynamic_cast<SpacevectorValue*>(valueb)->v.get(vb);
				resulttype = VALUE_Spacevector;
			} else if (bisnum==4) {
				dynamic_cast<SpacevectorValue*>(valueb)->v.get(vb);
				resulttype = VALUE_Quaternion;
			}

			if ((aisnum == 1 && bisnum > 1) || (aisnum > 1 && bisnum == 1)) {
				 // r,v  v,r:  only multiply and maybe divide or mod
				if (aisnum == 1) vv = vb; else vv = va;

				if (operation==OP_Multiply) {
					if (aisnum == 1) { for (int c=0; c<4; c++) rv[c] = vv[c] * a; }
					else { for (int c=0; c<4; c++) rv[c] = vv[c] * b; }

				} else if (operation==OP_Divide || operation==OP_Mod) {
					if (bisnum != 1) {
						Error(_("Bad parameters"));
						return -1;
					}
					if (b == 0) {
						Error(_("Can't divide by 0"));
						return -1;
					}
					if (operation==OP_Mod) {
						for (int c=0; c<4; c++) rv[c] = vv[c] - b*int(vv[c]/b);

					} else for (int c=0; c<4; c++) rv[c] = vv[c] / b;

				} else {
					Error(_("Operation can't use those arguments"));
					return -1;
				}

			} else if ( (aisnum == 3 && bisnum == 3)
					 || (aisnum == 4 && bisnum == 4)
					 || (aisnum == 2 && bisnum == 2)) {

				resulttype = (aisnum==2 ? VALUE_Flatvector : (aisnum==3 ? VALUE_Spacevector : VALUE_Quaternion));

				if      (operation==OP_Add)        { for (int c=0; c<4; c++) rv[c] = va[c] + vb[c];
				} else if (operation==OP_Subtract) { for (int c=0; c<4; c++) rv[c] = va[c] - vb[c];
				} else if (operation==OP_Equals)   {
					resulttype = VALUE_Real;
					result = 1;
					for (int c=0; c<4; c++) if (va[c] != vb[c]) result = 0;

				} else if (operation==OP_Not_Equal) {
					resulttype = VALUE_Real;
					result = 0;
					for (int c=0; c<4; c++) if (va[c] != vb[c]) result = 1;

				} else if (operation==OP_Average) {
					for (int c=0; c<4; c++) rv[c] = (va[c] + vb[c])/2;

				} else if (operation==OP_Dot) {
					resulttype = VALUE_Real;
					result = 0;
					for (int c=0; c<4; c++) result += va[c]*vb[c];

				} else if (operation==OP_Cross         ) {
					if (aisnum == 4) resulttype = VALUE_None;
					else {
						resulttype = VALUE_Spacevector;
						spacevector aa(va), bb(vb);
						aa = aa/bb;
						aa.get(rv);
					}
				} else if (operation==OP_Perpendicular ) {
					double norm2 = vb[0]*vb[0]+vb[1]*vb[1]+vb[2]*vb[2]+vb[3]*vb[3];
					if (norm2 == 0) {
						Error(_("Vector b can't be 0"));
						return -1;
					}
					double dot = va[0]*vb[0]+va[1]*vb[1]+va[2]*vb[2]+va[3]*vb[3];
					dot /= norm2;
					for (int c=0; c<4; c++) rv[c] = va[c] - dot*vb[c];

				} else if (operation==OP_Parallel      ) {
					double norm2 = vb[0]*vb[0]+vb[1]*vb[1]+vb[2]*vb[2]+vb[3]*vb[3];
					if (norm2 == 0) {
						Error(_("Vector b can't be 0"));
						return -1;
					}
					double dot = va[0]*vb[0]+va[1]*vb[1]+va[2]*vb[2]+va[3]*vb[3];
					dot /= norm2;
					for (int c=0; c<4; c++) rv[c] = dot*vb[c];

				} else if (operation==OP_Angle_Between ) {
					if (aisnum == 4) resulttype = VALUE_None;
					else {
						if (aisnum == 2) {
							result = angle(flatvector(va), flatvector(vb));
						} else {
							result = angle(spacevector(va), spacevector(vb));
						}
						resulttype = VALUE_Real;
					}
				} else if (operation==OP_Angle2_Between) {
					if (aisnum == 4) resulttype = VALUE_None;
					else {
						if (aisnum == 2) {
							result = angle_full(flatvector(va), flatvector(vb));
						} else {
							result = angle(spacevector(va), spacevector(vb));
						}
						resulttype = VALUE_Real;
					}

				} else {
					resulttype = VALUE_None;
				}
			}

			if (resulttype == VALUE_None) {
				Error(_("Operation can't use those arguments"));
				return -1;
			}

			if (resulttype == VALUE_Real) {
				if (!dynamic_cast<DoubleValue*>(out)) {
					out = new DoubleValue(result);
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[3]->SetData(out, 1);
				} else dynamic_cast<DoubleValue*>(out)->d = result;

			} else if (resulttype == VALUE_Flatvector) {
				if (!out || out->type() != VALUE_Flatvector) {
					out = new FlatvectorValue(flatvector(rv));
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[3]->SetData(out, 1);
				} else dynamic_cast<FlatvectorValue*>(out)->v.set(rv);
				
			} else if (resulttype == VALUE_Spacevector) {
				if (!out || out->type() != VALUE_Spacevector) {
					out = new SpacevectorValue(spacevector(rv));
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[3]->SetData(out, 1);
				} else dynamic_cast<SpacevectorValue*>(out)->v.set(rv);
				
			} else if (resulttype == VALUE_Quaternion) {
				if (!out || out->type() != VALUE_Quaternion) {
					out = new QuaternionValue(Quaternion(rv));
					if (dosets) {
						if (c >= outset->n()) outset->Push(out, 1);
						else outset->Set(c, out, 1);
					} else properties.e[3]->SetData(out, 1);
				} else dynamic_cast<QuaternionValue*>(out)->v.set(rv);

			} else {
				Error(_("Operation can't use those arguments"));
				return -1;
			}
		} //ops on vectors
	}

	properties.e[3]->Touch();
	return 0;
}


//------------------------------ ConvertNumberNode --------------------------------------------

/*! \class ConvertNumberNode
 * Number units conversion
 */
class ConvertNumberNode : public NodeBase
{
  public:
  	static SingletonKeeper unitsMenu;
  	ObjectDef *GetConvertDef();

	ConvertNumberNode();
	virtual ~ConvertNumberNode();
	virtual int GetStatus();
	virtual int Update();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ConvertNumberNode(); }
};

SingletonKeeper ConvertNumberNode::unitsMenu;

ObjectDef *ConvertNumberNode::GetConvertDef()
{
	ObjectDef *def = dynamic_cast<ObjectDef*>(unitsMenu.GetObject());
	if (def) return def;

	def = new ObjectDef("ConvertDef", _("Number Conversions"), NULL,NULL,"enum", 0);
	unitsMenu.SetObject(def, 1);

	UnitManager *units = GetUnitManager();

	//units should be: singular plural abbreviation  localized_Name localized_Description
	char *shortname, *singular, *plural;
	const char *label;
	double scale;
	int id;

	for (int c=0; c<units->NumberOfUnits(); c++) {
		units->UnitInfoIndex(c, &id, &scale, &shortname, &singular, &plural, &label);
		def->pushEnumValue(shortname, label, nullptr, id);
	}

	return def;
}

ConvertNumberNode::ConvertNumberNode()
{
	makestr(Name, _("Convert number"));
	makestr(type, "ConvertNumber");

	ObjectDef *unitsdef = GetConvertDef();
	UnitManager *units = GetUnitManager();
	int u = units->DefaultUnits();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "In",    new DoubleValue(0),1,         _("In"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "From", new EnumValue(unitsdef, u),1, _("From"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "To",   new EnumValue(unitsdef, u),1, _("To"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out",  new DoubleValue(0),1, _("Out"), NULL, 0, false));
}

ConvertNumberNode::~ConvertNumberNode()
{
}

NodeBase *ConvertNumberNode::Duplicate()
{
	ConvertNumberNode *newnode = new ConvertNumberNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int ConvertNumberNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), NULL)) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ConvertNumberNode::Update()
{
	int isnum = 0;
	double in = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (!isnum) {
		return -1;
	}

	EnumValue *from = dynamic_cast<EnumValue*>(properties.e[1]->GetData());
	EnumValue *to   = dynamic_cast<EnumValue*>(properties.e[2]->GetData());

	UnitManager *units = GetUnitManager();
	int err = 0;
	double out = units->Convert(in, from->EnumId(), to->EnumId(), &err);
	DoubleValue *vv = dynamic_cast<DoubleValue*>(properties.e[3]->GetData());
	vv->d = out;
	properties.e[3]->Touch();
	if (err != 0) return -1;

	return NodeBase::Update();
}


//----------------------------------- ExpressionNode ------------------------------

class ExpressionNode : public NodeBase
{
	clock_t last_eval;

  public:
	ExpressionNode(const char *expr);
	virtual ~ExpressionNode();

	virtual int Set(const char *expr, bool force_remap);
	virtual const char *Label();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ExpressionNode(nullptr); }
};

ExpressionNode::ExpressionNode(const char *expr)
{
	makestr(type, "Math/Expression");
	// makestr(Name, _("Expression"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Expr", new StringValue(""),1, nullptr,_("Expression")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", nullptr,1, _("Out"), NULL,0, false));

	last_eval = 0;
	Set(expr, true);

	Update();
}

ExpressionNode::~ExpressionNode()
{
}

const char *ExpressionNode::Label()
{
	if (!isblank(Name)) return Name;

	StringValue *v = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!v || isblank(v->str)) return _("Expression");
	return v->str;
}

/*! Intercept to make sure property labels are correct.
 */
void ExpressionNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att, flag, context);

	//make sure out it at the end
	int propi = -1;
	FindProperty("Out", &propi);
	if (propi != properties.n-1) { //sometimes out gets lost on read in
		properties.slide(propi, properties.n-1);
	}

	//make sure prop labels are "thing", not "p_thing" like the names
	for (int c=1; c<properties.n-1; c++) {
		properties.e[c]->Label(properties.e[c]->name+2);
	}
}

/*! Set up properties to show unknow names in expression.
 * Installs expression as LOCAL data of property 0, no matter what is connected to it.
 * Returns 0 for ok, nonzero for some kind of error.
 */
int ExpressionNode::Set(const char *expr, bool force_remap)
{
	if (isblank(expr)) {
		return 1;
	}

	StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->data);
	if (!s) {
		s = new StringValue(expr);
		properties.e[0]->SetData(s, 1);
	} else {
		if (!force_remap && expr && s->str && !strcmp(expr, s->str)) return 0;
		s->Set(expr);
		properties.e[0]->Touch();
	}

	int propi = -1;
	FindProperty("Out", &propi);
	if (propi != properties.n-1) { //sometimes out gets lost on read in
		properties.slide(propi, properties.n-1);
	}


	// determine unknow names and use those as properties
	PtrStack<char> names(LISTS_DELETE_Array);
	ErrorLog log;
	int status = laidout->calculator->FindUnknownNames(expr,-1, nullptr, nullptr, &log, names);
	if (status != 0) {
		char *er = log.FullMessageStr();
		if (er) {
			makestr(error_message, er);
			delete[] er;
		}
		return -1;
	}

	//modify or add new properties
	Utf8String str;
	for (int c=0; c<names.n; c++) {
		str.Sprintf("p_%s", names.e[c]);

		NodeProperty *prop = FindProperty(str.c_str(), &propi);
		if (prop) {
			if (propi != 1+c) properties.slide(propi, 1+c);
		} else {
			if (c+1 == properties.n-1) {
				//insert new prop
				NodeProperty *prop = new NodeProperty(NodeProperty::PROP_Input,  true, str.c_str(), new DoubleValue(0),1, names.e[c], nullptr, 0, true);
				AddProperty(prop, c+1);

			} else {
				if (!str.Equals(properties.e[c+1]->name)) {
					makestr(properties.e[c+1]->name,  str.c_str());
					makestr(properties.e[c+1]->label, names.e[c]);
				}
			}
		}
	}

	// remove extraneous properties
	for (int c = properties.n-2; c > names.n; c--) {
		if (!properties.e[c]->IsConnected())
			RemoveProperty(properties.e[c]);
	}

	Wrap();
	return 0;
}

int ExpressionNode::UpdatePreview()
{
	Previewable *p = dynamic_cast<Previewable*>(properties.e[properties.n-1]->GetData());
	if (!p) {
		if (total_preview) { total_preview->dec_count(); total_preview = nullptr; }
		return 0;
	}

	LaxImage *img = p->GetPreview();
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	}
	return 1;
}

NodeBase *ExpressionNode::Duplicate()
{
	const char *expr = nullptr;
	StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (s) expr = s->str;
	ExpressionNode *newnode = new ExpressionNode(expr);

	for (int c=1; c<properties.n-1 && c < newnode->properties.n-1; c++) {
		Value *v = properties.e[c]->GetData();
		if (v) newnode->properties.e[c]->SetData(v->duplicate(), 1);
	}

	return newnode;
}

//0 ok, -1 bad ins, 1 just needs updating
int ExpressionNode::GetStatus()
{
	StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!s || isblank(s->str)) return -1;

	clock_t ins = MostRecentIn(nullptr);
	if (ins > last_eval) Error(nullptr);
	// if (ErrorMessage()) return 1;

	return NodeBase::GetStatus();
}

//0 ok, -1 bad ins, 1 just needs updating
int ExpressionNode::Update()
{
	Error(nullptr);
	if (GetStatus() == -1) return -1;

	StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	// if (!s) return -1; <- checked in GetStatus()
	const char *expression = s->str;

	if (modtime == 0 || modtime < properties.e[0]->modtime) {
		if (Set(expression, modtime == 0) || error_message) return -1;
	}

	ValueHash params;
	for (int c=1; c<properties.n-1; c++) {
		Value *v = properties.e[c]->GetData();
		if (!v) return -1;
		params.push(properties.e[c]->name+2, v); //prop names are p_propertyname
	}
	int status;
	ErrorLog log;
	Value *ret = nullptr;

	last_eval = times(nullptr);
	status = laidout->calculator->EvaluateWithParams(expression,-1, nullptr, &params, &ret, &log);
	if (status != 0 || !ret) {
		char *er = log.FullMessageStr();
		if (er) {
			makestr(error_message, er);
			delete[] er;
		}
		if (ret) ret->dec_count();
		return -1;
	}

	properties.e[properties.n-1]->SetData(ret, 1);
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}


//------------------------------ MathConstantsNode --------------------------------------------

class MathConstantNode : public NodeBase
{
	static SingletonKeeper defkeeper; //the def for the op enum

  public:
	MathConstantNode(int which);
	virtual ~MathConstantNode();
	ObjectDef *GetDef() { return dynamic_cast<ObjectDef*>(defkeeper.GetObject()); }
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus() { return 0; }
	virtual const char *Label();
};

enum MathConstant {
	CONST_Pi,
	CONST_Pi_Over_2,
	CONST_2_Pi,
	CONST_E,
	CONST_Tau,
	CONST_1_Over_Tau
};

ObjectDef *DefineMathConstants()
{
	ObjectDef *def = new ObjectDef("MathConstants", _("Various constants"), NULL,NULL,"enum", 0);

	 //1 argument
	def->pushEnumValue("pi"   ,_("pi"),   _("3.1415"), CONST_Pi        );
	def->pushEnumValue("pi_2" ,_("pi/2"), _("1.5708"), CONST_Pi_Over_2 );
	def->pushEnumValue("pi2"  ,_("2*pi"), _("6.2832"), CONST_2_Pi      );
	def->pushEnumValue("e"    ,_("e"),    _("2.7182"), CONST_E         );
	def->pushEnumValue("tau"  ,_("tau"),  _("1.6180"), CONST_Tau       );
	def->pushEnumValue("taui" ,_("1/tau"),_("0.6180"), CONST_1_Over_Tau);

	return def;
}

SingletonKeeper MathConstantNode::defkeeper(DefineMathConstants(), true);

MathConstantNode::MathConstantNode(int which)
{
	//makestr(Name, _("Math Constant"));
	makestr(type, "Math/Constant");

	ObjectDef *enumdef = GetDef();
	EnumValue *e = new EnumValue(enumdef, 0);
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "which", e,1, "")); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", new DoubleValue(0),1, _("Out"),nullptr,0,false));
}

MathConstantNode::~MathConstantNode()
{
}

const char *MathConstantNode::Label()
{
	if (Name) return Name;

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	const char *Nm = ev->EnumLabel();

	return Nm;
}

NodeBase *MathConstantNode::Duplicate()
{
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	MathConstantNode *newnode = new MathConstantNode(ev->value);
	newnode->DuplicateBase(this);
	return newnode;
}

int MathConstantNode::Update()
{
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	//ObjectDef *def = ev->GetObjectDef();

	DoubleValue *v = dynamic_cast<DoubleValue*>(properties.e[1]->GetData());
	int id = ev->value;

	if      (id == CONST_Pi        ) v->d = M_PI;
	else if (id == CONST_Pi_Over_2 ) v->d = M_PI/2;
	else if (id == CONST_2_Pi      ) v->d = 2*M_PI;
	else if (id == CONST_E         ) v->d = M_E;
	else if (id == CONST_Tau       ) v->d = (1+sqrt(5))/2;
	else if (id == CONST_1_Over_Tau) v->d = 2/(1+sqrt(5));

	return NodeBase::Update();
}

Laxkit::anObject *newMathConstants(int p, Laxkit::anObject *ref)
{
	return new MathConstantNode(0);
}


//------------ TypeInfoNode ----------------------

/*! \class TypeInfoNode
 * Make a string with the type of value, or "null" for no value.
 */

class TypeInfoNode : public NodeBase
{
  public:
	TypeInfoNode();
	virtual ~TypeInfoNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

TypeInfoNode::TypeInfoNode()
{
	makestr(type, "TypeInfo");
	makestr(Name, _("Type Info"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",    nullptr,1, _("In"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "type",  new StringValue("null"),1 , _("typeinfo") ,nullptr,0,false )); 
}

TypeInfoNode::~TypeInfoNode()
{
}

NodeBase *TypeInfoNode::Duplicate()
{
	TypeInfoNode *newnode = new TypeInfoNode();
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int TypeInfoNode::GetStatus()
{
	return NodeBase::GetStatus(); //default checks mod times
}

int TypeInfoNode::Update()
{
	Value *v = properties.e[0]->GetData();
	
	dynamic_cast<StringValue*>(properties.e[1]->GetData())->Set(v ? v->whattype() : "null");

	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}


Laxkit::anObject *newTypeInfoNode(int p, Laxkit::anObject *ref)
{
	return new TypeInfoNode();
}



//------------ ImageNode

/*! \class ImageNode
 * Create a new LaxImage from a width, height, and color.
 */

class ImageNode : public NodeBase
{
  public:
	int for_info;
	ImageNode(int width, int height);
	virtual ~ImageNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int UpdatePreview();
};

ImageNode::ImageNode(int width, int height)
{
	makestr(type, "Image");
	makestr(Name, _("Image"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "width",  new IntValue(width),1 , _("Width")  )); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "height", new IntValue(height),1, _("Height") )); 

	//ObjectDef *enumdef = GetImageDepthDef();
	//EnumValue *e = new EnumValue(enumdef, 0);
	//AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Depth"), e, 1)); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Initial Color"), new ColorValue("#ffffff"), 1)); 

	//depth: 8, 16, 24, 32, 32f, 64f
	//format: gray, graya, rgb, rgba
	//backend: raw, default, gegl, gmic, gm, cairo

	ImageValue *v = new ImageValue();
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Image"), v,1)); 
}

ImageNode::~ImageNode()
{
}

int ImageNode::UpdatePreview()
{
	if (!show_preview) return 1;

	if (!total_preview) {
		// generate preview, scale down if big
		ImageValue *img = dynamic_cast<ImageValue*>(properties.e[3]->GetData());
		if (img && img->image) {
			if (img->image->w() < 200 && img->image->h() < 200) {
				total_preview = img->image;
				total_preview->inc_count();
			} else {
				total_preview = GeneratePreview(img->image, 200,200, true);
			}
		}
	}

	return 0;
}

NodeBase *ImageNode::Duplicate()
{
	int isnum;
	int width  = getNumberValue(properties.e[0]->GetData(), &isnum);
	int height = getNumberValue(properties.e[1]->GetData(), &isnum);

	ImageNode *newnode = new ImageNode(width, height);
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int ImageNode::GetStatus()
{
	int isnum;
	int width = getIntValue(properties.e[0]->GetData(), &isnum);
	if (!isnum || width <= 0) { makestr(error_message, _("Width must be positive")); return -1; }
	int height = getIntValue(properties.e[1]->GetData(), &isnum);
	if (!isnum || height <= 0) { makestr(error_message, _("Height must be positive")); return -1; }
	if (!dynamic_cast<ColorValue*>(properties.e[2]->GetData())) return -1;

	if (!properties.e[3]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ImageNode::Update()
{
	int isnum = 0;
	int width = getIntValue(properties.e[0]->GetData(), &isnum);
	if (!isnum || width <= 0 || width > 20000) return -1;
	int height = getIntValue(properties.e[1]->GetData(), &isnum);
	if (!isnum || height <=0 || height > 20000) return -1;
	ColorValue *color = dynamic_cast<ColorValue*>(properties.e[2]->GetData());
	if (!color) return -1;

	ImageValue *v = dynamic_cast<ImageValue*>(properties.e[3]->GetData());
	if (!v) {
		v = new ImageValue;
		properties.e[3]->SetData(v, 1);
	}
	if (v->image && (v->image->w() != width || v->image->h() != height)) {
		v->image->dec_count();
		v->image = nullptr;
	}
	if (!v->image) {
		v->image = ImageLoader::NewImage(width, height);
	}

	v->image->Set(color->color.Red(), color->color.Green(), color->color.Blue(), color->color.Alpha());

	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}


SingletonKeeper imageDepthKeeper;

ObjectDef *GetImageDepthDef()
{ 
	ObjectDef *edef = dynamic_cast<ObjectDef*>(imageDepthKeeper.GetObject());

	if (!edef) {
		//ObjectDef *def = new ObjectDef("ImageNode", _("Image Node"), NULL,NULL,"class", 0);

		edef = new ObjectDef("ColorDepth", _("Color depth"), NULL,NULL,"enum", 0);
		edef->pushEnumValue("d8",_("8"),_("8"));
		edef->pushEnumValue("d16",_("16"),_("16"));
		edef->pushEnumValue("d24",_("24"),_("24"));
		edef->pushEnumValue("d32",_("32"),_("32"));
		edef->pushEnumValue("d32f",_("32f"),_("32f"));
		edef->pushEnumValue("d64f",_("64f"),_("64f"));

		imageDepthKeeper.SetObject(edef, true);
	}

	return edef;
}

Laxkit::anObject *newImageNode(int p, Laxkit::anObject *ref)
{
	return new ImageNode(100, 100);
}


//------------ ImageFileNode

/*! \class ImageFileNode
 * Create a new LaxImage from a file.
 */

class ImageFileNode : public NodeBase
{
  public:
	ImageFileNode(const char *filename);
	virtual ~ImageFileNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int UpdatePreview();
	const char *GetFilename();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ImageFileNode(nullptr); }
};

ImageFileNode::ImageFileNode(const char *filename)
{
	makestr(type, "ImageFile");
	makestr(Name, _("Image File"));

// NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
// 					const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable=true);
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "file",  new FileValue(),1 , _("File"), nullptr,0,true  )); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Image"), nullptr,1));
}

ImageFileNode::~ImageFileNode()
{
}

int ImageFileNode::UpdatePreview()
{
	if (!show_preview) return 1;

	if (!total_preview) {
		ImageValue *img = dynamic_cast<ImageValue*>(properties.e[1]->GetData());
		if (img && img->image) {
			if (img->image->w() < 200 && img->image->h() < 200) {
				total_preview = img->image;
				total_preview->inc_count();
			} else {
				total_preview = GeneratePreview(img->image, 200,200, true);
			}
		}
	}

	return 0;
}

const char *ImageFileNode::GetFilename()
{
	Value *pv = properties.e[0]->GetData();
	StringValue *f = dynamic_cast<StringValue*>(pv);
	if (f) return f->str;
	else {
		FileValue *fv = dynamic_cast<FileValue*>(pv);
		if (fv) return fv->filename;
	}	
	return nullptr;
}

NodeBase *ImageFileNode::Duplicate()
{
	const char *file = GetFilename();
	
	ImageFileNode *newnode = new ImageFileNode(file);
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int ImageFileNode::GetStatus()
{
	//ImageValue *i = dynamic_cast<ImageValue*>(properties.e[1]->GetData());
	return NodeBase::GetStatus(); //default checks mod times
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int ImageFileNode::Update()
{
	const char *file = GetFilename();
	if (isblank(file)) return -1;

	ImageValue *iv = dynamic_cast<ImageValue*>(properties.e[1]->GetData());

	if (iv && iv->image && iv->image->filename && file && !strcmp(iv->image->filename, file)) {
		//same filename, assume its up to date *** should really check file mod time
		DBG cerr << " *** note to self, need to check file mod time for ImageFileNode::Update"<<endl;
		return NodeBase::Update();
	}
	
	//otherwise, create new image and install
	LaxImage *img = ImageLoader::LoadImage(file);
	if (!img) return -1;
	if (!iv) {
		iv = new ImageValue();
		properties.e[1]->SetData(iv, 1);
	}

	iv->SetImage(img);
	img->dec_count();
	if (total_preview) { total_preview->dec_count(); total_preview = nullptr; }
	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}


//------------ ImageInfoNode ----------------------

/*! \class ImageInfoNode
 * Expose information about the attached image..
 */

class ImageInfoNode : public NodeBase
{
  public:
	ImageInfoNode();
	virtual ~ImageInfoNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int UpdatePreview();
};

ImageInfoNode::ImageInfoNode()
{
	makestr(type, "ImageInfo");
	makestr(Name, _("Image Info"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",    nullptr,1, _("Image"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "width",  new IntValue(width),1 , _("Width") ,nullptr,0,false )); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "height", new IntValue(height),1, _("Height"),nullptr,0,false )); 

	//ObjectDef *enumdef = GetImageDepthDef();
	//EnumValue *e = new EnumValue(enumdef, 0);
	//AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Depth"), e, 1)); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "file", new StringValue(""),1, _("Filename"),nullptr,0,false)); 
}

ImageInfoNode::~ImageInfoNode()
{
}

int ImageInfoNode::UpdatePreview()
{
	if (!show_preview) return 1;

	// *** should probably scale down
	if (!total_preview) {
		ImageValue *img = dynamic_cast<ImageValue*>(properties.e[0]->GetData());
		total_preview = img->image;
	}

	return 0;
}

NodeBase *ImageInfoNode::Duplicate()
{
	ImageInfoNode *newnode = new ImageInfoNode();
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int ImageInfoNode::GetStatus()
{
	ImageValue *v = dynamic_cast<ImageValue*>(properties.e[0]->GetData());
	if (!v || !v->image) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ImageInfoNode::Update()
{
	ImageValue *v = dynamic_cast<ImageValue*>(properties.e[0]->GetData());
	if (!v || !v->image) return -1;

	dynamic_cast<IntValue*>(properties.e[1]->GetData())->i = v->image->w();
	dynamic_cast<IntValue*>(properties.e[2]->GetData())->i = v->image->h();
	dynamic_cast<StringValue*>(properties.e[3]->GetData())->Set(v->image->filename ? v->image->filename : "");

	UpdatePreview();
	Wrap();

	return NodeBase::Update();
}


Laxkit::anObject *newImageInfoNode(int p, Laxkit::anObject *ref)
{
	return new ImageInfoNode();
}


//------------ GenericNode

class GenericNode;
typedef int (*GenericNodeUpdateFunc)(GenericNode *node, anObject *data);
typedef int (*GenericNodeStatusFunc)(GenericNode *node);

/*! \class GenericNode
 * Class to hold custom nodes without a lot of c++ class definition overhead.
 */
class GenericNode : public NodeBase
{
  public:
	GenericNodeStatusFunc status_func;
	GenericNodeUpdateFunc update_func;
	anObject *func_data;

	GenericNode(ObjectDef *ndef);
	virtual ~GenericNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

GenericNode::GenericNode(ObjectDef *ndef)
{
	update_func = NULL;
	func_data = NULL;

	def = ndef;
	if (def) def->inc_count();

	ObjectDef *field;
	for (int c=0; c<def->getNumFields(); c++) {
		field = def->getField(c);

		if (field->format==VALUE_Function || field->format==VALUE_Class || field->format==VALUE_Operator || field->format==VALUE_Namespace) continue;

		//NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
		//			const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable=true);
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, field->name, NULL,1, field->Name, NULL, 0,true));
	}


	Update();
}

GenericNode::~GenericNode()
{
	if (func_data) func_data->dec_count();
}

NodeBase *GenericNode::Duplicate()
{
	cerr << " *** need to implement GenericNode::Duplicate!"<<endl;
	//newnode->DuplicateBase(this);
	return NULL;
}

int GenericNode::Update()
{
	if (update_func) update_func(this, func_data);
	return NodeBase::Update();
}

int GenericNode::GetStatus()
{
	if (status_func) return status_func(this);
	return NodeBase::GetStatus();
}


//------------ blank NodeGroup creator

Laxkit::anObject *newNodeGroup(int p, Laxkit::anObject *ref)
{
	NodeGroup *group = new NodeGroup;
	//group->InitializeBlank();
	return group;
}


//------------------------ RandomNode ------------------------

/*! \class RandomNode
 */
class RandomNode : public NodeBase
{
  public:
	RandomNode(int seed=0, double min=0, double max=1);
	virtual ~RandomNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new RandomNode(0, 0., 1.); }
};


RandomNode::RandomNode(int seed, double min, double max)
{
	makestr(Name, _("Random"));
	makestr(type, "Random");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Seed",    new IntValue(seed),1,      _("Seed"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Minimum", new DoubleValue(min),1,    _("Minimum"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Maximum", new DoubleValue(max),1,    _("Maximum"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false,"Integer", new BooleanValue(false),1, _("Integer"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Number", new DoubleValue(),1, _("Number"), nullptr,0,false)); 
}

RandomNode::~RandomNode()
{
}

NodeBase *RandomNode::Duplicate()
{
	int isnum=0;
	int   seed = getNumberValue(properties.e[0]->GetData(), &isnum);
	double min = getNumberValue(properties.e[1]->GetData(), &isnum);
	double max = getNumberValue(properties.e[2]->GetData(), &isnum);

	RandomNode *newnode = new RandomNode(seed, min, max);
	newnode->DuplicateBase(this);
	return newnode;
}

int RandomNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[1]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), NULL)) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

int RandomNode::Update()
{
	int isnum=0;
	int   seed = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (!isnum) return -1;
	double min = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	double max = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	bool isint = dynamic_cast<BooleanValue*>(properties.e[3]->GetData())->i;

	srandom(abs(seed));
	double num = random()/double(RAND_MAX) * (max-min) + min;
	
	Value *v = properties.e[4]->GetData();

	if (isint) {
		if (!dynamic_cast<IntValue*>(v)) {
			v = new IntValue(num+.5);
			properties.e[4]->SetData(v, 1);
		} else dynamic_cast<IntValue*>(v)->i = num+.5;
	} else {
		if (!dynamic_cast<DoubleValue*>(v)) {
			v = new DoubleValue(num);
			properties.e[4]->SetData(v, 1);
		} else dynamic_cast<DoubleValue*>(v)->d = num;
	}
	properties.e[4]->Touch();

	return NodeBase::Update();
}


//------------------------------ StringNode --------------------------------------------

class StringNode : public NodeBase
{
  public:
	StringNode(const char *s1);
	virtual ~StringNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

StringNode::StringNode(const char *s1)
{
	makestr(Name, _("String"));
	makestr(type, "String");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "S", new StringValue(s1),1, _("S")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new StringValue(s1),1, _("Out"), NULL,0,false));
}

StringNode::~StringNode()
{
}

NodeBase *StringNode::Duplicate()
{
	StringValue *s1 = dynamic_cast<StringValue*>(properties.e[0]->GetData());

	StringNode *newnode = new StringNode(s1 ? s1->str : NULL);
	newnode->DuplicateBase(this);
	return newnode;
}

int StringNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;

	if (!properties.e[1]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int StringNode::Update()
{
	StringValue *out = dynamic_cast<StringValue*>(properties.e[1]->GetData());

	char *str=NULL;
	int isnum=0;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (isnum) {
		str = numtostr(d);
		out->Set(str);
		delete[] str;
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
		if (!s) return -1;
		out->Set(s->str);
	}

	properties.e[1]->Touch();
	Touch();
	return NodeBase::Update();
}


Laxkit::anObject *newStringNode(int p, Laxkit::anObject *ref)
{
	return new StringNode(NULL);
}


//------------------------------ concat --------------------------------------------

class ConcatNode : public NodeBase
{
	bool GetValueString(Value *v, const char **cstr, char **str, SetValue **set);

  public:
	ConcatNode(const char *s1, const char *s2);
	virtual ~ConcatNode();
	virtual int GetStatus();
	virtual int Update();
	virtual NodeBase *Duplicate();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ConcatNode(NULL,NULL); }
};

ConcatNode::ConcatNode(const char *s1, const char *s2)
{
	makestr(Name, _("Concat"));
	makestr(type, "Concat");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "A", new StringValue(s1),1, _("A")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "B", new StringValue(s2),1, _("B")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", nullptr,1, _("Out"), nullptr, 0, false));
}

ConcatNode::~ConcatNode()
{
}

NodeBase *ConcatNode::Duplicate()
{
	StringValue *s1 = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	StringValue *s2 = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	ConcatNode *newnode = new ConcatNode(s1 ? s1->str : nullptr, s2 ? s2->str : nullptr);
	newnode->DuplicateBase(this);
	return newnode;
}

int ConcatNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!isNumberType(v, NULL) && v->type() != VALUE_String && v->type() != VALUE_Set) return -1;
	v = properties.e[1]->GetData();
	if (!isNumberType(v, NULL) && v->type() != VALUE_String && v->type() != VALUE_Set) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

bool ConcatNode::GetValueString(Value *v, const char **cstr, char **str, SetValue **set)
{
	if (!v) return false;
	int isnum=0;
	double d1 = getNumberValue(v, &isnum);
	if (isnum) {
		*str = numtostr(d1);
	} else {
		StringValue *s = dynamic_cast<StringValue*>(v);
		if (s) {
			*cstr = s->str;
		} else {
			*set = dynamic_cast<SetValue*>(v);
			if (!*set) {
				Error(_("In must be a number, string, or set of either."));
				return false;
			}
		}
	}
	return true;
}

int ConcatNode::Update()
{
	Value *v = properties.e[0]->GetData();
	char *str1 = nullptr;
	const char *strin1 = nullptr;
	SetValue *setin1 = nullptr;
	if (!GetValueString(v, &strin1, &str1, &setin1)) return -1;

	v = properties.e[1]->GetData();
	char *str2 = nullptr;
	const char *strin2 = nullptr;
	SetValue *setin2 = nullptr;
	if (!GetValueString(v, &strin2, &str2, &setin2)) return -1;

	bool isset = (setin1 || setin2);
	int max = 0;
	if (setin1 && setin1->n() > max) max = setin1->n();
	if (setin2 && setin2->n() > max) max = setin2->n();

	StringValue *out = nullptr;
	SetValue *outset = nullptr;
	if (isset) {
		outset = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
		if (!outset) {
			outset = new SetValue();
			properties.e[properties.n-1]->SetData(outset, 1);
		} else properties.e[properties.n-1]->Touch();
	} else {
		out = dynamic_cast<StringValue*>(properties.e[properties.n-1]->GetData());
		if (!out) {
			out = new StringValue();
			properties.e[properties.n-1]->SetData(out, 1);
		} else properties.e[properties.n-1]->Touch();
	}

	for (int c=0; c < (isset ? max : 1); c++) {
		if (setin1 && c < setin1->n()) {
			v = setin1->e(c);
			if (!GetValueString(v, &strin1, &str1, &setin1)) return -1;
		}
		if (setin2 && c < setin2->n()) {
			v = setin2->e(c);
			if (!GetValueString(v, &strin2, &str2, &setin2)) return -1;
		}
		if (outset) {
			if (c < outset->n()) {
				out = dynamic_cast<StringValue*>(outset->e(c));
				if (!out) {
					out = new StringValue();
					outset->Set(c, out, 1);
				}
			} else {
				out = new StringValue();
				outset->Push(out, 1);
			}
		}

		char *ss = newstr(strin1);
		appendstr(ss, strin2);
		out->InstallString(ss);

		if (str1) delete[] str1;
		if (str2) delete[] str2;
	}

	if (outset) {
		while (outset->n() > max) outset->Remove(max);
	}


	return NodeBase::Update();
}


//------------------------------ SplitNode  --------------------------------------------

class SplitNode : public NodeBase
{
  public:
	SplitNode();
	virtual ~SplitNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SplitNode(); }
};

SplitNode::SplitNode()
{
	makestr(Name, _("Split"));
	makestr(type, "Split strings");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", new StringValue(),1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "delim", new StringValue(","),1, _("Delimiter"), _("Use \\n for newline and \\t for tab.")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", nullptr,1, _("Out"),
								 nullptr, 0, false));
}

SplitNode::~SplitNode()
{
}

NodeBase *SplitNode::Duplicate()
{
	SplitNode *newnode = new SplitNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int SplitNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v || !(v->type() == VALUE_String && v->type() != VALUE_Set)) return -1;
	v = properties.e[1]->GetData();
	if (!v || v->type() != VALUE_String) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

int SplitNode::Update()
{
	StringValue *strin = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	SetValue *setin = nullptr;
	if (!strin) setin = dynamic_cast<SetValue*>(properties.e[0]->GetData());
	if (!strin && !setin) return -1;
	if (setin && setin->n() == 0) return -1;

	StringValue *delim = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!delim || isblank(delim->str)) return -1;
	Utf8String delimiter(delim->str);
	delimiter.Replace("\\n", "\n", true);
	delimiter.Replace("\\t", "\t", true);
	
	char *str = strin ? strin->str : nullptr;

	SetValue *outset = dynamic_cast<SetValue*>(properties.e[2]->GetData());
	if (!outset) {
		outset = new SetValue();
		properties.e[2]->SetData(outset, 1);
	} else {
		outset->Flush();
		properties.e[2]->Touch();
	}

	for (int c=0; c< (setin ? setin->n() : 1); c++){
		if (setin) {
			StringValue *sv = dynamic_cast<StringValue*>(setin->e(c));
			if (!sv) return -1;
			str = sv->str;
		}

		int n = 0;
		char **strs = split(str, delimiter.c_str(), &n);
		SetValue *set = outset;
		if (setin) {
			set = new SetValue();
			outset->Push(set, 1);
		}

		for (int c=0; c<n; c++) {
			set->Push(new StringValue(strs[c]), 1);
		}
		deletestrs(strs, n);
	}
	
	return NodeBase::Update();
}


//------------------------------ SliceNode --------------------------------------------

class SliceNode : public NodeBase
{
	char sliceLabel[90];
  public:
	SliceNode(const char *s1, int from, int to);
	virtual ~SliceNode();
	virtual const char *Label();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SliceNode(NULL,0,0); }
};

SliceNode::SliceNode(const char *s1, int from, int to)
{
	sliceLabel[0] = '\0';
	//makestr(Name, _("Slice"));
	makestr(type, "Slice");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "s",    new StringValue(s1),1, _("s")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "from", new IntValue(from),1,  _("From"), _("Negative values are from end of string")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "to",   new IntValue(to),1,    _("To"), _("Negative values are from end of string")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out")));
}

SliceNode::~SliceNode()
{
}

NodeBase *SliceNode::Duplicate()
{
	StringValue *s1 = dynamic_cast<StringValue*>(properties.e[0]->GetData());

	int isnum=0;
	int from = getNumberValue(properties.e[1]->GetData(), &isnum);
	int to   = getNumberValue(properties.e[2]->GetData(), &isnum);

	SliceNode *newnode = new SliceNode(s1 ? s1->str : NULL, from, to);
	newnode->DuplicateBase(this);
	return newnode;
}

int SliceNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;
	if (!isNumberType(properties.e[1]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), NULL)) return -1;

	if (!properties.e[3]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

const char *SliceNode::Label()
{
	if (!isblank(Name)) return Name;
	return sliceLabel;
}

int SliceNode::Update()
{
	char *str=NULL;

	int isnum=0;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (isnum) {
		str = numtostr(d);
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
		if (!s) return -1;

		str = newstr(s->str ? s->str : "");
	}

	int from = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;

	int to = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;

	StringValue *out = dynamic_cast<StringValue*>(properties.e[3]->GetData());
	if (!out) {
		out = new StringValue();
		properties.e[3]->SetData(out, 1);
	}

	int slen = strlen(str);
	if (from > slen) from = slen;
	else if (from < -slen) from = -slen;

	if (to > slen) to = slen;
	else if (to < -slen) to = -slen;
	sprintf(sliceLabel, _("Slice %d:%d"), from,to);

	if (from<0) from += slen;
	if (to<0)   to   += slen;


	if (to<from) {
		slen = from;
		from = to;
		to = slen;
	}

	out->Set(str+from, to-from+1);

	delete[] str;

	return NodeBase::Update();
}



//------------------------------ NumToStringNode --------------------------------------------

class NumToStringNode : public NodeBase
{
  public:
	NumToStringNode();
	virtual ~NumToStringNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new NumToStringNode(); }
};

NumToStringNode::NumToStringNode()
{
	makestr(Name, _("NumToString"));
	makestr(type, "Strings/NumToString");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Num", new DoubleValue(0),1, _("Num"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "asint", new BooleanValue(false),1, _("As integer")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Base", new IntValue(10),1, _("Base")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "capital", new BooleanValue(false),1, _("Capitalize"), _("For base 16, and exponentials")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Padding", new IntValue(0),1, _("Padding")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "zero", new BooleanValue(false),1, _("Pad 0")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Left", new BooleanValue(false),1, _("Left"), _("Left justify when value is thinner than padding")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Precision", new IntValue(1),1, _("Precision"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), nullptr, 0, false));
}

NumToStringNode::~NumToStringNode()
{
}

NodeBase *NumToStringNode::Duplicate()
{
	NumToStringNode *newnode = new NumToStringNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int NumToStringNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), NULL) && !dynamic_cast<SetValue*>(properties.e[0]->GetData())) return -1;
	if (!isNumberType(properties.e[1]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[3]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[4]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[5]->GetData(), NULL)) return -1;
	if (!isNumberType(properties.e[6]->GetData(), NULL)) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

int NumToStringNode::Update()
{
	Error(nullptr);
	SetValue *setin = nullptr;
	int isnum=0;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (!isnum) {
		setin = dynamic_cast<SetValue*>(properties.e[0]->GetData());
		if (!setin) {
			Error(_("In must be a number or a set of numbers"));
			return -1;
		}
	}

	bool asint = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Zero must be boolean"));
		return -1;
	}

	int base = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum || (base != 2 && base != 8 && base != 16 && base != 10)) {
		Error(_("Base must be 2, 8, 16, or 10"));
		return -1;
	}

	bool capitalize = getNumberValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Zero must be boolean"));
		return -1;
	}

	int padding = getNumberValue(properties.e[4]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Padding must be a number"));
		return -1;
	}
	if (padding < 0) padding = 0;

	bool zeropadding = getNumberValue(properties.e[5]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Zero must be boolean"));
		return -1;
	}
	
	bool left = getNumberValue(properties.e[6]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Left must be boolean"));
		return -1;
	}

	int precision = getNumberValue(properties.e[7]->GetData(), &isnum);
	if (!isnum || precision < 0) {
		Error(_("Precision must be a non-negative number"));
		return -1;
	}
	if (padding < 0) padding = 0;

	SetValue *setout = nullptr;
	StringValue *out = nullptr; 
	if (setin) {
		setout = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
		if (!setout) {
			setout = new SetValue();
			properties.e[properties.n-1]->SetData(setout, 1);
		} else properties.e[properties.n-1]->Touch();

	} else {
		out = dynamic_cast<StringValue*>(properties.e[properties.n-1]->GetData());
		if (!out) {
			out = new StringValue();
			properties.e[properties.n-1]->SetData(out, 1);
		} else properties.e[properties.n-1]->Touch();
	}

	Utf8String scratch;
	char format[50];
	if (asint) {
		char type = 'd';
		if (base == 8) type = 'o';
		else if (base == 16) type = capitalize ? 'X' : 'x';
		sprintf(format, "%%%s%s%d%c", left ? "-" : "", zeropadding ? "0" : (padding > 0 ? " " : ""), padding, type);
	} else {
		char type = capitalize ? 'G' : 'g'; // %010.10g
		sprintf(format, "%%%s%s%d.%d%c", left ? "-" : "", zeropadding ? "0" : (padding > 0 ? " " : ""), padding, precision, type);
	}
	for (int c=0; c < (setin ? setin->n() : 1); c++) {
		if (setin) {
			d = getNumberValue(setin->e(c), &isnum);
			if (!isnum) {
				Error(_("In must be a number or a set of numbers"));
				return -1;
			}

			if (c < setout->n()) {
				out = dynamic_cast<StringValue*>(setout->e(c));
				if (!out) {
					out = new StringValue();
					setout->Set(c, out, 1);
				}
			} else {
				out = new StringValue();
				setout->Push(out, 1);
			}
		}

		if (asint) {
			scratch.Sprintf(format, (int)(d+.5));
		} else {
			scratch.Sprintf(format, d);
		}
		out->Set(scratch.c_str());

	}

	return NodeBase::Update();
}



//------------------------ ThreadNode ------------------------

/*! \class ThreadNode
 * Starts an execution path.
 */

class ThreadNode : public NodeBase
{
  public:
	ThreadNode();
	virtual ~ThreadNode();

	virtual NodeBase *Duplicate();
	//virtual int Update();
	//virtual int GetStatus();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
};

ThreadNode::ThreadNode()
{
	makestr(type, "Threads/Thread");
	makestr(Name, _("Thread"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "out",  NULL,1, _("Out")));
}

ThreadNode::~ThreadNode()
{
}


NodeBase *ThreadNode::Duplicate()
{
	ThreadNode *node = new ThreadNode();
	node->DuplicateBase(this);
	return node;
}

NodeBase *ThreadNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	if (!properties.e[0]->connections.n) return NULL;
	return properties.e[0]->connections.e[0]->to;
}


Laxkit::anObject *newThreadNode(int p, Laxkit::anObject *ref)
{
	return new ThreadNode();
}


//------------------------ IfNode ------------------------

/*! \class IfNode
 * For loop node.
 */

class IfNode : public NodeBase
{
  public:
	IfNode(bool iftrue);
	virtual ~IfNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
};


IfNode::IfNode(bool iftrue)
{
	makestr(type,   "Threads/If");
	makestr(Name, _("If"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "In",    NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,    true, "If",    new BooleanValue(iftrue),1,_("if"), NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "True",  NULL,1, _("True")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "False", NULL,1, _("False")));
}

IfNode::~IfNode()
{
}

NodeBase *IfNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	int isnum;
	bool iftrue = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return NULL;

	NodeProperty *prop;
	if (iftrue) prop = properties.e[2];
	else prop = properties.e[3];

	NodeBase *next = NULL;
	if (prop->connections.n) next = prop->connections.e[0]->to;

	modtime = times(NULL);
	MarkMustUpdate();
	// PropagateUpdate();

	return next;
}

NodeBase *IfNode::Duplicate()
{
	int isnum;
	bool iftrue = getNumberValue(properties.e[3]->GetData(), &isnum);
	IfNode *node = new IfNode(iftrue);
	node->DuplicateBase(this);
	return node;
}

int IfNode::Update()
{
	int isnum;
	getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	return 0; //we only actually update from Execute
}

int IfNode::GetStatus()
{
	int isnum;
	getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	return 0;
}


Laxkit::anObject *newIfNode(int p, Laxkit::anObject *ref)
{
	return new IfNode(1);
}


//------------------------ LoopNode ------------------------

/*! \class LoopNode
 * Traditional for loop with start, end, and step.
 */

class LoopNode : public NodeBase
{
  public:
	double start, end, step, current;
	int running;

	LoopNode(double nstart, double nend, double nstep);
	virtual ~LoopNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
	virtual void ExecuteReset();
};


LoopNode::LoopNode(double nstart, double nend, double nstep)
{
	start   = nstart;
	end     = nend;
	step    = nstep;
	current = start;
	running = 0;

	makestr(type,   "Threads/Loop");
	makestr(Name, _("Loop"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "In",    NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Done",  NULL,1, _("Done")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Loop",  NULL,1, _("Loop")));
	 // *** while in loop, run to loop.
	 // when thread dead ends, return to this node and go out on Done

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Start", new DoubleValue(start),1,_("Start"), NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "End",   new DoubleValue(end),1,  _("End"),   NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step",  new DoubleValue(step),1, _("Step"),  NULL));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Current",  new DoubleValue(current),1, _("Current"),  NULL, 0, false));
}

LoopNode::~LoopNode()
{
}

void LoopNode::ExecuteReset()
{
	running = 0;
}

NodeBase *LoopNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	NodeProperty *done = properties.e[1];
	NodeProperty *loop = properties.e[2];

	if ((start>end && step>=0) || (start<end && step<=0)) return NULL;

	NodeBase *next = NULL;

	if (!running) {
		 //initialize loop, add as scope to the thread
		running = 1;
		current = start;
		//if      (loop->connections.n) thread->UpdateThread(loop->connections.e[0]->to, loop->connections.e[0]);
		//else if (done->connections.n) thread->UpdateThread(done->connections.e[0]->to, done->connections.e[0]);
		//else thread->UpdateThread(this, NULL);

		if      (loop->connections.n) next = loop->connections.e[0]->to;
		else if (done->connections.n) next = done->connections.e[0]->to;
		else next = this;

		dynamic_cast<DoubleValue*>(properties.e[6]->GetData())->d = current;
		properties.e[6]->Touch();
		MarkMustUpdate();
		// PropagateUpdate();
		thread->scopes.push(this);
		return next;
	}

	 //update current
	if (current + step > end) {
		 //end loop
		running = 0;
		if (done->connections.n) next = done->connections.e[0]->to;
		else next = NULL;
		thread->scopes.remove(-1);

	} else {
		 //continue loop
		current += step;
		if (loop->connections.n) next = loop->connections.e[0]->to;
		else next = this;

		dynamic_cast<DoubleValue*>(properties.e[6]->GetData())->d = current;
		properties.e[6]->Touch();
		PropagateUpdate();
	}

	return next;
}

NodeBase *LoopNode::Duplicate()
{
	LoopNode *node = new LoopNode(start, end, step);
	node->DuplicateBase(this);
	return node;
}

int LoopNode::Update()
{
	makestr(error_message, NULL);

	int isnum;
	start = getNumberValue(properties.e[3]->GetData(), &isnum);  if (!isnum) return -1;
	end   = getNumberValue(properties.e[4]->GetData(), &isnum);  if (!isnum) return -1;
	step  = getNumberValue(properties.e[5]->GetData(), &isnum);  if (!isnum) return -1;

	if ((start>end && step>=0) || (start<end && step<=0)) {
		makestr(error_message, _("Bad step value"));
		return -1;
	}
	return NodeBase::Update();
}

int LoopNode::GetStatus()
{
	//assume start, end, step all set correctly
	if ((start>end && step>=0) || (start<end && step<=0)) return -1;
	return 0;
}


Laxkit::anObject *newLoopNode(int p, Laxkit::anObject *ref)
{
	return new LoopNode(0,10,1);
}


//------------------------ ForeachNode ------------------------

/*! \class
 * For each loop node.
 */

class ForeachNode : public NodeBase
{
  public:
	int current;
	int running;

	ForeachNode();
	virtual ~ForeachNode();

	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
	virtual void ExecuteReset();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ForeachNode(); }
};


ForeachNode::ForeachNode()
{
	current = -1;
	running = 0;

	makestr(type,   "Threads/Foreach");
	makestr(Name, _("Foreach"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "In",    NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Done",  NULL,1, _("Done")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Foreach",  NULL,1, _("Foreach")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "list",  NULL,1, _("List")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Index",   new IntValue(current),1, _("Index"),   NULL, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Element", NULL,1, _("Element"), NULL, 0, false));
}

ForeachNode::~ForeachNode()
{
}

NodeBase *ForeachNode::Duplicate()
{
	ForeachNode *node = new ForeachNode();
	node->DuplicateBase(this);
	return node;
}

int ForeachNode::GetStatus()
{
	return 0;
}

void ForeachNode::ExecuteReset()
{
	current = -1;
	running = 0;
	//dynamic_cast<IntValue*>(FindProperty("Index")->GetData())->i = current;
}

NodeBase *ForeachNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	NodeProperty *done = properties.e[1];
	NodeProperty *loop = properties.e[2];

	NodeBase *next = NULL;

	int max = 0;
	Value *vin = properties.e[3]->GetData();
	Value *el = nullptr;
	ValueHash *hash = nullptr;
	SetValue *set = nullptr;

	if (vin->type() == VALUE_Hash) {
		hash = dynamic_cast<ValueHash*>(vin);
		max = hash->n();

	} else if (vin->type() == VALUE_Set) {
		set = dynamic_cast<SetValue*>(vin);
		max = set->n();

	} //else vectors? each point in a path?

	if (!running && max>0) {
		 //initialize loop
		running = 1;
		current = 0;

		if      (loop->connections.n) next = loop->connections.e[0]->to;
		else if (done->connections.n) next = done->connections.e[0]->to;
		else next = this;

		if (hash) el = hash->value(current);
		else if (set) el = set->e(current);
		properties.e[5]->SetData(el, 0);

		dynamic_cast<IntValue*>(properties.e[4]->GetData())->i = current;
		properties.e[4]->Touch();
		MarkMustUpdate();
		// PropagateUpdate();
		thread->scopes.push(this);
		return next;
	}

	 //update current
	if (current == max) {
		 //end loop
		running = 0;
		if (done->connections.n) next = done->connections.e[0]->to;
		else next = NULL;
		thread->scopes.remove(-1);

	} else {
		 //continue loop
		current++;
		if (loop->connections.n) next = loop->connections.e[0]->to;
		else next = this;

		if (hash) el = hash->value(current);
		else if (set) el = set->e(current);
		properties.e[5]->SetData(el, 0);

		dynamic_cast<IntValue*>(properties.e[4]->GetData())->i = current;
		properties.e[4]->Touch();
		MarkMustUpdate();
		// PropagateUpdate();
	}

	return next;
}

int ForeachNode::Update()
{
	Error(nullptr);
	Value *vin = properties.e[3]->GetData();
	if (vin->type() != VALUE_Hash && vin->type() != VALUE_Set) return -1;
	return NodeBase::Update();
}


//------------------------ ForkNode ------------------------

/*! \class ForkNode
 * Input one thread, output two.
 * The second one does NOT inherit the input scopes.
 */

class ForkNode : public NodeBase
{
  public:
	ForkNode();
	virtual ~ForkNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
};


ForkNode::ForkNode()
{
	makestr(type,   "Threads/Fork");
	makestr(Name, _("Fork"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "In",    NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Out",  NULL,1, _("Out")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "Out2", NULL,1, _("Out2"), _("Does not inherit in's scope")));
}

ForkNode::~ForkNode()
{
}

NodeBase *ForkNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	NodeBase *next = NULL, *next2 = NULL;
	NodeProperty *prop;

	prop = properties.e[1];
	if (prop->connections.n) next = prop->connections.e[0]->to;

	NodeProperty *prop2 = properties.e[2];
	if (prop2->connections.n) {
		next2 = prop2->connections.e[0]->to;
		forks.push(new NodeThread(next2, prop2, NULL,0));
	}

	modtime = times(NULL);
	//PropagateUpdate();

	return next;
}

NodeBase *ForkNode::Duplicate()
{
	ForkNode *node = new ForkNode();
	node->DuplicateBase(this);
	return node;
}

int ForkNode::Update()
{
	return 0; //we only actually update from Execute
}

int ForkNode::GetStatus()
{
	return 0;
}


Laxkit::anObject *newForkNode(int p, Laxkit::anObject *ref)
{
	return new ForkNode();
}


//------------------------------ SetVariableNode --------------------------------------------

/*! \class SetVariableNode
 * In threads, set a state variable.
 */

class SetVariableNode : public NodeBase
{
  public:
	SetVariableNode();
	virtual ~SetVariableNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
};

SetVariableNode::SetVariableNode()
{
	makestr(type, "Threads/SetVariable");
	makestr(Name, _("Set Variable"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "in",   NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "out",  NULL,1, _("Out")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "name",   new StringValue("v"),1, _("Name")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "value",  NULL,1,                 _("Value")));
}

SetVariableNode::~SetVariableNode()
{
}

NodeBase *SetVariableNode::Duplicate()
{
	SetVariableNode *node = new SetVariableNode();
	node->DuplicateBase(this);
	return node;
}

int SetVariableNode::Update()
{
	return 0; //do nothing here!
}

int SetVariableNode::GetStatus()
{
	NodeProperty *nameprop  = properties.e[2];
	StringValue *s = dynamic_cast<StringValue*>(nameprop->GetData());
	if (!s || isblank(s->str)) return -1;
	return 0;
}

//void SetVariableNode::ExecuteReset()
//{
//}

NodeBase *SetVariableNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	NodeProperty *nameprop  = properties.e[2];
	NodeProperty *valueprop = properties.e[3];

	StringValue *s = dynamic_cast<StringValue*>(nameprop->GetData());
	if (!s || isblank(s->str)) return NULL;

	Value *value = valueprop->GetData();

	thread->data->push(s->str, value);

	NodeProperty *out = properties.e[1];
	if (out->connections.n) return out->connections.e[0]->to;

	MarkMustUpdate();
	// PropagateUpdate();
	return NULL;
}



Laxkit::anObject *newSetVariableNode(int p, Laxkit::anObject *ref)
{
	return new SetVariableNode();
}


//------------------------------ GetVariableNode --------------------------------------------

/*! \class GetVariableNode
 * Map arrays to other arrays using a special GetVariable interface.
 */

class GetVariableNode : public NodeBase
{
  public:
	GetVariableNode();
	virtual ~GetVariableNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
	virtual void ExecuteReset();
};

GetVariableNode::GetVariableNode()
{
	makestr(type, "Threads/GetVariable");
	makestr(Name, _("Get Variable"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "in",   NULL,1, _("In")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "out",  NULL,1, _("Out")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "name",   new StringValue("v"),1, _("Name")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "value",  NULL,1,                 _("Value"), NULL, 0, false));
}

GetVariableNode::~GetVariableNode()
{
}

NodeBase *GetVariableNode::Duplicate()
{
	GetVariableNode *node = new GetVariableNode();
	node->DuplicateBase(this);
	return node;
}

int GetVariableNode::Update()
{
	return 0; //do nothing here! Things happen in Execute()
}

int GetVariableNode::GetStatus()
{
	NodeProperty *nameprop  = properties.e[2];
	StringValue *s = dynamic_cast<StringValue*>(nameprop->GetData());
	if (!s || isblank(s->str)) return -1;
	return 0;
}

void GetVariableNode::ExecuteReset()
{
	properties.e[3]->SetData(NULL,0);
}

NodeBase *GetVariableNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	NodeProperty *nameprop  = properties.e[2];

	StringValue *s = dynamic_cast<StringValue*>(nameprop->GetData());
	if (!s || isblank(s->str)) {
		properties.e[3]->SetData(NULL,0);
	} else {
		Value *value = thread->data->find(s->str);
		properties.e[3]->SetData(value,0);
	}

	NodeProperty *out = properties.e[1];
	if (out->connections.n) return out->connections.e[0]->to;

	MarkMustUpdate();
	// PropagateUpdate();
	return NULL;
}


Laxkit::anObject *newGetVariableNode(int p, Laxkit::anObject *ref)
{
	return new GetVariableNode();
}


//------------------------------ DelayNode --------------------------------------------

/*! \class DelayNode
 * Map arrays to other arrays using a special Delay interface.
 */

class DelayNode : public NodeBase
{
  public:
	std::clock_t wait_until;
	bool is_waiting;

	DelayNode(double seconds);
	virtual ~DelayNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	//virtual int GetStatus();
	virtual NodeBase *Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks);
};

DelayNode::DelayNode(double seconds)
{
	makestr(type, "Threads/Delay");
	makestr(Name, _("Delay"));

	wait_until = 0;
	is_waiting = false;

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_In,  true, "in",    NULL, 1, _("In"),NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "out",   NULL, 1, _("Out"),NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,    true, "Delay", new DoubleValue(seconds), 1, _("Delay"), _("In seconds")));
}

DelayNode::~DelayNode()
{
}

NodeBase *DelayNode::Duplicate()
{
	int isnum;
	double secs = getNumberValue(properties.e[2]->GetData(), &isnum);

	DelayNode *node = new DelayNode(secs);
	node->DuplicateBase(this);
	return node;
}

NodeBase *DelayNode::Execute(NodeThread *thread, Laxkit::PtrStack<NodeThread> &forks)
{
	if (!is_waiting) {
		is_waiting = true;
		Update(); //updates wait_until
		return this;
	}

	clock_t curtime = times(NULL);
	if (wait_until > curtime) {
		//still waiting
		return this;
	}
	
	//else we've waited long enough.. continue!

	is_waiting = false;

	if (!properties.e[1]->connections.n) {
		return NULL;
	}

	return properties.e[1]->connections.e[0]->to;
}

int DelayNode::Update()
{
	//maybe delay value was changed
	int isnum = 0;
	double secs = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (isnum && secs>0) {
		clock_t curtime = times(NULL);
		wait_until = curtime + secs * sysconf(_SC_CLK_TCK);
	}

	return 0;
}


Laxkit::anObject *newDelayNode(int p, Laxkit::anObject *ref)
{
	return new DelayNode(1);
}


//------------------------ LerpNode ------------------------

/*! \class LerpNode
 */
class LerpNode : public NodeBase
{
  public:
	LerpNode(double a=0, double b=1, double r=0);
	virtual ~LerpNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

LerpNode::LerpNode(double a, double b, double r)
{
	makestr(Name, _("Lerp"));
	makestr(type, "Lerp");
	//makestr(description, _("Linear interpolation"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "A", new DoubleValue(a),1,    _("A"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "B", new DoubleValue(b),1,    _("B"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "r", new DoubleValue(r),1,    _("r"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new DoubleValue(),1, _("Out"), NULL,0,false)); 
}

LerpNode::~LerpNode()
{
}

NodeBase *LerpNode::Duplicate()
{
	int isnum=0;
	double a = getNumberValue(properties.e[0]->GetData(), &isnum);
	double b = getNumberValue(properties.e[1]->GetData(), &isnum);
	double r = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (a==b && a==0) b=1;

	LerpNode *newnode = new LerpNode(a,b,r);
	newnode->DuplicateBase(this);
	return newnode;
}

int LerpNode::GetStatus()
{
	if (!isNumberType(properties.e[2]->GetData(), NULL)) return -1;

	int avn = isVectorType(properties.e[0]->GetData(), NULL);
	if (!avn) return -1;
	int bvn = !isVectorType(properties.e[1]->GetData(), NULL);
	if (!bvn) return -1;
	if (avn!=bvn) return -1;

	if (!properties.e[3]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int LerpNode::Update()
{
	int isnum=0;
	double r = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;

	double av[4], bv[4];
	int avn = isVectorType(properties.e[0]->GetData(), av);
	if (!avn) return -1;
	int bvn = isVectorType(properties.e[1]->GetData(), bv);
	if (!bvn) return -1;
	if (avn != bvn) return -1;

	for (int c=0; c<avn; c++) av[c] = av[c] + (bv[c] - av[c])*r;
	
	Value *v = properties.e[3]->GetData();

	if (avn==1) {
		if (!dynamic_cast<DoubleValue*>(v)) {
			v = new DoubleValue(av[0]);
			properties.e[3]->SetData(v, 1);
		} else dynamic_cast<DoubleValue*>(v)->d = av[0];

	} else if (avn==2) {
		if (!dynamic_cast<FlatvectorValue*>(v)) {
			v = new FlatvectorValue(flatvector(av));
			properties.e[3]->SetData(v, 1);
		} else dynamic_cast<FlatvectorValue*>(v)->v.set(av);

	} else if (avn==3) {
		if (!dynamic_cast<SpacevectorValue*>(v)) {
			v = new SpacevectorValue(spacevector(av));
			properties.e[3]->SetData(v, 1);
		} else dynamic_cast<SpacevectorValue*>(v)->v.set(av);

	} else if (avn==4) {
		if (!dynamic_cast<QuaternionValue*>(v)) {
			v = new QuaternionValue(Quaternion(av));
			properties.e[3]->SetData(v, 1);
		} else dynamic_cast<QuaternionValue*>(v)->v.set(av);
	}

	return NodeBase::Update();
}

Laxkit::anObject *newLerpNode(int p, Laxkit::anObject *ref)
{
	return new LerpNode(0., 1., 0.);
}


//------------------------ MapRangeNode ------------------------

/*! \class MapRangeNode
 */
class MapRangeNode : public NodeBase
{
  public:
	bool mapto;
	MapRangeNode(bool map_to, double min, double max, bool clamp);
	virtual ~MapRangeNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

MapRangeNode::MapRangeNode(bool map_to, double min, double max, bool clamp)
{
	mapto = map_to;

	if (mapto) {
		makestr(Name, _("0..1 to range"));
		makestr(type, "MapToRange");
	} else {
		makestr(Name, _("Range to 0..1"));
		makestr(type, "MapFromRange");
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", new DoubleValue(0),1,    _("In"),
		mapto ? _("0..1 maps to Min..Max") : _("Min..Max maps to 0..1"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Min", new DoubleValue(min),1,    _("Min"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Max", new DoubleValue(max),1,    _("Max"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false,"Clamp", new BooleanValue(clamp),1, _("Clamp"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new DoubleValue(),1, _("Out"), NULL,0,false)); 
}

MapRangeNode::~MapRangeNode()
{
}

NodeBase *MapRangeNode::Duplicate()
{
	int isnum=0;
	double min = getNumberValue(properties.e[1]->GetData(), &isnum);
	double max = getNumberValue(properties.e[2]->GetData(), &isnum);
	bool clamp = dynamic_cast<BooleanValue*>(properties.e[3]->GetData())->i;

	MapRangeNode *newnode = new MapRangeNode(mapto, min,max, clamp);
	newnode->DuplicateBase(this);
	return newnode;
}

int MapRangeNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!isNumberType(v, NULL) && !(v && v->type() == VALUE_Set)) return -1;

	int isnum;
	v = properties.e[1]->GetData();
	getNumberValue(v, &isnum);
	if (!isnum && !(v && v->type() == VALUE_Set)) return -1;

	v = properties.e[2]->GetData();
	getNumberValue(v, &isnum);
	if (!isnum && !(v && v->type() == VALUE_Set)) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

int MapRangeNode::Update()
{
	ClearError();

	int num_ins = 3;
	Value *ins[num_ins];
	for (int c=0; c<num_ins; c++) ins[c] = properties.e[c]->GetData();
	
	SetValue *setins[num_ins];
	SetValue *outset = nullptr;

	int max = 0;
	bool dosets = false;
	if (DetermineSetIns(num_ins, ins, setins, max, dosets) == -1) { //does not check contents of sets.
		//had a null input
		return -1;
	}

	DoubleValue *dv = UpdatePropType<DoubleValue>(properties.e[properties.n-1], dosets, max, outset);

	// DoubleValue *in[num_ins];
	// for (int c=0; c<num_ins; c++) in[c] = nullptr;

	double input=0, min=0, maxv=0;

	bool clamp = dynamic_cast<BooleanValue*>(properties.e[3]->GetData())->i;

	for (int c=0; c<max; c++) {
		if (GetInNumber(c, dosets, input, ins[0], setins[0]) != 0) {
			Error(_("Bad in!"));
			return -1;
		}
		if (GetInNumber(c, dosets, min, ins[1], setins[1]) != 0) {
			Error(_("Bad min!"));
			return -1;
		}
		if (GetInNumber(c, dosets, maxv, ins[2], setins[2]) != 0) {
			Error(_("Bad max!"));
			return -1;
		}

		double num;
		if (mapto) {
			num = min + (maxv-min)*input;
			if (clamp) {
				if (maxv>min) {
					if (num<min) num=min;
					else if (num>maxv) num=maxv;
				} else {
					if (num<maxv) num=maxv;
					else if (num>min) num=min;
				}
			}
		} else {
			if (maxv == min) num = 0;
			else {
				num = (input-min) / (maxv-min);
				if (clamp) {
					if (num<0) num=0;
					if (num>1) num=1;
				}
			}
		}

		GetOutValue<DoubleValue>(c, dosets, dv, outset);
		dv->d = num;
	}

	properties.e[properties.n-1]->Touch();
	return NodeBase::Update();
}

Laxkit::anObject *newMapToRangeNode(int p, Laxkit::anObject *ref)
{
	return new MapRangeNode(true, 0,1, false);
}

Laxkit::anObject *newMapFromRangeNode(int p, Laxkit::anObject *ref)
{
	return new MapRangeNode(false, 0,1, false);
}


//------------ EmptySetNode

Laxkit::anObject *newEmptySetNode(int p, Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	makestr(node->Name, _("Set"));
	makestr(node->type, "Basics/EmptySet");

	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "set", new SetValue(), 1, _("Set")));
	return node;
}


//------------------------------ GetSizeNode --------------------------------------------

/*! \class GetSizeNode
 * Get the size of an input set
 */

class GetSizeNode : public NodeBase
{
  public:
	GetSizeNode();
	virtual ~GetSizeNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	
	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GetSizeNode(); }
};

GetSizeNode::GetSizeNode()
{
	makestr(type, "Lists/GetSize");
	makestr(Name, _("Size"));


	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "obj",  nullptr,1, _("Object")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  new IntValue(),1,    _("Out"), NULL, 0, false));
}

GetSizeNode::~GetSizeNode()
{
}

NodeBase *GetSizeNode::Duplicate()
{
	GetSizeNode *node = new GetSizeNode();
	node->DuplicateBase(this);
	return node;
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int GetSizeNode::GetStatus()
{
	Value *in = properties.e[0]->GetData();
	if (!in) return -1;
	if (in->type() != VALUE_Set && in->type() != VALUE_Hash && in->type() != PointSetValue::TypeNumber()) return -1;
	return NodeBase::GetStatus();
}

int GetSizeNode::Update()
{
	Error(nullptr);

	SetValue *set = dynamic_cast<SetValue*>(properties.e[0]->GetData());
	if (set) {
		dynamic_cast<IntValue*>(properties.e[1]->GetData())->i = set->n();
	} else {
		ValueHash *hash = dynamic_cast<ValueHash*>(properties.e[0]->GetData());
		if (hash) {
			dynamic_cast<IntValue*>(properties.e[1]->GetData())->i = set->n();
		} else {
			PointSet *ps = dynamic_cast<PointSet*>(properties.e[0]->GetData());
			if (ps) {
				dynamic_cast<IntValue*>(properties.e[1]->GetData())->i = ps->NumPoints();
			} else return -1;
		}
	}
	
	properties.e[1]->Touch();
	return NodeBase::Update();
}


//------------------------------ GetElementNode --------------------------------------------

/*! \class GetElementNode
 * Get sub element.
 */

class GetElementNode : public NodeBase
{
	int which;

  public:
	GetElementNode(int which);
	virtual ~GetElementNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual Value *PreviewFrom() { return properties.e[2]->GetData(); }

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GetElementNode(p); }
};

GetElementNode::GetElementNode(int which)
{
	this->which = which;
	makestr(type, which ? "Drawable/GetChild" : "Lists/GetElement");
	makestr(Name, which ? _("Get child") : _("Get Element"));


	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "obj",  nullptr,1, _("Object")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "index", new IntValue(0),1, _("Index")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  NULL,1,            _("Out"), NULL, 0, false));
}

GetElementNode::~GetElementNode()
{
}

NodeBase *GetElementNode::Duplicate()
{
	GetElementNode *node = new GetElementNode(which);
	node->DuplicateBase(this);
	return node;
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int GetElementNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return -1;
	if (v->type() != VALUE_Set && v->type() != VALUE_Hash && !dynamic_cast<DrawableObject*>(v)) {
		Error(_("Input must be a list, dictionary, or Drawable!"));
		return -1;
	}

	v = properties.e[1]->GetData();
	if (!v || (!isNumberType(v, nullptr) && v->type() != VALUE_String)) {
		Error(_("Index must be a number or object name!"));
		return -1;
	}
	return NodeBase::GetStatus();
}

int GetElementNode::Update()
{
	Error(nullptr);

	Value *in = properties.e[0]->GetData();
	SetValue *set = dynamic_cast<SetValue*>(in);
	DrawableObject *dobj = nullptr;
	ValueHash *hash = nullptr;
	if (!set) {
		dobj = dynamic_cast<DrawableObject*>(in);
		if (!dobj) {
			hash = dynamic_cast<ValueHash*>(in);
			if (!hash) {
				Error(_("Input must be a list or a Drawable object!"));
				return -1;
			}
		}
	}

	int isnum = 0;
	Value *v = properties.e[1]->GetData();
	int index = getIntValue(v, &isnum);
	Value *out = nullptr;

	if (!isnum && v->type() != VALUE_String) {
		Error(_("Index must be a number or name!"));
		return -1;
	}
	if (v->type() == VALUE_String) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (set) {
			out = set->FindID(sv->str);
		} else if (hash) {
			out = hash->find(sv->str);
		} else { // dobj
			out = dynamic_cast<DrawableObject*>(dobj->FindChild(sv->str));
		}
		if (!out) {
			Error(_("Bad index!"));
			return -1;
		}
	} else { //use index
		if (index < 0 || index >= (set ? set->n() : (hash ? hash->n() : dobj->NumKids()))) {
			Error(_("Bad index!"));
			return -1;
		}
		if (set) out = set->e(index);
		else if (hash) out = hash->value(index);
		else out = dynamic_cast<DrawableObject*>(dobj->Child(index));
	}
	
	if (!out) return -1;
	properties.e[2]->SetData(out,0);
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}


//------------------------------ SwitchNode --------------------------------------------

/*! \class SwitchNode
 * Make a set containing all the inputs.
 */

class SwitchNode : public NodeBase
{
  public:
	SwitchNode();
	virtual ~SwitchNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SwitchNode(); }
};

SwitchNode::SwitchNode()
{
	makestr(type, "Lists/Switch");
	makestr(Name, _("Switch"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "index", new IntValue(0),1, _("Index"))); 
	AddNewIn(true, "in", _("In"), nullptr);

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  NULL,1, _("Out"), NULL, 0, false));
}

SwitchNode::~SwitchNode()
{
}

NodeBase *SwitchNode::Duplicate()
{
	SwitchNode *node = new SwitchNode();
	node->DuplicateBase(this);
	return node;
}

/*! Clean up after NodeBase::dump_in_atts() to make sure the out property is at the end.
 */
void SwitchNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att,flag,context);
	int i = -1;
	NodeProperty *prop = FindProperty("out", &i);
	if (prop && i != properties.n-1) {
		properties.slide(i, properties.n-1);
	}
	prop = FindProperty("in", &i);
	if (prop && i != properties.n-2) {
		properties.slide(i, properties.n-2);
	}
	for (int c=0; c<properties.n-1; c++) {
		properties.e[c]->Label(_("In"));
		if (c < properties.n-2) {
			// *** this really should be automated somehow
			properties.e[c]->SetFlag(NodeProperty::PROPF_New_In, false);
			properties.e[c]->SetFlag(NodeProperty::PROPF_List_In, true);
		}
	}
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int SwitchNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int SwitchNode::Update()
{
	ClearError();

	int isnum = 0;
	int index = getIntValue(properties.e[0]->GetData(), &isnum);
	if (!isnum || index < 0 || index >= properties.n-3) {
		Error(_("Bad index!"));
		return -1;
	}

	Value *out = properties.e[properties.n-1]->GetData();
	Value *el = properties.e[index]->GetData();
	if (el != out) {
		properties.e[properties.n-1]->SetData(el, 0);
	}
	properties.e[properties.n-1]->Touch();

	return NodeBase::Update();
}


//------------------------------ SubsetNode --------------------------------------------

/*! \class SubsetNode
 * Get a subset of a list.
 */

class SubsetNode : public NodeBase
{
  public:
	SubsetNode();
	virtual ~SubsetNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SubsetNode(); }
};

SubsetNode::SubsetNode()
{
	makestr(type, "Lists/Subset");
	makestr(Name, _("Subset"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "set",  nullptr,1, _("Set")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "from", new IntValue(0),1, _("From"), _("Negative numbers count from end")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "to",   new IntValue(-1),1, _("To"), _("Negative numbers count from end")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  NULL,1,            _("Out"), NULL, 0, false));
}

SubsetNode::~SubsetNode()
{
}

NodeBase *SubsetNode::Duplicate()
{
	SubsetNode *node = new SubsetNode();
	node->DuplicateBase(this);
	return node;
}

int SubsetNode::Update()
{
	Error(nullptr);

	Value *in = properties.e[0]->GetData();
	SetValue *set = dynamic_cast<SetValue*>(in);
	if (!set) {
		Error(_("Input must be a list!"));
		return -1;
	}

	int isnum = 0;
	int from = getIntValue(properties.e[1]->GetData(), &isnum);
	int to;
	if (isnum) to = getIntValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) {
		Error(_("From and to must be numbers!"));
		return -1;
	}
	if (from < 0) from = set->n()+from;
	if (to < 0) to = set->n()+to;
	if (from < 0 || from >= set->n() || to < 0 || to >= set->n()) {
		Error(_("Index out of range!"));
		return -1;
	}
	
	SetValue *out = dynamic_cast<SetValue*>(properties.e[3]->GetData());
	if (!out) {
		out = new SetValue();
		properties.e[3]->SetData(out, 1);
	} else out->Flush();

	for (int c=from; (from < to && c <= to) || (from >= to && c >= to); c += (from > to ? -1 : 1)) {
		out->Push(set->e(c), 0);
	}

	return NodeBase::Update();
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int SubsetNode::GetStatus()
{
	SetValue *set = dynamic_cast<SetValue*>(properties.e[0]->GetData());
	if (!set) {
		Error(_("Input must be a list!"));
		return -1;
	}

	int isnum = 0;
	getIntValue(properties.e[1]->GetData(), &isnum);
	if (isnum) getIntValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) {
		Error(_("Index must be a number!"));
		return -1;
	}
	return NodeBase::GetStatus();
}


//------------------------------ JoinSetsNode --------------------------------------------

/*! \class JoinSetsNode
 * Concatenate sets.
 */

class JoinSetsNode : public NodeBase
{
  public:
	JoinSetsNode();
	virtual ~JoinSetsNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new JoinSetsNode(); }
};

JoinSetsNode::JoinSetsNode()
{
	makestr(type, "Lists/JoinSets");
	makestr(Name, _("Join sets"));

	// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "set",  nullptr,1, _("Set")));
	// virtual NodeProperty *AddNewIn (int is_for_list, const char *nname, const char *nlabel, const char *ttip, int where=-1);
	AddNewIn(true, "set", _("Set"), nullptr);

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  NULL,1, _("Out"), NULL, 0, false));
}

JoinSetsNode::~JoinSetsNode()
{
}

NodeBase *JoinSetsNode::Duplicate()
{
	JoinSetsNode *node = new JoinSetsNode();
	node->DuplicateBase(this);
	return node;
}

/*! Clean up after NodeBase::dump_in_atts() to make sure the out property is at the end.
 */
void JoinSetsNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att,flag,context);
	int i = -1;
	NodeProperty *prop = FindProperty("out", &i);
	if (prop && i != properties.n-1) {
		properties.slide(i, properties.n-1);
	}
	prop = FindProperty("set", &i);
	if (prop && i != properties.n-2) {
		properties.slide(i, properties.n-2);
	}
	for (int c=0; c<properties.n-1; c++) {
		properties.e[c]->Label(_("Set"));
		if (c < properties.n-2) {
			// *** this really should be automated somehow
			properties.e[c]->SetFlag(NodeProperty::PROPF_New_In, false);
			properties.e[c]->SetFlag(NodeProperty::PROPF_List_In, true);
		}
	}
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int JoinSetsNode::GetStatus()
{
	for (int c=0; c<properties.n-2; c++) {
		if (!properties.e[c]->GetData()) continue;
		SetValue *set = dynamic_cast<SetValue*>(properties.e[c]->GetData());
		if (!set) {
			Error(_("Input must be a list!"));
			return -1;
		}
	}
	return NodeBase::GetStatus();
}

int JoinSetsNode::Update()
{
	Error(nullptr);

	for (int c=0; c<properties.n-2; c++) {
		if (!properties.e[c]->GetData()) continue;
		SetValue *set = dynamic_cast<SetValue*>(properties.e[c]->GetData());
		if (!set) {
			Error(_("Input must be a list!"));
			return -1;
		}
	}

	SetValue *out = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
	if (!out) {
		out = new SetValue();
		properties.e[properties.n-1]->SetData(out, 1);
	} else {
		out->Flush();
		properties.e[properties.n-1]->Touch();
	}

	for (int c=0; c<properties.n-2; c++) {
		SetValue *set = dynamic_cast<SetValue*>(properties.e[c]->GetData());
		if (!set) continue;
		for (int c2=0; c2<set->n(); c2++) {
			out->Push(set->e(c2), 0);
		}
	}

	return NodeBase::Update();
}


//------------------------------ MakeSetNode --------------------------------------------

/*! \class MakeSetNode
 * Make a set containing all the inputs.
 */

class MakeSetNode : public NodeBase
{
  public:
	MakeSetNode();
	virtual ~MakeSetNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new MakeSetNode(); }
};

MakeSetNode::MakeSetNode()
{
	makestr(type, "Lists/MakeSet");
	makestr(Name, _("Make set"));

	// AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "set",  nullptr,1, _("Set")));
	// virtual NodeProperty *AddNewIn (int is_for_list, const char *nname, const char *nlabel, const char *ttip, int where=-1);
	AddNewIn(true, "in", _("In"), nullptr);

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  NULL,1, _("Out"), NULL, 0, false));
}

MakeSetNode::~MakeSetNode()
{
}

NodeBase *MakeSetNode::Duplicate()
{
	MakeSetNode *node = new MakeSetNode();
	node->DuplicateBase(this);
	return node;
}

/*! Clean up after NodeBase::dump_in_atts() to make sure the out property is at the end.
 */
void MakeSetNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att,flag,context);
	int i = -1;
	NodeProperty *prop = FindProperty("out", &i);
	if (prop && i != properties.n-1) {
		properties.slide(i, properties.n-1);
	}
	prop = FindProperty("in", &i);
	if (prop && i != properties.n-2) {
		properties.slide(i, properties.n-2);
	}
	for (int c=0; c<properties.n-1; c++) {
		properties.e[c]->Label(_("In"));
		if (c < properties.n-2) {
			// *** this really should be automated somehow
			properties.e[c]->SetFlag(NodeProperty::PROPF_New_In, false);
			properties.e[c]->SetFlag(NodeProperty::PROPF_List_In, true);
		}
	}
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int MakeSetNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int MakeSetNode::Update()
{
	Error(nullptr);

	SetValue *out = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
	if (!out) {
		out = new SetValue();
		properties.e[properties.n-1]->SetData(out, 1);
	} else {
		out->Flush();
		properties.e[properties.n-1]->Touch();
	}

	for (int c=0; c<properties.n-2; c++) {
		Value *v = properties.e[c]->GetData();
		if (!v) return -1; //don't allow null inputs
		out->Push(v, 0);
	}

	return NodeBase::Update();
}


//------------------------ NumberListNode ------------------------

/*! \class NumberListNode
 * Make a set of numbers.
 */

class NumberListNode : public NodeBase
{
  public:
	NumberListNode();
	virtual ~NumberListNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	
	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new NumberListNode(); }
};


NumberListNode::NumberListNode()
{
	makestr(Name, _("Number list"));
	makestr(type,   "Lists/NumberList");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Start", new DoubleValue(1),1,  _("Start"), NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "End",   new DoubleValue(10),1, _("End"),   NULL));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Step",  new DoubleValue(1),1,  _("Step"),  NULL));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",  nullptr,1, _("out"),  NULL, 0, false));
}

NumberListNode::~NumberListNode()
{
}

NodeBase *NumberListNode::Duplicate()
{
	NumberListNode *newnode = new NumberListNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int NumberListNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[1]->GetData(), nullptr)) return -1;
	if (!isNumberType(properties.e[2]->GetData(), nullptr)) return -1;
	return NodeBase::GetStatus(); //default checks mod times
}

int NumberListNode::Update()
{
	Error(nullptr);

	int isnum;
	double start = getNumberValue(properties.e[0]->GetData(), &isnum);  if (!isnum) return -1;
	double end   = getNumberValue(properties.e[1]->GetData(), &isnum);  if (!isnum) return -1;
	double step  = getNumberValue(properties.e[2]->GetData(), &isnum);  if (!isnum) return -1;

	if ((start>end && step>=0) || (start<end && step<=0)) {
		Error(_("Bad step value"));
		return -1;
	}

	if (fabs((end-start)/step) > 2000) {
		Error(_("Too many values!"));
		return -1;
	}

	SetValue *out = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
	if (!out) {
		out = new SetValue();
		properties.e[properties.n-1]->SetData(out, 1);
	} else properties.e[properties.n-1]->Touch();
	
	int i = 0;
	for (double c = start; (start < end && c < end) || (start > end && c > end); c += step) {
		DoubleValue *v = nullptr;
		if (i < out->n()) v = dynamic_cast<DoubleValue*>(out->e(i));
		if (!v) {
			v = new DoubleValue(c);
			if (i < out->n()) out->Set(i, out, 1);
			else out->Push(v, 1);
		} else v->d = c;
		i++;
	}
	while (out->n() > i) out->Remove(i);

	return NodeBase::Update();
}


//----------------------- ShuffleNode ------------------------

/*! \class ShuffleNode
 *
 * Shuffle a set.
 */
class ShuffleNode : public NodeBase
{
  public:
	ShuffleNode();
	virtual ~ShuffleNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ShuffleNode(); }
};

ShuffleNode::ShuffleNode()
{
	makestr(type, "Lists/Shuffle");
	makestr(Name, _("Shuffle"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in",     NULL,1,  _("Set"), _("Either a Set or a PointSet")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL,1, _("Out"), NULL,0, false));
}

ShuffleNode::~ShuffleNode()
{
}

NodeBase *ShuffleNode::Duplicate()
{
	ShuffleNode *newnode = new ShuffleNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int ShuffleNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return -1;
	if (v->type() != VALUE_Set && v->type() != PointSetValue::TypeNumber()) return -1;
	return NodeBase::GetStatus();
}

int ShuffleNode::Update()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return -1;
	
	SetValue *set = dynamic_cast<SetValue*>(v);
	if (set) {
		SetValue *out = dynamic_cast<SetValue*>(properties.e[1]->GetData());
		if (!out) {
			out = new SetValue();
			properties.e[1]->SetData(out, 1);
		}

		int n = set->n();
		for (int c=0; c<n; c++) {
			if (c > out->n()) out->Push(set->e(c), 0);
			else out->Set(c, set->e(c), 0);
		}

		int i;
		for (int c=n-1; c>0; c--) {
			i = c * (double)random()/RAND_MAX;
			out->values.swap(c,i);
		}

	} else {
		PointSetValue *pset = dynamic_cast<PointSetValue*>(v);
		if (!pset) return -1;
		PointSetValue *out = dynamic_cast<PointSetValue*>(properties.e[1]->GetData());
		if (!out) {
			out = new PointSetValue();
			properties.e[1]->SetData(out, 1);
		}

		out->CopyFrom(pset, 1, 0);
		out->Shuffle();
	}

	properties.e[1]->Touch();
	return NodeBase::Update();
}


//------------ ObjectNode

/*! \class ObjectNode
 * Holds a DrawableObject as a source or output.
 */

ObjectNode::ObjectNode(int for_out, DrawableObject *nobj, int absorb)
{
	is_out = for_out;

	makestr(Name, _("Object"));
	makestr(type, is_out ? "ObjectOut" : "ObjectIn");

	if (is_out) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "out", nobj, absorb, NULL, NULL, 0, false)); 
	} else {
		AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", NULL,0, NULL, 0, false)); 
	}
}

ObjectNode::~ObjectNode()
{
}

NodeBase *ObjectNode::Duplicate()
{
	ObjectNode *newnode = new ObjectNode(is_out, dynamic_cast<DrawableObject*>(properties.e[0]->GetData()), 0);
	newnode->DuplicateBase(this);
	return newnode;
}

int ObjectNode::GetStatus()
{
	//Value *obj = properties.e[0]->GetData();
	//if (!dynamic_cast<DrawableObject*>(obj)) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ObjectNode::Update()
{
	return NodeBase::Update();
}

Laxkit::anObject *newObjectInNode(int p, Laxkit::anObject *ref)
{
	return new ObjectNode(0, NULL,0);
}

Laxkit::anObject *newObjectOutNode(int p, Laxkit::anObject *ref)
{
	return new ObjectNode(1, NULL,0);
}

int ObjectNode::UpdatePreview()
{
	// *** copy the object's preview
	//***
	return 1;
}




//------------ TransformAffineNode

class TransformAffineNode : public NodeBase
{
  public:
	TransformAffineNode();
	virtual ~TransformAffineNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	Value *TransformData(Affine *affine, Value *v, Value *old_out);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new TransformAffineNode(); }
};

TransformAffineNode::TransformAffineNode()
{
	makestr(Name, _("Transform Affine"));
	makestr(type, "Filters/TransformAffine");

	Value *v = new AffineValue();
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", NULL,0, _("In"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Affine", v,1, _("Affine"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,0, _("Out"), nullptr, 0, false)); 
}

TransformAffineNode::~TransformAffineNode()
{
}

NodeBase *TransformAffineNode::Duplicate()
{
	TransformAffineNode *newnode = new TransformAffineNode();
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int TransformAffineNode::GetStatus()
{
	//make sure transform is affine
	Affine *affine  = dynamic_cast<Affine*>(properties.e[1]->GetData());
	if (!affine) return -1;

	// in can be a vector, or an affine
	Value *v = properties.e[0]->GetData();
	if (v) {
		int vtype = v->type();
		if (   vtype != AffineValue::TypeNumber()
			&& vtype != VALUE_Flatvector
			&& vtype != VALUE_Set
			&& vtype != PointSetValue::TypeNumber()
			&& !dynamic_cast<Affine*>(v)
		   ) return -1;
	}

	if (v && !properties.e[2]->data) return 1; //just needs update

	return NodeBase::GetStatus(); //default checks mod times
}

Value *TransformAffineNode::TransformData(Affine *affine, Value *v, Value *old_out)
{
	int vtype = v->type();
	Value *nv = nullptr;

	if (vtype == VALUE_Flatvector) {
		FlatvectorValue *fv  = dynamic_cast<FlatvectorValue*>(v);
		FlatvectorValue *nnv = dynamic_cast<FlatvectorValue*>(old_out);
		if (!nnv) nnv = new FlatvectorValue(affine->transformPoint(fv->v));
		else {
			nnv->v = affine->transformPoint(fv->v);
			nnv->inc_count();
		}
		nv = nnv;

	} else if (dynamic_cast<DrawableObject*>(v)) {
		// ***** refs cause unending render loops
//			DrawableObject *d = dynamic_cast<DrawableObject*>(v);
//			LSomeDataRef *ref = dynamic_cast<LSomeDataRef*>(properties.e[2]->GetData());
//			if (!ref) ref = new LSomeDataRef(d);
//			else {
//				ref->Set(d, 0);
//				ref->inc_count();
//			}
//			ref->Multiply(*affine);
//			nv = ref;
		//---------------------
		DrawableObject *d = dynamic_cast<DrawableObject*>(v);
		anObject *filter = d->filter;
		d->filter = NULL;
		DrawableObject *copy = dynamic_cast<DrawableObject*>(d->duplicate());
		copy->FindBBox();
		d->filter = filter;
		copy->Multiply(*affine);
		nv = copy;

	} else if (vtype == AffineValue::TypeNumber()) {
		AffineValue *fv  = dynamic_cast<AffineValue*>(v);
		AffineValue *nnv = dynamic_cast<AffineValue*>(old_out);
		if (!nnv) nnv = new AffineValue(fv->m());
		else {
			nnv->m(fv->m());
			nnv->inc_count();
		}
		nnv->Multiply(*affine);
		nv = nnv;

	} else if (vtype == PointSetValue::TypeNumber()) {
		nv = v->duplicate();
		PointSetValue *pv = dynamic_cast<PointSetValue*>(nv);
		pv->Map([&](const flatpoint &pin, flatpoint &pout) { pout = affine->transformPoint(pin); return 1; } );

	} else if (vtype == VALUE_Set) {
		SetValue *sv = dynamic_cast<SetValue*>(v);
		SetValue *sout = dynamic_cast<SetValue*>(old_out);
		nv = sout;
		if (!sout) {
			sout = new SetValue();
			properties.e[2]->SetData(sout, true);
		} else {
			sout->Flush();
			nv->inc_count();
		}

		for (int c=0; c<sv->n(); c++) {
			Value *nnv = TransformData(affine, sv->e(c), nullptr);
			if (!nnv) return nullptr;
			sout->Push(nnv, 1);
		}

	} else {
		Error(_("Cannot transform that"));
		return nullptr;
	}

	return nv;
}

int TransformAffineNode::Update()
{
	ClearError();

	Value *nv = nullptr;
	Value *v = properties.e[0]->GetData();
	if (!v) {
		return -1;
	}

	Value *aff = properties.e[1]->GetData();
	DBG cerr << "TransformAfffineNode::Update with prop 1 type: "<<(aff ? aff->whattype() : "null")<<endl;
	Affine *affine  = dynamic_cast<Affine*>(aff);
	if (!affine) {
		Error(_("Transform must be affine"));
		return -1;
	} else if (!affine->IsInvertible()) {
		Error(_("Invalid affine"));
		return -1;
	}

	Value *old_out = properties.e[2]->GetData();
	nv = TransformData(affine, v, old_out);
	if (!nv) {
		return -1;
	}

	properties.e[2]->SetData(nv, 1);

	return NodeBase::Update();
}



//------------------------ CurveProperty ------------------------

/*! \class CurveProperty
 * For use in CurveNode.
 */


SingletonKeeper CurveProperty::interfacekeeper;

LaxInterfaces::CurveMapInterface *CurveProperty::GetCurveInterface()
{
	return dynamic_cast<CurveMapInterface*>(interfacekeeper.GetObject());
}

CurveProperty::CurveProperty(CurveValue *ncurve, int absorb, int isout)
{
	type = (isout ? NodeProperty::PROP_Output : NodeProperty::PROP_Block);
	is_linkable = isout ? true : false;
	makestr(name, "Curve");
	makestr(label, _("Curve"));
	//makestr(tooltip, _(""));
	SetFlag(NODES_PropResize, true);

	curve = ncurve;
	if (curve && !absorb) curve->inc_count();
	data = curve;
	if (data) data->inc_count();

	CurveMapInterface *interface = GetCurveInterface();
	if (!interface) {
		interface = new CurveMapInterface(getUniqueNumber(), NULL);
		makestr(interface->owner_message, "CurveChange");
		interface->style |= CurveMapInterface::RealSpace | CurveMapInterface::Expandable;
		interfacekeeper.SetObject(interface, 1);
	}

}

CurveProperty::~CurveProperty()
{
	DBG cerr <<"...deleting curve prop data"<<endl;
	if (curve) curve->dec_count();
}

/*! Set a default width and height based on colors->font->textheight().
 */
void CurveProperty::SetExtents(NodeColors *colors)
{
	width = height = 4 * colors->font->textheight();
}

bool CurveProperty::HasInterface()
{
	return true;
}

/*! If interface!=NULL, then try to use it. If NULL, create and return a new appropriate interface.
 * Also set the interface to use *this.
 */
LaxInterfaces::anInterface *CurveProperty::PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp)
{
	CurveMapInterface *interf = NULL;
	if (interface) {
		if (strcmp(interface->whattype(), "CurveMapInterface")) return NULL;
		interf = dynamic_cast<CurveMapInterface*>(interface);
		if (!interf) return NULL; //wrong ref interface!!
	}

	if (!interf) interf = GetCurveInterface();
	interf->Dp(dp);
	interf->SetInfo(curve);
	double th = owner->colors->font->textheight();
	interf->SetupRect(owner->x + x + th/2, owner->y + y, width-th, height);

	return interf;
}

void CurveProperty::Draw(Laxkit::Displayer *dp, int hovered)
{
	anInterface *interf = PropInterface(NULL, dp);
	if (hovered) interf->Refresh();
	else interf->DrawData(curve);
}


/*! Whether to accept link to this value type.
 * Accepts DoubleValue, ImageValue, or ColorValue.
 */
bool CurveProperty::AllowType(Value *v)
{
	int vtype = v->type();
	if (vtype == VALUE_Real)  return true;
	if (vtype == VALUE_Color) return true;
	if (vtype == VALUE_Image) return true;
	// *** vectors, transform each value individually, or all
	return false;
}


//--------------------- CurveNode ---------------------

/*! \class CurveNode
 */
class CurveNode : public NodeBase
{
  public:
	bool for_creation; //true when creating a curve, not a transforming node

	ObjectDef channels;
	Laxkit::RefPtrStack<CurveValue> curves;

	int current_curve;
	int use_max; //whether curve affects the maximum channel, like when it's an alpha channel

	CurveNode(int is_for_creation, CurveValue *ncurve);
	virtual ~CurveNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att, LaxFiles::DumpContext *context);
};

/*! absorbs ncurve.
 */
CurveNode::CurveNode(int is_for_creation, CurveValue *ncurve)
{
	if (is_for_creation) {
		makestr(Name, _("Curve"));
		makestr(type, "Curve");
	} else {
		makestr(Name, _("Curve Transform"));
		makestr(type, "CurveTransform");
	}

	for_creation = is_for_creation;

	//in should accept:
	//  doubles, assume adjust within range 0..1
	//  image data, create one curve for all except alpha, one curve for each channel
	//
	//out should be same format as in

	CurveValue *curve = ncurve;
	if (!curve) curve = new CurveValue();
	curves.push(curve);
	if (!ncurve) curve->dec_count();
	current_curve = 0;

	if (!for_creation) AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "In", NULL, 1));

	AddProperty(new CurveProperty(curve,0, 1));

	if (!for_creation) {
		 //create selector for channel, including "all"
		//AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "Channel", new EnumValue(0, &channels), 1));
		AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL, 0));
	}
}

CurveNode::~CurveNode()
{
}

NodeBase *CurveNode::Duplicate()
{
	CurveValue *v = dynamic_cast<CurveProperty*>(FindProperty("Curve"))->curve;
	v = dynamic_cast<CurveValue*>(v->duplicate());
	CurveNode *newnode = new CurveNode(for_creation, v);
	newnode->DuplicateBase(this);
	newnode->Wrap();
	return newnode;
}

/*! Return 1 for set, 0 for not set.
 */
int CurveNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att, LaxFiles::DumpContext *context)
{
	if (!strcmp(propname, "Curve")) {
		CurveProperty *prop = dynamic_cast<CurveProperty*>(FindProperty(propname));
		CurveValue *curve = prop->curve;
		curve->dump_in_atts(att,0,NULL);
		prop->SetData(curve, 0);
	}
	
	return NodeBase::SetPropertyFromAtt(propname, att, context);
}

int CurveNode::Update()
{ // ***
//	if (!properties.e[1]->IsConnected()) {
//		 //no connected value, just use 0
//		Value *outdata = properties.e[2]->GetData();
//		DoubleValue *d = dynamic_cast<DoubleValue*>(outdata);
//
//		if (!d) {
//			if (outdata) { outdata->dec_count(); outdata=NULL; }
//			outdata = d = new DoubleValue();
//		}
//		d->d = 0;
//		return 0;
//	}

	return NodeBase::Update();
}

int CurveNode::GetStatus()
{
	return NodeBase::GetStatus();
}
 

Laxkit::anObject *newCurveNode(int p, Laxkit::anObject *ref)
{
	return new CurveNode(p, NULL);
}


//------------------------ GradientProperty ------------------------

/*! \class GradientProperty
 */


SingletonKeeper GradientProperty::interfacekeeper;

GradientProperty::GradientProperty(GradientValue *ngradient, int absorb, int isout)
{
	type = (isout ? NodeProperty::PROP_Output : NodeProperty::PROP_Block);
	is_linkable = isout ? true : false;
	makestr(name, "gradient");
	makestr(label, _("Gradient"));
	//makestr(tooltip, _(""));
	SetFlag(NODES_PropResize, true);
	is_editable = true;

	// gradient = ngradient;
	// if (gradient && !absorb) gradient->inc_count();
	// data = dynamic_cast<Value*>(ngradient);
	data = ngradient;
	if (data) data->inc_count();

	ginterf = nullptr;
}

GradientProperty::~GradientProperty()
{
	DBG cerr <<"...deleting gradient prop data"<<endl;
	// if (gradient) gradient->dec_count();
	if (ginterf) ginterf->dec_count();
}

/*! Set a default width and height based on colors->font->textheight().
 */
void GradientProperty::SetExtents(NodeColors *colors)
{
	width = height = 4 * colors->font->textheight();
}

bool GradientProperty::HasInterface()
{
	return true;
}

/*! Static function to get an approprate interface for GradientProperty.
 */
LaxInterfaces::GradientInterface *GradientProperty::GetGradientInterface()
{
	if (ginterf) return ginterf;

	GradientInterface *interf = dynamic_cast<GradientInterface*>(interfacekeeper.GetObject());
	if (!interf) {
		interf = new GradientInterface(getUniqueNumber(), NULL);
		makestr(interf->owner_message, "GradientChange");
		interf->style |= GradientInterface::RealSpace | GradientInterface::RealSpaceMouse | GradientInterface::Expandable | GradientInterface::EditStrip;
		interfacekeeper.SetObject(interf, 1);
	}

	ginterf = dynamic_cast<GradientInterface*>(interf->duplicate(nullptr));
	ginterf->showdecs |= GradientInterface::ShowColors;
	makestr(ginterf->owner_message, "GradientChange");
	return ginterf;
}

/*! If interface!=NULL, then try to use it. If NULL, create and return a new appropriate interface.
 * Also set the interface to use *this.
 * If supplied interface is the wrong type, return null.
 */
LaxInterfaces::anInterface *GradientProperty::PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp)
{
	GradientInterface *interf = NULL;
	if (interface) {
		if (strcmp(interface->whattype(), "GradientInterface")) return NULL;
		interf = dynamic_cast<GradientInterface*>(interface);
		if (!interf) return NULL; //wrong ref interface!!
	}

	if (!interf) interf = GetGradientInterface();

	interf->Dp(dp);
	interf->UseThisObject(dynamic_cast<GradientStrip*>(data));
	double th = owner->colors->font->textheight();
	interf->SetupRect(owner->x + x + th, owner->y + y, width-2*th, height-th/2);

	return interf;
}

void GradientProperty::Draw(Laxkit::Displayer *dp, int hovered)
{
	anInterface *interf = PropInterface(NULL, dp);
	if (hovered) interf->Refresh();
	else interf->Refresh();
	//else interf->DrawData(gradient);
}


///*! Whether to accept link to this value type.
// * Accepts DoubleValue, ImageValue, or ColorValue.
// */
//bool GradientProperty::AllowType(Value *v)
//{
//	int vtype = v->type();
//	if (vtype == VALUE_Real)  return true;
//	if (vtype == VALUE_Color) return true;
//	if (vtype == VALUE_Image) return true;
//	// *** vectors, transform each value individually, or all
//	return false;
//}



//--------------------- GradientNode ---------------------

/*! \class GradientNode
 */
class GradientNode : public NodeBase
{
  public:
	bool for_creation; //true when creating a curve, not a transforming node

	GradientNode(int is_for_creation, GradientValue *ngradient);
	virtual ~GradientNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	// virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att, LaxFiles::DumpContext *context);
};

Laxkit::anObject *newGradientNode(int p, Laxkit::anObject *ref)
{
	return new GradientNode(1, nullptr);
}

Laxkit::anObject *newGradientTransformNode(int p, Laxkit::anObject *ref)
{
	return new GradientNode(0, nullptr);
}

/*! absorbs ngradient.
 */
GradientNode::GradientNode(int is_for_creation, GradientValue *ngradient)
{
	if (is_for_creation) {
		makestr(Name, _("Gradient"));
		makestr(type, "Gradient");
	} else {
		makestr(Name, _("Gradient Transform"));
		makestr(type, "GradientTransform");
	}

	for_creation = is_for_creation;

	//in should accept:
	//  doubles, assume adjust within range 0..1
	//  image data, create one curve for all except alpha, one curve for each channel
	//
	//out should be same format as in

	GradientValue *gradient = ngradient;
	if (!gradient) gradient = new GradientValue();

	if (!for_creation) AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "In", new DoubleValue(0), 1));

	AddProperty(new GradientProperty(gradient,0, for_creation));
	if (!ngradient) gradient->dec_count();

	if (!for_creation) {
		 //create selector for channel, including "all"
		//AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "Channel", new EnumValue(0, &channels), 1));
		AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", NULL, 0));
	}
}

GradientNode::~GradientNode()
{
}

NodeBase *GradientNode::Duplicate()
{
	GradientProperty *prop = dynamic_cast<GradientProperty*>(FindProperty("gradient"));
	GradientValue *v = prop ? dynamic_cast<GradientValue*>(prop->data) : nullptr;
	if (v) v = dynamic_cast<GradientValue*>(v->duplicate());
	GradientNode *newnode = new GradientNode(for_creation, v);
	newnode->DuplicateBase(this);
	newnode->Wrap();
	return newnode;
}

// /*! Return 1 for set, 0 for not set.
//  */
// int GradientNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att, LaxFiles::DumpContext *context)
// {
// 	int status = NodeBase::SetPropertyFromAtt(propname, att, context);
// 	return status;
// }

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int GradientNode::Update()
{
	Error(nullptr);

	if (for_creation) {
		return NodeBase::Update();
	}

	// properties.e[properties.n-1]->SetData(properties.e[0]->GetData(), 0);
	
	//float -> color
	//color -> grayscale -> color
	//image -> grayscale -> new image
	
	GradientValue *gv = dynamic_cast<GradientValue*>(properties.e[1]->GetData());
	Value *in = properties.e[0]->GetData();
	if (!in) return -1;

	double d;
	bool isnum = false;
	SetValue *set = nullptr;
	NumStack<double> vals;
	
	if (in->type() == VALUE_Set) {
		set = dynamic_cast<SetValue*>(in);
		int num = 0;
		for (int c=0; c<set->n(); c++) {
			Value *v = set->e(c);
			if (!v) {
				vals.push(0);
				continue;
			}
			isnum = false;
			d = 0;
			if (isNumberType(in, &d)) isnum = true;
			else if (dynamic_cast<ColorValue*>(in)) {
				ColorValue *cv = dynamic_cast<ColorValue*>(in);
				d = simple_rgb_to_grayf(cv->color.Red(), cv->color.Green(), cv->color.Blue());
				isnum = true;
			}
			vals.push(d);
			if (isnum) num++;
		}
		if (num == 0) {
			Error(_("Set contains no numbers"));
			return -1;
		}
	} else {
		d = 0;
		if (isNumberType(in, &d)) isnum = true;
		else if (dynamic_cast<ColorValue*>(in)) {
			ColorValue *cv = dynamic_cast<ColorValue*>(in);
			d = simple_rgb_to_grayf(cv->color.Red(), cv->color.Green(), cv->color.Blue());
			isnum = true;
		} else {
			return -1;
		}
	}

	ColorValue *cv = nullptr;
	SetValue *outset = nullptr;
	if (set) {
		outset = dynamic_cast<SetValue*>(properties.e[2]->GetData());
		if (!outset) {
			outset = new SetValue();
			properties.e[2]->SetData(outset,1);
		}
	}
	for (int c=0; c < (set ? vals.n : 1); c++) {
		if (set) d = vals.e[c];
		ScreenColor col;
		gv->WhatColor(d, &col, true);

		if (set) {
			cv = new ColorValue();
			outset->Push(cv, 1);
		} else {
			cv = dynamic_cast<ColorValue*>(properties.e[2]->GetData());
			if (!cv) {
				cv = new ColorValue();
				properties.e[2]->SetData(cv,1);
			}
		}
		cv->color.SetRGB(col.Red(), col.Green(), col.Blue());
	}

	properties.e[2]->Touch();
	return NodeBase::Update();
}

int GradientNode::GetStatus()
{
	return NodeBase::GetStatus();
}
 


//------------------------ NewObjectNode ------------------------


class NewObjectNode : public NodeBase
{
  public:
	NewObjectNode(const char *what);
	virtual ~NewObjectNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new NewObjectNode(nullptr); }
	static SingletonKeeper menuKeeper;
	static ObjectDef *GetObjectList();

};

SingletonKeeper NewObjectNode::menuKeeper;

/*! Return a menu for known objects from Laidout and Core calculator modules.
 * Calling code must dec_count() the returned def.
 */
ObjectDef *NewObjectNode::GetObjectList()
{
	ObjectDef *def = dynamic_cast<ObjectDef*>(menuKeeper.GetObject());
	if (!def) {
		def = new ObjectDef("Objects", _("Objects"), NULL,NULL,"enum", 0);

		int i = 0;
		// InterfaceManager *imanager=InterfaceManager::GetDefault(true); // *** can't use this since these aren't Value derived!! aaaarg!
	 //    ObjectFactory *lobjectfactory = imanager->GetObjectFactory();
	 //    for (int c = 0; c < lobjectfactory->NumTypes(); c++) {
	 //    	const char *tstr = lobjectfactory->TypeStr(c);
	 //    	def->pushEnumValue(tstr, nullptr, nullptr);
	 //    	if (what && !strcmp(tstr, what)) whati = i;
	 //    	DBG cerr << "NewObjectNode: adding from imanager: "<<tstr<<endl;
	 //    	i++;
	 //    }

		//grab from the Laidout namespace
		for (int c = 0; c < stylemanager.getNumFields(); c++) {
			ObjectDef *d = stylemanager.getField(c);
			if (d->format == VALUE_Class)	{
				// DBG if (!d->newfunc && !d->stylefunc) cerr << "NewObjectNode: *** Warning! Missing constructor for "<<d->name<<". FIXME!!"<<endl;
				// DBG else cerr << "NewObjectNode:  newfunc: "<<(d->newfunc ? "yes": "no ")<<" ofunc: "<<(d->stylefunc? "yes  ": "no   ")<<d->name<<endl;
				def->pushEnumValue(d->name, d->Name, d->description, i);
				i++;
			}
		}

		//grab from the Core namespace
		ObjectDef *core = laidout->calculator->FindModule("Core");
		for (int c = 0; c < core->getNumFields(); c++) {
			ObjectDef *d = core->getField(c);
			if (d->format == VALUE_Class)	{
				// DBG if (!d->newfunc && !d->stylefunc) cerr << "NewObjectNode: *** Warning! Missing constructor for "<<d->name<<". FIXME!!"<<endl;
				// DBG else cerr << "NewObjectNode:  newfunc: "<<(d->newfunc ? "yes": "no ")<<" ofunc: "<<(d->stylefunc? "yes  ": "no   ")<<d->name<<endl;
				def->pushEnumValue(d->name, d->Name, d->description, i);
				i++;
			}
		}

		def->Sort();
		
		menuKeeper.SetObject(def, false);
	} else def->inc_count();
	return def;
}

NewObjectNode::NewObjectNode(const char *what)
{
	makestr(Name, _("New Object"));
	makestr(type, "NewObject");

	// enum of all objects	
	def = GetObjectList();
	int whati = 0;
	if (what) {
		for (int c=0; c<def->getNumFields(); c++) {
			if (!strcmp(def->name, what)) {
				whati = c;
				break;
			}
		}
	}
	
	EnumValue *e = new EnumValue(def, whati);
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "what", e,1, "", NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Object"), NULL,0, false));
}

NewObjectNode::~NewObjectNode()
{
}

NodeBase *NewObjectNode::Duplicate()
{
	const char *what = nullptr;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	if (ev && ev->GetObjectDef()) {
		ev->GetObjectDef()->getEnumInfo(ev->value, &what);
	}

	NewObjectNode *node = new NewObjectNode(what);
	node->DuplicateBase(this);
	return node;
}

//0 ok, -1 bad ins, 1 just needs updating
int NewObjectNode::GetStatus()
{
	if (!properties.e[1]->GetData()) return 1;
	return NodeBase::GetStatus();
}

int NewObjectNode::Update()
{
	// if enum not equal value->whattype, recreate
	const char *what = nullptr;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	if (ev && ev->GetObjectDef()) {
		ev->GetObjectDef()->getEnumInfo(ev->value, &what);
	}

	if (!what) return -1;

	Value *o = properties.e[1]->GetData();
	ObjectDef *odef = o ? o->GetObjectDef() : nullptr;
	if (!o || (o && strcmp(what, odef->name))) { //missing or different out, recreate
		ObjectDef *d = stylemanager.FindDef(what);
		if (!d) {
			ObjectDef *core = laidout->calculator->FindModule("Core");
			d = core->FindDef(what);
		}
		if (d) {
			if (d->newfunc) o = d->newfunc();
			else if (d->stylefunc) {
				cerr << "Warning! "<< d->name<<" missing newfunc! FIXME!! attempting to use objectfunc."<<endl;
				ErrorLog log;
				d->stylefunc(nullptr, nullptr, &o, log);
						
			} else {
				cerr << "Warning! "<<d->name<<" has no constructor! FIXME!!" <<endl;
			}
		// } else {
		// 	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	 //    	ObjectFactory *lobjectfactory = imanager->GetObjectFactory();
		// 	int i = lobjectfactory->FindType(what);
		// 	if (i >= 0) {
		// 		anObject *ao = lobjectfactory->NewObject(what);
		// 		if (ao) {
		// 			o = dynamic_cast<Value*>(ao);
		// 			if (!o) ao->dec_count();
		// 		}
		// 	}
		}
		properties.e[1]->SetData(o,1);

	} else properties.e[1]->Touch();;

	if (!properties.e[1]->GetData()) return -1;
	return NodeBase::Update();
}


//------------------------ ListOfNode ------------------------


class ListOfNode : public NodeBase
{
  public:
	ListOfNode(const char *what, int n);
	virtual ~ListOfNode();

	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();
	virtual int UpdateProps(int num);
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ListOfNode(nullptr, 1); }
};

ListOfNode::ListOfNode(const char *what, int n)
{
	makestr(Name, _("List Of"));
	makestr(type, "Lists/List");

	if (isblank(what)) what = "real";

	// enum of all objects	
	def = NewObjectNode::GetObjectList();
	int whati = 0;
	if (what) {
		const char *nm;
		for (int c=0; c<def->getNumEnumFields(); c++) {
			def->getEnumInfo(c, &nm);
			if (nm && !strcmp(nm, what)) {
				whati = c;
				break;
			}
		}
	}
	
	EnumValue *e = new EnumValue(def, whati);
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "what", e,1, "", NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "num", new IntValue(n),1, _("Num"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Set"), NULL,0, false));
}

ListOfNode::~ListOfNode()
{
}

NodeBase *ListOfNode::Duplicate()
{
	const char *what = nullptr;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	if (ev && ev->GetObjectDef()) {
		ev->GetObjectDef()->getEnumInfo(ev->value, &what);
	}

	int isnum;
	int num = getIntValue(properties.e[1]->GetData(), &isnum);
	if (num < 0) num = 1;

	ListOfNode *node = new ListOfNode(what, num);
	node->DuplicateBase(this);
	return node;
}

//0 ok, -1 bad ins, 1 just needs updating
int ListOfNode::GetStatus()
{
	return NodeBase::GetStatus();
}

void ListOfNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att,flag,context);
	int i = -1;
	NodeProperty *out = FindProperty("out", &i);
	if (out && i != properties.n-1) {
		properties.slide(i, properties.n-1);
	}
}

int ListOfNode::UpdateProps(int num)
{
	// make sure we have the right number of extra props
	// do NOT disconnect any extra automatically
	// does not check existing data types, that is done in Update
	
	if (properties.n == num+3) return 1;

	char scratch[20], scratch2[20];
	while (properties.n < num+3) {
		sprintf(scratch, "el%d", properties.n-3);
		sprintf(scratch2, "%d", properties.n-3);
		NodeProperty *prop = new NodeProperty(NodeProperty::PROP_Input, true, scratch, NULL,0, scratch2, nullptr, false, true);
		AddProperty(prop, properties.n-1);
	}

	for (int c=2+num; c<properties.n-1; c++)
		if (properties.e[c]->IsConnected()) return 0; //force users to unconnect elements more than num

	while (properties.n > num+3) {
		RemoveProperty(properties.e[properties.n-2]);
	}
	
	return 1;
}

int ListOfNode::Update()
{
	ClearError();

	// if enum not equal value->whattype, recreate
	const char *what = nullptr;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	if (ev && ev->GetObjectDef()) {
		ev->GetObjectDef()->getEnumInfo(ev->value, &what);
	}

	if (!what) return -1;

	int isnum = 0;
	int num = getIntValue(properties.e[1]->GetData(), &isnum);
	if (!isnum || num < 0) {
		Error(_("Invalid number!"));
		return -1;
	}

	SetValue *out = dynamic_cast<SetValue*>(properties.e[properties.n-1]->GetData());
	if (!out || out->type() != VALUE_Set) {
		out = new SetValue();
		properties.e[properties.n-1]->SetData(out, 1);
	}

	// bool needtowrap = false;
	if (num != properties.n-3) {
		if (!UpdateProps(num)) { //3 == -out-type-num
			Wrap();
			Error(_("Please unlink old data"));
			return -1;
		}
		Wrap();
		// needtowrap = true;
	}

	for (int c=0; c<num; c++) {
		if (c+2 >= properties.n-1) {
			Error(_("Aaaaaaaa!! FIXME"));
			return -1;
		}
		Value *e = properties.e[c+2]->GetData();
		ObjectDef *odef = e ? e->GetObjectDef() : nullptr;
		if (!e || strcmp(what, odef->name)) {
			//wrong type!
			if (properties.e[c+2]->IsConnected()) {
				Error(_("In object wrong type!"));
				return -1;
			}

			ObjectDef *d = stylemanager.FindDef(what);
			if (!d) {
				ObjectDef *core = laidout->calculator->FindModule("Core");
				d = core->FindDef(what);
			}
			if (d) {
				if (d->newfunc) e = d->newfunc();
				else if (d->stylefunc) {
					cerr << "Warning! "<< d->name<<" missing newfunc! FIXME!! attempting to use objectfunc."<<endl;
					ErrorLog log;
					d->stylefunc(nullptr, nullptr, &e, log);
							
				} else {
					cerr << "Warning! "<<d->name<<" has no constructor! FIXME!!" <<endl;
				}
			}

			properties.e[c+2]->SetData(e,1);
		}
		if (c < out->n()) {
			if (e != out->e(c)) out->Set(c, e, 0);
		} else out->Push(e, 0);
	}
	while (out->n() > num) out->Remove(out->n()-1);

	properties.e[properties.n-1]->Touch();
	// if (needtowrap) Wrap();
	return NodeBase::Update();
}



//------------ ObjectInfoNode


/*! \class ObjectInfoNode
 * Class to expose any properties in a Value's ObjectDef.
 */
class ObjectInfoNode : public NodeBase
{
  public:
	Value *obj; // we keep this around so as to not dup properties unnecessarily

	// ObjectInfoNode(DrawableObject *nobj, int absorb);
	ObjectInfoNode(Value *nobj, int absorb);
	virtual ~ObjectInfoNode();
	virtual int Update();
	virtual void UpdateProps();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int Connected(NodeConnection *connection);
	virtual int Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
	virtual Value *PreviewFrom() { return properties.e[0]->GetData(); }
	// virtual int UpdatePreview();
	
	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ObjectInfoNode(nullptr, 0); }
};

ObjectInfoNode::ObjectInfoNode(Value *nobj, int absorb)
{
	makestr(type, "ObjectInfoNode");
	makestr(Name, _("Object Info"));

	obj = nobj;
	if (obj) {
		if (!absorb) obj->inc_count();
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in", obj,0, _("In"), NULL, 0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "id", NULL,1, _("Id"), NULL, 0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "type", NULL,1, _("Type"), NULL, 0,false));
	
	if (obj) UpdateProps();
	Update();
}

ObjectInfoNode::~ObjectInfoNode()
{
	if (obj) obj->dec_count();
}

NodeBase *ObjectInfoNode::Duplicate()
{
	NodeBase *newnode = new ObjectInfoNode(nullptr,0);
	newnode->DuplicateBase(this);
	return newnode;
}

int ObjectInfoNode::Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced)
{
	//*** handle disconnecting in how?
	return NodeBase::Disconnected(connection, from_will_be_replaced, to_will_be_replaced);
}

int ObjectInfoNode::Connected(NodeConnection *connection)
{
	if (this == connection->to && connection->toprop == properties.e[0]) {
		// we just connected something to in slot
		Value *nobj = connection->fromprop->GetData();
		
		if (nobj != obj) {
			if (obj) obj->dec_count();
			obj = nobj;
			if (obj) obj->inc_count();

			UpdateProps();
		}
	}

	return NodeBase::Connected(connection);
}

// int ObjectInfoNode::UpdatePreview()
// {
// 	Previewable *obj = dynamic_cast<Previewable*>(properties.e[0]->GetData());
// 	LaxImage *img = nullptr;
// 	if (obj) img = obj->GetPreview();
// 	if (img) {
// 		if (img != total_preview) {
// 			if (total_preview) total_preview->dec_count();
// 			total_preview = img;
// 			total_preview->inc_count();
// 		}
// 	} else {
// 		if (total_preview) total_preview->dec_count();
// 		total_preview = nullptr;
// 	}
// 	return 1;
// }

/*! Update the prop field arrangement. Does NOT update the actual values here. Use the usual Update() for that.
 */
void ObjectInfoNode::UpdateProps()
{
	int added = 0;

	if (obj) { //only update props when non-null obj
		// obj->inc_count();
		ObjectDef *def = obj->GetObjectDef(); //warning: shawdows NodeBase::def
		
		ObjectDef *field;

		for (int c=0; c<def->getNumFields(); c++) {
			field = def->getField(c);

			if (field->format==VALUE_Function || field->format==VALUE_Class || field->format==VALUE_Operator || field->format==VALUE_Namespace)
				continue;

			// update property with new name, Name, desc...
			NodeProperty *prop = nullptr;
			// Value *val = obj->dereference(field->name,-1);
			if (added+3 < properties.n) {
				// modify property
				prop = properties.e[added+3];
				makestr(prop->name, field->name);
				makestr(prop->label, field->Name);
				makestr(prop->tooltip, field->description);
				// prop->SetData(val,1);

			} else {
				// insert new property
				prop = new NodeProperty(NodeProperty::PROP_Output,  true, field->name, nullptr,1, field->Name, field->description, 0, false);
				AddProperty(prop, added+3);
			}
			added++;
		}
	}

	// remove old extra properties
	for (int c = added+3; c<properties.n; ) {
		// if (properties.e[c]->IsConnected()) {
		// 	for (int c2=0; c2<properties.e[c]->connections.n; c2++) {
		// 		owner->Disconnect(properties.e[c]->connections.e[c2], false, false);
		// 	}
		// }
		RemoveProperty(properties.e[c]);
	}

	Wrap();
}

int ObjectInfoNode::Update()
{
	Value *o = properties.e[0]->GetData();
	if (!o) {
		properties.e[1]->SetData(nullptr, 0); //id
		// properties.e[2]->SetData(nullptr, 0); //type
		StringValue *sv = dynamic_cast<StringValue*>(properties.e[2]->GetData());
		if (!sv) {
			sv = new StringValue("null");
			properties.e[2]->SetData(sv, 1);
		} else {
			sv->Set("null");
		}
		UpdatePreview();
		Wrap();
		return -1;
	}

	if (o != obj) {
		bool doprops = true;
		if (obj && o && obj->GetObjectDef() == o->GetObjectDef()) doprops = false;

		if (obj) obj->dec_count();
		obj = o;
		obj->inc_count();

		if (doprops) UpdateProps();
	}

	// always set id
	StringValue *sv = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!sv) {
		sv = new StringValue(o->Id());
		properties.e[1]->SetData(sv, 1);
	} else {
		sv->Set(o->Id());
	}

	// always set type
	sv = dynamic_cast<StringValue*>(properties.e[2]->GetData());
	if (!sv) {
		sv = new StringValue(o->whattype());
		properties.e[2]->SetData(sv, 1);
	} else {
		sv->Set(o->whattype());
		properties.e[2]->Touch();
	}

	for (int c=3; c<properties.n; c++) {
		Value *v = o->dereference(properties.e[c]->Name(), -1);
		properties.e[c]->is_editable = false; // hack to recover from property read in not checking for editability
		properties.e[c]->SetData(v, 1);
	}

	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int ObjectInfoNode::GetStatus()
{
	if (!properties.e[0]->GetData()) return -1;
	return NodeBase::GetStatus();
}


//------------ ModifyObjectNode


/*! \class ModifyObjectNode
 * Class to allow changing any DrawableObject properties in its ObjectDef.
 */
class ModifyObjectNode : public NodeBase
{
  public:
	Value *obj; // we keep this around so as to not dup properties unnecessarily

	// ModifyObjectNode(DrawableObject *nobj, int absorb);
	ModifyObjectNode(Value *nobj, int absorb);
	virtual ~ModifyObjectNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int Connected(NodeConnection *connection);
	virtual int Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ModifyObjectNode(nullptr, 0); }
};

ModifyObjectNode::ModifyObjectNode(Value *nobj, int absorb)
{
	makestr(type, "ModifyObjectNode");
	makestr(Name, _("Modify Object"));

	obj = nobj;
	if (obj) {
		if (!absorb) obj->inc_count();
		def = obj->GetObjectDef();
	}
	if (def) def->inc_count();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", obj,0, _("In"), NULL, 0,false));

	if (def) {
		ObjectDef *field;
		bool isdrawable = (dynamic_cast<DrawableObject*>(obj) != nullptr && strcmp(obj->whattype(), "Group"));

		for (int c=0; c<def->getNumFields(); c++) {
			field = def->getField(c);

			if (field->format==VALUE_Function || field->format==VALUE_Class || field->format==VALUE_Operator || field->format==VALUE_Namespace)
				continue;

			if (isdrawable && ( // these are usually readonly, so special case skip.. need a better way to standardize readonly overrides
					!strcmp(field->name, "minx") || 
					!strcmp(field->name, "maxx") || 
					!strcmp(field->name, "miny") || 
					!strcmp(field->name, "maxy")
				 ))
				continue;

			//NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
			//			const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable=true);
			AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, field->name, NULL,1, field->Name, field->description, 0,true));
		}
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("Out"), NULL, 0,false));

	Update();
}

ModifyObjectNode::~ModifyObjectNode()
{
	if (obj) obj->dec_count();
}

NodeBase *ModifyObjectNode::Duplicate()
{
	NodeBase *newnode = new ModifyObjectNode(nullptr,0);
	newnode->DuplicateBase(this);
	return newnode;
}

int ModifyObjectNode::Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced)
{
	//*** handle disconnecting in how?
	return NodeBase::Disconnected(connection, from_will_be_replaced, to_will_be_replaced);
}

int ModifyObjectNode::Connected(NodeConnection *connection)
{
	if (this == connection->to && connection->toprop == properties.e[0]) {
		// we just connected something to in slot
		Value *nobj = connection->fromprop->GetData();
		
		if (obj) obj->dec_count();
		obj = nobj;
		int added = 0;
		if (obj) {
			 //Group is currently the only DrawableObject that has editable min/max
			bool isdrawable = (dynamic_cast<DrawableObject*>(obj) != nullptr && strcmp(obj->whattype(), "Group"));

			obj->inc_count();
			if (def) def->dec_count();
			def = obj->GetObjectDef();
			def->inc_count();

			ObjectDef *field;
			for (int c=0; c<def->getNumFields(); c++) {
				field = def->getField(c);

				if (field->format==VALUE_Function || field->format==VALUE_Class || field->format==VALUE_Operator || field->format==VALUE_Namespace)
					continue;

				if (isdrawable && ( // these are usually readonly, so special case skip.. need a better way to standardize readonly overrides
						!strcmp(field->name, "minx") || 
						!strcmp(field->name, "maxx") || 
						!strcmp(field->name, "miny") || 
						!strcmp(field->name, "maxy")
					 ))
					continue;

				// update property with new name, Name, desc...
				NodeProperty *prop = nullptr;
				// Value *val = obj->dereference(field->name,-1);
				if (added+1 < properties.n-1) {
					// modify property
					prop = properties.e[added+1];
					makestr(prop->name, field->name);
					makestr(prop->label, field->Name);
					makestr(prop->tooltip, field->description);
					// prop->SetData(val,1);

				} else {
					// insert new property
					prop = new NodeProperty(NodeProperty::PROP_Input,  true, field->name, nullptr,1, field->Name, field->description, 0, false);
					AddProperty(prop, added+1);
				}
				added++;
			}

			// remove old extra properties
			for (int c = added+1 ; c<properties.n-1; ) {
				RemoveProperty(properties.e[c]);
			}
		}
		properties.e[properties.n-1]->Touch();
		Wrap();
	}

	return NodeBase::Connected(connection);
}

/*! Clean up after NodeBade::dump_in_atts() to make sure the out property is at the end.
 */
void ModifyObjectNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att,flag,context);
	int i = -1;
	NodeProperty *out = FindProperty("out", &i);
	if (out && i != properties.n-1) {
		properties.slide(i, properties.n-1);
	}
}

int ModifyObjectNode::Update()
{
	Value *o = properties.e[0]->GetData();
	if (o) {
		o = o->duplicate();
		for (int c=1; c<properties.n-1; c++) {
			Value *v = properties.e[c]->GetData();
			if (v) {
				o->assign(v, properties.e[c]->Name());
				// v->dec_count();
			}
		}
	}
	properties.e[properties.n-1]->SetData(o,1);
	if (!o) return -1;
	return NodeBase::Update();
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int ModifyObjectNode::GetStatus()
{
	if (!properties.e[0]->GetData()) return -1;
	return NodeBase::GetStatus();
}


//------------ ObjectFunctionNode


/*! \class ObjectFunctionNode
 * Class to allow calling any function from an object's ObjectDef.
 * Please note the object is duplicated first, then the function is called.
 * If the function has output, that output is returned, else the object is returned.
 */
class ObjectFunctionNode : public NodeBase
{
  public:
	// Value *obj; // we keep this around so as to not dup properties unnecessarily
	char *lastfunc;

	ObjectFunctionNode();
	virtual ~ObjectFunctionNode();
	virtual int Update();
	virtual void UpdateProps();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int Connected(NodeConnection *connection);
	virtual int Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
	virtual Value *PreviewFrom() { return FindProperty("out")->GetData(); }
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);
	
	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ObjectFunctionNode(); }
};

ObjectFunctionNode::ObjectFunctionNode()
{
	makestr(type, "Objects/ObjectFunction");
	makestr(Name, _("Object Function"));

	lastfunc = nullptr;
	
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in", nullptr,0, _("In"), NULL, 0,false));

	ObjectDef *funcs = new ObjectDef(nullptr, "", "", nullptr, "enum", nullptr, nullptr);
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, true, "function", new EnumValue(funcs,0),1, "", NULL, 0,true));
	funcs->dec_count();

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,1, _("out"), NULL, 0,false));
	
	// if (obj) UpdateProps();
	Update();
}

ObjectFunctionNode::~ObjectFunctionNode()
{
	// if (obj) obj->dec_count();
	// if (func) func->dec_count();
	delete[] lastfunc;
}

/*! Clean up after NodeBade::dump_in_atts() to make sure the out property is at the end.
 */
void ObjectFunctionNode::dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context)
{
	NodeBase::dump_in_atts(att,flag,context);
	int i = -1;
	NodeProperty *out = FindProperty("out", &i);
	if (out && i != properties.n-1) {
		properties.slide(i, properties.n-1);
	}
}

NodeBase *ObjectFunctionNode::Duplicate()
{
	NodeBase *newnode = new ObjectFunctionNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int ObjectFunctionNode::Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced)
{
	//*** handle disconnecting in how?
	return NodeBase::Disconnected(connection, from_will_be_replaced, to_will_be_replaced);
}

int ObjectFunctionNode::Connected(NodeConnection *connection)
{
	if (this == connection->to && connection->toprop == properties.e[0]) {
		// we just connected something to in slot
		// Value *nobj = connection->fromprop->GetData();
		
		// if (nobj != obj) {
		// 	if (obj) obj->dec_count();
		// 	obj = nobj;
		// 	if (obj) obj->inc_count();

		// 	UpdateProps();
		// }
		UpdateProps();
		Update();
	}

	return NodeBase::Connected(connection);
}


/*! Update the prop field arrangement. Does NOT update the actual values here. Use the usual Update() for that.
 */
void ObjectFunctionNode::UpdateProps()
{
	int added = 0;

	Value *obj = properties.e[0]->GetData();
	if (obj) {
		// obj->inc_count();
		ObjectDef *def = obj->GetObjectDef(); //warning: shawdows NodeBase::def
		if (!def) {
			Error(_("Missing object def!"));
			return;
		}

		//update funcs list stored in prop 1 enumvalue. Will contain name and Name of all functions in object's def
		EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[1]->GetData());
		ObjectDef *funcs = ev->GetObjectDef();
		// const char *oldfunc = ev->EnumLabel();

		if (strcmp(funcs->name, def->name)) {
			//different objectdef, so we need to update the funcs list, copy over only function names
			funcs->Clear();
			makestr(funcs->name, def->name);
			makestr(funcs->Name, def->Name);

			ev->value = -1;
			for (int c=0; c<def->getNumFields(); c++) {
				ObjectDef *field = def->getField(c);
				if (field->format != VALUE_Function) continue;
				funcs->pushFunction(field->name, field->name, nullptr, nullptr, nullptr);
				added++;
			}
			funcs->Sort();
			if (ev->tempkey) {
				ev->value = funcs->findfield(ev->tempkey, nullptr);
				delete[] ev->tempkey;
				ev->tempkey = nullptr;
				ev->value = funcs->getNumEnumFields()-1;
			}

			if (ev->value < 0) ev->value = 0;
			delete[] lastfunc;
			lastfunc = nullptr;

			if (added == 0) {
				Error(_("Object has no functions"));
				return;
			}
		}

		//update properties to reflect the parameters of the new function
		const char *funcname = ev->EnumName();
		if (!funcname) funcname = ev->tempkey;
		added = 0;
		if (funcname && !strEquals(funcname, lastfunc)) {
			ObjectDef *funcdef = def->FindDef(funcname);
			if (funcdef) {
				makestr(lastfunc, funcname);

				for (int c=0; c<funcdef->getNumFields(); c++) {
					ObjectDef *field = funcdef->getField(c);

					// update property with new name, Name, desc...
					NodeProperty *prop = nullptr;
					Value *default_val = nullptr;
					if (funcdef->defaultValue) default_val = funcdef->defaultValue->duplicate();
					else {
						if (funcdef->format_str) {
							//try to create new object from type
							default_val = NewSimpleType(field->format);
						}
					}
					
					if (added+2 < properties.n-1) {
						// modify property
						prop = properties.e[added+2];
						makestr(prop->name, field->name);
						makestr(prop->label, field->Name);
						makestr(prop->tooltip, field->description);
						prop->SetData(default_val,1);

					} else {
						// insert new property
						prop = new NodeProperty(NodeProperty::PROP_Input,  true, field->name, default_val,1, field->Name, field->description, 0, true);
						AddProperty(prop, added+2);
					}
					added++;
				}
			}
		}
	}

	// remove old extra properties
	for (int c = added+2; c<properties.n-1; ) {
		RemoveProperty(properties.e[c]);
	}

	Wrap();
}

int ObjectFunctionNode::Update()
{
	Value *o = properties.e[0]->GetData();
	if (!o) {
		makestr(lastfunc,nullptr);
		UpdatePreview();
		Wrap();
		return -1;
	}
	if (!lastfunc) {
		UpdateProps();
	}

	//check function
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[1]->GetData());
	const char *fname = ev->EnumName();
	// if (!fname) fname = ev->tempkey;
	if (!strEquals(fname,lastfunc)) UpdateProps();

	ValueHash parameters;

	for (int c=2; c<properties.n-1; c++) {
		parameters.push(properties.e[c]->name, properties.e[c]->GetData());
	}

// *  0 for success, value optionally returned.
//  * -1 for no value returned due to incompatible parameters or name not known, which aids in function overloading.
//  *  1 for parameters ok, but there was somehow an error, so no value returned.
// virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
// 						 Value **value_ret,
// 						 Laxkit::ErrorLog *log) = 0;

	// Value *oo = o; oo->inc_count();
	Value *oo = o->duplicate(); // *** should do this only for non-"const" functions, or have it be optional
	FunctionEvaluator *func = dynamic_cast<FunctionEvaluator*>(oo);
	if (!func) {
		Error(_("Value cannot evaluate functions"));
	} else {
		Value *result = nullptr;
		// ValueHash *context = nullptr;
		ErrorLog log;
		// oo->Evaluate(fname,-1, context, &parameters, settings, &result, &log);
		if (func->Evaluate(fname,strlen(fname), nullptr, &parameters, nullptr, &result, &log) != 0) {
			// error!
			char *err = log.FullMessageStr();
			Error(err ? err : _("Error returned!"));
			delete[] err;
		} else {
			if (result) properties.e[properties.n-1]->SetData(result,1);
			else properties.e[properties.n-1]->SetData(oo,0);
		}
	}
	oo->dec_count();

	properties.e[properties.n-1]->Touch();
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int ObjectFunctionNode::GetStatus()
{
	if (!properties.e[0]->GetData()) return -1;
	return NodeBase::GetStatus();
}


//------------------------ TextFileNode ------------------------


class TextFileNode : public NodeBase
{
  public:
	TextFileNode(const char *txt);
	virtual ~TextFileNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new TextFileNode(nullptr); }
};

TextFileNode::TextFileNode(const char *what)
{
	makestr(Name, _("Text from file"));
	makestr(type, "Files/TextFile");

	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "what", new FileValue(what),1, _("File"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Text"), NULL,0, false));
}

TextFileNode::~TextFileNode()
{
}

NodeBase *TextFileNode::Duplicate()
{
	const char *file = nullptr;
	Value *v = properties.e[0]->GetData();
	if (v->type() == VALUE_String) file = dynamic_cast<StringValue*>(v)->str;
	else if (v->type() == VALUE_File) file = dynamic_cast<FileValue*>(v)->filename;
	
	TextFileNode *node = new TextFileNode(file);
	node->DuplicateBase(this);
	return node;
}

int TextFileNode::Update()
{
	const char *file = nullptr;
	Value *v = properties.e[0]->GetData();
	if (v->type() == VALUE_String) file = dynamic_cast<StringValue*>(v)->str;
	else if (v->type() == VALUE_File) file = dynamic_cast<FileValue*>(v)->filename;
	if (isblank(file)) return -1;

	int n = 0;
	char *contents = LaxFiles::read_in_whole_file(file, &n);
	if (!contents) return -1;

	StringValue *sv = dynamic_cast<StringValue*>(properties.e[1]->data);
	if (!sv) {
		sv = new StringValue();
		properties.e[1]->SetData(sv, 1);
	}
	sv->InstallString(contents);
	properties.e[1]->Touch();

	return NodeBase::Update();
}

//0 ok, -1 bad ins, 1 just needs updating
int TextFileNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return 1;
	if (v->type() != VALUE_String && v->type() != VALUE_File) return -1;
	return NodeBase::GetStatus();
}


//------------------------ JsonFileNode ------------------------

class JsonFileNode : public NodeBase
{
  public:
	JsonFileNode(const char *txt);
	virtual ~JsonFileNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new JsonFileNode(nullptr); }
};

JsonFileNode::JsonFileNode(const char *what)
{
	makestr(Name, _("Json file to object"));
	makestr(type, "Files/JsonFile");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "file", new FileValue(what),1, _("Json File"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Json"), NULL,0, false));
}

JsonFileNode::~JsonFileNode()
{
}

NodeBase *JsonFileNode::Duplicate()
{
	const char *file = nullptr;
	Value *v = properties.e[0]->GetData();
	if (v->type() == VALUE_String) file = dynamic_cast<StringValue*>(v)->str;
	else if (v->type() == VALUE_File) file = dynamic_cast<FileValue*>(v)->filename;
	
	JsonFileNode *node = new JsonFileNode(file);
	node->DuplicateBase(this);
	return node;
}

int JsonFileNode::Update()
{
	const char *file = nullptr;
	Value *v = properties.e[0]->GetData();
	if (v->type() == VALUE_String) file = dynamic_cast<StringValue*>(v)->str;
	else if (v->type() == VALUE_File) file = dynamic_cast<FileValue*>(v)->filename;
	if (isblank(file)) return -1;

	int n = 0;
	char *contents = LaxFiles::read_in_whole_file(file, &n);
	if (!contents) return -1;

	const char *error_ptr = nullptr;
	Value *json = JsonToValue(contents, &error_ptr);
	delete[] contents;
	if (!json) return -1;

	properties.e[1]->SetData(json, 1);

	return NodeBase::Update();
}

//0 ok, -1 bad ins, 1 just needs updating
int JsonFileNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return 1;
	if (v->type() != VALUE_String && v->type() != VALUE_File) return -1;
	return NodeBase::GetStatus();
}


//------------------------ CSVFileNode ------------------------

class CSVFileNode : public NodeBase
{
  public:
	CSVFileNode(const char *txt);
	virtual ~CSVFileNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new CSVFileNode(nullptr); }
};

CSVFileNode::CSVFileNode(const char *what)
{
	makestr(Name, _("CSV file to object"));
	makestr(type, "Files/CSVFile");

	//csv import options often:
	//  has headers (always strings)
	//  delimiter: comma, tab  --OR-- fixed width
	//  
	//  encoding
	//  Auto type conversion

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "file", new FileValue(what),1, _("CSV File"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "delimiter", new StringValue(","),1, _("Delim"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "headers", new BooleanValue(true),1, _("Has headers"),
											_("If true, use hashes based on first row, else use sets in return value")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "cols", new BooleanValue(true),1, _("Prefer columns"),
											_("Hash or sets of columns, or Set of (hashes or sets) per row"),0,true));
	// AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "coltypes", nullptr,1, _("Column types"), _("Default to strings, or numbers if convertible")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("CSV"), NULL,0, false));
}

CSVFileNode::~CSVFileNode()
{
}

NodeBase *CSVFileNode::Duplicate()
{
	const char *file = nullptr;
	Value *v = properties.e[0]->GetData();
	if (v->type() == VALUE_String) file = dynamic_cast<StringValue*>(v)->str;
	else if (v->type() == VALUE_File) file = dynamic_cast<FileValue*>(v)->filename;
	
	CSVFileNode *node = new CSVFileNode(file);
	node->DuplicateBase(this);
	return node;
}

//0 ok, -1 bad ins, 1 just needs updating
int CSVFileNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return 1;
	if (v->type() != VALUE_String && v->type() != VALUE_File) return -1;
	return NodeBase::GetStatus();
}

int CSVFileNode::Update()
{
	ClearError();

	const char *file = nullptr;
	Value *v = properties.e[0]->GetData();
	if (v->type() == VALUE_String) file = dynamic_cast<StringValue*>(v)->str;
	else if (v->type() == VALUE_File) file = dynamic_cast<FileValue*>(v)->filename;
	if (isblank(file)) return -1;

	StringValue *sv = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	if (!sv || isblank(sv->str)) return -1;
	const char *delimiter = sv->str;
	if (!delimiter) return -1;

	int isnum = 0;
	bool has_headers = getIntValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	bool prefer_cols = getIntValue(properties.e[3]->GetData(), &isnum);
	if (!isnum) return -1;

	int n = 0;
	char *contents = LaxFiles::read_in_whole_file(file, &n);
	if (!contents) return -1;

	int err = 0;
	v = CSVStringToValue(contents, delimiter, has_headers, prefer_cols, &err);
	if (err) return -1;

	properties.e[properties.n-1]->SetData(v, 1);
	properties.e[properties.n-1]->Touch();

	return NodeBase::Update();
}


//------------------------ ToFileNode ------------------------


class ToFileNode : public NodeBase
{
  public:
	ToFileNode();
	virtual ~ToFileNode();
	virtual NodeBase *Duplicate();

	virtual void ButtonClick(NodeProperty *prop, NodeInterface *controller);
	virtual int SaveNow();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ToFileNode(); }
};

ToFileNode::ToFileNode()
{
	makestr(Name, _("Object to file"));
	makestr(type, "Strings/ToFile");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", nullptr,1, _("In"), NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "file", new FileValue(),1, _("File"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false, "autosave", new BooleanValue(false),1, _("Save every update"), NULL,0, true));
	AddProperty(new NodeProperty(NodeProperty::PROP_Button, false, "savenow", nullptr,1, _("Save now"), NULL,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", nullptr,1, _("Text"), NULL,0, false));
}

ToFileNode::~ToFileNode()
{
}

NodeBase *ToFileNode::Duplicate()
{
	ToFileNode *newnode = new ToFileNode();
	newnode->DuplicateBase(this);
	return newnode;
}

void ToFileNode::ButtonClick(NodeProperty *prop, NodeInterface *controller)
{
	SaveNow();
}

int ToFileNode::SaveNow()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return 0;

	const char *file = nullptr;
	Value *vv = properties.e[1]->GetData();
	if (vv->type() == VALUE_String) file = dynamic_cast<StringValue*>(vv)->str;
	else if (vv->type() == VALUE_File) file = dynamic_cast<FileValue*>(vv)->filename;
	if (isblank(file)) {
		Error(_("Missing file"));
		return 0;
	}

	FILE *f = fopen(file, "w");
	if (!f) {
		Error(_("Cannot open file for writing"));
		return 0;
	}

	DumpContext context;
	v->dump_out(f, 0, 0, &context);

	fclose(f);

	return 1;
}

//0 ok, -1 bad ins, 1 just needs updating
int ToFileNode::GetStatus()
{
	Value *v = properties.e[0]->GetData();
	if (!v) return -1;
	v = properties.e[1]->GetData();
	if (v->type() != VALUE_String && v->type() != VALUE_File) return -1;
	return NodeBase::GetStatus();
}

int ToFileNode::Update()
{
	Error(nullptr);

	int isnum = 0;
	bool autosave = getIntValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	if (!autosave) return NodeBase::Update();
	if (!SaveNow()) {
		return -1;
	}
	return NodeBase::Update();
}



//------------------------------ GetResourceNode --------------------------------------------

/*! \class GetResourceNode
 * Get an object from the resource manager.
 */

class GetResourceNode : public NodeBase, public EventReceiver
{
	// static SingletonKeeper typeskeeper; //the def for the op enum
	// static ObjectDef *GetTypes() { return dynamic_cast<ObjectDef*>(typeskeeper.GetObject()); }
	// EventReceiver *eventcatch;

  public:
  	// std::clock_t lastcheck;
	GetResourceNode(const char *what);
	virtual ~GetResourceNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual void ButtonClick(NodeProperty *prop, NodeInterface *controller);
	virtual Value *PreviewFrom() { return FindProperty("out")->GetData(); }
	virtual int Event(const EventData *data,const char *mes);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GetResourceNode(nullptr); }
};

GetResourceNode::GetResourceNode(const char *what)
{
	// eventcatch = nullptr;

	// ObjectDef *resourcetypes = GetTypes();
	// if (!resourcetypes) {
	// 	resourcetypes = new ObjectDef(nullptr, "Resources", _("Resources"), nullptr, "enum", nullptr, nullptr);
	// 	typeskeeper.SetObject(resourcetypes, 1);

	// 	ResourceManager *resources = InterfaceManager::GetDefault(true)->GetResourceManager();
	// 	// *** this sucks because contents can change any time... need a dynamic menu for this
	// 	for (int c=0; c<resources->NumTypes(); c++) {
	// 		ResourceType *res = resources->GetTypeFromIndex(c);
	// 		resources.pushEnumValue(res->name, res->Name, res->description, c);
	// 	}
	// }

	makestr(type, "Resources/GetResource");
	makestr(Name, _("Get Resource"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Button, false, "Select", nullptr,1, _("Select..."), nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  false, "type",  new StringValue(what),1, _("Type"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  false, "which", new StringValue(),1, _("Name"), nullptr,0,false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out",   NULL,1, _("Out"), nullptr, 0, false));
}

GetResourceNode::~GetResourceNode()
{
	// if (eventcatch) delete eventcatch;
}

NodeBase *GetResourceNode::Duplicate()
{
	const char *what = nullptr;
	// StringValue *v = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	// if (v) what = v->str;
	GetResourceNode *node = new GetResourceNode(what);
	node->DuplicateBase(this);
	return node;
}

void GetResourceNode::ButtonClick(NodeProperty *prop, NodeInterface *controller)
{
	DBG cerr << "Get resource CLICK!!"<<endl;

	// if (!eventcatch) eventcatch = new EventReceiver();

	ResourceManager *resources = InterfaceManager::GetDefault(true)->GetResourceManager();

	MenuInfo *menu = new MenuInfo();
	for (int c=0; c<resources->NumTypes(); c++) {
		ResourceType *res = resources->GetTypeFromIndex(c);
		if (res->NumResources() == 0) continue;
		menu->AddItem(res->name);
		menu->SubMenu();
		res->AppendMenu(menu, false, nullptr, 0, 0);
		menu->EndSubMenu();
	}

	if (menu->n()) {
		PopupMenu *popup=new PopupMenu(NULL,_("Select Resource..."), 0,
								0,0,0,0, 1,
								object_id,"resource",
								0, //mouse to position near?
								menu,1, NULL,
								TREESEL_LEFT|TREESEL_SEND_PATH|TREESEL_LIVE_SEARCH);
		popup->WrapToMouse(0);
		anXApp::app->rundialog(popup);

	} else delete menu;
}

int GetResourceNode::Event(const EventData *data,const char *mes)
{
	DBG cerr << "GetResourceNode::Event: "<<mes<<endl;

	const SimpleMessage *e = dynamic_cast<const SimpleMessage*>(data);
	if (!e) return 1;

	const char *ptr = strchr(e->str, '/');
	if (!ptr) return 0; //malformed return string, expected "ResourceType/Blah/Blah/ResourceName"
	char *rtype = newnstr(e->str, ptr - e->str);
	const char *objname = strrchr(e->str, '/');
	if (!objname) return 0;
	objname++;

	ResourceManager *resources = InterfaceManager::GetDefault(true)->GetResourceManager();
	anObject *obj = resources->FindResource(objname, rtype, nullptr);

	//update rtype property
	StringValue *sv = dynamic_cast<StringValue*>(properties.e[1]->data);
	if (!sv) {
		sv = new StringValue(rtype);
		properties.e[1]->SetData(sv,1);
	} else {
		sv->Set(rtype);
		properties.e[1]->Touch();
	}

	//update which property
	sv = dynamic_cast<StringValue*>(properties.e[2]->data);
	if (!sv) {
		sv = new StringValue(objname);
		properties.e[2]->SetData(sv,1);
	} else {
		sv->Set(objname);
		properties.e[2]->Touch();
	}

	if (obj) {
		Value *v = dynamic_cast<Value*>(obj);
		if (v) FindProperty("out")->SetData(v, 0);
		else {
			ObjectValue *vv = new ObjectValue(obj);
			vv->Id(obj->Id());
			FindProperty("out")->SetData(vv, 1);
		}
		Update();
	} else return -1;

	return 0;
}

int GetResourceNode::Update()
{
	// FindProperty("out")->Touch();
	UpdatePreview();
	Wrap();
	return NodeBase::Update();
}

int GetResourceNode::GetStatus()
{
	return 0; //either something's there or not. c'est la vie!
}


//------------------------------ GetGlobalNode --------------------------------------------

/*! \class GetGlobalNode
 * Get a laidout->global variable.
 */

class GetGlobalNode : public NodeBase
{
  public:
  	// std::clock_t lastcheck;
	GetGlobalNode(const char *what);
	virtual ~GetGlobalNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GetGlobalNode(nullptr); }
};

GetGlobalNode::GetGlobalNode(const char *what)
{
	// lastcheck = 0;

	makestr(type, "Resources/GetGlobal");
	makestr(Name, _("Get Global Variable"));
	special_type = NODES_Global_Var;

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "name",   new StringValue(what),1, _("Name")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output,  true, "value",  NULL,1, _("Value"), nullptr, 0, false));
}

GetGlobalNode::~GetGlobalNode()
{
}

NodeBase *GetGlobalNode::Duplicate()
{
	const char *what = nullptr;
	StringValue *v = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (v) what = v->str;
	GetGlobalNode *node = new GetGlobalNode(what);
	node->DuplicateBase(this);
	return node;
}

int GetGlobalNode::Update()
{
	const char *what = nullptr;
	StringValue *sv = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!sv) return -1;
	what = sv->str;
	if (isblank(what)) return -1;

	Value *shouldbe = laidout->globals.find(what);
	if (!shouldbe) return -1;

	Value *v = properties.e[1]->GetData();
	if (v != shouldbe) {
		properties.e[1]->SetData(shouldbe, 0);
	} else properties.e[1]->Touch();

	// return 0; //do nothing here!
	return NodeBase::Update();
}

int GetGlobalNode::GetStatus()
{
	const char *what = nullptr;
	StringValue *sv = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!sv) return -1;
	what = sv->str;
	if (isblank(what)) return -1;

	Value *shouldbe = laidout->globals.find(what);
	if (!shouldbe) return -1;

	Value *v = properties.e[1]->GetData();
	if (!v) return 1;
	if (v != shouldbe) return 1;

	return NodeBase::GetStatus();
}


//------------------------------ SetGlobalNode --------------------------------------------

/*! \class SetGlobalNode
 * Set a laidout->global variable.
 */

class SetGlobalNode : public NodeBase
{
  public:
	SetGlobalNode(const char *what);
	virtual ~SetGlobalNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new SetGlobalNode(nullptr); }
};

SetGlobalNode::SetGlobalNode(const char *what)
{
	makestr(type, "Resources/SetGlobal");
	makestr(Name, _("Set Global Variable"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "name",   new StringValue(what),1, _("Name")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "value",  NULL,1,                 _("Value")));
}

SetGlobalNode::~SetGlobalNode()
{
}

NodeBase *SetGlobalNode::Duplicate()
{
	const char *what = nullptr;
	StringValue *v = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (v) what = v->str;
	SetGlobalNode *node = new SetGlobalNode(what);
	node->DuplicateBase(this);
	return node;
}

int SetGlobalNode::Update()
{
	const char *what = nullptr;
	StringValue *sv = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (sv) what = sv->str;

	if (isblank(what)) return -1;

	Value *shouldbe = properties.e[1]->GetData();
	if (!shouldbe) return -1;

	Value *v = laidout->globals.find(what);
	if (!v) {
		laidout->globals.push(what, shouldbe);
	} else if (v != shouldbe) {
		// properties.e[1]->SetData(v);
		laidout->globals.set(what, shouldbe);
	}

	// return 0; //do nothing here!
	return NodeBase::Update();
}

int SetGlobalNode::GetStatus()
{
	const char *what = nullptr;
	StringValue *sv = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!sv) return -1;
	what = sv->str;
	if (!what) return -1;
	Value *shouldbe = properties.e[1]->GetData();
	if (!shouldbe) return -1;
	Value *v = laidout->globals.find(what);
	if (v != shouldbe) return 1;

	return NodeBase::GetStatus();
}


//------------------------------ GetDocumentNode --------------------------------------------

/*! \class GetDocumentNode
 * Get document object. Kind of dangerous!!
 */

class GetDocumentNode : public NodeBase
{
  public:
	GetDocumentNode();
	virtual ~GetDocumentNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new GetDocumentNode(); }
};

GetDocumentNode::GetDocumentNode()
{
	makestr(type, "Document/GetDocument");
	makestr(Name, _("Get Document"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  false, "index",  new IntValue(0),1, _("Index"), _("Index into list of open documents")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "doc",  NULL,1,  _("Document"), nullptr, 0, false));
}

GetDocumentNode::~GetDocumentNode()
{
}

NodeBase *GetDocumentNode::Duplicate()
{
	GetDocumentNode *node = new GetDocumentNode();
	node->DuplicateBase(this);
	return node;
}

int GetDocumentNode::Update()
{
	Error(nullptr);
	IntValue *v = dynamic_cast<IntValue*>(properties.e[0]->GetData());
	if (!v || v->i < 0 || v->i >= laidout->project->docs.n) {
		Error(_("Bad index!"));
		return -1;
	}
	
	properties.e[1]->SetData(laidout->project->docs.e[v->i]->doc, 0);

	return NodeBase::Update();
}

int GetDocumentNode::GetStatus()
{
	if (!dynamic_cast<Document*>(properties.e[1]->GetData())) return -1;
	return NodeBase::GetStatus();
}


//------------------------------ ExportNode --------------------------------------------

/*! \class ExportNode
 * Export something to a file.
 */

class ExportNode : public NodeBase
{
  public:
	ExportNode();
	virtual ~ExportNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual void ButtonClick(NodeProperty *prop, NodeInterface *controller);
	virtual void ExportNow(DocumentExportConfig *config);

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new ExportNode(); }
};

ExportNode::ExportNode()
{
	makestr(type, "Document/Export");
	makestr(Name, _("Export"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "config",  nullptr,1, _("Config")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Button, false, "savenow", nullptr,1, _("Export now"), nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "autosave",  new BooleanValue(false),1, _("Auto process"), _("Export every single time input changes")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "value",  NULL,1,                 _("Value")));
}

ExportNode::~ExportNode()
{
}

NodeBase *ExportNode::Duplicate()
{
	ExportNode *node = new ExportNode();
	node->DuplicateBase(this);
	return node;
}

int ExportNode::Update()
{
	Error(nullptr);
	DocumentExportConfig *config = dynamic_cast<DocumentExportConfig*>(properties.e[0]->GetData());
	if (!config) {
		Error(_("Missing config"));
		return -1;
	}
	
	int isnum = 0;
	bool save_now = getIntValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	if (!save_now) return NodeBase::Update();

	ExportNow(config);

	return NodeBase::Update();
}

int ExportNode::GetStatus()
{
	if (!dynamic_cast<DocumentExportConfig*>(properties.e[0]->GetData())) return -1;
	return 0;
}

void ExportNode::ButtonClick(NodeProperty *prop, NodeInterface *controller)
{
	DBG cerr << "Export now!"<<endl;
	ExportNow(nullptr);
}

void ExportNode::ExportNow(DocumentExportConfig *config)
{
	if (!config) {
		config = dynamic_cast<DocumentExportConfig*>(properties.e[0]->GetData());
		if (!config) {
			Error(_("Missing config"));
			return;
		}	
	}

	if (config->range.NumRanges() == 0) {
		config->range.AddRange(0,-1,0);
	}

	ErrorLog log;
	int status = export_document(config, log);

	if (status != 0) {
		char *msg = log.FullMessageStr();
		if (status == 1) { //error
			Error(msg);
		} // else just warning
		DBG cerr << "Export message: "<<msg<<endl;
		delete[] msg;
	}
}


//------------------------------ RerouteNode --------------------------------------------

/*! \class RerouteNode
 */

RerouteNode::RerouteNode()
{
	makestr(type, "Reroute");
	makestr(Name, _("Reroute"));
	special_type = NODES_Reroute;

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",  NULL,1, _("In"),  nullptr,0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", NULL,0, _("Out"), nullptr,0, false)); 
}

RerouteNode::~RerouteNode()
{}

NodeBase *RerouteNode::Duplicate()
{
	RerouteNode *node = new RerouteNode();
	node->DuplicateBase(this);
	return node;
}

int RerouteNode::Update()
{
	properties.e[1]->SetData(properties.e[0]->GetData(), 0);
	return NodeBase::Update();
}

int RerouteNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int RerouteNode::Wrap()
{
	width = 0;
	height = 0;
	properties.e[0]->x = properties.e[0]->y = properties.e[0]->width = properties.e[0]->height = 0;
	properties.e[1]->x = properties.e[1]->y = properties.e[1]->width = properties.e[1]->height = 0;
	return 0;
}


//----------------------- DebugNode ------------------------

/*! \class DebugNode
 *
 * Do stuff.
 */
class DebugNode : public NodeBase
{
  public:
	DebugNode();
	virtual ~DebugNode();
	virtual NodeBase *Duplicate();
	virtual int GetStatus();
	virtual int Update();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new DebugNode(); }
};

DebugNode::DebugNode()
{
	makestr(type, "Debug");
	makestr(Name, _("Debug"));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "in", NULL,1, _("In")));
	special_type = NODES_Debug;
}

DebugNode::~DebugNode()
{
}

NodeBase *DebugNode::Duplicate()
{
	DebugNode *newnode = new DebugNode();
	newnode->DuplicateBase(this);
	return newnode;
}

int DebugNode::GetStatus()
{
	return NodeBase::GetStatus();
}

int DebugNode::Update() //bare bones
{
	return NodeBase::Update();
}


//--------------------------- SetupDefaultNodeTypes() -----------------------------------------

/*! Install default built in node types to factory.
 * This is called when the NodeGroup::NodeFactory singleton is created.
 */
int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory)
{

	//------------------ Misc value types -------------

	 //--- basics
	factory->DefineNewObject(getUniqueNumber(), "Basics/Integer",    newIntNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Basics/Boolean",    newBooleanNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Basics/Set",        newEmptySetNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Basics/Value",      newDoubleNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Basics/String",     newStringNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Basics/Color",      newColorNode,  NULL, 0);

	 //--- GradientStrip
	factory->DefineNewObject(getUniqueNumber(), "Gradient", newGradientNode,  NULL, 0);
	 //--- GradientTransform
	factory->DefineNewObject(getUniqueNumber(), "GradientTransform", newGradientTransformNode,  NULL, 0);

	 //--- Images
	factory->DefineNewObject(getUniqueNumber(), "Images/Image",     newImageNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Images/ImageFile", ImageFileNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Images/ImageInfo", newImageInfoNode,  NULL, 0);

	 //--- CurveNodes
	factory->DefineNewObject(getUniqueNumber(), "Curve",         newCurveNode,  NULL, 1);
	factory->DefineNewObject(getUniqueNumber(), "CurveTransform",newCurveNode,  NULL, 0);


	//------------------ Object maintenance -------------

	factory->DefineNewObject(getUniqueNumber(), "Object/TypeInfo",      newTypeInfoNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Object/NewObject",     NewObjectNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Object/ModifyObject",  ModifyObjectNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Object/ObjectInfo",    ObjectInfoNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Object/ObjectFunction",ObjectFunctionNode::NewNode,  NULL, 0);


	//------------------ Groups -------------

	 //--- NodeGroup
	factory->DefineNewObject(getUniqueNumber(), "NodeGroup",newNodeGroup,  NULL, 0);

	 //--- ObjectNodes
	factory->DefineNewObject(getUniqueNumber(), "Object In", newObjectInNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Object Out",newObjectOutNode, NULL, 0);

	//------------------ Bounds and transforms -------------

	 //--- RectangleNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Rectangle", RectangleNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/BBox",      RectangleNode::NewNode,  NULL, 1);
	factory->DefineNewObject(getUniqueNumber(), "Math/BBoxInfo",  BBoxInfoNode::NewNode,  NULL, 1);
	factory->DefineNewObject(getUniqueNumber(), "Math/BBoxPoint", BBoxPointNode::NewNode,  NULL, 0);

	 //--- Affine nodes
	factory->DefineNewObject(getUniqueNumber(), "Math/Affine",      newAffineNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Affine2",     newAffineNode2, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/AffineExpand",newAffineExpandNode, NULL, 0);

	//------------------ Math -------------
	factory->DefineNewObject(getUniqueNumber(), "Math/Math1",        newMathNode1,   NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Math2",        newMathNode2,   NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Value",        newDoubleNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Vector2",      newFlatvectorNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Vector3",      newSpacevectorNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Quaternion",   newQuaternionNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/ExpandVector", newExpandVectorNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Random",       RandomNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Lerp",         newLerpNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/MapToRange",   newMapToRangeNode,    NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/MapFromRange", newMapFromRangeNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Constant",     newMathConstants,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Expression",   ExpressionNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Invert",       InvertNode::NewNode, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/ConvertNumber",ConvertNumberNode::NewNode, NULL, 0);
	
	//------------------ Lists -------------
	factory->DefineNewObject(getUniqueNumber(), "Lists/EmptySet",    newEmptySetNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/GetElement",  GetElementNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/GetSize",     GetSizeNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/ListOf",      ListOfNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/Subset",      SubsetNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/JoinSets",    JoinSetsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/MakeSet",     MakeSetNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/NumberList",  NumberListNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/Shuffle",     ShuffleNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Lists/Switch",      SwitchNode::NewNode,  NULL, 0);
	//special duplication of GetElement
	factory->DefineNewObject(getUniqueNumber(), "Drawable/GetChild", GetElementNode::NewNode,  NULL, 1);

	//------------------ String -------------
	factory->DefineNewObject(getUniqueNumber(), "Strings/String",      newStringNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Strings/Concat",      ConcatNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Strings/Slice",       SliceNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Strings/Split",       SplitNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Strings/NumToString", NumToStringNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Strings/ToFile",      ToFileNode::NewNode,  NULL, 0);

	factory->DefineNewObject(getUniqueNumber(), "Strings/TextFromFile",TextFileNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Files/JsonFile",      JsonFileNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Files/CSVFile",       CSVFileNode::NewNode,  NULL, 0);

	 //-------------------- FILTERS -------------
	factory->DefineNewObject(getUniqueNumber(), "Filters/TransformAffine", TransformAffineNode::NewNode,  NULL, 0);

	 //-------------------- THREADS -------------
	factory->DefineNewObject(getUniqueNumber(), "Threads/Thread",     newThreadNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/Loop",       newLoopNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/If",         newIfNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/Fork",       newForkNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/Delay",      newDelayNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/Foreach",    ForeachNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/SetVariable",newSetVariableNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Threads/GetVariable",newGetVariableNode,  NULL, 0);

	 //-------------------- Resources -------------
	factory->DefineNewObject(getUniqueNumber(), "Resources/SetGlobal",  SetGlobalNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Resources/GetGlobal",  GetGlobalNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Resources/GetResource",GetResourceNode::NewNode,  NULL, 0);

	 //-------------------- Specials -------------
	factory->DefineNewObject(getUniqueNumber(), "Reroute",RerouteNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Debug",  DebugNode::NewNode,  NULL, 0);

	factory->DefineNewObject(getUniqueNumber(), "Document/Export",      ExportNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Document/GetDocument", GetDocumentNode::NewNode,  NULL, 0);



	//Register nodes for DrawableObject filters:
	RegisterFilterNodes(factory);

	//Register DrawableObject nodes:
	SetupDataObjectNodes(factory);
	

	return 0;
}



} //namespace Laidout

