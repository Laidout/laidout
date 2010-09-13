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
// Copyright (C) 2010 by Tom Lechner
//

#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/checkbox.h>

#include "../language.h"
#include "../headwindow.h"
#include "netdialog.h"
#include "impositions/netimposition.h"
#include "utils.h"
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;



//--------------------------------- NetDialog ------------------------------------
/*! \class NetDialog
 *
 * A hacky dialog to select from a polyhedron file, a built in dodecahedron, or a box.
 *
 * \todo make this not hacky
 */  

/*! If doc!=NULL, then assume we are editing settings of that document.
 */
NetDialog::NetDialog(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
					 unsigned int owner, const char *mes,
					 PaperStyle *paper)
		: RowFrame(parnt,nname,ntitle,ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER,
					0,0,500,500,0, NULL,owner,mes,
					10)
{
	//curorientation=0;
	//papersizes=NULL;

	paperstyle=(PaperStyle*)paper->duplicate();

	//doc=ndoc;
	//if (doc) doc->inc_count();
	
	boxdims=NULL;
	impfromfile=NULL;
}

NetDialog::~NetDialog()
{
	if (paperstyle) paperstyle->dec_count();
}


int NetDialog::init()
{
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+12;
	Button *tbut=NULL;
	anXWindow *last=NULL;
	//LineInput *linp=NULL;
	CheckBox *check=NULL;



	// --- file, with extra scaling, if from hedron
	// --- dodecahedron
	// --- box: ___w,h,d_______


	//------------------------------ Dodecahedron -------------------------------------------------------
	last=check=new CheckBox(this,_("Select Dodecahedron"),"selectdodecahedron",CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"checkdod",
							_("Dodecahedron"), 5,5);
	AddWin(check, check->win_w,0,0,50,0, linpheight,0,0,50,0);
	AddWin(NULL, 2000,1990,0,50,0, 20,0,0,50,0); //basically force line break, left justify
	

	 //------------- imposition from file -------------------------------
	last=check=new CheckBox(this,"selectfile",_("Select File"),CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"checkhedron",
							_("Polyhedron or net"), 5,5);
	check->tooltip(_("Create from a polyhedron or a net file."));
	AddWin(check, check->win_w,0,0,50,0, linpheight,0,0,50,0);

	last=impfromfile=new LineInput(this,"impfromfile",NULL,LINP_FILE|LINP_ONLEFT, 0,0,0,0, 0, 
									last,object_id,"impfromfile",
			        			    " ",NULL,0,
						            0,0,1,0,3,3);
	impfromfile->tooltip(_("Create from a polyhedron or a net file."));
	impfromfile->GetLineEdit()->setWinStyle(LINEEDIT_SEND_FOCUS_OFF,1);
	AddWin(impfromfile, impfromfile->win_w,0,2000,50,0, linpheight,0,0,50,0);
	last=tbut=new Button(this,"impfileselect",NULL,0, 0,0,0,0, 1, 
					last,object_id,"impfileselect",
					-1,
					"...",NULL,NULL,3,3);
	tbut->tooltip(_("Search for a polyhedron or a net file"));
	AddWin(tbut, tbut->win_w,0,50,50,0, linpheight,0,0,50,0);
	AddNull();


	//------------------------------ box -------------------------------------------------------
	last=check=new CheckBox(this,_("Select Box"),"selectbox",CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"checkbox",
							_("Box "), 5,5);
	AddWin(check, check->win_w,0,0,50,0, linpheight,0,0,50,0);
	
	last=boxdims=new LineInput(this,"box dims","box dims",LINP_ONLEFT, 0,0,0,0, 0, 
								last,object_id,"boxdims",
				        	    _("Width, length, height:"),"1,1,1",0,
			    	        	100,0,1,1,3,3);
	AddWin(boxdims, boxdims->win_w,0,50,50,0, linpheight,0,0,50,0);




	AddWin(NULL, 2000,1990,0,50,0, 20,0,0,50,0); //force left justify and line break

	//------------------------------ final ok -------------------------------------------------------

	
	 // [ ok ]   [ cancel ]
	last=tbut=new Button(this,"ok",NULL,0, 0,0,0,0,1, last,object_id,"Ok",
						 BUTTON_OK,
						 //doc?_("Apply settings"):_("Create Document"),
						 _("Ok"),
						 NULL,NULL);
	AddWin(tbut, tbut->win_w,0,50,50,0, linpheight,0,0,50,0);
	AddWin(NULL, 20,0,0,50,0, 5,0,0,50,0); // add space of 20 pixels

	last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
	AddWin(tbut, tbut->win_w,0,50,50,0, linpheight,0,0,50,0);


	
	tbut->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

int NetDialog::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

	if (!strcmp(mes,"checkhedron")) {
	} else if (!strcmp(mes,"checkdod")) {
	} else if (!strcmp(mes,"checkbox")) {

	} else if (!strcmp(mes,"impfile")) {
		 //comes after a file select dialog for polyhedron file
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		impfromfile->SetText(s->str);
		//updateImposition();
		return 0;

	} else if (!strcmp(mes,"impfromfile")) { 
		 //activity in hedron file input
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (s->info1==3 || s->info1==1) {
			 //focus was lost or enter pressed from imp file input
			//updateImposition();
		}
		return 0;

	} else if (!strcmp(mes,"impfileselect")) { // from hedron file "..." control button
		app->rundialog(new FileDialog(NULL,NULL,_("Imposition from file"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "impfile",
					FILES_OPEN_ONE,
					impfromfile->GetCText()));
		return 0;


	} else if (!strcmp(mes,"Ok")) {
//		int c=file_exists(saveas->GetCText(),1,NULL);
//		if (c && c!=S_IFREG) {
//			app->setfocus(saveas->GetController(),0);
//			return 0;
//		}
		sendNewImposition();
		if (win_parent && dynamic_cast<HeadWindow*>(win_parent)) dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent && dynamic_cast<HeadWindow*>(win_parent)) dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		else app->destroywindow(this);
		return 0;
	}

	return RowFrame::Event(data,mes);
}


//! Create and fill a Document, and tell laidout to install the new document
void NetDialog::sendNewImposition()
{
}

