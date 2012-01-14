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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef ACTIONAREA_H
#define ACTIONAREA_H


#include <lax/doublebbox.h>

//------------------------------------- ActionArea ---------------------------

enum ActionAreaType {
	AREA_Handle,
	AREA_Slider,
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
	flatpoint *outline, offset, hotspot;
	int npoints;
	int visible; //1 for yes and filled, 2 for selectable, but not drawn,3 for outline only
	int hidden; //skip checks for this one
	unsigned long color;

	int real; //use real coordinates, not screen coordinates
	int action; //id for the action this overlay corresponds to
	int category; //extra identifier
	int type; //basic type this overlay is: handle (movable), slider, button, display only, pan, menu trigger
	int PointIn(double x,double y);

	ActionArea(int what,int ntype,const char *txt,const char *ntooltip,
			   int isreal,int isvisible,unsigned long ncolor,int ncategory);
	virtual ~ActionArea();
	virtual flatpoint *Points(flatpoint *pts, int n, int takethem);
	virtual void FindBBox();
	virtual flatpoint Position() { return offset+hotspot; }
	virtual void Position(double x,double y,int which=3);
};


#endif

