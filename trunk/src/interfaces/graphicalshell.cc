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
// Copyright (C) 2013 by Tom Lechner
//


#include "../language.h"
#include "graphicalshell.h"
#include "../viewwindow.h"
#include "../drawdata.h"
#include "../dataobjects/printermarks.h"
#include <lax/inputdialog.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/colors.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {



//------------------------------------- GraphicalShellEdit --------------------------------------

class GraphicalShellEdit : public Laxkit::LineEdit
{
  public:
	GraphicalShell *shell;
	GraphicalShellEdit(anXWindow *viewport, GraphicalShell *sh, int x,int y,int w,int h);
	virtual ~GraphicalShellEdit();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
};

GraphicalShellEdit::GraphicalShellEdit(anXWindow *viewport, GraphicalShell *sh, int x,int y,int w,int h)
  : LineEdit(viewport, "Shell",_("Shell"),
		     LINEEDIT_SEND_ANY_CHANGE | LINEEDIT_GRAB_ON_MAP | ANXWIN_HOVER_FOCUS,
			 x,y,w,h,0, NULL,sh->object_id,"shell",NULL,0)
{
	shell=sh;
}

GraphicalShellEdit::~GraphicalShellEdit()
{
}

int GraphicalShellEdit::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		 //turn off gshell
		dynamic_cast<ViewerWindow*>(shell->CurrentWindow(NULL)->win_parent)->SelectTool(shell->id);
		return 0;
	}

	return LineEdit::CharInput(ch,buffer,len,state,d);
}


//------------------------------------- GraphicalShell --------------------------------------
	
/*! \class GraphicalShell 
 * \brief Interface to allow input and something like tab completion for easy access to whatever is available with scripting.
 *
 */

GraphicalShell::GraphicalShell(int nid,Displayer *ndp)
	: anInterface(nid,ndp),
	  history(2),
	  completion(2)
{
	showdecs=0;
	doc=NULL;
	sc=NULL;
	interface_type=INTERFACE_Overlay;
	le=NULL;
	active=0;
	pad=5;
	boxcolor.rgbf(.5,.5,.5);

	placement_gravity=LAX_BOTTOM;
	showcompletion=0;
}

GraphicalShell::GraphicalShell(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	showdecs=0;
	doc=NULL;
	sc=NULL;
	interface_type=INTERFACE_Overlay;
	le=NULL;
	active=0;
	pad=10;
	boxcolor.rgbf(.5,.5,.5);

	placement_gravity=LAX_BOTTOM;
	showcompletion=0;
}

GraphicalShell::~GraphicalShell()
{
	DBG cerr <<"GraphicalShell destructor.."<<endl;

	if (doc) doc->dec_count();
	if (sc) sc->dec_count();
}

const char *GraphicalShell::Name()
{ return _("Shell"); }


int GraphicalShell::Setup()
{
	// TODO
	//
	// code minimum: Value functions for viewport, a tool, its object
	//   needs object extends DrawableObject, extends Affine, DoubleBBox
	//
	//
	//Bring in ObjectDefs from default areas:
	//  LaidoutViewport aka "Viewport"
	//  Current tool
	//  current object
	//
	//  todo: current selection
	//
	//  Do special check for changing tools to something else. Special ObjectDef where each tool
	//  has a function that establishes that tool in viewport
	//  namespace Select:
	//    Select.ObjectInterface
	//    Select.ImageInterface
	//    Select. ...
	//  or maybe shortcut to:
	//    viewport.SelectTool("Object")
	//    viewport.SelectTool("Image")


	if (!le) {
		double width=dp->Maxx-dp->Minx-2*pad;
		double height=dp->textheight()*1.5;
		le=new GraphicalShellEdit(viewport, this, dp->Minx+pad,dp->Maxy-height-pad,width,height);
		box.minx=le->win_x-pad;
		box.maxx=le->win_x+le->win_w+pad;
		box.miny=le->win_y-pad;
		box.maxy=le->win_y+le->win_h+pad;
		app->addwindow(le,1,1);

	} else {
		//*** make sure prompt is still in window
		box.minx=le->win_x-pad;
		box.maxx=le->win_x+le->win_w+pad;
		box.miny=le->win_y-pad;
		box.maxy=le->win_y+le->win_h+pad;
		if (box.minx<0) {
			double a=box.minx;
			box.minx-=a;
			box.maxx-=a;
		}
	}
	app->mapwindow(le);
	Update();

	return 0;
}

int GraphicalShell::ChangeContext(const char *name, Value *value)
{
	if (!context.find(name)) {
		 //unknown context name
		context.push(name,value);
	} else context.set(name,value);

	//if (!strcmp(name,"object")) {
	//if (!strcmp(name,"selection")) {}
	//if (!strcmp(name,"viewport")) {}
	//if (!strcmp(name,"tool")) {}

	return 0;
}

