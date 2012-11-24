//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef LIMAGEPATCH_H
#define LIMAGEPATCH_H

#include "drawableobject.h"
#include <lax/interfaces/imagepatchinterface.h>
#include <lax/interfaces/colorpatchinterface.h>


//------------------------------- LImagePatchData ---------------------------------------
class LImagePatchData : public DrawableObject, public LaxInterfaces::ImagePatchData
{
  public:
	LImagePatchData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LImagePatchData();
	virtual const char *whattype() { return "ImagePatchData"; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void FindBBox();
};


//------------------------------- LColorPatchData ---------------------------------------
class LColorPatchData : public DrawableObject, public LaxInterfaces::ColorPatchData
{
  public:
	LColorPatchData(LaxInterfaces::SomeData *refobj=NULL);
	virtual ~LColorPatchData();
	virtual const char *whattype() { return "ColorPatchData"; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void FindBBox();
};


//------------------------------- LImagePatchInterface ---------------------------------------
class LImagePatchInterface : public LaxInterfaces::ImagePatchInterface
{
 public:
	LImagePatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k);
};


//------------------------------- LColorPatchInterface ---------------------------------------
class LColorPatchInterface : public LaxInterfaces::ColorPatchInterface
{
 public:
	LColorPatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *k);
};


#endif

