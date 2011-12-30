//
// $Id$
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
// Copyright (C) 2010 by Tom Lechner
//
#ifndef DRAWABLEOBJECT_H
#define DRAWABLEOBJECT_H


#include <lax/interfaces/somedata.h>
#include "objectcontainer.h"
#include "objectfilter.h"


//----------------------------- DrawableObject ---------------------------------
class DrawableObject :  virtual public ObjectContainer,
						virtual public Laxkit::SomeData,
						virtual public Laxkit::Tagged
{
 protected:
 public:

	SomeData *clip; //If not a PathsData, then is an object for a softmask
	PathsData *wrap_path;
	PathsData *inset_path;
	double autowrap, autoinset; //distance away from default to put the paths when auto generated

	Laxkit::RefPtrStack<DrawObjectChain> chains; //for linked objects
	DrawableObject *parent;
	int locks; //lock object contents|matrix|rotation|shear|scale|kids|selectable

	//RefPtrStack<RefCounted *> refs; //what other resources this objects depends on?

	ObjectStream *path_stream;
	ObjectStream *area_stream;
	RefPtrStack<ObjectFilter> filters;
	RefPtrStack<DrawableObject> subobjects;
	double alpha; //object alpha applied to anything drawn by this and kids
	double blur; //one built in filter?

	LaxFiles::Attribute metadata;
	LaxFiles::Attribute iohints;

	DrawableObject();
	virtual ~DrawableObject();

	 //from ObjectContainer
	virtual int n();
	virtual Laxkit::anObject *object_e(int i);

	 //new functions for DrawableObject
	PathsData *GetAreaPath();
	PathsData *GetInsetPath(); //return an inset path, may or may not be inset_path, where streams are laid into
	PathsData *GetWrapPath(); //path inside which external streams can't go
	
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


//---------------------------------- DrawObjectChain ---------------------------------
class DrawObjectChain : public Laxkit::anObject, public Laxkit::RefCounted, protected Laxkit::PtrStack<DrawableObject>
{
  public:
	char *id;
	DrawObjectChain();
	virtual ~DrawObjectChain();
};

#endif

