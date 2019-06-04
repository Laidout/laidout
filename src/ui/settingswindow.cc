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
// Copyright (C) 2004-2007 by Tom Lechner
//

#include <lax/messagebar.h>
#include <lax/button.h>
#include <lax/tabframe.h>

#include <lax/shortcutwindow.h>

#include "settingswindow.h"
#include "about.h"
#include "../language.h"
#include "headwindow.h"
#include "helpwindow.h"
#include "../laidout.h"
#include "valuewindow.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;



namespace Laidout {


/*! Class to hold Shortcuts, About, and Settings.
 */
class HelpAbout : public Laxkit::TabFrame
{
  public:
	HelpAbout()
		: TabFrame(NULL,"Help",_("Help"),
				   ANXWIN_REMEMBER |ANXWIN_ESCAPABLE |BOXSEL_LEFT |BOXSEL_TOP |BOXSEL_ONE_ONLY |BOXSEL_ROWS,
				   0,0,600,600,0, NULL,0,NULL)
		{}
	virtual ~HelpAbout() {}
	virtual const char *whattype() { return "Help"; }
};

Laxkit::anXWindow *newSettingsWindow(const char *which, const char *place)
{
	TabFrame *frame = new HelpAbout();

	 //shortcuts tab
	laidout->InitializeShortcuts();
	ShortcutManager *manager=GetDefaultShortcutManager();

	if (isblank(manager->setfile)) {
		makestr(manager->setfile,laidout->config_dir);
		appendstr(manager->setfile,"/default.keys");
	}

	ShortcutWindow *shortcutwin=new ShortcutWindow(frame,"Shortcuts",_("Shortcuts"),
					ANXWIN_REMEMBER|SHORTCUTW_Show_Search|SHORTCUTW_Load_Save,
					0,0,400,600,0,place);
	shortcutwin->SetWinStyle(ANXWIN_ESCAPABLE, 0);
	makestr(shortcutwin->textheader,"#\n# Laidout shortcuts\n#\n");

	frame->AddWin(shortcutwin, 1, _("Shortcuts"), NULL, 0);


	 //settings tab
	ValueWindow *settings = new ValueWindow(nullptr, "Settings", _("Settings"), frame->object_id, "settings", &laidout->prefs);
	settings->Initialize();
	frame->AddWin(settings,1,
				   _("Settings"), NULL, 0);
	//frame->AddWin(new MessageBar(frame, "Settings", _("Settings"), MB_CENTER, 0,0,0,0,0, "TODO!"),1,
	//			   _("Settings"), NULL, 0);


	 //about tab
	AboutWindow *about=new AboutWindow(frame);
	about->SetWinStyle(ANXWIN_ESCAPABLE, 0);
	frame->AddWin(about, 1, _("About"), NULL, 0);



	if (!strcmp(which, "about")) frame->SelectN(2);
	else if (!strcmp(which, "settings")) frame->SelectN(1);
	else frame->SelectN(0);

	return frame;
}



} // namespace Laidout

