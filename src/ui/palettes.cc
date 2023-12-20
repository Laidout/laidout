//
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//


#include <lax/colors.h>

#include "palettes.h"
#include "../laidout.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


namespace Laidout {


PalettePane::PalettePane(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend)
	: PaletteWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{}

const char *PalettePane::PaletteDir()
{
	return laidout->prefs.palette_dir;
}


//! Send a anXWindow::sendthis message to laidout->lastview. 
int PalettePane::send()
{
	if (!win_sendthis || !palette || curcolor<0) return 0;

	unsigned long owner=0;
	if (laidout->LastView()) {
		owner = laidout->LastView()->object_id;
		anXWindow *colorbox = laidout->LastView()->findChildWindowByName("colorbox");
		if (colorbox) owner = colorbox->object_id;
	}
	if (!owner) owner = win_owner;
	if (!owner) return 0;

	ColorEventData *e = new ColorEventData(palette->colors.e[curcolor]->color, 0, 0,0,0);
	e->colorindex = -1;
	
	app->SendMessage(e,owner,win_sendthis,object_id);
	return 1;
}

} // namespace Laidout

