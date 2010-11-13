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
// Copyright (C) 2010 by Tom Lechner
//
#ifndef NEDIALOG_H
#define NEDIALOG_H


#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include "../papersizes.h"

class NetDialog : public Laxkit::RowFrame
{
 public:
	PaperStyle *paperstyle;
	Laxkit::CheckBox *checkbox,*checkdod,*checkfile;
	Laxkit::LineInput *boxdims,*impfromfile,*scaling;

	NetDialog(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
					 unsigned int owner, const char *mes,
					 PaperStyle *paper);
	virtual ~NetDialog();
	virtual const char *whattype() { return "NetDialog"; }
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	int sendNewImposition();
};



#endif

