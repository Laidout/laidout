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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef INTERFACES_SHAPEINTERFACE_H
#define INTERFACES_SHAPEINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/refptrstack.h>

#include "../laidout.h"


namespace Laidout {




//---------------------------------- ShapeInfo -----------------------------------------
class ShapeInfo : public Laxkit::anObject
{
  public:
	char *name;
	int numsides;
	int innerpoints; //1 or 0
	double round1,round2;
	double edgelength;
	
	ShapeInfo();
	virtual ~ShapeInfo();
};


//------------------------------------- ShapeInterface --------------------------------------

class ShapeInterface : public LaxInterfaces::anInterface
{
  protected:
	Laxkit::ButtonDownInfo buttondown;

	unsigned long color_arrow, color_num;


	ShapeInfo *shapeinfo;

	PathsData *possible;
	Laxkit::RefPtrStack<LaxInterfaces::PathsData> faces;

	int showdecs;
	int firsttime;

	virtual int scan(int x,int y);
  public:
	unsigned long shape_style;//options for interface

	ShapeInterface(int nid=0,Laxkit::Displayer *ndp=NULL);
	ShapeInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~ShapeInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "Shape"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ShapeInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
};



} //namespace Laidout

#endif

