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

#include "objecttree.h"
#include "../dataobjects/drawableobject.h"
#include "../language.h"
#include "../laidout.h"
#include "viewwindow.h"

#include <lax/button.h>
#include <lax/laxutils.h>
#include <lax/displayer.h>


#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;


namespace Laidout {



// domain: viewport
//    limbo
//      limbo obj1 
//      libmo obj2
//    page 1
//      page layer
//        page obj 1
//        page obj 1
//    page 16
//      layer
//        obj 3
//        obj 4
//
// domain: Project
//   limbos
//     limbo1
//     limbo2
//   doc 1
//     pages
//     ...
//   doc 2
//
// show only: selection, full tree



//------------------------------- ObjectTree ----------------------------------

/*! \class ObjectTree
 * \brief Class to allow browsing object trees.
 */


ObjectTree::ObjectTree(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *mes)
  : TreeSelector(parnt,nname,ntitle, ANXWIN_REMEMBER, //| ANXWIN_ESCAPABLE,
                0,0,400,400,0,
                nullptr,nowner,mes,
                TREESEL_SEND_PATH | TREESEL_SEND_ON_UP,
                nullptr)
{
	tree_column = 1;
	AddColumn("Flags", nullptr, 4*GetDefaultDisplayer()->textheight(),0,    TreeSelector::ColumnInfo::ColumnFlags,  1);
	AddColumn("Object",nullptr, 400-2*GetDefaultDisplayer()->textheight(),0,TreeSelector::ColumnInfo::ColumnString, 0);
}


ObjectTree::~ObjectTree()
{
}

char ObjectTree::ToggleChar(char current)
{
	if (current == ' ') return ' ';
	return TreeSelector::ToggleChar(current);
}


//------------------------------- ObjectTreeWindow ----------------------------------

/*! \class ObjectTree
 * \brief Class to allow browsing object trees.
 */

ObjectTreeWindow::ObjectTreeWindow(anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner,const char *nsend,
						ObjectContainer *container)
  : RowFrame(parnt, nname, ntitle ? ntitle : _("Object Tree"),
               ROWFRAME_ROWS|ROWFRAME_VCENTER|ANXWIN_REMEMBER,
               0,0,400,500,0,
               nullptr,nowner,nsend, 5)

{
    menu         = nullptr;
    tree         = nullptr;
    objcontainer = nullptr;
	domain       = Unknown;
	selection    = nullptr;
	inited       = false; //whether dump_in has been run already

	if (dynamic_cast<LaidoutViewport*>(container)) domain = InViewport;

    if (container) UseContainer(container);
    // else UseContainer(domain);

	ConstructTree();
}

ObjectTreeWindow::~ObjectTreeWindow()
{
	if (menu) menu->dec_count();
	if (objcontainer) objcontainer->dec_count();
	if (tree) tree->dec_count();
	if (selection) selection->dec_count();
}

LaidoutViewport *ObjectTreeWindow::FindViewport()
{
	ViewWindow *viewer = nullptr;
	LaidoutViewport *viewport = nullptr;
	if (win_parent) {
		for (int c=0; c<win_parent->NumWindowKids(); c++) {
			viewer = dynamic_cast<ViewWindow*>(win_parent->WindowChild(c));
			if (viewer) break;
		}
	}
	if (!viewer) viewer = dynamic_cast<ViewWindow*>(laidout->lastview);
	if (viewer) {
		viewport = dynamic_cast<LaidoutViewport*>(viewer->viewport);
	}

	return viewport;
}

void ObjectTreeWindow::UseContainer(ObjectTreeWindow::Domain container)
{
	if (container == SingleObject) {
		domain = SingleObject;
		return;
	}

	if (container == InViewport) {
		LaidoutViewport *viewport = FindViewport();
		if (viewport) UseContainer(viewport);

		return;
	}

	if (container == WholeProject) {
		UseContainer(laidout->project);
		return;
	}
}


/*! Connect to a governing container.
 * Does container->inc_count().
 */
void ObjectTreeWindow::UseContainer(ObjectContainer *container)
{
	if (!container) return;

	domain = SingleObject;
	if (menu) menu->Flush();

	UseContainerRecursive(container);
	if (tree) tree->InstallMenu(menu);

	container->inc_count();
	if (objcontainer) objcontainer->dec_count();
	objcontainer = container;
}

/*! Populate menu with all the items.
 */
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
		nm = container->object_e_name(c);
		//if (!nm) continue;

