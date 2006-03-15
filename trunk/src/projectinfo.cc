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
/********* laidout/projectinfo.cc **************/

//TODO
//
// add goodeditww for info, and menuselector(...) for files
//
//
//
//Project Info
//------- ----
//Project Name:          ____Kick Ass Whammy_____________
//Project Path:          ________/blah/project.laidout___
//General Import Directory:
//Image Import Directory:
//Default Export Directory:
//Font Path:             ___/usr/X11R6/lib/X11/fonts .___
//
//Project files: 
//  ADD..
//  f1.lay
//  f2.lay
//  f3.lay
//  blah.py
//
//Info:
// [				]
// [				]
// [				]
//
////Copyright:   *** this is future version stuff, right now,
////Author:      *** just plop it all in info
////             ***
////Add Field... ***
//
//-------------------------------------------------------------------



#include "projectinfo.h"
	

class ProjectInfo
{	
	char *projectname;
	char *projectdir;
	char *exportdir;
	char *fontpath;
	char *imagedir;
	char *info;
	char **files;
	ProjectInfo() { projectdir=exportdir=fontpath=imagedir=projectname=info=NULL; file=NULL; }
	~ProjectInfo();
};

ProjectInfo::~ProjectInfo()
{
	delete[] projectname;
	delete[] projectdir;
	delete[] exportdir;
	delete[] fontpath;
	delete[] imagedir;
	delete[] info;
	for (int c=0; files[c]; c++) {
		delete[] files[c];
	}
	delete[] files;
}

//------------------------------------------------------------------------

//class ProjectInfoWindow : public RowFrame
//{
// public:
//	ProjectInfo *project;
//	
//	LineEdit *lineedit;
//	LineInput *paperx,*papery;
//	MessageBar *mesbar;
//	CheckBox *defaultpage,*custompage;
// 	ProjectInfoWindow(anXApp *napp,anXWindow *parnt,const char *ntitle,unsigned long nstyle,int xx,int yy,int ww,int hh,unsigned int border);
//	~ProjectInfoWindow();
//	int init();
////	int Refresh();
////	virtual int CharInput(unsigned int ch,unsigned int state);
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//};

ProjectInfoWindow::ProjectInfoWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,int xx,int yy,int ww,int hh,unsigned int border)
		: RowFrame(parnt,ntitle,nstyle|ROWFRAME_HORIZONTAL|ROWFRAME_CENTER,xx,yy,ww,hh,border,10)
{
	project=NULL;
}

