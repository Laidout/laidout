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
// Copyright (C) 2004-2006,2010,2014 by Tom Lechner
//
#ifndef STYLEWINDOW_H
#define STYLEWINDOW_H

#include "laidout.h"
#include <lax/rowframe.h>


namespace Laidout {


class GenericValueDialog : public Laxkit::RowFrame
{
 protected:
	Value *style;
	ObjectDef *def;
	anXWindow *last;
 public:
	GenericValueDialog(Value *nvalue,anXWindow *owner);
	GenericValueDialog(ObjectDef *ndef,anXWindow *owner);
	virtual ~GenericValueDialog();
	virtual int init();
	virtual int Event(EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual void MakeControls(const char *startext,ObjectDef *ndef);
};



} // namespace Laidout

#endif

