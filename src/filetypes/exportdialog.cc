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


#include <lax/strsliderpopup.h>
#include <lax/simpleprint.h>
#include <lax/mesbar.h>

#include "../laidout.h"
#include "../language.h"
#include "exportdialog.h"


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//---------------------------- ExportDialog -------------------------
/*! \class ExportDialog
 * \brief Dialog to edit an export section. Sends a DocumentExportConfig object.
 *
 * \todo should allow alternate imposition, notably PosterImposition
 */


/*! Controls are added here, rather than waiting for init().
 */
ExportDialog::ExportDialog(unsigned long nstyle,Window nowner,const char *nsend,
						   Document *doc,
						   ExportFilter *nfilter,
						   const char *file, 
						   int layout, //!< Type of layout to export
						   int pmin, //!< The minimum of the range
						   int pmax, //!< The maximum of the range
						   int pcur) //!< The current element of the range
	: RowFrame(NULL,_("Export"),
			   nstyle|ROWFRAME_ROWS|ROWFRAME_VCENTER,
			   0,0,500,300,0,
			   NULL,nowner,nsend, 5)
{
	win_style|=ANXWIN_DELETEABLE;
	//DocumentExportConfig(Document *ndoc, const char *file, const char *to, int l,int s,int e);
	config=new DocumentExportConfig(doc,file,NULL,layout,pmin,pmax);
	filter=nfilter;
	if (!filter && laidout->exportfilters.n) filter=laidout->exportfilters.e[0];

	cur=pcur;
}

ExportDialog::~ExportDialog()
{
	delete config;
}

