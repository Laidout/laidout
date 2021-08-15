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
// Copyright (C) 2014 by Tom Lechner
//
#ifndef OBJECTTREE_H
#define OBJECTTREE_H


#include <lax/treeselector.h>
#include <lax/rowframe.h>
#include <lax/interfaces/selection.h>

#include "../dataobjects/objectcontainer.h"
#include "viewwindow.h"


namespace Laidout {


class ObjectTree : public Laxkit::TreeSelector
{
  public:
	ObjectTree(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *mes);
	virtual ~ObjectTree();
	virtual const char *whattype() { return "ObjectTree"; }
	virtual char ToggleChar(char current);
};



class ObjectTreeWindow : public Laxkit::RowFrame
{
  public:
	enum Domain { Unknown, WholeProject, InViewport, SingleObject };

  protected: 
	ObjectTree *tree;
	ObjectContainer *objcontainer;
	LaxInterfaces::Selection *selection;
	Laxkit::MenuInfo *menu;

	Domain domain;
	bool inited;

	virtual void UseContainerRecursive(ObjectContainer *container);
	void ConstructTree();
	int UpdateSelection();
	LaidoutViewport *FindViewport();

  public:
	ObjectTreeWindow(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *nsend,
						ObjectContainer *container);
	virtual ~ObjectTreeWindow();
	virtual const char *whattype() { return "ObjectTreeWindow"; }
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual void RefreshList();

	virtual void UseContainer(ObjectTreeWindow::Domain container);
	virtual void UseContainer(ObjectContainer *container);

    virtual void       dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);

};



} //namespace Laidout


#endif

