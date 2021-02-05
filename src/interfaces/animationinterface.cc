//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
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

//template implementation:
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


#define DBG
#include <iostream>
using namespace std;


namespace Laidout {




//------------------------------------- ClipInfo --------------------------------------

/*! \class ClipInfo
 */

ClipInfo::ClipInfo(const char *nname, double len, double in, double out)
{
	name      = newstr(nname);
	length    = len;
	this->in  = in;
	this->out = out;
	current   = 0;

	//annotations: 
	//  time
	//  label
	//  extra info, like an object to jump to
	//  color
}

ClipInfo::~ClipInfo()
{
	delete[] name;
}



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
	ANIM_Current, //on the ball
	ANIM_Current_Time, //on the number
	ANIM_Current_Frame,
	ANIM_Timeline,
	ANIM_Jump_to,
	ANIM_Jump_toward,
	ANIM_Move_Strip,
	ANIM_Start_Time,
	ANIM_Start_Frame,
	ANIM_End_Time,
	ANIM_End_Frame,
	ANIM_Duration,
	ANIM_FPS,
	ANIM_Timeline_Size,

	ANIM_MAX
};

enum AnimationToolModes {
	ANIMMODE_Normal
};

AnimationInterface::AnimationInterface(anInterface *nowner,int nid,Laxkit::Displayer *ndp)
  : anInterface(nowner,nid,ndp)
{
	mode = ANIMMODE_Normal;
	interface_type = INTERFACE_Overlay;

	selection = NULL;

	animation_style = 0;
	hover  = ANIM_None;
	hoveri = -1;

	sc = NULL;
	font = app->defaultlaxfont->duplicate();

	firsttime      = 1;
	uiscale        = 1;
	bg_color       = rgbcolorf(.9,.9,.9);
	hbg_color      = rgbcolorf(1.,1.,1.);
	fg_color       = rgbcolorf(.1,.1,.1);
	activate_color = rgbcolorf(0.,.783,0.);
	deactivate_color=rgbcolorf(1.,.392,.392);

	playing         = false;
	timerid         = 0;
	current_time    = 0;
	total_time      = 0;
	start_time      = 0;
	end_time        = start_time + 10;
	fps             = 12;
	current_fps     = 0;
	current_frame   = 0;
	speed           = 1;
	//last_time       = 0;

	show_fps        = true;
}

AnimationInterface::~AnimationInterface()
{
	//if (source_objs) source_objs->dec_count();
	if (sc) sc->dec_count();
	if (selection) { selection->dec_count(); selection=NULL; }
	if (font) font->dec_count();
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

enum AnimationInterfaceActions {
		ANIMATION_AddKeyframe
	};

Laxkit::ShortcutHandler *AnimationInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler("AnimationInterface");
	if (sc) return sc;

//	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler("AnimationInterface");

	sc->Add(ANIMATION_AddKeyframe,  'k',0,0,  "AddKeyframe",    _("Add keyframe for selected objects"),    NULL,0);


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
	if (action==ANIMATION_AddKeyframe) {
		 //add for all selected objects
		if (selection->n()==0) return 0; 
		return 0;
	}

	return 1;
}

int AnimationInterface::InterfaceOn()
{
	if (!laidout->globals.find("frame")) {
		SetCurrentFrame();
	}
	needtodraw=1;
	return 0;
}

