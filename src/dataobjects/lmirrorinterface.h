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
// Copyright (C) 2021 by Tom Lechner
//
#ifndef LMIRRORINTERFACE_H
#define LMIRRORINTERFACE_H

#include <lax/interfaces/mirrorinterface.h>
#include <lax/singletonkeeper.h>
#include "drawableobject.h"
#include "objectfilter.h"



namespace Laidout {



//------------------------------- LMirrorInterface --------------------------------

class MirrorPathNode;

class LMirrorInterface : public LaxInterfaces::MirrorInterface
{
 protected:
	MirrorPathNode *node;
	bool mirror_node_connected;

	virtual void Modified(int level=0);

 public:
	LMirrorInterface(LaxInterfaces::anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~LMirrorInterface();
	virtual const char *whattype() { return "MirrorInterface"; }
	virtual LaxInterfaces::anInterface *duplicate(LaxInterfaces::anInterface *dup);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int Event(const Laxkit::EventData *e, const char *mes);

	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);


	//from value
	//virtual Value *duplicate();
	//virtual ObjectDef *makeObjectDef();
	//virtual int assign(FieldExtPlace *ext,Value *v);
	//virtual Value *dereference(const char *extstring, int len);

	//virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	//virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------- MirrorNode --------------------------------

class MirrorPathNode : public ObjectFilterNode
{
	static SingletonKeeper keeper; //the def for the op enum

  public:
	static LaxInterfaces::MirrorInterface *GetMirrorInterface();

	MirrorPathNode();
	virtual ~MirrorPathNode();

	virtual NodeBase *Duplicate();
	virtual int Update();
	virtual int GetStatus();

	virtual LaxInterfaces::anInterface *ObjectFilterInterface();
	virtual DrawableObject *ObjectFilterData();
	// virtual int Mute(bool yes=true);
	//virtual int Connected(NodeConnection *connection);

	virtual int UpdateMirror(flatpoint p1, flatpoint p2);
};


} //namespace Laidout

#endif

