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
// Copyright (C) 2010-2013 by Tom Lechner
//
#ifndef SIGNATURES_H
#define SIGNATURES_H

#include <lax/dump.h>
#include <lax/interfaces/linestyle.h>
#include "imposition.h"



namespace Laidout {


#define AUTOMARK_Margins           1
#define AUTOMARK_InnerDot          2
#define AUTOMARK_InnerDottedLines  4

//------------------------------------- Fold --------------------------------------

const char *FoldDirectionName(char dir, int under, int translate=1);

class Fold : public Value
{
 public:
	char direction; //l,r,t, or b
	int under; //1 for folding under in direction
	int whichfold; //index from the left or top side of completely unfolded paper of which fold to act on
	int face; //only for Nets, whichfold is edge of this face

	Fold(char dir,int u, int which, int whichface = -1) { direction = dir; under = u; whichfold = which; face = whichface; }
	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicate();
};

//--------------------------------------- FoldedPageInfo ---------------------------------------
class FoldedPageInfo
{
 public:
	int currentrow, currentcol; //where this original is currently
	int y_flipped, x_flipped; //how this one is flipped around in its current location
	int finalxflip, finalyflip;
	int finalindexfront, finalindexback;

	double rotation; //minute adjustment when putting page to paper
	Laxkit::flatpoint shift; //minute adjustment when putting page to paper

	Laxkit::NumStack<int> pages; //what pages are actually there, r,c are pushed

	FoldedPageInfo();
};



//------------------------------------ Signature -----------------------------------------
ObjectDef *makeSignatureImpositionObjectDef();

class Signature : public Value
{
 public:
	char *name;
	char *description;

	double patternwidth, patternheight;

	int sheetspersignature; //this is a hint only. SignatureInstance has actual values

	int numhfolds, numvfolds;
	Laxkit::PtrStack<Fold> folds;

	double trimleft, trimright, trimtop, trimbottom; // number<0 means don't trim that edge (for accordion styles)
	double marginleft, marginright, margintop, marginbottom;

	char up; //which direction is up 'l|r|t|b', ie 'l' means points toward the left
	char binding;    //direction to place binding 'l|r|t|b'
	char positivex;  //direction of the positive x axis: 'l|r|t|b'
	char positivey;  //direction of the positive y axis: 'l|r|t|b'

	 //for easy storing of final arrangement:
	FoldedPageInfo **foldinfo;
	virtual void reallocateFoldinfo();
	virtual void resetFoldinfo(FoldedPageInfo **finfo);
	virtual int  applyFold(FoldedPageInfo **finfo, int foldlevel);
	virtual void applyFold(FoldedPageInfo **finfo, char folddir, int index, int under);
	virtual int checkFoldLevel(FoldedPageInfo **finfo, int *finalrow,int *finalcol);
	virtual int HasFinal();

	Signature();
	virtual ~Signature();
	const Signature &operator=(const Signature &sig);
	virtual ObjectDef *makeObjectDef();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

	virtual int IsVertical();
	virtual unsigned int Validity();
	virtual double PatternHeight();
	virtual double PatternWidth();
	virtual double PageHeight(int part=0);
	virtual double PageWidth(int part=0);
	virtual Laxkit::DoubleBBox *PageBounds(int part, Laxkit::DoubleBBox *bbox);

	virtual int PagesPerPattern();

	virtual int SetPatternSize(double w,double h);
	virtual int locatePaperFromPage(int pagenumber, int *row, int *col, int num_sheets);

	virtual Value *duplicate();
};


//------------------------------------ PaperPartition -----------------------------------------
enum WorkAndTurnTypes {
	SIGT_None,
	SIGT_Work_and_Turn,
	SIGT_Work_and_Turn_BF,
	SIGT_Work_and_Tumble,
	SIGT_Work_and_Tumble_BF
};

class PaperPartition : public Value
{
  public:
	PaperStyle *paper;
	double totalwidth, totalheight;

	double insetleft, insetright, insettop, insetbottom;

	int tilex, tiley; //how many partitions per sheet of paper
	double tilegapx, tilegapy;

	WorkAndTurnTypes work_and_turn;
	
	PaperPartition();
	virtual ~PaperPartition();
	virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();

	virtual int SetPaper(PaperStyle *p);
	virtual double PatternHeight();
	virtual double PatternWidth();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};

//------------------------------------ SignatureInstance -----------------------------------------
class SignatureInstance : public Value
{
  public:
	RectPageStyle *pagestyle,*pagestyleodd;
	PaperPartition *partition;
	Signature *pattern;

	SignatureInstance *partitioned_from; // non-null if this instance spawned from a piece

	int base_sheets_per_signature; // number of sheets from only this instance, without partition stacking
	int sheetspersignature; //Current number of sheets including partition stacking. May change if autoaddsheets!=0
	int autoaddsheets;

	int automarks;
	bool spine_marks; // diagonal markings on binding side to keep track of order
	LaxInterfaces::LineStyle *linestyle; //for optional automatic printer marks

	enum TileStacking {
		Repeat,
	    StackThenFold,
	    FoldThenInsert,
	    FoldThenPlaceAdjacent,
	    Custom // use full custom partition
	};
	TileStacking tile_stacking = Repeat;

	double creep; //amount of creep for this signature stack, size difference between innermost and outer page