int AnimationInterface::InterfaceOff()
{
	if (selection) { selection->dec_count(); selection=NULL; }
	
	if (timerid) app->removetimer(this,timerid);
	timerid=0;
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

/*! Assuming current_time is accurate, compute current_frame.
 * Set laidout->global["frame"] and laidout->global["frame_time"].
 * Returns current_frame. 
 */
int AnimationInterface::SetCurrentFrame()
{
	int old = current_frame;
	current_frame = (current_time - start_time) * fps;

	Value *v = laidout->globals.find("frame");
	if (!v) laidout->globals.push("frame", current_frame);
	else {
		if (v->type() == VALUE_Int) dynamic_cast<IntValue*>(v)->i = current_frame;
		else if (v->type() == VALUE_Real) dynamic_cast<DoubleValue*>(v)->d = current_frame;
		else { //was some other type, just wipe it out
			DBG cerr << "Warning! laidout->globals[frame] was not int or double!"<<endl;
			v = new IntValue(current_frame);
			laidout->globals.push("frame", v);
			v->dec_count();
		}
	}

	v = laidout->globals.find("anim_time");
	if (!v) laidout->globals.push("anim_time", current_time);
	else {
		if (v->type() == VALUE_Real) dynamic_cast<DoubleValue*>(v)->d = current_time;
		else { //was some other type, just wipe it out
			DBG cerr << "Warning! laidout->globals[anim_time] was not double!"<<endl;
			v = new DoubleValue(current_time);
			laidout->globals.push("anim_time", v);
			v->dec_count();
		}
	}

	v = laidout->globals.find("total_time");
	if (!v) laidout->globals.push("total_time", total_time);
	else {
		if (v->type() == VALUE_Real) dynamic_cast<DoubleValue*>(v)->d = total_time;
		else { //was some other type, just wipe it out
			DBG cerr << "Warning! laidout->globals[total_time] was not double!"<<endl;
			v = new DoubleValue(total_time);
			laidout->globals.push("total_time", v);
			v->dec_count();
		}
	}

	if (old != current_frame) {
		dynamic_cast<LaidoutViewport*>(curwindow)->TriggerFilterUpdates();
	}

	return current_frame;
}

Laxkit::MenuInfo *AnimationInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
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
//    menu->AddSep();
//	menu->AddItem(_("Render animation"), CLONEM_RenderAnim);
//	
//    return menu;

	return menu;
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

    const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
    if (!s) return 1;

    double d;
    int i;

	if (!strcmp(mes, "starttime")) {
		if (DoubleAttribute(s->str, &d) && d >= 0) {
			start_time = d;
			if (start_time > end_time) start_time = end_time;
			needtodraw = 1;
		}

	} else if (!strcmp(mes, "startframe")) {
		if (IntAttribute(s->str, &i) && i >= 0) {
			start_time = i / fps;
			if (start_time > end_time) start_time = end_time;
			needtodraw = 1;
		}
	} else if (!strcmp(mes, "curtime")) {
		if (DoubleAttribute(s->str, &d) && d >= 0) {
			if (d < start_time) d = start_time;
			else if (d > end_time) d = end_time;
			if (d != current_time) {
				current_time = d;
				total_time = d;
				SetCurrentFrame();
			}
			needtodraw = 1;
		}
		
	} else if (!strcmp(mes, "curframe")) {
		if (DoubleAttribute(s->str, &d) && d >= 0) {
			if (d < start_time * fps) d = start_time * fps;
			else if (d > end_time * fps) d = end_time * fps;
			if (d != current_time * fps) {
				current_time = d / fps;
				total_time = current_time;
				SetCurrentFrame();
			}
			needtodraw = 1;
		}
		
	} else if (!strcmp(mes, "endtime")) {
		if (DoubleAttribute(s->str, &d) && d >= 0) {
			end_time = d;
			if (end_time < start_time + 1/fps) end_time = start_time + 1/fps;
			needtodraw = 1;
		}

	} else if (!strcmp(mes, "endframe")) {
		if (DoubleAttribute(s->str, &d) && d >= 0) {
			end_time = d / fps;
			if (end_time < start_time) end_time = start_time;
			if (end_time < start_time + 1/fps) end_time = start_time + 1/fps;
			needtodraw = 1;
		}

	} else if (!strcmp(mes, "duration")) {
		if (DoubleAttribute(s->str, &d) && d >= 0) {
			if (d < 1/fps) d = 1/fps;
			end_time = start_time + d;
			needtodraw = 1;
		}
		
	} else if (!strcmp(mes, "setfps")) {
		if (DoubleAttribute(s->str, &d) && d > 0) {
			if (fps != d) {
				fps = d;
				if (playing) app->modifytimer(this, timerid, 1/fps*1000, -1);
				needtodraw = 1;
			}
		}

	} else return 1;

	return 0;
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
	if (!dp || !needtodraw) return 0;
	needtodraw=0;


	double boxh = INTERFACE_CIRCLE*uiscale;

	//  1. rewind to beginning 
	//  2. play backwards/faster backwards
	//  3. play/pause, reset to normal play speed
	//  4. faster forwards
	//  5. advance to end
	//
	//   1  2  3  4  5     start time                    end time
	//  |<  <  >  >  >|    ---------*----------------------------
	//       fps           start frame                  end frame

	if (firsttime==1) {
		firsttime=2;
		controlbox.minx = boxh;
		controlbox.maxx = boxh + 6*boxh;
		controlbox.miny = boxh;
		controlbox.maxy = boxh + boxh;

		timeline.minx = controlbox.maxx + boxh/2;
		timeline.maxx = dp->Maxx - boxh;
		timeline.miny = controlbox.miny;
		timeline.maxy = controlbox.maxy;

	} else if (firsttime==2) {
		 //remap control box size only, leave in same place
		firsttime=0;
		controlbox.maxx = controlbox.minx + 6*boxh;
		controlbox.maxy = controlbox.miny + boxh;

		double ww = timeline.maxx - timeline.minx;
		timeline.minx = controlbox.maxx + boxh/2;
		timeline.maxx = timeline.minx + ww;
		if (timeline.maxx > dp->Maxx - boxh) timeline.maxx = dp->Maxx - boxh;
		timeline.miny = controlbox.miny;
		timeline.maxy = controlbox.maxy;
	}


	 //--------draw control box------
	dp->DrawScreen();
	dp->font(font);

	 //draw control box outline
	double h = controlbox.maxy-controlbox.miny;
	dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
	dp->moveto(controlbox.minx + h/2,   controlbox.miny);
	dp->lineto(controlbox.maxx - h/2, controlbox.miny);
	dp->curveto(flatpoint(controlbox.maxx,controlbox.miny), flatpoint(controlbox.maxx,controlbox.maxy), flatpoint(controlbox.maxx-h/2,controlbox.maxy));
	dp->lineto(controlbox.minx + .5*h,controlbox.maxy);
	dp->curveto(flatpoint(controlbox.minx,controlbox.maxy),flatpoint(controlbox.minx,controlbox.miny),flatpoint(controlbox.minx+.5*h,controlbox.miny));
	dp->closed();
	dp->NewFG(bg_color);
	dp->fill(1);
	dp->NewFG(fg_color);
	dp->stroke(0);

	double x = controlbox.minx + h;
	double y = (controlbox.miny + controlbox.maxy)/2;
	dp->drawthing(x,y, h/3,h/3, 1, THING_To_Left);
	x += h;
	dp->drawthing(x,y, h/3,h/3, 1, THING_Triangle_Left);
	x += h;
	if (playing && speed==1) dp->drawthing(x,y, h/3,h/3, 1, THING_Pause);
	else dp->drawthing(x,y, h/3,h/3, 1, THING_Triangle_Right);
	x += h;
	dp->drawthing(x,y, h/3,h/3, 1, THING_Triangle_Right);
	x += h;
	dp->drawthing(x,y, h/3,h/3, 1, THING_To_Right);


	 //fps counter
	char scratch[30];
	if (show_fps) {
		dp->NewFG(hover == ANIM_FPS ? fg_color : coloravg(fg_color,bg_color));
		sprintf(scratch, "%s: %d", (playing ? "cur fps" : "fps"), (int)(playing ? current_fps : fps));
		dp->textout((controlbox.minx+controlbox.maxx)/2, controlbox.maxy, scratch,-1, LAX_TOP|LAX_HCENTER);
	}

	 //------------draw timeline ------------
	double tstart = timeline.minx;
	double tend   = timeline.maxx;
	y = (timeline.miny + timeline.maxy)/2;
	h = timeline.boxheight();
	dp->NewFG(coloravg(fg_color,bg_color));
	dp->drawrectangle(tstart,y-h*.05, tend-tstart,h*.1, 1);

	// start time
	dp->NewFG(hover == ANIM_Start_Time ? fg_color : coloravg(fg_color,bg_color));
	sprintf(scratch, "%g", start_time);
	dp->textout(timeline.minx, timeline.miny, scratch,-1, LAX_LEFT|LAX_BOTTOM);

	// start frame
	dp->NewFG(hover == ANIM_Start_Frame ? fg_color : coloravg(fg_color,bg_color));
	sprintf(scratch, "%d", (int)(start_time * fps));
	dp->textout(timeline.minx, timeline.maxy, scratch,-1, LAX_LEFT|LAX_TOP);
	
	// end time
	dp->NewFG(hover == ANIM_End_Time ? fg_color : coloravg(fg_color,bg_color));
	sprintf(scratch, "%g", end_time);
	dp->textout(timeline.maxx, timeline.miny, scratch,-1, LAX_RIGHT|LAX_BOTTOM);

	// end frame
	dp->NewFG(hover == ANIM_End_Frame ? fg_color : coloravg(fg_color,bg_color));
	sprintf(scratch, "%d", int(end_time * fps));
	dp->textout(timeline.maxx, timeline.maxy, scratch,-1, LAX_RIGHT|LAX_TOP);

	 //timeline ball
	dp->NewFG(coloravg(fg_color,bg_color));
	double progress = (current_time - start_time)/(end_time - start_time);
	dp->drawthing(tstart+progress*(tend-tstart), y, h/3,h/3, 2, THING_Circle);
	//dp->drawthing(tstart+progress*(tend-tstart), y, h/3,h/3, 1, THING_Circle);

	if (hover == ANIM_Timeline_Size) {
		dp->drawthing(timeline.maxx + font->textheight(), (timeline.miny+timeline.maxy)/2,
						font->textheight(),font->textheight(), 1, THING_Double_Arrow_Horizontal);
	}

	dp->DrawReal();

	return 0;
}

