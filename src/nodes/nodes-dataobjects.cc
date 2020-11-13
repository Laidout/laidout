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
#include "../dataobjects/limagedata.h"
#include "../dataobjects/lpathsdata.h"
#include "../dataobjects/bboxvalue.h"
#include "../dataobjects/affinevalue.h"
#include "../dataobjects/pointsetvalue.h"
#include "../dataobjects/imagevalue.h"
#include "../core/objectiterator.h"

#include <unistd.h>


//template implementation
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


namespace Laidout {


//------------------------ LImageDataNode ------------------------

/*! \class Node for LImageData.
 */

class LImageDataNode : public NodeBase
{
  public:
	LImageData *imagedata;

	LImageDataNode();
	virtual ~LImageDataNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
	virtual int UpdatePreview();
};


LImageDataNode::LImageDataNode()
{
	makestr(Name, _("Image Data"));
	makestr(type, "Drawable/ImageData");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "transform",  new AffineValue(),1, _("Transform")));
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "image", new ColorValue(1.,1.,1.,1.),1, _("Image")));

	imagedata = new LImageData();
	AddProperty(new NodeProperty(NodeProperty::PROP_Output,true, "out", imagedata,1, NULL, 0, false)); 
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

	//if (col) imagedata->SetImageAsColor(col->color.Red(), col->color.Green(), col->color.Blue(), col->color.Alpha());
	//else
	imagedata->m(a->m());
	imagedata->SetImage(image->image, nullptr);

	return NodeBase::Update();
}

int LImageDataNode::GetStatus()
{
	if (!dynamic_cast<Affine*>(properties.e[0]->GetData())) return -1;
	if (!dynamic_cast<ColorValue*>(properties.e[1]->GetData())
		&& !dynamic_cast<ImageValue*>(properties.e[1]->GetData())) return -1;
	if (!properties.e[2]) return 1;

	return NodeBase::GetStatus();
}

int LImageDataNode::UpdatePreview()
{
	LaxImage *img = imagedata->GetPreview();
	if (img == nullptr) img = imagedata->image;
	if (img) {
		if (img != total_preview) {
			if (total_preview) total_preview->dec_count();
			total_preview = img;
			total_preview->inc_count();
		}
	}
	return 1;
}


Laxkit::anObject *newLImageDataNode(int p, Laxkit::anObject *ref)
{
	return new LImageDataNode();
}


//------------------------ LImageDataInfoNode ------------------------

