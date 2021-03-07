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
// Copyright (C) 2005-2007,2009-2013 by Tom Lechner
//

#include <lax/transformmath.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/rectpointdefs.h>
#include <lax/colors.h>
#include <lax/popupmenu.h>

#include "lsomedataref.h"
#include "groupinterface.h"
#include "../interfaces/anchorinterface.h"
#include "../interfaces/aligninterface.h"
#include "../interfaces/nupinterface.h"
#include "../interfaces/objectfilterinterface.h"
#include "printermarks.h"
#include "../core/project.h"
#include "../ui/viewwindow.h"
#include "../ui/metawindow.h"
#include "../language.h"
#include "../core/stylemanager.h"
#include "../calculator/shortcuttodef.h"
#include "objectfilter.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;
using namespace Laxkit;


namespace Laidout {


//----------------------------- GroupInterface -----------------------

/*! \class GroupInterface
 * \brief Interface for selecting multiple things, grouping, and ungrouping.
 */



GroupInterface::GroupInterface(int nid,Laxkit::Displayer *ndp)
	: ObjectInterface(nid,ndp)
{
	popupcontrols = GIA_No_Popup;
}

GroupInterface::~GroupInterface()
{
	DBG cerr <<"---- in GroupInterface destructor"<<endl;
}

anInterface *GroupInterface::duplicate(anInterface *dup)
{
	GroupInterface *g;
	if (dup==NULL) g=new GroupInterface(id,dp);
	else {g=dynamic_cast<GroupInterface *>(dup);
		if (g==NULL) return NULL;
	}
	return ObjectInterface::duplicate(g);
}

void GroupInterface::TransformSelection(const double *N, int s, int e) 
{
	for (int c=0; c<selection->n(); c++) {
		DBG cerr<<"-------ObjectInterfaceTransformSelection on "; ((VObjContext *)selection->e(c))->context.out(":");
	}
	ObjectInterface::TransformSelection(N,s,e);
}


int GroupInterface::draws(const char *atype)
{
	//if (!strcmp(atype,"Group") || !strcmp(atype,"MysteryData")) return 1;
	if (!strcmp(atype,"Group")) return 1;
	return 0;
}

/*! Return 0 if newdata accepted, 1 if accepted.
 *
 * \todo *** UseThis() should optionally use object contexts, not just an anObject.
 */
int GroupInterface::UseThis(anObject *newdata,unsigned int)
{
	if (!newdata) return 0;
	SomeData *d=dynamic_cast<SomeData *>(newdata);
	if (!d) return 0;

	FreeSelection();
	VObjContext oc;
	oc.obj=d; 
	d->inc_count();
	((LaidoutViewport *)viewport)->locateObject(d,oc.context);
	AddToSelection(&oc);
	needtodraw=1;
	return 1;
}

Laxkit::MenuInfo *GroupInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (child) return child->ContextMenu(x,y,deviceid, menu);
	//if (child) return menu;

	rx=x;
	ry=y;

	if (selection->n() > 1)
	{
		if (!menu) menu=new MenuInfo(_("Group Interface"));
		else menu->AddSep();
		menu->AddItem(_("Align..."), GIA_Align);
		menu->AddItem(_("Distribute..."), GIA_Distribute);

		if (selection->n() == 2) {
			
			ObjectContext *o1 = selection->e(0);
			ObjectContext *o2 = selection->e(1);
			if (dynamic_cast<PathsData*>(o1->obj) || dynamic_cast<PathsData*>(o2->obj))
			{
				//maybe apply a clip mask
				bool first  = (dynamic_cast<PathsData*>(o1->obj) != NULL);
				bool second = (dynamic_cast<PathsData*>(o2->obj) != NULL);
				int n = strlen(o1->obj->Id()) + strlen(o2->obj->Id());
				char scratch[strlen(_("Clip %s with %s")) + n + 1];

				if (first) {
					sprintf(scratch, _("Clip %s with %s"), o2->obj->Id(), o1->obj->Id());
					menu->AddItem(scratch, GIA_Clip_First_On_Second);
				}

				if (second) {
					sprintf(scratch, _("Clip %s with %s"), o1->obj->Id(), o2->obj->Id());
					menu->AddItem(scratch, GIA_Clip_Second_On_First);
				}
			}
		}

	}
	else if (selection->n() == 1)
	{
		DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(0)->obj);
		if (obj) {
			if (!menu) menu=new MenuInfo(_("Group Interface"));
			menu->AddItem(_("Edit filter nodes..."), GIA_Edit_Filter_Nodes);
			if (obj->filter) {
				menu->AddItem(_("Interactive filters..."), GIA_Filter_Editor);
				menu->AddItem(_("Remove filter"), GIA_Remove_Filter);
				menu->AddItem(_("Apply filter"), GIA_Apply_Filter);
				menu->AddItem(_("Update filter"), GIA_Refresh_Filter);

				// menu->SubMenu(_("Resources"));
				// menu->AddItem(_("Create resource from filter"), GIA_Resourceify_Filter);
				// menu->AddItem(_("Create linked resource from filter"), GIA_Resourceify_Linked_Filter);
				// menu->AddItem(_("Filter from resource"), GIA_Copy_Filter_Resource);
				// menu->AddItem(_("Link filter from resource"), GIA_Link_Filter_Resource);
				// menu->EndSubMenu();
			}
			if (obj->clip_path) {
				if (menu->n()) menu->AddSep();
				menu->AddItem(_("Remove clip path"), GIA_Remove_Clip);
				menu->AddItem(_("Extract clip path"), GIA_Extract_Clip);
				menu->AddItem(_("Edit clip path"), GIA_Edit_Clip);
			}

			if (menu->n()) menu->AddSep();
			menu->AddItem(_("Edit meta"), GIA_Edit_Object_Meta);
		}
	}

	if (selection->n() > 0) {
		DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(0)->obj);
		
		menu->AddSep(_("Parenting"));
		menu->AddItem(_("Reparent..."),    GIA_Reparent);
		if (obj->GetParent() && obj->GetParent()->Selectable()) {
			menu->AddItem(_("Jump to parent"), GIA_Jump_To_Parent);
			menu->AddItem(_("Unparent"),       GIA_Unparent);
		}
	}

	// *** for when multiobject meta edit available:
	//if (selection->n()) {
	//	if (!menu) menu=new MenuInfo(_("Group Interface"));
	//	if (menu->n()) menu->AddSep();
	//	menu->AddItem(_("Edit meta"), GIA_Edit_Object_Meta);
	//}

	LaidoutViewport *lvp=dynamic_cast<LaidoutViewport*>(viewport);
	if (lvp->papergroup) {
		if (!menu) menu=new MenuInfo(_("Group Interface"));
		else if (menu->n()==0) menu->AddSep(_("Group"));

		menu->AddItem(_("Add Registration Mark"), GIA_RegistrationMark);
		menu->AddItem(_("Add Gray Bars"), GIA_GrayBars);
		//menu->AddItem(_("Add Cut Marks"),PAPERM_CutMarks);
		//menu->AddSep();
	}

	if (!menu) menu = new MenuInfo(_("Group Interface"));
	if (menu->n()) menu->AddSep();
	menu->AddItem(_("Create empty"), GIA_Create_Empty);

	RectInterface::ContextMenu(x,y,deviceid,menu);
	return menu;
}

int GroupInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		if (child) return child->Event(e,mes);

		if (ObjectInterface::Event(e,mes) == 0) return 0;

		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i=s->info2; //id of menu item

		LaidoutViewport *lvp=dynamic_cast<LaidoutViewport*>(viewport);

		if (i == GIA_RegistrationMark) {
			if (!lvp->papergroup) return 0;
			DrawableObject *obj= RegistrationMark(18,1);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			lvp->papergroup->objs.push(obj);
			obj->dec_count();
			needtodraw=1;
			return 0;

		} else if (i == GIA_GrayBars) {
			if (!lvp->papergroup) return 0;
			DrawableObject *obj= BWColorBars(18,LAX_COLOR_GRAY);
			flatpoint fp=dp->screentoreal(rx,ry);
			obj->origin(fp);
			lvp->papergroup->objs.push(obj);
			obj->dec_count();
			needtodraw=1;
			return 0;

		} else if (i == GIA_Align
				|| i == GIA_Distribute
				|| i == GIA_Edit_Filter_Nodes
				|| i == GIA_Filter_Editor
				|| i == GIA_Remove_Filter
				|| i == GIA_Refresh_Filter
				|| i == GIA_Apply_Filter
				|| i == GIA_Remove_Clip
				|| i == GIA_Extract_Clip
				|| i == GIA_Edit_Clip
				|| i == GIA_Clip_First_On_Second
				|| i == GIA_Clip_Second_On_First
				|| i == GIA_Edit_Object_Meta
				|| i == GIA_Jump_To_Parent
				|| i == GIA_Reparent 
				|| i == GIA_Unparent
				|| i == GIA_Create_Empty
				) {
			PerformAction(i);
			return 0;

		}

	} else if (!strcmp(mes,"parentOptions")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		int i = s->info2; //id of menu item

		if (i == GIA_Jump_To_Parent
				|| i == GIA_Reparent 
				|| i == GIA_Unparent) {
			if (s->info3) {
				//hovered only, do special indicator hover
				hover = i;
				PostMessage(hoverMessage(hover));

				if (hover == GIA_Reparent) {
					if (selection->n() > 1) {
						if (reparent_temp.obj != selection->e(selection->n()-1)->obj)
							reparent_temp = *dynamic_cast<VObjContext*>(selection->e(selection->n()-1));
					}
				//} else if (hover == GIA_Jump_To_Parent) {
				} else {
					reparent_temp.SetObject(nullptr);
				}
				needtodraw = 1;
			} else PerformAction(i);
		}

		return 0;

	} else if (!strcmp(mes,"cloneOptions")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		int i = s->info2; //id of menu item

		PostMessage(hoverMessage(i));
		if (i == GIA_Jump_To_Link || i == GIA_Sever_Link) PerformAction(i);
		return 0;

	} else if (!strcmp(mes,"docTreeChange")) {
		const TreeChangeEvent *te = dynamic_cast<const TreeChangeEvent*>(e);
		if (te->changetype == TreeSelectionChange) {
			// tree was changed from the outside, so we need to adapt to what is now in selection
			// if (selection->n() == 0) FreeSelection();

		} else {
			 //valid all refs in selection
			for (int c=selection->n()-1; c>=0; c--) {
				if (!viewport->IsValidContext(selection->e(c))) {
					selection->Remove(c);
				} 
			}
		}

		RemapBounds();
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes,"objectMeta")) {
		if (!selection->n()) return 0;
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s || !s->object) return 0;
		AttributeObject *meta = dynamic_cast<AttributeObject*>(s->object);
		if (!meta) return 0;

		ObjectContext *oc = selection->e(0);
		DrawableObject *obj = dynamic_cast<DrawableObject*>(oc->obj);
		if (obj->metadata != meta) {
			if (obj->metadata) obj->metadata->dec_count();
			obj->metadata = meta;
			obj->metadata->inc_count();
		}

		needtodraw=1;
		return 0;

	}

	return ObjectInterface::Event(e,mes);
}

const char *GroupInterface::hoverMessage(int p)
{
	if (p == GIA_Link) return _("Clone options");
	if (p == GIA_Jump_To_Link) return _("Jump to original");
	if (p == GIA_Sever_Link) return _("Sever link");
	if (p == GIA_Parent_Link) return _("Parent options");
	if (p == GIA_Reparent) return _("Parent to");
	if (p == GIA_Unparent) return _("Remove parent of each selected");
	if (p == GIA_Jump_To_Parent) return _("Select parent");
	const char *mes = RectInterface::hoverMessage(p);
	static char msg[20];
	if (mes == nullptr) { sprintf(msg, "%d", p); mes = msg; }
	return mes;
}

/*! Called by RectInterface::scan(), for easier adaption to custom object controls.
 */
int GroupInterface::AlternateScan(flatpoint sp, flatpoint p, double xmag,double ymag, double onepix)
{
	//must scan for:
	//  link ball
	//  parent link
	//  constraints
	//  zone selector
	//  chains

	 //check for Somedataref indicator
	if (selection->n()==1 && !strcmp(selection->e(0)->obj->whattype(),"SomeDataRef")) {
		double dist=onepix*maxtouchlen*maxtouchlen/4;
		flatpoint pp=flatpoint((somedata->minx+somedata->maxx)/2, somedata->miny-maxtouchlen/ymag);
		DBG cerr <<"...alt scan: "<<p.x<<','<<p.y<< norm2(p-pp)<<endl;

		if (norm2(p-pp)<dist) return GIA_Link;

		if (popupcontrols == GIA_Link) {
			double th=dp->textheight();
			double w=dp->textextent(_("Jump to parent"),-1,NULL,NULL)*1.5;
		
			 //p.x,p.y, w/2,th*1.5
			pp=dp->realtoscreen(transform_point(somedata->m(),pp));
			pp.y+=2*th;
			pp.x-=w/2;
			if (pp.y+th>dp->Maxy) pp.y=dp->Maxy-th;
			if (sp.y>pp.y-th*1.5 && sp.y<pp.y+th*1.5) {
				if (sp.x>pp.x-w/2 && sp.x<pp.x+w/2) return GIA_Jump_To_Link;
				if (sp.x>pp.x+w/2 && sp.x<pp.x+w*3/2) return GIA_Sever_Link;
			}
		}
	}

	 //check for parent link
	//if (selection->n()==1) {
	if (selection->n()) {
		 //2 selected simplifies reparent
		flatpoint pp=transform_point(somedata->m(),(flatpoint(somedata->minx,somedata->miny)+flatpoint(somedata->maxx,somedata->maxy))/2); //center
		flatpoint p2=transform_point(somedata->m(),flatpoint(somedata->minx,somedata->maxy)); //upper left
		pp=dp->realtoscreen(pp);
		p2=dp->realtoscreen(p2);

		//flatpoint pp=(flatpoint(somedata->minx,somedata->miny)+flatpoint(somedata->maxx,somedata->maxy))/2; //center
		//flatpoint p2=flatpoint(somedata->minx,somedata->maxy); //upper left

		flatpoint v=p2-pp;
		v.normalize();
		pp=p2+v*maxtouchlen*1.5;

		DBG cerr <<"...alt scan: "<<p.x<<','<<p.y<<"  "<< norm2(p-pp)<<endl;

		//double dist=onepix*maxtouchlen*maxtouchlen/4;
		double dist=maxtouchlen*maxtouchlen/4;
		if (norm2(sp-pp)<dist) return GIA_Parent_Link;

		 //check for popup menu for parent link
		if (popupcontrols == GIA_Parent_Link) {
			double th=dp->textheight();
			double w=dp->textextent(_("Jump to parent"),-1,NULL,NULL)*1.5;
			pp+=v*maxtouchlen*.5;
			pp.y-=3*th;
			if (pp.y-th<dp->Miny) pp.y=dp->Miny+th;
			if (pp.x-w*.5<dp->Minx) pp.x=dp->Minx+w*.5;

			if (sp.y>pp.y-th && sp.y<pp.y+th) {
				//if (selection->n()>1 && sp.x<pp.x+w*.5 && sp.x>pp.x-w*.5) return GIA_Reparent;
			} else if (sp.y>pp.y+th && sp.y<pp.y+3*th) {
				if (sp.x<pp.x+w*.5 && sp.x>pp.x-w*.5) {
					if (selection->n()==1) return GIA_Jump_To_Parent;
					else return GIA_Reparent;
				}
			}

//			if (sp.y>pp.y-th && sp.y<pp.y+th) {
//				if (selection->n()==1 && sp.x>pp.x-w*1.5 && sp.x<pp.x-w*.5) return GIA_Parent_Matrix;
//				if (sp.x<pp.x+w*1.5 && sp.x>pp.x+w*.5) return GIA_Jump_To_Parent;
//			} else if (sp.y>pp.y+th && sp.y<pp.y+3*th) {
//				if (selection->n()==1 && sp.x>pp.x-w*1.5 && sp.x<pp.x-w*.5) return GIA_Parent_Align;
//				if (sp.x<pp.x+w*1.5 && sp.x>pp.x+w*.5) return GIA_Reparent;
//			}
		}
	}


	return RP_None;
}

