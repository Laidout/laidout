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
// Copyright (C) 2014 by Tom Lechner
//

#include "objecttree.h"
#include "../dataobjects/drawableobject.h"
#include <lax/displayer.h>


using namespace Laxkit;


namespace Laidout {

/*! \class ObjectTree
 * \brief Class to allow browsing object trees.
 */


ObjectTree::ObjectTree(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *mes)
  : TreeSelector(parnt,nname,ntitle, ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
                0,0,400,400,0,
                NULL,nowner,mes,
                0,NULL)
{
	tree_column=1;
	AddColumn("Flags", NULL, 2*GetDefaultDisplayer()->textheight(),    TreeSelector::ColumnInfo::ColumnFlags,  1);
	AddColumn("Object",NULL, 400-2*GetDefaultDisplayer()->textheight(),TreeSelector::ColumnInfo::ColumnString, 0);
}


ObjectTree::~ObjectTree()
{
}

void ObjectTree::UseContainer(ObjectContainer *container)
{
	if (!menu) menu=new MenuInfo();

	anObject *o;
	ObjectContainer *oc;
	DrawableObject *d;
	char flagstr[10];
	sprintf(flagstr,"el");

	for (int c=0; c<container->n(); c++) {
		menu->AddItem(container->object_e_name(c));

		o=container->object_e(c);
		oc=dynamic_cast<ObjectContainer*>(o);

		sprintf(flagstr,"  ");
		d=dynamic_cast<DrawableObject*>(o);
		if (d) {
			if (d->Visible()) flagstr[0]='E'; else flagstr[0]='e';
			if (d->IsLocked(0)) flagstr[1]='L'; else flagstr[1]='l';
		}
		menu->AddDetail(flagstr,NULL);

		if (oc && oc->n()) {
			menu->SubMenu();
			UseContainer(oc);
			menu->EndSubMenu();
		}
	}

	if (!menu->parent) {
		menuinfoDump(menu,0);
	}
}



} //namespace Laidout