//! Make sure the kid windows have this as owner.
int ExportDialog::init() 
{
	anXWindow *last=NULL;
	TextButton *tbut=NULL;
	int c;
	int linpheight=app->defaultlaxfont->textheight();

	if (!config->filename) {
		config->filename=newstr("output");
		appendstr(config->filename,".");
		appendstr(config->filename,filter->DefaultExtension());
	}
	
	//	CheckBox(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
	//						int xx,int yy,int ww,int hh,int brder,
	//						anXWindow *prev,Window nowner,const char *nsendmes,
	//						const char *nname=NULL,int npadx=0,int npady=0);
	//	LineEdit(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
	//			int xx,int yy,int ww,int hh,int brder,
	//			anXWindow *prev,Window nowner=None,const char *nsend=NULL,
	//			const char *newtext=NULL,unsigned int ntstyle=0);
	
	

	//------------------------Destination----------
	 
	 //--------- format
	int c2=-1;
	StrSliderPopup *format;
	last=format=new StrSliderPopup(this, "format",0, 0,0,0,0,1, last, window, "format");
	for (c=0; c<laidout->exportfilters.n; c++) {
		format->AddItem(laidout->exportfilters.e[c]->VersionName(),c);
		if (filter==laidout->exportfilters.e[c]) c2=c;
	}
	if (c2>=0) format->Select(c2);
	format->WrapWidth();
	AddWin(format, format->win_w,0,50,50, format->win_h,0,0,50);
	AddNull();

	 //--------- to file
	last=filecheck=new CheckBox(this,"ps-tofile",CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,window,"ps-tofile-check",
						 _("To File: "), 0,5);
	filecheck->State(LAX_ON);
	AddWin(filecheck);

//	 ***** have: [!] _filename_   <-- meaning file exists, tooltip to say what it means
//		         [O]              <-- meaning ok to overwrite
//				 [ ]              <-- meaning does not exist, ok to write to
	last=fileedit=new LineEdit(this,"ps-tofile-le",0, 
						 0,0,100,20, 1,
						 last,window,"ps-tofile-le",
						 config->filename,0);
	fileedit->padx=5;
	AddWin(fileedit, fileedit->win_w,0,1000,50, fileedit->win_h,0,0,50);
	last=tbut=new TextButton(this,"filesaveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"filesaveas",
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();
	
	 //--------- to files
//	 ***** have: [!] _filename_   <-- meaning file exists, tooltip to say what files in range will be overwritten
	last=filescheck=new CheckBox(this,"ps-tofiles",CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,window,"ps-tofiles-check",
						 _("To Files: "), 0,5);
	filescheck->State(LAX_OFF);
	AddWin(filescheck);

	last=filesedit=new LineEdit(this,"ps-tofile-le",0, 
						 0,0,100,20, 1,
						 last,window,"ps-tofile-le",
						 config->tofiles,0);
	filesedit->padx=5;
	AddWin(filesedit, filesedit->win_w,0,1000,50, filesedit->win_h,0,0,50);
	 // a "..." to pop file dialog:
	last=tbut=new TextButton(this,"filessaveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"filessaveas",
			"...",3,3);
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();
	
//   //-------------- by command
//	last=commandcheck=new CheckBox(this,"ps-command",CHECK_CIRCLE|CHECK_LEFT,
//						 0,0,0,0,0, 
//						 last,window,"ps-command-check",
//						 "By Command: ", 0,5);
//	commandcheck->State(LAX_OFF);
//	AddWin(commandcheck);
//
//	last=commandedit=new LineEdit(this,"ps-command-le",0, 
//						 0,0,100,20, 1,
//						 last,window,"ps-command-le",
//						 command,0);
//	commandedit->padx=5;
//	AddWin(commandedit, commandedit->win_w,0,1000,50, commandedit->win_h,0,0,50);
//	AddNull();
	

	 //--- add a vertical spacer
	AddWin(NULL, 0,0,9999,50, 12,0,0,50);
	AddNull();


	 //------------------------What kind of layout----------
	//****doc->docstyle->imposition->Layouts()
	StrSliderPopup *layouts;
	last=layouts=new StrSliderPopup(this, "layouts",0, 0,0,0,0,1, last, window, "layout");
//	for (c=0; c<config->doc->docstyle->imposition->NumLayouts(); c++) {
//		layouts->AddItem(config->doc->docstyle->imposition->LayoutName(c),c);
//		if (filter==laidout->exportfilters.e[c]) c2=c;
//	}
	layouts->AddItem(config->doc->docstyle->imposition->LayoutName(SINGLELAYOUT),SINGLELAYOUT);
	layouts->AddItem(config->doc->docstyle->imposition->LayoutName(PAGELAYOUT),  PAGELAYOUT);
	layouts->AddItem(config->doc->docstyle->imposition->LayoutName(PAPERLAYOUT), PAPERLAYOUT);
	layouts->Select(config->layout);
	layouts->WrapWidth();
	AddWin(layouts, layouts->win_w,0,50,50, format->win_h,0,0,50);
	AddNull();


	 //------------------------Range----------

	 //-------------[ ] All
	 //             [ ] From _____ to ______  <-- need to know their ranges!! and use labels for pages

	last=printall=new CheckBox(this,"ps-printall",CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,window,"ps-printall",
						 _("Export All"), 0,5);
	printall->State(LAX_ON);
	//AddWin(printall, win_w,0,2000,0, printall->win_h,0,0,50);
	AddWin(printall, win_w,0,2000,0, 20,0,0,50);
	AddNull();

	last=printcurrent=new CheckBox(this,"ps-printcurrent",CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,window,"ps-printcurrent",
						 _("Export Current"), 0,5);
	printcurrent->State(LAX_OFF);
	AddWin(printcurrent, win_w,0,2000,0, 20,0,0,50);
	AddNull();

	last=printrange=new CheckBox(this,"ps-printrange",CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,window,"ps-printrange",
						 _("Export From:"), 0,5);
	printrange->State(LAX_OFF);
	AddWin(printrange);

	char blah[15];
	sprintf(blah,"%d",config->start);
	last=printstart=new LineEdit(this,"ps-printstart",0, 
						 0,0,50,20, 1,
						 last,window,"ps-printstart",
						 blah,0);
	printstart->padx=5;
	AddWin(printstart, printstart->win_w,0,1000,50, printstart->win_h,0,0,50);
		
	AddWin(new MessageBar(this,"ps-to",0, 0,0,0,0,0, _("To:")));
	sprintf(blah,"%d",config->end);
	last=printend=new LineEdit(this,"ps-printend",0, 
						 0,0,50,20, 1,
						 last,window,"ps-printend",
						 blah,0);
	printend->padx=5;
	AddWin(printend, printend->win_w,0,1000,50, printend->win_h,0,0,50);
	AddNull();
	AddWin(NULL, 0,0,9999,50, 12,0,0,50);
	AddNull();

	AddWin(NULL, 0,0,1000,50, 0,0,0,50);
	last=tbut=new TextButton(this,"export",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"export",
			_("Export"),3,3);
	AddWin(tbut, tbut->win_w,0,50,50, tbut->win_h,0,0,50);
	last=tbut=new TextButton(this,"cancel",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"cancel",
			_("Cancel"),3,3);
	AddWin(tbut, tbut->win_w,0,50,50, tbut->win_h,0,0,50);
	AddWin(NULL, 0,0,1000,50, 0,0,0,50);
	last->CloseControlLoop();
	
	Sync(1);
	return 0;
}

//! Keep the controls straight.
/*! \todo Changes to printstart and end use the c function atoi()
 * and do not check for things like "1ouaoeao" which it sees as 1.
 */
int ExportDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	int d=1;
	if (!strcmp(mes,"ps-tofile-check")) {
		if (e->data.l[0]==LAX_OFF) {
			 // turn it back on
			filecheck->State(LAX_ON);
			return 0;
		}
		 //else turn off other
		commandcheck->State(LAX_OFF);
		changeTofile(1);
		return 0;
	} else if (!strcmp(mes,"ps-command-check")) {
		if (e->data.l[0]==LAX_OFF) {
			 // turn it back on
			commandcheck->State(LAX_ON);
			return 0;
		}
		 //else turn off other
		filecheck->State(LAX_OFF);
		changeTofile(0);
		return 0;
	} else if (!strcmp(mes,"ps-printall")) {
		printrange->State(LAX_OFF);
		printcurrent->State(LAX_OFF);
		printall->State(LAX_ON);
		return 0;
	} else if (!strcmp(mes,"ps-printcurrent")) {
		printrange->State(LAX_OFF);
		printcurrent->State(LAX_ON);
		printall->State(LAX_OFF);
		return 0;
	} else if (!strcmp(mes,"ps-printrange")) {
		printrange->State(LAX_ON);
		printcurrent->State(LAX_OFF);
		printall->State(LAX_OFF);
		return 0;
	} else if (!strcmp(mes,"ps-printstart")) {
		int a=atoi(printstart->GetCText()),
			b=atoi(printend->GetCText());
		char blah[15];
		if (a<config->start) {
			sprintf(blah,"%d",config->start);
			printstart->SetText(blah);
		} else {
			if (a>config->end) {
				a=b=config->end;
				sprintf(blah,"%d",a);
				printstart->SetText(blah);
				printend->SetText(blah);
			}
			if (a>b) {
				sprintf(blah,"%d",a);
				printend->SetText(blah);
			}
		}
		return 0;
	} else if (!strcmp(mes,"ps-printend")) {
		int a=atoi(printstart->GetCText()),
			b=atoi(printend->GetCText());
		char blah[15];
		if (b>config->end) {
			sprintf(blah,"%d",config->end);
			printend->SetText(blah);
		} else {
			if (b<config->start) {
				a=b=config->start;
				sprintf(blah,"%d",a);
				printstart->SetText(blah);
				printend->SetText(blah);
			}
			if (b<a) {
				sprintf(blah,"%d",b);
				printstart->SetText(blah);
			}
		}
		return 0;
	} else if (!strcmp(mes,"ps-tofile-le")) {
		send();
	} else if (!strcmp(mes,"ps-command-le")) {
		send();
	} else if (!strcmp(mes,"cancel")) {
		//make sure d stays 1
	} else if (e->data.l[1]==TBUT_PRINT) {
		send();
	} else d=0;

	if (d) app->destroywindow(this);
	return 0;
}