		menu->AddItem(nm ? nm : "(unnamed)");

		o  = container->object_e(c);
		oc = dynamic_cast<ObjectContainer*>(o);

		sprintf(flagstr,"  ");
		d = dynamic_cast<DrawableObject*>(o);
		if (d) {
			if (d->Visible()) flagstr[0]='E'; else flagstr[0]='e';
			if (d->IsLocked(0)) flagstr[1]='L'; else flagstr[1]='l';
		}
		menu->AddDetail(flagstr,nullptr);

		if (oc && oc->n()) {
			MenuItem *mi = menu->Top();
			menu->SubMenu();
			UseContainerRecursive(oc);
			menu->EndSubMenu();
			mi->Open();
		}
	}

	if (!menu->parent) {
		DBG menuinfoDump(menu,0);
	}
}

/*! Make sure the tree TreeSelector is created already.
 */
void ObjectTreeWindow::ConstructTree()
{
	if (tree) return;
	tree = new ObjectTree(this,"tree",nullptr,object_id,"tree");
	tree->InstallColors(THEME_Edit);
	tree->InstallMenu(menu);
	tree->tree_column = 1;
}

int ObjectTreeWindow::init()
{
	anXWindow *last = nullptr;
	Button *tbut;
	Displayer *dp=GetDefaultDisplayer();
	double th=dp->textheight();
	double lineh = th*1.5;

	if (domain == Unknown) UseContainer(InViewport);


 	 //add the tree
	if (!tree) ConstructTree();
	AddWin(tree,0, tree->win_w,tree->win_w/2,500,50,0, th*4,0,5000,50,0, -1);
	AddNull();


	 //Add buttons: [add][remove][up][down][dup]

 	 //add
    last = tbut = new Button(this,"add",nullptr,IBUT_ICON_ONLY, 0,0,0,0, 1,
                        last,object_id,"add",
                        0,_("+"),nullptr,nullptr,3,3);
	tbut->SetIcon(laidout->icons->GetIcon("Add"));
	tbut->tooltip(_("Add an empty group"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, lineh,0,0,50,0, -1);

	 //up
    last=tbut=new Button(this,"up",nullptr,IBUT_ICON_ONLY, 0,0,0,0, 1,
                        last,object_id,"up",
                        0,_("^"),nullptr,nullptr,3,3);
	tbut->SetIcon(laidout->icons->GetIcon("MoveUp"));
	tbut->tooltip(_("Move each selected up within each layer"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, lineh,0,0,50,0, -1);

	 //down
    last=tbut=new Button(this,"down",nullptr,IBUT_ICON_ONLY, 0,0,0,0, 1,
                        last,object_id,"down",
                        0,_("v"),nullptr,nullptr,3,3);
	tbut->SetIcon(laidout->icons->GetIcon("MoveDown"));
	tbut->tooltip(_("Move each selected down within each layer"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, lineh,0,0,50,0, -1);

	 //duplicate
    last=tbut=new Button(this,"dup",nullptr,IBUT_ICON_ONLY, 0,0,0,0, 1,
                        last,object_id,"dup",
                        0,_("dup"),nullptr,nullptr,3,3);
	tbut->SetIcon(laidout->icons->GetIcon("Duplicate"));
	tbut->tooltip(_("Duplicate each selected"));
    AddWin(tbut,1, tbut->win_w,0,50,50,0, lineh,0,0,50,0, -1);

	AddHSpacer(th/2,th/2,th/2, 50,-1);

	 //remove
    last=tbut=new Button(this,"remove",nullptr,IBUT_ICON_ONLY, 0,0,0,0, 1,
                        last,object_id,"remove",
                        0,_("-"),nullptr,nullptr,3,3);
	tbut->SetIcon(laidout->icons->GetIcon("Trash"));
	tbut->tooltip(_("Remove selected")); 
    AddWin(tbut,1, tbut->win_w,0,50,50,0, lineh,0,0,50,0, -1);


	last->CloseControlLoop();
	Sync(1);
	return 0;
}

int ObjectTreeWindow::Event(const Laxkit::EventData *data,const char *mes)
{
	if (!strcmp(mes,"tree")) {
		const SimpleMessage *sm = dynamic_cast<const SimpleMessage*>(data);
		DBG cerr << "ObjectTreeWindow event: "<<sm->info1<<" "<<sm->info2<<" "<<sm->info3<<" "<<sm->info4<<" "<<(sm->str ? sm->str : "null")<<endl;

		if (sm->info3 == 2) { //flag toggled
			LaidoutViewport *viewport = FindViewport();

			if (sm->info4 == 'e' || sm->info4 == 'E') {
				bool visible = (sm->info4 == 'E');
				DBG cerr << ".. toggle visible, now: "<<visible<<endl;

				//viewport->SetVisible(sm->str, visible);
				if (viewport) {
					VObjContext *oc = viewport->GetContextFromPath(sm->str);
					if (oc) {
						DBG cerr << "found context "<<*oc<<endl;
						oc->obj->Visible(visible);
						viewport->Needtodraw(1);
						delete oc;
					}
				}
				
			} else if (sm->info4 == 'l' || sm->info4 == 'L') {
				bool locked = (sm->info4 == 'L');
				DBG cerr << ".. toggle locked, now: "<<locked<<endl;

				//viewport->SetLocked(sm->str, locked);
				if (viewport) {
					VObjContext *oc = viewport->GetContextFromPath(sm->str);
					if (oc) {
						DBG cerr << "found context "<<*oc<<endl;
						if (locked) oc->obj->Lock(~0);
						else oc->obj->Unlock(~0);
						viewport->Needtodraw(1);
						delete oc;
					}
				}
			}

		} else {
			//TODO: this should probably add objects, then remove the ones in viewport->selection that are not selected 
			//      so as to avoid disruption as much as possible
			
			//probably just a selection change, try to sync up with viewport.
			LaidoutViewport *viewport = FindViewport();
			viewport->ClearSelection();

			for (int c=0; c<tree->NumSelected(); c++) {
				MenuItem *itm = tree->GetSelected(c);

				char *path = nullptr;
				makestr(path, itm ? itm->name : NULL);
	            while (itm && itm->parent && itm->parent->parent) {
	                itm = itm->parent->parent;
	                prependstr(path, "/");
	                prependstr(path, itm->name);
	            }
				viewport->SelectObject(path, false, false);
			}
		}

		// DBG LaidoutViewport *viewport = FindViewport();
		// DBG if (!viewport) return 0;
		// DBG viewport->PostMessage(sm->str);

		// if (viewport && domain == InViewport) {
		// 	// viewport->Select(sm->str);
		// }

		return 0;

	} else if (!strcmp(mes,"docTreeChange")) {
		const TreeChangeEvent *te=dynamic_cast<const TreeChangeEvent *>(data);
		if (!te) return 0;
		DBG cerr << "ObjectTreeWindow got docTreeChange "<<te->Message()<<endl;

		if (te->changetype == TreeSelectionChange) {
			UpdateSelection();

		// } else if (te->changetype == TreeDocGone) {
		} else {
			//tree has altered somehow, to be safe regenerate the whole tree
			RefreshList();
		}

		return 0;
	}
	
	return anXWindow::Event(data, mes);
}

int ObjectTreeWindow::UpdateSelection()
{
	LaidoutViewport *viewport = FindViewport();
	if (!viewport) return 1;

	LaxInterfaces::Selection *selection = viewport->GetSelection();
	PtrStack<const char> path(LISTS_DELETE_None);
	ObjectContainer *cc;
	VObjContext *context;

	if (selection->n() > 0) tree->DeselectAll();

	for (int c=0; c<selection->n(); c++) {
		// DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(c)->obj);
		// if (!obj) continue;

		// cc = dynamic_cast<ObjectContainer*>(obj);
		// path.flush();
		// while (cc) {
		// 	path.push(cc->Id(), 0, 0);
		// 	cc = cc->container_parent();
		// }
		
		context = dynamic_cast<VObjContext*>(selection->e(c));
		if (!context) continue;

		cc = viewport;
		for (int c2=0; c2<context->context.n(); c2++) {
			path.push(cc->object_e_name(context->context.e(c2)));
			cc = dynamic_cast<ObjectContainer*>(cc->object_e(context->context.e(c2)));
			if (!cc) break;
		}

		DBG cerr << "selectatpath: ";
		DBG for (int c2=0; c2<path.n; c2++) cerr <<"  "<<path.e[c2];
		DBG cerr <<endl;
		tree->SelectAtPath(path.e, path.n, false);
	}

	return 0;
}

void ObjectTreeWindow::RefreshList()
{
	DBG cerr << "ObjectTreeWindow::RefreshList()"<<endl;
	if (objcontainer) UseContainer(objcontainer);
}

int ObjectTreeWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	//if (ch == 'r' && (state & ControlMask)) {
	//	RefreshList();
	//}

	return anXWindow::CharInput(ch, buffer, len, state, d);
}


void ObjectTreeWindow::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *savecontext)
{   
    Laxkit::Attribute att;
    dump_out_atts(&att,what,savecontext); 
    att.dump_out(f,indent);
}   

Laxkit::Attribute *ObjectTreeWindow::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{ 
	if (tree) {
		att = tree->dump_out_atts(att,what,savecontext); 
		makestr(att->name, whattype());

		//remove the anxwindow specific stuff, as we override with our own
		int i = -2;
		if (att->find("win_x", &i)) att->remove(i);
		if (att->find("win_y", &i)) att->remove(i);
		if (att->find("win_w", &i)) att->remove(i);
		if (att->find("win_h", &i)) att->remove(i);
		if (att->find("win_flags", &i)) att->remove(i);
		if (att->find("win_themestyle", &i)) att->remove(i);
	}

	const char *dstr = "WholeProject";
	if (domain == InViewport) dstr = "InViewport";
	else if (domain == SingleObject) dstr = "SingleObject";
	att->push("domain", dstr);
	return anXWindow::dump_out_atts(att,what,savecontext);
}

void ObjectTreeWindow::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *loadcontext)
{	
	Attribute *datt = att->find("domain");
	if (datt) {
		Domain ndomain = WholeProject;
		if (strstr(datt->value, "WholeProject") == datt->value) ndomain = WholeProject;
		else if (strstr(datt->value, "InViewport") == datt->value) ndomain = InViewport;
		else if (strstr(datt->value, "SingleObject") == datt->value) ndomain = SingleObject;
		if (ndomain != domain) {
			// remap container before tree gets dumped in, as maybe it has open/unopen list

			if (ndomain == SingleObject) {
				const char *id = datt->value + 12;
				while (isspace(*id)) id++;
				ObjectContainer *obj = dynamic_cast<ObjectContainer*>(laidout->project->FindObject(id));
				if (obj) UseContainer(obj);

			} else {
				UseContainer(ndomain);
			}
		}
	}

	if (tree) {
		if (!inited) tree->dump_in_atts(att,flag,loadcontext); 
		inited = true;
	} else anXWindow::dump_in_atts(att,flag,loadcontext);
}


} //namespace Laidout


