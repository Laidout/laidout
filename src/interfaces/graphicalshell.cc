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
// Copyright (C) 2013 by Tom Lechner
//


#include "../language.h"
#include "graphicalshell.h"
#include "../ui/viewwindow.h"
#include "../core/drawdata.h"
#include "../dataobjects/printermarks.h"

#include <lax/inputdialog.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/colors.h>

//template implementation:
#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


enum GShellScanMenuItems {
	GSHELL_TextChanged,
	GSHELL_Enter,
	GSHELL_FocusIn,
	GSHELL_FocusOut,
	GSHELL_HistoryUp,
	GSHELL_HistoryDown,

	GSHELL_Up,
	GSHELL_Down,
	GSHELL_Tab,
	GSHELL_ShiftTab,
	GSHELL_Esc,
	GSHELL_MMAX
};

enum GraphicalShellActions {
	GSHELL_Activate,
	GSHELL_ToggleMinimized,
	GSHELL_Decorations,
	GSHELL_ToggleGravity,
	GSHELL_MAX
};

//these get put in buttondown info2 when not actual item
enum GraphicalShellControlTypes {
	GSHELL_None         =0,
	GSHELL_Column_Up    =-2,
	GSHELL_Column_Down  =-3,
	GSHELL_Item         =-4,
	GSHELL_Num_Lines    =-5
};


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
	error_message_type=1;

	searchterm=NULL;
	searchexpression=NULL;
	searcharea=NULL;
	searcharea_str=NULL;

	needtomap=true;
	num_lines_above=-1; //number of lines of matches to show, -1 means fill to top
	num_lines_input=-1; //default -1, to use just 1, but autoexpand when necessary
	current_column=0; //-1 means inside edit box. >0 means default to that column on key up
	current_item=-1; //-1 means inside edit box
	hover_item=-1;
	hover_column=-1;
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
{ return 0; }


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

void GraphicalShell::ClearError()
{
	showerror=0;
	delete[] error_message;
	error_message=NULL;
}

/*! Initialize the lineedit, map it, and InitAreas().
 */
int GraphicalShell::Setup()
{
	if (!le) {
		LaxFont *font = app->defaultlaxfont->duplicate();
		double width=dp->Maxx-dp->Minx-2*pad;
		double height=font->textheight()*1.5;
		le=new GraphicalShellEdit(viewport, this, dp->Minx+pad,dp->Maxy-height-pad,width,height);
		le->UseThisFont(font);
		font->dec_count();
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

	int hh=(dp->Maxy-dp->Miny-(box.maxy-box.miny))/le->GetFont()->textheight()+1;
	if (num_lines_above<0 || num_lines_above>=hh) num_lines_above=hh-1;

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

	
	 //tree has immediate names of things in context. These get squashed to
	 //normal strings later
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

	columns[0].items.Flush();
	AddTreeToCompletion(&tree);


	return 0;
}


Laxkit::MenuInfo *GraphicalShell::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	return menu;
}

/*! Reposition the lineedit when parent viewport size changed.
 */
void GraphicalShell::ViewportResized()
{
	if (placement_gravity==LAX_BOTTOM) {
		double width=dp->Maxx-dp->Minx-2*pad;
		double height=le->GetFont()->textheight()*1.5;
		le->MoveResize(dp->Minx+pad,dp->Maxy-height-pad, width,height);
		box.minx=le->win_x-pad;
		box.maxx=le->win_x+le->win_w+pad;
		box.miny=le->win_y-pad;
		box.maxy=le->win_y+le->win_h+pad;

	} else if (placement_gravity==LAX_TOP) {
		double width=dp->Maxx-dp->Minx-2*pad;
		double height=le->GetFont()->textheight()*1.5;
		le->MoveResize(dp->Minx+pad,dp->Miny+pad, width,height);
		box.minx=le->win_x-pad;
		box.maxx=le->win_x+le->win_w+pad;
		box.miny=le->win_y-pad;
		box.maxy=le->win_y+le->win_h+pad;
	}
}

/*! Make sure tool, object are current in context.
 *
 * Return 1 if context has been changed, else 0.
 */
