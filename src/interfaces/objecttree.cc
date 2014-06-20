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
#include "../language.h"

#include <lax/button.h>
#include <lax/laxutils.h>
#include <lax/displayer.h>


using namespace Laxkit;


namespace Laidout {




//------------------------------- ObjectTree ----------------------------------

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

//------------------------------- ObjectTreeWindow ----------------------------------

/*! \class ObjectTree
 * \brief Class to allow browsing object trees.
 */

ObjectTreeWindow::ObjectTreeWindow(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *nsend,
						ObjectContainer *container)
  : RowFrame(NULL, nname, ntitle ? ntitle : _("Object Tree"),
               ROWFRAME_ROWS|ROWFRAME_VCENTER|ANXWIN_REMEMBER,
               0,0,400,500,0,
               NULL,nowner,nsend, 5)

{
	menu=NULL;
	tree=NULL;
	objcontainer=NULL;
	if (container) UseContainer(container);
}

ObjectTreeWindow::~ObjectTreeWindow()
{
	if (menu) menu->dec_count();
	if (objcontainer) objcontainer->dec_count();
}

/*! Does container->inc_count().
 */
void ObjectTreeWindow::UseContainer(ObjectContainer *container)
{
	if (!container) return;
	UseContainerRecursive(container);
	if (tree) tree->InstallMenu(menu);
	if (objcontainer) objcontainer->dec_count();
	objcontainer=container;
	objcontainer->inc_count();
}

void ObjectTreeWindow::UseContainerRecursive(ObjectContainer *container)
{
	if (!menu) menu=new MenuInfo();

	anObject *o;
	ObjectContainer *oc;
	DrawableObject *d;
	char flagstr[10];
	sprintf(flagstr,"el");
	const char *nm;

	for (int c=0; c<container->n(); c++) {
		nm=container->object_e_name(c);
		//if (!nm) continue;

		menu->AddItem(nm?nm:"(unnamed)");

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
			UseContainerRecursive(oc);
			menu->EndSubMenu();
		}
	}

	if (!menu->parent) {
		menuinfoDump(menu,0);
	}
}

int ObjectTreeWindow::init()
{
	anXWindow *last=NULL;
	Button *tbut;
	Displayer *dp=GetDefaultDisplayer();
	double th=dp->textheight();


 	 //add the tree
	if (!tree) {
		tree=new ObjectTree(this,"tree",NULL,object_id,"tree");
		tree->installColors(app->color_edits);
		tree->InstallMenu(menu);
	}
	tree->tree_column=1;
	tree->AddColumn("Flags", NULL, 4*th,     TreeSelector::ColumnInfo::ColumnFlags,  1);
	tree->AddColumn("Object",NULL, 400-2*th, TreeSelector::ColumnInfo::ColumnString, 0);

	AddWin(tree,1, tree->win_w,tree->win_w/2,500,50,0, th*4,0,5000,50,0, -1);
	AddNull();


	 //Add buttons: [add][remove][up][down][dup]

 	 //add
    last=tbut=new Button(this,"add",NULL,0, 0,0,0,0, 1,
                        last,object_id,"add",
                        0,_("+"),NULL,NULL,3,3);
	tbut->tooltip(_("Add an empty group"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);

	 //up
    last=tbut=new Button(this,"up",NULL,0, 0,0,0,0, 1,
                        last,object_id,"up",
                        0,_("^"),NULL,NULL,3,3);
	tbut->tooltip(_("Move each selected up within each layer"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);

	 //down
    last=tbut=new Button(this,"down",NULL,0, 0,0,0,0, 1,
                        last,object_id,"up",
                        0,_("v"),NULL,NULL,3,3);
	tbut->tooltip(_("Move each selected down within each layer"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);

	 //duplicate
    last=tbut=new Button(this,"dup",NULL,0, 0,0,0,0, 1,
                        last,object_id,"dup",
                        0,_("dup"),NULL,NULL,3,3);
	tbut->tooltip(_("Duplicate each selected"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);

	AddHSpacer(th/2,th/2,th/2, 50,-1);

	 //remove
    last=tbut=new Button(this,"remove",NULL,0, 0,0,0,0, 1,
                        last,object_id,"remove",
                        0,_("-"),NULL,NULL,3,3);
	tbut->tooltip(_("Remove selected")); 
    AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);


	last->CloseControlLoop();
	Sync(1);
	return 0;
}

} //namespace Laidout


