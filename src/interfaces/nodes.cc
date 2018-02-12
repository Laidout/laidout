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

//------------ FlatvectorNode

class VectorNode : public NodeBase
{
  public:
	int dims;
	VectorNode(int dimensions); //up to 4
	virtual ~VectorNode();
	virtual int Update();
	virtual int GetStatus();
};

VectorNode::VectorNode(int dimensions)
{
	dims = dimensions;
	//node->Id("Value");
	Name = newstr(dimensions==2 ? _("Vector2") : _("Vector3"));
	type = newstr(dimensions==2 ? "flatvector" : "spacevector");

	const char *labels[] = { _("x"), _("y"), _("z"), _("w") };
	const char *names[]  = {   "x" ,   "y" ,   "z" ,   "w"  };

	for (int c=0; c<dims; c++) {
		AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, names[c], new DoubleValue(0), 1, labels[c])); 
	}
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, _("V"), new FlatvectorValue(), 1)); 
}

VectorNode::~VectorNode()
{
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
		if      (dims == 2) properties.e[2]->data=new FlatvectorValue(flatvector(vs));
		else if (dims == 3) properties.e[3]->data=new SpacevectorValue(spacevector(vs));
	} else {
		//assume correct format already in prop
		if      (dims == 2) dynamic_cast<FlatvectorValue* >(properties.e[2]->data)->v = flatvector(vs);
        else if (dims == 3) dynamic_cast<SpacevectorValue*>(properties.e[3]->data)->v = spacevector(vs);
	}
	properties.e[dims]->modtime = time(NULL);

	return NodeBase::Update();
}

Laxkit::anObject *newFlatvectorNode(int p, Laxkit::anObject *ref)
{
	return new VectorNode(2);
}

Laxkit::anObject *newSpacevectorNode(int p, Laxkit::anObject *ref)
{
	return new VectorNode(3);
}


//------------ RectangleNode

class RectangleNode : public NodeBase
{
  public:
	RectangleNode();
	virtual ~RectangleNode();
	virtual int Update();
	virtual int GetStatus();
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

	properties.e[4]->modtime = time(NULL);

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


//------------ MathNode

class MathNode : public NodeBase
{
  public:
	static SingletonKeeper mathnodekeeper; //the def for the op enum
	static ObjectDef *GetMathNodeDef() { return dynamic_cast<ObjectDef*>(mathnodekeeper.GetObject()); }

