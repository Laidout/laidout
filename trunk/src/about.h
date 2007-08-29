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
#ifndef ABOUT_H
#define ABOUT_H

#include <lax/messagebox.h>

class AboutWindow : public Laxkit::MessageBox
{
 public:
 	AboutWindow();
	virtual ~AboutWindow() {}
 	virtual const char *whattype() { return "AboutWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int CharInput(unsigned int ch,unsigned int state);
};

#endif

