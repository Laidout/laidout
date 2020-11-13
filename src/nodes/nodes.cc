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
	int dims;
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
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("V"), out,1)); 
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
		getNumberValue(properties.e[c]->GetData(),&isnum);
		if (!isnum) return -1;
	}

	if (!properties.e[dims]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int VectorNode::Update()
{
	int isnum;
	double vs[4];
	for (int c=0; c<dims; c++) {
		vs[c] = getNumberValue(properties.e[c]->GetData(),&isnum);
	}

	//if (!properties.e[2]->data) return 1;

	if (!properties.e[dims]->data) {
		if      (dims == 2) properties.e[2]->data=new FlatvectorValue (flatvector (vs));
		else if (dims == 3) properties.e[3]->data=new SpacevectorValue(spacevector(vs));
		else if (dims == 4) properties.e[4]->data=new QuaternionValue (Quaternion (vs));
	} else {
		//assume correct format already in prop
		if      (dims == 2) dynamic_cast<FlatvectorValue* >(properties.e[2]->data)->v = flatvector (vs);
        else if (dims == 3) dynamic_cast<SpacevectorValue*>(properties.e[3]->data)->v = spacevector(vs);
        else if (dims == 4) dynamic_cast<QuaternionValue* >(properties.e[4]->data)->v = Quaternion (vs);
	}
	properties.e[dims]->modtime = times(NULL);

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
	RectangleNode();
	virtual ~RectangleNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

RectangleNode::RectangleNode()
{
	Id("Rectangle");
	Label(_("Rectangle"));
	makestr(type, "Rectangle");

	const char *names[]  = {   "x" ,   "y" ,   "width" ,   "height"  };
	const char *labels[] = { _("x"), _("y"), _("width"), _("height") };

	for (int c=0; c<4; c++) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, names[c], new DoubleValue(0), 1, labels[c])); 
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Rect", new BBoxValue(), 1, _("Rect")));
}

RectangleNode::~RectangleNode()
{
}

NodeBase *RectangleNode::Duplicate()
{
	RectangleNode *newnode = new RectangleNode();
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

	if (!properties.e[4]->data) properties.e[4]->data = new BBoxValue(vs[0], vs[0]+vs[2], vs[1], vs[1]+vs[3]);
	else {
		BBoxValue *v = dynamic_cast<BBoxValue*>(properties.e[4]->data);
		v->setbounds(vs[0], vs[0]+vs[2], vs[1], vs[1]+vs[3]);
	}

	properties.e[4]->Touch();

	return NodeBase::Update();
}

Laxkit::anObject *newRectangleNode(int p, Laxkit::anObject *ref)
{
	return new RectangleNode();
}


//------------ ColorNode