int GraphicalShell::UpdateContext()
{
	int mod=0;
	
	DBG anInterface *curtool=dynamic_cast<ViewerWindow*>(viewport->win_parent)->CurrentTool();
	DBG anInterface *contexttool= dynamic_cast<anInterface*>(context.find("tool"));
	DBG DrawableObject *curobject=dynamic_cast<DrawableObject*>(dynamic_cast<LaidoutViewport*>(viewport)->curobj.obj);
	DBG DrawableObject *contextobject=dynamic_cast<DrawableObject*>(context.find("object"));

	DBG cerr <<" UpdateContext():"<<endl;
	DBG cerr <<"    "<< (curtool==contexttool ? "tool same" : "tool different") <<", "<<(curtool?curtool->whattype():"null")<<endl;
	DBG cerr <<"    "<< (curobject==contextobject ? "object same" : "object different") <<", "<<(curobject?curobject->whattype():"null")<<endl;

	DBG if (curobject!=contextobject) {
	DBG 	cerr <<"booyah"<<endl;
	DBG }

	if (dynamic_cast<ViewerWindow*>(viewport->win_parent)->CurrentTool()
			 != dynamic_cast<anInterface*>(context.find("tool"))) {
		context.pushObject("tool",dynamic_cast<ViewerWindow*>(viewport->win_parent)->CurrentTool());
		mod=1;
	}

	if (dynamic_cast<LaidoutViewport*>(viewport)->curobj.obj
			!= dynamic_cast<DrawableObject*>(context.find("object"))) {
		context.pushObject("object",dynamic_cast<LaidoutViewport*>(viewport)->curobj.obj);
		mod=1;
	}

	if (mod) {
		calculator.InstallVariables(&context);

		 //tree has immediate names of things in context. These get squashed to
		 //normal strings later
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

		columns[0].items.Flush();
		AddTreeToCompletion(&tree);
	}

	return mod;
}

///*! Install or replace name with value in context.
// */
//int GraphicalShell::ChangeContext(const char *name, Value *value)
//{
//	if (!context.find(name)) {
//		 //unknown context name
//		context.push(name,value);
//	} else context.set(name,value);
//
//	//if (!strcmp(name,"object")) {
//	//if (!strcmp(name,"selection")) {}
//	//if (!strcmp(name,"viewport")) {}
//	//if (!strcmp(name,"tool")) {}
//
//	return 0;
//}


/*! Update search hit tags in the columns.
 * Updates original selection of strings if it seems like the context has changed (see UpdateContext()).
 */
void GraphicalShell::UpdateMatches()
{
	UpdateContext();

	for (int c=0; c<3; c++) {
		columns[c].num_matches=columns[c].items.Search(searchterm,1,1);
		//columns[c].width=-1;
	}
	needtomap=true;
}

void GraphicalShell::ClearSearch()
{
	for (int c=0; c<3; c++) {
		columns[c].num_matches=columns[c].items.n();
		columns[c].items.ClearSearch();
		columns[c].width=-1;
	}
	needtomap=true;
}




/*! Grab the portion of string that seems self contained. For instance, a string
 * of "1+Math.pi" would set searchterm to "Math.pi".
 *
 * This does not update the results of a search, only what and where to search.
 */
