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
#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <lax/messagebox.h>

class HelpWindow : public Laxkit::MessageBox
{
 public:
 	HelpWindow();
	virtual ~HelpWindow() {}
 	virtual const char *whattype() { return "HelpWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int CharInput(unsigned int ch,unsigned int state);
};

#endif

