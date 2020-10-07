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
// Copyright (C) 2018 by Tom Lechner
//
#ifndef LPERSPECTIVEINTERFACE_H
#define LPERSPECTIVEINTERFACE_H

#include <lax/interfaces/perspectiveinterface.h>
#include <lax/singletonkeeper.h>
#include "drawableobject.h"
#include "objectfilter.h"



namespace Laidout {



//------------------------------- LPerspectiveInterface --------------------------------
class PerspectiveNode;

class LPerspectiveInterface : public LaxInterfaces::PerspectiveInterface
{
 protected:
	PerspectiveNode *pnode;

	virtual void Modified();

 public:
	LPerspectiveInterface(LaxInterfaces::anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~LPerspectiveInterface();
	virtual const char *whattype() { return "PerspectiveInterface"; }
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);

	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);


	//from value
	//virtual Value *duplicate();
	//virtual ObjectDef *makeObjectDef();
	//virtual int assign(FieldExtPlace *ext,Value *v);
	//virtual Value *dereference(const char *extstring, int len);

	//virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	//virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------- PerspectiveNode --------------------------------
class PerspectiveNode : public ObjectFilterNode
{
	static SingletonKeeper keeper; //the def for the op enum

  public:
	static LaxInterfaces::PerspectiveInterface *GetPerspectiveInterface();

	bool render_preview;
	double render_dpi;
	LaxInterfaces::PerspectiveTransform *transform;
	// Laxkit::Affine render_transform;

	PerspectiveNode();
	virtual ~PerspectiveNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int UpdateTransform();
	virtual int GetStatus();

	virtual LaxInterfaces::anInterface *ObjectFilterInterface();
	virtual DrawableObject *ObjectFilterData();
	// virtual int Mute(bool yes=true);
	//virtual int Connected(NodeConnection *connection);

};


} //namespace Laidout

#endif

