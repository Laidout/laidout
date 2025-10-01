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
// Copyright (C) 2013-2015,2017 by Tom Lechner
//

#include "../calculator/values.h"
#include "../dataobjects/drawableobject.h"
#include "../language.h"
#include "../ui/viewwindow.h"

#include <lax/lists.h>
#include <lax/laximages.h>
#include <lax/dump.h>
#include <lax/transformmath.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/selection.h>


namespace Laidout {



//------------------------------------- TileCloneInfo ------------------------------------

// *** not used currently, but may one day if Tiling::dimensions gets implemented:
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
	bool traceable; //whether this dest should be outlined on Render(). sometimes it is just a base for further objs

	 //conditions for traversal
	unsigned int conditions; //1 use iterations, 2 use max size, 3 use min size, 4 use scripted
	int max_iterations; // <0 for endless, use other constraints to control
	int recurse_objects; // what is supposed to recurse
	double max_size, min_size;
	double traversal_chance;
	char *scripted_condition;

	 //transform from current space to this destination cell
	Value *scripted_transform;
	Laxkit::Affine transform;

	TilingDest();
	virtual ~TilingDest();
	virtual TilingDest *duplicate();
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
	virtual TilingOp *duplicate();

	virtual int NumTransforms() { return transforms.n; }
	virtual Laxkit::Affine Transform(int which);
	virtual int isRecursive(int which);
	//virtual int RecurseInfo(int which, int *numtimes, double *minsize, double *maxsize);

	virtual TilingDest *AddTransform(Laxkit::Affine &transform);
	virtual TilingDest *AddTransform(TilingDest *dest);
};


//------------------------------------- Tiling ------------------------------------

class Tiling : public Value
{
  protected:
	void InsertClone(Group *parent_space, LaxInterfaces::SomeData *object, 
			Laxkit::Affine *sourcem, Laxkit::Affine *basecellmi, Laxkit::Affine &clonet, Laxkit::Affine *final_orient,
			const char *idname);

  public:
	char *name;
	char *category;
	Laxkit::LaxImage *icon;

	std::string required_interface;

	Laxkit::Affine repeat_basis; //of overall p1 (before final_transform applied)
	int repeatable; // &1 for x, &2 for y

	int radial_divisions; 
	ValueHash properties; //these are hints to various contstructors for how to remap a tiling

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
	virtual const char *whattype() { return "Tiling"; }
	virtual Value *duplicateValue();

	virtual void InstallDefaultIcon();
	virtual void DefaultHex(double side_length);

	virtual int isXRepeatable();
	virtual int isYRepeatable();
	virtual Laxkit::flatpoint repeatOrigin();
	virtual Laxkit::flatpoint repeatOrigin(Laxkit::flatpoint norigin);
	virtual Laxkit::flatpoint repeatXDir();
	virtual Laxkit::flatpoint repeatXDir(Laxkit::flatpoint nx);
	virtual Laxkit::flatpoint repeatYDir();
	virtual Laxkit::flatpoint repeatYDir(Laxkit::flatpoint ny);
	virtual Laxkit::Affine finalTransform(); //transform applied after tiling to entire pattern, to squish around

	virtual int HasRecursion();

	virtual TilingOp *AddBase(LaxInterfaces::PathsData *outline, int absorb_count, int lock_base,
								bool shearable=false, bool flexible_base=false);

	virtual Group *Render(Group *parent_space,
					   Group *source_objects,
					   Laxkit::Affine *base_offsetm,
					   Group *base_lines, //!< Optional base cells. If null, then create copies of tiling's default.
					   int p1_minx, int p1_maxx, int p1_miny, int p1_maxy,
					   LaxInterfaces::PathsData *boundary,
					   Laxkit::Affine *final_orient);
//	virtual void RenderRecursive(TilingDest *dest, int iterations, Laxkit::Affine current_space,
//					   Group *parent_space,
//					   LaxInterfaces::ObjectContext *base_object_to_update, //!< If non-null, update relevant clones connected to base object
//					   bool trace_cells,
//					   LaxInterfaces::ViewportWindow *viewport
//					   );
	
	virtual ObjectDef *makeObjectDef();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att, int, Laxkit::DumpContext *context);
};


//------------------------------------- CloneInterface --------------------------------------

class CloneInterface : public LaxInterfaces::anInterface
{
  protected:

	Tiling *tiling;
	int num_input_fields;
	Laxkit::PtrStack<char> extra_input_fields;

	int firsttime;
	int lastover;
	int lastoveri;
	int cur_tiling; //for built ins
	int mode;

	bool active;
	bool preview_orient;
	bool snap_to_base;
	bool color_to_stroke;
	bool show_p1;

	bool trace_cells;
	bool preview_lines;
	VObjContext *previewoc;
	Group *preview;
	Group *lines;

	LaxInterfaces::PathsData *boundary;

	Group *base_cells;
	Group *source_proxies; //points to a child in base_cells
	int current_base;
	LaxInterfaces::LineStyle preview_cell;
	LaxInterfaces::LineStyle preview_cell2;


	double uiscale;
	Laxkit::DoubleBBox box;

	 //selected mode stuff
	int current_selected;
	int num_rows, num_cols;
	int selected_offset;
	double icon_width;
	double base_lastm[6];
	bool preempt_clear;


	unsigned int bg_color;
	unsigned int hbg_color;
	unsigned int fg_color;
	unsigned int activate_color;
	unsigned int deactivate_color;

	LaxInterfaces::RectInterface rectinterface;

	virtual int scan(int x,int y, int *i, int *dest);
	virtual int scanBasecells(Laxkit::flatpoint fp, int *i, int *dest);
	virtual int scanSelected(int x,int y);

	virtual int ToggleOrientations();
	virtual int ToggleActivated();
	virtual int TogglePreview();
	virtual int Render();
	virtual void DrawSelected();
	virtual Laxkit::ScreenColor *BaseCellColor(int which);
	virtual TilingDest *GetDest(const char *str);
	virtual DrawableObject *GetProxy(int base, int which);
	virtual int NumProxies();
	virtual LaxInterfaces::PathsData *GetBasePath(int which=-1);
	virtual int UpdateBasecells();
	virtual int UpdateFromSelection();

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

	//virtual int EditBoundary();
	virtual int EditThis(LaxInterfaces::SomeData *object, const char *message);

  public:
	unsigned long cloner_style;//options for interface

	CloneInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~CloneInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicateInterface(anInterface *dup);

	virtual const char *IconId() { return "Tiler"; }
	virtual const char *Name();
	virtual const char *whattype() { return "CloneInterface"; }
	virtual const char *whatdatatype() { return NULL; }
	virtual int draws(const char *atype) { return 0; }

	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual void RefreshSelectMode();
	virtual int Mode(int newmode);

	virtual int SetTiling(Tiling *newtiling);
	virtual int SetCurrentBase(int which);

	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
};





//------------------------------------ Tiling Creating Functions --------------------------------

Tiling *CreateWallpaper(const char *group);

Tiling *CreateRadial(double start_angle, double end_angle,   double start_radius, double end_radius,  
					 int num_divisions,  int mirrored, Tiling *oldtiling);

Tiling *CreateSpiral(double start_angle, double end_angle,   double start_radius, double end_radius,  
					 int num_divisions,   Tiling *oldtiling   );

Tiling *CreateFrieze(const char *group);

Tiling *CreateUniformColoring(const char *coloring);


} //namespace Laidout

