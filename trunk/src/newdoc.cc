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


//TODO ******** THIS FILE IS OLD! WILL BE CHANGED DRAMATICALLY
//
//
//
//Overall Document settings
//------- -------- --------
//Save as...	~/other/temp/blah.ldt     --> breaks into working directory/filename
//General Import Directory:
//Image Import Directory:
//
//Paper Size		x: 14    y: 8.5	    _Legal________ (_Whatever_, like for laying out a yard or something, don't show paper/page bounds)
//									_Landscape____
//			Paper Layout Scheme: 	Single, Doublesided, One fold, Business Cards (2x3)...
//
//Page Size  [*] Default   [ ] Custom
//		x: __7___  y: __8.5___
//	Number of Pages: ____23__
//
//View:	[ ] Single Page
//		[ ] Page Layout
//		[ ] Paper Layout (shows printer marks)
//
//Target DPI:      __300____
//Default Unit:    __inch___
//Color Mode:    Black and White, Grayscale, RGB, CMYK, Other
//Target Printer:  ___Whatever____ (file, pdf, html, png, select ppd, ...)
//		[ Set Options From PPD... ]
//
//Paper Color: 	_White___ (modifiable per paper later on)
//
//*** the following only appear when creating new doc, not modifying???
//Initial page setup style: __Default____ (Below are default page setup options)
//  [ Choose Template... ]
//Page Specific Settings (referenced by each page, modifiable)
//---- -------- -------- ---------------------
//										(separate but nested window, based on setup style)
//First Page is Page Number: 	___1_
//
//Margins	Top			__.5___
//		Bottom			__.5___
//		Left/Outside	__.5___
//		Right/Inside	__.5___
//	
//	[ ] Margins clip
//	[ ] Facing Pages Carry Over
//
//
//	_1_Inch__  of paper  =  __1_Inch_  of Displayed Page
//
//
//-------------------------------------------
//	[ OK ]   [ Cancel ]  [ Apply ] <-- Apply only shows if not creating
//
//-------------------------------------------------------------------

//------------------------------ NewDocWindow --------------------------------------------

#include "language.h"
#include "newdoc.h"
#include "impositions/impositioninst.h"
#include "impositions/netimposition.h"
#include "utils.h"
#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/tabframe.h>
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


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
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state);
	virtual int DataEvent(EventData *data,const char *mes);
};

/*! If whichstart==0, then start with the New Document tab up. 1 is for the New Project tab,
 * and 2 is for the open tab.
 */
LaidoutOpenWindow::LaidoutOpenWindow(int whichstart)
	: TabFrame(NULL,_("Laidout"),
			   ANXWIN_REMEMBER|BOXSEL_LEFT|BOXSEL_TOP|BOXSEL_ONE_ONLY|BOXSEL_ROWS|STRICON_STR_ICON,
			   0,0,500,500,0,
			   NULL,None,NULL, 50)
{
	AddWin(new NewDocWindow(this,"New Document",ANXWIN_LOCAL_ACTIVE,0,0,0,0, 0),
				_("New Document"),
				NULL);
	AddWin(new NewProjectWindow(this,"New Project",ANXWIN_LOCAL_ACTIVE,0,0,0,0, 0),
				_("New Project"),
				NULL);

	FileDialog *fd=new FileDialog(this,"open doc",
					ANXWIN_REMEMBER|FILES_NO_CANCEL|FILES_OPEN_MANY, 0,0, 0,0,0,
					None, "open doc", NULL,NULL,NULL, "Laidout");
	fd->AddFinalButton(_("Open a copy"),_("This means use that document as a template"),2,1);
	AddWin(fd, _("Open"), NULL);


}

LaidoutOpenWindow::~LaidoutOpenWindow()
{ }

int LaidoutOpenWindow::init()
{ 
	for (int c=0; c<_kids.n; c++) _kids.e[c]->SetOwner(window);
	return TabFrame::init();
}

//! Cancel if ESC.
int LaidoutOpenWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state)
{
	if (ch==LAX_Esc) {
		app->destroywindow(this);
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state);
}