/*! \class Node for LImageDataInfo.
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

/*! \class Node for constructing PathsData objects..
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



//------------------------ PointsToDotsNode ------------------------

/*! \class Node to convert a PointSet to a dots in a PathsData.
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

/*! \class Convert PointSet to a bar chart, sorted by x.
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

	for (int c=0; c<dup.NumPoints(); c++) {
		out->pushEmpty();
		double v = (dup.points.e[c]->p.y - databounds->miny) / databounds->boxheight();
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

/*! \class Node to convert a PointSet to a bars in a PathsData, sorted by point index order.
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
		
	} else if (settype == HexGrid) {
		makestr(type, "Points/HexGrid");
		makestr(Name, _("Hex Point Grid"));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "o", new FlatvectorValue(),1, _("Center"),   NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "length", new DoubleValue(1),1,  _("Edge"),  NULL));
		AddProperty(new NodeProperty(NodeProperty::PROP_Input,  true, "num", new IntValue(4),1,  _("Num on edge"), NULL));
		
	} else if (settype == RandomSquare) {
		makestr(type, "Points/RandomSquare");
		makestr(Name, _("Random Square"));
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
	const char *sig = "nnnnnnvnn   vnnnn vnnn  ";
	int sigoff = 0;

		// Grid,    // x,y
		// HexGrid, // num on edge    
		// RandomSquare, // n
		// RandomCircle, // 
		
#define OFFGRID     0
#define OFFHEX      6
#define OFFRSQUARE  12
#define OFFRCIRCLE  18

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
		if (!isnum) { makestr(error_message, _("Bad num x")); return -1; }
		int ny = getNumberValue(properties.e[5]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad num y")); return -1; }

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
		
		int n = getNumberValue(properties.e[3]->GetData(), &isnum);
		if (!isnum) { makestr(error_message, _("Bad num points")); return -1; }

		int seed = getNumberValue(properties.e[4]->GetData(), &isnum);
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
	LPathsData *pathsdata = dynamic_cast<LPathsData*>(properties.e[properties.n-1]->GetData());
	if (!pathsdata) return 0;
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
	const char *sig = "     nnnn n    nnnn snnn ssnnns    snnn ssnnn";
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

	for (int c=0; c<properties.n-1; c++) {
		Value *data = properties.e[c+1]->GetData();
		if (!data) { types[c] = ' '; continue; }
		stype = data->whattype();
		if (isNumberType(data, nullptr)) types[c] = 'n';
		else if (!strcmp(stype, "StringValue")) types[c] = 's';
		else types[c] = ' ';
	}
	for (int c=properties.n-2; c<5; c++) types[c] = ' ';
	types[5] = '\0';

	if (strncmp(sig+sigoff, types, 5)) return -1;

	return NodeBase::GetStatus();
}

//0 ok, -1 bad ins, 1 just needs updating
int PathGeneratorNode::Update()
{
	makestr(error_message, NULL);
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

	if (pathtype == Square) {
		if (path) path->appendRect(0,0,1,1);
		else {
			set->AddPoint(flatpoint(0,0));
			set->AddPoint(flatpoint(1,0));
			set->AddPoint(flatpoint(1,1));
			set->AddPoint(flatpoint(0,1));
		}

	} else if (pathtype == Rectangle) {
		// path->clear();
		int isnum;
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
		int isnum = 0;
		int n = getNumberValue(properties.e[1]->GetData(), &isnum);
		if (!isnum || n <= 0) {makestr(error_message, _("Bad number of points")); return -1; }

		if (path) path->appendEllipse(flatpoint(), 1,1, 2*M_PI, 0, n, 1);
		else set->CreateCircle(n, 0,0, 1);

	} else if (pathtype == Polygon) {
		// make an n sided polygon
		// path->clear();
		
		int isnum = 0;
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

	path->FindBBox();
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

/*! \class Node to get a reference to other DrawableObjects in same page.
 */

class FindDrawableNode : public NodeBase
{
  public:
	FindDrawableNode();
	virtual ~FindDrawableNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

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
	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "pattern", NULL,1, _("Pattern"), _("Pattern to search for")));

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

/*! Return 0 for no error and everything up to date.
 * -1 means bad inputs and node in error state.
 * 1 means needs updating.
 */
int FindDrawableNode::Update()
{
	DrawableObject *base = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!base) return -1;

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
	return NodeBase::Update();
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


//------------------------ DrawableInfoNode ------------------------

/*! \class Node for basic DrawableObject information, like name, parent, transform, and bounds..
 */

class DrawableInfoNode : public NodeBase
{
  public:
	DrawableInfoNode();
	virtual ~DrawableInfoNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	static Laxkit::anObject *NewNode(int p, Laxkit::anObject *ref);
};

Laxkit::anObject *DrawableInfoNode::NewNode(int p, Laxkit::anObject *ref)
{
	return new DrawableInfoNode();
}

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

int DrawableInfoNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;

	return NodeBase::GetStatus();
}



//--------------------------- SetupDataObjectNodes() -----------------------------------------

/*! Install default built in node types to factory.
 * This is called when the NodeGroup::NodeFactory singleton is created.
 */
int SetupDataObjectNodes(Laxkit::ObjectFactory *factory)
{
	 //--- Drawables
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageData",     newLImageDataNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageDataInfo", newLImageDataInfoNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/DrawableInfo",  DrawableInfoNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Drawable/FindDrawable",  FindDrawableNode::NewNode,  NULL, 0);

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

	factory->DefineNewObject(getUniqueNumber(), "Points/Grid",         newPointsGridNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/HexGrid",      newPointsHexNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/RandomSquare", newPointsRandomSquareNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/RandomCircle", newPointsRandomRadialNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToDots", PointsToDotsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToBars", PointsToBarsNode::NewNode,  NULL, 0);
	factory->DefineNewObject(getUniqueNumber(), "Points/PointsToBarChart", PointsToBarChartNode::NewNode,  NULL, 0);

	return 0;
}

} //namespace Laidout

