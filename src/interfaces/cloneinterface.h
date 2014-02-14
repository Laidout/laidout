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
// Copyright (C) 2013-2014 by Tom Lechner
//

#include "../calculator/values.h"
#include "../dataobjects/group.h"
#include "../language.h"
#include "../viewwindow.h"
#include "selection.h"

#include <lax/lists.h>
#include <lax/laximages.h>
#include <lax/dump.h>
#include <lax/transformmath.h>
#include <lax/interfaces/pathinterface.h>


namespace Laidout {



//------------------------------------- TileCloneInfo ------------------------------------

class TileCloneInfo
{
  public:
	int baseid; //object that is the base of the clone set
	int cloneid;//clone instance id (not object_id)
	int x,y, i; //standard indexing: containing p1 x,y, and index to op

	Laxkit::NumStack<int> dim; //one per dimension
	int iteration;
};


//------------------------------------- TilingDest ------------------------------------

class TilingDest
{
  public:
	int cloneid; //of containing Tiling
	int op_id; //of newly placed clone, can be any of uber parent Tiling::basecells
	int parent_op_id; //of containing TilingOp

	bool is_progressive;
	bool traceable;

	 //conditions for traversal
	unsigned int conditions; //1 use iterations, 2 use max size, 3 use min size, 4 use scripted
	int max_iterations; // <0 for endless, use other constraints to control
	int recurse_objects;
	double max_size, min_size;
	double traversal_chance;
	char *scripted_condition;

	 //transform from current space to this destination cell
	Value *scripted_transform;
	Laxkit::Affine transform;

	TilingDest();
	virtual ~TilingDest();
};


//------------------------------------- TilingOp ------------------------------------

class TilingOp
{
  public:
	int id;

	int basecell_is_editable;
	bool shearable;
	bool flexible_aspect;
	LaxInterfaces::PathsData *celloutline; //a path around original cell. This could be considered a hint for selecting what is to be repeated.

	Laxkit::PtrStack<TilingDest> transforms;

	TilingOp();
	virtual ~TilingOp();

	virtual int NumTransforms() { return transforms.n; }
	virtual Laxkit::Affine Transform(int which);
	virtual int isRecursive(int which);
	//virtual int RecurseInfo(int which, int *numtimes, double *minsize, double *maxsize);

	virtual TilingDest *AddTransform(Laxkit::Affine &transform);
	virtual TilingDest *AddTransform(TilingDest *dest);
};


//------------------------------------- Tiling ------------------------------------

class Tiling : public Laxkit::anObject, public LaxFiles::DumpUtility //, public MetaInfo
{
  protected:

  public:
	char *name;
	char *category;
	Laxkit::LaxImage *icon;

	int id;
	std::string required_interface;

	Laxkit::Affine repeat_basis; //of overall p1 (before final_transform applied)
	int repeatable; // &1 for x, &2 for y

	Laxkit::Affine final_transform; //a final transform to apply to whole grid
	LaxInterfaces::PathsData *boundary;

	Laxkit::PtrStack<TilingOp> basecells; //typically basecells must share same coordinate system, 
								 //and must be defined such as they appear laid into the default unit

	 //more flexible than built in overal P1 repeating.
	 //1st dimension is simply basecells stack. Additional dimensions are applied on top
	 //of each object produced by the 1st dimension.
	Laxkit::RefPtrStack<Tiling> dimensions;

	Tiling(const char *nname=NULL, const char *ncategory=NULL);
	virtual ~Tiling();

	virtual void InstallDefaultIcon();
	virtual void DefaultHex(double side_length);

	virtual int isXRepeatable();
	virtual int isYRepeatable();
	virtual flatpoint repeatOrigin();
	virtual flatpoint repeatOrigin(flatpoint norigin);
	virtual flatpoint repeatXDir();
	virtual flatpoint repeatXDir(flatpoint nx);
	virtual flatpoint repeatYDir();
	virtual flatpoint repeatYDir(flatpoint ny);
	virtual Laxkit::Affine finalTransform(); //transform applied after tiling to entire pattern, to squish around

	virtual TilingOp *AddBase(LaxInterfaces::PathsData *outline, int absorb_count, int lock_base,
								bool shearable=false, bool flexible_base=false);

	virtual Group *Render(Group *parent_space,
					   Selection *base_objects,
					   Laxkit::Affine *base_offsetm,
					   int p1_minx, int p1_maxx, int p1_miny, int p1_maxy,
					   LaxInterfaces::ViewportWindow *viewport,
					   LaxInterfaces::PathsData *boundary,
					   Laxkit::Affine *final_orient);
//	virtual void RenderRecursive(TilingDest *dest, int iterations, Laxkit::Affine current_space,
//					   Group *parent_space,
//					   LaxInterfaces::ObjectContext *base_object_to_update, //!< If non-null, update relevant clones connected to base object
//					   bool trace_cells,
//					   LaxInterfaces::ViewportWindow *viewport
//					   );
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att, int, Laxkit::anObject*);
};


//------------------------------------- CloneInterface --------------------------------------

class CloneInterface : public LaxInterfaces::anInterface
{
  protected:

	Tiling *tiling;


	int firsttime;
	int lastover;
	int lastoveri;
	int cur_tiling; //for built ins

	bool active;

	bool trace_cells;
	bool previewactive;
	VObjContext *previewoc;
	Group *preview;
	Group *lines;

	LaxInterfaces::PathsData *boundary;

	int current_base;
	Group base_cells;
	LaxInterfaces::LineStyle preview_cell;
	LaxInterfaces::LineStyle preview_cell2;

	Selection sources;

	double uiscale;
	Laxkit::DoubleBBox box;

	unsigned int bg_color;
	unsigned int hbg_color;
	unsigned int fg_color;
	unsigned int activate_color;
	unsigned int deactivate_color;

	virtual int scan(int x,int y, int *i);

//***	virtual void createControls();
//***	virtual void remapControls(int tempdir=-1);
//***	virtual const char *controlTooltip(int action);
//***	virtual const char *flowtypeMessage(int set);
//***	virtual int Apply(int updateorig);
//***	virtual int Reset();
	virtual int ToggleActivated();
	virtual int TogglePreview();
	virtual int Render();
	virtual void DrawSelected();

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

  public:
	unsigned long cloner_style;//options for interface

	CloneInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~CloneInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup=NULL);

	virtual const char *IconId() { return "Tiler"; }
	virtual const char *Name();
	virtual const char *whattype() { return "CloneInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype) { return 0; }

	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
//***	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
//***	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	virtual int SetTiling(Tiling *newtiling);

//***	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
//***	virtual int validateInfo();

//***	virtual int FreeSelection();
//***	virtual int AddToSelection(Laxkit::PtrStack<LaxInterfaces::ObjectContext> &objs);
};





//------------------------------------ Tiling Creating Functions --------------------------------

Tiling *CreateWallpaper(const char *group, LaxInterfaces::SomeData *centered_on);

Tiling *CreateRadial(double start_angle, double end_angle,   double start_radius, double end_radius,  
					 int num_divisions,  int mirrored);

Tiling *CreateFrieze(const char *group, LaxInterfaces::SomeData *centered_on);

//Tiling *CreateUniformColoring(const char *coloring, ***);


} //namespace Laidout

