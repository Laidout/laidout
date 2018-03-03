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
#include "nodes.h"
#include "nodeinterface.h"
#include "../calculator/calculator.h"


//template implementation
#include <lax/lists.cc>


namespace Laidout {



//-------------------------------------- Define the built in nodes types --------------------------


//------------ DoubleNode

Laxkit::anObject *newDoubleNode(int p, Laxkit::anObject *ref)
{
	NodeBase *node = new NodeBase;
	//node->Id("Value");
	makestr(node->Name, _("Value"));
	makestr(node->type, "Value");
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("V"), new DoubleValue(0), 1));
	return node;
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
	}

	//if (!properties.e[2]->data) return 1;

	if (!properties.e[4]->data) properties.e[4]->data = new BBoxValue(vs[0], vs[0]+vs[2], vs[1], vs[1]+vs[3]);
	else {
		BBoxValue *v = dynamic_cast<BBoxValue*>(properties.e[4]->data);
		v->setbounds(vs[0], vs[0]+vs[2], vs[1], vs[1]+vs[3]);
	}

	properties.e[4]->modtime = times(NULL);

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
	int atype; //0 == a,b,c,d,e,f, 1= posx, posy, scalex, scaley, anglex, angley_off_90, 2 = xv, yv, pv, 
	AffineNode(int ntype, const double *values);
	virtual ~AffineNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

AffineNode::AffineNode(int ntype, const double *values)
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

	if (atype == 0) {
		makestr(Name, _("Affine"));
		makestr(type,   "Affine");
		labels = labels1;
		names  = names1;
		if (!values) values = v1;

//	} else if (atype == 1) {
	} else {
		makestr(Name, _("Affine2"));
		makestr(type,   "Affine2");
		labels = labels2;
		names  = names2;
		if (!values) values = v2;

//	} else {
//		makestr(Name, _("Affine Vectors"));
//		makestr(type,   "AffineV" );
//		labels = { _("X"), _("Y"), _("Position"), NULL, NULL, NULL };
	}

	for (int c=0; c<6; c++) {
		if (!labels[c]) break;
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, names[c],
					new DoubleValue(values ? values[c] : 0), 1, labels[c])); 
	}

	Value *v = new AffineValue();
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Affine"), v,1)); 

}

AffineNode::~AffineNode()
{
}

NodeBase *AffineNode::Duplicate()
{
	double vs[6];
	int isnum;
	for (int c=0; c<6; c++) {
		vs[c] = getNumberValue(properties.e[c]->GetData(), &isnum);
	}

	AffineNode *newnode = new AffineNode(atype, vs);
	newnode->DuplicateBase(this);
	return newnode;
}

/*! -1 for bad values. 0 for ok, 1 for just needs update.
 */
int AffineNode::GetStatus()
{
	int isnum;
	//double v[6];
	for (int c=0; c<6; c++) {
		getNumberValue(properties.e[c]->GetData(), &isnum);
		if (!isnum) return -1;
	}

	// maybe check this is invertible
//	if (m[0]*m[3]-m[1]*m[2] == 0) {
//		 //degenerate matrix!
//		if (!error_message) makestr(error_message, _("Bad matrix"));
//		return -1;
//	} else if (error_message) makestr(error_message, NULL);

	if (!properties.e[6]->data) return 1;

	return NodeBase::GetStatus(); //default checks mod times
}

int AffineNode::Update()
{
	double vs[6];
	int isnum;
	for (int c=0; c<6; c++) {
		vs[c] = getNumberValue(properties.e[c]->GetData(), &isnum);
	}

	AffineValue *v = dynamic_cast<AffineValue*>(properties.e[6]->GetData());
	if (!v) {
		v = new AffineValue;
	} else v->inc_count();

	if (atype == 1) {
		//convert scale, angle, pos to vectors
		v->setBasics(vs[4], vs[5], vs[0], vs[1], vs[2]*M_PI/180, vs[3]*M_PI/180);
	}

	properties.e[6]->SetData(v, 1);
	v->m(vs);

	return NodeBase::Update();
}

