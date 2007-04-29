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
#ifndef STYLEWINDOW_H
#define STYLEWINDOW_H

#include "laidout.h"
#include <lax/rowframe.h>


class GenericStyleDialog : public Laxkit::RowFrame
{
 protected:
	Style *style;
	StyleDef *def;
	anXWindow *last;
 public:
	GenericStyleDialog(Style *nstyle,anXWindow *owner);
	GenericStyleDialog(StyleDef *nsd,anXWindow *owner);
	virtual ~GenericStyleDialog();
	virtual int init();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual void MakeControls(const char *startext,StyleDef *sd);
	virtual int CharInput(unsigned int ch,unsigned int state);
};



#endif

