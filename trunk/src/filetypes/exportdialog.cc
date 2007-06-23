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


#include <cups/cups.h>

#include <lax/strsliderpopup.h>
#include <lax/simpleprint.h>
#include <lax/mesbar.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>

#include "../laidout.h"
#include "../language.h"
#include "exportdialog.h"


#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


//---------------------------- ConfigEventData -------------------------
/*! \class ConfigEventData
 * \brief Class to transport a DocumentExportConfig.
 */


/*! Incs count on config.
 */
ConfigEventData::ConfigEventData(DocumentExportConfig *c)
{
	config=c;
	if (config) config->inc_count();
}

/*! Decs count on config.
 */
ConfigEventData::~ConfigEventData()
{
	if (config) config->dec_count();
}

//---------------------------- ExportDialog -------------------------
/*! \class ExportDialog
 * \brief Dialog to edit an export section. Sends a DocumentExportConfig object.
 *
 * \todo should allow alternate imposition, notably PosterImposition
 * \todo allow remember settings, and when popped up, bring up last one accessed..
 * \todo overwrite protection
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
			   (nstyle&ANXWIN_MASK)|ROWFRAME_ROWS|ROWFRAME_VCENTER,
			   0,0,500,300,0,
			   NULL,nowner,nsend, 5)
{
	win_style|=ANXWIN_DELETEABLE;
	dialog_style=nstyle&~ANXWIN_MASK;

	//DocumentExportConfig(Document *ndoc, const char *file, const char *to, int l,int s,int e);
	config=new DocumentExportConfig(doc,file,NULL,layout,pmin,pmax);
	filter=nfilter;
	if (!filter && laidout->exportfilters.n) filter=laidout->exportfilters.e[0];

	cur=pcur;


	fileedit=filesedit=printstart=printend=command=NULL;
	filecheck=filescheck=commandcheck=printall=printcurrent=printrange=NULL;
}

/*! Decs count of config.
 */
ExportDialog::~ExportDialog()
{
	if (config) config->dec_count();
}

//! Based on config->layout, set min and max accordingly.
void ExportDialog::findMinMax()
{
	min=0;
	max=config->doc->docstyle->imposition->NumSpreads(config->layout)-1;
}

//! Make sure the kid windows have this as owner.
int ExportDialog::init() 
{
	findMinMax();

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
	format->tooltip(_("The file format to export into"));
	AddWin(format, format->win_w,0,50,50, format->win_h,0,0,50);
	AddNull();

	 //--------- to command
	if (dialog_style&EXPORT_COMMAND) {
		last=commandcheck=new CheckBox(this,"command-check",CHECK_CIRCLE|CHECK_LEFT, 
							 0,0,0,0,0, 
							 last,window,"command-check",
							 _("By Command: "), 0,5);
		commandcheck->State(LAX_ON);
		commandcheck->tooltip(_("Run this command on a single exported file"));
		AddWin(commandcheck);

		last=command=new LineEdit(this,"command",LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF, 
							 0,0,100,20, 1,
							 last,window,"command",
							 "lp",0);
		command->padx=5;
		command->tooltip(_("Run this command on a single exported file"));
		AddWin(command, command->win_w,0,1000,50, command->win_h,0,0,50);
		AddNull();
	}
	
	 //--------- to file
	last=filecheck=new CheckBox(this,"tofile-check",CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,window,"tofile-check",
						 _("To File: "), 0,5);
	filecheck->State(LAX_ON);
	filecheck->tooltip(_("Export to this file"));
	AddWin(filecheck);

//	 ***** have: [!] _filename_   <-- meaning file exists, tooltip to say what it means
//		         [O]              <-- meaning ok to overwrite
//				 [ ]              <-- meaning does not exist, ok to write to
	last=fileedit=new LineEdit(this,"tofile",
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_ANY_CHANGE, 
						 0,0,100,20, 1,
						 last,window,"tofile",
						 config->filename,0);
	fileedit->padx=5;
	fileedit->tooltip(_("Export to this file"));
	AddWin(fileedit, fileedit->win_w,0,1000,50, fileedit->win_h,0,0,50);
	last=tbut=new TextButton(this,"filesaveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"filesaveas",
			"...",3,3);
	tbut->tooltip(_("Browse for a new location"));
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();
	
	 //--------- to files
//	 ***** have: [!] _filename_   <-- meaning file exists, tooltip to say what files in range will be overwritten
	last=filescheck=new CheckBox(this,"tofiles",CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,window,"tofiles-check",
						 _("To Files: "), 0,5);
	filescheck->State(LAX_OFF);
	filescheck->tooltip(_("Export to these files. A '#' is replaced with\n"
						  "the spread index. A \"###\" for an index like 3\n"
						  "will get replaced with \"003\"."));
	AddWin(filescheck);

	last=filesedit=new LineEdit(this,"tofiles",
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_ANY_CHANGE, 
						 0,0,100,20, 1,
						 last,window,"tofiles",
						 config->tofiles,0);
	filesedit->padx=5;
	filesedit->tooltip(_("Export to these files. A '#' is replaced with\n"
						  "the spread index. A \"###\" for an index like 3\n"
						  "will get replaced with \"003\"."));
	AddWin(filesedit, filesedit->win_w,0,1000,50, filesedit->win_h,0,0,50);
	 // a "..." to pop file dialog:
	last=tbut=new TextButton(this,"filessaveas",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			last,window,"filessaveas",
			"...",3,3);
	tbut->tooltip(_("Browse for a new location"));
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50);
	AddNull();
	

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
	layouts->tooltip(_("The type of spreads to export"));
	AddWin(layouts, layouts->win_w,0,50,50, layouts->win_h,0,0,50);
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
	last=printstart=new LineEdit(this,"start",
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF, 
						 0,0,50,20, 1,
						 last,window,"start",
						 blah,0);
	printstart->padx=5;
	printstart->tooltip(_("The starting index"));
	AddWin(printstart, printstart->win_w,0,1000,50, printstart->win_h,0,0,50);
		
	AddWin(new MessageBar(this,"end",0, 0,0,0,0,0, _("To:")));
	sprintf(blah,"%d",config->end);
	last=printend=new LineEdit(this,"end",LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF, 
						 0,0,50,20, 1,
						 last,window,"end",
						 blah,0);
	printend->padx=5;
	printend->tooltip(_("The ending index"));
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
	
	updateExt();

	win_h=0;
	m[1]=m[7]=BOX_SHOULD_WRAP;
	Sync(1);
	Resize(m[0],m[6]);

	overwriteCheck();
	return 0;
}