	SignatureInstance *next_insert,*prev_insert; //insert this into this one at middle when folded up 
	SignatureInstance *next_stack, *prev_stack; //another signature laid on side of this one

	SignatureInstance(Signature *sig=NULL, PaperPartition *paper=NULL);
	virtual ~SignatureInstance();
	virtual Value *duplicate();
	virtual SignatureInstance *duplicateSingle();
	virtual ObjectDef *makeObjectDef();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

	virtual int PagesPerSignature(int whichstack, int ignore_inserts);
	virtual int PaperSpreadsPerSignature(int whichstack, int ignore_inserts);
	virtual int IsVertical();

	virtual LaxInterfaces::SomeData *GetPageOutline();
	virtual LaxInterfaces::SomeData *GetPageMarginOutline(int pagenum);
	virtual void setPageStyles(int force_new);
	virtual int SetPaper(PaperStyle *p, int all_with_same_size);
	virtual int SetPaperFromFinalSize(double w,double h, int all);
	virtual int NumStacks();
	virtual int NumStacks(int which);
	virtual int NumInserts();
	virtual SignatureInstance *GetSignature(int stack,int insert);
	virtual double PatternHeight();
	virtual double PatternWidth();
	virtual int UseThisSignature(Signature *sig, int link);
	
	virtual int AddInsert(SignatureInstance *insert);
	virtual int AddStack(SignatureInstance *stack);

	virtual SignatureInstance *locateInsert(int pagenumber, int *insertnum, int *deep);
	virtual int locatePaperFromPage(int pagenumber,
									int *stack,
									int *insert,
									int *insertpage,
									int *row,
									int *col,
								    SignatureInstance **ssig);
	virtual SignatureInstance *InstanceFromPaper(int whichpaper,
									int *stack,
									int *insert,
									int *sigpaper,
									int *pageoffset,
									int *inneroffset,
									int *groups);
	virtual SignatureInstance *InstanceFromPage(int pagenumber,
								    int *stack,      //!< return index of stack, 0 for this
								    int *insert,     //!< return index of insert, 0 for this
								    int *insertpage, //!< return page number on the insert if insert were isolated
								    int *row,        //!< return row and column of page in pattern, seen from front side
								    int *col,
								    int *paper);
};


//------------------------------------ SignatureImposition -----------------------------------------
class SignatureImposition : public Imposition
{
  protected:
  	PaperGroup *papergroup;
	SignatureInstance *signatures;
	
	virtual void setPageStyles(int force_new);
	virtual void fixPageBleeds(int index,Page *page, bool update_pagestyle);

  public:
	SignatureImposition(SignatureInstance *newsig=NULL);
	virtual ~SignatureImposition();
	virtual const char *whattype() { return "SignatureImposition"; }

	 //Imposition functions:
	virtual ObjectDef *makeObjectDef();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual ImpositionInterface *Interface();

	virtual const char *BriefDescription();
	virtual void GetDefaultPaperDimensions(double *x, double *y);
	virtual void GetDefaultPageDimensions(double *x, double *y);
	virtual Value *duplicate();
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual int SetPaperGroup(PaperGroup *ngroup);
	virtual PaperGroup *GetPaperGroup(int layout, int index);
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	virtual PaperStyle *GetDefaultPaper();
	
	virtual int SyncPageStyles(Document *doc,int start,int n, bool shift_within_margins);
	
	virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1);
	virtual LaxInterfaces::SomeData *GetPageOutline(int pagenum,int local);
	virtual LaxInterfaces::SomeData *GetPageMarginOutline(int pagenum,int local);
	
	virtual Spread *Layout(int layout,int which); 
	virtual int NumLayoutTypes();
	virtual const char *LayoutName(int layout); 
	virtual Spread *SingleLayout(int whichpage); 
	virtual Spread *PageLayout(int whichspread); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Spread *GetLittleSpread(int whichspread);

	virtual int NumSpreads(int layout); 
	virtual int NumPapers();
	virtual int NumPapers(int npapers);
	virtual int NumPages();
	virtual int NumPages(int npages);

	virtual int PaperFromPage(int pagenumber);
	virtual int SpreadFromPage(int layout, int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual int GetNumInPaperGroupForSpread(int layout, int spread);

	virtual int NumPageTypes();
	virtual const char *PageTypeName(int pagetype);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);


	 //signature specific stuff:
	int showwholecover; //whether you see the cover+backcover next to each other in page spreads, or by themselves

	virtual int UseThisSignature(Signature *newsig);
	virtual int SetPaperFromFinalSize(double w,double h);
	static ImpositionResource **getDefaultResources();
	virtual int SetDefaultPaperSize(PaperStyle *npaper);

	virtual int NumStacks(int which);
	virtual int TotalNumStacks();
	virtual int Creep(int which,double d);
	virtual int IsVertical();
	virtual SignatureInstance *GetSignature(int stack,int insert);
	virtual SignatureInstance *AddStack(int stack, int insert, SignatureInstance *sn);
	virtual int RemoveStack(int stack, int insert);
	virtual SignatureInstance *InstanceFromPaper(int whichpaper,
									int *stack,
									int *insert,
									int *sigpaper,
									int *pageoffset,
									int *inneroffset,
									int *groups);
};


} // namespace Laidout

#endif

