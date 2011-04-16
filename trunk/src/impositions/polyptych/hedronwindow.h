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

#ifndef POLYPTYCH_HEDRONWINDOW_H
#define POLYPTYCH_HEDRONWINDOW_H


#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <FTGL/ftgl.h>

#include <lax/anxwindow.h>
#include <lax/buttondowninfo.h>

#include "glbase.h"
#include "gloverlay.h"
#include "poly.h"
#include "polyrender.h"


namespace Polyptych {

//--------------------------- HedronWindow -------------------------------


class HedronWindow : public anXWindow
{
 public:
	GLfloat movestep;
	GLuint spheretexture, flattexture;
	int rendermode, mode, oldmode;
	int autorepeat, curobj;
	flatpoint leftb;
	double fovy;
	int view;
	int draw_edges;
	int draw_seams;
	double cylinderscale;
	double fontsize;
	double pad;

	RefPtrStack<Net> nets;

	ButtonDownInfo buttondown;

	char *currentmessage, *lastmessage;
	int messagetick;

	int firsttime;
	int rbdown,mbdown;

	Net *currentnet;
	double unwrapangle;

	spacevector tracker;
	spaceline pointer; //points where mouse points

	int currentpotential;   //index in currentnet->faces, or -1
	int currentface;   //index in Polyhedron of current face, or -1
	int currentfacestatus;

	int draw_axes;
	int draw_info;
	int draw_overlays;
	int mouseover_overlay; //which overlay mouse is currently over
	int grab_overlay;     //if lbdown on an overlay, all input corresponds to that one
	ActionType active_action; //determined by current overlay, affects behavior of left mouse button
	PtrStack<Overlay> overlays;

	 //gl objects, cameras, and lights
	PtrStack<Thing> things;
	Thing camera_shape;
	PtrStack<EyeType> cameras;
	int current_camera=-1;
	PtrStack<Light> lights;
	struct Material lightmaterial;
	void setlighting(void);


	HedronWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		 		 int xx,int yy,int ww,int hh,int brder);
	~HedronWindow();
	virtual int MoveResize(int x,int y,int w,int h);
	virtual int Resize(int w,int h);
	virtual int event(XEvent *e);
	virtual int init();
	virtual void installOverlays();
	virtual void placeOverlays();

	virtual void Refresh();
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *kb);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *mouse);
	virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse);
	virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *mouse);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *mouse);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *mouse);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse);
	virtual int Event(const EventData *data,const char *mes);
	void newMessage(const char *str);


	 //hedron gl mapping
	void triangulate(spacepoint p1 ,spacepoint p2 ,spacepoint p3,
							 spacepoint p1o,spacepoint p2o,spacepoint p3o,
							 int n);
	void mapPolyhedronTexture(Thing *thing, Polyhedron *poly);
	Thing *makePolyhedron(Polyhedron *poly);

	void makecameras(void);

	 //drawing
	void reshape (int w, int h);
	void transparentFace(int face, double r, double g, double b, double a);
	void drawRect(DoubleBBox &box, 
							double bg_r, double bg_g, double bg_b,
							double line_r, double line_g, double line_b,
							double alpha);
	void drawPotential(Net *net, int face);
	virtual void drawbg();
	virtual void drawHelp();

	 //net building
	void edgeScaleFromBox();
	int Reseed(int original);
	int recurseUnwrap(Net *netf, int fromneti, Net *nett, int toneti);
	void recurseCache(Net *net,int neti);
	int unwrapTo(int from,int to);
	Net *establishNet(int original);
	int removeNet(int netindex);
	int removeNet(Net *net);
	Net *findNet(int id);
	int findCurrentPotential();
	virtual int findCurrentFace();
	virtual void remapCache(int start=-1, int end=-1);

	 //input/output
	virtual int Save(const char *saveto);
	virtual int installImage(const char *file);
	virtual int installPolyhedron(const char *file);
};

} //namespace Polyptych



#endif

