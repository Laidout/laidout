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
	virtual int send(int i);
};

GraphicalShellEdit::GraphicalShellEdit(anXWindow *viewport, GraphicalShell *sh, int x,int y,int w,int h)
  : LineEdit(viewport, "Shell",_("Shell"),
		     LINEEDIT_SEND_ANY_CHANGE | LINEEDIT_GRAB_ON_MAP | ANXWIN_HOVER_FOCUS
			  | LINEEDIT_SEND_FOCUS_ON | LINEEDIT_SEND_FOCUS_OFF,
			 x,y,w,h,0, NULL,sh->object_id,"shell",NULL,0)
{
	shell=sh;
}

GraphicalShellEdit::~GraphicalShellEdit()
{
}

int GraphicalShellEdit::send(int i)
{
	if (i<10) return LineEdit::send(i);

	SimpleMessage *data=new SimpleMessage(NULL, i, 0, curpos,0, win_sendthis);
	app->SendMessage(data, win_owner, win_sendthis, object_id);
	return 0;
}

int GraphicalShellEdit::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	//from LineEdit::send():
	//If i==0, then the text was modified.
	//If i==1, then enter was pressed.
	//If i==2, then the edit got the focus.
	//If i==3, then the edit lost the focus.
#define GSHELL_TextChanged  0
#define GSHELL_Enter        1
#define GSHELL_FocusIn      2
#define GSHELL_FocusOut     3
#define GSHELL_HistoryUp    10
#define GSHELL_HistoryDown  11
#define GSHELL_Tab          12
#define GSHELL_ShiftTab     13
#define GSHELL_Esc          14

	if (ch==LAX_Esc) {
		 //turn off gshell
		send(GSHELL_Esc);
		return 0;

	} else if (ch==LAX_Up) {
		send(GSHELL_HistoryUp);
		return 0;

	} else if (ch==LAX_Down) {
		send(GSHELL_HistoryDown);
		return 0;

	} else if (ch=='\t') {
		if ((state&LAX_STATE_MASK)==0) send(GSHELL_Tab);
		else if ((state&LAX_STATE_MASK)==ShiftMask) send(GSHELL_ShiftTab);
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
	  completion(0)
{
	base_init();
}

GraphicalShell::GraphicalShell(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp),
	  history(2),
	  completion(0)
{
	base_init();
}

void GraphicalShell::base_init()
{
	showdecs=0;
	doc=NULL;
	sc=NULL;
	interface_type=INTERFACE_Overlay;
	le=NULL;
	active=0;
	pad=10;
	boxcolor.rgbf(.5,.5,.5);
	showerror=0;

	currenthistory=-1;
	tablevel=-1;
	numresults=0;
	searchterm=NULL;
	searcharea=NULL;
	searcharea_str=NULL;

	placement_gravity=LAX_BOTTOM;
	showcompletion=0;
}

GraphicalShell::~GraphicalShell()
{
	DBG cerr <<"GraphicalShell destructor.."<<endl;

	if (doc) doc->dec_count();
	if (sc) sc->dec_count();
	if (searchterm) delete[] searchterm;
	if (searcharea_str) delete[] searcharea_str;
	if (searcharea) searcharea->dec_count();

	// *** NEED to figure out ownership protocol here!!! -> if (le) app->destroywindow(le);
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
		app->addwindow(le,1,0);

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
	InitAreas();

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
int GraphicalShell::InitAreas()
{
	context.flush();
	context.pushObject("viewport",viewport);
	context.pushObject("tool",dynamic_cast<ViewerWindow*>(viewport->win_parent)->CurrentTool());
	context.pushObject("object",dynamic_cast<LaidoutViewport*>(viewport)->curobj.obj);
	//context.push("selection",viewport->selection);
	//context.pushOrSet("",);

	calculator.InstallVariables(&context);

	
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

	UpdateSearchTerm(NULL,0, 1);

	//DBG menuinfoDump(&tree,0);

	return 0;
}

int GraphicalShell::UpdateCompletion()
{
	numresults=completion.Search(searchterm,1,1);
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

/*! Grab the portion of string that seems self contained. For instance, a string
 * of "1+Math.pi" would set searchterm to "Math.pi".
 *
 * This does not update the results of a search, only what and where to search.
 */
void GraphicalShell::UpdateSearchTerm(const char *str,int pos, int firsttime)
{
	//viewport.blah|  
	//viewport.spread.1.|
	//object.scale((1,0), | ) <- get def of function, keep track of which parameters used?
	//object.1.(3+23).|
	//
	int len=(str?strlen(str):0);
	if (pos<=0) pos=len-1;
	int last=pos, pose=pos;
	const char *first=NULL;
	while (pos>0) {
		while (pos>0 && isspace(str[pos])) pos--;
		while (pos>0 && isalnum(str[pos])) pos--;
		if (!first) { first=str+pos; pose=pos; }
		if (pos>0 && str[pos]=='.') pos--;
		else break; //do not parse backward beyond a whole term
	}

	if (first && *first=='.') first++;
	makenstr(searchterm,first,last-pos+1);
	if (!searchterm) searchterm=newstr("");

	char *areastr=newnstr(str+pos,pose-pos);
	ObjectDef *area=GetContextDef(areastr);
	if (!area) area=calculator.GetInfo(areastr);

	if (area!=searcharea || firsttime) {
		if (searcharea) searcharea->dec_count();
		searcharea=area;
		if (area) area->inc_count();
		makestr(searcharea_str,areastr);
		char *s;

		completion.Flush();

		if (searcharea) {
			const char *name;
			for (int c=0; c<searcharea->getNumFields(); c++) {
				name=NULL;
				searcharea->getInfo(c, &name);
				if (name) {
					if (searcharea_str) {
						s=newstr(searcharea_str);
						appendstr(s,".");
					} else s=NULL;
					appendstr(s,name);

					completion.AddItem(s);
					delete[] s;
				}
			}

		} else { //no distinct search area, so use all of tree
			AddTreeToCompletion(&tree);
			//DBG menuinfoDump(&completion,0);
		}
	}
}

ObjectDef *GraphicalShell::GetContextDef(const char *expr)
{
	if (isblank(expr)) return NULL;

    ObjectDef *def=NULL;
	const char *area;
	int pos=0;
	int len=strlen(expr);

	for (int c=0; c<context.n(); c++) {
		area=context.key(c);
		if (!strncmp(expr,area,strlen(area))) {
			def=context.value(c)->GetObjectDef();
			pos+=strlen(area);
			break;
		}
	}


     // *** WARNING! this does not do index parsing, so "blah[123].blah" will not work,
     //     nor will blah.(34+2).blah
    int n;
    const char *showwhat=NULL;
    while (isspace(expr[pos])) pos++;
    while (def && expr[pos]=='.') { //for "Math.sin", for instance
        n=0;
        while (pos+n<len && (isalnum(expr[pos+n]) || expr[pos+n]=='_')) n++;
        showwhat=expr+pos;

        ObjectDef *ssd=def->FindDef(showwhat,n);
        if (!ssd) break;
        pos+=n;
        def=ssd;
    } //if foreach dereference
	return def;
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

		if (type==GSHELL_TextChanged) {
			showerror=0;
			showcompletion=1;
			tablevel=-1;
			UpdateSearchTerm(s->str,s->info3);
			if (!isblank(s->str)) {
				 //text was changed
			} else {
				completion.ClearSearch();
			}
			UpdateCompletion();
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_FocusIn) {
			Setup();
			showcompletion=1;
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_FocusOut) {
			showcompletion=0;
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_Enter) {
			 //enter pressed
			//execute command
			char *command=le->GetText();
			if (currenthistory>=0) history.remove(history.n);
			currenthistory=-1;
			tablevel=-1;

			if (isblank(command)) return 0;
			int answertype;
			char *result=calculator.In(command,&answertype);
			if (!isblank(result)) PostMessage(result);

			 //update history
			if (answertype==1 || answertype==2) {
				 //successful command
				le->SetText("");
				 //add, but not if is same as top of history stack!
				if (!history.n || (history.n && strcmp(history.e[history.n-1],command))) history.push(command);
			} else showerror=1;
			if (answertype==1 && !isblank(result)) {
				 //add, but not if is same as top of history stack! for instance a simple number input, don't add twice
				if (!history.n || (history.n && strcmp(history.e[history.n-1],result))) history.push(result);
			}
			
			UpdateSearchTerm(NULL,0, 1);
			UpdateCompletion();
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_Esc) {
			if (currenthistory!=-1 || tablevel!=-1) {
				 //escape from history browsing
				EscapeBrowsing();
				needtodraw=1;
				return 0;
			}

			if (!isblank(le->GetCText())) {
				 //escape when there is text just clears text
				le->SetText("");
				completion.ClearSearch();
				UpdateCompletion();
				needtodraw=1;
				return 0;
			}

			 //else escape from whole thing!
			le->SetText("");
			dynamic_cast<ViewerWindow*>(viewport->win_parent)->SelectTool(id);
			return 0;

		} else if (type==GSHELL_HistoryUp) {
			if (history.n==0) return 0;
			if (currenthistory==-1) {
				currenthistory=history.n-1;
				history.push(le->GetText()); //GetText returns a new'd char[]
				if (currenthistory>=0) {
					le->SetText(history.e[currenthistory]);
					le->SetCurpos(-1);
				}

			} else if (currenthistory!=0) {
				currenthistory--;
				le->SetText(history.e[currenthistory]);
				le->SetCurpos(-1);
			}
			return 0;

		} else if (type==GSHELL_HistoryDown) {
			if (history.n==0) return 0;
			if (currenthistory>=0) {
				currenthistory++;
				le->SetText(history.e[currenthistory]);
				le->SetCurpos(-1);
				if (currenthistory==history.n-1) {
					 //is on working string
					le->SetText(history.e[history.n-1]);
					le->SetCurpos(-1);
					history.remove(history.n-1);
					currenthistory=-1;
				}
			}
			return 0;

		} else if (type==GSHELL_Tab) {
			if (numresults==0) return 0;
			if (tablevel==-1) history.push(le->GetText());
			tablevel++;
			if (tablevel==numresults) tablevel=-1;
			if (tablevel==-1) {
				le->SetText(history.e[history.n-1]);
				le->SetCurpos(-1);
				history.remove(history.n-1);
				return 0;
			}
			int i=tablevel+1;
			UpdateFromTab(&completion,i);
			return 0;

		} else if (type==GSHELL_ShiftTab) {
			if (numresults==0) return 0;
			if (tablevel==-1) history.push(le->GetText());
			tablevel--;
			if (tablevel<-1) tablevel=numresults-1;
			if (tablevel==-1) {
				le->SetText(history.e[history.n-1]);
				le->SetCurpos(-1);
				history.remove(history.n-1);
				return 0;
			}
			int i=tablevel+1;
			UpdateFromTab(&completion,i);
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

//! Revert any browsing mode to normal edit mode.
void GraphicalShell::EscapeBrowsing()
{
	if (currenthistory!=-1) {
		 //escape from history browsing
		le->SetText(history.e[history.n-1]);
		le->SetCurpos(-1);
		history.remove(history.n-1);
		currenthistory=-1;
		needtodraw=1;
		return;

	} else if (tablevel!=-1) {
		 //escape from tab browsing
		le->SetText(history.e[history.n-1]);
		le->SetCurpos(-1);
		history.remove(history.n-1);
		tablevel=-1;
		needtodraw=1;
		return;
	}
}

//! Add menuitems from tree to completion.
void GraphicalShell::AddTreeToCompletion(MenuInfo *menu)
{
	MenuItem *mi, *mii;

	for (int c=0; c<menu->n(); c++) {
		mi=menu->e(c);

		mii=mi;
		char *str=NULL;
		TextFromItem(mii,str);
		completion.AddItem(str);
		delete[] str;

		if (!mi->GetSubmenu(0)) continue;
		AddTreeToCompletion(mi->GetSubmenu(0));
	}
}

//! Set edit text to a search result.
void GraphicalShell::UpdateFromTab(MenuInfo *menu, int &i)
{
	if (tablevel==-1) {
		//***install non-result string
		return;
	}

	MenuItem *mi, *mii;

	for (int c=0; c<menu->n(); c++) {
		mi=menu->e(c);
		if (mi->state&MENU_SEARCH_HIT) {
			mii=mi;
			i--;
			if (i<0) return; //already found what we were looking for!
			if (i==0) {
				char *str=NULL;
				TextFromItem(mii,str);
				le->SetText(str);
				le->SetCurpos(-1);
				delete[] str;
				i=-1;
				return;
			}
		}
		if (!(mi->state&(MENU_SEARCH_PARENT|MENU_SEARCH_HIT))) continue;

		if (!mi->GetSubmenu(0)) continue;
		UpdateFromTab(mi->GetSubmenu(0), i);
	}
}

/*! Recursively build the search result name.
 */
void GraphicalShell::TextFromItem(MenuItem *mii,char *&str)
{
	if (!mii) return;
	MenuInfo *mif=mii->parent;
	if (mif && mif->parent) {
		TextFromItem(mif->parent, str);
		appendstr(str,".");
	}
	appendstr(str,mii->name);
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
	if (showerror) dp->NewFG(.7,0.,0.); else dp->NewFG(&boxcolor);
	dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx, box.maxy-box.miny, 1);


	if (showcompletion) {
		int y=box.miny-pad;
		y=box.miny-pad;

		if (!completion.n() && searcharea==NULL) RefreshTree(&tree,box.minx,y);
		else RefreshTree(&completion, box.minx,y);
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


	return 1;
	//return 0;
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

	return 1;
	//return 0;
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