void GraphicalShell::UpdateSearchTerm(const char *str,int pos, int firsttime)
{
	makestr(searchterm,str);


//---faulty stuff below:
//	//viewport.blah|  
//	//viewport.spread.1.|
//	//object.scale((1,0), | ) <- get def of function, keep track of which parameters used?
//	//object.1.(3+23).|
//	//
//	int len=(str?strlen(str):0);
//	if (pos<=0) pos=len-1;
//	int last=pos, pose=pos;
//	const char *first=str;
//	while (pos>0) {
//		while (pos>0 && isspace(str[pos])) pos--;
//		while (pos>0 && isalnum(str[pos])) pos--;
//		if (!first) { first=str+pos; pose=pos; }
//		if (pos>0 && str[pos]=='.') pos--;
//		else break; //do not parse backward beyond a whole term
//	}
//
//	if (first && *first=='.') first++;
//	makenstr(searchterm,first,last-pos+1);
//	if (!searchterm) searchterm=newstr("");
//
//	char *areastr=newnstr(str+pos,pose-pos);
//	ObjectDef *area=GetContextDef(areastr);
//	if (!area) area=calculator.GetInfo(areastr);
//
//	if (area!=searcharea || firsttime) {
//		if (searcharea) searcharea->dec_count();
//		searcharea=area;
//		if (area) area->inc_count();
//		makestr(searcharea_str,areastr);
//		char *s;
//
//		columns[0].items.Flush();
//
//		if (searcharea) {
//			const char *name;
//			for (int c=0; c<searcharea->getNumFields(); c++) {
//				name=NULL;
//				searcharea->getInfo(c, &name);
//				if (name) {
//					if (searcharea_str) {
//						s=newstr(searcharea_str);
//						appendstr(s,".");
//					} else s=NULL;
//					appendstr(s,name);
//
//					columns[0].items.AddItem(s);
//					delete[] s;
//				}
//			}
//
//		} else { //no distinct search area, so use all of tree
//			AddTreeToCompletion(&tree);
//			//DBG menuinfoDump(&columns[0].items,0);
//		}
//	}
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
			UpdateSearchTerm(s->str,s->info3,0);
			if (!isblank(s->str)) {
				 //text was changed
			} else {
				ClearSearch();
			}
			UpdateMatches();
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_FocusIn) {
			hover_item=-1;
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_FocusOut) {
			hover_item=-1;
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_Enter) {
			 //enter pressed
			//execute command
			
			DBG cerr <<" -------shell enter------"<<endl;
			UpdateContext();

			char *command=le->GetText();
			if (isblank(command)) {
				PostMessage(_("You are surrounded by twisty passages, all alike."));
				return 0;
			}
			//if (currenthistory>=0) columns[1].items.menuitems.remove(columns[1].items.n());
			current_item=-1;

			char *result=NULL;
			Value *answer=NULL;
			ErrorLog log;

			int status = calculator.Evaluate(command,-1, &answer,&log);
			if (log.Total()) result=log.FullMessageStr();
			if (!isblank(result) && status==0) PostMessage(result);

			 //update history
			if (status==0 || status==-1) {
				 //successful command
				le->SetText("");
				 //add, but not if is same as top of history stack! (minimize dups)
				if (!columns[1].items.n() || (columns[1].items.n() && strcmp(columns[1].items.e(0)->name,command)))
					columns[1].items.AddItem(command,0,0,nullptr,0);
			}
			
			if (status==1 || status==-1) {
				 //there was an error
				showerror=1;
				makestr(error_message, result);
				error_message_type=(status==1?1:0);
			}

			if (answer) {
				 //add value to third column
				if (answer!=past_values.e(past_values.n()-1)) {
					int added=0;

					int needed=answer->getValueStr(NULL,0);
					if (needed>30) {
						ObjectDef *def=answer->GetObjectDef();
						makestr(result,def->name);
						if (!isblank(result)) {
							columns[2].items.AddItem(result,0,0,nullptr,0);
							added=1;
						}
						
					} else {
						char *str=NULL;
						int len=0;
						answer->getValueStr(&str,&len,1);
						if (!isblank(str)) {
							columns[2].items.AddItem(str,0,0,nullptr,0);
							added=1;
						}
						delete[] str;
					}

					if (added) past_values.push("ans",answer,0);

				}
				answer->dec_count();
			}
			
			ClearSearch();
			UpdateSearchTerm(NULL,0, 1);
			UpdateMatches();
			delete[] result;
			needtodraw=1;
			needtomap=true;
			DBG cerr <<" -------shell enter done------"<<endl;
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

		} else if (type==GSHELL_Tab) {
			 //change column
			current_column=NextColumn(current_column);
			UpdateFromItem();
			needtodraw=1;
			return 0;

		} else if (type==GSHELL_ShiftTab) {
			 //change column
			current_column=PreviousColumn(current_column);
			UpdateFromItem();
			needtodraw=1;
			return 0;
		}

		return 0;
	}

	return 1;
}

