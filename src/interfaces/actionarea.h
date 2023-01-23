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
// Copyright (C) 2012-2013 by Tom Lechner
//
#ifndef ACTIONAREA_H
#define ACTIONAREA_H


#include <lax/doublebbox.h>


namespace Laidout {


//------------------------------------- ActionArea ---------------------------

enum ActionAreaType {
	AREA_Handle,
	AREA_Slider,
	AREA_H_Slider,
	AREA_V_Slider,
	AREA_Button,
	AREA_Display_Only,
	AREA_Mode,
	AREA_H_Pan,
	AREA_V_Pan,
	AREA_Menu_Trigger
};

class ActionArea : public Laxkit::DoubleBBox
{
  public:
	char *tip;
	char *text;
	Laxkit::flatpoint offset;
	Laxkit::flatpoint xdirection;
	Laxkit::flatpoint hotspot;
	Laxkit::flatpoint tail;
	int usetail;

	Laxkit::flatpoint *outline;
	int npoints;

	unsigned long color;
	unsigned long color_text;
	int visible; //1 for yes and filled, 2 for selectable, but not drawn,3 for outline only
	int hidden; //skip checks for this one
	int real; //0=screen coordinates, 1=real coordinates, 2=position is real, but outline is screen

	int action; //id for the action this overlay corresponds to
	int mode; //mode that must be active for this area to be active
	int category; //extra identifier
	int type; //basic type this overlay is: handle (movable), slider, button, display only, pan, menu trigger

	ActionArea(int what,int ntype,const char *txt,const char *ntooltip,
			   int isreal,int isvisible,unsigned long ncolor,int ncategory,unsigned long ntcolor=0);
	virtual ~ActionArea();
	virtual Laxkit::flatpoint *Points(Laxkit::flatpoint *pts, int n, int takethem);
	virtual void FindBBox();
	virtual Laxkit::flatpoint Position();
	virtual void Position(double x,double y,int which=3);
	virtual int PointIn(double x,double y);
	virtual Laxkit::flatpoint Center();
	virtual void Tail(Laxkit::flatpoint ntail) { tail=ntail; usetail=1; }
	virtual void SetRect(double x,double y,double w,double h);
};


} //namespace Laidout

#endif

