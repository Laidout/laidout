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
#ifndef INTERFACES_NODES_H
#define INTERFACES_NODES_H


#include <lax/objectfactory.h>
#include "nodeinterface.h"

#include <lax/interfaces/curvemapinterface.h>
#include "../dataobjects/lgradientdata.h"
#include "../calculator/curvevalue.h"


namespace Laidout { 


//--------------------- Setup --------------------------------
int SetupDefaultNodeTypes(Laxkit::ObjectFactory *factory);


//------------ blank NodeGroup creator, used by resource manager, as well as node manager
Laxkit::anObject *newNodeGroup(int p, Laxkit::anObject *ref);


//--------------------- Misc node related objects used by other things ----------------------

//--------------------- CurveProperty ---------------------------------
class CurveProperty : public NodeProperty
{
	static SingletonKeeper interfacekeeper;

  public:
	static LaxInterfaces::CurveMapInterface *GetCurveInterface();

	CurveValue *curve;

	CurveProperty(CurveValue *ncurve, int absorb, int isout);
	virtual ~CurveProperty();

	virtual void SetExtents(NodeColors *colors);
	virtual bool AllowType(Value *v);

	virtual LaxInterfaces::anInterface *PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp);
	virtual const char *PropInterfaceName() { return "CurveMapInterface"; }
	virtual bool HasInterface();
	virtual void Draw(Laxkit::Displayer *dp, int hovered);
};


//--------------------- GradientProperty ---------------------------------

class GradientProperty : public NodeProperty
{
	static SingletonKeeper interfacekeeper;

  public:
	LaxInterfaces::GradientInterface *GetGradientInterface();

	LaxInterfaces::GradientInterface *ginterf;

	GradientProperty(GradientValue *ncurve, int absorb, int isout);
	virtual ~GradientProperty();

	virtual void SetExtents(NodeColors *colors);
	
	virtual LaxInterfaces::anInterface *PropInterface(LaxInterfaces::anInterface *interface, Laxkit::Displayer *dp);
	virtual const char *PropInterfaceName() { return "GradientInterface"; }
	virtual bool HasInterface();
	virtual void Draw(Laxkit::Displayer *dp, int hovered);
};


//-------------------------- class ObjectNode ---------------------------------
class ObjectNode : public NodeBase
{
  public:
	int is_out;

	DrawableObject *obj;

	ObjectNode(int for_out, DrawableObject *nobj, int absorb);
	virtual ~ObjectNode();
	virtual int Update();
	virtual int GetStatus();
	virtual NodeBase *Duplicate();
	virtual int UpdatePreview();
};



} //namespace Laidout


#endif

