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
// Copyright (C) 2004-2009 by Tom Lechner
//


#include <lax/multilineedit.h>
#include <lax/mesbar.h>
//#include <lax/textbutton.h>
#include <lax/menubutton.h>
#include <lax/filedialog.h>

#include "plaintextwindow.h"
#include "language.h"
#include "laidout.h"


#define DBG
#include <iostream>
using namespace std;

using namespace Laxkit;

#define TEXT_Select_Temp     -1
#define TEXT_Add_New         -2
#define TEXT_Delete_Current  -3
#define TEXT_Save_Internally -4
#define TEXT_Save_In_File    -5


//------------------------------ PlainTextWindow -------------------------------
/*! \class PlainTextWindow
 * \brief Editor for plain text
 *
 * <pre>
 *  [  the edit box       ]
 *  [owner/filename][apply][name [v]] <-- click name to retrieve other plain text objects
 *  [run (internally)][run with... (externally)]
 * </pre>
 */


/*! Increments the count of newtext.
 */
PlainTextWindow::PlainTextWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder,
		PlainText *newtext)
	: RowFrame(parnt,ntitle,ANXWIN_REMEMBER|ROWFRAME_ROWS|nstyle, xx,yy,ww,hh,brder, NULL,None,NULL)
{
	textobj=newtext;
	if (textobj) textobj->inc_count();
}

PlainTextWindow::~PlainTextWindow()
{
	if (textobj) textobj->dec_count();
}

int PlainTextWindow::DataEvent(Laxkit::EventData *data,const char *mes)
{
	if (!strcmp(mes,"openPopup")) {
		StrEventData *s=dynamic_cast<StrEventData *>(data);
		if (!s || !s->str) return 1;

		 //remove old text object, and install new one
		PlainText *newobj=new PlainText();
		if (newobj->LoadFromFile(s->str)==0) {
			newobj->texttype=TEXT_Note;
			UseThis(newobj);
			laidout->project->textobjects.push(newobj);
		} // else failed to load, do not replace text object
		newobj->dec_count(); //remove excess count

		delete data;
		return 0;
	}
	return 1;
}

int PlainTextWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	DBG cerr <<"plaintext message: "<<mes<<endl;
	if (!strcmp(mes,"text change")) {
		//***update textobj from edit
	} else if (!strcmp(mes,"open")) {
		FileDialog *fd=new FileDialog(NULL,"Open text file...",
					ANXWIN_REMEMBER|FILES_FILES_ONLY|FILES_OPEN_ONE|FILES_PREVIEW,
					0,0,0,0,0, window,"openPopup",
					NULL);
		fd->OkButton(_("Open as text"),NULL);
		app->rundialog(fd);
		return 0;

	} else if (!strcmp(mes,"whichtext")) { 
		int i=e->data.l[1];
		if (i<0) {
			if (i==TEXT_Select_Temp) {
			} else if (i==TEXT_Save_Internally) {
			} else if (i==TEXT_Save_In_File) {
			} else if (i==TEXT_Add_New) {
			} else if (i==TEXT_Delete_Current) {
			}
			//**** was command
		} else if (i>=0 && i<laidout->project->textobjects.n) {
			UseThis(laidout->project->textobjects.e[i]);
		}
		return 0;

	} else if (!strcmp(mes,"whichtextbutton")) { 
		MenuInfo *menu;
		menu=new MenuInfo("Text Objects");

		 //---add textobject list, numbers start at 0
		menu->AddSep(_("Project texts"));
		int c,pos=-1, currentobj=-1, isprojects=0;
		if (laidout->project->textobjects.n) {
			for (c=0; c<laidout->project->textobjects.n; c++) {
				pos=menu->AddItem(laidout->project->textobjects.e[c]->name,c,LAX_ISTOGGLE)-1;
				if (textobj==laidout->project->textobjects.e[c]) {
					currentobj=pos;
					isprojects=1;
					menu->menuitems.e[pos]->state|=LAX_CHECKED;
				}
			}
		} 

		 //------- Content save location
		menu->AddSep(_("Content save location"));
		int isinternal=-1;
		if (textobj && textobj->owner) {
			if (!strcmp(textobj->owner->whattype(),"FileRef")) isinternal=0;
			else isinternal=0;
		} 
		pos=menu->AddItem(_("(Temporary)"),TEXT_Select_Temp,LAX_ISTOGGLE)-1;
		menu->AddItem(_("Save internally"),TEXT_Save_Internally,LAX_ISTOGGLE|(isinternal==1?LAX_CHECKED:0));
		menu->AddItem(_("Save in file"),   TEXT_Save_In_File,LAX_ISTOGGLE|(isinternal==0?LAX_CHECKED:0));

		 //-----further text object operations
		if (currentobj>=0) menu->menuitems.e[currentobj]->state|=LAX_CHECKED;
		menu->AddSep();
		menu->AddItem(_("Add new to project"), TEXT_Add_New);
		if (isprojects) menu->AddItem(_("Remove current from project"), TEXT_Delete_Current);


		 //create the actual popup menu...
		MenuSelector *popup;
		popup=new MenuSelector(NULL,_("Documents"), ANXWIN_BARE|ANXWIN_HOVER_FOCUS,
						0,0,0,0, 1, 
						NULL,window,"whichtext", 
						MENUSEL_ZERO_OR_ONE|MENUSEL_CURSSELECTS
						 //| MENUSEL_SEND_STRINGS
						 | MENUSEL_FOLLOW_MOUSE|MENUSEL_SEND_ON_UP
						 | MENUSEL_GRAB_ON_MAP|MENUSEL_OUT_CLICK_DESTROYS
						 | MENUSEL_CLICK_UP_DESTROYS|MENUSEL_DESTROY_ON_FOCUS_OFF
						 | MENUSEL_CHECK_ON_LEFT|MENUSEL_LEFT,
						menu,1);
		popup->pad=5;
		popup->Select(0);
		popup->WrapToMouse(None);
		app->rundialog(popup);
		return 0;
	} else if (!strcmp(mes,"save")) { 
	} else if (!strcmp(mes,"apply")) { 
	} else if (!strcmp(mes,"run")) { 
	} else if (!strcmp(mes,"nameinput")) { 
	}
	return 1;
}

