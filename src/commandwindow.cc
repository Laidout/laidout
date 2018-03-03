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
	textstyle |= TEXT_WORDWRAP;
	padx = pady = 6;
	calculator = new LaidoutCalculator();
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

	 //intercept to change the running interpreter
	while (isspace(*in)) in++;
	if (!strncmp(in, "ChangeInterpreter", 17) && isspace(in[17])) {
		try {
			in += 18;
			while (isspace(*in)) in++;
			Interpreter *ii = laidout->FindInterpreter(in);
			if (ii) {
				if (ii != calculator) {
					calculator->dec_count();
					calculator = ii;
					calculator->inc_count();
				}

				char *msg = new char[strlen(_("Now using \"%s\".")) + 1 + strlen(in)];
				sprintf(msg, _("Now using \"%s\"."), in);
				return msg;
			}

			char *msg = new char[strlen(_("Unknown interpreter \"%s\".")) + 1 + strlen(in)];
			sprintf(msg, _("Unknown interpreter \"%s\"."), in);
			return msg;

		} catch (const char *error_msg) {
			return newstr(error_msg);
		}
	}

	//else
	char *str = calculator->In(in, NULL);
	return str ? str : newstr(_("You are surrounded by twisty passages, all alike."));
}

} // namespace Laidout

