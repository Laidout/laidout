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

#include "papersizes.h"
#include "document.h"
#include "project.h"
#include "newdoc.h"
#include "interfaces.h"
#include "impositions/imposition.h"

const char *LaidoutVersion();

enum TreeChangeType {
		TreeDocGone,
		TreePagesAdded,
		TreePagesDeleted,
		TreePagesMoved,
		TreeObjectRepositioned,
		TreeObjectReorder,
		TreeObjectDiffPage,
		TreeObjectDeleted,
		TreeObjectAdded
	};

class TreeChangeEvent : public Laxkit::EventData
{
 public:
	Laxkit::anObject *changer;
	TreeChangeType changetype;
	union {
		Document *doc;
		Page *page;
		Laxkit::anObject *obj;
	} obj;
	int start,end;
};

class LaidoutApp : public Laxkit::anXApp
{
 public:
//	ControlPanel *maincontrolpanel;
	Project *project;
	Document *curdoc;
	Laxkit::anXWindow *lastview;

	unsigned long curcolor;
	
//	Laxkit::PtrStack<Style> stylestack:
//	Laxkit::PtrStack<FontThing> fontstack;
//	Laxkit::PtrStack<Project> projectstack;
//	ScreenStyle *screen;
	Laxkit::PtrStack<LaxInterfaces::anInterface> interfacepool;
	Laxkit::PtrStack<Imposition> impositionpool;
	Laxkit::PtrStack<PaperStyle> papersizes;
	LaidoutApp();
	virtual ~LaidoutApp();
	virtual int init(int argc,char **argv);
	virtual void setupdefaultcolors();
	void parseargs(int argc,char **argv);
	int readinlaidoutrc();

	Document *findDocument(const char *saveas);
	Document *LoadDocument(const char *filename);
	int NewDocument(DocumentStyle *docinfo, const char *filename);
	int NewDocument(const char *spec);
	int DumpWindows(FILE *f,int indent,Document *doc);

	void notifyDocTreeChanged(Laxkit::anXWindow *callfrom,TreeChangeType change,int s,int e);
};

// if included in laidout.cc, then don't include "extern" when defining *laidout
#ifndef LAIDOUT_CC
extern
#endif
LaidoutApp *laidout;


#endif

