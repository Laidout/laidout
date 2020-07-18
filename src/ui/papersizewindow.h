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
// Copyright (C) 2020 by Tom Lechner
//
#ifndef PAPERSIZEWINDOW_H
#define PAPERSIZEWINDOW_H

#include <lax/rowframe.h>
#include <lax/lineedit.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include <lax/sliderpopup.h>
#include <lax/messagebar.h>

#include "../laidout.h"
#include "../core/papersizes.h"


namespace Laidout {



class PaperSizeWindow : public Laxkit::RowFrame
{
	int custom_index;

	virtual void send();
	const char *pagesDescription(int updatetoo);
	void UpdatePaperName();

 public:
	PaperStyle *papertype;
	int curorientation; //0 for portrait
	int cur_units;
	bool modify_in_place;
	bool with_dpi;
	bool with_color;

	Laxkit::SliderPopup *papernames, *orientation;
	Laxkit::PtrStack<PaperStyle> *papersizes;

	Laxkit::LineInput *paperx,*papery;

 	PaperSizeWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 			unsigned long owner, const char *msg,
			PaperStyle *paper, bool mod_in_place, bool edit_dpi, bool edit_color);
	virtual ~PaperSizeWindow();
	virtual const char *whattype() { return "PaperSizeWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	void UpdatePaper(int dialogtoimp);
};

} //namespace Laidout

#endif