//! Make sure the range is valid, and re-set the edits if necessary.
/*! Also sets the values in config based on the values in the edits.
 */
void ExportDialog::configBounds()
{
	int a=start(),
		b=end();

	if (a<min) a=min;
	else if (a>max) a=max;
	if (b<min) b=min;
	else if (b>max) b=max;

	config->start=a;
	config->end  =b;

	start(a);
	end(b);
}

//! Set the start of the range in config and edit based on index s.
/*! \todo *** just does index right now!! must use page labels
 */
void ExportDialog::start(int s)
{
	printstart->SetText(s);
}

//! Set the end of the range in config and edit based on index s.
/*! \todo *** just does index right now!! must use page labels
 */
void ExportDialog::end(int e)
{
	printend->SetText(e);
}

//! Return the start of the range as an index, not a name.
/*! \todo *** just does index right now!! must use page labels
 */
int ExportDialog::start()
{
	int e;
	int a=printstart->GetLong(&e);
	if (e) return 0;
	return a;
}

//! Return the end of the range as an index, not a name.
/*! \todo *** just does index right now!! must use page labels
 */
int ExportDialog::end()
{
	int e;
	int a=printend->GetLong(&e);
	if (e) return 0;
	return a;
}

//! Update extensions in file edits.
int ExportDialog::updateExt()
{
	 //do file
	char *s=fileedit->GetText();
	char *p=strrchr(s,'.'), *b=strrchr(s,'/');
	if (p) {
		if (!b || b && p>b) {
			*p='\0';
			appendstr(s,".");
			appendstr(s,filter->DefaultExtension());
			fileedit->SetText(s);
			makestr(config->filename,s);
			delete[] s;
		}
	}
	 //do files
	s=filesedit->GetText();
	p=strrchr(s,'.');
	b=strrchr(s,'/');
	if (p) {
		if (!b || b && p>b) {
			*p='\0';
			appendstr(s,".");
			appendstr(s,filter->DefaultExtension());
			filesedit->SetText(s);
			makestr(config->tofiles,s);
			delete[] s;
		}
	}
	return 0;
}

//! Keep the controls straight.
/*! 
 * \todo ability to use page/layout names like iii, instead of index numbers
 */
int ExportDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"format")) {
		filter=laidout->exportfilters.e[e->data.l[0]];
		findMinMax();
		configBounds();
		updateExt();
		return 0;
	} else if (!strcmp(mes,"command-check")) {
		changeTofile(3);
		return 0;
	} else if (!strcmp(mes,"tofile-check")) {
		changeTofile(1);
		return 0;
	} else if (!strcmp(mes,"tofiles-check")) {
		changeTofile(2);
		return 0;
	} else if (!strcmp(mes,"ps-printall")) {
		changeRangeTarget(1);
		return 0;
	} else if (!strcmp(mes,"ps-printcurrent")) {
		changeRangeTarget(2);
		return 0;
	} else if (!strcmp(mes,"ps-printrange")) {
		changeRangeTarget(3);
		return 0;
	} else if (!strcmp(mes,"start")) {
		DBG cerr <<"start data: "<<e->data.l[0]<<endl;
		if (e->data.l[0]==2) {
			changeRangeTarget(3);
		} else {
			configBounds();
		}
		return 0;
	} else if (!strcmp(mes,"end")) {
		DBG cerr <<"end data: "<<e->data.l[0]<<endl;
		if (e->data.l[0]==2) {
			 //focus on
			changeRangeTarget(3);
		} else {
			configBounds();
		}
		return 0;
	} else if (!strcmp(mes,"tofiles")) {
		if (e->data.l[0]==1) {
			send();
			return 0;
		} else if (e->data.l[0]==2) {
			 //focus on
			changeTofile(2);
			return 0;
		} else if (e->data.l[0]==3) {
			 //focus off
			makestr(config->tofiles,filesedit->GetCText());
			return 0;
		} else if (e->data.l[0]==0) {
			makestr(config->tofiles,filesedit->GetCText());
			overwriteCheck();
			return 0;
		}
	} else if (!strcmp(mes,"tofile")) {
		if (e->data.l[0]==1) {
			send();
			return 0;
		} else if (e->data.l[0]==2) {
			 //focus on
			changeTofile(1);
			return 0;
		} else if (e->data.l[0]==3) {
			 //focus off
			makestr(config->filename,fileedit->GetCText());
			return 0;
		} else if (e->data.l[0]==0) {
			makestr(config->filename,fileedit->GetCText());
			overwriteCheck();
			return 0;
		}
	} else if (!strcmp(mes,"command")) {
		if (e->data.l[0]==1) { 
			send();
			return 0; 
		} else if (e->data.l[0]==2) {
			 //focus on
			changeTofile(3);
			return 0;
		}
		return 0;
	} else if (!strcmp(mes,"cancel")) {
		app->destroywindow(this);
		return 0;
	} else if (!strcmp(mes,"export")) {
		send();
		return 0;
	} else if (!strcmp(mes,"filesaveas")) {
		app->rundialog(new FileDialog(NULL,"get new file",FILES_OPEN_ONE|ANXWIN_DELETEABLE,
									  0,0,400,500,0,window,"get new file",
									  fileedit->GetCText()));
		return 0;
	} else if (!strcmp(mes,"filessaveas")) {
		app->rundialog(new FileDialog(NULL,"get new file",FILES_OPEN_ONE|ANXWIN_DELETEABLE,
									  0,0,400,500,0,window,"get new files",
									  filesedit->GetCText()));
		return 0;
	}

	return 0;
}

