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
#include <lax/filedialog.h>
#include <lax/tabframe.h>
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

//--------------------------------- BrandNew() ------------------------------------

//! Return a TabFrame with a NewDocWindow and NewProjectWindow.
anXWindow *BrandNew()
{
	TabFrame *tf=new TabFrame(NULL,_("New"),ANXWIN_REMEMBER|BOXSEL_ROWS
					|BOXSEL_LEFT|BOXSEL_TOP|BOXSEL_ONE_ONLY|BOXSEL_ROWS|STRICON_STR_ICON,
								0,0,500,500,0,
								NULL,None,NULL,
								50);
	tf->AddWin(new NewDocWindow(tf,"New Document",ANXWIN_LOCAL_ACTIVE,0,0,0,0, 0),
				_("New Document"),
				NULL);
	tf->AddWin(new NewProjectWindow(tf,"New Project",ANXWIN_LOCAL_ACTIVE,0,0,0,0, 0),
				_("New Project"),
				NULL);

	FileDialog *fd=new FileDialog(tf,"open doc",
					ANXWIN_REMEMBER|FILES_NO_CANCEL|FILES_OPEN_ONE, 0,0, 0,0,0,
					None, "open doc", NULL,NULL,NULL, "Laidout");
	fd->OkButton(_("Open Document"),NULL);
	tf->AddWin(fd,
				_("Open"),
				NULL);


//	char *tempdir=newstr(laidout->config_dir);
//	appendstr(tempdir,"/templates");
//	tf->AddWin(new FileDialog(tf,"open template",
//					ANXWIN_REMEMBER|FILES_NO_CANCEL|FILES_OPEN_ONE, 0,0, 0,0,0,
//					None, "template",NULL,tempdir),
//				_("Open Template"),
//				NULL);
//	delete[] tempdir;
//	tf->AddWin(NULL,
//				_("Recent"),
//				NULL);
	return tf;
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
	marginl=_("Left:");
	marginr=_("Right:");
	margint=_("Top:");
	marginb=_("Bottom:");

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
	saveas=new LineInput(this,"save as",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"save as",
			            _("Save As:"),NULL,0,
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
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak

	 // ------Tiling
	last=tiley=new LineInput(this,"y tiling",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"ytile",
			            _("Tile y:"),"1", 0,
			           100,0,1,1,3,3);
	tiley->tooltip("Only for Single, Double, and Booklet");
	AddWin(tiley, tiley->win_w,0,50,50, linpheight,0,0,50);
	last=tilex=new LineInput(this,"x tiling",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"xtile",
			            _("Tile x:"),"1", 0,
			           100,0,1,1,3,3);
	tilex->tooltip(_("Only for Single, Double, and Booklet"));
	AddWin(tilex, tilex->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak

	
	 // -----Number of pages
	last=numpages=new LineInput(this,"numpages",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						last,window,"numpages",
			            _("Number of pages:"),"1",0, // *** must do auto set papersize
			            100,0,1,1,3,3);
	AddWin(numpages, numpages->win_w,0,50,50, linpheight,0,0,50);
	
	
	 // ------------- Imposition ------------------
	mesbar=new MessageBar(this,"mesbar 1.1",MB_MOVE, 0,0, 0,0, 0, _("Imposition:"));
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	last=impsel=new StrSliderPopup(this,"Imposition",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
						numpages,window,"imposition");
	for (c=0; c<laidout->impositionpool.n; c++)
		impsel->AddItem(laidout->impositionpool.e[c]->Stylename(),c);
	AddWin(impsel, 250,100,50,50, linpheight,0,0,50);
	AddNull();
	

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
	
	 // ------------------- view mode ---------------------------
	AddWin(new MessageBar(this,"view style",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, _("view:")));
	MenuSelector *msel;
	msel=new MenuSelector(this,"view style",ANXWIN_CLICK_FOCUS,
						0,0,0,0,0,
						last,window,"view style",
						MENUSEL_CHECKBOXES|MENUSEL_LEFT|MENUSEL_CURSSELECTS|MENUSEL_ONE_ONLY);
	msel->AddItem(_("single"),1,0);
	msel->AddItem(_("page layout"),2,0);
	msel->AddItem(_("paper layout"),3,0);
	AddWin(msel, 100,0,50,50, (textheight+5)*3,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,50);//*** forced linebreak
		
	 // ------------------- printing misc ---------------------------
	 // target dpi:		__300____
	linp=new LineInput(this,"dpi",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						msel,window,"dpi",
			            _("target dpi:"),"360",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // default unit: __inch___
	linp=new LineInput(this,"unit",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						linp,window,"unit",
			            _("default unit:"),"inch",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // color mode:		black and white, grayscale, rgb, cmyk, other
	linp=new LineInput(this,"colormode",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						linp,window,"colormode",
			            _("color mode:"),"rgb",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	AddWin(new MessageBar(this,"colormes",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, "paper color:"));
	ColorBox *cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, linp,window,"paper color", 255,255,255);
	AddWin(cbox, 40,0,50,50, linpheight,0,0,50);

	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // target printer: ___whatever____ (file, pdf, html, png, select ppd
	linp=new LineInput(this,"printer",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						cbox,window,"printer",
			            _("target printer:"),"default (cups)",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	 //   [ set options from ppd... ]
	tbut=new TextButton(this,"setfromppd",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, linp,window,"setfromppd",
			_("set options from ppd..."),3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak



	 // --------------------- page specific setup ------------------------------------------
	
	//***first page is page number: 	___1_

	 // ------------------ margins ------------------
	linp=new LineInput(this,"margin t",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						tbut,window,"margin t",
			            margint,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	linp=new LineInput(this,"margin b",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						linp,window,"margin b",
			            marginb,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	linp=new LineInput(this,"margin l",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						linp,window,"margin l",
			            marginl,NULL,0,
			            0,0,3,0,3,3);
	AddWin(linp, 150,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,50);//*** forced linebreak
	
	linp=new LineInput(this,"margin r",ANXWIN_CLICK_FOCUS|LINP_ONLEFT,
			            5,250,0,0, 0, 
						linp,window,"margin r",
			            marginr,NULL,0,
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
		//***
		if (e->data.l[0]<0 || e->data.l[0]>=laidout->impositionpool.n) return 0;
		if (imp) delete imp;
		imp=(Imposition *)laidout->impositionpool.e[e->data.l[0]]->duplicate();
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
	} else if (!strcmp(mes,"saveas")) { // from control button
		//***get defaults
		app->rundialog(new FileDialog(NULL,_("Save As"),
					ANXWIN_REMEMBER|FILES_SAVE_AS, 0,0, 0,0,0,
					saveas->window, "save as","untitled"));
		return 0;
	} else if (!strcmp(mes,"Ok")) {
		sendNewDoc();
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	}
	return 0;
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
	 
	last=new LineInput(this,"projdir",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"proj dir",
			            _("Project Directory:"),NULL,0,
			            0,0,1,0,3,3);
	AddWin(last, 300,0,2000,50, linpheight,0,0,50);
	last=tbut=new TextButton(this,"saveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"projdir",
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();
	

	

	 // -------------- absolute or relative paths --------------------
	
	CheckBox *check=NULL;
	last=check=new CheckBox(this,"abspath",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"abspath", _("Use absolute file paths in saved files"),5,5);
	check->State(LAX_ON);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"relpath",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"relpath", _("Use relative file paths in saved files"),5,5);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	AddNull();

	
		
	 // ------------------- printing misc ---------------------------
	 // target dpi:		__300____
	last=linp=new LineInput(this,"dpi",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"dpi",
			            _("target dpi:"),"360",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // default unit: __inch___
	last=linp=new LineInput(this,"unit",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last=linp,window,"unit",
			            _("default unit:"),"inch",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // color mode:		black and white, grayscale, rgb, cmyk, other
	last=linp=new LineInput(this,"colormode",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"colormode",
			            _("color mode:"),"rgb",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	AddWin(new MessageBar(this,"colormes",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, "paper color:"));
	ColorBox *cbox;
	last=cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, last,window,"paper color", 255,255,255);
	AddWin(cbox, 40,0,50,50, linpheight,0,0,50);

	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // target printer: ___whatever____ (file, pdf, html, png, select ppd
	last=linp=new LineInput(this,"printer",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						last,window,"printer",
			            _("target printer:"),"default (cups)",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	 //   [ set options from ppd... ]
	last=tbut=new TextButton(this,"setfromppd",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, last,window,"setfromppd",
			_("set options from ppd..."),3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak


	
	//------------------------------ final ok -------------------------------------------------------

	AddWin(NULL, 2000,1990,0,50, 20,0,0,50);
	
	 // [ ok ]   [ cancel ]
	//  TextButton(anxapp *napp,anxwindow *parnt,const char *ntitle,unsigned long nstyle,
	//                        int xx,int yy,int ww,int hh,unsigned int brder,anxwindow *prev,window nowner,atom nsendmes,int nid=0,
	//                        const char *nname=NULL,int npadx=0,int npady=0);
	//  
	last=tbut=new TextButton(this,"ok",ANXWIN_CLICK_FOCUS,0,0,0,0,1, last,window,"Ok", _("Create Project"),TBUT_OK);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 20,0,0,50, 5,0,0,50); // add space of 20 pixels
	last=tbut=new TextButton(this,"cancel",ANXWIN_CLICK_FOCUS|TBUT_CANCEL,0,0,0,0,1, last,window,"Cancel");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);


	
	tbut->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

NewProjectWindow::~NewProjectWindow()
{
}

int NewProjectWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
	DBG cerr <<"newprojmessage: "<<mes<<endl;
	if (!strcmp(mes,"paper size")) {
	} else if (!strcmp(mes,"ytile")) { 
		DBG cerr <<"**** newdoc: new y tile value"<<endl;
	} else if (!strcmp(mes,"xtile")) { 
		DBG cerr <<"**** newdoc: new x tile value"<<endl;
	} else if (!strcmp(mes,"paper name")) { 
	} else if (!strcmp(mes,"target dpi")) {
	} else if (!strcmp(mes,"target printer")) {
	} else if (!strcmp(mes,"pagesetup proc")) {
	} else if (!strcmp(mes,"paper layout")) {
	} else if (!strcmp(mes,"save as")) {
	} else if (!strcmp(mes,"projdir")) { // from control button
		//***get defaults
		app->rundialog(new FileDialog(NULL,_("Save Project In"),
					ANXWIN_REMEMBER|FILES_SAVE_AS, 0,0, 0,0,0,
					window, "save as","untitled"));
		return 0;
	} else if (!strcmp(mes,"Ok")) {
		sendNewProject();
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
	}
	return 0;
}

//! Create and fill a DocumentStyle, and tell laidout to make a new document
void NewProjectWindow::sendNewProject()
{
//	 // find and get dup of imposition
//	Imposition *imposition=NULL;
//	int c;
//	for (c=0; c<laidout->impositionpool.n; c++) {
//		if (!strcmp(laidout->impositionpool.e[c]->Stylename(),impsel->GetCurrentItem())) break;
//	}
//	if (c==laidout->impositionpool.n) imposition=new Singles();
//	else {
//		DBG cerr <<"****attempting to clone "<<(laidout->impositionpool.e[c]->Stylename())<<endl;
//		imposition=(Imposition *)(laidout->impositionpool.e[c]->duplicate());
//	}
//	if (!imposition) { cout <<"**** no imposition in newdoc!!"<<endl; return; }
//	
//	int npgs=atoi(numpages->GetCText()),
//		xtile=atoi(tilex->GetCText()),
//		ytile=atoi(tiley->GetCText());
//	if (npgs<=0) npgs=1;
//	if (xtile<=0) xtile=1;
//	if (ytile<=0) ytile=1;
//
//	Singles *s=dynamic_cast<Singles *>(imposition);
//	if (s) {
//		s->tilex=xtile;
//		s->tiley=ytile;
//	}
//		
//	imposition->NumPages(npgs);
//	DocumentStyle *newdoc=new DocumentStyle(imposition);
//	newdoc->imposition->SetPaperSize(papertype);
//	laidout->NewDocument(newdoc,saveas->GetCText());
}