Laxkit::anObject *newAffineNode(int p, Laxkit::anObject *ref)
{
	return new AffineNode(0, NULL);
}

Laxkit::anObject *newAffineNode2(int p, Laxkit::anObject *ref)
{
	return new AffineNode(1, NULL);
}

//Laxkit::anObject *newAffineNodeV(int p, Laxkit::anObject *ref)
//{
//	return new AffineNode(2);
//}

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

	int operation; //see MathNodeOps
	MathNode1(int op=0, double aa=0);
	virtual ~MathNode1();
	virtual int UpdateThisOnly();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

class MathNode2 : public NodeBase
{
  public:
	static SingletonKeeper mathnodekeeper; //the def for the op enum
	static ObjectDef *GetMathNode2Def() { return dynamic_cast<ObjectDef*>(mathnodekeeper.GetObject()); }

	int last_status;
	clock_t status_time;

	int operation; //see MathNode2Ops
	int numargs;
	double a,b,result;
	MathNode2(int op=0, double aa=0, double bb=0);
	virtual ~MathNode2();
	virtual int UpdateThisOnly();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
};

SingletonKeeper MathNode1::mathnodekeeper(DefineMathNode1Def(), true);
SingletonKeeper MathNode2::mathnodekeeper(DefineMathNode2Def(), true);

//------MathNode1

MathNode1::MathNode1(int op, double aa)
{
	type = newstr("Math1");
	Name = newstr(_("Math 1"));

	last_status = 1;
	status_time = 0;

	operation = op;

	ObjectDef *enumdef = GetMathNode1Def();
	enumdef->inc_count();


	EnumValue *e = new EnumValue(enumdef, 0);
	enumdef->dec_count();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "Op", e, 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "A", new DoubleValue(aa), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Result", NULL,0, _("Result"), NULL, 0, false));

	last_status = Update();
	status_time = MostRecentIn(NULL);
}

MathNode1::~MathNode1()
{
}

NodeBase *MathNode1::Duplicate()
{
	MathNode1 *newnode = new MathNode1;
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
	const char *nm = NULL;
	double result=0;
	operation = OP_None;
	def->getEnumInfo(ev->value, &nm, NULL,NULL, &operation);


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
	operation = op;
	numargs = 2;

	ObjectDef *enumdef = GetMathNode2Def();
	enumdef->inc_count();


	EnumValue *e = new EnumValue(enumdef, 0);
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
	MathNode2 *newnode = new MathNode2;
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
	const char *nm = NULL;
	operation = OP_None;
	def->getEnumInfo(ev->value, &nm, NULL,NULL, &operation);


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


//------------ FunctionNode

// sin cos tan asin acos atan sinh cosh tanh log ln abs sqrt int floor ceil factor random randomint
// fraction negative reciprocal clamp scale(old_range, new_range)
// pi tau e
//
//class FunctionNode : public NodeBase
//{
//  public:
//	virtual NodeBase *Duplicate();
//};


//------------ ImageNode

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
	NodeBase *node = new NodeBase;
	//node->Id("Image");
	
	makestr(node->type, "NewImage");
	makestr(node->Name, _("New Image"));
	//node->AddProperty(new NodeProperty(true, true, _("Filename"), new FileValue("."), 1)); 
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Width"),    new IntValue(100), 1)); 
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Height"),   new IntValue(100), 1)); 
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Channels"), new IntValue(4),   1)); 

	ObjectDef *enumdef = GetImageDepthDef();
	EnumValue *e = new EnumValue(enumdef, 0);
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Depth"), e, 1)); 

	node->AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, _("Initial Color"), new ColorValue("#ffffff"), 1)); 
	node->AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("Color"), NULL, 1)); 
	//depth: 8, 16, 24, 32, 32f, 64f
	//format: gray, graya, rgb, rgba
	//backend: raw, default, gegl, gmic, gm, cairo
	return node;
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
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Out", new StringValue(ns),1, _("Out")));
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
	virtual NodeBase *Execute(NodeThread *thread);
};

ThreadNode::ThreadNode()
{
	makestr(type, "Thread");
	makestr(Name, _("Thread"));

	AddProperty(new NodeProperty(NodeProperty::PROP_Exec_Out, true, "out",  NULL,1, _("Out")));
}