	int operation; //see MathNodeOps
	int numargs;
	double a,b,result;
	MathNode(int op=0, double aa=0, double bb=0);
	virtual ~MathNode();
	virtual int Update();
	virtual int GetStatus();
};


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
	OP_Clamp_To_1, // [0..1]
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

/*! Create and return a fresh instance of the def for a MathNode op.
 */
ObjectDef *DefineMathNodeDef()
{
	ObjectDef *def = new ObjectDef("MathNodeDef", _("Math Node Def"), NULL,NULL,"enum", 0);

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
	def->pushEnumValue("RandomR",    _("Random"),       _("Random(seed,max)"), OP_RandomRange );

	def->pushEnumValue("And"        ,_("And"       ),   _("And"       ),  OP_And         );
	def->pushEnumValue("Or"         ,_("Or"        ),   _("Or"        ),  OP_Or          );
	def->pushEnumValue("Xor"        ,_("Xor"       ),   _("Xor"       ),  OP_Xor         );
	def->pushEnumValue("ShiftLeft"  ,_("ShiftLeft" ),   _("ShiftLeft" ),  OP_ShiftLeft   );
	def->pushEnumValue("ShiftRight" ,_("ShiftRight"),   _("ShiftRight"),  OP_ShiftRight  );

	 //vector math, 2 args
	def->pushEnumValue("Dot"            ,_("Dot"),            _("Dot"),            OP_Dot            );
	def->pushEnumValue("Cross"          ,_("Cross"),          _("Cross product"),  OP_Cross          );
	def->pushEnumValue("Perpendicular"  ,_("Perpendicular"),  _("Part of A perpendicular to B"),OP_Perpendicular);
	def->pushEnumValue("Parallel"       ,_("Parallel"),       _("Part of A parallel to B"), OP_Parallel);
	def->pushEnumValue("Angle_Between"  ,_("Angle Between"),  _("Angle Between, 0..pi"),    OP_Angle_Between  );
	def->pushEnumValue("Angle2_Between" ,_("Angle2 Between"), _("Angle2 Between, 0..2*pi"), OP_Angle2_Between );


	 //1 argument
	//def->pushEnumValue("Clamp_To_1"         ,_("Clamp To 1"),    _("Clamp To 1"),    OP_Clamp_To_1    );
	//def->pushEnumValue("AbsoluteValue"      ,_("AbsoluteValue"), _("AbsoluteValue"), OP_AbsoluteValue );
	//def->pushEnumValue("Negative"           ,_("Negative"),      _("Negative"),      OP_Negative      );
	//def->pushEnumValue("Not"                ,_("Not"),           _("Not"),           OP_Not           );
	//def->pushEnumValue("Sqrt"               ,_("Square root"),   _("Square root"),   OP_Sqrt          );
	//def->pushEnumValue("Sqn"                ,_("Sign"),          _("Sign: 1, -1 or 0"),OP_Sgn         );
	//def->pushEnumValue("Radians_To_Degrees" ,_("Radians To Degrees"), _("Radians To Degrees"), OPRadians_To_Degrees );
	//def->pushEnumValue("Degrees_To_Radians" ,_("Degrees To Radians"), _("Degrees To Radians"), OPDegrees_To_Radians );
	//def->pushEnumValue("Sin"                ,_("Sin"),           _("Sin"),           OP_Sin           );
	//def->pushEnumValue("Cos"                ,_("Cos"),           _("Cos"),           OP_Cos           );
	//def->pushEnumValue("Tan"                ,_("Tan"),           _("Tan"),           OP_Tan           );
	//def->pushEnumValue("Asin"               ,_("Asin"),          _("Asin"),          OP_Asin          );
	//def->pushEnumValue("Acos"               ,_("Acos"),          _("Acos"),          OP_Acos          );
	//def->pushEnumValue("Atan"               ,_("Atan"),          _("Atan"),          OP_Atan          );
	//def->pushEnumValue("Sinh"               ,_("Sinh"),          _("Sinh"),          OP_Sinh          );
	//def->pushEnumValue("Cosh"               ,_("Cosh"),          _("Cosh"),          OP_Cosh          );
	//def->pushEnumValue("Tanh"               ,_("Tanh"),          _("Tanh"),          OP_Tanh          );
	//def->pushEnumValue("Asinh"              ,_("Asinh"),         _("Asinh"),         OP_Asinh         );
	//def->pushEnumValue("Acosh"              ,_("Acosh"),         _("Acosh"),         OP_Acosh         );
	//def->pushEnumValue("Atanh"              ,_("Atanh"),         _("Atanh"),         OP_Atanh         );
	 ////vector math, 1 arg
	//def->pushEnumValue("Norm"               ,_("Norm"),          _("Length of vector"),OP_Norm        );
	//def->pushEnumValue("Norm2"              ,_("Norm2"),         _("Square of length"),OP_Norm2       );
	//def->pushEnumValue("Flip"               ,_("Flip"),          _("Flip"),          OP_Flip          );
	//def->pushEnumValue("Normalize"          ,_("Normalize"),     _("Normalize"),     OP_Normalize     );
	//def->pushEnumValue("Angle"              ,_("Angle"),         _("Angle"),         OP_Angle         );
	//def->pushEnumValue("Angle2"             ,_("Angle2"),        _("Angle2"),        OP_Angle2        );


	 //3 arguments
	//def->pushEnumValue("Lerp" , _("Lerp"),  _("Lerp"),  OP_Lerp  );
	//def->pushEnumValue("Clamp" ,_("Clamp"), _("Clamp"), OP_Clamp );

	return def;
}

SingletonKeeper MathNode::mathnodekeeper(DefineMathNodeDef(), true);


MathNode::MathNode(int op, double aa, double bb)
{
	type = newstr("Math");
	Name = newstr(_("Math"));

	a=aa;
	b=bb;
	operation = op;
	numargs = 2;

	ObjectDef *enumdef = GetMathNodeDef();
	enumdef->inc_count();


	EnumValue *e = new EnumValue(enumdef, 0);
	enumdef->dec_count();

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, false, "Op", e, 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "A", new DoubleValue(a), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "B", new DoubleValue(b), 1));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "Result", NULL, 0, NULL, NULL, 0, false));

	//NodeProperty(PropertyTypes input, bool linkable, const char *nname, Value *ndata, int absorb_count,
					//const char *nlabel=NULL, const char *ntip=NULL, int info=0, bool editable);

	Update();
}

