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
#ifndef LAIDOUT_H
#define LAIDOUT_H

#include <lax/anxapp.h>
#include <lax/version.h>

#include "impositions/imposition.h"
#include "papersizes.h"
#include "document.h"
#include "project.h"
#include "newdoc.h"
#include "interfaces.h"

const char *LaidoutVersion();

class LaidoutApp : public Laxkit::anXApp
{
 public:
//	ControlPanel *maincontrolpanel;
	Project *project;
	Document *curdoc;
	
//	Laxkit::PtrStack<Style> stylestack:
//	Laxkit::PtrStack<FontThing> fontstack;
//	Laxkit::PtrStack<Project> projectstack;
//	ScreenStyle *screen;
	Laxkit::PtrStack<LaxInterfaces::anInterface> interfacepool;
	Laxkit::PtrStack<Imposition> impositionpool;
	Laxkit::PtrStack<PaperType> papersizes;
	LaidoutApp();
	virtual ~LaidoutApp();
	virtual int init(int argc,char **argv);
	virtual void setupdefaultcolors();
	void parseargs(int argc,char **argv);

	Document *findDocument(const char *saveas);
	Document *LoadDocument(const char *filename);
	int NewDocument(DocumentStyle *docinfo, const char *filename);
	int NewDocument(const char *spec);

	void notifyDocTreeChanged(Laxkit::anXWindow *callfrom=NULL);
};

// if included in laidout.cc, then don't include "extern" when defining *laidout
#ifndef LAIDOUT_CC
extern
#endif
LaidoutApp *laidout;


#endif

