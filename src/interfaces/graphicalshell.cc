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
#define GSHELL_Up           12
#define GSHELL_Down         13
#define GSHELL_Tab          14
#define GSHELL_ShiftTab     15
#define GSHELL_Esc          16

	if (ch==LAX_Esc) {
		 //turn off gshell
		send(GSHELL_Esc);
		return 0;

	} else if (ch==LAX_Up) {
		send(GSHELL_Up);
		//send(GSHELL_HistoryUp);
		return 0;

	} else if (ch==LAX_Down) {
		//send(GSHELL_HistoryDown);
		send(GSHELL_Down);
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
	: anInterface(nid,ndp)
{
	base_init();
}

GraphicalShell::GraphicalShell(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp)
{
	base_init();
}

void GraphicalShell::base_init()
{
	interface_type=INTERFACE_Overlay;
	placement_gravity=LAX_BOTTOM;

	showcompletion=0;
	showdecs=0;
	doc=NULL;
	sc=NULL;
	le=NULL;
	pad=10;
	boxcolor.rgbf(.5,.5,.5);

	showerror=0;
	error_message=NULL;

	searchterm=NULL;
	searchexpression=NULL;
	searcharea=NULL;
	searcharea_str=NULL;

	num_lines_above=-1; //number of lines of matches to show, -1 means fill to top
	num_lines_input=-1; //default -1, to use just 1, but autoexpand when necessary
	current_column=0; //-1 means inside edit box. >0 means default to that column on key up
	current_item=-1; //-1 means inside edit box
}

GraphicalShell::~GraphicalShell()
{
	DBG cerr <<"GraphicalShell destructor.."<<endl;

	if (doc) doc->dec_count();
	if (sc) sc->dec_count();

	delete[] error_message;
	delete[] searchterm;
	delete[] searchexpression;
	delete[] searcharea_str;
	if (searcharea) searcharea->dec_count();

	// *** NEED to figure out ownership protocol here!!! -> if (le) app->destroywindow(le);
	//if (le) app->destroywindow(le);
}

const char *GraphicalShell::Name()
{ return _("Shell"); }

void GraphicalShell::ClearError()
{
	showerror=0;
	delete[] error_message;
	error_message=NULL;
}

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

/*! Install or replace name with value in context.
 */
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

//! Update context from viewport, tree.Flush(), and add items from context, then UpdateSearchTerm().
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

void GraphicalShell::UpdateMatches()
{
	for (int c=0; c<3; c++) {
		columns[c].items.Search(searchterm,1,1);
		columns[c].width=-1;
	}
}

void GraphicalShell::ClearSearch()
{
	for (int c=0; c<3; c++) {
		columns[c].num_matches=columns[c].items.n();
		columns[c].items.ClearSearch();
		columns[c].width=-1;
	}
}




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

	} else if (placement_gravity==LAX_TOP) {
		double width=dp->Maxx-dp->Minx-2*pad;
		double height=dp->textheight()*1.5;
		le->MoveResize(dp->Minx+pad,dp->Miny+pad, width,height);
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

		columns[0].items.Flush();

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

					columns[0].items.AddItem(s);
					delete[] s;
				}
			}

		} else { //no distinct search area, so use all of tree
			AddTreeToCompletion(&tree);
			//DBG menuinfoDump(&columns[0].items,0);
		}
	}
}