//! This allows special things to happen when a different printing target is selected.
void ExportDialog::changeTofile(int t)
{
	tofile=t;
}

//! Run the command, or do to file, shut this box down, and send notification to owner.
/*! ****
 *
 * Otherwise, run the command or copy printthis to the chosen file. WARNING: the
 * command is not currently checked for offensive commands. Whatever the command in
 * the window is, "lp" say, then the command executed is "lp [printthis]".
 *
 * For printthis!=NULL and tofile, copy printthis to the chosen file.
 * 
 * If win_style&SIMPP_DEL_PRINTTHIS then delete printthis after it's done,
 * the calling window must respond to the StrEventData.
 * The file is not copied here to try to enforce an overwrite check....?
 */
int ExportDialog::send()
{
	
	StrEventData *data=new StrEventData;
	data->info=data->info2=data->info3=-1;
	if (win_style&SIMPP_PRINTRANGE) {
		if (printcurrent->State()==LAX_ON) {
			data->info2=data->info3=cur;
		} else if (printall->State()==LAX_ON) {
			data->info2=config->start;
			data->info3=config->end;
		} else {
			data->info2=atoi(printstart->GetCText());
			data->info3=atoi(  printend->GetCText());
		}
	}
	if (tofile) {
		DBG cout << " print to file: \""<<fileedit->GetCText()<<"\""<<endl;

		data->info=1;
		data->str=newstr(fileedit->GetCText());
		app->SendMessage(data,owner,sendthis,window);
		return 0;
	} else {
		DBG cout << " print with: \""<<commandedit->GetCText()<<"\""<<endl;

		data->info=0;
		data->str=newstr(commandedit->GetCText());
		app->SendMessage(data,owner,sendthis,window);
		return 0;
	}
}

//! Character input.
/*! ESC  cancel exporting
 */
int ExportDialog::CharInput(unsigned int ch, unsigned int state)
{
	if (ch==LAX_Esc) {
		app->destroywindow(this);
		return 0;
	}
	return 1;
}

} // namespace Laxkit

