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
	if (laidout->lastview) owner = laidout->lastview->object_id;
	if (!owner) owner=win_owner;
	if (!owner) return 0;

	SimpleColorEventData *e=new SimpleColorEventData;

	e->max = 65535; //palette->defaultmaxcolor;
	e->numchannels = palette->colors.e[curcolor]->color->nvalues;
	e->channels = new int[e->numchannels];

	int c;
	for (c=0; c<palette->colors.e[curcolor]->color->nvalues; c++) 
		e->channels[c] = palette->colors.e[curcolor]->color->values[c] * 65535;
	
	app->SendMessage(e,owner,win_sendthis,object_id);
	return 1;
}

} // namespace Laidout

