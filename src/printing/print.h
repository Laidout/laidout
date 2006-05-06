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

#ifndef PRINT_H
#define PRINT_H

#include <lax/simpleprint.h>
#include "../document.h"

class PrintingDialog : public Laxkit::SimplePrint
{
 protected:
	Laxkit::LineEdit *filesedit;
	Laxkit::CheckBox *filescheck;
	virtual void changeTofile(int t);
	int curpage;
	Document *doc;
 public:
	PrintingDialog(Document *ndoc,Window nowner,const char *nsend,
						 const char *file="output.ps", const char *command="lp",
						 const char *thisfile=NULL,
						 int ntof=1,int pmin=-1,int pmax=-1,int pcur=-1);
	virtual ~PrintingDialog() { }
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int Print();
};



#endif

