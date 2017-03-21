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
// Copyright (C) 2004-2006,2010 by Tom Lechner
//

#include "headwindow.h"
#include "laidout.h"
#include "viewwindow.h"
#include "spreadeditor.h"
#include "helpwindow.h"
#include "commandwindow.h"
#include "buttonbox.h"
#include "palettes.h"
#include "headwindow.h"
#include "plaintextwindow.h"
#include "interfaces/objecttree.h"
#include "language.h"

// if gl:
//#include "polyptych/hedronwindow.h"

#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/mouseshapes.h>
#include <lax/shortcutwindow.h>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {


//---------------------------- HeadWindow Pane Generators -----------------------

/*! \defgroup mainwindows Main Pane Windows For HeadWindows
 *
 * These become the panes of a HeadWindow.
 *
 * To create a new one, simply define a creator function, then push that creation function
 * onto the list in HeadWindow::HeadWindow().
 */

////---------------------- newHedronWindowFunc
///*! \ingroup mainwindows
// * \brief HedronWindow window generator for use in HeadWindow.
// */
//anXWindow *newHedronWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style)
//{
//	Polyptych::HedronWindow *text=new Polyptych::HedronWindow(parnt,ntitle,ntitle,style, 0,0,0,0,1, NULL);
//	return text;
//}

//////---------------------- newColorSlidersFunc
///*! \ingroup mainwindows
// * \brief ColorSliders window generator for use in HeadWindow.
// */
//anXWindow *newColorSlidersFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
//{
//	// *** for this to work right, this window needs to send color events to last viewport,
//	// *** and be updated when the color in the last viewport changes, or last viewport changes
//
//	ColorSliders *color=new ColorSliders(***parnt,ntitle,ntitle,style, 0,0,0,0,1, NULL);
//	return color;
//}

////---------------------- newPlainTextWindowFunc
/*! \ingroup mainwindows
 * \brief PlainTextWindow window generator for use in HeadWindow.
 */
anXWindow *newPlainTextWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	PlainTextWindow *text=new PlainTextWindow(parnt,ntitle,ntitle,style, 0,0,0,0,1, NULL);
	return text;
}

////---------------------- newPaletteWindowFunc
/*! \ingroup mainwindows
 * \brief ButtonBox window generator for use in HeadWindow.
 */
anXWindow *newPaletteWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	unsigned long owner=0;
	if (laidout->lastview) owner=laidout->lastview->object_id;
	PaletteWindow *palette=new PalettePane(parnt,ntitle,ntitle,style, 0,0,0,0,1, NULL,owner,"curcolor");
	return palette;
}

////---------------------- newCommandWindowFunc
/*! \ingroup mainwindows
 * \brief ButtonBox window generator for use in HeadWindow.
 */
anXWindow *newButtonBoxFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	ButtonBox *buttons=new ButtonBox(parnt,ntitle,ntitle,style, 0,0,0,0,1);
	return buttons;
}

////---------------------- newCommandWindowFunc
/*! \ingroup mainwindows
 * \brief CommandWindow window generator for use in HeadWindow.
 */
anXWindow *newCommandWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	CommandWindow *command=new CommandWindow(parnt,ntitle,ntitle,style, 0,0,0,0,1);
	return command;
}

////---------------------- newViewWindowFunc
/*! \ingroup mainwindows
 * \brief ViewWindow window generator for use in HeadWindow.
 */
anXWindow *newViewWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	return new ViewWindow(parnt,ntitle,ntitle,style, 0,0,0,0,1, NULL);
}

//------------------------ newSpreadEditorFunc
/*! \ingroup mainwindows
 * \brief SpreadEditor window generator for use in HeadWindow.
 */
anXWindow *newSpreadEditorFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	return new SpreadEditor(parnt,ntitle,ntitle,style, 0,0,0,0,1, NULL,NULL);
}

//------------------------ newLayersFunc
/*! \ingroup mainwindows
 * \brief Layers window generator for use in HeadWindow.
 */
anXWindow *newObjectTreeWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	ViewWindow *view=dynamic_cast<ViewWindow*>(laidout->lastview);

	return new ObjectTreeWindow(parnt,ntitle,ntitle,
								nowner ? nowner->object_id : 0,  NULL,
								dynamic_cast<ObjectContainer*>(view ? view->viewport : NULL));
								//dynamic_cast<ObjectContainer*>(laidout->lastview));
								//dynamic_cast<ObjectContainer*>(laidout));
}

//------------------------ newHelpWindowFunc
/*! \ingroup mainwindows
 * \brief Help window generator for use in HeadWindow.
 */
anXWindow *newHelpWindowFunc(anXWindow *parnt,const char *ntitle,unsigned long style, anXWindow *nowner)
{
	anXWindow *help=newHelpWindow(NULL);
	help->win_style&=~ANXWIN_ESCAPABLE;
	help->win_parent=parnt;
	return help;
}

