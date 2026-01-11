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
// Copyright (C) 2025 by Tom Lechner
//

#ifndef NETINTERFACE_H
#define NETINTERFACE_H


#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

#include "netimposition.h"
#include "../interfaces/paperinterface.h"


namespace Laidout {



//--------------------------- PanoramaInfo -------------------------------

/*! Mapping of an equirectangular image onto a NetImposition.
 */
class PanoramaInfo : public Laxkit::anObject
{
  public:
	char *filename;
	int spheremap_width;
	int spheremap_height;
	Laxkit::Basis extra_basis;

	unsigned char *spheremap_data;
	unsigned char *spheremap_data_rotated;

	PanoramaInfo(const char *file);

	int SavePolyptych(const char *saveto);
	int RenderPanorama(Document *doc, const Laxkit::Basis &basis_tweak);
};


//--------------------------- NetInterface -------------------------------

class NetInterface : virtual public ImpositionInterface
{
  protected:
  	PaperInterface *paper_interface = nullptr;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

  public:
	double cylinderscale;
	int touchmode;

	Laxkit::LaxFont *font;

	// viewable things
	bool draw_edges;
	bool draw_unwrap_path;
	bool draw_axes;
	bool draw_info;
	bool draw_texture;
	bool draw_overlays;
	bool draw_papers;

	Document *doc = nullptr;
	NetImposition *original_netimp;
	NetImposition *current_netimp;
	AbstractNet *abstract_net;
	Net *currentnet;
	int current_paper_spread = -1;

	Polyhedron *poly;

	Laxkit::RefPtrStack<Net> nets;
	PaperGroup *papers;
	PaperBox *default_paper;

	int currentpotential; //index in currentnet->faces, or -1
	int currentface;      //index in Polyhedron of current face, or -1
	int currentfacestatus;


	// messages and overlays
	Utf8String currentmessage, lastmessage;

	double pad; //for overlay text
	int mouseover_overlay; //which overlay mouse is currently over
	int mouseover_index;
	int mouseover_group;  //which section mouseover_overlay is index in
	int mouseover_paper;
	int grab_overlay;     //if lbdown on an overlay, all input corresponds to that one
	ActionType active_action; //determined by current overlay, affects behavior of left mouse button

	Laxkit::PtrStack<Overlay> overlays;
	Laxkit::PtrStack<Overlay> paperoverlays;


	NetInterface(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		 		 int xx,int yy,int ww,int hh,int brder,
				 NetImposition *new_net);
	virtual ~NetInterface();
	virtual const char *whattype() { return "NetInterface"; }
	virtual const char *IconId() { return "NetInterface"; }
	virtual const char *Name();
	virtual int init();
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	virtual void Refresh();
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *kb);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	// virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	// virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	// virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	// virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	
	// // hedron gl mapping
	// virtual void installSpheremapTexture(int definetid);
	// virtual void triangulate(Laxkit::spacepoint p1 ,Laxkit::spacepoint p2 ,Laxkit::spacepoint p3,
	// 						 Laxkit::spacepoint p1o,Laxkit::spacepoint p2o,Laxkit::spacepoint p3o,
	// 						 int n);
	// virtual void mapPolyhedronTexture(Thing *thing);
	// virtual Thing *makeGLPolyhedron();

	
	// drawing
	virtual void reshape (int x, int y, int w, int h);
	virtual void transparentFace(int face, double r, double g, double b, double a);
	virtual void drawPotential(Net *net, int face);
	
	// net building
	virtual int Reseed(int original);
	virtual int recurseUnwrap(Net *netf, int fromneti, Net *nett, int toneti);
	virtual void recurseCache(Net *net,int neti);
	virtual int unwrapTo(int from,int to);
	virtual Net *establishNet(int original);
	virtual int removeNet(int netindex);
	virtual int removeNet(Net *net);
	virtual Net *findNet(int id);

	// mouse position
	virtual int findCurrentPotential();
	virtual int findCurrentFace();
	virtual int scanPaper(int x,int y, int &index);
	virtual Laxkit::flatpoint pointInNetPlane(int x,int y);

	// misc
	virtual int changePaper(int towhich,int index);

	// input/output
	virtual int SavePolyptych(const char *saveto);
	virtual void UseGenericImageData(double fg_r=-1, double fg_g=-1, double fg_b=-1,  double bg_r=-1, double bg_g=-1, double bg_b=-1);
	virtual int InstallImage(const char *file);
	virtual int InstallPolyhedron(const char *file);
	virtual int InstallPolyhedron(Polyhedron *ph);
	virtual int SetFiles(const char *hedron, const char *image, const char *project);
	virtual int AddNet(Net *net);
	virtual int AddPaper(PaperBound *paper);
	virtual Polyhedron *defineCube();

	// from ImpositionEditor:
    virtual const char *ImpositionType() { return "NetImposition"; }
    virtual Imposition *GetImposition();
    virtual int SetTotalDimensions(double width, double height);
    virtual int GetDimensions(double &width, double &height); //Return default paper size
    virtual int SetPaper(PaperStyle *paper); // installs duplicate of paper
    virtual int UseThisDocument(Document *doc);
    virtual int UseThisImposition(Imposition *imp);

    virtual int ShowThisPaperSpread(int index);
    virtual void ShowSplash(int yes);
};

} //namespace Laidout



#endif