/*! Search for the ObjectDef of expr within context. This is 
 * to search in immediately relevant areas first, before a broader
 * search in calculator.
 */
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
			ClearError();
			showcompletion=1;
			UpdateSearchTerm(s->str,s->info3);
			if (!isblank(s->str)) {
				 //text was changed
			} else {
				ClearSearch();
			}
			UpdateMatches();
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
			if (isblank(command)) {
				PostMessage(_("You are surrounded by twisty passages, all alike."));
				return 0;
			}
			//if (currenthistory>=0) columns[1].items.menuitems.remove(columns[1].items.n());
			current_item=-1;

			if (isblank(command)) return 0;
			char *result=NULL;
			Value *answer=NULL;
			ErrorLog log;

			int status=calculator.evaluate(command,-1, &answer,&log);
			if (log.Total()) result=log.FullMessageStr();
			if (!isblank(result)) PostMessage(result);

			 //update history
			if (status==0) {
				 //successful command
				le->SetText("");
				 //add, but not if is same as top of history stack!
				if (!columns[1].items.n() || (columns[1].items.n() && strcmp(columns[1].items.e(columns[1].items.n()-1)->name,command)))
					columns[1].items.AddItem(command);
			} else {
				 //there was an error
				showerror=1;
				makestr(error_message, result);
			}

			if (answer) {
				 //add value to third column
				 //add, but not if is same as top of history stack! for instance a simple number input, don't add twice
				if (answer!=past_values.e(past_values.n()-1)) {
					int needed=answer->getValueStr(NULL,0);
					past_values.push("ans",answer);

					if (needed>30) {
						ObjectDef *def=answer->GetObjectDef();
						makestr(result,def->name);
						columns[2].items.AddItem(result);
						
					} else {
						char *str=NULL;
						int len=0;
						answer->getValueStr(&str,&len,1);
						if (str) {
							columns[2].items.AddItem(str);
						}
						delete[] str;
					}
				}
				answer->dec_count();
			}
			
			UpdateSearchTerm(NULL,0, 1);
			UpdateMatches();
			delete[] result;
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_Esc) {
			ClearError();

			if (current_column>=0 && current_item>=0) {
				 //escape from history browsing
				EscapeBrowsing();
				needtodraw=1;
				return 0;
			}

			if (!isblank(le->GetCText())) {
				 //escape when there is text just clears text
				le->SetText("");
				ClearSearch();
				UpdateMatches();
				needtodraw=1;
				return 0;
			}

			 //else escape from whole thing!
			le->SetText("");
			dynamic_cast<ViewerWindow*>(viewport->win_parent)->SelectTool(id);
			return 0;


		} else if (type==GSHELL_HistoryUp || type==GSHELL_Up) {
			app->setfocus(viewport,0,NULL);
			if (placement_gravity==LAX_BOTTOM) {
				current_item++;
				if (current_item>=columns[current_column].num_matches)
					current_item=0;
			} else {
				current_item--;
				if (current_item<0) current_item=columns[current_column].num_matches-1;
			}
			needtodraw=1;
			return 0;
//
//			-------
//			if (history.n()==0) return 0;
//			if (current_item==-1) {
//				current_item=history.n()-1;
//				*** history.push(le->GetText()); //GetText returns a new'd char[]
//				if (currenthistory>=0) {
//					le->SetText(history.e[currenthistory]);
//					le->SetCurpos(-1);
//				}
//
//			} else if (current_item!=0) {
//				currenthistory--;
//				le->SetText(history.e[currenthistory]);
//				le->SetCurpos(-1);
//			}
//			return 0;

		} else if (type==GSHELL_HistoryDown || type==GSHELL_Down) {
			app->setfocus(viewport,0,NULL);
			if (placement_gravity==LAX_BOTTOM) {
				current_item--;
				if (current_item<0) current_item=columns[current_column].num_matches-1;
			} else {
				current_item++;
				if (current_item>=columns[current_column].num_matches)
					current_item=0;
			}
			needtodraw=1;
			return 0;

//			----------
//			if (history.n==0) return 0;
//			if (currenthistory>=0) {
//				currenthistory++;
//				le->SetText(history.e[currenthistory]);
//				le->SetCurpos(-1);
//				if (currenthistory==history.n-1) {
//					 //is on working string
//					le->SetText(history.e[history.n-1]);
//					le->SetCurpos(-1);
//					history.remove(history.n-1);
//					currenthistory=-1;
//				}
//			}
//			return 0;

		} else if (type==GSHELL_Tab) {
			 //change column
			current_column++;
			if (current_column>2) current_column=0;
			UpdateFromItem();
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_ShiftTab) {
			 //change column
			current_column--;
			if (current_column<0) current_column=2;
			UpdateFromItem();
			needtodraw=1;
			return 0;
		}

		return 0;
	}

	return 1;
}