//! Update context from viewport.
int GraphicalShell::Update()
{
	context.pushObject("viewport",viewport);
	context.pushObject("tool",dynamic_cast<ViewerWindow*>(viewport->win_parent)->CurrentTool());
	context.pushObject("object",dynamic_cast<LaidoutViewport*>(viewport)->curobj.obj);
	//context.push("selection",viewport->selection);
	//context.pushOrSet("",);

	//-----
	//contextdef.pushVariable("viewport",NULL,NULL, v,0);
	//contextdef.pushVariable("tool",    NULL,NULL, v,0);
	//contextdef.pushVariable("object",  NULL,NULL, v,0);
	
	tree.Flush();
	ObjectDef *def;
	const char *name;
	const char *area;
	for (int c=0; c<context.n(); c++) {
		area=context.key(c);
		tree.AddItem(area);
		def=context.value(c)->GetObjectDef();
		if (def && def->getNumFields()) {
			tree.SubMenu();
			for (int c2=0; c2<def->getNumFields(); c2++) {
				name=NULL;
				def->getInfo(c2, &name);
				if (name) {
					tree.AddItem(name);
				}
			}
			tree.EndSubMenu();
		}
	}

	return 0;
}

int GraphicalShell::UpdateCompletion()
{
	tree.Search(le->GetCText(),1,1);
	return 0;
}


//menu ids for context menu
//#define GSHELLM_PaperSize        1


Laxkit::MenuInfo *GraphicalShell::ContextMenu(int x,int y,int deviceid)
{
	return NULL;
//	rx=x,ry=y;
//	MenuInfo *menu=new MenuInfo(_("Paper Interface"));
//
//	menu->AddItem(_("Add Registration Mark"),GSHELLM_RegistrationMark);
//	menu->AddSep();
//
//	return menu;
}

void GraphicalShell::ViewportResized()
{
	if (placement_gravity==LAX_BOTTOM) {
		double width=dp->Maxx-dp->Minx-2*pad;
		double height=dp->textheight()*1.5;
		le->MoveResize(dp->Minx+pad,dp->Maxy-height-pad, width,height);
		box.minx=le->win_x-pad;
		box.maxx=le->win_x+le->win_w+pad;
		box.miny=le->win_y-pad;
		box.maxy=le->win_y+le->win_h+pad;
	}
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int GraphicalShell::Event(const Laxkit::EventData *e,const char *mes)
{

	if (!strcmp(mes,"shell")) {
		 //input from the lineedit
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 1;
		int type=s->info1;
		if (areas.n) {
			areas.flush();
			needtodraw=1;
		}
		if (type==0) {
			if (!isblank(s->str)) {
				 //text was changed
				ObjectDef *def;
				for (int c=0; c<context.n(); c++) {
					if (strcasestr(context.key(c),s->str)==context.key(c)) {
						def= context.value(c)->GetObjectDef();
						if (def) areas.push(def);
					}
				}
			} else {
				tree.ClearSearch();
			}
			UpdateCompletion();
			needtodraw=1;
			return 0;

		} else if (type==1) {
			 //enter pressed
			//execute command
			char *command=le->GetText();
			if (isblank(command)) return 0;
			int answertype;
			char *result=calculator.In(command,&answertype);

			 //update history
			if (answertype==1 || answertype==2) {
				 //successful command
				le->SetText("");
				history.push(command);
				PostMessage(result);
			}
			if (answertype==1 && !isblank(result)) history.push(result);
			
			UpdateCompletion();
			needtodraw=1;
			return 0;
		}

		return 0;
	}

//	if (!strcmp(mes,"menuevent")) {
//		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
//		int i=s->info2; //id of menu item
//		if (i==GSHELLM_Portrait) {
//			 //portrait
//			return 0;
//
//		} else if (i==GSHELLM_Landscape) {
//		}
//
//	}

	return 1;
}

/*! incs count of ndoc if ndoc is not already the current document.
 *
 * Return 0 for success, nonzero for fail.
 */
int GraphicalShell::UseThisDocument(Document *ndoc)
{
	if (ndoc==doc) return 0;
	if (doc) doc->dec_count();
	doc=ndoc;
	if (ndoc) ndoc->inc_count();
	return 0;
}

/*! None.
 */
int GraphicalShell::draws(const char *atype)
{
	return 0;
}


//! Return a new GraphicalShell if dup=NULL, or anInterface::duplicate(dup) otherwise.
anInterface *GraphicalShell::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new GraphicalShell(id,NULL);
	else if (!dynamic_cast<GraphicalShell *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int GraphicalShell::InterfaceOn()
{
	DBG cerr <<"gshell On()"<<endl;
	Setup();
	showdecs=1;
	needtodraw=1;
	return 0;
}

int GraphicalShell::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;

	if (le) app->unmapwindow(le);

	needtodraw=1;
	DBG cerr <<"gshell Off()"<<endl;
	return 0;
}

int GraphicalShell::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	return 0;
}