void ExportDialog::overwriteCheck()
{
	DBG cerr <<"-----overwrite check "<<endl;

	int valid,err;
	unsigned long color=app->rgbcolor(255,255,255);

	if (filecheck->State()==LAX_ON) {
		 //else check file
		if (isblank(fileedit->GetCText())) valid=1;
		else valid=file_exists(fileedit->GetCText(),1,&err);
		if (valid) {
			if (valid!=S_IFREG) { // exists, but is not regular file
				if (valid!=1) color=app->rgbcolor(255,100,100);
				fileedit->tooltip(_("Cannot overwrite this kind of file!"));
				findChildWindow("export")->Grayed(1);
			} else { // was existing regular file
				color=app->rgbcolor(255,255,0);
				fileedit->tooltip(_("WARNING: This file will be overwritten on export!"));
				findChildWindow("export")->Grayed(0);
			}
		} else {
			fileedit->tooltip(_("Export to this file"));
			findChildWindow("export")->Grayed(0);
		}
		fileedit->Valid(!valid,color);
	} else if (filescheck->State()==LAX_ON) {
		if (isblank(filesedit->GetCText())) {
			filesedit->tooltip(_("Cannot write to nothing!"));
			findChildWindow("export")->Grayed(1);
			filesedit->Valid(0,color);
			return;
		}

		char *filebase;
		int e=0,w=0; //errors and warnings
		filebase=LaxFiles::make_filename_base(config->tofiles);
		char file[strlen(filebase)+10]; //*** someone could trick this with more than "##" blocks maybe!! check!
		for (int c=config->start; c<=config->end; c++) {
			sprintf(file,filebase,c);

			valid=file_exists(file,1,&err);
			if (valid) {
				if (valid!=S_IFREG) { // exists, but is not regular file
					e++;
				} else { // was existing regular file
					w++;
				}
			}
		}
		if (e) {
			color=app->rgbcolor(255,100,100);
			filesedit->tooltip(_("Some files cannot be overwritten!"));
			findChildWindow("export")->Grayed(1);
		} else if (w) {
			color=app->rgbcolor(255,255,0);
			filesedit->tooltip(_("Warning: Some files will be overwritten!"));
			findChildWindow("export")->Grayed(0);
		} else {
			 //note: this must be same tip as in init().
			filesedit->tooltip(_("Export to these files. A '#' is replaced with\n"
					  "the spread index. A \"###\" for an index like 3\n"
					  "will get replaced with \"003\"."));
			findChildWindow("export")->Grayed(0);
		}
		filesedit->Valid(!valid,color);
	}
}

int ExportDialog::DataEvent(EventData *data,const char *mes)
{
	if (!strcmp(mes,"get new file")) {
		StrEventData *s=dynamic_cast<StrEventData *>(data);
		if (!s) return 1;
		fileedit->SetText(s->str);
		delete data;
		return 0;
	} else if (!strcmp(mes,"get new files")) {
		StrEventData *s=dynamic_cast<StrEventData *>(data);
		if (!s) return 1;
		filesedit->SetText(s->str);
		delete data;
		return 0;
	}
	return 1;
}


//! This allows special things to happen when a different printing target is selected.
/*! Updates the target checkboxes.
 *
 * 1=to file,
 * 2=to files,
 * 3=by command.
 *
 * \todo maybe check against range, and whether the filter supports multipage
 */
void ExportDialog::changeTofile(int t)
{
	tofile=t;

	filecheck-> State(tofile==1?LAX_ON:LAX_OFF);
	filescheck->State(tofile==2?LAX_ON:LAX_OFF);
	if (commandcheck) commandcheck->State(tofile==3?LAX_ON:LAX_OFF);

	overwriteCheck();
}

//! This allows special things to happen when a different range target is selected.
/*! Updates the range checkboxes.
 *
 * 1=all,
 * 2=current.
 * 3=from __ to __
 *
 * \todo update range indicator when current or all selected
 */
void ExportDialog::changeRangeTarget(int t)
{
	printrange->State(t==3?LAX_ON:LAX_OFF);
	printcurrent->State(t==2?LAX_ON:LAX_OFF);
	printall->State(t==1?LAX_ON:LAX_OFF);
}

//! Send the config object to owner.
/*! Note that a new config object is sent, and the receiving code should delete it.
 */
int ExportDialog::send()
{
	if (findChildWindow("export")->Grayed()) return 0;

	config->filter=filter;
	if (commandcheck && commandcheck->State()==LAX_ON) {
		//----------**** clean this up or move it back to ViewWindow!!
		char *cm=newstr(command->GetCText());
		appendstr(cm," ");
		//***investigate tmpfile() tmpnam tempnam mktemp
		
		char tmp[256];
		cupsTempFile2(tmp,sizeof(tmp));
		DBG cerr <<"attempting to write temp file for printing: "<<tmp<<endl;

		FILE *f=fopen(tmp,"w");
		if (f) {
			fclose(f);

			//mesbar->SetText(_("Printing, please wait...."));
			//mesbar->Refresh();
			//XSync(app->dpy,False);

			char *error;
			if (filter->Out(tmp,config,&error)==1) {
				appendstr(cm,tmp);

				 //now do the actual command
				int c=system(cm); //-1 for error, else the return value of the call
				if (c!=0) {
					DBG cerr <<"there was an error printing...."<<endl;
				}
				//*** have to delete (unlink) tmp!
				
				//mesbar->SetText(_("Document sent to print."));
			} else {
				//there was an error during filter export
			}
		}
		//---------
		app->destroywindow(this);
		return 0;
	}


	ConfigEventData *data=new ConfigEventData(config);
	app->SendMessage(data,owner,sendthis,window);
	app->destroywindow(this);
	return 1;
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


