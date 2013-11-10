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
#ifndef INTERFACES_SIGNATUREINTERFACE_H
#define INTERFACES_SIGNATUREINTERFACE_H

#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/aninterface.h>

#include "../laidout.h"
#include "../interfaces/actionarea.h"

#include "signatures.h"



namespace Laidout {


class ImpositionEditor;

//------------------------------------- SignatureInterface --------------------------------------

class SignatureInterface : public ImpositionInterface
{
 protected:
	int showdecs;
	int showsplash;
	int showthumbs;
	Laxkit::PtrStack<ActionArea> controls;

	unsigned long color_inset, color_margin, color_trim, color_binding, color_h, color_text;
	int insetmask, trimmask, marginmask;
	int firsttime;

	int lbdown_row, lbdown_col;

	 //to keep track of current partial fold:
	int foldr1, foldc1, foldr2, foldc2;
	int folddirection;
	int foldunder;
	int foldindex;
	double foldprogress;

	int currentPaperSpread;
	int finalr,finalc; //cell location of totally folded pages
	int hasfinal; //whether the pattern has been totally folded yet or not

	int activetilex,activetiley;
	int onoverlay; //nonzero if mouse clicked down on and is over an overlay
	int onoverlay_i, onoverlay_ii;
	int overoverlay; //handle of item last hovered over
	double arrowscale;
	ActionArea *control(int what);

	int OnBack() { return currentPaperSpread%2==1; }

	int foldlevel; //how hany folds are actively displayed
	FoldedPageInfo **foldinfo;
	void reallocateFoldinfo();
	void applyFold(char folddir, int index, int under);
	void applyFold(Fold *fold);

	int scan(int x,int y,int *row,int *col,double *ex,double *ey, int *tile_row, int *tile_col);
	int scanHandle(int x,int y, int *i1=NULL, int *i2=NULL);
	int scanStacks(int x,int y, int *stacki, int *inserti);
	int checkFoldLevel(int update);
	void getFoldIndicatorPos(int which, double *x,double *y, double *w,double *h);
	int scanForFoldIndicator(int x, int y, int ignorex);
	void remapHandles(int which=0);
	void createHandles();
	int offsetHandle(ActionArea *which, flatpoint d);
	int offsetHandle(int which, flatpoint d);
	int adjustControl(int handle, int dir);
	void remapAffectedCells(int whichfold);
	void drawHandle(ActionArea *area, flatpoint offset);
	virtual void drawStacks();
	void drawAdd(double x,double y,double r, int fill);

	void dumpFoldinfo();//for debugging

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
 public:
	Document *document;
	SignatureImposition *sigimp;
	SignatureInstance *siginstance;
	Signature *signature;

	SignatureInterface(LaxInterfaces::anInterface *nowner=NULL,int nid=0,Laxkit::Displayer *ndp=NULL,SignatureImposition *sig=NULL, PaperStyle *p=NULL);
	virtual ~SignatureInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *Name();
	virtual const char *IconId() { return "Folding"; }
	virtual const char *whattype() { return "SignatureInterface"; }
	virtual const char *whatdatatype() { return "SignatureImposition"; }
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

	virtual int UseThisSignature(Signature *sig);

	 //from ImpositionInterface:
	virtual const char *ImpositionType() { return whattype(); }
    virtual Imposition *GetImposition() { return sigimp; }
    virtual int SetTotalDimensions(double width, double height);
    virtual int GetDimensions(double &width, double &height);
    virtual int SetPaper(PaperStyle *paper);
    virtual int UseThisDocument(Document *doc);
    virtual int UseThisImposition(Imposition *imp);
    virtual int ShowThisPaperSpread(int index);
    virtual void ShowSplash(int yes) { showsplash=yes; needtodraw=1; }


	friend class ImpositionEditor;
};




} // namespace Laidout

#endif