void GraphicalShell::Clear(SomeData *d)
{
}


int GraphicalShell::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	dp->DrawScreen();
	dp->NewFG(&boxcolor);
	dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx, box.maxy-box.miny, 1);


	if (showcompletion) {
		int y=box.miny-pad;
		y=box.miny-pad;
		RefreshTree(&tree,box.minx,y);
	}

	dp->DrawReal();
	return 1;
}

/*! Draw whole search results
 */
void GraphicalShell::RefreshTree(MenuInfo *menu, int x,int &y)
{
	MenuItem *mi, *mii;
	for (int c=0; c<menu->n(); c++) {
		mi=menu->e(c);
		if (mi->state&MENU_SEARCH_HIT) {
			int xx=x;
			mii=mi;

			DrawName(mii,xx,y);
			y-=dp->textheight();
		}
		if (!(mi->state&(MENU_SEARCH_PARENT|MENU_SEARCH_HIT))) continue;

		if (!mi->GetSubmenu(0)) continue;
		RefreshTree(mi->GetSubmenu(0), x,y);
	}
}

/*! Recursively draw hit name with parent names before it.
 */
void GraphicalShell::DrawName(MenuItem *mii, int &x,int y)
{
	if (!mii) return;
	MenuInfo *mif=mii->parent;
	if (mif && mif->parent) {
		DrawName(mif->parent, x,y);
		x+=dp->textout(x,y, ".",1,LAX_LEFT|LAX_BOTTOM);
	}

	x+=dp->textout(x,y, mii->name,-1,LAX_LEFT|LAX_BOTTOM);
}

//! Return the papergroup->papers element index underneath x,y, or -1.
int GraphicalShell::scan(int x,int y)
{
	return -1;
}

/*! Add maybe box if shift is down.
 */
int GraphicalShell::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;
	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	DBG flatpoint fp;
	DBG fp=dp->screentoreal(x,y);

	int over=scan(x,y);

	DBG fp=dp->screentoreal(x,y);
	if ((state&LAX_STATE_MASK)==ShiftMask && over<0) {
		needtodraw=1;
	}


	return 0;
}

int GraphicalShell::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	buttondown.up(d->id,LEFTBUTTON);

	DBG flatpoint fp=dp->screentoreal(x,y);
	DBG cerr <<"9 *****ARG**** "<<fp.x<<","<<fp.y<<endl;

	//***
	//if (curbox) { curbox->dec_count(); curbox=NULL; }
	//if (curboxes.n) curboxes.flush();

	return 0;
}

int GraphicalShell::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{

	//special move outs for different kinds when inputing parameters of known types.
	//  -> flatpoints, drag out a circle hovering above input area.. cursor keys to move it around,
	//  		modifiers to move at different speeds. Updates number in command box
	//  -> numbers, control to drag out values
	//  -> colors, control to pop up a color selector


	DBG flatpoint fpp=dp->screentoreal(x,y);
	DBG cerr <<"mm *****ARG**** "<<fpp.x<<","<<fpp.y<<endl;

	int over=scan(x,y);

	DBG cerr <<"over box: "<<over<<endl;

	int mx,my;
	buttondown.move(mouse->id,x,y, &mx,&my);
	if (!buttondown.any()) {
		if (!(state&ShiftMask)) return 1;
		needtodraw=1;
		return 0;
	}


	//flatpoint fp=dp->screentoreal(x,y);
	//flatpoint d =fp-dp->screentoreal(mx,my);

	 //plain or + moves curboxes (or the box given by editwhat)
	if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
		needtodraw=1;
		return 0;
	}

	return 0;
}

enum GraphicalShellActions {
	GSHELL_Activate,
	GSHELL_ToggleMinimized,
	GSHELL_Decorations,
	GSHELL_MAX
};

Laxkit::ShortcutHandler *GraphicalShell::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("GraphicalShell");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("GraphicalShell");

	sc->Add(GSHELL_Activate,    '/',0,0,         "Activate",  _("Turn on the prompt, if it was off."),NULL,0);
	sc->Add(GSHELL_Decorations, 'd',0,0,         "Decs",    _("Toggle decorations"),NULL,0);


	manager->AddArea("GraphicalShell",sc);
	return sc;
}

int GraphicalShell::PerformAction(int action)
{
	if (action==GSHELL_Decorations) {
		showdecs++;
		if (showdecs>2) showdecs=0;
		needtodraw=1;
		return 0;

	} else if (action==GSHELL_Activate) {
		if (!le) {
			Setup();
		}
		return 0;
	}

	return 1;
}

int GraphicalShell::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1;
}

int GraphicalShell::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
//	if (ch==LAX_Shift) {
//		if (!maybebox) return 1;
//		maybebox->dec_count();
//		maybebox=NULL;
//		needtodraw=1;
//		return 0;
//	}
//	return 1;
}


} // namespace Laidout

