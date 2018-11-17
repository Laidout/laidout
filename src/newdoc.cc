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
// Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/tabframe.h>
#include <lax/units.h>

#include "language.h"
#include "newdoc.h"
#include "impositions/singles.h"
#include "impositions/netimposition.h"
#include "impositions/impositioneditor.h"
#include "impositions/netdialog.h"
#include "impositions/singleseditor.h"
#include "impositions/signatures.h"
#include "utils.h"
#include "filetypes/scribus.h"
#include "headwindow.h"
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;
using namespace Polyptych;


namespace Laidout {



//--------------------------------- LaidoutOpenWindow ------------------------------------

/* \class LaidoutOpenWindow
 * \brief The dialog that Laidout starts with if no document is specified.
 *
 * Allows users to select an existing file, create new project, or create new document.
 */
class LaidoutOpenWindow : public Laxkit::TabFrame
{
 public:
	LaidoutOpenWindow(int whichstart);
	virtual ~LaidoutOpenWindow();
	virtual const char *whattype() { return "LaidoutOpenWindow"; }
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int Event(const EventData *data,const char *mes);
};

/*! If whichstart==0, then start with the New Document tab up. 1 is for the New Project tab,
 * and 2 is for the open tab.
 */
LaidoutOpenWindow::LaidoutOpenWindow(int whichstart)
	: TabFrame(NULL,"Laidout Open",_("Laidout"),
			   ANXWIN_REMEMBER|ANXWIN_ESCAPABLE
			   |BOXSEL_LEFT|BOXSEL_TOP|BOXSEL_ONE_ONLY|BOXSEL_ROWS|STRICON_STR_ICON,
			   0,0,500,500,0,
			   NULL,0,NULL, 50)
{
	padinset=laidout->defaultlaxfont->textheight()/3;

	AddWin(new NewDocWindow(this,"New Document",_("New Document"),0, 0,0,0,0, 0), 1,
				_("New Document"),
				NULL,
				0);

	if (laidout->experimental) {
		AddWin(new NewProjectWindow(this,"New Project",_("New Project"),0, 0,0,0,0, 0), 1,
				_("New Project"),
				NULL,
				0);
	}

	FileDialog *fd=new FileDialog(this,"open doc","open doc",
					ANXWIN_REMEMBER, 0,0,0,0, 0,
					object_id, "open doc",
					FILES_NO_CANCEL |FILES_OPEN_MANY |FILES_FILES_ONLY |FILES_PREVIEW,
					NULL,NULL,NULL,
					"Laidout");//recent group
	fd->AddFinalButton(_("Open a copy"),_("This means use that document as a template"),2,1);
	AddWin(fd, 1, _("Open"), NULL,0);


}

LaidoutOpenWindow::~LaidoutOpenWindow()
{ }

int LaidoutOpenWindow::init()
{ 
	for (int c=0; c<_kids.n; c++) _kids.e[c]->SetOwner(this);
	return TabFrame::init();
}

//! Cancel if ESC.
int LaidoutOpenWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		app->destroywindow(this);
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

int LaidoutOpenWindow::Event(const EventData *data,const char *mes)
{
	//****this could be wrapped into a FileDialog subclass specifically for 
	//    opening Laidout documents and projects...
	if (!strcmp(mes,"open doc")) {
		const StrsEventData *strs=dynamic_cast<const StrsEventData *>(data);
		if (!strs || !strs->n) return 1;
		char openingdocs=-1;

		int n=0;
		ErrorLog log;
		for (int c=0; c<strs->n; c++) {
			if (openingdocs==-1 && laidout_file_type(strs->strs[c],NULL,NULL,NULL,"Project",NULL)==0) {
				 //file is project. open and return.
				if (strs->info==1) laidout->Load(strs->strs[c],log);
				app->destroywindow(this);
				return 0;
			}

			if (laidout_file_type(strs->strs[c],NULL,NULL,NULL,"Document",NULL)==0) {
				 //file is document
				n++;
				openingdocs=1;
				int numwindows=laidout->numTopWindows();
				if (strs->info==1) {
					int ret=laidout->Load(strs->strs[c],log);
					if (ret!=0) n--; //load failed
					else {
						if (numwindows==laidout->numTopWindows()) {
							 //loading did not create new window, so add one
							Document *doc=laidout->findDocument(strs->strs[c]);
							//if (doc) app->addwindow(newHeadWindow(doc,"ViewWindow"));
							if (doc) app->addwindow(newHeadWindow(doc));
						}
					}
				} else if (strs->info==2) {
					if (!laidout->LoadTemplate(strs->strs[c],log)) n--;
				}
				continue;
			}

			if (isScribusFile(strs->strs[c])) {
				if (addScribusDocument(strs->strs[c])==0) {
					n++;
					openingdocs=1;
					
					 //create a view window
					Document *doc=laidout->project->docs.e[laidout->project->docs.n-1]->doc;
					if (doc) app->addwindow(newHeadWindow(doc,"ViewWindow"));
				} else {
					//add to error log for failure to open!
				}
				continue;
			}

			 //else:
			//add to error log for failure to open!
		}

		if (n) app->destroywindow(this);

		return 0;

//		StrEventData *str=dynamic_cast<StrEventData *>(data);
//		if (str) {
//			cout << "LaidoutOpenWindow info:"<<str->info<<endl;
//			// ***
//			return 0;
//		}
	}
	return BoxSelector::Event(data,mes);
}

//--------------------------------- BrandNew() ------------------------------------

//! Return a LaidoutOpenWindow which contains a NewDocWindow and NewProjectWindow.
anXWindow *BrandNew(int which)
{
	return new LaidoutOpenWindow(which);
}


//--------------------------------- NewDocWindow ------------------------------------
/*! \class NewDocWindow
 * \brief Class to let users chose which imposition, paper size, and how many pages to make in a document.
 *
 * This is still currently a bit hacky.
 *
 * <pre> 
 *  todo: (?)
 *  *** NewDocWindow different than DocInfoWindow by the Ok/Cancel at bottom?? or difference by who calls it?
 *  *** clear up number of pages, when impositions say to use more than specified
 *  *** first page is page number ____
 *  		margins clip       [ ]
 *  		facing pages carry [ ]
 *  *** modify paper color
 * </pre> 
 *  
 */  

/*! If doc!=NULL, then assume we are editing settings of that document.
 */
NewDocWindow::NewDocWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder,
							Document *ndoc)
		: RowFrame(parnt,nname,ntitle,nstyle|ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER,
					xx,yy,ww,hh,brder, NULL,0,NULL,
					10)
{
	curorientation=0;
	papersizes=NULL;
	numpages=NULL;

	papertype=NULL;
	imp=NULL;
	oldimp=-1;

	doc=ndoc;
	if (doc) {
		imp=(Imposition*)doc->imposition->duplicate();
		doc->inc_count();
	}


	impfromfile=NULL;
	//pageinfo=NULL;
}