MathNode::~MathNode()
{
//	if (mathnodedef) {
//		if (mathnodedef->dec_count()<=0) mathnodedef=NULL;
//	}
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
int MathNode::GetStatus()
{
	Value *va = properties.e[1]->GetData();
	Value *vb = properties.e[2]->GetData();

	int isnum=0;
	a = getNumberValue(va, &isnum);
	if (!isnum) return -1;
	b = getNumberValue(vb, &isnum);
	if (!isnum) return -1;

	if ((operation==OP_Divide || operation==OP_Mod) && b==0) return -1;
	if (a==0 || (a<0 && fabs(b)-fabs(int(b))<1e-10)) return -1;
	if (!properties.e[3]->data) return 1;
	return 0;
}

int MathNode::Update()
{
	Value *valuea = properties.e[1]->GetData();
	Value *valueb = properties.e[2]->GetData();
	int aisnum=0, bisnum=0;
	a = getNumberValue(valuea, &aisnum);
	b = getNumberValue(valueb, &bisnum);

	//flatvector fva;
	//spacevector sva;
	//string
	//if (valuea->type() == VALUE_Flatvector

	EnumValue *ev = dynamic_cast<EnumValue*>(properties.e[0]->GetData());
	ObjectDef *def = ev->GetObjectDef();
	const char *nm = NULL;
	operation=OP_None;
	def->getEnumInfo(ev->value, &nm, NULL,NULL, &operation);

	//DBG cerr <<"MathNode::Update op: "<<operation<<" vs enum id: "<<id<<endl;

	if      (operation==OP_Add) result = a+b;
	else if (operation==OP_Subtract) result = a-b;
	else if (operation==OP_Multiply) result = a*b;
	else if (operation==OP_Divide) {
		if (b!=0) result = a/b;
		else {
			result=0;
			return -1;
		}

	} else if (operation==OP_Mod) {
		if (b!=0) result = a-b*int(a/b);
		else {
			result=0;
			return -1;
		}
	} else if (operation==OP_Power) {
		if (a==0 || (a<0 && fabs(b)-fabs(int(b))<1e-10)) {
			 //0 to a power fails, as does negative numbers raised to non-integer powers
			result=0;
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
	}

	if (!properties.e[3]->data) properties.e[3]->data=new DoubleValue(result);
	else dynamic_cast<DoubleValue*>(properties.e[3]->data)->d = result;
	properties.e[3]->modtime = time(NULL);

	return NodeBase::Update();
}

Laxkit::anObject *newMathNode(int p, Laxkit::anObject *ref)
{
	return new MathNode();
}


//------------ FunctionNode

// sin cos tan asin acos atan sinh cosh tanh log ln abs sqrt int floor ceil factor random randomint
// fraction negative reciprocal clamp scale(old_range, new_range)
// pi tau e
//
//class FunctionNode : public NodeBase
//{
//  public:
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




//--------------------------- SetupDefaultNodeTypes() -----------------------------------------

/*! Install default built in node types to factory.
 */
int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory)
{
	 //--- ColorNode
	factory->DefineNewObject(getUniqueNumber(), "Color",    newColorNode,  NULL, 0);

	 //--- ImageNode
	factory->DefineNewObject(getUniqueNumber(), "NewImage", newImageNode,  NULL, 0);

	 //--- MathNode
	factory->DefineNewObject(getUniqueNumber(), "Math",     newMathNode,   NULL, 0);

	 //--- DoubleNode
	factory->DefineNewObject(getUniqueNumber(), "Value",    newDoubleNode, NULL, 0);

	 //--- FlatvectorNode
	factory->DefineNewObject(getUniqueNumber(), "Vector2", newFlatvectorNode, NULL, 0);

	 //--- SpacevectorNode
	factory->DefineNewObject(getUniqueNumber(), "Vector3", newSpacevectorNode, NULL, 0);

	 //--- NodeGroup
	factory->DefineNewObject(getUniqueNumber(), "NodeGroup",newNodeGroup,  NULL, 0);

	 //--- RectangleNode
	factory->DefineNewObject(getUniqueNumber(), "Rectangle",newRectangleNode,  NULL, 0);

	return 0;
}



} //namespace Laidout

