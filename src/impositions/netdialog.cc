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
// Copyright (C) 2010,2012,2013 by Tom Lechner
//

#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/checkbox.h>

#include "../language.h"
#include "../headwindow.h"
#include "netdialog.h"
#include "netimposition.h"
#include "polyptychwindow.h"
#include "utils.h"
#include "polyptych/src/poly.h"
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;
using namespace Polyptych;



namespace Laidout {



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
					 PaperStyle *paper,NetImposition *cur)
		: RowFrame(parnt,nname,ntitle,ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
					0,0,500,500,0, NULL,owner,mes,
					10)
{
	//curorientation=0;
	//papersizes=NULL;

	doc=NULL;

	if (paper) paperstyle=(PaperStyle*)paper->duplicate();
	else paper=new PaperStyle;

	//doc=ndoc;
	//if (doc) doc->inc_count();
	
	boxdims=NULL;
	impfromfile=NULL;
	checkbox=checkdod=checkfile=NULL;
	original=cur;
	current=cur;
	if (current) {
		current->inc_count();
		current->inc_count();
	}
}

NetDialog::~NetDialog()
{
	if (paperstyle) paperstyle->dec_count();
	if (current) current->dec_count();
	if (original) original->dec_count();
	if (doc) doc->dec_count();
}


int NetDialog::init()
{
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+12;
	Button *tbut=NULL;
	anXWindow *last=NULL;
	//LineInput *linp=NULL;
	//CheckBox *check=NULL;



	// --- file, with extra scaling, if from hedron
	// --- dodecahedron
	// --- box: ___w,h,d_______


	//------------------------------ Current -------------------------------------------------------
	if (current) {
		last=checkcurrent=new CheckBox(this,_("Select Current"),"selectcurrent",CHECK_LEFT, 0,0,0,0,1, 
								last,object_id,"checkcurrent",
								_("Use current"), 5,5);
		checkcurrent->State(LAX_ON);
		AddWin(checkcurrent,1, checkcurrent->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
		AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1); //basically force line break, left justify
	} else checkcurrent=NULL;


	//------------------------------ Dodecahedron -------------------------------------------------------
	last=checkdod=new CheckBox(this,_("Select Dodecahedron"),"selectdodecahedron",CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"checkdod",
							_("Dodecahedron"), 5,5);
	if (!checkcurrent) checkdod->State(LAX_ON);
	AddWin(checkdod,1, checkdod->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1); //basically force line break, left justify
	

	 //------------- imposition from file -------------------------------
	last=checkfile=new CheckBox(this,"selectfile",_("Select File"),CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"checkfile",
							_("Polyhedron or net"), 5,5);
	checkfile->tooltip(_("Create from a polyhedron or a net file."));
	AddWin(checkfile,1, checkfile->win_w,0,0,50,0, linpheight,0,0,50,0, -1);

	last=impfromfile=new LineInput(this,"impfromfile",NULL,LINP_FILE|LINP_ONLEFT, 0,0,0,0, 0, 
									last,object_id,"impfromfile",
			        			    " ",NULL,0,
						            0,0,1,0,3,3);
	impfromfile->tooltip(_("Create from a polyhedron or a net file."));
	impfromfile->GetLineEdit()->setWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(impfromfile,1, impfromfile->win_w,0,2000,50,0, linpheight,0,0,50,0, -1);
	last=tbut=new Button(this,"impfileselect",NULL,0, 0,0,0,0, 1, 
					last,object_id,"impfileselect",
					-1,
					"...",NULL,NULL,3,3);
	tbut->tooltip(_("Search for a polyhedron or a net file"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();


	//------------------------------ box -------------------------------------------------------
	last=checkbox=new CheckBox(this,_("Select Box"),"selectbox",CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"checkbox",
							_("Box "), 5,5);
	AddWin(checkbox,1, checkbox->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	
	last=boxdims=new LineInput(this,"box dims","box dims",LINP_ONLEFT, 0,0,0,0, 0, 
								last,object_id,"boxdims",
				        	    _("Width, length, height:"),"1,1,1",0,
			    	        	100,0,1,1,3,3);
	boxdims->GetLineEdit()->setWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(boxdims,1, boxdims->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 3000,2990,0,50,0, 20,0,0,50,0, -1); //force left justify and line break


	//------------------------------ Polyptych editor -----------------------------
#ifndef LAIDOUT_NOGL
	last=tbut=new Button(this,"Edit with Polyptych",NULL,0, 0,0,0,0,1, last,object_id,"polyptych",
						 BUTTON_OK,
						 //doc?_("Apply settings"):_("Create Document"),
						 _("Edit with Polyptych"),
						 NULL,NULL);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

#endif


	//------------------------------ extra scaling -------------------------------------------------------
	AddWin(NULL,0, 3000,2990,0,50,0, 20,0,0,50,0, -1); //force left justify and line break
	AddWin(NULL,0, 3000,2990,0,50,0, linpheight/2,0,0,50,0, -1); //extra spacer

	last=scaling=new LineInput(this,"scaling","scaling",LINP_ONLEFT, 0,0,0,0, 0, 
								last,object_id,"scaling",
				        	    _("Extra scaling"),"1",0,
			    	        	100,0,1,1,3,3);
	scaling->tooltip(_("Extra scaling by which to multiply lengths of a net.\n1 means no extra scaling."));
	AddWin(scaling,1, scaling->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,1, 3000,2990,0,50,0, 20,0,0,50,0, -1); //force left justify and line break

	AddWin(NULL,1, 3000,2990,0,50,0, linpheight/2,0,0,50,0, -1); //extra spacer



	//------------------------------ final ok -------------------------------------------------------

	
	 // [ ok ]   [ cancel ]
//	last=tbut=new Button(this,"ok",NULL,0, 0,0,0,0,1, last,object_id,"Ok",
//						 BUTTON_OK,
//						 //doc?_("Apply settings"):_("Create Document"),
//						 _("Ok"),
//						 NULL,NULL);
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels
//
//	last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);


	
	last->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

int NetDialog::Event(const EventData *data,const char *mes)
{
	////DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

	if (!strcmp(mes,"checkfile")) {
		checkdod->State(LAX_OFF);
		checkbox->State(LAX_OFF);
		if (checkcurrent) checkcurrent->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkcurrent")) {
		checkdod->State(LAX_OFF);
		checkbox->State(LAX_OFF);
		checkfile->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkdod")) {
		checkfile->State(LAX_OFF);
		checkbox->State(LAX_OFF);
		if (checkcurrent) checkcurrent->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkbox")) {
		checkfile->State(LAX_OFF);
		checkdod->State(LAX_OFF);
		if (checkcurrent) checkcurrent->State(LAX_OFF);
		return 0;

#ifndef LAIDOUT_NOGL
	} else if (!strcmp(mes,"polyptych")) {
		NetImposition *imp=getNetImposition();
		////DBG cerr <<" ...editing with polyptych maybe..."<<endl;
		if (imp && imp->abstractnet && dynamic_cast<Polyhedron*>(imp->abstractnet)) {
			////DBG cerr <<" ...imp found, editing with polyptych definitely..."<<endl;
			 //ok from PolyptychWindow sends NetImposition to win_owner
			PolyptychWindow *pw=new PolyptychWindow(imp, NULL,win_owner,win_sendthis);
			app->rundialog(pw);
			app->destroywindow(this);
		}
		return 0;
#endif

	} else if (!strcmp(mes,"impfile")) {
		 //comes after a file select dialog for polyhedron file
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		impfromfile->SetText(s->str);
		//updateImposition();
		return 0;

	} else if (!strcmp(mes,"boxdims")) { 
		 //activity in hedron file input
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (s->info1==3 || s->info1==1) {
			 //focus was lost or enter pressed from imp file input
			//updateImposition();
		} else if (s->info1==2) { //focus on
			checkfile->State(LAX_OFF);
			checkbox->State(LAX_ON);
			checkdod->State(LAX_OFF);
		}
		return 0;

	} else if (!strcmp(mes,"impfromfile")) { 
		 //activity in hedron file input
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (s->info1==3 || s->info1==1) {
			 //focus was lost or enter pressed from imp file input
			//updateImposition();
		} else if (s->info1==2) { //focus on
			checkfile->State(LAX_ON);
			checkbox->State(LAX_OFF);
			checkdod->State(LAX_OFF);
		}
		return 0;

	} else if (!strcmp(mes,"impfileselect")) { // from hedron file "..." control button
		checkfile->State(LAX_ON);
		checkbox->State(LAX_OFF);
		checkdod->State(LAX_OFF);
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
		int status=sendNewImposition();
		if (status!=0) return 0;

		if (win_parent && dynamic_cast<HeadWindow*>(win_parent)) dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		EventData *e=new EventData(LAX_onCancel);
		app->SendMessage(e, win_owner, win_sendthis, object_id);

		if (win_parent && dynamic_cast<HeadWindow*>(win_parent)) dynamic_cast<HeadWindow*>(win_parent)->WindowGone(this);
		else app->destroywindow(this);
		return 0;
	}

	return RowFrame::Event(data,mes);
}

NetImposition *NetDialog::getNetImposition()
{
	NetImposition *imp=NULL;
	if (checkdod->State()==LAX_ON) {
		imp=new NetImposition;
		imp->SetNet("dodecahedron");

	} else if (checkfile->State()==LAX_ON) {
		imp=new NetImposition;
		if (imp->SetNetFromFile(impfromfile->GetCText())!=0) {
			delete imp;
			return NULL;
		}

	} else if (checkcurrent->State()==LAX_ON && current) {
		imp=current;
		imp->inc_count();

	} else { //if (checkbox->State()==LAX_ON) { //is box
		char *str=boxdims->GetText();
		prependstr(str,"box "); //plain box makes cube
		imp=new NetImposition;
		imp->SetNet(str);
		delete[] str;

	}

	double s=scaling->GetDouble();
	if (s<=0) s=1;
	imp->scalefromnet*=s;

	return imp;
}

//! Create and fill a Document, and tell laidout to install the new document
/*! Return 0 for success, 1 for failure and nothing sent.
 */
int NetDialog::sendNewImposition()
{
	NetImposition *imp=getNetImposition();
	if (!imp) return 1;

	RefCountedEventData *data=new RefCountedEventData(imp);
	imp->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);

	return 0;
}

const char *NetDialog::ImpositionType()
{ return "NetImposition"; }

Imposition *NetDialog::GetImposition()
{ return current; }

int NetDialog::UseThisDocument(Document *ndoc)
{
	if (doc!=ndoc) {
		if (doc) doc->dec_count();
		doc=ndoc;
		if (doc) doc->inc_count();
	}
	return 0;
}

/*! Return 0 for success, 1 for not a net imposition.
 */
int NetDialog::UseThisImposition(Imposition *nimp)
{
	if (!dynamic_cast<NetImposition*>(nimp)) return 1;

	if (current!=nimp) {
		if (current) current->dec_count();
		current=dynamic_cast<NetImposition*>(nimp);
		if (current) current->inc_count();
	}
	return 0;

}

void NetDialog::ShowSplash(int yes)
{ }

} // namespace Laidout