int GraphicalShell::PreviousColumn(int cc)
{
	int c=cc;
	do {
		c--;
		if (c<0) c=2;
		if (columns[c].width>0 && columns[c].num_matches>0) break;
	} while (c!=cc);

	return c;
}

int GraphicalShell::NextColumn(int cc)
{
	int c=cc;
	do {
		c++;
		if (c>2) c=0;
		if (columns[c].width>0 && columns[c].num_matches>0) break;
	} while (c!=cc);

	return c;
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

//! Add menuitems from tree to column 0.
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



int GraphicalShell::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	dp->DrawScreen();
	dp->font(le->GetFont());

	
	 //remap columns if necessary
	if (showcompletion && needtomap) {
		for (int c=0; c<3; c++) {
			//if (columns[c].width<=0) {
				if (columns[c].num_matches==0) {
					columns[c].width=0;
					continue;
				}

				double maxw=0, w;
				for (int c2=0; c2<columns[c].items.n(); c2++) {
					w=dp->textextent(columns[c].items.e(c2)->name,-1, NULL,NULL);
					if (w>maxw) maxw=w;
				}
				columns[c].width=maxw;
			//}
		}

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
		needtomap=false;
	}



	if (!showcompletion) {
		 //draw thick border all around the edit
		if (showerror) dp->NewFG(.7,0.,0.); else dp->NewFG(&boxcolor);
		dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx, box.maxy-box.miny, 1);

	} else {

		 //draw num lines above indicator
		double th=le->GetFont()->textheight();
		int max=(hover_item==GSHELL_Num_Lines ? 10 : 0);
		double ss=.4;
		for (int c=0; c<max; c++) {
			dp->NewFG(1-ss+ss/max*c, 1-ss+ss/max*c, 1-ss+ss/max*c);
			if (placement_gravity==LAX_BOTTOM)
				dp->drawline(dp->Minx,box.miny-th*num_lines_above-c, dp->Maxx,box.miny-th*num_lines_above-c);
			else
				dp->drawline(dp->Minx,box.maxy+th*num_lines_above+c, dp->Maxx,box.maxy+th*num_lines_above+c);
		}

		if (showerror) dp->NewFG(.7,0.,0.); else dp->NewFG(&boxcolor);

		 //draw thick border, but not a thick border near the current column
//		if (current_column>=0) {
//			double trim=(placement_gravity==LAX_BOTTOM?pad-1:0);
//
//			int x=columns[current_column].x;
//			int w=columns[current_column].width;
//
//			dp->drawrectangle(box.minx,box.miny,  x-box.minx, box.maxy-box.miny, 1);
//			dp->drawrectangle(x,box.miny+trim,  w, box.maxy-box.miny-(pad-1), 1);
//			dp->drawrectangle(x+w,box.miny, box.maxx-(x+w), box.maxy-box.miny, 1);
//		
//		} else {
			 //No active column, draw thick border all around the edit
			dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx, box.maxy-box.miny, 1);
