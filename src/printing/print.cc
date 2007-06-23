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
 * \brief Laidout's printing dialog.
 *
 * This is in contrast to ExportDialog. This dialog is intended for sending postscript
 * to a printer. Eventually, it'll allow selecting known printers, via Cups, for instance.
 * Currently, only "print by command" really sends anything to the printer.
 *
 * \todo this needs a lot of work
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

/*! Rename the "Export" button to "Print".
 */
int PrintingDialog::init()
{
	ExportDialog::init();
	changeTofile(3);

	
	TextButton *print=dynamic_cast<TextButton *>(findChildWindow("export"));
	print->SetName(_("Print"));

	return 0;
}

//! Keep the controls straight.
int PrintingDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	return ExportDialog::ClientEvent(e,mes);
}



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
//			DBG cerr << "*** ViewWindow Printed to command: "<<cm<<endl;
//		}
//
//
//		delete data;
//		return 0;
