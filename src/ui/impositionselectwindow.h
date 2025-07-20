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
// Copyright (C) 2025 by Tom Lechner
//
#ifndef IMPOSITIONSELECTWINDOW_H
#define IMPOSITIONSELECTWINDOW_H

// #include <lax/rowframe.h>
// #include <lax/lineedit.h>
// #include <lax/lineinput.h>
// #include <lax/checkbox.h>
#include <lax/interfaces/interfacewindow.h>
#include <lax/interfaces/gridselectinterface.h>

// #include "../laidout.h"
// #include "../core/papersizes.h"
// #include "papersizewindow.h"

#include "../impositions/imposition.h"

namespace Laidout {

class ImpositionSelectWindow : public LaxInterfaces::InterfaceWindow
{
	LaxInterfaces::GridSelectInterface *ginterface;

	// virtual void sendNewDoc();
	// const char *pagesDescription(int updatetoo);

  public:
  	int icon_size = 75;

	Imposition *imp = nullptr;
	Document *doc = nullptr;
	PaperStyle *papertype = nullptr;

	// Laxkit::PtrStack<PaperStyle> *papersizes;
	// Laxkit::SliderPopup *impsel;
	// Laxkit::LineEdit *lineedit;
	// PaperSizeWindow *psizewindow;
	// Laxkit::LineInput *saveas,*numpages,*impfromfile;
	// Laxkit::MessageBar *impmesbar, *pageinfo;
	// Laxkit::CheckBox *defaultpage,*custompage;

	ImpositionSelectWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle, unsigned long owner, const char *msg);
	virtual ~ImpositionSelectWindow();
	virtual const char *whattype() { return "ImpositionSelectWindow"; }
	// virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	bool InstallDefaultList();
	void ColorsFromTheme();
	int UseThisImposition(Imposition *imp);
	bool Select(int id);
};

} // namespace Laidout

#endif