int AnimationInterface::scan(int x,int y, int *i)
{
	if (controlbox.boxcontains(x,y)) {
		double h = controlbox.maxy-controlbox.miny;
		double xx = (x - controlbox.minx - h/2) / h;
		//y -= controlbox.miny;

		if (xx<1) return ANIM_Rewind;
		if (xx<2) return ANIM_Backwards;
		if (xx<3) return ANIM_Play;
		if (xx<4) return ANIM_Faster;
		return ANIM_To_End;
	}

	double th = font->textheight();

	if (x >= controlbox.minx && x <= controlbox.maxx && y >= controlbox.maxy && y <= controlbox.maxy+th)
		return ANIM_FPS;

	//scan for on timeline
	if (timeline.boxcontains(x,y)) {
		double curpos = (current_time - start_time) /(end_time - start_time) * timeline.boxwidth() + timeline.minx;
		if (fabs(x-curpos) < 10) return ANIM_Current;
		return ANIM_Timeline;
	}

	if (x >= timeline.minx && x <= timeline.maxx) {
		if (y <= timeline.miny && y >= timeline.miny-th) {
			if (x < timeline.minx + 5*th) return ANIM_Start_Time;
			if (x > timeline.maxx - 5*th) return ANIM_End_Time;
		} else if (y >= timeline.maxy && y <= timeline.maxy+th) {
			if (x < timeline.minx + 5*th) return ANIM_Start_Frame;
			if (x > timeline.maxx - 5*th) return ANIM_End_Frame;
		}
	}

	if (x >= timeline.maxx && x < timeline.maxx+th && y >= timeline.miny && y <= timeline.maxy)
		return ANIM_Timeline_Size;

	// ANIM_Current_Time, //on the number
	// ANIM_Current_Frame,

	return ANIM_None;
}