int GroupInterface::LBDown(int x, int y,unsigned int state, int count,const Laxkit::LaxMouse *mouse)
{
	DBG cerr <<"GroupInterface::LBDown..."<<endl;

	if (count == 2 && selection->n() == 1 && !strcmp(selection->e(0)->obj->whattype(), "Group")) {
		//enter group and clear selection
		ObjectContext *oc = selection->e(0)->duplicate();
		FreeSelection();
		oc->SetObject(NULL);
		((LaidoutViewport *)viewport)->ChangeContext(oc);
		delete oc;
		needtodraw=1;

		return 0;
	}

	int c = ObjectInterface::LBDown(x,y,state,count,mouse); //ultimately uses AlternateScan() to update buttondown extra info

	int curpoint;
	buttondown.getextrainfo(mouse->id,LEFTBUTTON,&curpoint);
	if (curpoint == GIA_Link) {
		PopupCloneOptions(mouse);
		return 0;

	} else if (curpoint == GIA_Parent_Link) {
		PopupParentOptions(x,y,state,mouse);
		return 0;
	}

	//if (count==2 && selection->n()==1 && strcmp(selection->e(0)->obj->whattype(),"Group")) {
	if (count==2 && selection->n() && strcmp(selection->e(0)->obj->whattype(),"Group")) {
		//double click to switch to more specific tool
		if (viewport) viewport->ChangeObject(selection->e(0),1,true);
		buttondown.clear();
	}
	return c;
}

void GroupInterface::PopupParentOptions(double x,double y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	//popupcontrols = GIA_Parent_Link;
	//PostMessage(" ");

	MenuInfo *menu = new MenuInfo(_("Parent options"));

	menu->AddItem(_("Jump to parent"), GIA_Jump_To_Parent);
	menu->AddItem(_("Reparent..."),    GIA_Reparent);
	menu->AddItem(_("Unparent"),       GIA_Unparent);

	PopupMenu *popup = new PopupMenu(NULL,_("Parent options"), 0,
                        0,0,0,0, 1,
                        object_id,"parentOptions",
                        mouse->id, //mouse to position near?
                        menu,1, NULL,
                        TREESEL_LEFT | TREESEL_SEND_HOVERED);
	popup->WrapToMouse(None);
	app->rundialog(popup);
}

void GroupInterface::PopupCloneOptions(const Laxkit::LaxMouse *mouse)
{
	//popupcontrols = GIA_Link;

	MenuInfo *menu = new MenuInfo(_("Clone options"));

	menu->AddItem(_("Jump to original"), GIA_Jump_To_Link);
	menu->AddItem(_("Sever clone link"), GIA_Sever_Link);

	PopupMenu *popup = new PopupMenu(NULL,_("Clone options"), 0,
                        0,0,0,0, 1,
                        object_id,"cloneOptions",
                        mouse->id, //mouse to position near?
                        menu,1, NULL,
                        TREESEL_LEFT /*| TREESEL_SEND_HOVERED*/);
	popup->WrapToMouse(None);
	app->rundialog(popup);
}

int GroupInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	//int curpoint;
	//buttondown.getextrainfo(d->id,LEFTBUTTON,&curpoint);
	if (popupcontrols == GIA_Link) {
		buttondown.up(d->id,LEFTBUTTON);

		if (hover == GIA_Sever_Link) {
			PerformAction(GIA_Sever_Link);

		} else if (hover == GIA_Jump_To_Link) {
			PerformAction(GIA_Jump_To_Link);

		}

		popupcontrols = GIA_No_Popup;
		hover=0;
		needtodraw=1;
		return 0;

	} else if (popupcontrols == GIA_Parent_Link) {
		buttondown.up(d->id,LEFTBUTTON);
		DrawableObject *obj=dynamic_cast<DrawableObject*>(selection->e(0)->obj);

		if (hover==GIA_Parent_Align) {
			 //switch to anchor align tool
			ViewerWindow *vw=dynamic_cast<ViewerWindow*>(viewport->win_parent);
			vw->SelectTool("AnchorInterface");
			AnchorInterface *anchor=dynamic_cast<AnchorInterface*>(vw->CurrentTool());
			
			 //add anchors of parent
			LaidoutViewport *vp=((LaidoutViewport *)viewport);
			//anchor->AddToSelection(selection);
			anchor->SetCurrentObject(&vp->curobj);

			FreeSelection();

		} else if (hover==GIA_Parent_Matrix) {
			obj->AddAlignmentRule(NULL); //removes any alignment rules

		} else if (hover==GIA_Jump_To_Parent) {
			PerformAction(GIA_Jump_To_Parent);

		} else if (hover==GIA_Reparent) {
			 //reparent
			if (reparent_temp.obj) {
				PerformAction(GIA_Reparent);
			}
		}

		popupcontrols = GIA_No_Popup;
		hover=0;
		needtodraw=1;
		return 0;
	}

	int status = ObjectInterface::LBUp(x,y,state,d);

	 //update filters as needed
	PerformAction(GIA_Refresh_Filter);

	return status;
}

int GroupInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (popupcontrols == GIA_Link) {
		DBG cerr <<"checking for GIA_Link..."<<endl;
		int newhover=scan(x,y);
		if (newhover!=hover) {
			hover=newhover;
			needtodraw|=2;
            const char *mes=hoverMessage(hover);
            PostMessage(mes?mes:" ");
		}
		return 0;

	} else if (popupcontrols == GIA_Parent_Link) {
		DBG cerr <<"checking for GIA_Parent_Link..."<<endl;
		int newhover=scan(x,y);
		int oldhover=hover;

		if (newhover!=hover) {
			hover=newhover;
			needtodraw|=2;
            const char *mes=hoverMessage(hover);
            PostMessage(mes?mes:" ");
		}

		if (hover == GIA_Reparent) {
			//highlight object to parent to, use last object if more than one selection
			//check is mousemove if selection->n()==1, then grab from viewport search
			if (selection->n()>1) {
				if (reparent_temp.obj != selection->e(selection->n()-1)->obj)
					reparent_temp=*dynamic_cast<VObjContext*>(selection->e(selection->n()-1));
			}
		} else if (oldhover==GIA_Reparent && hover!=oldhover) {
			reparent_temp.SetObject(NULL); 
		}
		return 0;
	}

	return ObjectInterface::MouseMove(x,y,state,d);
}

/*! Group all objects in selection, even if there is only one selected.
 *
 * Return 1 if change, else 0.
 */
int GroupInterface::GroupObjects()
{
	if (selection->n()==0) {
		PostMessage(_("No objects selected."));
		return 0;
	}

	int error=0;
	Group *base=NULL;
	FieldPlace place;

	if (!((LaidoutViewport *)viewport)->locateObject(selection->e(0)->obj, place)) {
		PostMessage("Ugly internal error finding a selected object! Fire the programmer.");
		return 0;
	} 

	DBG place.out("toggle group point: ");

	base = dynamic_cast<Group*>(selection->e(0)->obj->GetParent());
	if (!base) {
		PostMessage(_("Bad group parent."));
		return 0;
	}

	
	 // check that all selected objects are same level, 
	 // then make new Group object and fill in selection,
	 // updating LaidoutViewport and Document
	 
	FieldPlace place1;
	Laxkit::NumStack<int> list;
	LaidoutViewport *vp=((LaidoutViewport *)viewport);
	list.push(place.pop()); //remove top index from place, which was selection[0]

	for (int c=1; c<selection->n(); c++) {
		vp->locateObject(selection->e(c)->obj,place1);
		list.pushnodup(place1.pop());

		if (!(place1==place)) {
			PostMessage("Items must all be at same level to group.");
			return 0;
		}
	}
	
	 //now place points to the parent object, and base is limbo or spread->page->layers
//	if (place.e(0)==0) place.pop(0); // remove spread index
//	else if (place.e(0)==1) { 
//		place.pop(0); // remove spread index and pagelocation index
//		place.pop(0); 
//	} else {
//		PostMessage("Containing object must be limbo or a spread.");
//		return 0;
//	}

	int index=-1;
	error=base->GroupObjs(list.n,list.e, &index);
	PostMessage(error ? _("Group failed.") : _("Grouped."));
	
	if (!error && index>=0) {
		FreeSelection();//only flushes (not deletes) selection object
		place.push(index);
		
		VObjContext context;
		context.SetObject(base->e(index));
		context.context = place;
		AddToSelection(&context);

		DBG place.out(".....group position after grouping: ");

		((LaidoutViewport *)viewport)->ChangeObject(&context, 0,true);

	} else {
		((LaidoutViewport *)viewport)->clearCurobj();
	}


	return 1;
}

/*! Ungroup any Group objects in selection.
 *
 * Return 1 if change, else 0.
 */
int GroupInterface::UngroupObjects()
{
	if (selection->n()==0) {
		PostMessage("No objects selected.");
		return 0;
	}

	Selection *newselection = new Selection;
	Group *group;
	int error=0;

	for (int c=0; c<selection->n(); c++) {
		group = dynamic_cast<Group*>(selection->e(c)->obj);
		if (!group) {
			newselection->Add(selection->e(c),-1);
			continue;
		}

		Group *base=NULL;
		FieldPlace place;

		if (!((LaidoutViewport *)viewport)->locateObject(group, place)) {
			//PostMessage("Ugly internal error finding a selected object! Fire the programmer.");
			continue;
		} 

		base = dynamic_cast<Group*>(group->GetParent());
		if (!base) {
			PostMessage(_("Bad ungroup parent."));
			delete newselection;
			return 0;
		}

		int numgrouped=group->NumKids();
		int nerror = base->UnGroup(place.e(place.n()-1)); //group is invalid now
		error |= nerror;
		if (nerror) continue;

		 //objects are ungrouped now, need to repopulate selection
		VObjContext element;
		if (nerror==0) {
			int which=place.e(place.n()-1);
			for (int c=which; c<which+numgrouped; c++) {
				element.clear();
				element.SetObject(base->e(c));
				for (int c2=0; c2<place.n(); c2++) {
					element.push(c2==place.n()-1 ? c : place.e(c2));
					//element.push(place.e(c2));
				}
				newselection->Add(&element, -1);
			}
		} 
	}

	FreeSelection();
	AddToSelection(newselection);
	newselection->dec_count(); 

	((LaidoutViewport *)viewport)->clearCurobj();
	((LaidoutViewport *)viewport)->SetSelection(selection);

	PostMessage(error ? _("Ungrouped (with some errors)") : _("Ungrouped.")); 
	
	return 1;
}

/*! Return 1 if change, else 0.
 *
 * If only one object selected, then ungroup if it is a group. If not a group,
 * then nothing is done.
 *
 * If many selected, then group them.
 */
int GroupInterface::ToggleGroup()
{
	if (selection->n()==0) {
		PostMessage(_("No objects selected."));
		return 0;
	} 
	
	if (selection->n()==1) {
		 // a single Group is selected, ungroup its objects...
		Group *group = dynamic_cast<Group*>(selection->e(0)->obj);
		if (!group) return 0;
		return UngroupObjects();
	}

	return GroupObjects();
}

/*! Returns the number of new things added to the selection.
 */
int GroupInterface::GrabSelection(unsigned int state)
{
	if (!data) return 1;

	DoubleBBox bbox;
	bbox.addtobounds(data->m(),data);
	
	DBG cerr <<"grab from: "<<bbox.minx<<','<<bbox.miny<<endl;
	DBG cerr <<"grab to:   "<<bbox.maxx<<','<<bbox.maxy<<endl;
	
	int n=0;
	VObjContext **objs=NULL;
	n=viewport->FindObjects(&bbox,0,0,NULL,(ObjectContext ***)(&objs));

	DBG if (n && !objs) cerr <<"*******ERROR! says found objects, but no objects returned."<<endl;
	DBG else {
	DBG 	cerr <<"find in box: "<<data->minx<<","<<data->miny<<" -> "<<data->maxx<<","<<data->maxy<<endl;
	DBG 	cerr <<"Found objects: "<<n<<endl;
	DBG }

	 //add
	for (int c=0; c<n; c++) {
		DBG cerr << "  ";
		DBG if (objs[c]) objs[c]->context.out("");

		AddToSelection(objs[c]);
		delete objs[c];
	}
	if (n==0) {
		deletedata();
		needtodraw=1;
	}
	
	return n;
}

