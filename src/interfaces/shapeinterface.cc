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
// Copyright (C) 2013 by Tom Lechner
//


#include "../language.h"
#include "nupinterface.h"
//#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>


#include <lax/refptrstack.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {



#define PAD 5
#define fudge 5.0







//------------------------------------- ShapeInfo --------------------------------------
/*! \class ShapeInfo
 * \brief Info about how to add on new shapes.
 *
 * See also ShapeInterface.
 */


ShapeInfo::ShapeInfo()
{
	name=NULL;
	numsides=4;
	innerpoints=0; //1 or 0
	round1=round2=0;
	edgelength=1;
}

ShapeInfo::~ShapeInfo()
{
	if (name) delete[] name;
}



//------------------------------------- ShapeInterface --------------------------------------
	
/*! \class ShapeInterface 
 * \brief Interface to build nets out of various shapes. See also ShapeInfo.
 */


ShapeInterface::ShapeInterface(int nid,Displayer *ndp)
	: anInterface(nid,ndp) 
{
	//nup_style=NUP_Has_Ok|NUP_Has_Type;
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);

	shapeinfo=new ShapeInfo;
}

ShapeInterface::ShapeInterface(anInterface *nowner,int nid,Displayer *ndp)
	: anInterface(nowner,nid,ndp) 
{
	firsttime=1;
	showdecs=0;
	color_arrow=rgbcolor(60,60,60);
	color_num=rgbcolor(0,0,0);

	shapeinfo=new ShapeInfo;
}

ShapeInterface::~ShapeInterface()
{
	//DBG cerr <<"ShapeInterface destructor.."<<endl;

	if (shapeinfo) shapeinfo->dec_count();

	//if (doc) doc->dec_count();
}


const char *ShapeInterface::Name()
{ return _("Shape"); }



/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *ShapeInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo(_("Shape Interface"));
	else menu->AddSep(_("Shapes"));

	menu->AddItem(dirname(LAX_LRTB),LAX_LRTB,LAX_ISTOGGLE|(shapeinfo->direction==LAX_LRTB?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_LRBT),LAX_LRBT,LAX_ISTOGGLE|(shapeinfo->direction==LAX_LRBT?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_RLTB),LAX_RLTB,LAX_ISTOGGLE|(shapeinfo->direction==LAX_RLTB?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_RLBT),LAX_RLBT,LAX_ISTOGGLE|(shapeinfo->direction==LAX_RLBT?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_TBLR),LAX_TBLR,LAX_ISTOGGLE|(shapeinfo->direction==LAX_TBLR?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_BTLR),LAX_BTLR,LAX_ISTOGGLE|(shapeinfo->direction==LAX_BTLR?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_TBRL),LAX_TBRL,LAX_ISTOGGLE|(shapeinfo->direction==LAX_TBRL?LAX_CHECKED:0)|LAX_OFF,1);
	menu->AddItem(dirname(LAX_BTRL),LAX_BTRL,LAX_ISTOGGLE|(shapeinfo->direction==LAX_BTRL?LAX_CHECKED:0)|LAX_OFF,1);

	menu->AddSep();

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int ShapeInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i =s->info2; //id of menu item
		int ii=s->info4; //extra id, 1 for direction
		if (ii==0) {
			cerr <<"change direction to "<<i<<endl;
			return 0;

		} else {
			if (i==NUP_Grid) {
			} else if (i==NUP_Sized_Grid) {
			} else if (i==NUP_Flowed) {
			} else if (i==NUP_Random) {
			} else if (i==NUP_Unclump) {
			} else if (i==NUP_Unoverlap) {
			}
			return 0;

		}

		return 0;
	}
	return 1;
}


/*! Will say it cannot draw anything.
 */
int ShapeInterface::draws(const char *atype)
{ return 0; }