int LaidoutOpenWindow::DataEvent(EventData *data,const char *mes)
{
	//****this could be wrapped into a FileDialog subclass specifically for 
	//    opening Laidout documents and projects...
	if (!strcmp(mes,"open doc")) {
		StrsEventData *strs=dynamic_cast<StrsEventData *>(data);
		if (!strs || !strs->n) return 1;
		char openingdocs=-1;

		int n=0;
		for (int c=0; c<strs->n; c++) {
			if (openingdocs==-1 && laidout_file_type(strs->strs[c],NULL,NULL,NULL,"Project",NULL)==0) {
				 //file is project. open and return.
				if (strs->info==1) laidout->Load(strs->strs[c],NULL);
				delete data;
				app->destroywindow(this);
				return 0;
			}
			if (laidout_file_type(strs->strs[c],NULL,NULL,NULL,"Document",NULL)==0) {
				 //file is document
				n++;
				openingdocs=1;
				if (strs->info==1) laidout->Load(strs->strs[c],NULL);
				else if (strs->info==2) laidout->LoadTemplate(strs->strs[c]);
			}
		}

		if (n) app->destroywindow(this);

		delete data;
		return 0;

//		StrEventData *str=dynamic_cast<StrEventData *>(data);
//		if (str) {
//			cout << "LaidoutOpenWindow info:"<<str->info<<endl;
//			//***
//			delete data;
//			return 0;
//		}
	}
	return 1;
}

//--------------------------------- BrandNew() ------------------------------------

//! Return a TabFrame with a NewDocWindow and NewProjectWindow.
/*! \todo is this function really still useful?
 */
anXWindow *BrandNew()
{
	return new LaidoutOpenWindow(0);
}


//--------------------------------- NewDocWindow ------------------------------------
/*! \class NewDocWindow
 *
 * This is currently a god awful hack to get things off the ground.
 * Must be restructured to handle any kind of imposition... 
 * gotta finish the GenericStyleWindow thing, so don't go adding all kinds
 * of hard-coded windows and such here...
 *
 * <pre> 
 * ***this should be able to double up as New Doc creation, and also for modifying page parameters later on
 *  		That means breaking down into:
 *  			ProjectInfoWindow
 *  			DocInfoWindow
 *  			PageSetupWindow
 *  		and selecting between "New Document" and "New Project"
 *  *** NewDocWindow different than DocInfoWindow by the Ok/Cancel at bottom?? or difference by who calls it?
 *  *** page size: auto adjust by layout scheme
 *  		x: 20%  (custom, is 20% of default width)
 *  		x: 123   (Default)
 *  *** need function to sync curpaper with the various controls, thus have the right numbers
 *  		appear in the controls on startup
 *  		this needs to have name,x,y,orientation..
 *  *** default units must adjust controls accordingly
 *  *** number of pages
 *  *** first page is page number ____
 *  		margins clip       [ ]
 *  		facing pages carry [ ]
 * </pre> 
 *  
 */  

NewDocWindow::NewDocWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: RowFrame(parnt,ntitle,nstyle|ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER,
					xx,yy,ww,hh,brder, NULL,None,NULL,
					10)
{
	margintextl=_("Left:");
	margintextr=_("Right:");
	margintextt=_("Top:");
	margintextb=_("Bottom:");

	curorientation=0;
	papersizes=NULL;
	numpages=NULL;

	imp=NULL;
	papertype=NULL;
}

int NewDocWindow::preinit()
{
	anXWindow::preinit();
	if (win_w<=0) win_w=500;
	if (win_h<=0) win_h=600;
	return 0;
}