//---------------------------- newHeadWindow() -----------------------
/*! \ingroup mainwindows
 * \brief Create a new head split window, and fill it with a which window.
 *
 * Current main windows are:
 * <pre>
 *  ViewWindow
 *  SpreadEditor
 *  HelpWindow
 *  CommandWindow
 *  PaletteWindow (will be modified later)
 *  PlainTextEditor
 *
 *   TODO:
 *  ObjectTreeEditor ***not imp
 *  Icons/Menus with optional dialogs ***
 *  Directory Window? FileOpener? ImageFileOpener->to drag n drop images? ***not imp
 *  StoryEditor *** not imp
 * </pre>
 *
 * \todo *** might want to remove doc, or have some more general way to start what is in which.
 *    perhaps have app keep track of most recently accessed doc? or have option to keep a new
 *    head empty
 * \todo *** WindowGone if was only one window, then remove the headwindow, if that was last one, then
 *    either quit or spring up default panel
 * \todo *** this will have to be modified later to more easily add windows through plugins...
 */
anXWindow *newHeadWindow(Document *doc,const char *which)
{
	if (!laidout->dpy) return NULL;
	if (doc==NULL) {
		doc=laidout->curdoc;
		if (!doc) {
			if (!laidout->project) return NULL;
			if (laidout->project->docs.n) doc=laidout->project->docs.e[0]->doc;
		}
	}
	HeadWindow *head=new HeadWindow(NULL,"head",_("Laidout"),0, 0,0,700,700,0);
	//HeadWindow *head=new HeadWindow(NULL,"head",0, 0,0,700,700,0);

	 // put a new which in it. default to view
	if (which) head->Add(which);
	else head->Add(new ViewWindow(doc),0,1);

	return head;
}

//! Create a new head window based on att, that has not been added to the reigning anXApp.
/*! \todo *** should return NULL if the att was invalid
 *  \todo *** for plugins that have new pane types, there must be mechanism to
 *    add those types to new and existing headwindows
 */
anXWindow *newHeadWindow(LaxFiles::Attribute *att)
{
	HeadWindow *head=new HeadWindow(NULL,"head",_("Laidout"),0, 0,0,700,700,0);
	head->dump_in_atts(att,0,NULL);//**context?
	return head;
}

//------------------------------- HeadWindow ---------------------------------------
/*! \class HeadWindow
 * \brief Top level windows to hold other stuff such as a ViewWindow.
 *
 * When coding new pane types, currently the built in ones must be manually added
 * in the constructor.
 * 
 * <pre>
 * *** menu: (todo for splitwindow probably)
 *  hide tabs for stacked panes
 *  show tabs for stacked panes
 *  mark for swap
 *  swap with marked
 *  float
 * </pre>
 */  

 //for SplitWindow::mode
#define NORMAL          0
#define MOVE_EDGES      1
#define VERTICAL_SPLIT  2
#define HORIZ_SPLIT     3
#define MAXIMIZED       4
#define SWAPWITH        50
#define DROPTO          51


Laxkit::PlainWinBox *HeadWindow::markedpane=NULL;
HeadWindow *HeadWindow::markedhead=NULL;

//! Pass SPLIT_WITH_SAME|SPLIT_BEVEL|SPLIT_DRAG_MAPPED to SplitWindow.
/*! Adds the main window type generating functions.
 *
 * \todo Currently, every new HeadWindow instance gets its own set of window generator functions.
 *   This should be changed to a HeadWindow static list, allowing all instances to share one
 *   list, which can easily be added to by plugins..
 */
HeadWindow::HeadWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: SplitWindow(parnt,nname,ntitle,
				nstyle|SPLIT_WITH_SAME|SPLIT_BEVEL|SPLIT_DRAG_MAPPED,
				xx,yy,ww,hh,brder,
				NULL,0,NULL)
{
	//*** should fill with funcs for all known default main windows:
	//  ViewWindow
	//  SpreadEditor
	//  Buttons
	//  FileDialog?
	//  ArrangementEditor
	//  ObjectTreeEditor

	WindowColors *newcolors=new WindowColors(*win_colors);
	installColors(newcolors);
	newcolors->dec_count();
	win_colors->bg=coloravg(win_colors->bg,0,.33);
	space=4;

	tooltip("With mouse in the gutter:\n"
			"  control-left click Splits\n"
			"  shift-left click Joins\n"
			"  right click brings up menu"
		   );

	lastview=lastedit=NULL;

	// add the window generator funcs
	AddWindowType("ViewWindow","View Window",0,newViewWindowFunc,1);
	AddWindowType("SpreadEditor","Spread Editor",0,newSpreadEditorFunc,0);
	AddWindowType("HelpWindow","Help Window",0,newHelpWindowFunc,0);
	AddWindowType("CommandWindow","Command Prompt",0,newCommandWindowFunc,0);
	AddWindowType("PaletteWindow","Palette",PALW_DBCLK_TO_LOAD,newPaletteWindowFunc,0);
	AddWindowType("PlainTextWindow","Text Editor",0,newPlainTextWindowFunc,0);
	AddWindowType("ObjectTreeWindow","Layers",0,newObjectTreeWindowFunc,0);
	//AddWindowType("ColorSliders","Color picker",0,newColorSlidersFunc,0);
	//AddWindowType("HedronWindow","Polyhedron Unwrapper",0,newHedronWindowFunc,0);
	//AddWindowType("ButtonBox","Buttons",
	//		BOXSEL_STRETCHX|BOXSEL_ROWS|BOXSEL_BOTTOM,
	//		newButtonBoxFunc,0);

	sc=NULL;
}

