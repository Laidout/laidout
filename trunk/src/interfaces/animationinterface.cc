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
// Copyright (C) 2014 by Tom Lechner
//



#include "animationinterface.h"
#include "../laidout.h"
#include "../dataobjects/datafactory.h"

#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/rectinterface.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


#define DBG
#include <iostream>
using namespace std;


namespace Laidout {




//------------------------------------- AnimationInterface --------------------------------------


/*! \class AnimationInterface
 *
 * Interface for simple animation.
 */



#define INTERFACE_CIRCLE  20


enum AnimationActions {
	ANIM_None=0,
	ANIM_Play,
	ANIM_Rewind,
	ANIM_To_End,
	ANIM_Backwards,
	ANIM_Faster,
	ANIM_Slower,
	ANIM_Toggle_Backwards,

	ANIM_MAX
};

enum AnimationToolModes {
	ANIMMODE_Normal
};

AnimationInterface::AnimationInterface(anInterface *nowner,int nid,Laxkit::Displayer *ndp)
  : anInterface(nowner,nid,ndp)
{
	mode=ANIMMODE_Normal;

	selection=NULL;

	animation_style=0;
	hover=ANIM_None;
	hoveri=-1;
	playing=false;

	sc=NULL;

	firsttime=1;
	uiscale=1;
	bg_color =rgbcolorf(.9,.9,.9);
	hbg_color=rgbcolorf(1.,1.,1.);
	fg_color =rgbcolorf(.1,.1,.1);
	activate_color=rgbcolorf(0.,.783,0.);
	deactivate_color=rgbcolorf(1.,.392,.392);

}

AnimationInterface::~AnimationInterface()
{
	//if (source_objs) source_objs->dec_count();
	if (sc) sc->dec_count();
	if (selection) { selection->dec_count(); selection=NULL; }
}

const char *AnimationInterface::Name()
{
	return _("Animation");
}


anInterface *AnimationInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new AnimationInterface(NULL,id,NULL);
	else if (!dynamic_cast<AnimationInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}

void AnimationInterface::Clear(LaxInterfaces::SomeData *d)
{ // ***
}

Laxkit::ShortcutHandler *AnimationInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("AnimationInterface");
	if (sc) return sc;

//	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("AnimationInterface");

//	sc->Add(CLONEIA_Next_Tiling,        LAX_Left,0,0,  "NextTiling",        _("Select next tiling"),    NULL,0);
//	sc->Add(CLONEIA_Previous_Tiling,    LAX_Right,0,0, "PreviousTiling",    _("Select previous tiling"),NULL,0);
//	sc->Add(CLONEIA_Next_Basecell,      LAX_Up,0,0,    "NextBasecell",      _("Select next base cell"),    NULL,0);
//	sc->Add(CLONEIA_Previous_Basecell,  LAX_Down,0,0,  "PreviousBasecell",  _("Select previous base cell"),    NULL,0);
//	sc->Add(CLONEIA_Toggle_Lines,       'l',0,0,       "ToggleLines",       _("Toggle rendering cell lines"),NULL,0);
//	sc->Add(CLONEIA_Toggle_Render,      LAX_Enter,0,0, "ToggleRender",      _("Toggle rendering"),NULL,0);
//	sc->Add(CLONEIA_Toggle_Preview,     'p',0,0,       "TogglePreview",     _("Toggle preview of lines"),NULL,0);
//	sc->Add(CLONEIA_Toggle_Orientations,'o',0,0,       "ToggleOrientations",_("Toggle preview of orientations"),NULL,0);
//	sc->Add(CLONEIA_Edit,            'e',ControlMask,0,"Edit",              _("Edit"),NULL,0);
//	sc->Add(CLONEIA_Select,             's',0,0,       "Select",            _("Select tile mode"),NULL,0);
//
//	//sc->AddShortcut(LAX_Del,0,0, PAPERI_Delete);


	manager->AddArea("AnimationInterface",sc);
	return sc;
}



int AnimationInterface::PerformAction(int action)
{

	return 1;
}

int AnimationInterface::InterfaceOn()
{
	needtodraw=1;
	return 0;
}

int AnimationInterface::InterfaceOff()
{
	if (selection) { selection->dec_count(); selection=NULL; }
	playing=false;

	needtodraw=1;
	return 0;
}

enum AnimationMenuItems {
	ANIM_Clear_Base_Objects,
	ANIM_Include_Lines,
	ANIM_Groupify,
	ANIM_Load,
	ANIM_Save
};

Laxkit::MenuInfo *AnimationInterface::ContextMenu(int x,int y,int deviceid)
{
//    MenuInfo *menu=new MenuInfo(_("Animation Interface"));
//
//    menu->AddItem(_("Clear current base objects"), CLONEM_Clear_Base_Objects);
//    menu->AddSep();
//    menu->AddItem(_("Include lines"),      CLONEM_Include_Lines, LAX_ISTOGGLE|(trace_cells?LAX_CHECKED:0));
//	menu->AddItem(_("Groupify base cells"),    CLONEM_Groupify, LAX_ISTOGGLE|(groupify_clones?LAX_CHECKED:0));
//    menu->AddSep();
//    menu->AddItem(_("Load resource"), CLONEM_Load);
//    menu->AddItem(_("Save as resource"), CLONEM_Save);
//
//    return menu;

	return NULL;
}

