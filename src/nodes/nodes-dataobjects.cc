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
#include "nodeinterface.h"
#include "../dataobjects/limagedata.h"
#include "../dataobjects/bboxvalue.h"

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
	AffineValue *a = dynamic_cast<AffineValue*>(properties.e[0]->GetData());
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
	if (!dynamic_cast<AffineValue*>(properties.e[0]->GetData())) return -1;
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

	AddProperty(new NodeProperty(NodeProperty::PROP_Input,true, "out", nullptr,1, NULL, 0, false)); 

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


//------------------------ ObjectInfoNode ------------------------

/*! \class Node for basic DrawableObject information, like name, parent, transform, and bounds..
 */

class ObjectInfoNode : public NodeBase
{
  public:
	ObjectInfoNode();
	virtual ~ObjectInfoNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();
};


ObjectInfoNode::ObjectInfoNode()
{
	makestr(Name, _("Object Info"));
	makestr(type, "Drawable/ObjectInfo");

	AddProperty(new NodeProperty(NodeProperty::PROP_Input, true, "in",  NULL,1, _("In")));

	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "name",      new StringValue(),1, _("Name"),     nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "parent",    nullptr,1, _("Parent"),     nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "transform", new AffineValue(),1, _("Transform"),nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "fulltransform", new AffineValue(),1, _("Full Transform"),nullptr, 0, false));
	AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "bounds",    new BBoxValue(),1,   _("Bounds"),   nullptr, 0, false));
	//AddProperty(new NodeProperty(NodeProperty::PROP_Output, true, "clippath", nullptr,1, _("ClipPath"), nullptr,0,false));
}

ObjectInfoNode::~ObjectInfoNode()
{
}

NodeBase *ObjectInfoNode::Duplicate()
{
	ObjectInfoNode *node = new ObjectInfoNode();
	node->DuplicateBase(this);
	return node;
}

int ObjectInfoNode::Update()
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

	return NodeBase::Update();
}

int ObjectInfoNode::GetStatus()
{
	DrawableObject *dr = dynamic_cast<DrawableObject*>(properties.e[0]->GetData());
	if (!dr) return -1;

	return NodeBase::GetStatus();
}



Laxkit::anObject *newObjectInfoNode(int p, Laxkit::anObject *ref)
{
	return new ObjectInfoNode();
}


//--------------------------- SetupDataObjectNodes() -----------------------------------------

/*! Install default built in node types to factory.
 * This is called when the NodeGroup::NodeFactory singleton is created.
 */
int SetupDataObjectNodes(Laxkit::ObjectFactory *factory)
{
	 //--- LImageDataNode
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageData",  newLImageDataNode,  NULL, 0);

	 //--- LImageDataInfoNode
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageDataInfo",  newLImageDataInfoNode,  NULL, 0);

	 //--- ObjectInfoNode
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ObjectInfo",  newObjectInfoNode,  NULL, 0);


	return 0;
}


} //namespace Laidout

