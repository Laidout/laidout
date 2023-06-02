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

#ifndef PRINT_H
#define PRINT_H

#include "../filetypes/exportdialog.h"
#include "../core/document.h"
#include <lax/messagebar.h>


namespace Laidout {


class PrintingDialog : public ExportDialog
{
 protected:
 public:
	PrintingDialog(Document *ndoc,unsigned long nowner,const char *nsend,
						 const char *file, const char *command,
						 const char *thisfile,
						 int layout,int pmin,int pmax,int pcur, int cur_page,
						 PaperGroup *group,
						 Group *limbo,
						 Laxkit::MessageBar *progress);
	virtual ~PrintingDialog();
	virtual const char *whattype() { return "PrintingDialog"; }
	virtual int init();
};


} // namespace Laidout

#endif