ThreadNode::~ThreadNode()
{
}


NodeBase *ThreadNode::Duplicate()
{
	ThreadNode *node = new ThreadNode();
	return node;
}

NodeBase *ThreadNode::Execute(NodeThread *thread)
{
	if (!properties.e[0]->connections.n) return NULL;
	return properties.e[0]->connections.e[0]->to;
}


Laxkit::anObject *newThreadNode(int p, Laxkit::anObject *ref)
{
	return new ThreadNode();
}


//------------------------ LoopNode ------------------------

/*! \class For loop node.
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
	virtual NodeBase *Execute(NodeThread *thread);
};


LoopNode::LoopNode(double nstart, double nend, double nstep)
{
	start   = nstart;
	end     = nend;
	step    = nstep;
	current = start;
	running = 0;

	makestr(type,   "Loop");
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

NodeBase *LoopNode::Execute(NodeThread *thread)
{
	NodeProperty *done = properties.e[1];
	NodeProperty *loop = properties.e[2];

	if ((start>end && step>=0) || (start<end && step<=0)) return NULL;

	NodeBase *next = NULL;

	if (!running) {
		 //initialize loop
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

//--------------------------- SetupDefaultNodeTypes() -----------------------------------------

/*! Install default built in node types to factory.
 */
int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory)
{
	 //--- ColorNode
	factory->DefineNewObject(getUniqueNumber(), "Color",    newColorNode,  NULL, 0);

	 //--- ImageNode
	factory->DefineNewObject(getUniqueNumber(), "NewImage", newImageNode,  NULL, 0);

	 //--- MathNode1
	factory->DefineNewObject(getUniqueNumber(), "Math1",     newMathNode1,   NULL, 0);

	 //--- MathNode2
	factory->DefineNewObject(getUniqueNumber(), "Math2",     newMathNode2,   NULL, 0);

	 //--- DoubleNode
	factory->DefineNewObject(getUniqueNumber(), "Value",    newDoubleNode, NULL, 0);

	 //--- FlatvectorNode
	factory->DefineNewObject(getUniqueNumber(), "Vector2", newFlatvectorNode, NULL, 0);

	 //--- SpacevectorNode
	factory->DefineNewObject(getUniqueNumber(), "Vector3", newSpacevectorNode, NULL, 0);

	 //--- QuaternionNode
	factory->DefineNewObject(getUniqueNumber(), "Vector4", newQuaternionNode, NULL, 0);

	 //--- ExpandVectorNode
	factory->DefineNewObject(getUniqueNumber(), "ExpandVector", newExpandVectorNode, NULL, 0);

	 //--- NodeGroup
	factory->DefineNewObject(getUniqueNumber(), "NodeGroup",newNodeGroup,  NULL, 0);

	 //--- RectangleNode
	factory->DefineNewObject(getUniqueNumber(), "Rectangle",newRectangleNode,  NULL, 0);

	 //--- Affine nodes
	factory->DefineNewObject(getUniqueNumber(), "Affine", newAffineNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Affine2",newAffineNode2, NULL, 0);

	 //--- RandomNode
	factory->DefineNewObject(getUniqueNumber(), "Random",newRandomNode,  NULL, 0);

	 //--- ConcatNode
	factory->DefineNewObject(getUniqueNumber(), "Concat",newConcatNode,  NULL, 0);

	 //--- ThreadNode
	factory->DefineNewObject(getUniqueNumber(), "Thread",newThreadNode,  NULL, 0);

	 //--- LoopNode
	factory->DefineNewObject(getUniqueNumber(), "Loop",newLoopNode,  NULL, 0);

	 //--- LerpNode
	factory->DefineNewObject(getUniqueNumber(), "Lerp",newLerpNode,  NULL, 0);

	 //--- MapRangeNodes
	factory->DefineNewObject(getUniqueNumber(), "MapToRange",  newMapToRangeNode,    NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "MapFromRange",newMapFromRangeNode,  NULL, 0);

	return 0;
}



} //namespace Laidout

