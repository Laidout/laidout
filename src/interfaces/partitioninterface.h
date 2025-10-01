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
#ifndef INTERFACES_PARTITION_H
#define INTERFACES_PARTITION_H

#include <lax/singletonkeeper.h>
#include <lax/interfaces/aninterface.h>

#include "../impositions/signatures.h"
#include "../core/papersizes.h"


namespace Laidout {


//------------------------------------- Region --------------------------------------

class PaperCut
{
  public:
  	Laxkit::Utf8String label;
  	Laxkit::ScreenColor color;
  	Laxkit::flatpoint from;
  	Laxkit::flatpoint to;

  	enum CutType {
  		CUT_Unknown = 0,
  		CUT_Single,
  		CUT_InsetTop,
  		CUT_InsetRight,
  		CUT_InsetBottom,
  		CUT_InsetLeft,
  		CUT_Tiles,
  		CUT_MAX
  	};
  	int cut_type = 0; // usually one of CutType
  	int group_id = 0; // for instance, all tile cuts belong to the same group

  	bool IsInset() {
  		return cut_type == CUT_InsetTop || cut_type == CUT_InsetRight || cut_type == CUT_InsetBottom || cut_type == CUT_InsetLeft;
  	}
};

class PaperGrid : public Laxkit::RefCounted
{
  public:
  	int tile_x = 1;
  	int tile_y = 1;
  	double gap_x = 0;
  	double gap_y = 0;
  	double FinalWidth (double media_w) { return (media_w - (tile_x-1)*gap_x)/ tile_x; }
  	double FinalHeight(double media_h) { return (media_h - (tile_y-1)*gap_y)/ tile_y; }
};

class Region : virtual public LaxInterfaces::SomeData, virtual public Value
{
  public:
  	Laxkit::Utf8String label;
  	Laxkit::ScreenColor color;
  	int index = -1;

  	bool is_waste = false;
  	ValueHash properties;

  	// if not a final region, this Region is cut up by these cuts, and produce child and waste regions
  	Laxkit::PtrStack<PaperCut> cuts;
  	WorkAndTurnTypes work_and_turn = SIGT_None; // see WorkAndTurnTypes in signatures.h

  	Laxkit::DoubleBBox area_in_parent;
  	Region *spawned_from = nullptr;
  	Laxkit::RefPtrStack<Region> child_regions;
  	Laxkit::RefPtrStack<Region> waste_regions;

  	// for final regions that have no more cuts, you can stack or duplicate from another region
  	Region *stack_on = nullptr;
  	Region *duplicate_from = nullptr;

  	// final regions can be be PaperPartition and run through an imposition.
  	// Optionally perform work and turn before passing on to imposition
  	// Imposition *imposition;

  	Region();
  	~Region();

	virtual const char *whattype() { return "Region"; }
	virtual const char* Id() { return Laxkit::anObject::Id(); }
	virtual const char* Id(const char *new_id) { return Laxkit::anObject::Id(new_id); }

	virtual void dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context);
	virtual Laxkit::Attribute* dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute*att, int flag, Laxkit::DumpContext *context);

  	// Value funcs
  	virtual Value *duplicateValue();
  	virtual anObject *duplicate() { return duplicateValue(); }
	virtual ObjectDef *makeObjectDef();

	Laxkit::flatpoint DetectNewCutBoundary(Laxkit::flatvector cut_from, Laxkit::flatvector dir, int ignore, int *on_cut_ret);
  	bool IsFinal() { return cuts.n == 0; }
  	double Area() { if (validbounds()) return boxwidth() * boxheight(); else return 0; }

  	int AddInsets(double top, double right, double bottom, double left);
  	int RemoveInsets();
  	bool HasInsets();

  	bool HasTiles();
};


//------------------------------------- PartitionInterfaceSettings --------------------------------------

class PartitionInterfaceSettings : public Laxkit::anObject
{
  public:
  	Laxkit::ScreenColor outline_color;
	Laxkit::LaxFont *font = nullptr;
	double default_linewidth = 1;
	double arrow_size = 10; // multiples of ScreenLine()

	PartitionInterfaceSettings();
	virtual ~PartitionInterfaceSettings();
};

//------------------------------------- PartitionInterface --------------------------------------

class PartitionInterface : virtual public LaxInterfaces::anInterface
{
  	static Laxkit::SingletonKeeper settingsObject; // static so that it is easily shared between all tool instances

  protected:
	int showdecs = 1;
	bool show_labels = true;
	bool show_indices = true;
	double rx, ry;

	bool show_hover_line = false;
	Region *hover_region = nullptr;
	int hover_cut = -1;
	Laxkit::flatpoint hover_from, hover_to;

	PaperStyle *paper_style = nullptr;
	Region *main_region = nullptr;
	Region *current_region = nullptr;

	PartitionInterfaceSettings *settings;

	virtual int scan(int x,int y, Region *start_here, int &edge_ret, Laxkit::flatpoint &p_ret, double &dist, Region *&region_ret);
	virtual int SnapBoxes();

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

  public:
	PartitionInterface();
	PartitionInterface(anInterface *nowner = nullptr, int nid = 0, Laxkit::Displayer *ndp = nullptr);
	virtual ~PartitionInterface();
	virtual LaxInterfaces::anInterface *duplicateInterface(LaxInterfaces::anInterface *dup);
	virtual Laxkit::ShortcutHandler *GetShortcuts();

	virtual const char *IconId() { return "Partition"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PartitionInterface"; }
	virtual const char *whatdatatype() { return nullptr; }
	// virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	
	// return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	
	virtual int Refresh();
	void DrawRegion(Region *data, bool arrow);
	// virtual int DrawDataDp(Laxkit::Displayer *tdp,LaxInterfaces::SomeData *tdata,
	// 				Laxkit::anObject *a1=nullptr,Laxkit::anObject *a2=nullptr,int info=1);

	
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0); 
	//virtual int DrawData(Laxkit::anObject *ndata,
	//		Laxkit::anObject *a1=nullptr,Laxkit::anObject *a2=nullptr,int info=0);
	//virtual int DrawDataDp(Laxkit::Displayer *tdp,LaxInterfaces::SomeData *tdata,
	//		Laxkit::anObject *a1=nullptr,Laxkit::anObject *a2=nullptr,int info=1);
	
	// virtual int UseThisDocument(Document *doc);
};


} //namespace Laidout


#endif