Laxkit::ShortcutHandler *GroupInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=ObjectInterface::GetShortcuts();

	sc->AddMode(1, "button", _("Button"), _("When button is down"));

	sc->Add(GIA_Align,     'a',0,0,    "Align",       _("Align selected objects"),NULL,0);
	sc->Add(GIA_Distribute,'n',0,0,    "Nup",         _("Distribute selected objects in rows and columns"),NULL,0);

	sc->AddAction(GIA_Clone,     "Clone",     _("Clone objects"),NULL,0, 1);
	sc->Add(GIA_CloneB,    ' ',ControlMask,1,"CloneB",_("Clone objects, if button down"),NULL,0);
	sc->AddAction(GIA_Duplicate, "Duplicate", _("Duplicate objects"),NULL,0, 1);
	sc->Add(GIA_DuplicateB,' ',0,1,    "DuplicateB",  _("Duplicate objects, if button down"),NULL,0);

	sc->Add(GIA_Reparent,  'p',AltMask,0,          "Parent",  _("Parent selected objects to last selected"),NULL,0);
	sc->Add(GIA_Unparent,  'P',AltMask|ShiftMask,0,"Unparent",_("Unparent selected objects from their immediate parents"),NULL,0);

	sc->Add(GIA_Edit_Contents,LAX_Enter,0,0, "EditObject",   _("Switch to content edit tool"),NULL,0);
	sc->Add(GIA_Create_Empty, 'e',0,0,       "CreateEmpty",  _("Create empty group object"),NULL,0);

	//...shortcuts to switch to other tools are handled in ViewWindow


	// *** initialize nup and align shortcuts.. there should be a better way to do this!!
	NUpInterface nup((int)0);
	nup.GetShortcuts();
	AlignInterface align(nullptr);
	align.GetShortcuts();

	return sc;
}

/*! Check for has selection, and button is down.
 */
int GroupInterface::GetMode()
{
	if (selection->n() && buttondown.any(0,LEFTBUTTON)) return 1;
	return 0;
}

