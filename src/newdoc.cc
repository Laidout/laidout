//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/********* laidout/newdoc.cc **************/

//TODO
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


#include "newdoc.h"
#include "dispositioninst.h"
#include <lax/filedialog.h>
	
using namespace Laxkit;


/*! \class NewDocWindow
 *
 * This is currently a god awful hack to get things off the ground.
 * Must be restructured to handle any kind of disposition... 
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
//class NewDocWindow : public RowFrame
//{
//	int mx,my;
//	virtual void sendNewDoc();
// public:
//	int curorientation;
//	// the names of each, so to change Left->Inside, Top->Inside (like calender), etc
//	const char *marginl,*marginr,*margint,*marginb; 
//	Disposition *disp;
//	PaperType *papertype;
//	
//	Laxkit::StrSliderPopup *dispsel;
//	Laxkit::LineEdit *lineedit;
//	Laxkit::LineInput *saveas,*paperx,*papery,*numpages;
//	Laxkit::MessageBar *mesbar;
//	Laxkit::CheckBox *defaultpage,*custompage;
// 	NewDocWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
// 		int xx,int yy,int ww,int hh,int brder);
//	virtual ~NewDocWindow();
//	virtual int init();
////	int Refresh();
////	virtual int CharInput(char ch,unsigned int state);
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//};

NewDocWindow::NewDocWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
							int xx,int yy,int ww,int hh,int brder)
		: RowFrame(parnt,ntitle,nstyle|ROWFRAME_HORIZONTAL|ROWFRAME_CENTER,xx,yy,ww,hh,brder,10)
{
	marginl="Left:";
	marginr="Right:";
	margint="Top:";
	marginb="Bottom:";

	curorientation=0;
	papersizes=NULL;
	numpages=NULL;

	disp=NULL;
	papertype=NULL;
}

int NewDocWindow::init()
{
	if (!window) return 1;

	
	int textheight=app->defaultfont->max_bounds.ascent+app->defaultfont->max_bounds.descent;
	int linpheight=textheight+12;
	TextButton *tbut;
	MenuSelector *msel;


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
	LineInput *linp;
	saveas=new LineInput(this,"save as",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"save as",
			            "Save As:",NULL,0,
			            0,0,1,0,3,3);
	AddWin(saveas, 300,0,2000,50, linpheight,0,0,50);
	tbut=new TextButton(this,"saveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			saveas,window,"saveas",
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();//*** forced linebreak
	

	 // -------------- Paper Size --------------------
	
	papersizes=&laidout->papersizes;
	papertype=(PaperType *)papersizes->e[0]->duplicate();
	char blah[100],blah2[100];
	o=papertype->flags;
	 // -----Paper Size X
	sprintf(blah,"%.10g", papertype->w());
	sprintf(blah2,"%.10g",papertype->h());
	paperx=new LineInput(this,"paper x",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						tbut,window,"paper x",
			            "Paper Size  x:",(o&1?blah2:blah),0,
			            100,0,1,1,3,3);
	AddWin(paperx, paperx->win_w,0,50,50, linpheight,0,0,50);
	
	 // -----Paper Size Y
	papery=new LineInput(this,"paper y",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						paperx,window,"paper y",
			            "y:",(o&1?blah:blah2),0,
			           100,0,1,1,3,3);
	AddWin(papery, papery->win_w,0,50,50, linpheight,0,0,50);
	
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
	popup=new StrSliderPopup(this,"paperOrientation",ANXWIN_CLICK_FOCUS, 0,0, 0,0, 1, popup,window,"orientation");
	popup->AddItem("Portrait",0);
	popup->AddItem("Landscape",0);
	popup->Select(o&1?1:0);
	AddWin(popup, 200,100,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak

	 // -----Number of pages
	numpages=new LineInput(this,"numpages",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 0, 
						popup,window,"numpages",
			            "Number of pages:","10",0, // *** must do auto set papersize
			            100,0,1,1,3,3);
	AddWin(numpages, numpages->win_w,0,50,50, linpheight,0,0,50);
	
	
	 // ------------- Disposition ------------------
	mesbar=new MessageBar(this,"mesbar 1.1",MB_MOVE, 0,0, 0,0, 0, "Disposition:");
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	dispsel=new StrSliderPopup(this,"Disposition",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
						numpages,window,"disposition");
	for (c=0; c<laidout->dispositionpool.n; c++)
		dispsel->AddItem(laidout->dispositionpool.e[c]->Stylename(),c);
	AddWin(dispsel, 250,100,50,50, linpheight,0,0,50);
	AddNull();
	

	 // -------------- page size --------------------
	
	mesbar=new MessageBar(this,"mesbar 2",ANXWIN_HOVER_FOCUS|MB_MOVE, 0,0, 0,0, 0, "pagesize:");
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);

	defaultpage=new CheckBox(this,"default",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						dispsel,window,"check default", "default",5,5);
	defaultpage->SetState(LAX_ON);
	AddWin(defaultpage, defaultpage->win_w,0,0,50, linpheight,0,0,50);
	
	custompage=new CheckBox(this,"custom",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						defaultpage,window,"check custom", "custom",5,5);
	AddWin(custompage, custompage->win_w,0,0,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak

	linp=new LineInput(this,"page x",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						custompage,window,"page x",
			            "x:",NULL,0,
			            100,0,1,1,3,3);
	AddWin(linp, 120,0,50,50, linpheight,0,0,50);
	
	linp=new LineInput(this,"page y",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						linp,window,"page y",
			            "y:",NULL,0,
			            100,0,1,1,3,3);
	AddWin(linp, 120,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // ------------------- view mode ---------------------------
	AddWin(new MessageBar(this,"view style",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, "view:"));
	msel=new MenuSelector(this,"view style",ANXWIN_CLICK_FOCUS,
						0,0,0,0,0,
						linp,window,"view style",
						MENUSEL_CHECKBOXES|MENUSEL_LEFT|MENUSEL_CURSSELECTS|MENUSEL_ONE_ONLY);
	msel->AddItem("single",1,0);
	msel->AddItem("page layout",2,0);
	msel->AddItem("paper layout",3,0);
	AddWin(msel, 100,0,50,50, (textheight+5)*3,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,50);//*** forced linebreak
		
	 // ------------------- printing misc ---------------------------
	 // target dpi:		__300____
	linp=new LineInput(this,"dpi",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						msel,window,"dpi",
			            "target dpi:","360",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // default unit: __inch___
	linp=new LineInput(this,"unit",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						linp,window,"unit",
			            "default unit:","inch",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // color mode:		black and white, grayscale, rgb, cmyk, other
	linp=new LineInput(this,"colormode",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						linp,window,"colormode",
			            "color mode:","rgb",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	AddWin(new MessageBar(this,"colormes",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, "paper color:"));
	ColorBox *cbox=new ColorBox(this,"paper color",COLORBOX_DRAW_NUMBER, 0,0,0,0, 1, linp,window,"paper color", 255,255,255);
	cbox->tooltip("left button: red\nmiddle button: green\nright button: blue");
	AddWin(cbox, 40,0,50,50, linpheight,0,0,50);

	AddWin(NULL, 2000,2000,0,50, 0,0,0,0);//*** forced linebreak
	
	 // target printer: ___whatever____ (file, pdf, html, png, select ppd
	linp=new LineInput(this,"printer",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 5,250,0,0, 0, 
						cbox,window,"printer",
			            "target printer:","default (cups)",0,
			            0,0,1,1,3,3);
	AddWin(linp, linp->win_w,0,50,50, linpheight,0,0,50);

	 //   [ set options from ppd... ]
	tbut=new TextButton(this,"setfromppd",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, linp,window,"setfromppd",
			"set options from ppd...",3,3);
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

	 // [ ok ]   [ cancel ]
	//  TextButton(anxapp *napp,anxwindow *parnt,const char *ntitle,unsigned long nstyle,
	//                        int xx,int yy,int ww,int hh,unsigned int brder,anxwindow *prev,window nowner,atom nsendmes,int nid=0,
	//                        const char *nname=NULL,int npadx=0,int npady=0);
	//  
	tbut=new TextButton(this,"ok",ANXWIN_CLICK_FOCUS|TBUT_OK,0,0,0,0,1, linp,window,"Ok");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddWin(NULL, 20,0,0,50, 5,0,0,50); // add space of 20 pixels
	tbut=new TextButton(this,"cancel",ANXWIN_CLICK_FOCUS|TBUT_CANCEL,0,0,0,0,1, tbut,window,"Cancel");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);


	
	tbut->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}

NewDocWindow::~NewDocWindow()
{
	if (disp) delete disp;
	delete papertype;
}

int NewDocWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
cout <<"newdocmessage: "<<mes<<endl;
	if (!strcmp(mes,"paper size")) {
	} else if (!strcmp(mes,"paper name")) { 
		 // new paper selected from the popup, so must find the x/y and set x/y appropriately
		int i=e->data.l[0];
cout <<"new paper size:"<<i<<endl;
		if (i<0 || i>=papersizes->n) return 0;
		delete papertype;
		papertype=(PaperType *)papersizes->e[i]->duplicate();
		if (!strcmp(papertype->name,"custom")) return 0;
		papertype->flags=curorientation;
		char num[30];
		numtostr(num,30,papertype->w(),0);
		papery->SetText(num);
		numtostr(num,30,papertype->h(),0);
		paperx->SetText(num);
		//*** would also reset page size x/y based on Layout Scheme, and if page is Default
	} else if (!strcmp(mes,"orientation")) {
		int l=(int)e->data.l[0];
cout <<"New orientation:"<<l<<endl;
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
	} else if (!strcmp(mes,"disposition")) {
		//***
		if (e->data.l[0]<0 || e->data.l[0]>=laidout->dispositionpool.n) return 0;
		if (disp) delete disp;
		disp=(Disposition *)laidout->dispositionpool.e[e->data.l[0]]->duplicate();
		return 0;
		
	} else if (!strcmp(mes,"papersizex")) {
	} else if (!strcmp(mes,"papersizey")) {
	} else if (!strcmp(mes,"pagesizex")) {
	} else if (!strcmp(mes,"pagesizey")) {
	} else if (!strcmp(mes,"check custom")) {
		if (custompage->GetState()==LAX_OFF) custompage->SetState(LAX_ON);
		defaultpage->SetState(LAX_OFF);
	} else if (!strcmp(mes,"check default")) {
		if (defaultpage->GetState()==LAX_OFF) defaultpage->SetState(LAX_ON);
		custompage->SetState(LAX_OFF);
		//***gray out the x/y inputs for page size? or auto convert to custom when typing there?
	} else if (!strcmp(mes,"target dpi")) {
	} else if (!strcmp(mes,"target printer")) {
	} else if (!strcmp(mes,"pagesetup proc")) {
	} else if (!strcmp(mes,"paper layout")) {
	} else if (!strcmp(mes,"save as")) {
	} else if (!strcmp(mes,"saveas")) { // from control button
		//***get defaults
		app->rundialog(new FileDialog(NULL,"Save As",FILES_SAVE_AS, 0,0, 400,400,0,
					saveas->window, "save as","untitled"));
		return 0;
	} else if (!strcmp(mes,"Ok")) {
		sendNewDoc();
		app->destroywindow(this);
	} else if (!strcmp(mes,"Cancel")) {
		app->destroywindow(this);
	}
	return 0;
}

//! Create and fill a DocumentStyle, and tell laidout to make a new document
void NewDocWindow::sendNewDoc()
{
	 // find and get dup of disposition
	Disposition *disposition=NULL;
	int c;
	for (c=0; c<laidout->dispositionpool.n; c++) {
		if (!strcmp(laidout->dispositionpool.e[c]->Stylename(),dispsel->GetCurrentItem())) break;
	}
	if (c==laidout->dispositionpool.n) disposition=new Singles();
	else {
		cout <<"****attempting to clone "<<(laidout->dispositionpool.e[c]->Stylename())<<endl;
		disposition=(Disposition *)(laidout->dispositionpool.e[c]->duplicate());
	}
	if (!disposition) { cout <<"**** no disposition in newdoc!!"<<endl; return; }
	
	int npgs=atoi(numpages->GetCText());
	if (npgs<=0) npgs=1;
	disposition->NumPages(npgs);
	DocumentStyle *newdoc=new DocumentStyle(disposition);
	newdoc->disposition->SetPaperSize(papertype);
	laidout->NewDocument(newdoc,saveas->GetCText());
}

