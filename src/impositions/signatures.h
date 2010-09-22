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
// Copyright (C) 2010 by Tom Lechner
//
#ifndef SIGNATURES_H
#define SIGNATURES_H

//#include <lax/anobject.h>
//#include <lax/dump.h>
//#include <lax/refcounted.h>
#include "imposition.h"


//------------------------------------- Fold --------------------------------------

const char *FoldDirectionName(char dir, int under, int translate=1);

class Fold
{
 public:
	char direction; //l,r,t, or b
	int under; //1 for folding under in direction
	int whichfold; //index from the left or top side of completely unfolded paper of which fold to act on
	Fold(char dir,int u, int which) { direction=dir; under=u; whichfold=which; }
};

//--------------------------------------- FoldedPageInfo ---------------------------------------
class FoldedPageInfo
{
 public:
	int currentrow, currentcol; //where this original is currently
	int y_flipped, x_flipped; //how this one is flipped around in its current location
	int finalxflip, finalyflip;
	int finalindexfront, finalindexback;
	Laxkit::NumStack<int> pages; //what pages are actually there, r,c are pushed

	FoldedPageInfo();
};



//------------------------------------ Signature -----------------------------------------
StyleDef *makeSignatureImpositionStyleDef();

class Signature : public Laxkit::anObject, public Laxkit::RefCounted, public LaxFiles::DumpUtility
{
 public:
	PaperStyle *paperbox; //optional
	double totalwidth, totalheight;

	int sheetspersignature;
	int autoaddsheets; //whether you increase the num of sheets per sig, or num sigs when adding pages, 
	double insetleft, insetright, insettop, insetbottom;

	double tilegapx, tilegapy;
	int tilex, tiley; //how many partitions per sheet of paper

	double creep;
	double rotation; //in practice, not really sure how this works, 
					 //it is mentioned here and there that minor rotation is sometimes needed
					 //for some printers
	char work_and_turn; //0 for no, 1 work and turn == rotate on axis || to major dim, 2 work and tumble=rot axis || minor dim
	int pilesections; //if tiling, whether to repeat content, or continue content (ignored currently)

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


	Signature();
	virtual ~Signature();
	const Signature &operator=(const Signature &sig);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	virtual int IsVertical();
	virtual unsigned int Validity();
	virtual double PatternHeight();
	virtual double PatternWidth();
	virtual double PageHeight(int part=0);
	virtual double PageWidth(int part=0);
	virtual Laxkit::DoubleBBox *PageBounds(int part, Laxkit::DoubleBBox *bbox);

	virtual int PagesPerPattern();
	virtual int PagesPerSignature();

	virtual int SetPaper(PaperStyle *p);
	virtual int locatePaperFromPage(int pagenumber, int *row, int *col);

	virtual Signature *duplicate();
};


//------------------------------------ SignatureImposition -----------------------------------------
class SignatureImposition : public Imposition
{
 protected:
	int showwholecover; //whether you see the cover+backcover next to each other, or by themselves
	PaperStyle *papersize;
	RectPageStyle *pagestyle,*pagestyleodd;
	int numsignatures;
	//PaperPartition *partition; //partition to insert folding pattern
	
	virtual void setPageStyles();
	virtual void fixPageBleeds(int index,Page *page);
 public:
	Signature *signature;      //folding pattern

	SignatureImposition(Signature *newsig=NULL);
	virtual ~SignatureImposition();
	virtual const char *whattype() { return "SignatureImposition"; }
	static ImpositionResource **getDefaultResources();

	virtual int UseThisSignature(Signature *newsig);
	virtual StyleDef *makeStyleDef();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	virtual const char *BriefDescription();
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPaperSize(PaperStyle *npaper);
	virtual int SetPaperGroup(PaperGroup *ngroup);
	virtual PageStyle *GetPageStyle(int pagenum,int local);
	
	virtual Page **CreatePages();
	virtual int SyncPageStyles(Document *doc,int start,int n);
	
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
	virtual int NumSpreads(int layout,int setthismany); 
	virtual int NumPapers();
	virtual int NumPapers(int npapers);
	virtual int NumPages();
	virtual int NumPages(int npages);

	virtual int PaperFromPage(int pagenumber);
	virtual int SpreadFromPage(int layout, int pagenumber);
	virtual int GetPagesNeeded(int npapers);
	virtual int GetPapersNeeded(int npages);
	virtual int GetSpreadsNeeded(int npages);
	virtual int *PrintingPapers(int frompage,int topage);

	virtual int NumPageTypes();
	virtual const char *PageTypeName(int pagetype);
	virtual int PageType(int page);
	virtual int SpreadType(int spread);
};



#endif