int GroupInterface::PerformAction(int action)
{
	if (action==GIA_Align) {
		 //change to align interface with the objects
		if (selection->n()<=1) return 0;
		AlignInterface *align=new AlignInterface(NULL,10000,dp);
		align->AddToSelection(selection);
		align->owner=this;
		child=align;
		viewport->Push(align,-1,0);
		PostMessage(_("Align"));
		//viewport->Pop(this);
		FreeSelection();
		return 0;

	} else if (action==GIA_Distribute) {
		 //change to nup interface with the objects
		if (selection->n()<=1) return 0;
		NUpInterface *nup=new NUpInterface(NULL,10001,dp);
		nup->AddToSelection(selection);
		nup->owner=this;
		child=nup;
		viewport->Push(nup,-1,0);
		PostMessage(_("Flow objects"));
		FreeSelection();
		return 0;

	} else if (action == GIA_Edit_Contents) {
		if (selection->n() == 0) return 0;

		if (strcmp(selection->e(0)->obj->whattype(),"Group")) {
			//double click to switch to more specific tool
			if (viewport) viewport->ChangeObject(selection->e(0),1,true);
			buttondown.clear();

		} else { //was group, enter group
			//enter group and clear selection
			ObjectContext *oc = selection->e(0)->duplicate();
			FreeSelection();
			oc->SetObject(NULL);
			((LaidoutViewport *)viewport)->ChangeContext(oc);
			delete oc;
			needtodraw=1;
		}

	} else if (action == GIA_Edit_Clip) {
		if (selection->n()!=1) return 0;

		ObjectContext *oc = selection->e(0);
		DrawableObject *obj = dynamic_cast<DrawableObject*>(oc->obj);
		if (!obj || !obj->clip_path) return 0;

		PathInterface *pathi = new PathInterface(getUniqueNumber(), dp);
		pathi->pathi_style = PATHI_One_Path_Only|PATHI_Esc_Off_Sub|PATHI_Two_Point_Minimum; //|PATHI_Path_Is_M_Real;
		pathi->Setting(PATHI_No_Weights, 1);
		pathi->primary = 1;

		double m[6];
		viewport->transformToContext(m,oc,0,1);
		obj->clip_path->style |= PathsData::PATHS_Ignore_Weights;
		pathi->UseThisObject(obj->clip_path, m);//copies voc

		pathi->owner = this;
		child = pathi;
		viewport->Push(pathi,-1,0);
		PostMessage(_("Editing clip path"));
		//FreeSelection();
		return 0;

	} else if (action == GIA_Clip_First_On_Second || action == GIA_Clip_Second_On_First) {
		if (selection->n() != 2) return 0;
		int toremove = 0; 
		ObjectContext *o1 = selection->e(0);
		ObjectContext *o2 = selection->e(1);
		if (action == GIA_Clip_Second_On_First) {
			ObjectContext *o = o1;
			o1 = o2;
			o2 = o;
			toremove = 1;
		}
		PathsData *paths = dynamic_cast<PathsData*>(o1->obj);
		DrawableObject *dobj = dynamic_cast<DrawableObject*>(o2->obj);
		if (dobj && paths) {
			// remove paths from tree
			viewport->ChangeObject(selection->e(toremove),0,true); 
            viewport->DeleteObject(); 
            selection->Remove(toremove);
			viewport->ChangeObject(selection->e(0),0,true);

			//adjust paths transform
			paths->Multiply(dobj->Inversion());
			paths->style |= PathsData::PATHS_Ignore_Weights;
			paths->fill(NULL);

			if (dobj->InstallClip(paths) == 0) PostMessage(_("Clip path installed"));
			else PostMessage(_("Could not install clip path!"));
		} else PostMessage(_("Can't clip without a path!"));

		return 0;

	} else if (action == GIA_Remove_Clip) {
		for (int c=0; c<selection->n(); c++) {
			DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(c)->obj);
			if (obj && obj->clip_path) {
				obj->InstallClip(NULL);
			}
		}
		PostMessage(_("Clipping removed."));
		return 0;

	} else if (action == GIA_Extract_Clip) {
		//put clip path next to object it was on
		for (int c=0; c<selection->n(); c++) {
			ObjectContext *oc = selection->e(c);
			DrawableObject *obj = dynamic_cast<DrawableObject*>(oc->obj);
			if (obj && obj->clip_path) {
				DrawableObject *o = dynamic_cast<DrawableObject*>(obj->clip_path);
				if (o) {
					o->inc_count();
					o->Multiply(obj->m());
					ObjectContext *noc = NULL;
					viewport->ChangeContext(oc);
					viewport->NewData(o, &noc, false);
					selection->Add(noc, -1, 0);
				}
				obj->InstallClip(NULL);

			}
		}
		PostMessage(_("Extracted"));
		return 0;

	} else if (action == GIA_Edit_Filter_Nodes) {
		//bring up node editor for obj->filter
		if (selection->n() != 1) return 0;

		DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(0)->obj);
		if (!obj) return 0;

		ObjectFilter *filter = dynamic_cast<ObjectFilter*>(obj->filter);
		if (filter) filter->inc_count();
		else {
			filter = new ObjectFilter(obj, 1);
			filter->Id(obj->Id());
			obj->SetFilter(filter, 0);
		}

		NodeInterface *i = new NodeInterface(NULL,10002,dp);
		i->UseThis(filter);
		filter->dec_count();
		// i->owner = this;
		// child = i;
		inc_count();
		ViewerWindow *viewer = dynamic_cast<ViewerWindow *>(curwindow->win_parent);
		viewer->PopInterface(this); //makes sure viewer->curtool is maintained
		// viewport->Pop(this);
		viewport->Push(i,-1,1);
		dec_count();
		PostMessage(_("Edit filter nodes"));

		return 0;

	} else if (action == GIA_Filter_Editor) {
		 //bring up node editor for obj->filter
		if (selection->n() != 1) return 0;

		DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(0)->obj);
		if (!obj) return 0;

		ObjectFilterInterface *i = new ObjectFilterInterface(NULL,10003,dp);
		i->UseThisObject(selection->e(0));
		// i->owner = this;
		// child = i;
		inc_count();
		ViewerWindow *viewer = dynamic_cast<ViewerWindow *>(curwindow->win_parent);
		viewer->PopInterface(this); //makes sure viewer->curtool is maintained
		// viewport->Pop(this);
		viewport->Push(i,-1,1);
		dec_count();
		PostMessage(_("Edit filters"));

		return 0;

	} else if (action == GIA_Remove_Filter) {
		DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(0)->obj);
		if (!obj || !obj->filter) return 0;
		obj->SetFilter(NULL,0);
		PostMessage(_("Filter removed."));
		needtodraw=1;
		return 0;

	} else if (action == GIA_Apply_Filter) {
		PostMessage("IMPLEMENT ME");

		DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(0)->obj);
		if (!obj || !obj->filter) return 0;
		DrawableObject *fobj = dynamic_cast<DrawableObject*>(obj->filter)->FinalObject();
		if (!fobj) {
			PostMessage(_("Filter does not produce an object!"));
			return 0;
		}
		// need to replace in tree and reparent children
		//*** 
		needtodraw = 1;
		return 0;

	} else if (action == GIA_Refresh_Filter) {
		if (!selection->n()) return 0;

		for (int c=0; c<selection->n(); c++) {
			DrawableObject *obj = dynamic_cast<DrawableObject*>(selection->e(c)->obj);
			if (!obj || !obj->filter) continue;;
			ObjectFilter *filter = dynamic_cast<ObjectFilter*>(obj->filter);
			// filter->FindProperty("in")->topropproxy->owner->Update();
			filter->ForceUpdates();
		}

		//PostMessage(_("Filter refresh."));
		needtodraw=1;
		return 0;

	} else if (action==GIA_Clone || action==GIA_CloneB) {
		//if (!buttondown.any()) return 0;

		 // duplicate selection as clones
		SomeData *obj;
		LSomeDataRef *lobj;
		SomeData *toclone;
		for (int c=0; c<selection->n(); c++) {
			obj=NULL;
			toclone=selection->e(c)->obj;
			DBG cerr <<" - Clone "<<toclone->whattype()<<":"<<toclone->object_id<<endl;

			lobj=new LSomeDataRef();
			lobj->Set(toclone,0);
			lobj->m(toclone->m());
			lobj->SetParent(toclone->GetParent());
			obj=lobj;
			viewport->ChangeContext(selection->e(c));
			viewport->NewData(obj,NULL, false);
			obj->dec_count();
			PostMessage(_("Cloned."));
		}
		return 0;

	} else if (action==GIA_Duplicate || action==GIA_DuplicateB) {
		 // duplicate selection
		SomeData *obj;
		for (int c=0; c<selection->n(); c++) {
			obj=NULL;
			DBG cerr <<" - Duplicate "<<selection->e(c)->obj->whattype()<<":"<<selection->e(c)->obj->object_id<<endl;

			DrawableObject *dobj = dynamic_cast<DrawableObject*>(selection->e(c)->obj);
			anObject *filter = dobj->filter;
			if (filter) dobj->filter = nullptr;
			obj = dobj->duplicate(NULL);
			// *** duplicate filter separately for safety... otherwise needs a ton of overloaded funcs to deal with.. needs more thought
			if (filter) {
				ObjectFilter *nfilter = dynamic_cast<ObjectFilter*>(dynamic_cast<ObjectFilter*>(filter)->Duplicate());
				DrawableObject *dup = dynamic_cast<DrawableObject*>(obj);
				dup->filter = nfilter;
				nfilter->SetParent(dup);
			}
			if (filter) dobj->filter = filter; //put filter back on
			obj->FindBBox();
			obj->m(selection->e(c)->obj->m());

			viewport->ChangeContext(selection->e(c));
			viewport->NewData(obj,NULL, false); //should push on top of stack, so remapping current objects shouldn't be necessary
			obj->dec_count();
			PostMessage(_("Duplicated."));
		}
		return 0;

	} else if (action == GIA_Jump_To_Link) {
		LaidoutViewport *vp = ((LaidoutViewport *)viewport);

		LSomeDataRef *ref = dynamic_cast<LSomeDataRef*>(selection->e(0)->obj);
		if (!ref || !ref->thedata) {
			PostMessage(_("Can't jump to object"));
			return 0;
		}
		SomeData *obj = ref->thedata;
		VObjContext context;
		context.SetObject(obj);
		if (vp->locateObject(obj,context.context)) {
			//update viewport to view that object, it was found in current spread
			vp->ChangeObject(&context,0,true);
			FreeSelection();
			AddToSelection(&context);
		} else {
			//object was not found in current spread, so search in all limbos and docs
			int found=laidout->project->FindObject(obj,context.context);
			if (found && context.context.e(0)==0) {
				//object was found in a limbo
			} else if (found) {
				//object was found in a document page
				//place 0 is doc, 1 is page number
				vp->SelectPage(context.context.e(1));
			} else found=0;

			if (found && vp->locateObject(obj,context.context)) {
				FreeSelection();
				AddToSelection(&context);
			} else {
				//FreeSelection();
				PostMessage(_("Can't jump to object"));
			}
		}
		hover=0;
		needtodraw=1;
		return 0;

	} else if (action == GIA_Sever_Link) {
		LaidoutViewport *vp = ((LaidoutViewport *)viewport);
		LSomeDataRef *ref = dynamic_cast<LSomeDataRef*>(selection->e(0)->obj);
		if (!ref || !ref->thedata) {
			PostMessage(_("Nothing to sever!"));
			return 0;
		}
		SomeData *obj = ref->thedata->duplicate(NULL);
		obj->m(ref->m());
		obj->FindBBox();
		selection->e(0)->SetObject(obj);
		vp->DeleteObject();
		vp->NewData(obj,NULL);
		PostMessage(_("Severed."));
		hover=0;
		needtodraw=1;
		return 0;

	} else if (action==GIA_Jump_To_Parent) {
		if (selection->n()==0) return 0;

		LaidoutViewport *vp = ((LaidoutViewport *)viewport);
		//VObjContext oc;
		//oc = *dynamic_cast<VObjContext*>(selection->e(0));
		VObjContext oc = *dynamic_cast<VObjContext*>(selection->e(0));
		oc.SetObject(NULL);
		oc.context.pop();

		DrawableObject *pp=dynamic_cast<DrawableObject*>(vp->getanObject(oc.context,0,0));

		if (pp && pp->Selectable()) {
			oc.SetObject(pp);
			vp->ChangeObject(&oc,0,true);
			FreeSelection();
			AddToSelection(&oc);
			((LaidoutViewport *)viewport)->clearCurobj();
			((LaidoutViewport *)viewport)->ClearSearch();

		} else {
			PostMessage(_("Cannot select parent"));
		}

		return 0;

	} else if (action == GIA_Reparent) {
		if (selection->n()<=1) { PostMessage(_("Need at least 2 objects to do parenting")); return 0; }

		DrawableObject *newparent=dynamic_cast<DrawableObject*>(selection->e(selection->n()-1)->obj);
		DrawableObject *oldparent;
		ObjectContext *oc;
		double m[6],mnewinv[6],mnew[6],mm[6];
		viewport->transformToContext(mnew,selection->e(selection->n()-1),0,1);
		transform_invert(mnewinv,mnew);

		for (int c=0; c<selection->n()-1; c++) {
			oc=selection->e(c);
			oldparent=NULL;

			if (dynamic_cast<DrawableObject*>(oc->obj)) {
				oldparent=dynamic_cast<DrawableObject*>(oc->obj->GetParent());
				viewport->transformToContext(m,oc,0,1);

				if (oldparent && oldparent->popp(oc->obj)) {
					 //successful popping, now add to selection n-1
					transform_mult(mm,m,mnewinv);
					oc->obj->m(mm);
					newparent->push(oc->obj);
					oc->obj->dec_count(); //from the popp
				}
			}
		}

		((LaidoutViewport *)viewport)->clearCurobj();
		FreeSelection();
		reparent_temp.SetObject(NULL);
		PostMessage(_("Parented."));
		needtodraw=1;
		return 0;

	} else if (action==GIA_Unparent) {
		if (selection->n()==0) return 0;

		 //Move object to its parent's context
		ObjectContext *oc;
		DrawableObject *obj, *parent, *gparent;
		double m[6];
		int n=0;

		for (int c=0; c<selection->n(); c++) {
			oc=selection->e(c);
			obj = dynamic_cast<DrawableObject*>(oc->obj);
			if (!obj) continue;
			parent = dynamic_cast<DrawableObject*>(obj->GetParent());
			if (!parent || !parent->Selectable()) continue;
			gparent = dynamic_cast<DrawableObject*>(parent->GetParent());
			if (!gparent) continue;

			parent->popp(obj);
			//transform_invert(mm, parent->m());
			//transform_mult(m, obj->m(), mm);
			transform_mult(m, obj->m(), parent->m());
			obj->m(m);
			gparent->push(obj);
			obj->dec_count(); //from popp
			n++;
		}

		 //remap selection object places
		LaidoutViewport *lvp = (LaidoutViewport *)viewport;
		for (int c=0; c<selection->n(); c++) {
			if (lvp->IsValidContext(selection->e(c))) continue;
			lvp->locateObject(selection->e(c)->obj, dynamic_cast<VObjContext*>(selection->e(c))->context);
		}

		needtodraw=1;
		((LaidoutViewport *)viewport)->clearCurobj();
		//FreeSelection();
		if (n) PostMessage(_("Unparented."));
		else   PostMessage(_("Could not unparent."));
		return 0;

	} else if (action == GIA_Edit_Object_Meta) {
		if (!selection->n()) return 0;

		AttributeObject *meta = nullptr;
		if (selection->n() == 1) {
			ObjectContext *oc = selection->e(0);
			DrawableObject *obj = dynamic_cast<DrawableObject*>(oc->obj);
			meta = obj->metadata;
		
			app->addwindow(new MetaWindow(NULL,"meta",_("Object Meta"),0, object_id,"objectMeta", meta));
		}
		
		return 0;
	} else if (action == GIA_Create_Empty) {
		GroupData *g = dynamic_cast<GroupData *>(InterfaceManager::GetDefault()->NewObject(LAX_GROUPDATA));
		DrawableObject *d = dynamic_cast<DrawableObject*>(g);
		double halfdim = .5; // / dp->Getmag();
		d->minx = -halfdim;
		d->maxx = halfdim;
		d->miny = -halfdim;
		d->maxy = halfdim;
		char *id = make_id(_("Empty"));
		d->Id(id);
		delete[] id;

		viewport->NewData(d, nullptr);
		d->dec_count();
		d->origin(dp->screentoreal(rx,ry)); // *** FIXME!! not proper transform when nested

		return 0;
	}

	return ObjectInterface::PerformAction(action);
}


int GroupInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<" ****************GroupInterface::CharInput"<<endl;

	if (ch == LAX_Esc && child) {
		RemoveChild();
		needtodraw=1;
		return 0;
	}

	return ObjectInterface::CharInput(ch,buffer,len,state,d);
}

int GroupInterface::Refresh()
{
	if (!needtodraw) return 0;
	if (child) { needtodraw=0; return 0; }


	ObjectInterface::Refresh();

	if (selection->n()==0) return 0;


	 //if we are moving only one object, then put various doodads around it.
	 
	//DoubleBBox box;
	//GetOuterRect(&box,NULL);


	 //if is link, draw clone options
	if (selection->n()==1 && !strcmp(selection->e(0)->obj->whattype(),"SomeDataRef")) {
		SomeData *obj=selection->e(0)->obj;
		double m[6];
		viewport->transformToContext(m,selection->e(0),0,1);

		flatpoint p =transform_point(m, (flatpoint(obj->minx,obj->miny)+flatpoint(obj->maxx,obj->maxy))/2); //center
		flatpoint p2=transform_point(m, (flatpoint(obj->minx,obj->miny)+flatpoint(obj->maxx,obj->miny))/2); //edge midpoint

		p=dp->realtoscreen(p);
		p2=dp->realtoscreen(p2);

		flatpoint v=p2-p;
		v.normalize();
		p=p2+v*maxtouchlen;

		dp->DrawScreen();
		dp->NewFG(0.,.7,0.);
		dp->drawpoint(p, maxtouchlen/2,hover==GIA_Link?1:0);
		//dp->PushAndNewTransform(m);
		//dp->PopAxes();

		if (popupcontrols == GIA_Link) {
			double th=dp->textheight();
			double w=dp->textextent(_("Original"),-1,NULL,NULL)*1.5;
			p.y+=2*th;
			p.x-=w/2;
			if (p.y+th>dp->Maxy) p.y=dp->Maxy-th;
			dp->NewFG(.8,.8,.8);
			if (hover==GIA_Jump_To_Link) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			dp->drawellipse(p.x,p.y, w/2,th*1.5, 0,2*M_PI, 2);
			if (hover==GIA_Sever_Link) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			dp->drawellipse(p.x+w,p.y, w/2,th*1.5, 0,2*M_PI, 2);

			dp->NewFG(0.,0.,0.);
			dp->textout(p.x,p.y, _("Jump to"),-1, LAX_HCENTER|LAX_BOTTOM);
			dp->textout(p.x,p.y+th, _("Original"),-1, LAX_HCENTER|LAX_BOTTOM);
			p.x+=w;
			dp->textout(p.x,p.y, _("Sever"),-1, LAX_HCENTER|LAX_BOTTOM);
			dp->textout(p.x,p.y+th, _("Link"),-1, LAX_HCENTER|LAX_BOTTOM);

		}

		dp->DrawReal();
	}


	 //parent link
	if (selection->n()) {
		//double m[6];

		flatpoint p =transform_point(somedata->m(), (flatpoint(somedata->minx,somedata->miny)+flatpoint(somedata->maxx,somedata->maxy))/2); //center
		flatpoint p2=transform_point(somedata->m(), flatpoint(somedata->minx,somedata->maxy)); //upper left
		p=dp->realtoscreen(p);
		p2=dp->realtoscreen(p2);

		flatpoint v=p2-p;
		v.normalize();
		p=p2+v*maxtouchlen*2;

		dp->DrawScreen();
		dp->NewFG(0.,.7,0.);
		//dp->drawpoint(p, maxtouchlen/2,hover==GIA_Parent_Link?1:0);
		dp->moveto(p);
		dp->lineto(p-v*maxtouchlen+transpose(v)*maxtouchlen/2);
		dp->lineto(p-v*maxtouchlen-transpose(v)*maxtouchlen/2);
		dp->closed();
		if (hover==GIA_Parent_Link) dp->fill(0); else dp->stroke(0);

		if (popupcontrols == GIA_Parent_Link) {
			double th=dp->textheight();
			double w=dp->textextent(_("Jump to parent"),-1,NULL,NULL)*1.5;

			p.y-=3*th;
			if (p.y-th<dp->Miny) p.y=dp->Miny+th;
			if (p.x-w*.5<dp->Minx) p.x=dp->Minx+w*.5;
			dp->NewFG(.8,.8,.8);

			//if (selection->n()==1) {
			//	if (hover==GIA_Parent_Matrix) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			//	dp->drawellipse(p.x-w,p.y, w/2,th, 0,2*M_PI, 2);
			//	if (hover==GIA_Parent_Align) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			//	dp->drawellipse(p.x-w,p.y+2*th, w/2,th, 0,2*M_PI, 2);
			//}
			//----
			//if (selection->n()>1) {
			//	if (hover==GIA_Reparent) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			//	dp->drawellipse(p.x,p.y, w/2,th, 0,2*M_PI, 2);
			//}
			//if (hover==GIA_Jump_To_Parent) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			//----
			if (hover==GIA_Reparent || hover==GIA_Jump_To_Parent) dp->NewBG(.9,.9,.9); else dp->NewBG(1.,1.,1.);
			dp->drawellipse(p.x,p.y+2*th, w/2,th, 0,2*M_PI, 2);
			//----

			dp->NewFG(0.,0.,0.);
			//if (selection->n()==1) {
			//	dp->textout(p.x-w,p.y,     _("Matrix"),-1,  LAX_CENTER);
			//	dp->textout(p.x-w,p.y+2*th,_("Align"),-1,   LAX_CENTER);
			//}
			if (selection->n()>1) dp->textout(p.x,p.y+2*th, _("Reparent"),  -1, LAX_CENTER);
			else dp->textout(p.x,p.y+2*th,_("Jump to parent"),-1,LAX_CENTER);

		}

		dp->DrawReal();

		 //draw heavy outline of object to reparent to
		if (reparent_temp.obj) {
			dp->NewFG(0.,.7,0.);
			dp->DrawScreen();
			dp->LineAttributes(-1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
			dp->LineWidthScreen(3);
			double m[6];
			viewport->transformToContext(m,&reparent_temp,0,1);
			dp->moveto(dp->realtoscreen(transform_point(m,reparent_temp.obj->minx,reparent_temp.obj->miny)));
			dp->lineto(dp->realtoscreen(transform_point(m,reparent_temp.obj->maxx,reparent_temp.obj->miny)));
			dp->lineto(dp->realtoscreen(transform_point(m,reparent_temp.obj->maxx,reparent_temp.obj->maxy)));
			dp->lineto(dp->realtoscreen(transform_point(m,reparent_temp.obj->minx,reparent_temp.obj->maxy)));
			dp->closed();
			dp->stroke(0);
			dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

			DBG cerr << "GroupInterface hover: "<<hoverMessage(hover)<<endl;
			if (hover == GIA_Reparent && selection->n()>1) DrawReparentArrows();
			dp->DrawReal();
		}

	}
	

	 //object edit zone
	//***
	

	 //transform constraints
	//***


	 //chains
	//***

	return 0;
}

void GroupInterface::DrawReparentArrows()
{
	//assume dp->DrawScreen() already
	SomeData *obj = selection->e(selection->n()-1)->obj;
	double m[6];
	viewport->transformToContext(m,selection->e(selection->n()-1),0,1);

	flatpoint pp;
	flatpoint p = transform_point(m, (flatpoint(obj->minx,obj->miny)+flatpoint(obj->maxx,obj->maxy))/2); //new parent center
	p = dp->realtoscreen(p);
	double th = dp->textheight();

	dp->NewBG(0.,0.,0.,.85);
	dp->LineAttributes(-2,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
	dp->LineWidthScreen(2);
	for (int c=0; c<selection->n()-1; c++) {
		obj = selection->e(c)->obj;
		viewport->transformToContext(m,selection->e(c),0,1);
		pp = transform_point(m, (flatpoint(obj->minx,obj->miny)+flatpoint(obj->maxx,obj->maxy))/2); //center
		pp = dp->realtoscreen(pp);
		dp->NewFG(0.,.7,0.);
		dp->drawline(pp, p);
		//dp->drawarrow(pp,p-pp, 0,1,2,3);
		dp->NewFG(0.,1.,0.);
		dp->textout_bg(th/4, pp.x,pp.y, _("Child"),-1, LAX_CENTER);
	}

	dp->NewBG(0.,0.,0.,.85);
	dp->NewFG(1.,1.,1.,1.);
	dp->textout_bg(th/4, p.x,p.y, _("Parent"),-1, LAX_CENTER, true);

	//dp->NewBG(0.,0.,0.,.85);
	//dp->NewFG(1.,1.,1.,1.);
	//dp->textout_bg(th/4, p.x,p.y,        _("Parent"),-1, LAX_CENTER);
}

//! Returns this, but count is incremented.
Value *GroupInterface::duplicate()
{
    this->inc_count();
    return this;
}


ObjectDef *GroupInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ObjectInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"ObjectInterface",
            _("Group Interface"),
            _("Group Interface"),
            "class",
            NULL,NULL);

	if (!sc) sc=GetShortcuts();
	ShortcutsToObjectDef(sc, sd);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}


///*!
// * Return
// *  0 for success, value optionally returned.
// * -1 for no value returned due to incompatible parameters, which aids in function overloading.
// *  1 for parameters ok, but there was somehow an error, so no value returned.
// */
//int GroupInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
//{
//	return 1;
//}

/*! *** for now, don't allow assignments
 *
 * If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int GroupInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *GroupInterface::dereference(const char *extstring, int len)
{
	return NULL;
}


void GroupInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	ObjectInterface::dump_out(f,indent,what,context);
}

void GroupInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	ObjectInterface::dump_in_atts(att,flag,context);
}

LaxFiles::Attribute *GroupInterface::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	return att;
}

} //namespace Laidout

