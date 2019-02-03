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
};

LImageDataNode::LImageDataNode()
{
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


Laxkit::anObject *newLImageDataNode(int p, Laxkit::anObject *ref)
{
	return new LImageDataNode();
}


//--------------------------- SetupDataObjectNodes() -----------------------------------------

/*! Install default built in node types to factory.
 * This is called when the NodeGroup::NodeFactory singleton is created.
 */
int SetupDataObjectNodes(Laxkit::ObjectFactory *factory)
{
	 //--- LImageDataNode
	factory->DefineNewObject(getUniqueNumber(), "Drawable/ImageData",  newLImageDataNode,  NULL, 0);


	return 0;
}


} //namespace Laidout

