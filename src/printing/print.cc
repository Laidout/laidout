//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
//

#include "print.h"

using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

//---------------------------- PrintingDialog ----------------------------------

/*! \class Print
 * \brief Laidout's printing dialog
 */
//class PrintingDialog : public Laxkit::SimplePrint
//{
// protected:
//	Laxkit::LineEdit *filesedit;
//	Laxkit::CheckBox *filescheck;
//	virtual void changeTofile(int t);
//	int curpage;
//	Document *doc;
// public:
//	PrintingDialog(Document *ndoc,Window nowner,const char *nsend,
//						 const char *file="output.ps", const char *command="lp",
//						 const char *thisfile=NULL,
//						 int ntof=1,int pmin=-1,int pmax=-1,int pcur=-1);
//	virtual ~PrintingDialog() { }
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//	virtual int Print();
//};

/*! Install options to print to eps files.
 */
PrintingDialog::PrintingDialog(Document *ndoc,Window nowner,const char *nsend,
						 const char *file, const char *command,
						 const char *thisfile,
						 int ntof,int pmin,int pmax,int pcur)
	: SimplePrint(ANXWIN_DELETEABLE|ANXWIN_CENTER|SIMPP_PRINTRANGE,
		nowner,nsend,file,command,thisfile,ntof,pmin,pmax,pcur)
{
	doc=ndoc;

	fileedit->tooltip("Output papers to a single postscript file");
	filecheck->tooltip("Output papers to a single postscript file");
	commandedit->tooltip("Process a single postscript file\nof the papers with this command");
	commandcheck->tooltip("Process a single postscript file\nof the papers with this command");
	
	curpage=pcur;
	cur=doc->docstyle->imposition->PaperFromPage(cur);
	
	int c;
	WinFrameBox *wbox;
	for (c=0; c<wholelist.n; c++) {
		wbox=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
		if (!wbox || !wbox->win || !wbox->win->win_title) continue;
		if (!strcmp(wbox->win->win_title,"ps-command")) break;
	}

	anXWindow *last=wbox->win->prevcontrol;
		
	last=filescheck=new CheckBox(this,"ps-tofiles",CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,window,"ps-tofiles-check",
						 "To Files: ", 0,5);
	filescheck->State(LAX_OFF);
	filescheck->tooltip("Output individual pages to several EPS files");
	AddWin(filescheck,c++);

	//	LineEdit(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
	//			int xx,int yy,int ww,int hh,int brder,
	//			anXWindow *prev,Window nowner=None,const char *nsend=NULL,
	//			const char *newtext=NULL,unsigned int ntstyle=0);
	last=filesedit=new LineEdit(this,"ps-tofiles-le",0, 
						 0,0,100,20, 1,
						 last,window,"ps-tofiles-le",
						 "output###.eps",0);
	filesedit->tooltip("Output several EPS files");
	filesedit->padx=5;
	AddWin(filesedit, filesedit->win_w,0,1000,50, filesedit->win_h,0,0,50, c++);
	AddNull(c++);
}


//! Keep the controls straight.
/*! \todo Changes to printstart and end use the c function atoi()
 * and do not check for things like "1ouaoeao" which it sees as 1.
 */
int PrintingDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	SimplePrint::ClientEvent(e,mes);
	
	if (!strcmp(mes,"ps-tofile-check")) {
		if (e->data.l[0]==LAX_ON) filescheck->State(LAX_OFF);
	} else if (!strcmp(mes,"ps-command-check")) {
		if (e->data.l[0]==LAX_ON) filescheck->State(LAX_OFF);
	} else if (!strcmp(mes,"ps-tofiles-check")) {
		if (e->data.l[0]==LAX_OFF) {
			 // turn it back on
			filescheck->State(LAX_ON);
			return 0;
		}
		 //else turn off other
		filecheck->State(LAX_OFF);
		commandcheck->State(LAX_OFF);
		changeTofile(2);
		return 0;
	}
	
	return 0;
}

//! Adjust min, max, and cur based on printing pages or papers.
void PrintingDialog::changeTofile(int t)
{
	if (t==tofile) return;

	tofile=t;
	if (tofile==2) {
		max=doc->pages.n;
		cur=curpage;
	} else {
		max=doc->docstyle->imposition->numpages;
		cur=doc->docstyle->imposition->PaperFromPage(cur);
	}
	
	int a=atoi(printstart->GetCText()),
		b=atoi(printend->GetCText());
	char blah[15];
	if (b>max) {
		sprintf(blah,"%d",max);
		printend->SetText(blah);
	}
	if (a>max) {
		sprintf(blah,"%d",max);
		printstart->SetText(blah);
	}
}

//! Do things different when tofile=2.
int PrintingDialog::Print()
{
	if (tofile!=2) return SimplePrint::Print();	

	StrEventData *data=new StrEventData;
	data->info=data->info2=data->info3=-1;
	if (win_style&SIMPP_PRINTRANGE) {
		if (printcurrent->State()==LAX_ON) {
			data->info2=data->info3=cur;
		} else if (printall->State()==LAX_ON) {
			data->info2=min;
			data->info3=max;
		} else {
			data->info2=atoi(printstart->GetCText());
			data->info3=atoi(  printend->GetCText());
		}
	}
	DBG cout << " print to file: \""<<filesedit->GetCText()<<"\""<<endl;
	data->info=2;
	data->str=newstr(filesedit->GetCText());
	app->SendMessage(data,owner,sendthis,window);
	return 0;
}