//! Empty virtual destructor.
HeadWindow::~HeadWindow()
{
	DBG cerr <<"in HeadWindow destructor"<<endl;
	if (sc) sc->dec_count();
}

//! Initialize the shortcuts for HeadWindow, and all possible subwindows.
/*! For the purpose of editing or listing all shortcuts, they must first be initialized
 * in the shortcutmanager. This means each window type must be instantiated, and shortcuts output.
 */
void HeadWindow::InitializeShortcuts()
{
	 //initialize head's shortcuts
	GetShortcuts();

	 //initialize window panes' shortcuts
	ShortcutManager *manager=GetDefaultShortcutManager();
	anXWindow *win;

	//DBG int i=0;
	for (int c=0; c<winfuncs.n; c++) {
		if (manager->FindHandler(winfuncs.e[c]->name)) continue;
		//DBG cerr << "init shortcuts for "<<winfuncs.e[c]->name<<endl;

		//DBG i++;
		//DBG if (i!=2) continue;

		win=winfuncs.e[c]->function(NULL,"blah",winfuncs.e[c]->style,NULL);
		win->GetShortcuts();
		win->dec_count();
	}
}

//! Redefined to use a global mark, rather than per SplitWindow.
/*! If c==-1 then mark curbox. Else if c is out of range, then do not change
 * markedpane or markedhead. Else set them to the pane of that index.
 */
int HeadWindow::Mark(int c)
{
	if (c==-1) {
		markedpane=curbox;
		markedhead=this;
		DBG cerr <<"----head marking curbox in window "<<object_id<<endl;

	} else {
		if (c<0 || c>windows.n) return 1;
		markedpane=windows.e[c];
		markedhead=this;
		DBG cerr <<"----head marking box "<<c<<" in window "<<object_id<<endl;
	}
	return 0;
}

//! Redefined to use a global mark, rather than per SplitWindow.
/*! Returns 0 for success, nonzero error.
*/
int HeadWindow::SwapWithMarked()
{
	//DBG cerr <<" --SwapWithMarked: "<<
	if (curbox==NULL || markedpane==NULL || markedhead==NULL || curbox==markedpane) return 0;

	// make sure markedhead is toplevel
	if (laidout->isTopWindow(markedhead)) {
		// make sure markedpane is still pane of markedhead
		if (markedhead->FindBoxIndex(markedpane)>=0) {
			// must potentially reparent!!!
			if (markedhead!=this) {
				if (curbox->win()) app->reparent(curbox->win(),markedhead);
				if (markedpane->win()) app->reparent(markedpane->win(),this);
			}
			anXWindow *w=curbox->win();
			w->inc_count();
			curbox->NewWindow(markedpane->win());
			markedpane->NewWindow(w);
			w->dec_count();
			curbox->sync(space/2);
			markedpane->sync(space/2);
			return 0;
		}
	} 

	markedpane=NULL;
	markedhead=NULL;
	return 1;
}

/*! Return 0 if window taken, else nonzero error.
 *
 * Note that which is ignored. Works only on curbox.
 */
int HeadWindow::Change(anXWindow *towhat,int absorbcount,int which)
{
	//Get doc from curbox. If win has no doc, then get first
	//doc found in head. If none in head, then first in project (***should be last accessed?)
	Document *doc=findAnyDoc();

	//change the window
	if (SplitWindow::Change(towhat,absorbcount,which)!=0) return 1;
	if (!doc) return 0;

	// make sure same doc from old curbox is used for view and spread
	ViewWindow *v=dynamic_cast<ViewWindow *>(curbox->win());
	if (v) {
		((LaidoutViewport *)(v->viewport))->UseThisDoc(doc);
		v->doc=doc;
	} else {
		SpreadEditor *s=dynamic_cast<SpreadEditor *>(curbox->win());
		if (s) s->UseThisDoc(doc);
	}

	return 0;
}

