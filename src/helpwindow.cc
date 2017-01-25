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
// Copyright (C) 2004-2007 by Tom Lechner
//

#include <lax/messagebar.h>
#include <lax/button.h>
#include <lax/tabframe.h>

#include <lax/shortcutwindow.h>

#include "about.h"
#include "language.h"
#include "headwindow.h"
#include "helpwindow.h"
#include "laidout.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;



namespace Laidout {


//------------------------ newHelpWindow -------------------------

class HelpAbout : public Laxkit::TabFrame
{
  public:
	HelpAbout()
		: TabFrame(NULL,"Help",_("Help"),
				   ANXWIN_REMEMBER|ANXWIN_ESCAPABLE| BOXSEL_LEFT|BOXSEL_TOP|BOXSEL_ONE_ONLY|BOXSEL_ROWS,
				   0,0,600,600,0, NULL,0,NULL)
		{}
	virtual ~HelpAbout() {}
	virtual const char *whattype() { return "Help"; }
};

Laxkit::anXWindow *newHelpWindow(const char *place)
{
	laidout->InitializeShortcuts();
	ShortcutManager *manager=GetDefaultShortcutManager();

	if (isblank(manager->setfile)) {
		makestr(manager->setfile,laidout->config_dir);
		appendstr(manager->setfile,"/default.keys");
	}

	ShortcutWindow *shortcutwin=new ShortcutWindow(NULL,"Shortcuts",_("Shortcuts"),
					ANXWIN_REMEMBER|SHORTCUTW_Show_Search|SHORTCUTW_Load_Save,
					0,0,400,600,0,place);
	makestr(shortcutwin->textheader,"#\n# Laidout shortcuts\n#\n");

	return shortcutwin;
}

Laxkit::anXWindow *newHelpWindow2(const char *place)
{
	TabFrame *frame=new HelpAbout();

	laidout->InitializeShortcuts();
	ShortcutManager *manager=GetDefaultShortcutManager();

	if (isblank(manager->setfile)) {
		makestr(manager->setfile,laidout->config_dir);
		appendstr(manager->setfile,"/default.keys");
	}

	ShortcutWindow *shortcutwin=new ShortcutWindow(frame,"Shortcuts",_("Shortcuts"),
					ANXWIN_REMEMBER|SHORTCUTW_Show_Search|SHORTCUTW_Load_Save,
					0,0,400,600,0,place);
	makestr(shortcutwin->textheader,"#\n# Laidout shortcuts\n#\n");

	frame->AddWin(shortcutwin, 1, _("Shortcuts"), NULL, 0);


	AboutWindow *about=new AboutWindow(frame);
	frame->AddWin(about, 1, _("About"), NULL, 0);


	return frame;
}


//------------------------ HelpWindow -------------------------
//
/*! \class HelpWindow
 * \brief Currently just a message box with the list of all the shortcuts.
 *
 * In the future, this class will be rather more than that. Ultimately, the
 * Laxkit will have ability to track the short cuts on the fly, so this
 * window will latch on to that, as well as provide other info...
 */  


//! If style!=0, then do no special sizing in preinit...
/*! \todo anyhow need to work out sizing in Laxkit::MessageBox!!
 */
HelpWindow::HelpWindow(int style)
	: MessageBox(NULL,NULL,"Help!",ANXWIN_ESCAPABLE, 0,0,500,600,0, NULL,0,NULL, NULL)
{
	s=style;
}

/*! The default MessageBox::init() sets m[1]=m[7]=10000, which is supposed 
 * to trigger a wrap to extent. However, if a window has a stretch of 2000, say
 * like the main messagebar, then that window is stretched
 * to that amount, which is silly. So, intercept this to be a more reasonable width.
 */
int HelpWindow::preinit()
{
	if (s) return 0;
	Screen *screen=DefaultScreenOfDisplay(app->dpy);
	
	m[1]=screen->width/2;
	m[7]=10000; //<-- this triggers a wrap in rowcol-figureDims
	//WrapToExtent: 
	arrangeBoxes(1);
	win_w=m[1];
	win_h=m[7];

	if (win_h>(int)(.9*screen->height)) { 
		win_h=(int)(.9*screen->height);
	}
	if (win_w>(int)(.9*screen->width)) { 
		win_w=(int)(.9*screen->width);
	}
	return 0;
}

/*! Pops up a box with the list of shortcuts and an ok button.
 */
int HelpWindow::init()
{
	//MessageBar *mesbar=new MessageBar(this,"helpmesbar",MB_LEFT|MB_MOVE, 0,0,0,0,0, "test");
	char *help=newstr(
		  _("---- Laidout Quick key reference ----\n"
			"\n"
			"Press escape to get rid of this window.\n"
			"Right click drag scrolls this help.\n"
			"Right click drag with shift or control scrolls faster.\n"
			"\n"
			" + means Shift and ^ means control\n"
			"\n"
			"Window gutter:\n"
			"   ^left-click   Split window mouse was in last\n"
			"   +left-click   Join to adjacent window\n"
			"   right-click   Get a menu to split, join, or change\n"
			"\n"
			"\n"));

	laidout->InitializeShortcuts();
	ShortcutManager *m=GetDefaultShortcutManager();

	ShortcutDefs *s;
	WindowActions *a;
	WindowAction *aa;
	char buffer[100],str[400];
	for (int c=0; c<m->shortcuts.n; c++) {
	    sprintf(str,"%s:\n",m->shortcuts.e[c]->area);
		appendstr(help,str);

	    s=m->shortcuts.e[c]->Shortcuts();
	    a=m->shortcuts.e[c]->Actions();
	
	     //output all bound keys
	    if (s) {
	        for (int c2=0; c2<s->n; c2++) {
	            sprintf(str,"  %-15s ",m->ShortcutString(s->e[c2], buffer));
	            if (a) aa=a->FindAction(s->e[c2]->action); else aa=NULL;
	            if (aa) {
	                 //print out string id and commented out description
	                //sprintf(str+strlen(str),"%-20s",aa->name);
	                if (!isblank(aa->description)) sprintf(str+strlen(str),"%s",aa->description);
	                sprintf(str+strlen(str),"\n");
	            } else sprintf(str+strlen(str),"%d\n",s->e[c2]->action); //print out number only
				appendstr(help,str);
	        }
	    }

		 //output any unbound actions
		if (a) {
			int c2=0;
			for (c2=0; c2<a->n; c2++) {
				if (s && s->FindShortcutFromAction(a->e[c2]->id,0)) continue; //action is bound!
				sprintf(str,"none          %s\n",a->e[c2]->description);
				appendstr(help,str);
			}
		}
		appendstr(help,"\n");

	}


	appendstr(help,_(
			"--Extra mouse help--\n"
			"\n"
			"ObjectInterface:\n"
			"  ^left-click  define a center, rotation handle, or shear handle\n"
			"\n"));
	appendstr(help,_(
			"ImageInterface:\n"
            "  double-click  bring up the image properties dialog\n"
			"\n"
			"GradientInterface:\n"
			"  shift-left-click: add a new color spot\n"
			"\n"
			"\n"));


	MessageBar *mesbar=new MessageBar(this,"helpmesbar",NULL,MB_LEFT|MB_TOP|MB_MOVE, 0,0,0,0,0,help);
	delete[] help;
			
	mesbar->tooltip(_("Right click drag scrolls this help."));
	AddWin(mesbar,1,mesbar->win_w,mesbar->win_w*9/10,2000,50,0,
					mesbar->win_h,(mesbar->win_h>10?(mesbar->win_h-10):0),2000,50,0, -1);
	AddNull();
	AddButton(BUTTON_OK);
	
	MessageBox::init();

	app->addwindow(new ShortcutWindow(NULL,"Shortcuts","Shortcuts",
					ANXWIN_REMEMBER|SHORTCUTW_Show_Search|SHORTCUTW_Load_Save,
					0,0,400,600,0));

	return 0;
}

int HelpWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
	return MessageBox::Event(e,mes);
}

	
/*! Esc  dismiss the window.
 */
int HelpWindow::CharInput(unsigned int ch,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;
	}
	return 1;
}


} // namespace Laidout

