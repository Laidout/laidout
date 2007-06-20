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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <lax/lineedit.h>
#include <lax/checkbox.h>
#include <lax/messagebox.h>

#include "filefilters.h"

namespace Laxkit {

#define SIMPP_SEND_TO_PRINTER  (1<<16)
#define SIMPP_DEL_PRINTTHIS    (1<<17)
#define SIMPP_PRINTRANGE       (1<<18)
	
class ExportDialog : public Laxkit::RowFrame
{
 protected:
	DocumentExportConfig *config;
	
	int tofile, cur;
	LineEdit *fileedit,*filesedit,*commandedit,*printstart,*printend;
	CheckBox *filecheck,*filescheck,*commandcheck;
	CheckBox *printall,*printcurrent,*printrange;
	virtual void changeTofile(int t);
	ExportFilter *filter;
	virtual int send();
 public:
	ExportDialog(unsigned long nstyle,Window nowner,const char *nsend,
				 Document *doc, ExportFilter *nfilter, const char *file, int layout,
				 int pmin, int pmax, int pcur);
	virtual ~ExportDialog();
	virtual int init();
	virtual int CharInput(unsigned int ch, unsigned int state);
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
};

} // namespace Laxkit


#endif