//! Return doc associated with curbox, or with any pane in this, or first doc of laidout->project.
Document *HeadWindow::findAnyDoc()
{
	ViewWindow *v;
	SpreadEditor *s;
	if (curbox) {
		v=dynamic_cast<ViewWindow *>(curbox->win());
		if (v && v->doc) return v->doc;
		s=dynamic_cast<SpreadEditor *>(curbox->win());
		if (s && s->doc) return s->doc;
	}
	for (int c=0; c<windows.n; c++) {
		if (!windows.e[c]->win()) continue;
		v=dynamic_cast<ViewWindow *>(windows.e[c]->win());
		if (v) {
			if (v->doc) return v->doc;
			else continue;
		}
		s=dynamic_cast<SpreadEditor *>(windows.e[c]->win());
		if (s) {
			if (s->doc) return s->doc;
			else continue;
		}
	}
	if (laidout->project && laidout->project->docs.n) return laidout->project->docs.e[0]->doc;
	return NULL;
}

/*! \todo ***** should be able to duplicate views/spreadeditor/etc..
*/
int HeadWindow::splitthewindow(anXWindow *fillwindow)
{
	DBG cerr <<"********SPLITTING THE WINDOW"<<endl;
	//Get doc from curbox. If win has no doc, then get first
	//doc found in head. If none in head, then first in project (***should be last accessed?)
	Document *doc=findAnyDoc();

	anXWindow *winold=curbox->win();
	if (SplitWindow::splitthewindow(fillwindow)!=0) return 1;
	anXWindow *win=windows.e[windows.n-1]->win();
	if (!win) return 0;

	// make sure same doc from old box is used for view and spread
	ViewWindow *v=dynamic_cast<ViewWindow *>(win);
	if (v) {
		((LaidoutViewport *)(v->viewport))->UseThisDoc(doc);
		v->doc=doc;
		ViewWindow *vold=dynamic_cast<ViewWindow *>(winold);
		// duplicate view and page of old
		if (vold) {
			int pg, m=((LaidoutViewport *)(vold->viewport))->ViewMode(&pg);
			((LaidoutViewport *)(v->viewport))->SetViewMode(m,pg);
		}
	} else {
		SpreadEditor *s=dynamic_cast<SpreadEditor *>(win);
		if (s) s->UseThisDoc(doc);
	}
	return 0;
}


//! Dump out what's in the window, and the borders.
/*! \todo *** stacked panes, simply have multiple window blocks under pane
*/
void HeadWindow::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sxywh 0 0 700 700  #The x,y,width,height of the window on the screen\n",spc);
		fprintf(f,"%s#windows contain 1 or more panes, which contain a subwindow\n",spc);
		fprintf(f,"%s#It is dangerous to try to construct panes yourself, because\n",spc);
		fprintf(f,"%s#the borders must touch without gaps. The programming does not currently\n",spc);
		fprintf(f,"%s#recover well when the panes have improper metrics.\n",spc);
		fprintf(f,"%s#What follows is one pane for each of the installed subwindow types.\n",spc);
		anXWindow *win;
		DumpUtility *dump;
		for (int c=0; c<winfuncs.n; c++) {
			fprintf(f,"\n%spane\n",spc);
			fprintf(f,"%s  xyxy 0 0 700 700  #xmin,ymin, and xmax,ymax of the pane\n",spc);

			win=winfuncs.e[c]->function(this,"blah",winfuncs.e[c]->style,NULL);
			fprintf(f,"%s  window %s\n",spc,win->whattype());
			dump=dynamic_cast<DumpUtility *>(win);
			if (dump) dump->dump_out(f,indent+4,-1,NULL);
			else fprintf(f,"%s    #this window has no setable options\n",spc);
			delete win;
		}
		return;
	}

	//anXWindow *win;
	LaxFiles::DumpUtility *wind;

	fprintf(f,"%sxywh %d %d %d %d\n", spc,win_x,win_y,win_w,win_h);

	for (int c=0; c<windows.n; c++) {
		fprintf(f,"%spane\n",spc);
		fprintf(f,"%s  xyxy %d %d %d %d\n",
				spc,windows.e[c]->x1,windows.e[c]->y1,windows.e[c]->x2,windows.e[c]->y2);
		if (windows.e[c]->win()) {
			fprintf(f,"%s  window %s\n",spc,windows.e[c]->win()->whattype());
			wind=dynamic_cast<DumpUtility *>(windows.e[c]->win());
			if (wind) wind->dump_out(f,indent+4,what,context);
		}
	}
}

//! Set up the window according to windows listed in att.
/*! 
 * \todo *** as time goes on, must ensure that header can deal with new types of windows...
 */
