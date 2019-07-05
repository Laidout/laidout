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

#include "about.h"
#include "../language.h"
#include "headwindow.h"
#include "helpwindow.h"
#include "../laidout.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;



namespace Laidout {


//------------------------ newHelpWindow -------------------------

Laxkit::anXWindow *newHelpWindow(const char *place)
{
	laidout->InitializeShortcuts();
	ShortcutManager *manager=GetDefaultShortcutManager();

	if (isblank(manager->setfile)) {
		makestr(manager->setfile,laidout->config_dir);
		appendstr(manager->setfile,"/default.keys");
	}

	ShortcutWindow *shortcutwin=new ShortcutWindow(NULL,"Shortcuts",_("Shortcuts"),
					ANXWIN_REMEMBER|SHORTCUTW_Show_Search|SHORTCUTW_Load_Save,
					0,0,400,600,0,place);
	makestr(shortcutwin->textheader,"#\n# Laidout shortcuts\n#\n");

	return shortcutwin;
}


} // namespace Laidout