int AnimationInterface::Event(const Laxkit::EventData *e,const char *mes)
{
//    if (!strcmp(mes,"menuevent")) {
//        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
//        int i =s->info2; //id of menu item
//        //int ii=s->info4; //extra id, 1 for direction
//
//        if (i==CLONEM_Clear_Base_Objects) {
//		}
//
//		return 0;
//
//	} else if (!strcmp(mes,"load")) {

	return 1;
}

int AnimationInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	if (dynamic_cast<LineStyle*>(ndata)) {
		// *** set keyframe display color?
	}
	return 0;
}


int AnimationInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;


	double circle_radius=INTERFACE_CIRCLE*uiscale;

	//  1. rewind to beginning 
	//  2. play backwards/faster backwards
	//  3. play/pause, reset to normal play speed
	//  4. faster forwards
	//  5. advance to end
	//
	//   1  2  3  4  5
	//  |<  <  >  >  >|    ---------*----------------------------

	if (firsttime==1) {
		firsttime=0;
		box.minx=10;
		box.maxx=10+4*circle_radius*uiscale;
		box.miny=10;
		box.maxy=10+circle_radius*uiscale + 2*dp->textheight();
	} else if (firsttime==2) {
		 //remap control box size only, leave in same place
		firsttime=0;
		box.maxx=box.minx+4*circle_radius*uiscale;
		box.maxy=box.miny+circle_radius*uiscale + 2*dp->textheight();
	}


	 //--------draw control box------
	dp->DrawScreen();

	 //draw whole rect outline
	dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
	dp->NewBG(bg_color);
	dp->NewFG(fg_color);
	dp->drawrectangle(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny, 2);


	double h=box.maxy-box.miny;
	dp->drawthing(h/2+box.minx  ,box.miny+h/2, h/3,h/3, 1, THING_Double_Triangle_Left);
	dp->drawthing(h/2+box.minx+h,box.miny+h/2, h/3,h/3, 1, THING_Triangle_Left);
	if (playing) dp->drawthing(h/2+box.minx+2*h,box.miny+h/2, h/3,h/3, 1, THING_Pause);
	else dp->drawthing(h/2+box.minx+2*h,box.miny+h/2, h/3,h/3, 1, THING_Triangle_Right);
	dp->drawthing(h/2+box.minx+3*h,box.miny+h/2, h/3,h/3, 1, THING_Triangle_Right);
	dp->drawthing(h/2+box.minx+4*h,box.miny+h/2, h/3,h/3, 1, THING_Double_Triangle_Right);


	dp->DrawReal();

	return 0;
}

int AnimationInterface::scan(int x,int y, int *i)
{
	//double circle_radius=INTERFACE_CIRCLE*uiscale;

//	ANIM_Play,
//	ANIM_Rewind,
//	ANIM_To_End,
//	ANIM_Faster,
//	ANIM_Slower,
//	ANIM_Toggle_Backwards,

	 //check for things related to the tiling selector
	if (box.boxcontains(x,y)) {
		x-=box.minx;
		y-=box.miny;
		double h=box.maxy-box.miny;
		if (x<h) return ANIM_Rewind;
		if (x<2*h) return ANIM_Backwards;
		if (x<3*h) return ANIM_Play;
		if (x<4*h) return ANIM_Faster;
		return ANIM_To_End;
	}

	return ANIM_None;
}

int AnimationInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;


	 // else click down on something for overlay
	int over=scan(x,y,NULL);
	buttondown.down(d->id,LEFTBUTTON,x,y, over,state);

	return 0;
}

bool AnimationInterface::Play(int on)
{
	bool play=playing;

	if (on<0) play=!play;
	else if (on==0) play=false;
	else play=true;

	if (play==playing) return playing;
	playing=play;
	 
	if (playing) {
		//*** set up timer
	} else {
		// *** remove timer
	}

	needtodraw=1;
	PostMessage(playing ? _("Playing...") : _("Paused"));
	return playing;
}


int AnimationInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;


	int firstover=ANIM_None;
	//int dragged=
	buttondown.up(d->id,LEFTBUTTON, &firstover);


	return 0;
}

int AnimationInterface::Mode(int newmode)
{
	if (newmode==mode) return mode;


	return mode;
}

int AnimationInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{

	int i=-1;
	int over=scan(x,y,&i);
	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (hover!=over) needtodraw=1;
		hover=over;
		if (hover==ANIM_None) PostMessage(" ");
		return 0;
	}



//	 //button is down on something...
//	int lx,ly, oldover=CLONEI_None;
//	buttondown.move(mouse->id,x,y, &lx,&ly);
//	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &oldover);

	return 0;
}

int AnimationInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	return 1;
}

int AnimationInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	//int over=scan(x,y,NULL);
	//DBG cerr <<"wheel down clone interface: "<<over<<endl;

	return 1;
}

int AnimationInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<LAX_Shift<<"  "<<ShiftMask<<"  "<<(state&LAX_STATE_MASK)<<endl;
	

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}


	return 1;
}



} //namespace Laidout