int ProjectInfoWindow::init()
{
	if (!window) return 1;

	
	int textheight=app->defaultfont->max_bounds.ascent+app->defaultfont->max_bounds.descent;
	int linpheight=textheight+12;
	TextButton *tbut;


//	//AddWin(lineedit, w,ws,wg,h,valign); for horizontal rows
//	LineInput::LineInput(anXApp *napp,anXWindow *parnt,const char *ntitle,unsigned int nstyle,
//			            int xx,int yy,int ww,int hh,unsigned int bordr,
//			            aControl *prev,Window nowner,const char *atom,int nid,
//			            const char *newlabel,const char *newtext,unsigned int ntstyle,
//			            int nlw,nlh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0

	
	 // ------ General Directory Setup ---------------
	 
	 //------- Project Name
	LineInput *linp;
	linp=new LineInput(app,this,"project name",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						NULL,window,"project name",0,
			            "Project Name:",NULL,0,
			            0,0,1,0,3,3);
	AddWin(linp, 300,0,2000, linpheight,0);
	AddNull();
	
	 //--------- Export Dir
	linp=new LineInput(app,this,"export dir",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						tbut,window,"export dir",0,
					    "Export Directory:",NULL,0,
					    0,0,1,0,3,3);
	AddWin(linp, 2000,1950,0, linpheight,0);
	tbut=new TextButton(app,this,"exportdir",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			linp,window,"exportdir",0,
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50, linpheight,0);
	AddNull();
	
	 //------------ Import Dir
	linp=new LineInput(app,this,"import dir",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						tbut,window,"import dir",0,
					    "General Import Directory:",NULL,0,
					    0,0,1,0,3,3);
	AddWin(linp, 2000,1950,0, linpheight,0);
	tbut=new TextButton(app,this,"importdir",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			linp,window,"importdir",0,
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50, linpheight,0);
	AddNull();
	
	 //----------- Image dir
	linp=new LineInput(app,this,"image dir",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						tbut,window,"image dir",0,
			            "Image Import Directory:",NULL,0,
			            0,0,1,0,3,3);
	AddWin(linp, 2000,1950,0, linpheight,0);
	tbut=new TextButton(app,this,"imagedir",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			linp,window,"imagedir",0,
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50, linpheight,0);
	AddNull();
	
	 //-------------- Font Path
	linp=new LineInput(app,this,"font path",ANXWIN_CLICK_FOCUS|LINP_ONLEFT, 0,0,0,0, 1, 
						tbut,window,"font path",0,
			            "Font Path:",NULL,0,
			            0,0,1,0,3,3);
	AddWin(linp, 2000,1950,0, linpheight,0);
	tbut=new TextButton(app,this,"fontpath",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			linp,window,"fontpath",0,
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50, linpheight,0);
	AddNull();
	
	 //---------- Info
	//*** info
	//AddWin(new MessageBar(app,this,"Info",ANXWIN_CLICK_FOCUS|MB_MOVE, 0,0,0,0,0, "Info:"));
	//gww=new GoodEditWW(app,this,"info",ANXWIN_CLICK_FOCUS, 0,0,0,0,1);
	
	 //-------------- Project Files
	//*** files
	MenuSelector *msel=new MenuSelector(app,this,"Files",ANXWIN_CLICK_FOCUS,
						0,0,0,0,0,
						tbut,window,"files",0,
						MENUSEL_CHECKBOXES | MENUSEL_LEFT|MENUSEL_CURSSELECTS|MENUSEL_ONE_ONLY 
					);
	msel->AddItem("ADD...");
	for (int c=0; c<nfiles; c++) {***
		msel->AddItem(files[c],c);
	}
	AddWin(msel, 200,100,50, linpheight*4,0);
	AddNull();//*** forced linebreak

	
	
	//------------------------------ final ok -------------------------------------------------------

	 // [ OK ]   [ Cancel ]
	//  TextButton(anXApp *napp,anXWindow *parnt,const char *ntitle,unsigned long nstyle,
	//                        int xx,int yy,int ww,int hh,unsigned int brder,aControl *prev,Window nowner,Atom nsendmes,int nid=0,
	//                        const char *nname=NULL,int npadx=0,int npady=0);
	//  
	tbut=new TextButton(app,this,"ok",ANXWIN_CLICK_FOCUS|TBUT_OK,0,0,0,0,1, msel,window,"Ok");
	AddWin(tbut, tbut->win_w,0,50, linpheight,0);
	AddWin(NULL, 20,0,0, 5,0); // add space of 20 pixels
	tbut=new TextButton(app,this,"cancel",ANXWIN_CLICK_FOCUS|TBUT_CANCEL,0,0,0,0,1, tbut,window,"Cancel");
	AddWin(tbut, tbut->win_w,0,50, linpheight,0);


	
	tbut->CloseControlLoop();
	Sync(1);
//	WrapExtent();
}

ProjectInfoWindow::~ProjectInfoWindow()
{
}

	char *projectname;
	char *projectdir;
	char *exportdir;
	char *fontpath;
	char *imagedir;
	char *info;
	char **files;
int ProjectInfoWindow::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
cout <<"NewDocMessage: "<<mes<<endl;
	if (!strcmp(mes,"project name")) {
	} else if (!strcmp(mes,"projectname")) { 
	} else if (!strcmp(mes,"project dir")) {
	} else if (!strcmp(mes,"projectdir")) {
	} else if (!strcmp(mes,"export dir")) {
	} else if (!strcmp(mes,"exportdir")) {
	} else if (!strcmp(mes,"font path")) {
	} else if (!strcmp(mes,"fontpath")) {
	} else if (!strcmp(mes,"image dir")) {
	} else if (!strcmp(mes,"imagedir")) {
	} else if (!strcmp(mes,"info")) {
	} else if (!strcmp(mes,"files")) {
	} else if (!strcmp(mes,"Ok")) {
//		***create new doc based on all the settings here
//		app->SendData(whereto***,newdocstruct);
		app->destroywindow(this);
	} else if (!strcmp(mes,"Cancel")) {
		app->destroywindow(this);
	}
	return 0;
}

