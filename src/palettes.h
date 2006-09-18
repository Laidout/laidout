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
#ifndef PALETTES_H
#define PALETTES_H

#include <lax/palette.h>

class PalettePane : public Laxkit::PaletteWindow
{
 public:
	PalettePane(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,Window nowner,const char *nsend);
	virtual int send();
	virtual const char *PaletteDir();
};


#endif

