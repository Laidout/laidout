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



namespace Laidout {

//---------------------------- PrintingDialog ----------------------------------

/*! \class PrintingDialog
 * \brief Laidout's printing dialog.
 *
 * This is in contrast to ExportDialog. This dialog is intended for sending a PDF file
 * to a printer. It is supposed to allow selecting a printer destination, via Cups.
 *
 * \todo this needs a lot of work
 */


PrintingDialog::PrintingDialog(Document *ndoc,Window nowner,const char *nsend,
						 const char *file, const char *command,
						 const char *thisfile,
						 int layout,int pmin,int pmax,int pcur,
						 PaperGroup *group,
						 Group *limbo,
						 Laxkit::MessageBar *progress)
	: ExportDialog(EXPORT_COMMAND,nowner,nsend,
				   ndoc,limbo,group,NULL,file,
				   layout,pmin,pmax,pcur)
{
	for (int c=0; c<laidout->exportfilters.n; c++)
		if (!strcasecmp(laidout->exportfilters.e[c]->VersionName(), _("Pdf"))) { //Postscript LL3"))) {
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

	WindowTitle(_("Print"));
	Button *print=dynamic_cast<Button *>(findChildWindowByName("export"));
	print->Label(_("Print"));

	return 0;
}


} // namespace Laidout