void HeadWindow::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	char *name,*value;
	int c,c2,c3;
	PlainWinBox *box;
	anXWindow *win;
	LaxFiles::DumpUtility *wind;
	int i[4];
	windows.flush();
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"xywh")) {
			c2=IntListAttribute(value,i,4);
			if (c2>0) win_x=i[0];
			if (c2>1) win_y=i[1];
			if (c2>2) win_w=i[2];
			if (c2>3) win_h=i[3];

		} else if (!strcmp(name,"pane")) {
			box=new PlainWinBox(NULL,0,0,0,0);
			for (c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"xyxy")) {
					c3=IntListAttribute(value,i,4);
					if (c3>0) box->x1=i[0];
					if (c3>1) box->y1=i[1];
					if (c3>2) box->x2=i[2];
					if (c3>3) box->y2=i[3];
				} else if (!strcmp(name,"window")) {
					DBG cerr <<"HeadWindow add new "<<value<<endl;
					win=NewWindow(value);
					if (win) {
						wind=dynamic_cast<DumpUtility *>(win);
						if (wind) wind->dump_in_atts(att->attributes.e[c]->attributes.e[c2],flag,context);
						box->NewWindow(win); //incs count
						win->dec_count();
					} else {
						DBG cerr <<"**** *** warning: window func not found for "<<(value?value:"(unknown)")<<endl;
					}
				}
			}
			box->sync(space/2,1);
			windows.push(box);
		}
	}
	//***must check that window is actually reasonably onscreen 
}


//! Remove references to win.
void HeadWindow::WindowGone(Laxkit::anXWindow *win)
{
	if (!win) return;
	for (int c=0; c<windows.n; c++) {
		if (windows.e[c]->win()==win) {
			windows.e[c]->NewWindow(NULL);
			if (windows.n==1) app->destroywindow(this);
		}
	}
	needtodraw=1;
}

//! Returns SplitWindow::init().
int HeadWindow::init()
{
	return SplitWindow::init();
	//	if (!window) return 1;
	//
	//	//default SplitWindow just adds new box if windows.n==0
	//	//assume there's one alread?
	//	
	//	return 0;
}

//further menu codes. NOTE TO DEVS: These must not conflict with SplitWindow codes!!
#define HEADW_NewWindow       50
#define HEADW_DropTo          51
#define HEADW_Float           52
#define HEADW_SwapWith        53

/*! \todo *** work on this..
 * <pre>
 *  *** must make something like:
 *   Split
 *   Join
 *   Split...
 *   Change to >
 *   Float
 *   Stack on >
 *   ------
 *   (other things defined by curbox?...
 * </pre>
 */
MenuInfo *HeadWindow::GetMenu()
{
	DBG cerr <<"*************GetMenu: mode="<<mode<<endl;
	MenuInfo *menu=new MenuInfo();

	//make sure this always agrees with SplitWindow::mode!!

	//straight from Laxkit, do not change item ids:
	if (mode!=MAXIMIZED) {
		menu->AddItem("Split",SPLITW_Split);
		menu->AddItem("Join",SPLITW_Join);
	}
	if (winfuncs.n) {
		menu->AddItem("Change to");
		menu->SubMenu();
		for (int c=0; c<winfuncs.n; c++) {
			menu->AddItem(winfuncs.e[c]->desc,SPLITW_ChangeTo_Start+c);
		}
		menu->AddItem("(Blank)",SPLITW_ChangeTo_Blank);
		menu->EndSubMenu();
	}
	if (mode!=MAXIMIZED) {
		menu->AddItem("Mark",SPLITW_Mark);
		menu->AddItem("Swap with marked",SPLITW_Swap_With_Mark);

		//laidout additions:
		menu->AddItem("Swap with...",HEADW_SwapWith);
		menu->AddItem("Drop To...",HEADW_DropTo);
		menu->AddItem("Float",HEADW_Float);
	}

	//straight from Laxkit, do not change item ids:
	if (mode==MAXIMIZED) menu->AddItem("Un-Maximize",SPLITW_UnMaximize);
	else menu->AddItem("Maximize",SPLITW_Maximize);

	//laidout additions:
	menu->AddSep();
	menu->AddItem("New Window",HEADW_NewWindow);
	return menu;
}