//! Revert any browsing mode to normal edit mode.
void GraphicalShell::EscapeBrowsing()
{
	//app->setfocus(le,0,d);
	//return le->CharInput(ch,buffer,len,state,d);
	current_item=-1;
	needtodraw=1;
	return;
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
		columns[0].items.AddItem(str);
		delete[] str;

		if (!mi->GetSubmenu(0)) continue;
		AddTreeToCompletion(mi->GetSubmenu(0));
	}
}

//! Set edit text to a search result.
void GraphicalShell::UpdateFromItem()
{
	// *** need to update what is shown in le, which sometimes means a partial replacement
	// with context matches, value inserts, or past expressions

//	i=current_item;
//	if (i<0) {
//		if (searchexpression) {
//			le->SetText(searchexpression);
//			delete[] searchexpression; searchexpression=NULL;
//		}
//	} else {
//		 //probably we've moved to a new item
//		if (!isblank(le->GetCText()) && !searchexpression) {
//			makestr(searchexpression,le->GetCText());
//		}
//		char *str=
//				
//	}
//	----------
//	MenuItem *mi, *mii;
//
//	for (int c=0; c<menu->n(); c++) {
//		mi=menu->e(c);
//		if (mi->state&MENU_SEARCH_HIT) {
//			mii=mi;
//			i--;
//			if (i<0) return; //already found what we were looking for!
//			if (i==0) {
//				char *str=NULL;
//				TextFromItem(mii,str);
//				le->SetText(str);
//				le->SetCurpos(-1);
//				delete[] str;
//				i=-1;
//				return;
//			}
//		}
//		if (!(mi->state&(MENU_SEARCH_PARENT|MENU_SEARCH_HIT))) continue;
//
//		if (!mi->GetSubmenu(0)) continue;
//		UpdateFromTab(mi->GetSubmenu(0), i);
//	}
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
	ClearError();
}