int AnimationInterface::Idle(int tid, double delta)
{
	clock_gettime(CLOCK_REALTIME, &cur_timespec);
	delta = (cur_timespec.tv_nsec - last_time.tv_nsec) / 1000000000.0 + cur_timespec.tv_sec - last_time.tv_sec;
	last_time = cur_timespec;

	current_fps = 1/delta;
	current_time += delta * speed;
	total_time += delta * speed;

	if (current_time > end_time) {
		current_time = start_time;
	} else if (current_time < start_time) {
		current_time = end_time;
	}

	SetCurrentFrame();
	needtodraw=1;

	DBG cerr << "AnimationInterface Frame #"<<current_frame<<",  current fps: "<<current_fps<<endl;
	return 0;
}

bool AnimationInterface::Play(int on)
{
	//clock_t curtime = times(NULL);

	bool play=playing;

	if (on<0) play=!play;
	else if (on==0) play=false;
	else play=true;

	if (play == playing) return playing;
	playing = play;

	if (playing) {
		 //set up timer
		timerid = app->addtimer(this, 1/fps*1000, 1/fps*1000, -1);
		clock_gettime(CLOCK_REALTIME, &last_time);
		if (current_time == 0) total_time = 0;
		SetCurrentFrame();
	} else {
		 //remove timer
		if (timerid) app->removetimer(this,timerid);
		timerid=0;
	}

	needtodraw=1;
	PostMessage(playing ? _("Playing...") : _("Paused"));
	return playing;
}

