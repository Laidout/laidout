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

#ifndef PRINT_H
#define PRINT_H

#include "../filetypes/exportdialog.h"
#include "../document.h"
#include <lax/mesbar.h>

class PrintingDialog : public ExportDialog
{
 protected:
 public:
	PrintingDialog(Document *ndoc,Window nowner,const char *nsend,
						 const char *file, const char *command,
						 const char *thisfile,
						 int layout,int pmin,int pmax,int pcur,
						 Laxkit::MessageBar *progress);
	virtual ~PrintingDialog();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int Print();
	virtual int init();
};



#endif