int GraphicalShell::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	dp->DrawScreen();

	
	if (showcompletion) {
		bool needtomap=true;
		for (int c=0; c<3; c++) {
			if (columns[c].width<0) {
				needtomap=true;
				if (columns[c].num_matches==0) {
					columns[c].width=50;
					continue;
				}

				double maxw=0, w;
				for (int c2=0; c2<columns[c].items.n(); c2++) {
					w=dp->textextent(columns[c].items.e(c2)->name,-1, NULL,NULL);
					if (w>maxw) maxw=w;
				}
				columns[c].width=maxw;
			}
		}

		if (needtomap) {
			int maxcols=3;
			double r=((double)current_column)/(maxcols-1);

			columns[current_column].x=box.minx + (box.maxx-box.minx)*r - r*columns[current_column].width;

			for (int c=current_column-1; c>=0; c--) {
				r=((double)c)/(maxcols-1);
				columns[c].x=box.minx + (box.maxx-box.minx)*r - r*columns[c].width;
				if (columns[c].x+columns[c].width > columns[c+1].x)
					columns[c].x= columns[c+1].x - columns[c].width;
			}

			for (int c=current_column+1; c<maxcols; c++) {
				r=((double)c)/(maxcols-1);
				columns[c].x=box.minx + (box.maxx-box.minx)*r - r*columns[c].width;
				if (columns[c].x<columns[c-1].x+columns[c-1].width) 
					columns[c].x=columns[c-1].x+columns[c-1].width; 
			}
		}
	}



	if (!showcompletion) {
		 //draw thick border all around the edit
		if (showerror) dp->NewFG(.7,0.,0.); else dp->NewFG(&boxcolor);
		dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx, box.maxy-box.miny, 1);

	} else {
		if (showerror) dp->NewFG(.7,0.,0.); else dp->NewFG(&boxcolor);

		DoubleBBox bbox=box;
		double trim=(placement_gravity==LAX_BOTTOM?pad:0);

		 //draw thick border, but not a thick border near the current column
		if (current_column==0 && columns[0].width>0) {
			dp->drawrectangle(box.minx,box.miny+trim,  columns[0].width, box.maxy-box.miny-pad, 1);
			dp->drawrectangle(box.minx+columns[0].width,box.miny+trim, box.maxx-box.minx-columns[0].width, box.maxy-box.miny-trim, 1);

		} else if (current_column==1 && columns[1].width>0) {
			double x=(box.minx+box.maxx)/2-columns[1].width/2;
			dp->drawrectangle(box.minx,box.miny, x-box.minx, box.maxy-box.miny, 1);
			dp->drawrectangle(x,box.miny+trim, columns[1].width, box.maxy-box.miny-pad, 1);
			dp->drawrectangle(x+columns[1].width,box.miny, box.maxx-(x+columns[1].width), box.maxy-box.miny, 1);

		} else if (current_column==2 && columns[2].width>0) {
			dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx-columns[2].width, box.maxy-box.miny, 1);
			dp->drawrectangle(box.maxx-columns[2].width,box.miny+trim,  box.maxx-box.minx, box.maxy-box.miny-pad, 1);
		
		} else {
			 //No active column, draw thick border all around the edit
			dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx, box.maxy-box.miny, 1);
		}
	}


	if (showcompletion) {
		dp->NewFG(.5,.5,.5);

		for (int c=0; c<3; c++) {
			if (columns[c].num_matches<=0) continue;

			int y=box.miny-pad;
			RefreshTree(&columns[c].items, columns[c].x,y);
		}
	}

	if (showerror && error_message) {
		 //draw an error box with thick border, right at edge of edit
		int numlines=0;
		char *nl=error_message, *ls=error_message;
		double max_width=0, w;
		double textheight=dp->textheight();

		do {
			ls=nl;
			nl=strchrnul(nl,'\n');
			if (*nl) nl++;
			numlines++;
			w=dp->textextent(ls,nl-ls, NULL,NULL);
			if (w>max_width) max_width=w;
		} while (*nl);

		double x=(box.maxx+box.minx)/2;
		double y=box.miny+pad-textheight*numlines;
		if (placement_gravity==LAX_TOP) y=box.maxy-pad+1;

		dp->NewFG(.7,0.,0.);
		dp->drawrectangle(x-max_width/2-pad,y-pad, max_width+2*pad,numlines*textheight+2*pad, 1);
		dp->NewFG(.9,.9,.9);
		dp->drawrectangle(x-max_width/2,y, max_width,numlines*textheight-1, 1);

		nl=error_message;
		dp->NewFG(.1,.1,.1);
		dp->textout((box.maxx+box.minx)/2, y, error_message,-1, LAX_HCENTER|LAX_TOP);
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


enum GraphicalShellControlTypes {
	GSHELL_None         =0,
	GSHELL_Column_Up    =-2,
	GSHELL_Column_Down  =-3,
//	GSHELL_Column_1_up  =-2,
//	GSHELL_Column_1_down=-3,
//	GSHELL_Column_2_up  =-4,
//	GSHELL_Column_2_down=-5,
//	GSHELL_Column_3_up  =-6,
//	GSHELL_Column_3_down=-7,
	GSHELL_Item         =-8
};

/*! Return the navigation control (x,y) is over, and which column and item (or -1 for none of those).
 * Returns 0 if not over anything.
 */
int GraphicalShell::scan(int x,int y, int *column, int *item)
{

	if (!showcompletion) return GSHELL_None;

	double textheight=dp->textheight();

	if (column) *column=-1;
	if (item) *item=-1;

	int i=-1;
	if (placement_gravity==LAX_BOTTOM) i=(box.miny-y)/textheight;
	else i=(y-box.maxy)/textheight;

	for (int c=0; c<3; c++) {
		if (columns[c].num_matches==0) continue;

		if (x>=columns[c].x && x<=columns[c].x+columns[c].width) {
			if (column) *column=c;
			if (i==0 && columns[c].offset!=0)
				return (placement_gravity==LAX_BOTTOM ? GSHELL_Column_Down : GSHELL_Column_Up);
			if (i==num_lines_above && i+columns[c].offset!=0)
				return (placement_gravity==LAX_BOTTOM ? GSHELL_Column_Up   : GSHELL_Column_Down);

			i+=columns[c].offset;
			if (i<0 || i>=columns[c].num_matches) { if (item) *item=-1; return GSHELL_None; }
			
			if (item) *item=i;
			return GSHELL_Item;
		}
	}
	return 0;
}

/*! Add maybe box if shift is down.
 */
int GraphicalShell::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;

	if (showerror) {
		ClearError();
		needtodraw=1;
		return 0;
	}


	DBG flatpoint fp;
	DBG fp=dp->screentoreal(x,y);

	int column=-1, item=-1;
	int over=scan(x,y, &column,&item);
	if (over==GSHELL_None) return 1;

	buttondown.down(d->id,LEFTBUTTON,x,y,column,item);


	return 0;
}