//! Return a new ShapeInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *ShapeInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new ShapeInterface(id,NULL);
	else if (!dynamic_cast<ShapeInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int ShapeInterface::InterfaceOn()
{
	//DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	showdecs=2;
	needtodraw=1;
	return 0;
}

int ShapeInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void ShapeInterface::Clear(SomeData *d)
{
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int ShapeInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
		remapControls();
	}

	//DBG cerr <<"ShapeInterface::Refresh()..."<<endl;

	dp->DrawScreen();

	int x=shapeinfo->uioffset.x;
	int y=shapeinfo->uioffset.y;
	int dir=shapeinfo->direction;
	int majorn, minorn;
	unsigned int arrowcolor=rgbcolor(128,128,128);

	if (tempdir>=0) {
		arrowcolor=rgbcolor(0,200,0);
		dir=tempdir;
		remapControls(tempdir);
	}

	flatpoint p;
	char buffer[30];

	 //draw ui outline
	dp->NewFG(rgbcolor(128,128,128));
	dp->DrawScreen();
	dp->drawline(flatpoint(x+shapeinfo->minx,y+shapeinfo->miny),flatpoint(x+shapeinfo->maxx,y+shapeinfo->miny));
	dp->drawline(flatpoint(x+shapeinfo->maxx,y+shapeinfo->miny),flatpoint(x+shapeinfo->maxx,y+shapeinfo->maxy));
	dp->drawline(flatpoint(x+shapeinfo->maxx,y+shapeinfo->maxy),flatpoint(x+shapeinfo->minx,y+shapeinfo->maxy));
	dp->drawline(flatpoint(x+shapeinfo->minx,y+shapeinfo->maxy),flatpoint(x+shapeinfo->minx,y+shapeinfo->miny));

	 //draw major arrow number
	p=flatpoint(x+(majornum->minx+majornum->maxx)/2,y+(majornum->miny+majornum->maxy)/2);
	if (dir==LAX_LRTB || dir==LAX_LRBT || dir==LAX_RLTB || dir==LAX_RLBT) {
		majorn=shapeinfo->rows;
		minorn=shapeinfo->cols;
	} else {
		majorn=shapeinfo->cols;
		minorn=shapeinfo->rows;
	}

	if (majorn>=1) sprintf(buffer,"%d",majorn);
	else sprintf(buffer,"...");
	dp->textout(p.x,p.y, buffer,-1);

	 //draw minor arrow number
	p=flatpoint(x+(minornum->minx+minornum->maxx)/2,y+(minornum->miny+minornum->maxy)/2);
	if (minorn>=1) sprintf(buffer,"%d",minorn);
	else sprintf(buffer,"...");
	dp->textout(p.x,p.y, buffer,-1);

	 //draw arrows
	drawHandle(major,arrowcolor,shapeinfo->uioffset);
	drawHandle(minor,arrowcolor,shapeinfo->uioffset);

	 //draw ok
	if (nup_style&NUP_Has_Ok) drawHandle(okcontrol,okcontrol->color,shapeinfo->uioffset);

	 //draw type
	if (nup_style&NUP_Has_Type) drawHandle(typecontrol,typecontrol->color,shapeinfo->uioffset);
	


	dp->DrawReal();

	if (tempdir>=0) {
		remapControls(shapeinfo->direction);
	}

	return 1;
}

int ShapeInterface::scan(int x,int y)
{
	//scan for being inside a shape
	//scan for being on an edge of a shape
}

int ShapeInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.any(0,LEFTBUTTON)) return 0; //only allow one button at a time

	int action=scan(x,y);
	buttondown.down(d->id,LEFTBUTTON, x,y);

	return 1;
}

int ShapeInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{ //***
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);


//	if (!dragged) {
//		if ((state&LAX_STATE_MASK)==ControlMask) {
//			// *** edit base
//		} else {
//			// *** edit first
//		}
//	}

	return 0;
}

/*! Puts a polygon with shapeinfo->numsides number of sides in possible.
 */
void ShapeInterface::remapPossible(int x,int y)
{
	if (!possible) possible=new PathsData();
	else possible->paths.flush();

	flatpoint p=dp->screentoreal(x,y);
	
	for (int c=0; c<shapeinfo->numsides; c++) {
		possible->append(p + shapeinfo->scale*flatpoint(cos(2*M_PI*c/shapeinfo->numsides),sin(2*M_PI*c/shapeinfo->numsides)));
	}
	possible->close();
}

int ShapeInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (possible) {
		shapeinfo->numsides++;
		remapPossible(x,y);
		needtodraw=1;
		return 0;
	}
	return 1;
}

int ShapeInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (possible) {
		shapeinfo->numsides--;
		if (shapeinfo->numsides<3) shapeinfo->numsides=3;
		remapPossible(x,y);
		needtodraw=1;
		return 0;
	}
	return 1;
}


int ShapeInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{ //***

	int over=scan(x,y);

	return 0;
}

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int ShapeInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{ //***
	//DBG cerr<<" got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch==LAX_Esc) {

	} else if (ch=='o') {
	}
	return 1;
}

int ShapeInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{ //***
	return 1;
}


} // namespace Laidout

