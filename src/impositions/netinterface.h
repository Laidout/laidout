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
#include <lax/utf8string.h>

#include "netimposition.h"
#include "../interfaces/paperinterface.h"


namespace Laidout {



//--------------------------- PanoramaInfo -------------------------------

/*! Mapping of an equirectangular image onto a NetImposition.
 */
class PanoramaInfo : public Laxkit::anObject
{
  public:
	char *filename = nullptr;
	char *polyhedron_file = nullptr;
	char *sphere_file = nullptr;
	int spheremap_width;
	int spheremap_height;
	Laxkit::Basis extra_basis;

	unsigned char *spheremap_data;
	unsigned char *spheremap_data_rotated;

	PanoramaInfo(const char *file);
	~PanoramaInfo();

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

	void MakePaperInterface();

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

	Laxkit::ScreenColor color_potential;
	Laxkit::ScreenColor color_face;

	Document *doc = nullptr;
	NetImposition *original_netimp = nullptr;
	NetImposition *current_netimp = nullptr;
	Polyptych::Net *currentnet = nullptr;
	int current_paper_spread = -1;

	Polyptych::AbstractNet *abstract_net = nullptr;
	Polyptych::Polyhedron *poly = nullptr; // convenience cast for abstract_net
	Polyptych::BasicNet *net = nullptr; // convenience cast for abstract_net

	// Laxkit::RefPtrStack<Polyptych::Net> nets; // each net->info is the index of the original face that acts as the seed
 
	PaperGroup *papers = nullptr;
	PaperBox *default_paper = nullptr;

	int currentpotential; //index in currentnet->faces, or -1
	int currentface;      //index in Polyhedron of current face, or -1
	bool hover_face_is_leaf;

	int hover_group = -1;
	int hover_net = -1;
	int hover_index = -1;
	// int hover_overlay;
	int mouseover_group;  //which section mouseover_overlay is index in
	int mouseover_overlay; //which overlay mouse is currently over
	int mouseover_index;
	int grab_overlay;     //if lbdown on an overlay, all input corresponds to that one
	int active_action; //determined by current overlay, affects behavior of left mouse button

	// messages and overlays
	Laxkit::Utf8String currentmessage, lastmessage;

	double pad; //for overlay text
	

	// Laxkit::PtrStack<Overlay> overlays;
	// Laxkit::PtrStack<Overlay> paperoverlays;


	NetInterface(Laxkit::Displayer *dp);
	virtual ~NetInterface();
	virtual const char *whattype() { return "NetInterface"; }
	virtual const char *whatdatatype() { return nullptr; }
	virtual const char *IconId() { return "NetInterface"; }
	virtual const char *Name();
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	virtual int Refresh();
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
	
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int InterfaceOn();

	// net building
	virtual int Reseed(int original);
	virtual int recurseUnwrap(Polyptych::Net *netf, int fromneti, Polyptych::Net *nett, int toneti);
	// virtual void recurseCache(Polyptych::Net *net,int neti);
	// virtual int unwrapTo(int from,int to);
	virtual Polyptych::Net *establishNet(int original);
	virtual int removeNet(int netindex);
	virtual int removeNet(Polyptych::Net *net);
	virtual Polyptych::Net *findNet(int id);

	// mouse position
	virtual int findCurrentPotential(const Laxkit::flatpoint &p, int &neti);
	virtual int findCurrentFace(const Laxkit::flatpoint &p, int &neti);
	virtual int scanPaper(int x,int y, int &index);
	virtual int scanOverlays(int x,int y, int *action,int *index,int *group);
	// virtual Laxkit::flatpoint pointInNetPlane(int x,int y);

	// input/output
	// virtual int SavePolyptych(const char *saveto);
	// virtual void UseGenericImageData(double fg_r=-1, double fg_g=-1, double fg_b=-1,  double bg_r=-1, double bg_g=-1, double bg_b=-1);
	// virtual int InstallImage(const char *file);
	virtual int InstallPolyhedron(const char *file);
	virtual int InstallPolyhedron(Polyptych::Polyhedron *ph);
	// virtual int SetFiles(const char *hedron, const char *image, const char *project);
	virtual int AddNet(Polyptych::Net *net);
	// virtual int AddPaper(PaperStyle *paper);
	// virtual Polyptych::Polyhedron *defineCube();

	// from ImpositionEditor:
    virtual const char *ImpositionType() { return "NetImposition"; }
    virtual Imposition *GetImposition();
    virtual int SetTotalDimensions(double width, double height);
    virtual int GetDimensions(double &width, double &height); //Return default paper size
    virtual int SetPaper(PaperStyle *paper); // installs duplicate of paper
    virtual int UseThisDocument(Document *doc);
    virtual int UseThisImposition(Imposition *imp);

    virtual int ShowThisPaperSpread(int index);
    virtual void ShowSplash(int yes) {}
};

} //namespace Laidout



#endif

