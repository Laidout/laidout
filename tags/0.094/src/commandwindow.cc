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
// Copyright (C) 2004-2006 by Tom Lechner
//


#include "language.h"
#include "commandwindow.h"
#include "headwindow.h"
#include "laidout.h"
#include "stylemanager.h"

#include <lax/fileutils.h>
#include <sys/stat.h>

using namespace LaxFiles;



namespace Laidout {


/*! \class CommandWindow
 * \brief Command line input. Future home of interactive interpreter.
 */


//! Set pads to 6.
CommandWindow::CommandWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder)
	: PromptEdit(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,None,NULL)
{
	textstyle|=TEXT_WORDWRAP;
	padx=pady=6;
	calculator=new LaidoutCalculator();
}

//! Empty destructor.
CommandWindow::~CommandWindow()
{
	if (calculator) calculator->dec_count();
}

/*! Take a whole command line, and process it, returning the command's text output.
 *
 * Must return a new'd char[].
 *
 * \todo ***** need some way to kill wayward scripts
 */
char *CommandWindow::process(const char *in)
{
	if (!in) return NULL;
	char *str=calculator->In(in);
	return str?str:newstr(_("You are surrounded by twisty passages, all alike."));
}

} // namespace Laidout

