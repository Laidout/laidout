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

#include <lax/lists.h>
#include <lax/laximages.h>
#include <lax/dump.h>
#include <lax/transformmath.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/selection.h>

#include "../calculator/values.h"
#include "../dataobjects/group.h"
#include "../language.h"
#include "../viewwindow.h"

#include <iostream>


namespace Laidout {



//--------------------- ObjectTimeline ---------------------------------------

class KeyFrame
{
  public:
	double time;
	Value *value;

	KeyFrame() { time=0; value=NULL; }
	KeyFrame(double t,Value *nvalue, bool absorbcount) { time=t; value=value; if (!absorbcount && value) value->inc_count(); }
	~KeyFrame() { if (value) value->dec_count(); }
};

class KeyFrameProperty
{
  public:
	double cycle_period; //if nonzero, then repeat (first time)+cycle_period, using first point as final point
	std::string property; //field name within an object
	Laxkit::PtrStack<KeyFrame> frames;
	//AnimationPath interpolation_curve;

	virtual Value *GetValue(double time);
};

/*! \class ObjectTimeline
 * Objects can have one of these as an extra property.
 * All keyframes are stored within KeyFrameProperty objects, which in turn
 * store a stack of KeyFrame instances at specific times.
 */
class ObjectTimeline : public Laxkit::anObject
{
  public:
	DrawableObject *owner;
	double start; //of life of object. hidden outside of range
	double end;
	Laxkit::PtrStack<KeyFrame> keyframes;
};

class SceneInfo : public Laxkit::anObject
{
  public:
	char *name;
	double length_seconds;
	double offset_start;
};

//------------------------------------- AnimationInterface --------------------------------------

class AnimationInterface : public LaxInterfaces::anInterface
{
  protected:

	int firsttime;
	int hover;
	int hoveri;
	int mode;

	//ObjectTimeline *global_time;
	double animation_length; //in seconds
	double ui_first_time, ui_last_time;
	double current_time;
	double fps; //0 means continuous
	double current_fps; //==fps*speed
	double speed; //1==normal
	int timerid;
	int currentframe;
	bool playing;
	bool showdecs;

	LaxInterfaces::Selection *selection;

	double uiscale;
	Laxkit::DoubleBBox box;

	unsigned int bg_color;
	unsigned int hbg_color;
	unsigned int fg_color;
	unsigned int activate_color;
	unsigned int deactivate_color;


	virtual int scan(int x,int y, int *i);
	virtual void UpdateHoverMessage(int hover);

	virtual bool Play(int on); //-1==toggle


	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

  public:
	unsigned long animation_style;//options for interface

	AnimationInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~AnimationInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "Animation"; }
	virtual const char *Name();
	virtual const char *whattype() { return "AnimationInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype) { return 0; }

	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual int  Idle(int tid=0);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual int Mode(int newmode);

	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
};



} //namespace Laidout

