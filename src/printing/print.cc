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
//

#include "../language.h"
#include "../laidout.h"
#include "print.h"

using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

//---------------------------- PrintingDialog ----------------------------------

/*! \class PrintingDialog
 * \brief Laidout's printing dialog
 *
 * \todo this needs a lot of work
 *
 * Ideally would be like:
 * <pre>
 * Format_______svg_______[v]
 * To File________________
 * To Files_______________
 * By Command_____________
 *
 * Layout Type___Singles__[v]
 * [ ] All
 * [*] Current
 * [ ] From ____ to ______
 *
 * Settings: Custom [v]   --> is edit to allow change name
 *    pops a menu:
 *      settings 1
 *      pdf all
 *      [Remember current settings...] 
 * </pre>
 *
 * The format would be:
 *    a name,
 *    whether the format supports multiple pages,
 *      if no and to file selected, can only select one page, not from==to,
 *    default file name, files template, command
 */


/*! Install options to print to eps files.
 */
PrintingDialog::PrintingDialog(Document *ndoc,Window nowner,const char *nsend,
						 const char *file, const char *command,
						 const char *thisfile,
						 int layout,int pmin,int pmax,int pcur,
						 Laxkit::MessageBar *progress)
	: ExportDialog(ANXWIN_DELETEABLE|ANXWIN_CENTER|EXPORT_COMMAND,nowner,nsend,
				   ndoc,NULL,file,
				   layout,pmin,pmax,pcur)
{
	for (int c=0; c<laidout->exportfilters.n; c++)
		if (!strcmp(laidout->exportfilters.e[c]->VersionName(),_("Postscript LL3"))) {
			filter=laidout->exportfilters.e[c];
			break;
		}
}

PrintingDialog::~PrintingDialog()
{}

int PrintingDialog::init()
{
	ExportDialog::init();
	changeTofile(3);

	
	TextButton *print=dynamic_cast<TextButton *>(findChildWindow("export"));
	print->SetName(_("Print"));

	return 0;

	//fileedit->tooltip("Output papers to a single postscript file");
	//filecheck->tooltip("Output papers to a single postscript file");
	//commandedit->tooltip("Process a single postscript file\nof the papers with this command");
	//commandcheck->tooltip("Process a single postscript file\nof the papers with this command");
	//
	//filescheck->tooltip("Output individual pages to several EPS files");
	//filesedit->tooltip("Output several EPS files");
}

//! Keep the controls straight.
/*! \todo Changes to printstart and end use the c function atoi()
 * and do not check for things like "1ouaoeao" which it sees as 1.
 */
int PrintingDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	return ExportDialog::ClientEvent(e,mes);
}

//! Do things different when tofile=2.
int PrintingDialog::Print()
{
//	if (tofile!=2) return SimplePrint::Print();	
//
//	StrEventData *data=new StrEventData;
//	data->info=data->info2=data->info3=-1;
//	if (win_style&SIMPP_PRINTRANGE) {
//		if (printcurrent->State()==LAX_ON) {
//			data->info2=data->info3=cur;
//		} else if (printall->State()==LAX_ON) {
//			data->info2=min;
//			data->info3=max;
//		} else {
//			data->info2=atoi(printstart->GetCText());
//			data->info3=atoi(  printend->GetCText());
//		}
//	}
//	DBG cout << " print to file: \""<<filesedit->GetCText()<<"\""<<endl;
//	data->info=2;
//	data->str=newstr(filesedit->GetCText());
//	app->SendMessage(data,owner,sendthis,window);
	return 0;
}


//	} else if (!strcmp(mes,"reallyprintfile")) {
//		***
//		 // print to file without overwrite check 
//		 // *** hopping around with messages is not a
//		 // good overwrite protection
//		StrEventData *s=dynamic_cast<StrEventData *>(data);
//		if (!s) return 1;
//		
//		if (!is_good_filename(s->str)) {
//			GetMesbar()->SetText(_("Illegal characters in file name. Not printed."));
//			delete data;
//			return 0;
//		} 
//		
//		FILE *f=fopen(s->str,"w");
//		if (f) {
//			mesbar->SetText(_("Printing to file, please wait...."));
//			mesbar->Refresh();
//			XSync(app->dpy,False);
//	
//			psout(f,doc,s->info2-1,s->info3-1,0,NULL);
//			fclose(f);
//			
//			char tmp[21+strlen(s->str)];
//			sprintf(tmp,_("Printed to %s."),s->str);
//			mesbar->SetText(tmp);
//		} else {
//			char tmp[21+strlen(s->str)];
//			sprintf(tmp,_("Error printing to %s."),s->str);
//			mesbar->SetText(tmp);
//		}
//
//		DBG cout << "----- ViewWindow Print to file: "<<s->str<<endl;
//		delete data;
//		return 0;
//	} else if (!strcmp(mes,"printfile")) {
//		***** this needs to be rewritten in light of filter mechanism
//			
//		StrEventData *s=dynamic_cast<StrEventData *>(data);
//		if (!s) return 1;
//		if (s->info==2) { // print to files
//			DBG cout <<"***** print to epss: "<<s->str<<endl;
//			
//			mesbar->SetText(_("Printing to files, please wait...."));
//			mesbar->Refresh();
//			XSync(app->dpy,False);
//	
//			char blah[100];
//			int c=epsout(s->str,doc,s->info2-1,s->info3-1,SINGLELAYOUT,0,NULL);
//			if (c) {
//				sprintf(blah,_("Error printing to %s at file %d."),s->str,c);
//			} else {
//				sprintf(blah,_("Printed to %s."),s->str);
//			}
//			mesbar->SetText(blah);
//
//			delete data;
//			return 0;
//		} else if (s->info==1) { // print to file
//			 // overwrite protection
//			int c,err;
//			c=file_exists(s->str,1,&err);
//			if (c && c!=S_IFREG) {
//				 // has to be a regular file to overwrite
//				mesbar->SetText("Cannot overwrite that type of file.");
//				delete data;
//				return 0;
//			}
//			 // file existed, so ask to overwrite
//			if (c) {
//				anXWindow *ob=new Overwrite(window,"reallyprintfile", s->str, s->info, s->info2, s->info3);
//				app->rundialog(ob);
//				s->str=NULL;
//				delete data;
//				return 0;
//			}
//			 // else really print
//			DataEvent(s,"reallyprintfile");
//			return 0;
//		} else { // print by command
//			char *cm=newstr(s->str);
//			appendstr(cm," ");
//			//***investigate tmpfile() tmpnam tempnam mktemp
//			char tmp[256];
//			cupsTempFile2(tmp,sizeof(tmp));
//			FILE *f=fopen(tmp,"w");
//			if (f) {
//				mesbar->SetText(_("Printing, please wait...."));
//				mesbar->Refresh();
//				XSync(app->dpy,False);
//				psout(f,doc,s->info2-1,s->info3-1,0,NULL);
//				fclose(f);
//				appendstr(cm,tmp);
//				system(cm);
//				//*** have to delete (unlink) tmp!
//				
//				mesbar->SetText(_("Document sent to print."));
//			} else mesbar->SetText(_("Error printing."));
//			DBG cout << "*** ViewWindow Printed to command: "<<cm<<endl;
//		}
//
//
//		delete data;
//		return 0;