/*! Propagate TreeChangeEvent events
*/
int HeadWindow::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"HeadWindow got message: "<<(mes?mes:"(no str)")<<endl;
	if (!strcmp(mes,"docTreeChange")) {
		const TreeChangeEvent *te=dynamic_cast<const TreeChangeEvent *>(data);

		if (!te) return 1;
		ViewWindow *view;
		SpreadEditor *s;

		// Relay docTreeChange to each pane 
		int yes=0;
		for (int c=0; c<windows.n; c++) {
			view=dynamic_cast<ViewWindow *>(windows.e[c]->win());
			if (view) yes=1;
			else if (s=dynamic_cast<SpreadEditor *>(windows.e[c]->win()), s) yes=1;

			//construct events for the panes
			if (yes){
				TreeChangeEvent *edata=new TreeChangeEvent(*te);
				app->SendMessage(edata,windows.e[c]->win()->object_id,"docTreeChange",object_id);
				DBG cerr <<"---sending docTreeChange to "<<
				DBG 	(windows.e[c]->win()->WindowTitle())<<endl;
				yes=0;
			}
		}
		return 0;

	} else if (!strcmp(mes,"open document")) {
		const StrsEventData *s=dynamic_cast<const StrsEventData *>(data);
		if (!s || !s->n) return 1;

		//**** this is really hacky if doc already open...
		int nw;
		Document *d;
		ErrorLog log;
		for (int c=0; c<s->n; c++) {
			if (!s->strs[c]) continue;

			nw=laidout->numTopWindows();
			d=NULL;
			if (laidout->Load(s->strs[c],log)==0) d=laidout->curdoc;
			if (!d) {
				 //load fail
			} else {
				 //load succeeded, maybe errors
				if (nw==laidout->numTopWindows()) {
					app->addwindow(newHeadWindow(d));
				}
			}
		}
		return 0;

	} else if (data->type==LAX_onMouseOut && mode!=SWAPWITH && mode!=DROPTO) {
		mousein=0;
		DBG cerr <<"************************UNDEFINE CURSOR**************** mode="<<mode<<endl;
		const EnterExitData *e=dynamic_cast<const EnterExitData*>(data);
		if (!buttondown.any()) dynamic_cast<LaxMouse*>(e->device)->setMouseShape(this,0);

	} else if (!strcmp(mes,"popupsplitmenu")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		DBG cerr <<"popupsplitmenu: "<<s->info2<<endl;

		if (s->info2==HEADW_NewWindow) {
			 //add new window

			const char *type=NULL;
			if (curbox && curbox->win()) type=curbox->win()->whattype();
			app->addwindow(newHeadWindow(NULL,type));
			return 0;

		} else if (s->info2==HEADW_Float) {
			//Float

			//pop the window to new headwindow
			if (!curbox || !curbox->win() || windows.n<=1) return 0;

			HeadWindow *head=new HeadWindow(NULL,"head",_("Laidout"),0, 0,0,700,700,0);
			app->addwindow(head);
			head->Add(curbox->win());//this reparents win
			curbox->NewWindow(NULL);

			RemoveCurbox();
			return 0;

		} else if (s->info2==HEADW_DropTo) {
			 //set mode to select which pane to drop a window to

			if (mode!=0 || !curbox) return 0;
			DBG cerr <<"  HeadWindow:: Drop To..."<<endl;

			//XWindowAttributes atts;
			//XGetWindowAttributes(app->dpy,window, &atts);
			//if (atts.map_state!=IsViewable) return 0;

			//if (XGrabPointer(app->dpy, window, False,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
			//			GrabModeAsync,GrabModeAsync,
			//			None, None, CurrentTime)!=GrabSuccess) return 0;
			//DBG cerr <<"***********************GRAB***********************"<<endl;
			app->Tooltips(0);

			DBG cerr <<"***********************CURSOR***********************"<<endl;
			//***mouse->setMouseShape(this,LAX_MOUSE_Down);

			markedhead=NULL;
			markedpane=NULL;
			mode=DROPTO;
			DBG cerr <<"***************changing mode to DROPTO"<<endl;
			return 0;

		} else if (s->info2==HEADW_SwapWith) {
			// swap with...

			if (mode!=0 || !curbox) return 0;
			DBG cerr <<"  HeadWindow:: Swap With..."<<endl;

			//XWindowAttributes atts;
			//XGetWindowAttributes(app->dpy,window, &atts);
			//if (atts.map_state!=IsViewable) return 0;

			//DBG cerr <<"***********************GRAB***********************"<<endl;
			//if (XGrabPointer(app->dpy, window, False,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
			//			GrabModeAsync,GrabModeAsync,
			//			None, None, CurrentTime)!=GrabSuccess) return 0;

			DBG cerr <<"***********************CURSOR***********************"<<endl;
			//***mouse->setMouseShape(this,LAX_MOUSE_Exchange);

			app->Tooltips(0);
			markedhead=NULL;
			markedpane=NULL;
			mode=SWAPWITH;
			DBG cerr <<"***************changing mode to SWAPWITH"<<endl;
			return 0;
		}
	}

	return SplitWindow::Event(data,mes);
}

//! Intercept and do nothing when grabbed for a "drop to..." to or "swap with...".
int HeadWindow::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBG cerr <<"***********HeadWindow::LBDown mode=="<<mode<<endl;
	if (mode!=DROPTO && mode!=SWAPWITH) return SplitWindow::LBDown(x,y,state,count,d);
	//markedpane=NULL;
	//markedhead=NULL;
	return 0;
}

