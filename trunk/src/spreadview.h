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
// Copyright (C) 2010-2013 by Tom Lechner
//
#ifndef SPREADVIEW_H
#define SPREADVIEW_H

#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/interfaces/somedata.h>

#include "impositions/imposition.h"



namespace Laidout {


class Spread;

//------------------------------- arrangetype --------------------------------

enum ArrangeTypes {
	 // values for arrangestate
	ArrangeNeedsArranging = -1,
	ArrangeTempRow = 1,
	ArrangeTempColumn = 2,
	ArrangeTempGrid = 3,

	 // arrangetype has to be within the min to max.
	ArrangetypeMin = 4,
	ArrangeAutoAlways = 4,
	ArrangeAutoTillMod = 5,
	Arrange1Row = 6,
	ArrangeRows = 7,
	Arrange1Column = 8,
	ArrangeGrid = 9,
	ArrangeCustom = 10,
	ArrangetypeMax = 10,
};


const char *arrangetypestring(int a);


//----------------------- LittleSpread --------------------------------------

class LittleSpread : public LaxInterfaces::SomeData, virtual public Laxkit::Tagged
{
 public:
	int what;
	int deleteattachements;
	Spread *spread; // holds the outline, etc..
	LaxInterfaces::PathsData *connection;
	int lowestpage,highestpage;
	clock_t lasttouch;
	bool hidden;
	LittleSpread *prev,*next;

	LittleSpread();
	LittleSpread(Spread *sprd, LittleSpread *prv);
	virtual ~LittleSpread();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void mapConnection();
	virtual void FindBBox();
};

//----------------------- SpreadView --------------------------------------
class SpreadView : public Laxkit::anObject,
				   public LaxFiles::DumpUtility,
				   public Laxkit::DoubleBBox
{
 protected:
	virtual int validateTemppagemap();

 public:
	char *viewname;
	unsigned long doc_id;
	double matrix[6];
	int default_marker;
	int centerlabels;
	int arrangetype, arrangestate;
	char drawthumbnails;
	clock_t lastmodtime;
	Laxkit::PtrStack<LittleSpread> spreads;
	Laxkit::PtrStack<LittleSpread> threads;
	Laxkit::NumStack<int> temppagemap;
	//Laxkit::PtrStack<TextBlock> notes;

	SpreadView(const char *newname=NULL);
	virtual ~SpreadView();
	virtual const char *whattype() { return "SpreadView"; }

	virtual const char *Name() { return viewname?viewname:""; }
	virtual int reversemap(int i);
	virtual int map(int i);

	virtual int Modified();
	virtual int Update(Document *doc);//sync up with a Document, so as to not point to missing pages
	virtual void ArrangeSpreads(Laxkit::Displayer *dp,int how=-1);
	virtual int SwapPages(int previouspos, int newpos);
	virtual int ApplyChanges();
	virtual void Reset();
	virtual LittleSpread *findSpread(int x,int y, int *pagestacki, int *thread);
	virtual LittleSpread *SpreadOfPage(int page, int *thread, int *spreadi, int *psi, int skipmain);
	virtual int RemoveFromThread(int pageindex, int thread);
	virtual int MoveToThread(int pageindex,int thread, int threadplace);
	virtual void FindBBox();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};



} // namespace Laidout

#endif


