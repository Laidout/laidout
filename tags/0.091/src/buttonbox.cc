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
// Copyright (C) 2004-2006,2010 by Tom Lechner
//

#include <cstdio>
#include "buttonbox.h"
#include "configured.h"
#include "laidout.h"

using namespace Laxkit;
using namespace std;

ButtonBox::ButtonBox(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder)
	: TabFrame(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,0,NULL)
{
	//tabframe->AddWin(new ProgressBar2(tabframe,"tf-progressbar4",PROGRESS_OVAL, 0,0,0,0, 1), "p4",NULL,0);

	AddBox(NULL,laidout->icons.GetIcon("Image"),0);
	AddBox(NULL,laidout->icons.GetIcon("ColorPatch"),0);
	AddBox(NULL,laidout->icons.GetIcon("Gradient"),0);
	AddBox(NULL,laidout->icons.GetIcon("ImagePatch"),0);
	AddBox(NULL,laidout->icons.GetIcon("Path"),0);
	AddBox(NULL,laidout->icons.GetIcon("Text"),0);
	AddBox(NULL,laidout->icons.GetIcon("Zoom"),0);
	AddBox(NULL,laidout->icons.GetIcon("DumpInImages"),0);
	AddBox(NULL,laidout->icons.GetIcon("DeletePage"),0);
	AddBox(NULL,laidout->icons.GetIcon("AddPage"),0);
	AddBox(NULL,laidout->icons.GetIcon("PreviousSpread"),0);
	AddBox(NULL,laidout->icons.GetIcon("NextSpread"),0);
	AddBox(NULL,laidout->icons.GetIcon("ImportImage"),0);
	AddBox(NULL,laidout->icons.GetIcon("InsertImage"),0);
	AddBox(NULL,laidout->icons.GetIcon("PageClips"),0);
	AddBox(NULL,laidout->icons.GetIcon("NewDocument"),0);
	AddBox(NULL,laidout->icons.GetIcon("NewProject"),0);
	AddBox(NULL,laidout->icons.GetIcon("Open"),0);
	AddBox(NULL,laidout->icons.GetIcon("SinglePageView"),0);
	AddBox(NULL,laidout->icons.GetIcon("PageView"),0);
	AddBox(NULL,laidout->icons.GetIcon("PaperView"),0);
	AddBox(NULL,laidout->icons.GetIcon("Print"),0);
	AddBox(NULL,laidout->icons.GetIcon("Quit"),0);
	AddBox(NULL,laidout->icons.GetIcon("CloseDocument"),0);
	AddBox(NULL,laidout->icons.GetIcon("Save"),0);
	AddBox(NULL,laidout->icons.GetIcon("SaveAll"),0);
	AddBox(NULL,laidout->icons.GetIcon("SaveAs"),0);
	AddBox(NULL,laidout->icons.GetIcon("Undo"),0);
	AddBox(NULL,laidout->icons.GetIcon("Redo"),0);
	AddBox(NULL,laidout->icons.GetIcon("Help"),0);
}

ButtonBox::~ButtonBox()
{
}

int ButtonBox::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 0;
}

int ButtonBox::RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	return 0;
}

