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
#ifndef FINDWINDOW_H
#define FINDWINDOW_H

#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include <lax/sliderpopup.h>
#include <lax/messagebar.h>

#include "../laidout.h"
#include "../core/papersizes.h"


namespace Laidout {



class FindWindow : public Laxkit::RowFrame
{
	LaxInterfaces::Selection found;

	Laxkit::MessageBar *howmany;
	Laxkit::LineInput *pattern;
	Laxkit::IconSelector *founditems;

	bool regex;
	bool caseless;
	int where;

	enum FindThings {
		FIND_Anywhere = 1,
		FIND_InView = 2,
		FIND_InSelection = 3,
		FIND_MAX
	};
	FindThings where;

	virtual void send(bool also_delete);
	virtual void UpdateFound();

	virtual void SearchNext();
	virtual void SearchPrevious();
	virtual void SearchAll();

 public:

 	FindWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 			unsigned long owner, const char *msg);
	virtual ~FindWindow();
	virtual const char *whattype() { return "FindWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);
};

} //namespace Laidout

#endif

