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

#include "buttonbox.h"

using namespace Laxkit;

ButtonBox::ButtonBox(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder)
	: TabFrame(parnt,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,None,NULL)
{
	//tabframe->AddWin(new ProgressBar2(tabframe,"tf-progressbar4",PROGRESS_OVAL, 0,0,0,0, 1), "p4",NULL,0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Image.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/ColorPatch.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Gradient.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/ImagePatch.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Path.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Text.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Zoom.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/DumpInImages.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/DeletePage.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/AddPage.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/PreviousSpread.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/NextSpread.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/ImportImage.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/InsertImage.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/PageClips.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/NewDocument.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/NewProject.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Open.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/SinglePageView.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/PageView.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/PaperView.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Print.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Quit.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/CloseDocument.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Save.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/SaveAll.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/SaveAs.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Undo.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Redo.png",0);
	AddIcon("/home/tom/p/sourceforge/laidout/src/icons/Help.png",0);
}

ButtonBox::~ButtonBox()
{
}

int ButtonBox::RBDown(int x,int y,unsigned int state,int count)
{
	return 0;
}

int ButtonBox::RBUp(int x,int y,unsigned int state)
{
	return 0;
}