NewDocWindow::~NewDocWindow()
{
	if (doc) doc->dec_count();
	if (imp) imp->dec_count();
	delete papertype;
}

int NewDocWindow::preinit()
{
	anXWindow::preinit();
	if (win_w<=0) win_w=500;
	if (win_h<=0) win_h=600;
	return 0;
}

#define IMP_NEW_SINGLES    10000
#define IMP_NEW_SIGNATURE  10001
#define IMP_NEW_NET        10002
#define IMP_FROM_FILE      10003
#define IMP_CURRENT        10004

int NewDocWindow::init()
{
	
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+12;
	Button *tbut;
	anXWindow *last=NULL;
	LineInput *linp;
	MessageBar *mesbar;


	
	 // ------ General Directory Setup ---------------
	 
	int c,c2,o;
	char *where=NULL;
	if (doc) where=newstr(doc->Saveas());
	if (!where && !isblank(laidout->project->filename)) where=lax_dirname(laidout->project->filename,0);

	last=saveas=new LineInput(this,"save as",NULL,LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,object_id,"save as",
			            _("Save As:"),where,0,
			            0,0,1,0,3,3);
	if (where) { delete[] where; where=NULL; }
	AddWin(saveas,1, 300,0,2000,50,0, linpheight,0,0,50,0, -1);
	last=tbut=new Button(this,"saveas",NULL,0, 0,0,0,0, 1, 
			last,object_id,"saveas",
			-1,
			"...",NULL,NULL,3,3);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();//*** forced linebreak
	
	 //add thin spacer
	AddWin(NULL,0, 2000,2000,0,50,0, textheight*2/3,0,0,0,0, -1);//*** forced linebreak


	 // -------------- Paper Size --------------------
	
	papersizes=&laidout->papersizes;

	if (doc && doc->imposition->paper && doc->imposition->paper->paperstyle)
		papertype=(PaperStyle*)doc->imposition->paper->paperstyle->duplicate();
	if (!papertype) papertype=dynamic_cast<PaperStyle*>(laidout->GetDefaultPaper()->duplicate());

	char blah[100],blah2[100];
	o=papertype->landscape();
	curorientation=o;

	 // -----Paper Size X
	UnitManager *units=GetUnitManager();
	sprintf(blah,"%.10g", units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,NULL));
	sprintf(blah2,"%.10g",units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,NULL));
	last=paperx=new LineInput(this,"paper x",NULL,LINP_ONLEFT|LINP_FLOAT, 0,0,0,0, 0, 
						last,object_id,"paper x",
			            _("Paper Size  x:"),(o&1?blah2:blah),0,
			            100,0,1,1,3,3);
	AddWin(paperx,1, paperx->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	
	 // -----Paper Size Y
	last=papery=new LineInput(this,"paper y",NULL,LINP_ONLEFT|LINP_FLOAT, 0,0,0,0, 0, 
						last,object_id,"paper y",
			            _("y:"),(o&1?blah:blah2),0,
			           100,0,1,1,3,3);
	AddWin(papery,1, papery->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	 // -----Default Units
    SliderPopup *popup;
	last=popup=new SliderPopup(this,"units",NULL,0, 0,0, 0,0, 1, last,object_id,"units");
	char *tmp;
	c2=0;
	int uniti=-1,tid;
	units->UnitInfo(laidout->prefs.unitname,&uniti,NULL,NULL,NULL,NULL,NULL);
	for (int c=0; c<units->NumberOfUnits(); c++) {
		units->UnitInfoIndex(c,&tid,NULL,NULL,NULL,&tmp,NULL);
		if (uniti==tid) c2=c;
		popup->AddItem(tmp,c);
	}
	if (c2>=0) popup->Select(c2);
	AddWin(popup,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak

	
	 // -----Paper Name
	last=popup=new SliderPopup(this,"paperName",NULL,0, 0,0, 0,0, 1, last,object_id,"paper name");
	for (int c=0; c<papersizes->n; c++) {
		if (!strcmp(papersizes->e[c]->name,papertype->name)) c2=c;
		popup->AddItem(papersizes->e[c]->name,c);
	}
	popup->Select(c2);
	AddWin(popup,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	
	 // -----Paper Orientation
	last=popup=new SliderPopup(this,"paperOrientation",NULL,0, 0,0, 0,0, 1, last,object_id,"orientation");
	popup->AddItem(_("Portrait"),0);
	popup->AddItem(_("Landscape"),1);
	popup->Select(o&1?1:0);
	AddWin(popup,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);// forced linebreak

	 // -----Number of pages
	int npages=1;
	if (doc) npages=doc->pages.n;
	last=numpages=new LineInput(this,"numpages",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"numpages",
			            _("Number of pages:"),NULL,0, // *** must do auto set papersize
			            100,0,1,1,3,3);
	numpages->GetLineEdit()->setWinStyle(LINEEDIT_SEND_FOCUS_OFF,1);
	numpages->SetText(npages);
	numpages->tooltip(_("The number of pages with which to start a document."));
	AddWin(numpages,1, numpages->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	//pageinfo=mesbar=new MessageBar(this,"pageinfo",NULL,MB_MOVE, 0,0, 0,0, 0, pagesDescription(0));
	//AddWin(mesbar,1, 2000,1900,0,50,0, mesbar->win_h,0,0,50,0, -1);
	//AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);

	AddWin(NULL,0, 2000,2000,0,50,0, textheight*2/3,0,0,0,0, -1);// forced linebreak, vertical spacer


	 // ------------------- printing misc ---------------------------
	 // -------- target dpi:		__300____
	double d=papertype->dpi;
	last=linp=new LineInput(this,"dpi",NULL,LINP_ONLEFT, 5,250,0,0, 0, 
						last,object_id,"dpi",
			            _("Default dpi:"),NULL,0,
			            0,0,1,1,3,3);
	linp->SetText(d);
	AddWin(linp,1, linp->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	//AddWin(NULL, 2000,2000,0,50, 0,0,0,0,0);//*** forced linebreak
	

// ******* uncomment when implemented!!
//
//	 // ------- color mode:		black and white, grayscale, rgb, cmyk, other
//	last=popup=new SliderPopup(this,"colormode",0, 0,0, 0,0, 1, popup,object_id,"colormode");
//	popup->AddItem(_("RGB"),0);
//	popup->AddItem(_("CMYK"),1);
//	popup->AddItem(_("Grayscale"),1);
//	popup->Select(0);
//	AddWin(popup, 200,100,50,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0,0);// forced linebreak
//
//	AddWin(new MessageBar(this,"colormes",MB_MOVE, 0,0,0,0,0, _("Paper Color:")));
//	ColorBox *cbox;
//	last=cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, last,object_id,"paper color", LAX_COLORS_RGB,1./255, 1.,1.,1.);
//	AddWin(cbox, 40,0,50,50,0, linpheight,0,0,50,0);

	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak
	
//	 // ------- target printer: ___whatever____ (file, pdf, html, png, select ppd
//	last=linp=new LineInput(this,"printer",LINP_ONLEFT, 5,250,0,0, 0, 
//						last,object_id,"printer",
//			            _("Target Printer:"),"default (cups)",0,
//			            0,0,1,1,3,3);
//	AddWin(linp, linp->win_w,0,50,50,0, linpheight,0,0,50,0);

//	 //   [ set options from ppd... ]
//	last=tbut=new Button(this,"setfromppd",NULL,0, 0,0,0,0, 1, 
//			last,object_id,"setfromppd",
//			-1,
//			_("Set options from PPD..."),NULL,NULL,3,3);
//	AddWin(tbut, tbut->win_w,0,50,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//*** forced linebreak


	 //add thin spacer
	AddWin(NULL,0, 2000,2000,0,50,0, textheight*2/3,0,0,0,0, -1);//*** forced linebreak


	 //------------- Imposition selection menu ------------------
	
	mesbar=new MessageBar(this,"mesbar 1.1",NULL,MB_MOVE, 0,0, 0,0, 0, _("Imposition:"));
	AddWin(mesbar,1, mesbar->win_w,0,0,50,0, mesbar->win_h,0,0,50,0, -1);
	last=impsel=new SliderPopup(this,"Imposition",NULL,SLIDER_LEFT, 0,0,0,0, 1, 
						last,object_id,"imposition");
	int whichimp=-1,singles=-1;
	if (doc) {
		whichimp=laidout->impositionpool.n;
		impsel->AddItem(_("Current"),IMP_CURRENT);
	}
	for (c=0; c<laidout->impositionpool.n; c++) {
		impsel->AddItem(laidout->impositionpool.e[c]->name,c);
		if (whichimp<0 && doc && !strcmp(doc->imposition->Name(),laidout->impositionpool.e[c]->name))
			whichimp=c;
		if (!strcmp(laidout->impositionpool.e[c]->name,_("Singles"))) singles=c;
	}
	if (whichimp<0) whichimp=singles;

	 // *** these need to be all the imposition base creation types
	//impsel->AddItem(_("NEW Singles...",   IMP_NEW_SINGLES);
	impsel->AddSep();
	impsel->AddItem(_("From file..."),     IMP_FROM_FILE);
	impsel->SetState(-1,SLIDER_IGNORE_ON_BROWSE,1);
	impsel->AddItem(_("NEW Signature..."), IMP_NEW_SIGNATURE);
	impsel->SetState(-1,SLIDER_IGNORE_ON_BROWSE,1);
	impsel->AddItem(_("NEW Net..."),       IMP_NEW_NET);
	impsel->SetState(-1,SLIDER_IGNORE_ON_BROWSE,1);
	impsel->Select(whichimp);
	impsel->WrapToExtent();
	AddWin(impsel,1, impsel->win_w,0,50,50,0, linpheight,0,0,50,0, -1);


	 //--------edit imp...
	AddWin(NULL,0, linpheight/2,0,0,50,0, 20,0,0,50,0, -1);
	last=tbut=new Button(this,"impedit",NULL,0, 0,0,0,0, 1, 
			last,object_id,"impedit",
			-1,
			"Edit imposition...",NULL,NULL,3,3);
	tbut->tooltip(_("Edit the currently selected imposition"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1);//line break


	 //------ imposition brief description
	const char *brief=NULL;
	if (doc) brief=doc->imposition->BriefDescription();
	if (!brief && whichimp>=0 && whichimp<laidout->impositionpool.n) brief=laidout->impositionpool.e[whichimp]->description;

	impmesbar=new MessageBar(this,"mesbar 1.1",NULL,MB_LEFT|MB_MOVE, 0,0, 0,0, 0, brief);
	AddWin(impmesbar,1, 2500,2300,0,50,0, linpheight,0,0,50,0, -1);

	AddWin(NULL,0, 2000,2000,0,50,0, textheight*2/3,0,0,0,0, -1);// forced linebreak, vertical spacer


	 //------- scale pages to fit
	if (doc) {
//		CheckBox *box=NULL;
//		last=box=new CheckBox(this,"scalepages",NULL,CHECK_LEFT, 0,0,0,0,1, 
//				last,object_id,"scalepages", _("Scale pages to fit new imposition"),5,5);
//		box->tooltip(_("Scale each page up or down to fit the page sizes in a new imposition"));
//		AddWin(box,1, box->win_w,0,0,50,0, linpheight,0,0,50,0, -1);

		AddWin(NULL,0, 2000,2000,0,50,0, textheight*2/3,0,0,0,0, -1);// forced linebreak, vertical spacer
	}




	//------------------------------ final ok -------------------------------------------------------

	AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1);
	
	 // [ ok ]   [ cancel ]
	last=tbut=new Button(this,"ok",NULL,0, 0,0,0,0,1, last,object_id,"Ok",
						 BUTTON_OK,
						 doc?_("Apply settings"):_("Create Document"),
						 NULL,NULL);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

	last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);


	
	tbut->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

//! Update the message bar that says how many pages the imposition can hold.
const char *NewDocWindow::pagesDescription(int updatetoo)
{
	if (!imp) {
		//if (updatetoo) pageinfo->SetText(" ");
		return " ";
	}

	double x,y;
	imp->GetDimensions(1, &x,&y);
	int n=imp->NumPages();
	if (n<=0) {
		int nn=atoi(numpages->GetCText());
		if (nn<=0) nn=1;
		n=imp->NumPages(nn);
	}


	static char dims[100];
	if (n==1) sprintf(dims,_("Imposition holds 1 page, %.3g x %.3g"),x,y);
	else sprintf(dims,_("Imposition holds %d pages, %.3g x %.3g"),n,x,y);

	//if (updatetoo) pageinfo->SetText(dims);

	return dims;
}

int NewDocWindow::Event(const EventData *data,const char *mes)
{
	////DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

	if (!strcmp(mes,"save as")) {
		 //comes after a file select dialog for document save as
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		saveas->SetText(s->str);
		return 0;

	} else if (!strcmp(mes,"numpages")) {
		pagesDescription(1);
		return 0;

	} else if (!strcmp(mes,"impedit")) {
		 //edit the current imposition with specialized dialogs
		 //
		if (!imp) {
			 //find which resource is selected, create then edit
			int which=impsel->GetCurrentItemIndex();
			if (which<0 || which>=laidout->impositionpool.n) return 0;
			imp=laidout->impositionpool.e[which]->Create();
		}
		//if (imp->papergroup->GetBasePaper(0)) UpdatePaper(0);
		//else
		imp->SetPaperSize(papertype);
		app->rundialog(newImpositionEditor(NULL,"impose",_("Impose..."),this->object_id,"newimposition",
						papertype, imp->whattype(), imp, NULL, NULL));
		return 0;

	} else if (!strcmp(mes,"impfile")) {
		 //comes after a file select dialog for imposition file, from "From File..." imposition select
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		if (impfromfile) impfromfile->SetText(s->str);
		impositionFromFile(s->str);
		return 0;

	} else if (!strcmp(mes,"paper name")) { 
		 // new paper selected from the popup, so must find the x/y and set x/y appropriately
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		int i=s->info1;
		////DBG cerr <<"new paper size:"<<i<<endl;
		if (i<0 || i>=papersizes->n) return 0;
		delete papertype;
		papertype=(PaperStyle *)papersizes->e[i]->duplicate();
		if (!strcmp(papertype->name,"custom")) return 0;
		papertype->landscape(curorientation);
		char num[30];
		UnitManager *units=GetUnitManager();
		numtostr(num,30,units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		paperx->SetText(num);
		numtostr(num,30,units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		papery->SetText(num);

		UpdatePaper(1);

		//*** would also reset page size x/y based on Layout Scheme, and if page is Default
		return 0;

	} else if (!strcmp(mes,"orientation")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int l=s->info1;
		////DBG cerr <<"New orientation:"<<l<<endl;
		if (l!=curorientation) {
			char *txt=paperx->GetText(),
				*txt2=papery->GetText();
			paperx->SetText(txt2);
			papery->SetText(txt);
			delete[] txt;
			delete[] txt2;
			curorientation=(l?1:0);
			papertype->landscape(curorientation);
			UpdatePaper(1);
		}
		return 0;

	} else if (!strcmp(mes,"paper x")) {
		//***should switch to custom paper maybe
		makestr(papertype->name,_("Custom"));

	} else if (!strcmp(mes,"paper y")) {
		//***should switch to custom paper maybe
		makestr(papertype->name,_("Custom"));

	} else if (!strcmp(mes,"dpi")) {
		//****

	} else if (!strcmp(mes,"units")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		int i=s->info1;
		UnitManager *units=GetUnitManager();
		int id;
		char *name;
		units->UnitInfoIndex(i,&id, NULL,NULL,NULL,&name,NULL);
		paperx->SetText(units->Convert(paperx->GetDouble(), laidout->prefs.default_units,id,NULL));
		papery->SetText(units->Convert(papery->GetDouble(), laidout->prefs.default_units,id,NULL));
		laidout->prefs.default_units=id;
		makestr(laidout->prefs.unitname,name);
		return 0;

	} else if (!strcmp(mes,"imposition")) {
		 //sent by the impsel SliderPopup 

		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		 //when new imposition type selected from popup menu
		if (s->info1==IMP_NEW_SIGNATURE) {
			oldimp=impsel->GetCurrentItemIndex();
			SignatureImposition *sig=new SignatureImposition;
			app->rundialog(new ImpositionEditor(NULL,"impeditor",_("Imposition Editor"),
						   this->object_id,"newimposition",
						   doc, sig, papertype));
			sig->dec_count();
			return 0;

		} else if (s->info1==IMP_NEW_NET) {
			oldimp=impsel->GetCurrentItemIndex();
			NetImposition *net=new NetImposition;
			app->rundialog(new ImpositionEditor(NULL,"impeditor",_("Imposition Editor"),
						   this->object_id,"newimposition",
						   doc, net, papertype));
			net->dec_count();
			return 0;

		} else if (s->info1==IMP_FROM_FILE) {
			oldimp=impsel->GetCurrentItemIndex();
			app->rundialog(new FileDialog(NULL,NULL,_("Imposition from file"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "impfile",
					FILES_OPEN_ONE,
					impfromfile?impfromfile->GetCText():NULL));
			return 0;

		}

		if (s->info1==IMP_CURRENT && doc) {
			if (imp) imp->dec_count();
			imp=(Imposition*)doc->imposition->duplicate();
			impmesbar->SetText(imp->BriefDescription());
			return 0;

		} else if (s->info1<0 || s->info1>=laidout->impositionpool.n) return 0;

		if (imp) imp->dec_count();
		oldimp=s->info1;
		imp=laidout->impositionpool.e[s->info1]->Create();
		if (imp->papergroup && imp->papergroup->GetBasePaper(0)) UpdatePaper(0);
		else imp->SetPaperSize(papertype);

		int nn=atoi(numpages->GetCText());
		if (nn<=0) nn=1;
		imp->NumPages(nn);
		impmesbar->SetText(laidout->impositionpool.e[s->info1]->description);
		pagesDescription(1);

		return 0;

	} else if (!strcmp(mes,"newimposition")) {
		 //sent by the various imposition editors

		if (data->type==LAX_onCancel) {
			impsel->Select(oldimp);
			return 0;
		}

		const RefCountedEventData *r=dynamic_cast<const RefCountedEventData *>(data);
		Imposition *i=dynamic_cast<Imposition *>(const_cast<RefCountedEventData*>(r)->TheObject());
		if (!i) return 0;

		if (imp!=i) {
			if (imp) imp->dec_count();
			imp=i;
			imp->inc_count();
		}
		impmesbar->SetText(imp->BriefDescription());
		UpdatePaper(0);
		numpages->SetText(imp->NumPages());
		return 0;

	} else if (!strcmp(mes,"saveas")) { // from doc save as "..." control button
		app->rundialog(new FileDialog(NULL,NULL,_("Save As"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					saveas->object_id, "save as",
					FILES_SAVE_AS,
					saveas->GetCText()));
		return 0;

	} else if (!strcmp(mes,"Ok")) {
		int c=file_exists(saveas->GetCText(),1,NULL);
		if (c && c!=S_IFREG) {
			app->setfocus(saveas->GetController(),0);
			return 0;
		}
		sendNewDoc();
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;
	}
	return 1;
}

//! Make sure the dialog and the imposition have sync'd up paper types.
/*! If dialogtoimp!=0, then update imp with the dialog's paper, otherwise
 * update the dialog to reflect the imposition paper.
 */
void NewDocWindow::UpdatePaper(int dialogtoimp)
{
	if (dialogtoimp) {
		if (imp) imp->SetPaperSize(papertype);
		pagesDescription(1);	
		return;
	}

	if (!imp) return;

	PaperStyle *paper=imp->GetDefaultPaper();
	if (paper) {
		if (papertype) delete papertype;
		papertype=(PaperStyle*)paper->duplicate();

		 //update orientation
		SliderPopup *o=NULL;
		curorientation=papertype->landscape();
		o=dynamic_cast<SliderPopup*>(findChildWindowByName("paperOrientation"));
		o->Select(curorientation?1:0);

		 //update name
		o=dynamic_cast<SliderPopup*>(findChildWindowByName("paperName"));
		for (int c=0; c<papersizes->n; c++) {
			if (!strcasecmp(papersizes->e[c]->name,papertype->name)) {
				o->Select(c);
				break;
			}
		}

		 //update dimensions
		char num[30];
		UnitManager *units=GetUnitManager();
		numtostr(num,30,units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		paperx->SetText(num);
		numtostr(num,30,units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		papery->SetText(num);

		pagesDescription(1);	
	}
}

//! Use the specified imposition as potentially the new one for the document.
/*! Return 0 for success or nonzero for error, and imp not used.
 *
 * This will increment the count of newimp.
 */
int NewDocWindow::UseThisImposition(Imposition *newimp)
{
	if (!newimp) return 1;
	if (imp) imp->dec_count();
	imp=newimp;
	imp->inc_count();
	impmesbar->SetText(imp->BriefDescription());

	return 0;
}

//! Update imposition settings based on a changed imposition file
void NewDocWindow::impositionFromFile(const char *file)
{
	////DBG cerr<<"----------attempting to impositionFromFile()-------"<<endl;

	 //we load the off file here rather than sendNewDoc() 
	 //to check to see if it is possible to do so... maybe not so important...
	
	Polyhedron *poly=new Polyhedron();
	if (poly->dumpInFile(file,NULL)==0) {
		Net *net=new Net;
		net->basenet=poly;
		net->TotalUnwrap();
		if (imp) imp->dec_count();
		NetImposition *nimp;
		imp=nimp=new NetImposition();
		nimp->SetNet(net);

		 //update popup to net imposition;
		for (int c=0; c<laidout->impositionpool.n; c++) {
			if (!strcmp(imp->Name(),laidout->impositionpool.e[c]->name)) {
				impsel->Select(c);
				break;
			}
		}

		////DBG cerr<<"   installed polyhedron file..."<<endl;
		return;

	} else {
		////DBG cerr <<"*** Failure to read polyhedron file: "<<(file?file:"")<<endl;
		delete poly;
	}

	if (laidout_file_type(file,NULL,NULL,NULL,"Imposition",NULL)==0) {
		cout <<" ***must implement read in imposition, set things in newdoc dialog accordingly"<<endl;
		return;
	}

	if (impfromfile) impfromfile->GetLineEdit()->Valid(0);

	////DBG cerr<<"   impositionFromFile() FAILED..."<<endl;
}

//! Create and fill a Document, and tell laidout to install the new document
void NewDocWindow::sendNewDoc()
{
	 // find and get dup of imposition
	Imposition *imposition=imp; 
	imp=NULL; //disable so as to not delete it in ~NewDocWindow()
	int c;
	if (!imposition) {
		for (c=0; c<laidout->impositionpool.n; c++) {
			if (!strcmp(laidout->impositionpool.e[c]->name,impsel->GetCurrentItem())) break;
		}
		if (c==laidout->impositionpool.n) imposition=new Singles();
		else {
			////DBG cerr <<"****attempting to clone "<<(laidout->impositionpool.e[c]->name)<<endl;
			imposition=laidout->impositionpool.e[c]->Create();
		}
	}
	if (!imposition) { cout <<"**** no imposition in newdoc!!"<<endl; return; }
	
	int npgs=atoi(numpages->GetCText());
	if (npgs<=0) npgs=1;

	imposition->NumPages(npgs);
	imposition->SetPaperSize(papertype);

	if (doc) {
		 //we have a document already, so we are just reimposing
		CheckBox *box=dynamic_cast<CheckBox *>(findChildWindowByName("scalepages"));

		doc->Saveas(saveas->GetCText());
		doc->ReImpose(imposition,box && box->State()==LAX_ON); //1 for scale pages

	} else {
		 //create a new document based on imposition
		laidout->NewDocument(imposition,saveas->GetCText()); //incs imp count, should be 2 now
	}
	if (imposition) imposition->dec_count();//remove excess count
}

//--------------------------------- NewProjectWindow ------------------------------------
/*! \class NewProjectWindow
 *
 * This is currently a god awful hack to get things off the ground.
 * Must be restructured to handle any kind of imposition... 
 * gotta finish the GenericStyleWindow thing, so don't go adding all kinds
 * of hard-coded windows and such here...
 *
 */  

NewProjectWindow::NewProjectWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: RowFrame(parnt,nname,ntitle,nstyle|ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER,
					xx,yy,ww,hh,brder, NULL,0,NULL,
					10)
{
	projectfile=projectdir=NULL;
}

int NewProjectWindow::preinit()
{
	anXWindow::preinit();
	if (win_w<=0) win_w=500;
	if (win_h<=0) win_h=600;
	return 0;
}

int NewProjectWindow::init()
{
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+12;
	Button *tbut=NULL;
	anXWindow *last=NULL;
	LineInput *linp=NULL;


	
	 // ------ General Directory Setup ---------------
	 
	 //--------------Project Name
	last=new LineInput(this,"name",NULL,LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,object_id,"name",
			            _("Project Name:"),NULL,0,
			            0,0,1,0,3,3);
	last->tooltip(_("A descriptive name for the project"));
	AddWin(last,1, 300,0,2000,50,0, linpheight,0,0,50,0, -1);
	AddNull();

	 //------------Project file name
	last=linp=new LineInput(this,"filename",NULL,LINP_ONLEFT, 0,0,0,0, 1, 
							NULL,object_id,"filenameinput",
							_("Project filename:"),NULL,0,
							0,0,1,0,3,3);
	projectfile=linp->GetLineEdit();
	last->tooltip(_("Project file location"));
	AddWin(last,1, 300,0,2000,50,0, linpheight,0,0,50,0, -1);
	last=tbut=new Button(this,"saveprojectfile",NULL,0, 0,0,0,0, 1, 
						last,object_id,"projfilebrowse",
						-1,
						"...",NULL,NULL,3,3);
	last->tooltip(_("Browse for a location"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	 
	 //-------------Project Directory
	last=useprojectdir=new CheckBox(this,"usedir",NULL,CHECK_LEFT, 0,0,0,0,1, 
									last,object_id,"usedir", _("Create directory"),5,5);
	useprojectdir->tooltip(_("Check if you want to use a dedicated project directory"));
	AddWin(useprojectdir,1, useprojectdir->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	last=projectdir=new LineEdit(this,"projdir",NULL,
								LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_ANY_CHANGE, 
								0,0,0,0, 1, 
								last,object_id,"projdirinput",
								NULL,0);
	last->tooltip(_("Optional directory for storing project resources and data"));
	AddWin(last,1, 200,0,2000,50,0, linpheight,0,0,50,0, -1);
	last=tbut=new Button(this,"saveprojectdir",NULL,0, 0,0,0,0, 1, 
						last,object_id,"projdirbrowse",
						-1,
						"...",NULL,NULL,3,3);
	last->tooltip(_("Browse for a location"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1);

	

//	 // -------------- absolute or relative paths --------------------
//	
//	CheckBox *check=NULL;
//	last=check=new CheckBox(this,"abspath",CHECK_LEFT, 0,0,0,0,1, 
//						last,object_id,"abspath", _("Use absolute file paths in saved files"),5,5);
//	check->State(LAX_ON);
//	AddWin(check, check->win_w,0,0,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak
//	
//	last=check=new CheckBox(this,"relpath",CHECK_LEFT, 0,0,0,0,1, 
//						last,object_id,"relpath", _("Use relative file paths in saved files"),5,5);
//	AddWin(check, check->win_w,0,0,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak

	
//	 //------------------ new docs are internal ------------------------
//	
//	last=check=new CheckBox(this,"storedoc",CHECK_LEFT, 0,0,0,0,1, 
//						last,object_id,"storedoc", _("Store new documents in project file"),5,5);
//	AddWin(check, check->win_w,0,0,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak
		
	 // ------------------- printing misc ---------------------------
	 // target dpi:		__300____
	last=linp=new LineInput(this,"dpi",NULL,LINP_ONLEFT, 5,250,0,0, 0, 
						last,object_id,"dpi",
			            _("Default dpi:"),"360",0,
			            0,0,1,1,3,3);
	AddWin(linp,1, linp->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//forced linebreak
	
// ******* uncomment when implemented!!!
//	 // default unit: __inch___
//	last=linp=new LineInput(this,"unit",LINP_ONLEFT, 5,250,0,0, 0, 
//						last=linp,object_id,"unit",
//			            _("Default Units:"),"inch",0,
//			            0,0,1,1,3,3);
//	AddWin(linp, linp->win_w,0,50,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak
//	
//	 // color mode:		black and white, grayscale, rgb, cmyk, other
//	last=linp=new LineInput(this,"colormode",LINP_ONLEFT, 5,250,0,0, 0, 
//						last,object_id,"colormode",
//			            _("Color Mode:"),"rgb",0,
//			            0,0,1,1,3,3);
//	AddWin(linp, linp->win_w,0,50,50,0, linpheight,0,0,50,0);

	//AddWin(new MessageBar(this,"colormes",MB_MOVE, 0,0,0,0,0, _("Paper Color:")));
	//ColorBox *cbox;
	//last=cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, last,object_id,"paper color", LAX_COLORS_RGB,1./255, 1.,1.,1.);
	//AddWin(cbox, 40,0,50,50,0, linpheight,0,0,50,0);

	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//forced linebreak
	
//	 // target printer: ___whatever____ (file, pdf, html, png, select ppd
//	last=linp=new LineInput(this,"printer",LINP_ONLEFT, 5,250,0,0, 0, 
//						last,object_id,"printer",
//			            _("Target Printer:"),"default (cups)",0,
//			            0,0,1,1,3,3);
//	AddWin(linp, linp->win_w,0,50,50,0, linpheight,0,0,50,0);
//
//	 //   [ set options from ppd... ]
//	last=tbut=new Button(this,"setfromppd",0, 0,0,0,0, 1, last,object_id,"setfromppd",
//			_("Set options from PPD..."),3,3);
//	AddWin(tbut, tbut->win_w,0,50,50,0, linpheight,0,0,50,0);
//	AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak


	
	//------------------------------ final ok -------------------------------------------------------

	AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1);
	
	 // [ ok ]   [ cancel ]
	last=tbut=new Button(this,"ok",NULL,0,0,0,0,0,1, last,object_id,"Ok", BUTTON_OK,_("Create Project"));
	tbut->State(LAX_OFF);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels
	last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL,0,0,0,0,1, last,object_id,"Cancel");
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	UpdateOkToCreate();

	
	last->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

NewProjectWindow::~NewProjectWindow()
{
}

int NewProjectWindow::Event(const EventData *data,const char *mes)
{
	////DBG cerr <<"newprojmessage: "<<(mes?mes:"none")<<endl;

	if (!strcmp(mes,"savedir")) {
		 //new directory to save project in
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;

		projectdir->SetText(s->str);
		if (isblank(projectfile->GetCText())) {
			if (s->str[strlen(s->str)-1]=='/') s->str[strlen(s->str)-1]='\0';
			const char *dirend=lax_basename(s->str);
			if (dirend) {
				char *newname=newstr(dirend);
				appendstr(newname,".laidout");
				projectfile->SetText(newname);
				delete[] newname;
			}
		}

		UpdateOkToCreate();
		return 0;

	} else if (!strcmp(mes,"savefile")) {
		 //new file to save project in
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;

		char *dir=lax_dirname(s->str,0);
		const char *name=lax_basename(s->str);
		projectfile->SetText(name);
		if (dir) projectdir->SetText(dir);
		delete[] dir;

		UpdateOkToCreate();
		return 0;

	} else if (!strcmp(mes,"relpath")) { 
		CheckBox *box;
		box=dynamic_cast<CheckBox *>(findChildWindowByName("relpath"));
		if (box) box->State(LAX_ON);
		box=dynamic_cast<CheckBox *>(findChildWindowByName("abspath"));
		if (box) box->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"abspath")) { 
		CheckBox *box;
		box=dynamic_cast<CheckBox *>(findChildWindowByName("relpath"));
		if (box) box->State(LAX_OFF);
		box=dynamic_cast<CheckBox *>(findChildWindowByName("abspath"));
		if (box) box->State(LAX_ON);
		return 0;

	} else if (!strcmp(mes,"target dpi")) {
		//***

	} else if (!strcmp(mes,"target printer")) {
		//***
		
	} else if (!strcmp(mes,"Ok")) {
		if (sendNewProject()) return 0;
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"filenameinput")) {
		 // clicked in filename box, ***maybe should update project dir if checked
		UpdateOkToCreate();
		return 0;

	} else if (!strcmp(mes,"usedir")) {
		// clicked project dir checkbox
		UpdateOkToCreate();
		return 0;

	} else if (!strcmp(mes,"projdirinput")) {
		 //event from project dir LineInput
		//***must activate checkbox
		//    update filename if filename is default, or filename directory
		UpdateOkToCreate();
		return 0;

	} else if (!strcmp(mes,"projdirbrowse")) { 
		 // from project "..." button
		app->rundialog(new FileDialog(NULL,NULL,_("Save Project In"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "savedir",
					FILES_SAVE_AS,
					projectdir->GetCText()));
		return 0;

	} else if (!strcmp(mes,"projfilebrowse")) { 
		 // from filename "..." button
		app->rundialog(new FileDialog(NULL,NULL,_("Save Project In"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "savefile",
					FILES_SAVE_AS,
					projectfile->GetCText()));
		return 0;
	}
	return 0;
}

//! Tell laidout to establish a new document.
/*! Return 0 for project established, else nonzero for error.
 */
int NewProjectWindow::sendNewProject()
{
	if (!UpdateOkToCreate()) return 1;
		
	Project *proj=new Project();

	 //default dpi
	proj->defaultdpi=strtod(((LineInput *)findWindow("dpi"))->GetCText(),NULL);

	 //figure out dir and file name for project
	char *fullpath=full_path_for_file(projectfile->GetCText(),projectdir->GetCText());
	simplify_path(fullpath,1);
	char *dir=lax_dirname(fullpath,0);

	makestr(proj->filename,fullpath);
	if (useprojectdir->State()==LAX_ON) makestr(proj->dir,dir);

	delete[] fullpath;
	delete[] dir;

	 //project name
	const char *newname=((LineInput *)findWindow("name"))->GetCText();
	makestr(proj->name,isblank(newname)?_("New Project"):newname);

	ErrorLog log;
	if (laidout->NewProject(proj,log)) { delete proj; return 2; }
	
	return 0;
}

//! Gray and ungray the "Create Project" button.
/*! Return whether there is enough information to create a project.
 *
 * If there is no filename, there needs to be a project directory. If there is
 * no directory, and dir is unchecked, there needs to be a filename.
 * If there is a directory, but no file, and dir is checked, then the filename
 * defaults to the final bit of the directory plus ".laidout". That is,
 * if the directory is /1/2/3/blah, then the file will be saved in
 * /1/2/3/blah/blah.laidout
 */
int NewProjectWindow::UpdateOkToCreate()
{
	if (!projectdir || !projectfile) return 0; //dialog controls have to exist

	 //c=1 is both file and dir are blank, otherwise 0
	 //If file is blank, but dir exists
	int ok=0; 
	if (useprojectdir->State()==LAX_ON) ok=!isblank(projectfile->GetCText());
	ok=(ok || !isblank(projectfile->GetCText()));

	anXWindow *box=findChildWindowByName("ok");
	if (!box) return ok;
	if (ok) box->Grayed(0); else box->Grayed(1);
	return ok;
}


} // namespace Laidout