int NewDocWindow::init()
{
	if (!window) return 1;

	
	int textheight=app->defaultfont->max_bounds.ascent+app->defaultfont->max_bounds.descent;
	int linpheight=textheight+12;
	TextButton *tbut;
	anXWindow *last;
	LineInput *linp;


//	//AddWin(lineedit, w,ws,wg,h,valign); for horizontal rows
//	LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
//			            int xx,int yy,int ww,int hh,int bordr,
//			            anXWindow *prev,Window nowner,const char *atom,
//			            const char *newlabel,const char *newtext,unsigned int ntstyle,
//			            int nlw,nlh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0

// rowframe.cc:
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	
	 // ------ General Directory Setup ---------------
	 
	int c,c2,o;
	char *where=NULL;
	if (!where && !isblank(laidout->project->filename)) where=lax_dirname(laidout->project->filename,0);

	saveas=new LineInput(this,"save as",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"save as",
			            _("Save As:"),where,0,
			            0,0,1,0,3,3);
	AddWin(saveas, 300,0,2000,50, linpheight,0,0,50);
	tbut=new TextButton(this,"saveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			saveas,window,"saveas",
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();//*** forced linebreak
	

	 // -------------- Paper Size --------------------
	
	papersizes=&laidout->papersizes;
	papertype=(PaperStyle *)papersizes->e[0]->duplicate();
	char blah[100],blah2[100];
	o=papertype->flags;
	 // -----Paper Size X
	sprintf(blah,"%.10g", papertype->w());
	sprintf(blah2,"%.10g",papertype->h());
	paperx=new LineInput(this,"paper x",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						tbut,window,"paper x",
			            _("Paper Size  x:"),(o&1?blah2:blah),0,
			            100,0,1,1,3,3);
	AddWin(paperx, paperx->win_w,0,50,50, linpheight,0,0,50);
	
	 // -----Paper Size Y
	papery=new LineInput(this,"paper y",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						paperx,window,"paper y",
			            _("y:"),(o&1?blah:blah2),0,
			           100,0,1,1,3,3);
	AddWin(papery, papery->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak

	 // -----Paper Name
    StrSliderPopup *popup;
	popup=new StrSliderPopup(this,"paperName",ANXWIN_CLICK_FOCUS, 0,0, 0,0, 1, papery,window,"paper name",5);
	for (int c=0; c<papersizes->n; c++) {
		if (!strcmp(papersizes->e[c]->name,papertype->name)) c2=c;
		popup->AddItem(papersizes->e[c]->name,c);
	}
	popup->Select(c2);
	AddWin(popup, 200,100,50,50, linpheight,0,0,50);
	
	 // -----Paper Orientation
	last=popup=new StrSliderPopup(this,"paperOrientation",ANXWIN_CLICK_FOCUS, 0,0, 0,0, 1, popup,window,"orientation");
	popup->AddItem(_("Portrait"),0);
	popup->AddItem(_("Landscape"),1);
	popup->Select(o&1?1:0);
	AddWin(popup, 200,100,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);// forced linebreak

	 // -----Number of pages
	last=numpages=new LineInput(this,"numpages",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"numpages",
			            _("Number of pages:"),"1",0, // *** must do auto set papersize
			            100,0,1,1,3,3);
	AddWin(numpages, numpages->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);

	AddWin(NULL, 2000,2000,0,50, textheight*2/3,0,0,0);// forced linebreak, vertical spacer

	 // ------------------- printing misc ---------------------------
	 // target dpi:		__300____
	last=linp=new LineInput(this,"dpi",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"dpi",
			            _("Default dpi:"),"360",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	//AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // default unit: __inch___
	last=linp=new LineInput(this,"unit",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"unit",
			            _("Default Units:"),"inch",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // color mode:		black and white, grayscale, rgb, cmyk, other
	last=linp=new LineInput(this,"colormode",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last=linp,window,"colormode",
			            _("Color Mode:"),"rgb",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	AddWin(new MessageBar(this,"colormes",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, _("Paper Color:")));
	ColorBox *cbox;
	last=cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, last,window,"paper color", 255,255,255);
	AddWin(cbox, 40,0,50,50, linpheight,0,0,50);

	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
//	 // target printer: ___whatever____ (file, pdf, html, png, select ppd
//	last=linp=new LineInput(this,"printer",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
//						last,window,"printer",
//			            _("Target Printer:"),"default (cups)",0,
//			            0,0,1,1,3,3);
//	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

//	 //   [ set options from ppd... ]
//	last=tbut=new TextButton(this,"setfromppd",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
//			last,window,"setfromppd",
//			_("Set options from PPD..."),3,3);
//	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak


	AddWin(NULL, 2000,2000,0,50, textheight*2/3,0,0,0);//*** forced linebreak

	 // ------------- Imposition ------------------
	
	mesbar=new MessageBar(this,"mesbar 1.1",MB_MOVE, 0,0, 0,0, 0, _("Imposition:"));
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	last=impsel=new StrSliderPopup(this,"Imposition",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
						numpages,window,"imposition");
	for (c=0; c<laidout->impositionpool.n; c++)
		impsel->AddItem(laidout->impositionpool.e[c]->Stylename(),c);
	AddWin(impsel, 250,100,50,50, linpheight,0,0,50);

//	last=tbut=new TextButton(this,"impoptions",ANXWIN_CLICK_FOCUS,0,0,0,0,1, last,window,"ImpOptions", _("Imposition Options..."),1);
//	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);

	AddNull();
	
	 //imposition from file
	last=impfromfile=new LineInput(this,"impfromfile",LINP_FILE|ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"impfromfile",
			            _("From file:"),NULL,0,
			            0,0,1,0,3,3);
	impfromfile->tooltip(_("Use an imposition based on a file."));
	impfromfile->GetLineEdit()->setWinStyle(LINEEDIT_SEND_FOCUS_OFF,1);
	AddWin(impfromfile, impfromfile->win_w,0,2000,50, linpheight,0,0,50);
	tbut=new TextButton(this,"impfileselect",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			saveas,window,"impfileselect",
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();

	 // ------Tiling
	last=tiley=new LineInput(this,"y tiling",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"ytile",
			            _("Tile y:"),"1", 0,
			           100,0,1,1,3,3);
	tiley->tooltip("How many times to repeat a spread vertically on a paper.\nIgnored by net impositions");
	AddWin(tiley, tiley->win_w,0,50,50, linpheight,0,0,50);
	last=tilex=new LineInput(this,"x tiling",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"xtile",
			            _("Tile x:"),"1", 0,
			           100,0,1,1,3,3);
	tilex->tooltip(_("How many times to repeat a spread horizontally on a paper.\nIgnored by net impositions"));
	AddWin(tilex, tilex->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak

	
//	 // -------------- page size --------------------
//	
//	mesbar=new MessageBar(this,"mesbar 2",ANXWIN_HOVER_FOCUS|MB_MOVE, 0,0, 0,0, 0, 
//			_("\n\n(Unimplemented stuff follows,\nLook for it in future releases!)"));
//	AddWin(mesbar, 2000,1950,0,50, mesbar->win_h,0,0,50);
//	
//	mesbar=new MessageBar(this,"mesbar 2",ANXWIN_HOVER_FOCUS|MB_MOVE, 0,0, 0,0, 0, 
//			_("pagesize:"));
//	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
//
//	defaultpage=new CheckBox(this,"default",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
//						last,window,"check default", _("default"),5,5);
//	defaultpage->State(LAX_ON);
//	AddWin(defaultpage, defaultpage->win_w,0,0,50, linpheight,0,0,50);
//	
//	custompage=new CheckBox(this,"custom",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
//						defaultpage,window,"check custom", _("custom"),5,5);
//	AddWin(custompage, custompage->win_w,0,0,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
//
//	linp=new LineInput(this,"page x",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
//						custompage,window,"page x",
//			            _("x:"),NULL,0,
//			            100,0,1,1,3,3);
//	AddWin(linp, 120,0,50,50, linpheight,0,0,50);
//	
//	last=linp=new LineInput(this,"page y",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
//						linp,window,"page y",
//			            _("y:"),NULL,0,
//			            100,0,1,1,3,3);
//	AddWin(linp, 120,0,50,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	


//	 // ------------------- view mode ---------------------------
//**** always default to page layout if possible
//	MenuSelector *msel;
//	AddWin(new MessageBar(this,"view style",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, _("view:")));
//	msel=new MenuSelector(this,"view style",ANXWIN_CLICK_FOCUS,
//						0,0,0,0,0,
//						last,window,"view style",
//						MENUSEL_CHECKBOXES|MENUSEL_LEFT|MENUSEL_CURSSELECTS|MENUSEL_ONE_ONLY);
//	msel->AddItem(_("single"),1,0);
//	msel->AddItem(_("page layout"),2,0);
//	msel->AddItem(_("paper layout"),3,0);
//	AddWin(msel, 100,0,50,50, (textheight+5)*3,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,50);//*** forced linebreak
		

	 // --------------------- page specific setup ------------------------------------------
	
	//***first page is page number: 	___1_

	 // ------------------ margins ------------------
	last=linp=margint=new LineInput(this,"margin t",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,window,"margin t",
			            margintextt,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	last=linp=marginb=new LineInput(this,"margin b",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,window,"margin b",
			            margintextb,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	last=linp=marginl=new LineInput(this,"margin l",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,window,"margin l",
			            margintextl,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,50);//*** forced linebreak
	
	last=linp=marginr=new LineInput(this,"margin r",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,window,"margin r",
			            margintextr,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,50);//*** forced linebreak
	


	//***	[ ] margins clip
	//***	[ ] facing pages carry over
	//***	_1_inch__  of paper  =  __1_inch_  of displayed page


	
	//------------------------------ final ok -------------------------------------------------------

	AddWin(NULL, 2000,1990,0,50, 20,0,0,50);
	
	 // [ ok ]   [ cancel ]
	//  TextButton(anxapp *napp,anxwindow *parnt,const char *ntitle,unsigned long nstyle,
	//                        int xx,int yy,int ww,int hh,unsigned int brder,anxwindow *prev,window nowner,atom nsendmes,int nid=0,
	//                        const char *nname=NULL,int npadx=0,int npady=0);
	//  
	last=tbut=new TextButton(this,"ok",ANXWIN_CLICK_FOCUS,0,0,0,0,1, last,window,"Ok", _("Create Document"),TBUT_OK);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 20,0,0,50, 5,0,0,50); // add space of 20 pixels
	last=tbut=new TextButton(this,"cancel",ANXWIN_CLICK_FOCUS|TBUT_CANCEL,0,0,0,0,1, last,window,"Cancel");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);


	
	tbut->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

NewDocWindow::~NewDocWindow()
{
	if (imp) delete imp;
	delete papertype;
}

int NewDocWindow::DataEvent(EventData *data,const char *mes)
{
	if (!strcmp(mes,"save as")) {
		StrEventData *s=dynamic_cast<StrEventData *>(data);
		if (!s) return 1;
		saveas->SetText(s->str);
		delete data;
		return 0;
	}
	return 1;
}

int NewDocWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
	DBG cerr <<"newdocmessage: "<<mes<<endl;
	if (!strcmp(mes,"paper size")) {
	} else if (!strcmp(mes,"ytile")) { 
		DBG cerr <<"**** newdoc: new y tile value"<<endl;
	} else if (!strcmp(mes,"xtile")) { 
		DBG cerr <<"**** newdoc: new x tile value"<<endl;
	} else if (!strcmp(mes,"paper name")) { 
		 // new paper selected from the popup, so must find the x/y and set x/y appropriately
		int i=e->data.l[0];
		DBG cerr <<"new paper size:"<<i<<endl;
		if (i<0 || i>=papersizes->n) return 0;
		delete papertype;
		papertype=(PaperStyle *)papersizes->e[i]->duplicate();
		if (!strcmp(papertype->name,"custom")) return 0;
		papertype->flags=curorientation;
		char num[30];
		numtostr(num,30,papertype->w(),0);
		paperx->SetText(num);
		numtostr(num,30,papertype->h(),0);
		papery->SetText(num);
		//*** would also reset page size x/y based on Layout Scheme, and if page is Default
	} else if (!strcmp(mes,"orientation")) {
		int l=(int)e->data.l[0];
		DBG cerr <<"New orientation:"<<l<<endl;
		if (l!=curorientation) {
			char *txt=paperx->GetText(),
				*txt2=papery->GetText();
			paperx->SetText(txt2);
			papery->SetText(txt);
			delete[] txt;
			delete[] txt2;
			curorientation=l;
			papertype->flags=curorientation;
		}
	} else if (!strcmp(mes,"imposition")) {
		 //when new imposition type selected from popup menu
		if (e->data.l[0]<0 || e->data.l[0]>=laidout->impositionpool.n) return 0;
		if (imp) delete imp;
		imp=(Imposition *)laidout->impositionpool.e[e->data.l[0]]->duplicate();
		if (!strcmp(imp->Stylename(),"Net") || !strcmp(imp->Stylename(),"Singles")) {
			marginl->SetLabel(_("Left:"));
			marginr->SetLabel(_("Right:"));
		} else {
			marginl->SetLabel(_("Outside:"));
			marginr->SetLabel(_("Inside:"));
		}
		return 0;
		
	} else if (!strcmp(mes,"papersizex")) {
	} else if (!strcmp(mes,"papersizey")) {
	} else if (!strcmp(mes,"pagesizex")) {
	} else if (!strcmp(mes,"pagesizey")) {
	} else if (!strcmp(mes,"check custom")) {
		if (custompage->State()==LAX_OFF) custompage->State(LAX_ON);
		defaultpage->State(LAX_OFF);
	} else if (!strcmp(mes,"check default")) {
		if (defaultpage->State()==LAX_OFF) defaultpage->State(LAX_ON);
		custompage->State(LAX_OFF);
		//***gray out the x/y inputs for page size? or auto convert to custom when typing there?
	} else if (!strcmp(mes,"target dpi")) {
	} else if (!strcmp(mes,"target printer")) {
	} else if (!strcmp(mes,"pagesetup proc")) {
	} else if (!strcmp(mes,"paper layout")) {
	} else if (!strcmp(mes,"save as")) {
	} else if (!strcmp(mes,"impfromfile")) { // from control button
		if (e->data.l[0]==3 || e->data.l[0]==1) {
			 //focus was lost or enter pressed
			updateImposition();
		}
		return 0;
	} else if (!strcmp(mes,"impfileselect")) { // from control button
		app->rundialog(new FileDialog(NULL,_("Imposition from file"),
					ANXWIN_REMEMBER|FILES_OPEN_ONE, 0,0, 0,0,0,
					impfromfile->window, "save as",impfromfile->GetCText()));
		return 0;
	} else if (!strcmp(mes,"saveas")) { // from control button
		app->rundialog(new FileDialog(NULL,_("Save As"),
					ANXWIN_REMEMBER|FILES_SAVE_AS, 0,0, 0,0,0,
					saveas->window, "save as",saveas->GetCText()));
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
	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	}
	return 0;
}

//! Update imposition settings based on a changed file
void NewDocWindow::updateImposition()
{
	DBG cerr<<"----------attempting to updateImposition()-------"<<endl;
	const char *file=impfromfile->GetCText();
	if (isOffFile(file)) {
		Polyhedron *poly=new Polyhedron();
		if (poly->dumpInFile(file,NULL)) {
			 //failure!
			DBG cerr <<"*** Failure to read off file: "<<file<<endl;
			delete poly;
			return;
		}
		Net *net=new Net;
		net->basenet=poly;
		net->TotalUnwrap();
		delete imp;
		NetImposition *nimp;
		imp=nimp=new NetImposition();
		nimp->SetNet(net);

		 //update popup to net imposition;
		for (int c=0; c<laidout->impositionpool.n; c++) {
			if (!strcmp(imp->Stylename(),laidout->impositionpool.e[c]->Stylename())) {
				impsel->Select(c);
				break;
			}
		}
		
		DBG cerr<<"   installed OFF file..."<<endl;
		return;
	} else if (laidout_file_type(file,NULL,NULL,NULL,"Imposition",NULL)==0) {
		//***read in imposition, set things in dialog accordingly
		return;
	}
	impfromfile->GetLineEdit()->Valid(0);
	DBG cerr<<"   updateImposition() FAILED..."<<endl;
}

//! Create and fill a DocumentStyle, and tell laidout to make a new document
void NewDocWindow::sendNewDoc()
{
	 // find and get dup of imposition
	Imposition *imposition=NULL;
	int c;
	for (c=0; c<laidout->impositionpool.n; c++) {
		if (!strcmp(laidout->impositionpool.e[c]->Stylename(),impsel->GetCurrentItem())) break;
	}
	if (c==laidout->impositionpool.n) imposition=new Singles();
	else {
		DBG cerr <<"****attempting to clone "<<(laidout->impositionpool.e[c]->Stylename())<<endl;
		imposition=(Imposition *)(laidout->impositionpool.e[c]->duplicate());
	}
	if (!imposition) { cout <<"**** no imposition in newdoc!!"<<endl; return; }
	
	int npgs=atoi(numpages->GetCText()),
		xtile=atoi(tilex->GetCText()),
		ytile=atoi(tiley->GetCText());
	if (npgs<=0) npgs=1;
	if (xtile<=0) xtile=1;
	if (ytile<=0) ytile=1;

	Singles *s=dynamic_cast<Singles *>(imposition);
	if (s) {
		s->tilex=xtile;
		s->tiley=ytile;
		s->insetl=marginl->GetDouble();
		s->insetr=marginr->GetDouble();
		s->insett=margint->GetDouble();
		s->insetb=marginb->GetDouble();
	} else {
		NetImposition *n=dynamic_cast<NetImposition *>(imposition);
		if (n && n->nets.n==0) {
			n->SetNet("dodecahedron");
		}
	}
		
	imposition->NumPages(npgs);
	DocumentStyle *newdoc=new DocumentStyle(imposition);
	newdoc->imposition->SetPaperSize(papertype);
	laidout->NewDocument(newdoc,saveas->GetCText());
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

NewProjectWindow::NewProjectWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: RowFrame(parnt,ntitle,nstyle|ROWFRAME_HORIZONTAL|ROWFRAME_CENTER|ANXWIN_REMEMBER,
					xx,yy,ww,hh,brder, NULL,None,NULL,
					10)
{
	saveas=NULL;
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
	if (!window) return 1;

	
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+12;
	TextButton *tbut=NULL;
	anXWindow *last=NULL;
	LineInput *linp=NULL;


	
	 // ------ General Directory Setup ---------------
	 
	 //--------------Project Name
	last=new LineInput(this,"name",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"name",
			            _("Project Name:"),NULL,0,
			            0,0,1,0,3,3);
	last->tooltip(_("A descriptive name for the project"));
	AddWin(last, 300,0,2000,50, linpheight,0,0,50);
	AddNull();

	 //------------Project file name
	last=new LineInput(this,"filename",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"filename",
			            _("Project filename:"),NULL,0,
			            0,0,1,0,3,3);
	last->tooltip(_("Project file location"));
	AddWin(last, 300,0,2000,50, linpheight,0,0,50);
	AddNull();
	 
	 //-------------Project Directory
	CheckBox *check=NULL;
	last=check=new CheckBox(this,"usedir",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"usedir", _("Create directory"),5,5);
	check->State(LAX_ON);
	check->tooltip(_("Check if you want to use a dedicated project directory"));
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	last=saveas=new LineEdit(this,"projdir",
						LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_ANY_CHANGE, 
						0,0,0,0, 1, 
						last,window,"proj dir",
			            NULL,0);
	last->tooltip(_("Optional directory for storing project resources and data"));
	AddWin(last, 200,0,2000,50, linpheight,0,0,50);
	last=tbut=new TextButton(this,"saveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"projdir",
			"...",3,3);
	last->tooltip(_("Browse for a location"));
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();
	
	AddWin(NULL, 2000,1990,0,50, 20,0,0,50);

	

//	 // -------------- absolute or relative paths --------------------
//	
//	CheckBox *check=NULL;
//	last=check=new CheckBox(this,"abspath",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
//						last,window,"abspath", _("Use absolute file paths in saved files"),5,5);
//	check->State(LAX_ON);
//	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak
//	
//	last=check=new CheckBox(this,"relpath",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
//						last,window,"relpath", _("Use relative file paths in saved files"),5,5);
//	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak

	
//	 //------------------ new docs are internal ------------------------
//	
//	last=check=new CheckBox(this,"storedoc",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
//						last,window,"storedoc", _("Store new documents in project file"),5,5);
//	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak
		
	 // ------------------- printing misc ---------------------------
	 // target dpi:		__300____
	last=linp=new LineInput(this,"dpi",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"dpi",
			            _("Default dpi:"),"360",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak
	
	 // default unit: __inch___
	last=linp=new LineInput(this,"unit",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last=linp,window,"unit",
			            _("Default Units:"),"inch",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak
	
	 // color mode:		black and white, grayscale, rgb, cmyk, other
	last=linp=new LineInput(this,"colormode",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"colormode",
			            _("Color Mode:"),"rgb",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	//AddWin(new MessageBar(this,"colormes",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, _("Paper Color:")));
	//ColorBox *cbox;
	//last=cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, last,window,"paper color", 255,255,255);
	//AddWin(cbox, 40,0,50,50, linpheight,0,0,50);

	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak
	
//	 // target printer: ___whatever____ (file, pdf, html, png, select ppd
//	last=linp=new LineInput(this,"printer",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
//						last,window,"printer",
//			            _("Target Printer:"),"default (cups)",0,
//			            0,0,1,1,3,3);
//	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
//
//	 //   [ set options from ppd... ]
//	last=tbut=new TextButton(this,"setfromppd",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, last,window,"setfromppd",
//			_("Set options from PPD..."),3,3);
//	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
//	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//forced linebreak


	
	//------------------------------ final ok -------------------------------------------------------

	AddWin(NULL, 2000,1990,0,50, 20,0,0,50);
	
	 // [ ok ]   [ cancel ]
	//  TextButton(anxapp *napp,anxwindow *parnt,const char *ntitle,unsigned long nstyle,
	//                        int xx,int yy,int ww,int hh,unsigned int brder,anxwindow *prev,window nowner,atom nsendmes,int nid=0,
	//                        const char *nname=NULL,int npadx=0,int npady=0);
	//  
	last=tbut=new TextButton(this,"ok",ANXWIN_CLICK_FOCUS,0,0,0,0,1, last,window,"Ok", _("Create Project"),TBUT_OK);
	UpdateOkToCreate();
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 20,0,0,50, 5,0,0,50); // add space of 20 pixels
	last=tbut=new TextButton(this,"cancel",ANXWIN_CLICK_FOCUS|TBUT_CANCEL,0,0,0,0,1, last,window,"Cancel");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);


	
	last->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

NewProjectWindow::~NewProjectWindow()
{
}

int NewProjectWindow::DataEvent(EventData *data,const char *mes)
{
	if (!strcmp(mes,"save as")) {
		StrEventData *s=dynamic_cast<StrEventData *>(data);
		if (!s) return 1;
		saveas->SetText(s->str);
		delete data;
		return 0;
	}
	return 1;
}

int NewProjectWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
	DBG cerr <<"newprojmessage: "<<mes<<endl;
	if (!strcmp(mes,"relpath")) { 
		CheckBox *box;
		box=dynamic_cast<CheckBox *>(findChildWindow("relpath"));
		if (box) box->State(LAX_ON);
		box=dynamic_cast<CheckBox *>(findChildWindow("abspath"));
		if (box) box->State(LAX_OFF);
		return 0;
	} else if (!strcmp(mes,"abspath")) { 
		CheckBox *box;
		box=dynamic_cast<CheckBox *>(findChildWindow("relpath"));
		if (box) box->State(LAX_OFF);
		box=dynamic_cast<CheckBox *>(findChildWindow("abspath"));
		if (box) box->State(LAX_ON);
		return 0;
	} else if (!strcmp(mes,"target dpi")) {
	} else if (!strcmp(mes,"target printer")) {
	} else if (!strcmp(mes,"Ok")) {
		if (sendNewProject()) return 0;
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	} else if (!strcmp(mes,"filename")) {
		//*** clicked in filename box, update project dir if checked
	} else if (!strcmp(mes,"usedir")) {
		//*** clicked project dir checkbox
		UpdateOkToCreate();
	} else if (!strcmp(mes,"proj dir")) { //click in lineinput
		//*** clicked in project dir input, must activate checkbox
		//    update filename if filename is default, or filename directory
		UpdateOkToCreate();
	} else if (!strcmp(mes,"projdir")) { // from control button
		app->rundialog(new FileDialog(NULL,_("Save Project In"),
					ANXWIN_REMEMBER|FILES_SAVE_AS, 0,0, 0,0,0,
					window, "save as",saveas->GetCText()));
		return 0;
	}
	return 0;
}

//! Tell laidout to establish a new document.
/*! Return 0 for project established, else nonzero for error.
 */
int NewProjectWindow::sendNewProject()
{
	if (isblank(saveas->GetCText())) return 1;
		
	Project *proj=new Project();

	 //default dpi
	proj->defaultdpi=strtod(((LineInput *)findWindow("dpi"))->GetCText(),NULL);

	 //project file name
	char *fullpath=full_path_for_file(saveas->GetCText(),NULL);
	const char *projfile=lax_basename(fullpath);
	appendstr(fullpath,"/");
	appendstr(fullpath,projfile);
	while (fullpath[strlen(fullpath)-1]=='/') fullpath[strlen(fullpath)-1]='\0';
	appendstr(fullpath,".laidout");
	makestr(proj->filename,fullpath);
	delete[] fullpath;

	 //project name
	makestr(proj->name,((LineInput *)findWindow("name"))->GetCText());

	if (laidout->NewProject(proj,NULL)) { delete proj; return 2; }
	
	return 0;
}

//! Return whether there is enough information to create a project.
/*! If there is no filename, there needs to be a project directory. If there is
 * no directory, and dir is unchecked, there needs to be a filename.
 */
int NewProjectWindow::UpdateOkToCreate()
{
	if (!saveas) return 0;
	int c=!isblank(saveas->GetCText());

	anXWindow *box=findChildWindow("ok");
	if (!box) return 0;
	if (c) box->Grayed(0); else box->Grayed(1);
	return c;
}