//! Intercept to do a "drop to..." to or "swap with...".
int HeadWindow::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	DBG cerr <<"***********HeadWindow::LBUp mode=="<<mode<<endl;
	if (mode!=DROPTO && mode!=SWAPWITH) return SplitWindow::LBUp(x,y,state,d);

	XUngrabPointer(app->dpy, CurrentTime);
	app->Tooltips(1);
	DBG cerr <<"***********************UN-GRAB***********************"<<endl;
	if (!laidout->isTopWindow(markedhead)) { 
		DBG if (!markedhead) cerr <<"***********no marked head"<<endl;
		DBG else cerr <<"***********marked head is not top"<<endl;
		DBG cerr <<"***************changing mode to NORMAL"<<endl;
		mode=0;
		return 0; 
	}
	if (!markedpane) {
		DBG cerr <<"***********no marked pane"<<endl;
		return 0;
	}

	if (mode==DROPTO) {
		if (markedpane!=curbox) {
			// remove original pane and put it in markedhead/markedpane.
			// If there was only one pane in *this, then remove this HeadWindow.

			DBG cerr <<"********Dropping... \"float\" curbox, then split markedpane according to mouse position"<<endl;
			int side=-1,x,y;
			mouseposition(d->id,markedhead,&x,&y,NULL,NULL);
			DBG cerr <<"********** x,y: "<<x<<','<<y<<endl;
			//DBG cerr <<"********"<<markedhead->win_x<<","<<markedhead->win_y<<"  "<<
			//*** this should probably go by corner to corner, not x=y and x=-y.
			x-=markedpane->x1+(markedpane->x2-markedpane->x1)/2;
			y-=markedpane->y1+(markedpane->y2-markedpane->y1)/2;
			DBG cerr <<"********** x,y: "<<x<<','<<y<<endl;
			if (y>0 && y>x && y>-x) side=LAX_BOTTOM;
			else if (y<0 && y<x && y<-x) side=LAX_TOP;
			else if (x>0 && y<x && y>-x) side=LAX_RIGHT;
			else side=LAX_LEFT;
			DBG cerr <<"*******side="<<side<<endl;

			anXWindow *win=curbox->win();
			win->inc_count();
			curbox->NewWindow(NULL);
			DBG cerr <<"***********markedhead->numwindows()="<<markedhead->numwindows()<<endl;
			markedhead->Split(markedhead->FindBoxIndex(markedpane), side, win);//this reparents win
			win->dec_count();
			DBG cerr <<"***********markedhead->numwindows()="<<markedhead->numwindows()<<endl;
			RemoveCurbox();
			if (windows.n==1) {
				app->destroywindow(this);
			}
			DBG cerr <<"*************done Dropping"<<endl;
		}
	} else { //SWAPWITH
		DBG cerr <<"**************** SWAPWITH"<<endl;
		SwapWithMarked();
	}

	mode=0;
	DBG cerr <<"***************changing mode to NORMAL"<<endl;
	return 0;
}

//! Intercept for finding a drop to or swap with location, put in markedhead and markedpane.
/*! This stores the target head and pane in markedhead and markedpane.
*/
int HeadWindow::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{//***
	DBG cerr <<"***********HeadWindow::MouseMove  mode=="<<mode<<endl;
	if (mode!=DROPTO && mode!=SWAPWITH) return SplitWindow::MouseMove(x,y,state,d);

	//***based on mouseposition set markedhead and markedpane
	anXWindow *win=NULL;
	mouseposition(d->id,NULL,&x,&y,NULL,&win);
	if (!win) {
		markedpane=NULL;
		markedhead=NULL;
		return 0;
	}
	while (win->win_parent!=NULL) {
		DBG cerr <<"  -----"<<(win->WindowTitle())<<endl;
		win=win->win_parent;
	}

	markedhead=dynamic_cast<HeadWindow *>(win);
	if (!markedhead) {
		markedpane=NULL;
		return 0;
	}
	mouseposition(d->id,markedhead,&x,&y,NULL,NULL);
	x=markedhead->FindBox(x,y);
	if (x<0) markedhead=NULL;
	else markedhead->Mark(x);
	DBG cerr <<"***************markedpane="<<x<<endl;

	return 0;
}

enum HeadActions {
	HEAD_Quit=1,
	HEAD_OpenDoc,
	HEAD_OpenKeys,
	HEAD_MAX
};

Laxkit::ShortcutHandler *HeadWindow::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("HeadWindow");
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("HeadWindow");

	sc->Add(HEAD_Quit,     'q',ControlMask,0,           _("Quit"), _("Quit Laidout"),NULL,0);
	sc->Add(HEAD_OpenDoc,  'o',ControlMask,0,           _("OpenDoc"), _("Open a document"),NULL,0);
	sc->Add(HEAD_OpenKeys, 'K',ShiftMask|ControlMask,0, _("OpenKeys"), _("Open shortcut key editor"),NULL,0);

	manager->AddArea("HeadWindow",sc);
	return sc;
}

