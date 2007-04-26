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
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include "stylewindow.h"


#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;

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

//! Create new window from the StyleDef of a Style.
GenericStyleDialog::GenericStyleDialog(Style *nstyle,anXWindow *owner)
	: RowFrame(NULL,  nstyle?nstyle->Stylename():"random style",
			ROWFRAME_STRETCH|ROWFRAME_HORIZONTAL, 
			0,0,0,0,0, 5)
{
	def=NULL;
	style=nstyle;
	if (style) style->inc_count();
	
	last=NULL;
	win_xatts.event_mask|=KeyPressMask|KeyReleaseMask|ButtonPressMask|
						  ButtonReleaseMask|PointerMotionMask|ExposureMask;
	win_xattsmask|=CWEventMask;
}

//! Constructor from a StyleDef, ends by sending a GenericStyle object.
GenericStyleDialog::GenericStyleDialog(StyleDef *nsd,anXWindow *owner)
	: RowFrame(NULL,"random style", ROWFRAME_SPACE|ROWFRAME_HORIZONTAL, 
			   0,0,0,0,0, 5)
{
	def=nsd;
	style=NULL;
	
	last=NULL;
	win_xatts.event_mask|=KeyPressMask|KeyReleaseMask|ButtonPressMask|
						  ButtonReleaseMask|PointerMotionMask|ExposureMask;
	win_xattsmask|=CWEventMask;
}

GenericStyleDialog::~GenericStyleDialog()
{
	if (style) style->dec_count();
	if (def)   def->dec_count();
}
	
//! Populate the dialog with all the little controls.
int GenericStyleDialog::init()
{
	if (!def && style) def=style->GetStyleDef();
	if (!def) return 1;
	
	if (!def) { // the style doesn't cough up an appropriate StyleDef
		char *blah=NULL;
		if (!style || style && !style->Stylename()) makestr(blah,"Unknown style has unspecified fields.");
		else {
			makestr(blah,"Style "); 
			appendstr(blah,style->Stylename());
			appendstr(blah," has unspecified fields.");
		}
							
		AddWin(new MessageBar(this,style->Stylename()?style->Stylename():"unknown",MB_MOVE, 0,0,0,0, 0,blah));
		delete[] blah;
	} else {
		last=NULL;
		MakeControls(".1",def);
	}
	Sync(1);
	return 0;
}

//! Recursively build the dialog from a StyleDef.
/*! This creates a single new entry, and calls itself if
 * there are subentries.
 *
 * \todo ***Might even want to make this a stand alone function...
 */
void GenericStyleDialog::MakeControls(const char *startext,StyleDef *sd)
{
	 	//LineInput (anXWindow *parnt, const char *ntitle, unsigned int nstyle, 
		//			 int xx, int yy, int ww, int hh, int brder,
		//			 anXWindow *prev, Window nowner=None, const char *nsend=NULL, 
		//			 const char *newlabel=NULL, const char *newtext=NULL, 
		//			 unsigned int ntstyle=0, int nlew=0, int nleh=0, 
		//			 int npadx=0, int npady=0, int npadlx=0, int npadly=0)
		
	 //*** use Name, tooltip
	switch (sd->format) {
		 // lineinput:
		//Element_Int,
		//Element_Real,
		//Element_String,
		//Element_Fields, 
		//Element_Boolean,
		//Element_3bit,
		//Element_Enum,
		//Element_DynamicEnum,
		//Element_EnumVal,
		//Element_Function,
		//Element_Color,
		case Element_Int:
		case Element_Real:
		case Element_String: {
				char *blah=NULL;
				//if (sd->format==Element_Int) blah=numtostr(style->getint(startext));
				//else if (sd->format==Element_Real) blah=numtostr(style->getdouble(startext));
				//else makestr(blah,style->getstring(startext));

				LineInput *linp;
				last=linp=new LineInput(this,"***some name***", 0,
						0,0,0,0,0,
						last, window, startext,
						sd->Name, blah);
				last->tooltip(sd->tooltip);
				AddWin(linp,linp->win_w,0,0,50, linp->win_h,0,0,50);
			} break;
							  
		 // checkbox:
		case Element_Boolean: {
				CheckBox *box;
				last=box=new CheckBox(this,startext,CHECK_LEFT, 0,0,0,0,1, 
						last,window,"control",sd->Name);
				box->tooltip(sd->tooltip);
				AddWin(box,box->win_w,0,0,50, box->win_h,0,0,50);
			} break;

		 // ??? 3 field checkbox menuselector?
		case Element_3bit: {
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: 3bit"));
				//***
			} break;
							
		 // one only checkbox menuselector
		case Element_Enum: {
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: enum"));
				//***add sliderpopup with the field's enum names
			} break;
		case Element_EnumVal: { 
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: enumval"));
				cout << "***shouldn't have ENUM_VAL here!"<<endl; 
			} break;
		case Element_DynamicEnum: { 
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: dynamic enum"));
				cout << "***implement dynamice enum here!"<<endl; 
				//***add sliderinputpopup with the field's enum names
			} break;
								
		 // ???
//		case Element_Value: {
//				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: value"));
//				***
//			} break;

		case Element_Fields: {
				if (sd->fields) {
					 //*** --Name----
					 //	 [sub 1]
					 //	 [sub 2]
					AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,
										  sd->Name));
					char *ext=NULL;
					for (int c=0; c<sd->fields->n; c++) {
						ext=new char[strlen(startext)+6];
						sprintf(ext,"%s.%d",startext,c);
						MakeControls(ext,sd->fields->e[c]);
						AddNull();
						delete[] ext; ext=NULL;
					}
				} else {
					cout << "***GenericStyleDialog::MakeControls should be fields here!!"<<endl;
				}
			} break;
		case Element_Color: {
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: color"));
			} break;
		case Element_Function: {
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: function"));
				//nothing doing here.. could have option for this to be listed...
			} break;
		default: {
				AddWin(new MessageBar(this,"---unimplemented element---",MB_MOVE, 0,0,0,0, 0,"unimplemented: unknown"));
			 } break;
	}
}

int GenericStyleDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"xscroller")) {
		//***
	} else if (!strcmp(mes,"yscroller")) {
		//***
	} else if (mes[0]=='.') { // is hopefully an extension
		//***
	}
	return 0;
} 

int GenericStyleDialog::CharInput(unsigned int ch,unsigned int state)
{
	DBG cout <<"******************************************"<<endl;
	if (ch==LAX_Esc) app->destroywindow(this);
	return 0;
}





//--------------------------- MakeStyleEditWindow -----------------------------------------

////! Return a generic dialog from an arbitrary Style.
///*! \ingroup stylesandstyledefs
// *
// * *** should make efforts to find any custom edit dialog that exists for this type of style.
// */
//anXWindow *MakeStyleEditWindow(Style *style,anXWindow *owner)
//{***
//	if (!style || !style->GetStyleDef()) return NULL;
//	return new GenericStyleDialog(style,owner);
//}