int PlainTextWindow::init()
{
	anXWindow *last=NULL;

	MultiLineEdit *editbox;
	last=editbox=new MultiLineEdit(this,"plain-text-edit",0, 0,0,0,0,1, NULL,window,"ptedit",
							  0,textobj?textobj->thetext:NULL);
	AddWin(editbox, 100,95,2000,50, 100,95,20000,50);
	AddNull();


	//-----------textobject name edit
	LineInput *nameinput=NULL;
	const char *str=(textobj?(textobj->texttype==TEXT_Temporary?_("(temporary)"):textobj->name):NULL);
	last=nameinput=new LineInput(this,"nameinput",
						0, 0,0,0,0,0, 
						last,window,"nameinput",
						_("Name:"),str,0,
						0,0,2,2,2,2);
	AddWin(nameinput,200,100,1000,50, nameinput->win_h,0,0,50);


	 //------select text object
	IconButton *ibut=NULL;
	last=ibut=new IconButton(this,"whichtext",0, 0,0,0,0,1, NULL,window,"whichtextbutton",-1,
			(const char *)NULL,"v");
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);


	 //--------open
	last=ibut=new IconButton(this,"open",IBUT_ICON_ONLY, 0,0,0,0,1, last,window,"open",-1,
			laidout->icons.GetIcon("Open"),_("Open"));
	ibut->tooltip(_("Open a file from disk"));
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);

	 //--------save
	last=ibut=new IconButton(this,"save",IBUT_ICON_ONLY, 0,0,0,0,1, last,window,"saveDoc",-1,
			laidout->icons.GetIcon("Save"),_("Save"));
	ibut->tooltip(_("Save the current text"));
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);

	 //--------apply
	last=ibut=new IconButton(this,"apply",IBUT_ICON_ONLY, 0,0,0,0,1, last,window,"apply",-1,
			laidout->icons.GetIcon("ApplyText"),_("Apply"));
	ibut->tooltip(_("Update any object or resource that depends on this text"));
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);

	 //--------run
	last=ibut=new IconButton(this,"Run",IBUT_ICON_ONLY, 0,0,0,0,1, last,window,"run",-1,
			laidout->icons.GetIcon("Run"),_("Run"));
	ibut->tooltip(_("Run this text as a script"));
	AddWin(ibut,ibut->win_w,0,50,50, ibut->win_h,0,50,50);

	Sync(1);
	return 0;
}

/*! Return 0 for success, nonzero for error.
 *
 * Increments the count of txt.
 */
int PlainTextWindow::UseThis(PlainText *txt)
{
	if (txt==textobj) return 0;
	if (textobj) textobj->dec_count();
	textobj=txt;
	if (textobj) textobj->inc_count();

	//***update edit from textobj
	cout <<"*******need to update the controls in PlainTextWindow!!"<<endl;
	MultiLineEdit *edit=dynamic_cast<MultiLineEdit *>(findChildWindow("plain-text-edit"));
	if (edit) {
		edit->SetText(textobj->GetText());
	}
	LineInput *inp=dynamic_cast<LineInput *>(findChildWindow("nameinput"));
	if (inp) {
		const char *str=(textobj?(textobj->texttype==TEXT_Temporary?_("(temporary)"):textobj->name):NULL);
		inp->SetText(str);
	}

	return 0;
}

