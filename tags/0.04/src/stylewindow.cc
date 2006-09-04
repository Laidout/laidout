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
/******** stylewindow.cc **********/

// This file defines the functions to make a generic dialog for
// and arbitrary StyleDef.
//

#include <lax/mesbar.h>
#include "stylewindow.h"


#include <iostream>
using namespace std;
#define DBG 


//*** --stylename---     <-- instance name
// 	  -- styledef Name-- <-- type name
// 	   field 1
// 	   field 2
// 	   ...

//--------------------------- GenericStyleDialog  -----------------------------------------
/*! \class GenericStyleDialog
 * \brief Generic dialog based on any StyleDef.
 * \ingroup stylesandstyledefs
 * 
 * The basic strategy is this:
 * 
 *  1. Something within a window control changes, and the standard
 *     control message is sent of the form ".2.3" or some such.
 *     
 *  2. The controller receives this message, and looks up the window
 *     corresponding to ".2.3".
 *     
 *  3. The controller retrieves the current state of that control,
 *     and tells the Style to change Style.2.3.
 *
 *  4. Doing so causes Style to return a mask of what other fields
 *     change when changing the ".2.3" field, and the controller
 *     steps through that mask, retrieving the modified state from
 *     the Style, and telling the corresponding window control to
 *     use this new state.
 */
//class GenericStyleDialog : public RowFrame
//{
// protected:
//	Style *style;
//	StyleDef *def;
//	anXWindow *last;
// public:
//	GenericStyleDialog(Style *nstyle,anXWindow *owner);
//	GenericStyleDialog(StyleDef *nsd,anXWindow *owner);
//	virtual int init();
//	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
//}

//	anXWindow(anXApp *napp,anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//		int xx,int yy,int ww,int hh,unsigned int brder,anXWindow *prev,Window nowner=None,const char *nsend=NULL,int nid=0);
GenericStyleDialog::GenericStyleDialog(Style *nstyle,anXWindow *owner)
	: anXWindow(laidout,NULL,nstyle?nstyle->stylename:"random style",0, 0,0,0,0,0,
			NULL,owner?owner->window:None,NULL,0)
{
	style=nstyle;
	last=NULL;
}

//! Constructor from 
GenericStyleDialog::GenericStyleDialog(StyleDef *nsd,anXWindow *owner)
	: anXWindow(laidout,NULL,"random style",0, 0,0,0,0,0,
			NULL,owner?owner->window:None,NULL,0)
{
	style=NULL;
}

//! Populate the dialog with all the little controls.
int GenericStyleDialog::init()
{
	if (!style) return 1;
	StyleDef *sd=style->GetStyleDef();
	if (!sd) { // the style doesn't cough up an appropriate StyleDef
		char *blah=NULL;
		if (!style->stylename) makestr(blah,"Unknown style has unspecified fields.");
		else {
			makestr(blah,"Style "); 
			appendstr(style->stylename);
			appendstr(style->" has unspecified fields.");
		}
							
		AddWin(new MessageBar(laidout,NULL,style->stylename?style->stylename:"unknown",MB_MOVE, 0,0,0,0, 0,blah));
		delete[] blah;
		Sync(1);
		return 0;
	}
	MakeControls(".1",sd);
	Sync(1);
	return 0;
}

//! Return an anXWindow corresponding to sd.***Might even want to make this a stand alone function...
void GenericStyleDialog::MakeControls(const char *startext,StyleDef *sd)
{
	*** use Name, tooltip
	switch (sd->format) {
		 // lineinput:
		case STYLEDEF_INT:
		case STYLEDEF_REAL:
		case STYLEDEF_STRING: {
				char *blah=NULL;
				if (sd->format==STYLEDEF_INT) blah=numtostr(style->getint(startext));
				else if (sd->format==STYLEDEF_REAL) blah=numtostr(style->getdouble(startext));
				else makestr(blah,style->getstring(startext));
				LineInput *linp=new LineInput(***, blah);
			} break;
							  
		 // checkbox:
		case STYLEDEF_BIT: {
				CheckBox *box;
				last=box=new CheckBox(this,startext,CHECK_LEFT, 0,0,0,0,1, 
						last,window,"control",sd->Name);
				box->tooltip(sd->tooltip);
				AddWin(box,box->win_w,0,0,50, box->win_h,0,0,50);
			} break;

		 // ??? 3 field checkbox menuselector?
		case STYLEDEF_3BIT: {
				***
			} break;
							
		 // one only checkbox menuselector
		case STYLEDEF_ENUM: {
				***
			} break;
		case STYLEDEF_ENUM_VAL: { 
				cout << "***shouldn't have ENUM_VAL here!"<<endl); 
			} break;
								
		 // ???
		case STYLEDEF_VALUE: {
				***
			} break;

		case STYLEDEF_FIELDS: {
				if (sd->fields) {
					*** --Name----
						 [sub 1]
						 [sub 2]
					char *ext=NULL;
					for (int c=0; c<sd->fields->n; c++) {
						ext=new char[strlen(startext)+6];
						sprintf(ext,"%s.%d",startext,c);
						***add to what? AddWin(MakeControl(ext,sd->fields->e[c]);
						***add to what??? AddNull();
						delete[] ext; ext=NULL;
					}
				} else {
					cout << "***GenericStyleDialog::MakeControls should be fields here!!"<<endl;
				}
			} break;
		case STYLEDEF_FUNCTION: {
				//nothing doing here.. could have option for this to be listed...
			} break;
	}
}

int GenericStyleDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"xscroller")) {
		***
	} else if (!strcmp(mes,"yscroller")) {
		***
	} else if (mes[0]=='.') { // is hopefully an extension
		***
	}
	return 0;
} 





//--------------------------- MakeStyleEditWindow -----------------------------------------

//! Return a generic dialog from an arbitrary Style.
/*! \ingroup stylesandstyledefs
 *
 * *** should make efforts to find any custom edit dialog that exists for this type of style.
 */
anXWindow *MakeStyleEditWindow(Style *style,anXWindow *owner)
{***
	if (!style || !style->GetStyleDef()) return NULL;
	return new GenericStyleDialog(style,owner);
}

