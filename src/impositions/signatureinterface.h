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
// Copyright (C) 2010-2011 by Tom Lechner
//
#ifndef INTERFACES_SIGNATUREINTERFACE_H
#define INTERFACES_SIGNATUREINTERFACE_H

#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/aninterface.h>

#include "../laidout.h"
#include "signatures.h"


class SignatureEditor;

//------------------------------------- ActionArea ---------------------------
class ActionArea : public Laxkit::DoubleBBox
{
  public:
	char *tip;
	char *text;
	flatpoint *outline, offset, hotspot;
	int npoints;
	int visible; //1 for yes and filled, 2 for selectable, but not drawn,3 for outline only
	int hidden; //skip checks for this one
	unsigned long color;

	int real; //use real coordinates, not screen coordinates
	int action; //id for the action this overlay corresponds to
	int category; //extra identifier
	int type; //basic type this overlay is: handle (movable), slider, button, display only, pan, menu trigger
	int PointIn(double x,double y);

	ActionArea(int what,int ntype,const char *txt,const char *ntip,int r,int v,unsigned long col,int cat);
	virtual ~ActionArea();
	virtual flatpoint *Points(flatpoint *pts, int n, int takethem);
	virtual void FindBBox();
	virtual flatpoint Position() { return offset+hotspot; }
	virtual void Position(double x,double y,int which=3);
};

//------------------------------------- SignatureInterface --------------------------------------

class SignatureInterface : public LaxInterfaces::anInterface
{
 protected:
	int showdecs;
	int showsplash;
	Laxkit::PtrStack<ActionArea> controls;

	unsigned long color_inset, color_margin, color_trim, color_binding;
	int insetmask, trimmask, marginmask;
	int firsttime;

	int lbdown_row, lbdown_col;

	 //to keep track of current partial fold:
	int foldr1, foldc1, foldr2, foldc2;
	int folddirection;
	int foldunder;
	int foldindex;
	double foldprogress;

	int finalr,finalc; //cell location of totally folded pages
	int hasfinal; //whether the pattern has been totally folded yet or not

	int activetilex,activetiley;
	int onoverlay,overoverlay; //nonzero if mouse clicked down on and is over an overlay
	double arrowscale;
	ActionArea *control(int what);
	int adjustControl(int handle, int dir);

	int foldlevel; //how hany folds are actively displayed
	FoldedPageInfo **foldinfo;
	void reallocateFoldinfo();
	void applyFold(char folddir, int index, int under);
	void applyFold(Fold *fold);

	int scan(int x,int y,int *row,int *col,double *ex,double *ey, int *tile_row, int *tile_col);
	int scanHandle(int x,int y);
	int checkFoldLevel(int update);
	void getFoldIndicatorPos(int which, double *x,double *y, double *w,double *h);
	int scanForFoldIndicator(int x, int y, int ignorex);
	void remapHandles(int which=0);
	void createHandles();
	int offsetHandle(int which, flatpoint d);
	void remapAffectedCells(int whichfold);
	void drawHandle(ActionArea *area, flatpoint offset);

	void dumpFoldinfo();//for debugging
 public:
	Signature *signature;
	PaperStyle *papersize;

	SignatureInterface(int nid=0,Laxkit::Displayer *ndp=NULL,Signature *sig=NULL, PaperStyle *p=NULL);
	SignatureInterface(anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL);
	virtual ~SignatureInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual const char *Name();
	virtual const char *IconId() { return "Folding"; }
	virtual const char *whattype() { return "SignatureInterface"; }
	virtual const char *whatdatatype() { return "Signature"; }
	virtual int draws(const char *atype);

	virtual int InterfaceOn();
	virtual int InterfaceOff(); 
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y, int deviceid);
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	virtual int UseThisImposition(SignatureImposition *sigimp);
	virtual int UseThisSignature(Signature *sig);

	friend class SignatureEditor;
};


//------------------------------------- SignatureEditor --------------------------------------

class SignatureEditor : public LaxInterfaces::ViewerWindow
{
 protected:
	SignatureInterface *tool;
	char *imposeout, *imposeformat;
 public:
	SignatureEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						Laxkit::anXWindow *nowner, const char *mes,
						SignatureImposition *sigimp, PaperStyle *p,const char *imposearg=NULL);
	virtual ~SignatureEditor();
	virtual int init();
	virtual const char *whattype() { return "SignatureEditor"; }
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual void send();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


#endif

