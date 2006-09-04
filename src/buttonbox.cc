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

#include <cstdio>
#include "buttonbox.h"
#include "icons.h"

using namespace Laxkit;
using namespace std;

ButtonBox::ButtonBox(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder)
	: TabFrame(parnt,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,None,NULL)
{
	//tabframe->AddWin(new ProgressBar2(tabframe,"tf-progressbar4",PROGRESS_OVAL, 0,0,0,0, 1), "p4",NULL,0);
	char file[strlen(ICON_DIRECTORY)+50];

	sprintf(file,"%s/Image.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/ColorPatch.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Gradient.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/ImagePatch.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Path.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Text.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Zoom.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/DumpInImages.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/DeletePage.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/AddPage.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/PreviousSpread.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/NextSpread.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/ImportImage.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/InsertImage.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/PageClips.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/NewDocument.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/NewProject.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Open.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/SinglePageView.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/PageView.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/PaperView.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Print.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Quit.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/CloseDocument.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Save.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/SaveAll.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/SaveAs.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Undo.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Redo.png",ICON_DIRECTORY);
	AddIcon(file,0);
	sprintf(file,"%s/Help.png",ICON_DIRECTORY);
	AddIcon(file,0);
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