Laxkit::anObject *newColorNode(int p, Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	makestr(node->Name, _("Color"));
	makestr(node->type, "Color");

	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Color"), new ColorValue("#ffffff"), 1));
	//----------
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Red"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Green"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Blue"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, _("Alpha"), new DoubleValue(1), 1)); 
	//node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, false, _("Out"), new ColorValue("#ffffffff"), 1)); 
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

class MathNode2 : public NodeBase
{
  public:
	static SingletonKeeper mathnodekeeper; //the def for the op enum
	static ObjectDef *GetMathNode2Def() { return dynamic_cast<ObjectDef*>(mathnodekeeper.GetObject()); }

	int last_status;
	clock_t status_time;

	int numargs;
	double a,b,result;
	MathNode2(int op=0, double aa=0, double bb=0); //see 2 arg MathNodeOps for op
	virtual ~MathNode2();
	virtual int UpdateThisOnly();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	//virtual const char *Label();
};

SingletonKeeper MathNode1::mathnodekeeper(DefineMathNode1Def(), true);
SingletonKeeper MathNode2::mathnodekeeper(DefineMathNode2Def(), true);

//------MathNode1

MathNode1::MathNode1(int op, double aa)
{
	type = newstr("Math1");
	//Name = newstr(_("Math 1"));

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
	if (a) {
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
	if (proptime > last_status) {
		last_status = UpdateThisOnly();
		status_time = proptime;
	}
	return last_status;
}

int MathNode1::Update()
{
	int status = UpdateThisOnly();
	if (!status) {
		modtime = times(NULL);
		PropagateUpdate();
		return status;
	}
	return status;
}

int MathNode1::UpdateThisOnly()
{
	makestr(error_message, NULL);

	Value *valuea = properties.e[1]->GetData();
	int aisnum=0;
	double a = getNumberValue(valuea, &aisnum);
	if (aisnum) aisnum = 1; //else ints and booleans return 2 and 3

	if (!aisnum && dynamic_cast<FlatvectorValue *>(valuea)) aisnum = 2;
	if (!aisnum && dynamic_cast<SpacevectorValue*>(valuea)) aisnum = 3;
	if (!aisnum && dynamic_cast<QuaternionValue *>(valuea)) aisnum = 4;

	if (!aisnum) {
		makestr(error_message, _("Operation can't use that argument"));
		return -1;
	}

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();
	const char *nm = NULL, *Nm = NULL;
	double result=0;
	int operation = OP_None;
	def->getEnumInfo(ev->value, &nm, &Nm, NULL, &operation);
	//makestr(Name, Nm);


	if (aisnum == 1) {
		if        (operation == OP_AbsoluteValue  || operation == OP_Norm) { result = fabs(a);
		} else if (operation == OP_Norm2            ) { result = a*a;
		} else if (operation == OP_Negative || operation == OP_Flip) { result = -a;
		} else if (operation == OP_Sqrt             ) {
			if (a>=0) result = sqrt(a);
			else {
				makestr(error_message, _("Sqrt needs nonnegative number"));
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
				makestr(error_message, _("Argument needs to be in range [-1, 1]"));
				return -1;
			}
		} else if (operation == OP_Acos             )  {
			if (a <= 1 && a >= -1) result = acos(a);
			else {
				makestr(error_message, _("Argument needs to be in range [-1, 1]"));
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
				makestr(error_message, _("Argument needs to be >= 1"));
				return -1;
			}
		} else if (operation == OP_Atanh            )  {
			if (a > -1 && a < 1) result = atanh(a);
			else {
				makestr(error_message, _("Argument needs to be -1 > a > 1"));
				return -1;
			}
		} else if (operation == OP_Ceiling          )  { result = ceil(a);
		} else if (operation == OP_Floor            )  { result = floor(a);
		} else if (operation == OP_Clamp_To_1       )  { result = a > 1 ? 1 : a < 0 ? 0 : a;
		} else if (operation == OP_Clamp_To_pm_1    )  { result = a > 1 ? 1 : a < -1 ? -1 : a;
		} else {
			makestr(error_message, _("Operation can't use that argument"));
			return -1;
		}

		if (!dynamic_cast<DoubleValue*>(properties.e[2]->data)) {
			DoubleValue *newv = new DoubleValue(result);
			properties.e[2]->SetData(newv, 1);
		} else dynamic_cast<DoubleValue*>(properties.e[2]->data)->d = result;

		properties.e[2]->modtime = times(NULL);
		return 0;

	}
	
	//else involves vectors...
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
		for (int c=0; c<aisnum; c++) result += va[c]*va[c];
		result = sqrt(result);
		if (result == 0) {
			makestr(error_message, _("Can't normalize a null vector"));
			return -1;
		}
		for (int c=0; c<aisnum; c++) rv[c] = va[c]/result;

	} else if (operation == OP_Angle) {
		if (aisnum != 2) {
			makestr(error_message, _("Only for Vector2"));
			return -1;
		}
		resulttype = VALUE_Real;
		result = atan2(va[1], va[0]);

	} else resulttype = VALUE_None;

	if (resulttype == VALUE_None) {
		makestr(error_message, _("Operation can't use that argument"));
		return -1;
	}

	if (resulttype == VALUE_Real) {
		if (!dynamic_cast<DoubleValue*>(properties.e[2]->data)) {
			DoubleValue *newv = new DoubleValue(result);
			properties.e[2]->SetData(newv, 1);
		} else dynamic_cast<DoubleValue*>(properties.e[2]->data)->d = result;

	} else if (resulttype == VALUE_Flatvector) {
		if (!dynamic_cast<FlatvectorValue*>(properties.e[2]->data)) {
			FlatvectorValue *newv = new FlatvectorValue(flatvector(rv));
			properties.e[2]->SetData(newv, 1);
		} else dynamic_cast<FlatvectorValue*>(properties.e[2]->data)->v.set(rv);

	} else if (resulttype == VALUE_Spacevector) {
		if (!dynamic_cast<SpacevectorValue*>(properties.e[2]->data)) {
			SpacevectorValue *newv = new SpacevectorValue(spacevector(rv));
			properties.e[2]->SetData(newv, 1);
		} else dynamic_cast<SpacevectorValue*>(properties.e[2]->data)->v.set(rv);

	} else if (resulttype == VALUE_Quaternion) {
		if (!dynamic_cast<QuaternionValue*>(properties.e[2]->data)) {
			QuaternionValue *newv = new QuaternionValue(Quaternion(rv));
			properties.e[2]->SetData(newv, 1);
		} else dynamic_cast<QuaternionValue*>(properties.e[2]->data)->v.set(rv);

	}

	properties.e[2]->modtime = times(NULL);
	return 0;
}

Laxkit::anObject *newMathNode1(int p, Laxkit::anObject *ref)
{
	return new MathNode1();
}




//------MathNode2

MathNode2::MathNode2(int op, double aa, double bb)
{
	type = newstr("Math2");
	Name = newstr(_("Math 2"));

	last_status = 1;
	status_time = 0;

	a=aa;
	b=bb;
	numargs = 2;

	ObjectDef *enumdef = GetMathNode2Def();
	enumdef->inc_count();


	EnumValue *e = new EnumValue(enumdef, 0);
	e->SetFromId(op);
	enumdef->dec_count();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "Op", e, 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "A", new DoubleValue(a), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "B", new DoubleValue(b), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Result", NULL,0, _("Result"), NULL, 0, false));

	//NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
					//const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable);

	last_status = Update();
	status_time = MostRecentIn(NULL);
}

MathNode2::~MathNode2()
{
//	if (mathnodedef) {
//		if (mathnodedef->dec_count()<=0) mathnodedef=NULL;
//	}
}

NodeBase *MathNode2::Duplicate()
{
	int operation = -1;
	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *edef = ev->GetObjectDef();
	edef->getEnumInfo(ev->value, NULL,NULL,NULL, &operation);

	MathNode2 *newnode = new MathNode2(operation);
	Value *a = properties.e[1]->GetData();
	if (a) {
		a = a->duplicate();
		newnode->properties.e[1]->SetData(a, 1);
	}
	a = properties.e[2]->GetData();
	if (a) {
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
	if (proptime > last_status) {
		last_status = UpdateThisOnly();
		status_time = proptime;
	}
	return last_status;
}

int MathNode2::Update()
{
	int status = UpdateThisOnly();
	if (!status) {
		modtime = times(NULL);
		PropagateUpdate();
		return status;
	}
	return NodeBase::Update();
}

int MathNode2::UpdateThisOnly()
{
	makestr(error_message, NULL);

	Value *valuea = properties.e[1]->GetData();
	Value *valueb = properties.e[2]->GetData();
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

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();
	const char *nm = NULL, *Nm = NULL;
	int operation = OP_None;
	def->getEnumInfo(ev->value, &nm, &Nm, NULL, &operation);
	makestr(Name, Nm);


	if (aisnum == 1 && bisnum == 1) {
		if      (operation==OP_Add)      result = a+b;
		else if (operation==OP_Subtract) result = a-b;
		else if (operation==OP_Multiply) result = a*b;
		else if (operation==OP_Divide) {
			if (b!=0) result = a/b;
			else {
				result=0;
				makestr(error_message, _("Can't divide by 0"));
				return -1;
			}

		} else if (operation==OP_Mod) {
			if (b!=0) result = a-b*int(a/b);
			else {
				result=0;
				makestr(error_message, _("Can't divide by 0"));
				return -1;
			}
		} else if (operation==OP_Power) {
			if (a==0 || (a<0 && fabs(b)-fabs(int(b))<1e-10)) {
				 //0 to a power fails, as does negative numbers raised to non-integer powers
				result=0;
				makestr(error_message, _("Power must be positive"));
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
			makestr(error_message, _("Operation can't use those arguments"));
			return -1;
		}

		if (!dynamic_cast<DoubleValue*>(properties.e[3]->data)) {
			DoubleValue *newv = new DoubleValue(result);
			properties.e[3]->SetData(newv, 1);
		} else dynamic_cast<DoubleValue*>(properties.e[3]->data)->d = result;

		properties.e[3]->modtime = times(NULL);
		return 0;

	}
	
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
				makestr(error_message, _("Bad parameters"));
				return -1;
			}
			if (b == 0) {
				makestr(error_message, _("Can't divide by 0"));
				return -1;
			}
			if (operation==OP_Mod) {
				for (int c=0; c<4; c++) rv[c] = vv[c] - b*int(vv[c]/b);

			} else for (int c=0; c<4; c++) rv[c] = vv[c] / b;

		} else {
			makestr(error_message, _("Operation can't use those arguments"));
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
				makestr(error_message, _("Vector b can't be 0"));
				return -1;
			}
			double dot = va[0]*vb[0]+va[1]*vb[1]+va[2]*vb[2]+va[3]*vb[3];
			dot /= norm2;
			for (int c=0; c<4; c++) rv[c] = va[c] - dot*vb[c];

		} else if (operation==OP_Parallel      ) {
			double norm2 = vb[0]*vb[0]+vb[1]*vb[1]+vb[2]*vb[2]+vb[3]*vb[3];
			if (norm2 == 0) {
				makestr(error_message, _("Vector b can't be 0"));
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
		makestr(error_message, _("Operation can't use those arguments"));
		return -1;
	}

	if (resulttype == VALUE_Real) {
		if (!dynamic_cast<DoubleValue*>(properties.e[3]->data)) {
			DoubleValue *newv = new DoubleValue(result);
			properties.e[3]->SetData(newv, 1);
		} else dynamic_cast<DoubleValue*>(properties.e[3]->data)->d = result;

	} else if (resulttype == VALUE_Flatvector) {
		if (!dynamic_cast<FlatvectorValue*>(properties.e[3]->data)) {
			FlatvectorValue *newv = new FlatvectorValue(flatvector(rv));
			properties.e[3]->SetData(newv, 1);
		} else dynamic_cast<FlatvectorValue*>(properties.e[3]->data)->v.set(rv);

	} else if (resulttype == VALUE_Spacevector) {
		if (!dynamic_cast<SpacevectorValue*>(properties.e[3]->data)) {
			SpacevectorValue *newv = new SpacevectorValue(spacevector(rv));
			properties.e[3]->SetData(newv, 1);
		} else dynamic_cast<SpacevectorValue*>(properties.e[3]->data)->v.set(rv);

	} else if (resulttype == VALUE_Quaternion) {
		if (!dynamic_cast<QuaternionValue*>(properties.e[3]->data)) {
			QuaternionValue *newv = new QuaternionValue(Quaternion(rv));
			properties.e[3]->SetData(newv, 1);
		} else dynamic_cast<QuaternionValue*>(properties.e[3]->data)->v.set(rv);

	} else {
		makestr(error_message, _("Operation can't use those arguments"));
		return -1;
	}

	properties.e[3]->modtime = times(NULL);
	return 0;
}

Laxkit::anObject *newMathNode2(int p, Laxkit::anObject *ref)
{
	return new MathNode2();
}


//----------------------------------- ExpressionNode ------------------------------

class ExpressionNode : public NodeBase
{
  public:
	ExpressionNode(const char *expr);
	virtual ~ExpressionNode();

	virtual int Set(const char *expr, bool force_remap);
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
	makestr(Name, _("Expression"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "Expr", new StringValue(""),1, nullptr,_("Expression")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", nullptr,1, _("Out"), NULL,0, false));

	Set(expr, true);

	Update();
}

ExpressionNode::~ExpressionNode()
{
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
		return status;
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
	if (!s) return -1;

	if (ErrorMessage()) return -1;

	return NodeBase::GetStatus();
}

//0 ok, -1 bad ins, 1 just needs updating
int ExpressionNode::Update()
{
	Error(nullptr);
	if (GetStatus() == -1) return -1;

	StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	if (!s) return -1;
	const char *expression = s->str;

	if (modtime == 0 || modtime < properties.e[0]->modtime) {
		Set(expression, modtime == 0);
		if (error_message) return -1;
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
	ImageValue *i = dynamic_cast<ImageValue*>(properties.e[1]->GetData());
	if (!i || !i->image) return -1;
	if (i->image->w() <= 0 || i->image->h() <= 0) return -1;

	return NodeBase::GetStatus(); //default checks mod times
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int ImageFileNode::Update()
{
	const char *file = GetFilename();
	ImageValue *iv = dynamic_cast<ImageValue*>(properties.e[1]->GetData());

	if (iv && iv->image && iv->image->filename && file && !strcmp(iv->image->filename, file)) {
		//same filename, assume its up to date
		return 0;
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
};

RandomNode::RandomNode(int seed, double min, double max)
{
	makestr(Name, _("Random"));
	makestr(type, "Random");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Seed",    new IntValue(seed),1,      _("Seed"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Minimum", new DoubleValue(min),1,    _("Minimum"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Maximum", new DoubleValue(max),1,    _("Maximum"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Block, false,"Integer", new BooleanValue(false),1, _("Integer"))); 

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Number", new DoubleValue(),1, _("Number"))); 
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

	if (!properties.e[4]->data) return 1;

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

	return NodeBase::Update();
}

Laxkit::anObject *newRandomNode(int p, Laxkit::anObject *ref)
{
	return new RandomNode(0, 0., 1.);
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
	char *str=NULL;

	int isnum=0;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (isnum) {
		str = numtostr(d);
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
		if (!s) return -1;

		str = newstr(s->str);
	}

	StringValue *out = dynamic_cast<StringValue*>(properties.e[1]->GetData());
	out->Set(str);

	delete[] str;

	return NodeBase::Update();
}


Laxkit::anObject *newStringNode(int p, Laxkit::anObject *ref)
{
	return new StringNode(NULL);
}


//------------------------------ concat --------------------------------------------

class ConcatNode : public NodeBase
{
  public:
	ConcatNode(const char *s1, const char *s2);
	virtual ~ConcatNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};

ConcatNode::ConcatNode(const char *s1, const char *s2)
{
	makestr(Name, _("Concat"));
	makestr(type, "Concat");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "A", new StringValue(s1),1, _("A")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "B", new StringValue(s2),1, _("B")));

	char ns[1 + (s1 ? strlen(s1) : 0) + (s2 ? strlen(s2) : 0)];
	sprintf(ns, "%s%s", (s1 ? s1 : ""), (s2 ? s2 : ""));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new StringValue(ns),1, _("Out"),
								 nullptr, 0, false));
}

ConcatNode::~ConcatNode()
{
}

NodeBase *ConcatNode::Duplicate()
{
	StringValue *s1 = dynamic_cast<StringValue*>(properties.e[0]->GetData());
	StringValue *s2 = dynamic_cast<StringValue*>(properties.e[1]->GetData());

	ConcatNode *newnode = new ConcatNode(s1 ? s1->str : NULL, s2 ? s2->str : NULL);
	newnode->DuplicateBase(this);
	return newnode;
}

int ConcatNode::GetStatus()
{
	if (!isNumberType(properties.e[0]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[0]->GetData())) return -1;
	if (!isNumberType(properties.e[1]->GetData(), NULL) && !dynamic_cast<StringValue*>(properties.e[1]->GetData())) return -1;

	if (!properties.e[2]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int ConcatNode::Update()
{
	char *str=NULL;

	int isnum=0;
	double d = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (isnum) {
		str = numtostr(d);
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[0]->GetData());
		if (!s) return -1;

		str = newstr(s->str);
	}

	d = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (isnum) {
		char *ss = numtostr(d);
		appendstr(str, ss);
		delete[] ss;
	} else {
		StringValue *s = dynamic_cast<StringValue*>(properties.e[1]->GetData());
		if (!s) return -1;

		appendstr(str, s->str);
	}

	StringValue *out = dynamic_cast<StringValue*>(properties.e[2]->GetData());
	out->Set(str);

	delete[] str;

	return NodeBase::Update();
}


Laxkit::anObject *newConcatNode(int p, Laxkit::anObject *ref)
{
	return new ConcatNode(NULL,NULL);
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
	virtual int Update();
	virtual int GetStatus();
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


Laxkit::anObject *newSliceNode(int p, Laxkit::anObject *ref)
{
	return new SliceNode(NULL,0,0);
}


//------------------------ ThreadNode ------------------------

/*! \class Starts an execution path.
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
	PropagateUpdate();

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

/*! \class
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
		PropagateUpdate();
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

	PropagateUpdate();
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

	PropagateUpdate();
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
		makestr(Name, _("Map to range"));
		makestr(type, "MapToRange");
	} else {
		makestr(Name, _("Map from range"));
		makestr(type, "MapFromRange");
	}

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", new DoubleValue(0),1,    _("In"))); 

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
	if (!isNumberType(properties.e[0]->GetData(), NULL)) return -1;

	int isnum;
	double min = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	double max = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	if (min==max) return -1;

	if (!properties.e[4]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int MapRangeNode::Update()
{
	int isnum=0;
	double in = getNumberValue(properties.e[0]->GetData(), &isnum);
	if (!isnum) return -1;
	double min = getNumberValue(properties.e[1]->GetData(), &isnum);
	if (!isnum) return -1;
	double max = getNumberValue(properties.e[2]->GetData(), &isnum);
	if (!isnum) return -1;
	if (min==max) return -1;
	bool clamp = dynamic_cast<BooleanValue*>(properties.e[3]->GetData())->i;

	double num;
	if (mapto) {
		num = min + (max-min)*in;
		if (clamp) {
			if (max>min) {
				if (num<min) num=min;
				else if (num>max) num=max;
			} else {
				if (num<max) num=max;
				else if (num>min) num=min;
			}
		}
	} else {
		num = (in-min) / (max-min);
		if (clamp) {
			if (num<0) num=0;
			if (num>1) num=1;
		}
	}

	Value *v = properties.e[4]->GetData();

	if (!dynamic_cast<DoubleValue*>(v)) {
		v = new DoubleValue(num);
		properties.e[4]->SetData(v, 1);
	} else dynamic_cast<DoubleValue*>(v)->d = num;

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
};

TransformAffineNode::TransformAffineNode()
{
	makestr(Name, _("Transform Affine"));
	makestr(type, "Filters/TransformAffine");

	Value *v = new AffineValue();
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in", NULL,0, _("In"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "Affine", v,1, _("Affine"))); 
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "out", NULL,0, _("Out"))); 
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
			&& vtype != PointSetValue::TypeNumber()
			&& !dynamic_cast<Affine*>(v)
		   ) return -1;
	}

	if (v && !properties.e[2]->data) return 1; //just needs update

	return NodeBase::GetStatus(); //default checks mod times
}

int TransformAffineNode::Update()
{
	Error(nullptr);

	Value *nv = NULL;
	Value *v = properties.e[0]->GetData();

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

	if (!v) {
		 //clear output when there is no input
		if (properties.e[2]->GetData()) properties.e[2]->SetData(NULL,0);
		Error(_("Missing input"));
		return -1;

	} else {
		int vtype = v->type();
		if (vtype == VALUE_Flatvector) {
			FlatvectorValue *fv  = dynamic_cast<FlatvectorValue*>(v);
			FlatvectorValue *nnv = dynamic_cast<FlatvectorValue*>(properties.e[2]->GetData());
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
			AffineValue *nnv = dynamic_cast<AffineValue*>(properties.e[2]->GetData());
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

		} else {
			Error(_("Cannot transform that type"));
			return -1;
		}

	}

	properties.e[2]->SetData(nv, 1);

	return NodeBase::Update();
}

Laxkit::anObject *newTransformAffineNode(int p, Laxkit::anObject *ref)
{
	return new TransformAffineNode();
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
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);
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
int CurveNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
	if (!strcmp(propname, "Curve")) {
		CurveProperty *prop = dynamic_cast<CurveProperty*>(FindProperty(propname));
		CurveValue *curve = prop->curve;
		curve->dump_in_atts(att,0,NULL);
		prop->SetData(curve, 0);
	}
	
	return NodeBase::SetPropertyFromAtt(propname, att);
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

	gradient = ngradient;
	if (gradient && !absorb) gradient->inc_count();
	data = dynamic_cast<Value*>(gradient);
	if (data) data->inc_count();
}

GradientProperty::~GradientProperty()
{
	DBG cerr <<"...deleting gradient prop data"<<endl;
	if (gradient) gradient->dec_count();
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
	GradientInterface *interf = dynamic_cast<GradientInterface*>(interfacekeeper.GetObject());
	if (!interf) {
		interf = new GradientInterface(getUniqueNumber(), NULL);
		makestr(interf->owner_message, "GradientChange");
		interf->style |= GradientInterface::RealSpace | GradientInterface::RealSpaceMouse | GradientInterface::Expandable | GradientInterface::EditStrip;
		interfacekeeper.SetObject(interf, 1);
	}
	return interf;
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
	interf->UseThisObject(gradient);
	double th = owner->colors->font->textheight();
	interf->SetupRect(owner->x + x + th, owner->y + y, width-2*th, height);

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
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);
};

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
	GradientValue *v = dynamic_cast<GradientProperty*>(FindProperty("Gradient"))->gradient;
	v = dynamic_cast<GradientValue*>(v->duplicate());
	GradientNode *newnode = new GradientNode(for_creation, v);
	newnode->DuplicateBase(this);
	newnode->Wrap();
	return newnode;
}

/*! Return 1 for set, 0 for not set.
 */
int GradientNode::SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att)
{
	if (!strcmp(propname, "gradient")) {
		GradientProperty *prop = dynamic_cast<GradientProperty*>(FindProperty(propname));
		GradientValue *gradient = prop->gradient;
		gradient->dump_in_atts(att,0,NULL);
		prop->SetData(gradient, 0);
		return 1;
	}
	
	return NodeBase::SetPropertyFromAtt(propname, att);
}

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int GradientNode::Update()
{
	if (for_creation) return NodeBase::Update();

	//float -> color
	//color -> grayscale -> color
	//image -> grayscale -> new image
	
	GradientValue *gv = dynamic_cast<GradientValue*>(properties.e[1]->GetData());
	Value *in = properties.e[0]->GetData();
	double d;
	bool isnum = false;
	if (isNumberType(in, &d)) isnum = true;
	else if (dynamic_cast<ColorValue*>(in)) {
		ColorValue *cv = dynamic_cast<ColorValue*>(in);
		d = simple_rgb_to_grayf(cv->color.Red(), cv->color.Green(), cv->color.Blue());
		isnum = true;
	}

	if (isnum) {
		ColorValue *cv = dynamic_cast<ColorValue*>(properties.e[2]->GetData());
		if (!cv) cv = new ColorValue();
		else cv->inc_count();
		ScreenColor col;
		gv->WhatColor(d, &col, true);
		cv->color.SetRGB(col.Red(), col.Green(), col.Blue());
		properties.e[2]->SetData(cv,1);
	}
	else {
		// ***
		return -1;
	}

	return NodeBase::Update();
}

int GradientNode::GetStatus()
{
	return NodeBase::GetStatus();
}
 



Laxkit::anObject *newGradientNode(int p, Laxkit::anObject *ref)
{
	return new GradientNode(1, nullptr);
}

Laxkit::anObject *newGradientTransformNode(int p, Laxkit::anObject *ref)
{
	return new GradientNode(0, nullptr);
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

};

SingletonKeeper NewObjectNode::menuKeeper;

NewObjectNode::NewObjectNode(const char *what)
{
	makestr(Name, _("New Object"));
	makestr(type, "NewObject");

	// enum of all objects	
	def = dynamic_cast<ObjectDef*>(menuKeeper.GetObject());
	int whati = 0;
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

		for (int c = 0; c < stylemanager.getNumFields(); c++) {
			ObjectDef *d = stylemanager.getField(c);
			if (d->format == VALUE_Class)	{
				// DBG if (!d->newfunc && !d->stylefunc) cerr << "NewObjectNode: *** Warning! Missing constructor for "<<d->name<<". FIXME!!"<<endl;
				// DBG else cerr << "NewObjectNode:  newfunc: "<<(d->newfunc ? "yes": "no ")<<" ofunc: "<<(d->stylefunc? "yes  ": "no   ")<<d->name<<endl;
				if (what && !strcmp(d->name, what)) whati = i;
				def->pushEnumValue(d->name, d->Name, d->description);
				i++;
			}
		}

		def->Sort();
		
		menuKeeper.SetObject(def, false);
	} else def->inc_count();

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
	if (!o || (o && strcmp(what, o->whattype()))) { //missing or different out, recreate
		ObjectDef *d = stylemanager.FindDef(what);
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
	}

	if (!properties.e[1]->GetData()) return -1;
	return NodeBase::Update();
}

//0 ok, -1 bad ins, 1 just needs updating
int NewObjectNode::GetStatus()
{
	if (!properties.e[1]->GetData()) return -1;
	return NodeBase::GetStatus();
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

/*! Update the prop field arrangement. Does NOT update the actual values here. Use the usual Update() for that.
 */
void ObjectInfoNode::UpdateProps()
{
	int added = 0;

	if (obj) {
		obj->inc_count();
		ObjectDef *def = obj->GetObjectDef(); //warning: shawdows NodeBase::def
		
		ObjectDef *field;

		for (int c=0; c<def->getNumFields(); c++) {
			field = def->getField(c);

			if (field->format==VALUE_Function || field->format==VALUE_Class || field->format==VALUE_Operator || field->format==VALUE_Namespace)
				continue;

			// update property with new name, Name, desc...
			NodeProperty *prop = nullptr;
			// Value *val = obj->dereference(field->name,-1);
			if (added+3 < properties.n-1) {
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
		RemoveProperty(properties.e[c]);
	}

	Wrap();
}

int ObjectInfoNode::Update()
{
	Value *o = properties.e[0]->GetData();
	if (!o) {
		properties.e[1]->SetData(nullptr, 0); //id
		properties.e[2]->SetData(nullptr, 0); //type
		return -1;
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
	}

	for (int c=3; c<properties.n; c++) {
		Value *v = o->dereference(properties.e[c]->Name(), -1);
		properties.e[c]->is_editable = false; // hack to recover from property read in not checking fro editability
		properties.e[c]->SetData(v, 1);
	}

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
	}

	return 0; //do nothing here!
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
	if (!v) return -1;
	if (v != shouldbe) return 1;

	return 0;
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

	return 0; //do nothing here!
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
	if (v != shouldbe) return -1;

	return 0;
}


//------------------------------ RerouteNode --------------------------------------------

/*! \class RerouteNode
 * Map arrays to other arrays using a special Swizzle interface.
 */

class RerouteNode : public NodeBase
{
  public:
	RerouteNode();
	virtual ~RerouteNode();
	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int Wrap();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref) { return new RerouteNode(); }
};

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


//--------------------------- SetupDefaultNodeTypes() -----------------------------------------

/*! Install default built in node types to factory.
 * This is called when the NodeGroup::NodeFactory singleton is created.
 */
int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory)
{

	//------------------ Misc value types -------------

	 //--- ColorNode
	factory->DefineNewObject(getUniqueNumber(), "Color",    newColorNode,  NULL, 0);

	 //--- GradientStrip
	factory->DefineNewObject(getUniqueNumber(), "Gradient", newGradientNode,  NULL, 0);
	 //--- GradientTransform
	factory->DefineNewObject(getUniqueNumber(), "GradientTransform", newGradientTransformNode,  NULL, 0);

	 //--- Images
	factory->DefineNewObject(getUniqueNumber(), "Image", newImageNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "ImageFile", ImageFileNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "ImageInfo", newImageInfoNode,  NULL, 0);

	 //--- CurveNodes
	factory->DefineNewObject(getUniqueNumber(), "Curve",         newCurveNode,  NULL, 1);
	factory->DefineNewObject(getUniqueNumber(), "CurveTransform",newCurveNode,  NULL, 0);


	//------------------ Object maintenance -------------

	 //--- TypeInfoNode
	factory->DefineNewObject(getUniqueNumber(), "Object/TypeInfo", newTypeInfoNode,  NULL, 0);

	 //--- NewObjectNode
	factory->DefineNewObject(getUniqueNumber(), "Object/NewObject", NewObjectNode::NewNode,  NULL, 0);

	 //--- ModifyObjectNode
	factory->DefineNewObject(getUniqueNumber(), "Object/ModifyObject",  ModifyObjectNode::NewNode,  NULL, 0);

	factory->DefineNewObject(getUniqueNumber(), "Object/ObjectInfo",  ObjectInfoNode::NewNode,  NULL, 0);


	//------------------ Groups -------------

	 //--- NodeGroup
	factory->DefineNewObject(getUniqueNumber(), "NodeGroup",newNodeGroup,  NULL, 0);

	 //--- ObjectNodes
	factory->DefineNewObject(getUniqueNumber(), "Object In", newObjectInNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Object Out",newObjectOutNode, NULL, 0);

	//------------------ Bounds and transforms -------------

	 //--- RectangleNode
	factory->DefineNewObject(getUniqueNumber(), "Rectangle",newRectangleNode,  NULL, 0);

	 //--- Affine nodes
	factory->DefineNewObject(getUniqueNumber(), "Math/Affine", newAffineNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/Affine2",newAffineNode2, NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/AffineExpand",newAffineExpandNode, NULL, 0);


	//------------------ Math -------------

	 //--- MathNode1
	factory->DefineNewObject(getUniqueNumber(), "Math/Math1",     newMathNode1,   NULL, 0);

	 //--- MathNode2
	factory->DefineNewObject(getUniqueNumber(), "Math/Math2",     newMathNode2,   NULL, 0);

	 //--- DoubleNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Value",    newDoubleNode, NULL, 0);

	 //--- FlatvectorNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Vector2", newFlatvectorNode, NULL, 0);

	 //--- SpacevectorNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Vector3", newSpacevectorNode, NULL, 0);

	 //--- QuaternionNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Quaternion", newQuaternionNode, NULL, 0);

	 //--- ExpandVectorNode
	factory->DefineNewObject(getUniqueNumber(), "Math/ExpandVector", newExpandVectorNode, NULL, 0);

	 //--- RandomNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Random",newRandomNode,  NULL, 0);

	 //--- LerpNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Lerp",newLerpNode,  NULL, 0);

	 //--- MapRangeNodes
	factory->DefineNewObject(getUniqueNumber(), "Math/MapToRange",  newMapToRangeNode,    NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Math/MapFromRange",newMapFromRangeNode,  NULL, 0);

	 //--- MathConstantNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Constant",newMathConstants,  NULL, 0);

	 //--- ExpressionNode
	factory->DefineNewObject(getUniqueNumber(), "Math/Expression",ExpressionNode::NewNode,  NULL, 0);


	//------------------ String -------------

	 //--- ConcatNode
	factory->DefineNewObject(getUniqueNumber(), "Strings/Concat",newConcatNode,  NULL, 0);

	 //--- StringNode
	factory->DefineNewObject(getUniqueNumber(), "Strings/String",newStringNode,  NULL, 0);

	 //--- SliceNode
	factory->DefineNewObject(getUniqueNumber(), "Strings/Slice",newSliceNode,  NULL, 0);

	 //--- TextFileNode
	factory->DefineNewObject(getUniqueNumber(), "Strings/TextFromFile",TextFileNode::NewNode,  NULL, 0);


	 //-------------------- FILTERS -------------

	 //--- TransformAffineNodes
	factory->DefineNewObject(getUniqueNumber(), "Filters/TransformAffine", newTransformAffineNode,  NULL, 0);


	 //-------------------- THREADS -------------

	 //--- ThreadNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/Thread",newThreadNode,  NULL, 0);

	 //--- LoopNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/Loop",newLoopNode,  NULL, 0);

	 //--- IfNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/If",newIfNode,  NULL, 0);

	 //--- ForkNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/Fork",newForkNode,  NULL, 0);

	 //--- DelayNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/Delay",newDelayNode,  NULL, 0);

	 //--- SetVariableNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/SetVariable",newSetVariableNode,  NULL, 0);

	 //--- GetVariableNode
	factory->DefineNewObject(getUniqueNumber(), "Threads/GetVariable",newGetVariableNode,  NULL, 0);


	 //-------------------- Resources -------------

	 //--- set laidout->globals
	factory->DefineNewObject(getUniqueNumber(), "Resources/SetGlobal",SetGlobalNode::NewNode,  NULL, 0);

	 //--- get laidout->globals
	factory->DefineNewObject(getUniqueNumber(), "Resources/GetGlobal",GetGlobalNode::NewNode,  NULL, 0);


	 //-------------------- Specials -------------

	 //--- Reroute
	factory->DefineNewObject(getUniqueNumber(), "Reroute",RerouteNode::NewNode,  NULL, 0);


	//Register nodes for DrawableObject filters:
	RegisterFilterNodes(factory);

	//Register DrawableObject nodes:
	SetupDataObjectNodes(factory);
	

	return 0;
}



} //namespace Laidout

