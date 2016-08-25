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
// Copyright (C) 2016 by Tom Lechner
//
#ifndef AUTOSAVEWINDOW_H
#define AUTOSAVEWINDOW_H


#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>


namespace Laidout {


class AutosaveWindow : public Laxkit::RowFrame
{
	Laxkit::LineEdit *projectdir,*projectfile;
	Laxkit::CheckBox *useprojectdir;

  public:
 	AutosaveWindow(Laxkit::anXWindow *parnt);
	virtual ~AutosaveWindow();
	virtual const char *whattype() { return "AutosaveWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual int send();
};


} //namespace Laidout


#endif

