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
#ifndef SPREADVIEW_H
#define SPREADVIEW_H

#include <lax/anobject.h>
#include <lax/refcounted.h>
#include <lax/dump.h>
#include <lax/interfaces/somedata.h>

#include "impositions/imposition.h"

//------------------------------- arrangetype --------------------------------

 // values for arrangestate
 // arrangetype has to be within the min to max.
#define ArrangeNeedsArranging -1
#define ArrangeTempRow         1
#define ArrangeTempColumn      2
#define ArrangeTempGrid        3

#define  ArrangetypeMin        4
#define ArrangeAutoAlways      4
#define ArrangeAutoTillMod     5
#define Arrange1Row            6
#define Arrange1Column         7
#define ArrangeGrid            8
#define ArrangeCustom          9
#define  ArrangetypeMax        9


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
				   public Laxkit::RefCounted,
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
	virtual LittleSpread *SpreadOfPage(int page, int *thread,int skipmain=0);
	virtual int RemoveFromThread(int pageindex, int thread);
	virtual int MoveToThread(int pageindex,int thread, int threadplace);
	virtual void FindBBox();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};




#endif


