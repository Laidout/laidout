//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef LAIDOUT_H
#define LAIDOUT_H

#include <lax/anxapp.h>

#include "impositions/imposition.h"
#include "papersizes.h"
#include "document.h"
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
	Laxkit::PtrStack<Laxkit::anInterface> interfacepool;
	Laxkit::PtrStack<Imposition> impositionpool;
	Laxkit::PtrStack<PaperType> papersizes;
	LaidoutApp();
	virtual ~LaidoutApp();
	virtual int init(int argc,char **argv);
	virtual void setupdefaultcolors();
	void parseargs(int argc,char **argv);

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

