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


#include "somedialogs.h"
#include <lax/textbutton.h>
#include <lax/mesbar.h>

using namespace Laxkit;


//-------------------------- Overwrite ------------------------------------
//class Overwrite : public Laxkit::MessageBox
//{
// public:
//	char *file;
//	Overwrite(Window nowner,const char *mes, const char *nfile);
//	virtual ~Overwrite() { delete file; }
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//};


Overwrite::Overwrite(Window nowner,const char *mes, const char *nfile)
	: MessageBox(NULL, "Overwrite?", ANXWIN_CENTER|ANXWIN_DELETEABLE, 
				 0,0,0,0,0, NULL,nowner,mes, "Overwrite?")
{
	file=newstr(nfile);
	char *blah=newstr("Overwrite\n");
	appendstr(blah,nfile);
	appendstr(blah,"?");
	WinFrameBox *box=dynamic_cast<WinFrameBox *>(wholelist.e[0]);
	MessageBar *mesbar=dynamic_cast<MessageBar *>(box->win);
	if (mesbar) {
		mesbar->win_w=0;
		mesbar->win_h=0;
		mesbar->SetText(blah);
		mesbar->SetupMetrics();
		box->pw(mesbar->win_w);
		box->ph(mesbar->win_h);
	}
	AddButton(TBUT_OVERWRITE);
	AddButton(TBUT_NO);
}

//! Sends a StrSendData to owner with file in it.
int Overwrite::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	//DBG cout <<win_title<<" -- ClientMessage"<<endl;
	if (strcmp(mes,"mbox-mes")) return 1; //***

	if (e->data.l[1]==TBUT_OVERWRITE) {
		StrSendData *data=new StrSendData(file,sendthis,window,owner);
		app->SendMessage(data);
	}
	
	app->destroywindow(this);
	return 0;
}

////-------------------------- newOverwriteBox ------------------------------------
////! Return Laxkit::MessageBox with 'Overwrite [file]?' and Ok and Cancel buttons.
//anXWindow *newOverwriteBox(Window nowner, const char *mes, const char *file)
//{
//	char title[12+strlen(file)];
//	sprintf(title,"Overwrite %s?",file);
//	MessageBox *box=new MessageBox(NULL, "Overwrite?", ANXWIN_DELETEABLE, 0,0,0,0,0, NULL,nowner,mes, title);
//	box->AddButton(TBUT_YES);
//	box->AddButton(TBUT_NO);
//	return box;
//}


