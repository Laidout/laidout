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

#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

#include "glbase.h"
#include "gloverlay.h"
#include "poly.h"
#include "polyrender.h"


namespace Polyptych {


//--------------------------- FaceData -------------------------------

enum FaceDataType {
	FACE_Color,
	FACE_Image,
	FACE_Panorama
};

class FaceDataObj
{
  public:
	FaceDataType type;
	FaceDataObj(FaceDataType datatype);
	virtual ~FaceDataObj() {}
	virtual const char *Str()=0;
};

class FaceData : public Laxkit::anObject, public Laxkit::PtrStack<FaceDataObj>
{
  public:
	FaceData();
	virtual ~FaceData() {}
};

//--------------------------- ColorFace -------------------------------
class ColorFace : public FaceDataObj
{
  public:
	double red,green,blue,alpha;
	ColorFace(double r,double g, double b,double a);
};


//--------------------------- ImageFace -------------------------------
class ImageFace : public FaceDataObj
{
  public:
	char *filename;
	double matrix[6];
	double width,height;
	ImageFace(const char *file);
};


//--------------------------- PanoramaInfo -------------------------------
class PanoramaInfo : public Laxkit::anObject
{
  public:
	char *filename;

	unsigned char *spheremap_data;
	unsigned char *spheremap_data_rotated;
	int spheremap_width;
	int spheremap_height;
	basis extra_basis;

	PanoramaInfo(const char *file);
};

//--------------------------- PanoramaFace -------------------------------
class PanoramaFace : public FaceDataObj
{
  public:
	PanoramaInfo *panorama;
	PanoramaFace(PanoramaInfo *pano);
};


//--------------------------- HedronWindow -------------------------------

class HedronWindow : public Laxkit::anXWindow
{
  protected:
	int mbdown,rbdown;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
  public:
	GLfloat movestep;
	GLuint spheretexture, flattexture;
	int rendermode, mode, oldmode;
	int autorepeat, curobj;
	double fovy;
	int view;
	double cylinderscale;
	double fontsize;
	double pad;
	int touchmode;
	int helpoffset;

	int draw_edges;
	int draw_seams;
	int draw_axes;
	int draw_info;
	int draw_texture;
	int draw_overlays;
	int draw_papers;

	char *polyptychfile;
	char *polyhedronfile;
	char *spherefile;
	unsigned char *spheremap_data;
	unsigned char *spheremap_data_rotated;
	int spheremap_width;
	int spheremap_height;
	Polyhedron *poly;
	Thing *hedron;
	Laxkit::RefPtrStack<Net> nets;
	Laxkit::PtrStack<PaperBound> papers;
	PaperBound default_paper;
	int currentpaper;
	basis extra_basis;
	char *consolefontfile;
	FTFont *consolefont;

	char *currentmessage, *lastmessage;
	int messagetick;

	int firsttime;
	Laxkit::ButtonDownInfo buttondown;

	Net *currentnet;
	double unwrapangle;

	spacevector tracker;
	spaceline pointer; //points where mouse points

	int currentpotential;   //index in currentnet->faces, or -1
	int currentface;   //index in Polyhedron of current face, or -1
	int currentfacestatus;

	int mouseover_overlay; //which overlay mouse is currently over
	int mouseover_index;
	int mouseover_group;  //which section mouseover_overlay is index in
	int mouseover_paper;
	int grab_overlay;     //if lbdown on an overlay, all input corresponds to that one
	ActionType active_action; //determined by current overlay, affects behavior of left mouse button
	Laxkit::PtrStack<Overlay> overlays;
	Laxkit::PtrStack<Overlay> paperoverlays;

	 //gl objects, cameras, and lights
	Laxkit::PtrStack<Thing> things;
	Thing camera_shape;
	Laxkit::PtrStack<EyeType> cameras;
	int current_camera;
	Laxkit::PtrStack<Light> lights;
	struct Material lightmaterial;
	void setlighting(void);


	HedronWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		 		 int xx,int yy,int ww,int hh,int brder,
				 Polyhedron *newpoly);
	virtual ~HedronWindow();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *whattype() { return "HedronWindow"; }
	virtual int MoveResize(int x,int y,int w,int h);
	virtual int Resize(int w,int h);
	virtual int event(XEvent *e);
	virtual int init();

	virtual void installOverlays();
	virtual void remapPaperOverlays();
	virtual void placeOverlays();
	virtual Overlay *scanOverlays(int x,int y, int *action,int *index,int *group);
	virtual double getExtent(const char *str,int len, double *width,double *height);

	virtual void Refresh();
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *kb);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual void newMessage(const char *str);
	virtual double FontAndSize(const char *fontfile, double newfontsize);


	 //hedron gl mapping
	virtual void installSpheremapTexture(int definetid);
	virtual void triangulate(spacepoint p1 ,spacepoint p2 ,spacepoint p3,
							 spacepoint p1o,spacepoint p2o,spacepoint p3o,
							 int n);
	virtual void mapPolyhedronTexture(Thing *thing);
	virtual Thing *makeGLPolyhedron();

	virtual void makecameras(void);

	 //drawing
	virtual void reshape (int w, int h);
	virtual void transparentFace(int face, double r, double g, double b, double a);
	virtual void drawRect(Laxkit::DoubleBBox &box, 
							double bg_r, double bg_g, double bg_b,
							double line_r, double line_g, double line_b,
							double alpha);
	virtual void drawPotential(Net *net, int face);
	virtual void Refresh3d();
	virtual void drawHelp();

	 //net building
	virtual double edgeScaleFromBox();
	virtual int Reseed(int original);
	virtual int recurseUnwrap(Net *netf, int fromneti, Net *nett, int toneti);
	virtual void recurseCache(Net *net,int neti);
	virtual int unwrapTo(int from,int to);
	virtual Net *establishNet(int original);
	virtual int removeNet(int netindex);
	virtual int removeNet(Net *net);
	virtual Net *findNet(int id);
	virtual void remapCache(int start=-1, int end=-1);

	 //mouse position
	virtual int findCurrentPotential();
	virtual int findCurrentFace();
	virtual int scanPaper(int x,int y, int &index);
	virtual flatpoint pointInNetPlane(int x,int y);

	 //misc
	virtual int changePaper(int towhich,int index);

	 //input/output
	virtual int Save(const char *saveto);
	virtual void UseGenericImageData(double fg_r=-1, double fg_g=-1, double fg_b=-1,  double bg_r=-1, double bg_g=-1, double bg_b=-1);
	virtual int installImage(const char *file);
	virtual int installPolyhedron(const char *file);
	virtual int installPolyhedron(Polyhedron *ph);
	virtual int AddNet(Net *net);
	virtual int AddPaper(PaperBound *paper);
	virtual Polyhedron *defineCube();
};

} //namespace Polyptych



#endif

