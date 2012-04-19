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
// Copyright (C) 2011 by Tom Lechner
//

#ifndef GLOVERLAY_H
#define GLOVERLAY_H


#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include <string>

#include <lax/doublebbox.h>


namespace Polyptych {

enum ActionType {
	ACTION_None,
	ACTION_Roll,
	ACTION_Rotate,
	ACTION_Zoom,
	ACTION_Pan,
	ACTION_Toggle_Mode,
	ACTION_Shift_Texture,
	ACTION_Rotate_Texture,
	ACTION_Unwrap_Angle,
	ACTION_Unwrap,
	ACTION_Reseed,
	ACTION_Paper,
	ACTION_AddPaper
};

enum OverlayType {
	OVERLAY_Just_Display,  //only for drawing, no other interaction
	OVERLAY_Mode,         //info1 has mode group, only one from each group can be highlighted at any one time
	OVERLAY_Button,      //triggers something, but does not stay highlighted
	OVERLAY_Slider,     //move info1 from 0 to 1
	OVERLAY_H_Pan,       //center is 0, allow info1 change from -1 to 1
	OVERLAY_V_Pan,       //center is 0, allow info1 change from -1 to 1
	OVERLAY_Menu_Trigger //popup menu clump with id info1
};

class Overlay : public Laxkit::DoubleBBox
{
 public:
	int id;
	int type; //see OverlayType
	int gravity_group; // 123
					   // 456  each group is piled up together and put on that edge. 
					   // 789  group >=10 is popup menu
	double info1, info2;

	ActionType action;
	int index;
	int group;

	double info; //for sliders
	GLuint call_id; // the calllist, if any
	std::string text;

	Overlay(const char *str, ActionType a,int otype=OVERLAY_Button,int oindex=-1);
	virtual ~Overlay();

	virtual void Draw(int state);
	virtual int Create(int callnumber);
	virtual int Install(int callnumber);
	virtual const char *Text();
	virtual int PointIn(int x,int y);

};


} //namespace Polyptych

#endif

