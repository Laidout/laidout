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
// Copyright (C) 2004-2012,2016 by Tom Lechner
//


#ifndef PLAINTEXTWINDOW_H
#define PLAINTEXTWINDOW_H

#include "project.h"
#include "document.h"
#include "plaintext.h"
#include <lax/rowframe.h>


namespace Laidout {


//------------------------------ PlainTextWindow -------------------------------

class PlainTextWindow : public Laxkit::RowFrame
{
  protected:
	PlainText *textobj;
	int syncText(int filetoo);
	void uniqueName(PlainText *obj);

  public:
 	PlainTextWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder,
		PlainText *newtext);
 	virtual const char *whattype() { return "PlainTextWindow"; }
	virtual ~PlainTextWindow();
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int UseThis(PlainText *txt);
	virtual int init();
	virtual void updateControls();
	virtual void callSaveAs();

	 //for i/o
    virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

};


} // namespace Laidout

#endif