int HeadWindow::PerformAction(int action)
{
	if (action==HEAD_OpenDoc) {
		app->rundialog(new FileDialog(NULL,NULL,_("Open Document"),
					ANXWIN_REMEMBER,
					0,0,0,0,0, object_id,"open document",
					FILES_FILES_ONLY|FILES_OPEN_MANY|FILES_PREVIEW,
					NULL,NULL,NULL,"Laidout"));
		return 0;
		
	} else if (action==HEAD_OpenKeys) {
		laidout->InitializeShortcuts();
		app->addwindow(newHelpWindow("HeadWindow"));
		return 0;

	} else if (action==HEAD_Quit) {
		app->quit();
		cout <<"Quit!\n";
		return 0; 
	}

	return 1;
}

/*! Intercept Esc to revert anything to NORMAL mode.\n
 * 'o'  open new document\n
 * 'q'  quit
*/
int HeadWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	if (ch==LAX_Esc) {
		DBG cerr <<"***************changing mode to NORMAL"<<endl;
		mode=NORMAL;
		d->paired_mouse->setMouseShape(this,0);
		d->paired_mouse->ungrabDevice();
		needtodraw=1;
		return 0;

	} if (ch=='w' && (state&LAX_STATE_MASK)==ControlMask) {
		if (laidout->numTopWindows()>1) {
			app->destroywindow(this);
		}
		return 0;

	}


	return SplitWindow::CharInput(ch,buffer,len,state,d);
}

//! Reset mode when focus somehow leaves the window during non-normal modes.
int HeadWindow::FocusOff(const FocusChangeData *e)
{ //***
	DBG cerr <<"**********************HeadWindow::FocusOff"<<endl;
	if (e->target==this) {
		//if (mode!=SWAPWITH && mode!=DROPTO && mode!=MAXIMIZED) {
		if (mode==SWAPWITH || mode==DROPTO || mode==MAXIMIZED) {
			mode=NORMAL;
			DBG cerr <<"***********************UN-GRAB***********************"<<endl;

			LaxMouse *mouse=dynamic_cast<LaxKeyboard*>(const_cast<LaxDevice*>(e->device))->paired_mouse;
			mouse->ungrabDevice();
			app->Tooltips(1);
			if (mode!=NORMAL && buttondown.any()) {
				//****uh?
			}
		}
		return anXWindow::FocusOff(e);
	}		 
	return SplitWindow::FocusOff(e);
}

//! Create split panes with names like SplitPane12, where the number is getUniqueNumber().
/*! New spread editors will be created with the same document of the most recent view.
 */
anXWindow *HeadWindow::NewWindow(const char *wtype,anXWindow *likethis)
{
	if (!wtype) {
		if (defaultwinfunc<0) return NULL;
		else wtype=winfuncs.e[defaultwinfunc]->name;
	}
		
	anXWindow *win=NULL;
	char blah[100];
	sprintf(blah,"SplitPane%lu",getUniqueNumber());
	for (int c=0; c<winfuncs.n; c++) {
		if (!strcmp(winfuncs.e[c]->name,wtype)) {
			win=winfuncs.e[c]->function(this,blah,winfuncs.e[c]->style,NULL);
			if (!win || !likethis) return win;

			DBG cerr <<"*** need to repair HeadWindow::NewWindow"<<endl;

			// need to dump_out_atts from likethis, then if wtype is same, dump in atts,
			// else just transfer the Document...

			return win;
		}
	}
	return NULL;
}

//! Intercept when curbox changes to keep track what was the most recently focused viewer.
int HeadWindow::Curbox(int c)
{
	int cc=SplitWindow::Curbox(c);
	if (!curbox || !curbox->win()) return cc;
	
	anXWindow *win=curbox->win();
	if (win && !strcmp(win->whattype(),"ViewWindow")) lastview=win;

	return cc;
}

//! Return 1 for this's ViewWindows and SpreadEditors use only doc.
int HeadWindow::HasOnlyThis(Document *doc)
{
	ViewWindow *v;
	SpreadEditor *s;
	for (int c=0; c<windows.n; c++) {
		if (!windows.e[c]->win()) continue;
		v=dynamic_cast<ViewWindow *>(windows.e[c]->win());
		if (v) {
			if (v->doc && v->doc!=doc) return 0;
			else continue;
		}
		s=dynamic_cast<SpreadEditor *>(windows.e[c]->win());
		if (s) {
			if (s->doc && s->doc!=doc) return 0;
			else continue;
		}
	}
	return 1;
}

} // namespace Laidout

