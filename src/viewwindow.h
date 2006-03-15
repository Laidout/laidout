//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef VIEWWINDOW_H
#define VIEWWINDOW_H


#include <lax/interfaces/viewerwindow.h>
#include <lax/numinputslider.h>
#include <lax/lineedit.h>
#include <X11/extensions/Xdbe.h>
#include "document.h"



//------------------------------- VObjContext ---------------------------
class VObjContext : public Laxkit::ObjectContext
{
 public:
	FieldPlace context;
	VObjContext() { obj=NULL; context.push(0); }
	virtual ~VObjContext() {}
	virtual int isequal(const ObjectContext *oc);
	virtual int operator==(const ObjectContext &oc) { return isequal(&oc); }
	virtual VObjContext &operator=(const VObjContext &oc);
	virtual int set(Laxkit::SomeData *nobj, int n, ...);
	virtual void clear() { obj=NULL; context.flush(); }
	virtual void push(int i,int where=-1);
	virtual int pop(int where=-1);
	
	virtual int spread() { return context.e(0); }
	virtual int spreadpage() { if (context.n()>1 && context.e(0)!=0) return context.e(1); else return -1; }
	virtual int layer() { if (context.n()>2 && context.e(0)!=0) return context.e(2); else return -1; }
	virtual int layeri() { if (context.n()>3 && context.e(0)!=0) return context.e(3); else return -1; }
	virtual int limboi() { if (context.n()>1 && context.e(0)==0) return context.e(1); else return -1; }
	virtual int level() { if (obj) return context.n()-2; else return context.n()-1; }
};

//------------------------------- LaidoutViewport ---------------------------
class LaidoutViewport : public Laxkit::ViewportWindow, virtual public ObjectContainer
{
	char lfirsttime;
 protected:
	unsigned int drawflags;
	int viewmode,searchmode;
	int showstate;
	int transformlevel;
	double ectm[6];
	XdbeBackBuffer backbuffer;
	Group limbo;
	virtual void setupthings(int topage=-1);
	virtual void LaidoutViewport::setCurobj(VObjContext *voc);
	virtual void LaidoutViewport::findAny();
	virtual int nextObject(VObjContext *oc);
	virtual void transformToContext(double *m,FieldPlace &place,int invert=1);
 public:
	 //*** maybe these should be protected?
	char *pageviewlabel;
	
	 // these all have to refer to proper values in each other!
	Document *doc;
	Spread *spread;
	Page *curpage;
	 // these shadow viewport window variables of the same name but diff. type
	VObjContext curobj,firstobj,foundobj,foundtypeobj;
	
	LaidoutViewport(Document *newdoc);
	virtual ~LaidoutViewport();
	virtual void Refresh();
	virtual int init();
	virtual int CharInput(unsigned int ch,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	
	virtual int ApplyThis(Laxkit::anObject *thing,unsigned long mask);
	
	virtual flatpoint realtoscreen(flatpoint r);
	virtual flatpoint screentoreal(int x,int y);
	virtual double Getmag(int c=0);
	virtual double GetVMag(int x,int y);

	virtual const char *Pageviewlabel();
	virtual void Center(int w=0);
	virtual int NewData(Laxkit::SomeData *d,Laxkit::ObjectContext **oc=NULL);
	virtual int SelectPage(int i);
	virtual int NextSpread();
	virtual int PreviousSpread();
	
	virtual int ChangeObject(Laxkit::SomeData *d,Laxkit::ObjectContext *oc);
	virtual int LaidoutViewport::SelectObject(int i);
	virtual int FindObject(int x,int y, const char *dtype, 
					Laxkit::SomeData *exclude, int start,Laxkit::ObjectContext **oc);
	virtual void ClearSearch();
	virtual int ChangeContext(int x,int y,Laxkit::ObjectContext **oc);
	
	virtual const char *SetViewMode(int m);
	virtual int PlopData(Laxkit::SomeData *ndata);
	virtual void postmessage(const char *mes);
	virtual int DeleteObject();
	virtual int ObjectMove(Laxkit::SomeData *d);
	virtual int CirculateObject(int dir, int i,int objOrSelection);
	virtual int validContext(VObjContext *oc);
	virtual void clearCurobj();
	virtual int locateObject(Laxkit::SomeData *d,FieldPlace &place);
	virtual int n() { if (spread) return 2; return 1; }
	virtual Laxkit::anObject *object_e(int i);
	virtual int curobjPage();
};

//------------------------------- ViewWindow ---------------------------
class ViewWindow : public Laxkit::ViewerWindow
{
 protected:
	void setup();
	Laxkit::NumInputSlider *pagenumber;
	Laxkit::NumInputSlider *var1, *var2, *var3;
	Laxkit::LineEdit *loaddir;
 public:
	Project *project;
	Document *doc;

	ViewWindow(Document *newdoc);
	ViewWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						Document *newdoc);
	virtual int CharInput(unsigned int ch,unsigned int state);
	virtual int DataEvent(Laxkit::SendData *data,const char *mes);
	virtual int init();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual void updatePagenumber();
};

#endif