int GraphicalShell::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int oldcolumn, olditem;
	buttondown.up(d->id,LEFTBUTTON, &oldcolumn, &olditem);

	int column=-1, item=-1;
	int over=scan(x,y, &column,&item);
	if (over==GSHELL_None) return 0;
	if (column!=oldcolumn || item!=olditem || item<0) return 0;

	if (over==GSHELL_Column_Up) {
		columns[current_column].offset++;
		if (columns[current_column].offset+num_lines_above>=columns[current_column].num_matches)
			columns[current_column].offset=columns[current_column].num_matches-num_lines_above;

	} else if (over==GSHELL_Column_Down) {
		columns[current_column].offset--;
		if (columns[current_column].offset<0) columns[current_column].offset=0;


	} else if (over==GSHELL_Item && column>0 && item>=0) {
		const char *str=GetItemText(column,item);
		if (isblank(str)) return 0;

		if (searchexpression==NULL) searchexpression=newstr(le->GetCText());
		le->SetText(str);

	}


	needtodraw=1;
	return 0;
}

const char *GraphicalShell::GetItemText(int column,int item)
{
	if (column<0 || column>2) return NULL;
	if (item<0 || item>=columns[column].num_matches) return NULL;

	 //count item down, skipping hidden items in items stack
	for (int c=0; item>=0 && c<columns[column].items.n(); c++) {
		if (columns[column].items.e(c)->hidden()) continue;
		if (item==0) return columns[column].items.e(c)->name;
		item--;
	}

	return NULL;
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

	int column=-1,item=-1;
	int over=scan(x,y, &column,&item);

	DBG cerr <<"over box: "<<over<<"  column:"<<column<<"  item:"<<item<<endl;

	if (!buttondown.any()) {
		if (!(state&ShiftMask)) return 1;
		needtodraw=1;
		return 0;
	}

	int mx,my;
	buttondown.move(mouse->id,x,y, &mx,&my);


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
	GSHELL_ToggleGravity,
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

	sc->Add(GSHELL_Activate,    '/',0,0,         "Activate", _("Turn on the prompt, if it was off."),NULL,0);
	//sc->Add(GSHELL_Decorations, 'd',0,0,         "Decs",     _("Toggle decorations"),NULL,0);
	sc->Add(GSHELL_ToggleGravity, LAX_Up,ShiftMask|ControlMask,0,   "ToggleGravity",    _("Toggle placement of input bar, top or bottom"),NULL,0);
	sc->AddShortcut(LAX_Down,ShiftMask|ControlMask,0, GSHELL_ToggleGravity);


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

	} else if (action==GSHELL_ToggleGravity) {
		if (placement_gravity==LAX_TOP) placement_gravity=LAX_BOTTOM;
		else placement_gravity=LAX_TOP;
		ViewportResized();
		needtodraw=1;
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
		if (PerformAction(action)==0) return 0;
	}
	if (le) {
		app->setfocus(le,0,d);
		return le->CharInput(ch,buffer,len,state,d);
	}

	return 1;
}

int GraphicalShell::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
}


} // namespace Laidout

