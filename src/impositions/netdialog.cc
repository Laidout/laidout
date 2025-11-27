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
#include "../ui/headwindow.h"
#include "../core/utils.h"
#include "netdialog.h"
#include "netimposition.h"
#include "accordion.h"
#include "polyptychwindow.h"
#include "../polyptych/src/poly.h"
	
#include <lax/debug.h>
using namespace std;

using namespace Laxkit;
using namespace Polyptych;



namespace Laidout {



//--------------------------------- NetDialog ------------------------------------
/*! \class NetDialog
 *
 * A hacky dialog to select from a polyhedron file, a built in dodecahedron, or a box.
 *
 * \todo make this not hacky
 */  

/*! If doc!=nullptr, then assume we are editing settings of that document.
 */
NetDialog::NetDialog(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
					 unsigned int owner, const char *mes,
					 PaperStyle *paper,NetImposition *cur)
		: RowFrame(parnt,nname,ntitle,ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
					0,0,500,500,0, nullptr,owner,mes,
					10)
{
	//curorientation=0;
	//papersizes=nullptr;

	doc = nullptr;

	if (paper) paperstyle = dynamic_cast<PaperStyle*>(paper->duplicate());
	else paperstyle = new PaperStyle;

	//doc=ndoc;
	//if (doc) doc->inc_count();

	checkcurrent = checkbox = checkdod = checkfile = checkaccordion = nullptr;
	accordions = nullptr;
	accordion_1 = accordion_2 = nullptr;
	boxdims     = nullptr;
	impfromfile = nullptr;
	original    = cur;
	current     = cur;
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

int NetDialog::deletenow()
{
	return win_parent == nullptr;
}

int NetDialog::init()
{
	int        textheight = app->defaultlaxfont->textheight();
	int        linpheight = textheight + 12;
	Button    *tbut       = nullptr;
	anXWindow *last       = nullptr;
	// LineInput *linp=nullptr;
	// CheckBox *check=nullptr;

	// --- file, with extra scaling, if from hedron
	// --- dodecahedron
	// --- box: ___w,h,d_______


	//------------------------------ Paper size -------------------------------------------------------
	PaperStyle *papertype = nullptr;
	if (current) papertype = current->GetDefaultPaper();
	psizewindow = new PaperSizeWindow(this, "psizewindow", nullptr, 0, object_id, "papersize", 
										papertype, false, true, false, true);
	AddWin(psizewindow,1,-1);
	AddNull();


	//------------------------------ Current -------------------------------------------------------
	if (current) {
		last = checkcurrent = new CheckBox(this,_("Select Current"),"selectcurrent",CHECK_LEFT, 0,0,0,0,0, 
								last,object_id,"checkcurrent",
								_("Use current"), 5,5);
		checkcurrent->State(LAX_ON);
		AddWin(checkcurrent,1, checkcurrent->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
		AddWin(nullptr,0, 2000,1990,0,50,0, 20,0,0,50,0, -1); //basically force line break, left justify
	} else checkcurrent=nullptr;


	//------------------------------ Dodecahedron -------------------------------------------------------
	last=checkdod=new CheckBox(this,_("Select Dodecahedron"),"selectdodecahedron",CHECK_LEFT, 0,0,0,0,0, 
							last,object_id,"checkdod",
							_("Dodecahedron"), 5,5);
	if (!checkcurrent) checkdod->State(LAX_ON);
	AddWin(checkdod,1, checkdod->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	AddWin(nullptr,0, 2000,1990,0,50,0, 20,0,0,50,0, -1); //basically force line break, left justify
	

	 //------------- imposition from file -------------------------------
	last=checkfile=new CheckBox(this,"selectfile",_("Select File"),CHECK_LEFT, 0,0,0,0,0, 
							last,object_id,"checkfile",
							_("Polyhedron or net"), 5,5);
	checkfile->tooltip(_("Create from a polyhedron or a net file."));
	AddWin(checkfile,1, checkfile->win_w,0,0,50,0, linpheight,0,0,50,0, -1);

	last=impfromfile=new LineInput(this,"impfromfile",nullptr,LINP_FILE|LINP_ONLEFT, 0,0,0,0, 0, 
									last,object_id,"impfromfile",
			        			    " ",nullptr,0,
						            0,0,1,0,3,3);
	impfromfile->tooltip(_("Create from a polyhedron or a net file."));
	impfromfile->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(impfromfile,1, impfromfile->win_w,0,2000,50,0, linpheight,0,0,50,0, -1);
	last=tbut=new Button(this,"impfileselect",nullptr,0, 0,0,0,0, 1, 
					last,object_id,"impfileselect",
					-1,
					"...",nullptr,nullptr,3,3);
	tbut->tooltip(_("Search for a polyhedron or a net file"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();


	//------------------------------ box -------------------------------------------------------
	last=checkbox=new CheckBox(this,_("Select Box"),"selectbox",CHECK_LEFT, 0,0,0,0,0, 
							last,object_id,"checkbox",
							_("Box "), 5,5);
	AddWin(checkbox,1, checkbox->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	
	last=boxdims=new LineInput(this,"box dims","box dims",LINP_ONLEFT, 0,0,0,0, 0, 
								last,object_id,"boxdims",
				        	    _("Width, length, height:"),"1,1,1",0,
			    	        	100,0,1,1,3,3);
	boxdims->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(boxdims,1, boxdims->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(nullptr,0, 3000,2990,0,50,0, 20,0,0,50,0, -1); //force left justify and line break


	//------------------------------ Accordion -------------------------------------------------------
	last = checkaccordion = new CheckBox(this,_("Select Accordion"),"selectaccordion",CHECK_LEFT, 0,0,0,0,0, 
							last,object_id,"checkaccordion",
							_("Accordion"), 5,5);
	AddWin(checkaccordion,1, checkaccordion->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	
	ObjectDef *accordion_def = Accordion::GetPresets();
	last = accordions = new SliderPopup(this,"accordion_type",nullptr,SLIDER_POP_ONLY, 0,0, 0,0, 1, last,object_id,"accordion_type");
	int eid, ez = -1;
	const char *estr;
	for (int c = 1; c < accordion_def->getNumEnumFields(); c++) {
		accordion_def->getEnumInfo(c, nullptr, &estr, nullptr, &eid);
		accordions->AddItem(estr, c);
		if (eid == Accordion::EasyZine) ez = c;
	}
	accordions->Select(ez);
	AddWin(accordions,1, 250,100,50,50,0, linpheight,0,0,50,0, -1);

	last = accordion_1 = new LineInput(this,"accordion_1",nullptr,0, 0,0,0,0, 0, 
								last,object_id,"accordion_1",
				        	    " ","1",0);
	accordion_1->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(accordion_1,1, accordion_1->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	last = accordion_2 = new LineInput(this,"accordion_2",nullptr,0, 0,0,0,0, 0, 
								last,object_id,"accordion_2",
				        	    "x","1",0);
	accordion_2->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(accordion_2,1, accordion_2->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	AddNull();
	// AddWin(nullptr,0, 3000,2990,0,50,0, 20,0,0,50,0, -1); //force left justify and line break

	AddVSpacer(textheight,0,0,50);
	AddNull();
	

	//------------------------------ extra scaling -------------------------------------------------------
	last = scaling = new LineInput(this,"scaling","scaling",LINP_ONLEFT, 0,0,0,0, 0, 
								last,object_id,"scaling",
				        	    _("Extra scaling"),"1",0);
	scaling->tooltip(_("Extra scaling by which to multiply lengths of a net.\n1 means no extra scaling."));
	AddWin(scaling,1, scaling->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();


	//------------------------------ Polyptych editor -----------------------------
#ifndef LAIDOUT_NOGL
	AddVSpacer(textheight,0,0,50);
	AddNull();
	last = tbut = new Button(this,"Edit with Polyptych",nullptr,0, 0,0,0,0,1, last,object_id,"polyptych",
						 BUTTON_OK,
						 //doc?_("Apply settings"):_("Create Document"),
						 _("Edit with Polyptych"),
						 nullptr,nullptr);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();

#endif


	//------------------------------ final ok -------------------------------------------------------

	
	 // [ ok ]   [ cancel ]
//	last=tbut=new Button(this,"ok",nullptr,0, 0,0,0,0,1, last,object_id,"Ok",
//						 BUTTON_OK,
//						 //doc?_("Apply settings"):_("Create Document"),
//						 _("Ok"),
//						 nullptr,nullptr);
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddWin(nullptr,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels
//
//	last=tbut=new Button(this,"cancel",nullptr,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);


	
	last->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

int NetDialog::Event(const EventData *data,const char *mes)
{
	// DBGM("newdocmessage: "<<(mes?mes:"(unknown)"));

	if (!strcmp(mes,"checkfile")) {
		if (checkcurrent)
		  checkcurrent->State(LAX_OFF);
		checkdod      ->State(LAX_OFF);
		checkfile     ->State(LAX_ON);
		checkbox      ->State(LAX_OFF);
		checkaccordion->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkcurrent")) {
		if (checkcurrent)
		  checkcurrent->State(LAX_ON);
		checkdod      ->State(LAX_OFF);
		checkfile     ->State(LAX_OFF);
		checkbox      ->State(LAX_OFF);
		checkaccordion->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkdod")) {
		if (checkcurrent)
		  checkcurrent->State(LAX_OFF);
		checkdod      ->State(LAX_ON);
		checkfile     ->State(LAX_OFF);
		checkbox      ->State(LAX_OFF);
		checkaccordion->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkbox")) {
		if (checkcurrent)
		  checkcurrent->State(LAX_OFF);
		checkdod      ->State(LAX_OFF);
		checkfile     ->State(LAX_OFF);
		checkbox      ->State(LAX_ON);
		checkaccordion->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"checkaccordion")) {
		if (checkcurrent)
		  checkcurrent->State(LAX_OFF);
		checkdod      ->State(LAX_OFF);
		checkfile     ->State(LAX_OFF);
		checkbox      ->State(LAX_OFF);
		checkaccordion->State(LAX_ON);
		return 0;

	} else if (!strcmp(mes,"accordion_type")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int i = s->info1;
		int p1 = 0, p2 = 0;
		int n = Accordion::NumParams((Accordion::AccordionPresets)i, &p1, &p2);
		accordion_2->Grayed(n < 2); accordion_2->SetText(p2);
		accordion_1->Grayed(n < 1); accordion_1->SetText(p1);
		return 0;
		
#ifndef LAIDOUT_NOGL
	} else if (!strcmp(mes,"polyptych")) {
		NetImposition *imp=getNetImposition();
		// DBG cerr <<" ...editing with polyptych maybe..."<<endl;
		if (imp && imp->abstractnet && dynamic_cast<Polyhedron*>(imp->abstractnet)) {
			// DBG cerr <<" ...imp found, editing with polyptych definitely..."<<endl;
			 //ok from PolyptychWindow sends NetImposition to win_owner
			PolyptychWindow *pw=new PolyptychWindow(imp, nullptr,win_owner,win_sendthis);
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
		app->rundialog(new FileDialog(nullptr,nullptr,_("Imposition from file"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "impfile",
					FILES_OPEN_ONE,
					impfromfile->GetCText()));
		return 0;


	} else if (!strcmp(mes,"Ok")) {
//		int c=file_exists(saveas->GetCText(),1,nullptr);
//		if (c && c!=S_IFREG) {
//			app->setfocus(saveas->GetController(),0);
//			return 0;
//		}
		int status = sendNewImposition();
		if (status != 0) return 0;

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
	NetImposition *imp = nullptr;

	if (checkdod->Checked()) {
		imp = new NetImposition;
		imp->SetNet("dodecahedron");

	} else if (checkfile->Checked()) {
		imp = new NetImposition;
		if (imp->SetNetFromFile(impfromfile->GetCText())!=0) {
			delete imp;
			return nullptr;
		}

	} else if (checkcurrent->Checked() && current) {
		imp = current;
		imp->inc_count();

	} else if (checkbox->Checked()) {
		char *str = boxdims->GetText();
		prependstr(str,"box "); //plain box makes cube
		imp=new NetImposition;
		imp->SetNet(str);
		delete[] str;

	} else if (checkaccordion->Checked()) {
		int type = accordions->GetCurrentItemId();
		int p1 = accordion_1->GetLong();
		int p2 = accordion_2->GetLong();
		imp = Accordion::Build((Accordion::AccordionPresets)type, paperstyle->w(), paperstyle->h(), p1, p2, nullptr);
		imp->NumPages(imp->GetPagesNeeded(1));
	}

	if (!imp) return nullptr;

	double s = scaling->GetDouble();
	if (s <= 0) s = 1;
	imp->scalefromnet *= s;

	return imp;
}

//! Create and fill a Document, and tell laidout to install the new document
/*! Return 0 for success, 1 for failure and nothing sent.
 */
int NetDialog::sendNewImposition()
{
	NetImposition *imp = getNetImposition();
	if (!imp) return 1;

	RefCountedEventData *data = new RefCountedEventData(imp);
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