//		}

		dp->NewFG(.5,.5,.5);

		for (int c=0; c<3; c++) {
			if (columns[c].num_matches<=0) continue;

			int y=box.miny;
			int i=0;
			RefreshTree(&columns[c].items, columns[c].x,y, c,i);
		}
	}

	if (showerror && error_message) {
		 //draw an error box with thick border, right at edge of edit
		int numlines=0;
		char *nl=error_message, *ls=error_message;
		double max_width=0, w;
		double textheight=le->GetFont()->textheight();

		do {
			ls=lax_strchrnul(nl, '\n');
			if (*nl) nl++;
			numlines++;
			w=dp->textextent(ls,nl-ls, NULL,NULL);
			if (w>max_width) max_width=w;
		} while (*nl);

		double x=(box.maxx+box.minx)/2;
		double y=box.miny+pad-textheight*numlines;
		if (placement_gravity==LAX_TOP) y=box.maxy-pad+1;

		 //red for error, yellow for warning
		if (error_message_type==1) dp->NewFG(.7,0.,0.); else dp->NewFG(0.,.7,.7);
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
void GraphicalShell::RefreshTree(MenuInfo *menu, int x,int &y, int col,int &item)
{
	MenuItem *mi, *mii;
	for (int c=0; c<menu->n(); c++) {
		if (item>=columns[col].offset+num_lines_above) return;

		mi=menu->e(c);
		if (mi->state&MENU_SEARCH_HIT) {
			if (item>=columns[col].offset) {
				int xx=x;
				mii=mi;

				if (col==hover_column && item==hover_item) dp->NewFG(.1,.1,.1);
				DrawName(mii,xx,y);
				if (col==hover_column && item==hover_item) dp->NewFG(.5,.5,.5);

				y-=le->GetFont()->textheight();
			}
			item++;
		}
		if (!(mi->state&(MENU_SEARCH_PARENT|MENU_SEARCH_HIT))) continue;

		if (!mi->GetSubmenu(0)) continue;
		RefreshTree(mi->GetSubmenu(0), x,y, col,item);
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

/*! Return the navigation control (x,y) is over, and which column and item (or -1 for none of those).
 * Returns 0 if not over anything.
 */
int GraphicalShell::scan(int x,int y, int *column, int *item)
{

	if (!showcompletion) return GSHELL_None;

	double textheight=le->GetFont()->textheight();

	if (column) *column=-1;
	if (item) *item=-1;

	int i=-1;
	if (placement_gravity==LAX_BOTTOM) i=(box.miny-y)/textheight;
	else i=(y-box.maxy)/textheight;

	if (i==num_lines_above) return GSHELL_Num_Lines;

	DBG cerr <<"gshell scan curcol,item: "<<current_column<<","<<current_item<<endl;
	for (int c=0; c<3; c++) {
		if (columns[c].num_matches==0) continue;

		DBG cerr <<"gshell column "<<c<<" nummatches:"<<columns[c].num_matches<<endl;
		DBG cerr <<"  x:"<<x<<"  colx,w:"<<columns[c].x<<','<<columns[c].width<<endl;

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

	buttondown.down(d->id,LEFTBUTTON,x,y, column,(over!=GSHELL_Item ? over : item));

	if (over==GSHELL_Item && column>=0 && item>=0) {
		 //we are clicking down on something to activate later
		current_column=column;
		current_item=item;
		needtodraw=1;
	}


	return 0;
}

int GraphicalShell::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int oldcolumn, olditem;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &oldcolumn, &olditem);

	int column=-1, item=-1;
	int over=scan(x,y, &column,&item);

	DBG cerr <<" --- old/new col: "<<oldcolumn<<','<<column<<"  old/new item: "<<olditem<<','<<item<<"  over="<<over<<"  dragged="<<dragged<<endl;

	if (over==GSHELL_None) return 0;
	if (column!=oldcolumn || item!=olditem || item<0) return 0;

	if (!dragged) {
		if (over==GSHELL_Column_Up) {
			columns[current_column].offset++;
			if (columns[current_column].offset+num_lines_above>=columns[current_column].num_matches)
				columns[current_column].offset=columns[current_column].num_matches-num_lines_above;

		} else if (over==GSHELL_Column_Down) {
			columns[current_column].offset--;
			if (columns[current_column].offset<0) columns[current_column].offset=0;


		} else if (over==GSHELL_Item && column>=0 && item>=0) {
			const char *str=GetItemText(column,item);
			if (isblank(str)) {
				DBG cerr <<" *** BLANK LBUP STR!!!!!"<<endl;
				return 0;
			}

			if (searchexpression==NULL) searchexpression=newstr(le->GetCText());
			le->SetText(str);

		}
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

	DBG cerr <<" ** WARNING! couldn't find string for col,item:"<<column<<','<<item<<endl;
	return NULL;
}

int GraphicalShell::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{

	//special move outs for different kinds when inputing parameters of known types.
	//  -> flatpoints, drag out a circle hovering above input area.. cursor keys to move it around,
	//  		modifiers to move at different speeds. Updates number in command box
	//  -> numbers, control to drag out values
	//  -> colors, control to pop up a color selector


	int column=-1,item=-1;

	if (!buttondown.any()) {
		int over=scan(x,y, &column,&item);

		if (hover_column!=column || hover_item!=item) needtodraw=1;
		hover_column=column;
		hover_item=item;
		if (over!=GSHELL_Item) hover_item=over;

		DBG cerr <<"over box: "<<over<<"  column:"<<column<<"  item:"<<item<<endl;

		if (!(state&ShiftMask)) return 1;
		return 0;
	}

	int mx,my;
	buttondown.move(mouse->id,x,y, &mx,&my);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON,&column,&item);
	
	if (item==GSHELL_Num_Lines) {
		int nl;
		if (placement_gravity==LAX_TOP) nl=(y-box.maxy)/le->GetFont()->textheight()+1;
		else nl=(box.miny-y)/le->GetFont()->textheight()+1;

		DBG cerr << " **********new nl:"<<nl<<"  old: "<<num_lines_above<<endl;

		if (nl>0 && nl!=num_lines_above) {
			num_lines_above=nl;
			needtodraw=1;
		}
		return 0;
	}



	return 0;
}

int GraphicalShell::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int column=-1,item=-1;
	//int over=
	scan(x,y, &column,&item);

	DBG cerr <<" ***********wheel up ***********"<<endl;
	if (column>=0 && item>=0) {
		columns[column].offset++;
		if (columns[column].offset+num_lines_above > columns[column].num_matches)
			columns[column].offset = columns[column].num_matches-num_lines_above;
		if (columns[column].offset<0) columns[column].offset=0;
		needtodraw=1;
		return 0;
	}

	return 1;
}

int GraphicalShell::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int column=-1,item=-1;
	//int over=
	scan(x,y, &column,&item);

	DBG cerr <<" ***********wheel down ***********"<<endl;
	if (column>=0 && item>=0) {
		columns[column].offset--;
		if (columns[column].offset<0) columns[column].offset=0;
		needtodraw=1;
		return 0;
	}

	return 1;
}


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