int AnimationInterface::Mode(int newmode)
{
	if (newmode==mode) return mode;


	return mode;
}

void AnimationInterface::UpdateHoverMessage(int hover)
{
	if      (hover == ANIM_None)          PostMessage(" ");
	else if (hover == ANIM_Play)          PostMessage(playing && speed==1 ? _("Pause") : playing ? _("Play normal speed") : _("Play"));
	else if (hover == ANIM_Rewind)        PostMessage(_("Jump to start"));
	else if (hover == ANIM_To_End)        PostMessage(_("Jump to end"));
	else if (hover == ANIM_Faster)        PostMessage(_("Play faster"));
	else if (hover == ANIM_Backwards)     PostMessage(speed>0 ? _("Play backwards") : _("Play faster backwards"));
	else if (hover == ANIM_Timeline)      PostMessage(_("Timeline"));
	else if (hover == ANIM_Timeline_Size) PostMessage(_("Timeline Size"));
	else if (hover == ANIM_Current_Time)  PostMessage(_("Current Time"));
	else if (hover == ANIM_Current_Frame) PostMessage(_("Current Frame"));
	else if (hover == ANIM_Move_Strip)    PostMessage(_("Move Strip"));
	else if (hover == ANIM_Start_Time)    PostMessage(_("Start Time"));
	else if (hover == ANIM_Start_Frame)   PostMessage(_("Start Frame"));
	else if (hover == ANIM_End_Time)      PostMessage(_("End Time"));
	else if (hover == ANIM_End_Frame)     PostMessage(_("End Frame"));
	else if (hover == ANIM_Duration)      PostMessage(_("Duration"));
	else if (hover == ANIM_FPS)           PostMessage(_("FPS"));
	else PostMessage(" ");
}

int AnimationInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 1;


	 // else click down on something for overlay
	int over=scan(x,y,NULL);
	if (over != ANIM_None) {
		if (state & ShiftMask) over = ANIM_Move_Strip;
		buttondown.down(d->id,LEFTBUTTON,x,y, over,state);
		return 0;
	}

	return 1;
}


int AnimationInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;


	int firstover = ANIM_None;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &firstover);
	int over = scan(x,y,NULL);

	if (firstover==over && over==ANIM_Play) {
		if (playing && speed != 1) speed = 1;
		else Play(-1);
		UpdateHoverMessage(over);
		return 0;

	} else if (firstover==over && over==ANIM_Rewind) {
		current_time = 0;
		total_time = 0;
		SetCurrentFrame();
		needtodraw=1;
		return 0;

	} else if (firstover==over && over==ANIM_To_End) {
		current_time = end_time;
		total_time = current_time;
		SetCurrentFrame();

		needtodraw=1;
		return 0;

	} else if (firstover==over && over==ANIM_Faster) {
		if (speed < 0) speed = 1;
		else if (!dragged) speed *= 1.3;
		return 0;

	} else if (firstover==over && over==ANIM_Backwards) {
		if (speed > 0) speed = -1;
		if (!dragged) speed *= 1.3;
		speed = -1;

	} else if (firstover==over && over==ANIM_Timeline) {
		if (dragged <= DraggedThreshhold()) {
			double t = (x-timeline.minx) / timeline.boxwidth() * (end_time - start_time) + start_time;
			current_time = t;
			total_time = t;
			needtodraw=1;
		}
		return 0;
	}

	if (dragged < DraggedThreshhold()) {
		char str[30];
		const char *label = nullptr;
		const char *mes = nullptr;

		if (over == ANIM_Start_Time) {
			label = _("Start time");
			mes = "starttime";
			sprintf(str, "%g", start_time);

		} else if (over == ANIM_Start_Frame) {
			label = _("Start frame");
			mes = "startframe";
			sprintf(str, "%d", int(start_time * fps));

		} else if (over == ANIM_Current_Time) {
			label = _("Current time");
			mes = "curtime";
			sprintf(str, "%g", current_time);

		} else if (over == ANIM_Current_Frame) {
			label = _("Current frame");
			mes = "curframe";
			sprintf(str, "%d", int(current_time * fps));

		} else if (over == ANIM_End_Time) {
			label = _("End time");
			mes = "endtime";
			sprintf(str, "%g", end_time);

		} else if (over == ANIM_End_Frame) {
			label = _("End frame");
			mes = "endframe";
			sprintf(str, "%d", int(end_time * fps));

		} else if (over == ANIM_Duration) {
			label = _("Duration (seconds)");
			mes = "duration";
			sprintf(str, "%g", end_time - start_time);

		} else if (over == ANIM_FPS) {
			label = _("FPS");
			mes = "setfps";
			sprintf(str, "%g", fps);
		}

		if (mes) {
			DoubleBBox bounds;
			double th = font->textheight();
			bounds.minx = x - 5 * th;
			bounds.maxx = x + 5 * th;
			bounds.miny = y - th;
			bounds.maxy = y + th;			

			viewport->SetupInputBox(object_id, label, str, mes, bounds);
		}
	}

	return 0;
}

int AnimationInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{

	int i=-1;
	int over=scan(x,y,&i);
	DBG cerr <<"over box: "<<over<<endl;

	if (!buttondown.any()) {
		if (hover!=over) {
			needtodraw=1;
			UpdateHoverMessage(over);
		}
		hover=over;
		return 0;
	}


//	 //button is down on something...
	int lx,ly, action = ANIM_None;
	buttondown.move(mouse->id,x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &action);

	if (action == ANIM_Current || action == ANIM_Timeline) {
		current_time += (x-lx) / timeline.boxwidth() * (end_time - start_time);

		if (current_time > end_time) current_time = end_time;
		else if (current_time < start_time) current_time = start_time;
		total_time = current_time;
		SetCurrentFrame();

		needtodraw=1;
		return 0;

	} else if (action == ANIM_Faster) {
		//drag to change speed possible to .5, for instance
		if (speed < 0) speed = -speed;
		if (x-lx) {
			if (x>lx) speed *= 1 + (x-lx)*.1;
			else speed /= 1 + (lx-x)*.1;
		}

	} else if (action == ANIM_Backwards) {
		if (speed > 0) speed = -speed;
		if (x-lx) {
			if (x>lx) speed /= 1 + (x-lx)*.1;
			else speed *= 1 + (lx-x)*.1;
		}

	} else if (action == ANIM_Move_Strip) {
		flatpoint dp = flatpoint(x,y) - flatpoint(lx,ly);
		controlbox.minx += dp.x;
		controlbox.miny += dp.y;
		controlbox.maxx += dp.x;
		controlbox.maxy += dp.y;
		timeline.minx += dp.x;
		timeline.miny += dp.y;
		timeline.maxx += dp.x;
		timeline.maxy += dp.y;
		needtodraw = 1;

	} else if (action == ANIM_Timeline_Size) {
		timeline.maxx += x - lx;
		if( timeline.maxx < timeline.minx + 10*font->textheight())
			timeline.maxx = timeline.minx + 10*font->textheight();
		needtodraw = 1;
	}


	return 0;
}

int AnimationInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int over = scan(x,y,NULL);
	if (over == ANIM_Timeline && (state & ControlMask)) {
		double cur = timeline.maxx - timeline.minx;
		cur /= .9;
		timeline.maxx = timeline.minx + cur;
		needtodraw = 1;
		return 0;
	}
	return 1;
}

int AnimationInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	int over = scan(x,y,NULL);
	if (over == ANIM_Timeline && (state & ControlMask)) {
		double cur = timeline.maxx - timeline.minx;
		cur *= .9;
		if (cur > font->textheight() * 10) {
			timeline.maxx = timeline.minx + cur;
			needtodraw = 1;
		}
		return 0;
	}

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

