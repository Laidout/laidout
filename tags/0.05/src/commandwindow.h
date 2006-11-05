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

#ifndef COMMANDWINDOW_H
#define COMMANDWINDOW_H

#include <lax/promptedit.h>

class CommandWindow : public Laxkit::PromptEdit
{
 protected:
	char *dir;
	virtual char *process(const char *in);
 public:
 	CommandWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder);
 	virtual const char *whattype() { return "CommandWindow"; }
	virtual ~CommandWindow();
};

#endif