void GraphicalShell::MakeHoverInWindow()
{
	if (hover_item<0) return;

	if (hover_item<columns[hover_column].offset) columns[hover_column].offset=hover_item;
	else if (hover_item-columns[hover_column].offset>=num_lines_above)
		columns[hover_column].offset=-num_lines_above+hover_item+1;

	if (columns[hover_column].offset+num_lines_above > columns[hover_column].num_matches)
		columns[hover_column].offset = columns[hover_column].num_matches-num_lines_above;
	if (columns[hover_column].offset<0) columns[hover_column].offset=0;
}

int GraphicalShell::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		if (PerformAction(action)==0) return 0;
	}



	if (ch==LAX_Enter) {
		const char *str=GetItemText(hover_column,hover_item);
		if (isblank(str)) return 0;

		if (searchexpression==NULL) searchexpression=newstr(le->GetCText());
		le->SetText(str);
		app->setfocus(le,0,d);
		le->SetCurpos(-1);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Down) {
		if (hover_column<0) hover_column=0;
		hover_column--; hover_column=NextColumn(hover_column);
		hover_item--;
		if (hover_item<0) hover_item=columns[hover_column].num_matches-1;
		MakeHoverInWindow();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Up) {
		if (hover_column<0) hover_column=0;
		hover_column--; hover_column=NextColumn(hover_column);
		hover_item++;
		if (hover_item>=columns[hover_column].num_matches) hover_item=0;
		MakeHoverInWindow();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Left) {
		hover_column=PreviousColumn(hover_column);

		if (hover_item<0) hover_item=columns[hover_column].num_matches-1;
		if (hover_item>=columns[hover_column].num_matches) hover_item=0;
		MakeHoverInWindow();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Right) {
		hover_column=NextColumn(hover_column);

		if (hover_item<0) hover_item=columns[hover_column].num_matches-1;
		if (hover_item>=columns[hover_column].num_matches) hover_item=0;
		MakeHoverInWindow();
		needtodraw=1;
		return 0;

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

