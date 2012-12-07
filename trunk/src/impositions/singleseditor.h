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
#ifndef SINGLESEDITOR_H
#define SINGLESEDITOR_H

#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include "singles.h"


namespace Laidout {


class SinglesEditor : public Laxkit::RowFrame
{
	virtual void send();
 public:
	int curorientation;

	Singles *imp;
	Document *doc;
	PaperStyle *papertype;

	Laxkit::LineInput *paperx,*papery;
	Laxkit::LineInput *marginl,*marginr,*margint,*marginb;
	Laxkit::LineInput *insetl,*insetr,*insett,*insetb;
	Laxkit::LineInput *tilex,*tiley,*gapx,*gapy;

 	SinglesEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
			unsigned long nowner, const char *mes,
			Document *ndoc, Singles *simp, PaperStyle *paper);
	virtual ~SinglesEditor();
	virtual const char *whattype() { return "SinglesEditor"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	//int UseThisImposition(Imposition *imp);
	void UpdatePaper(int dialogtoimp);
};

} // namespace Laidout

#endif

