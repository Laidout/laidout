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
// Copyright (c) 2004-2010 Tom Lechner
//
#ifndef VIEWWINDOW_H
#define VIEWWINDOW_H


#include <lax/interfaces/viewerwindow.h>
#include <lax/numinputslider.h>
#include <lax/sliderpopup.h>
#include <lax/lineedit.h>
#include <lax/colorbox.h>
#include <lax/button.h>

#include "document.h"



//------------------------------- VObjContext ---------------------------
class VObjContext : public LaxInterfaces::ObjectContext
{
 public:
	FieldPlace context;
	VObjContext() { obj=NULL; context.push(0); }
	virtual ~VObjContext();
	virtual int isequal(const ObjectContext *oc);
	virtual int operator==(const ObjectContext &oc) { return isequal(&oc); }
	virtual VObjContext &operator=(const VObjContext &oc);
	virtual int set(LaxInterfaces::SomeData *nobj, int n, ...);
	virtual void clear();
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
class LaidoutViewport : public LaxInterfaces::ViewportWindow, virtual public ObjectContainer
{
	char lfirsttime;
 protected:
	int fakepointer;   //***for lack of screen record for multipointer
	flatpoint fakepos; //***for lack of screen record for multipointer

	unsigned int drawflags;
	int viewportmode;
	int viewmode;
	int searchmode,searchcriteria;
	int showstate;
	int transformlevel;
	double ectm[6];
	Group *limbo;

	virtual void setupthings(int tospread=-1,int topage=-1);
	virtual void setCurobj(VObjContext *voc);
	virtual void findAny();
	virtual int nextObject(VObjContext *oc,int inc=0);
	virtual void transformToContext(double *m,FieldPlace &place,int invert=1);
 public:
	 //*** maybe these should be protected?
	char *pageviewlabel;
	
	 // these all have to refer to proper values in each other!
	Document *doc;
	Spread *spread;
	PaperGroup *papergroup;
	int spreadi;
	Page *curpage;
	 // these shadow viewport window variables of the same name but diff. type
	VObjContext curobj,firstobj,foundobj,foundtypeobj;
	
	LaidoutViewport(Document *newdoc);
	virtual ~LaidoutViewport();
	virtual const char *whattype() { return "LaidoutViewport"; }
	virtual void Refresh();
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int FocusOn(const Laxkit::FocusChangeData *e);

	virtual int UseThisDoc(Document *ndoc);
	virtual int UseThisPaperGroup(PaperGroup *group);
	
	virtual int ApplyThis(Laxkit::anObject *thing,unsigned long mask);
	
	virtual flatpoint realtoscreen(flatpoint r);
	virtual flatpoint screentoreal(int x,int y);
	virtual double Getmag(int c=0);
	virtual double GetVMag(int x,int y);

	virtual const char *Pageviewlabel();
	virtual void Center(int w=0);
	virtual int NewData(LaxInterfaces::SomeData *d);
	virtual int NewCurobj(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext **oc=NULL);
	virtual int SelectPage(int i);
	virtual int NextSpread();
	virtual int PreviousSpread();
	
	virtual int ChangeObject(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext *oc);
	virtual int SelectObject(int i);
	virtual int FindObject(int x,int y, const char *dtype, 
					LaxInterfaces::SomeData *exclude, int start,
					LaxInterfaces::ObjectContext **oc);
	virtual int FindObjects(Laxkit::DoubleBBox *box, char real, char ascurobj,
							LaxInterfaces::SomeData ***data_ret, LaxInterfaces::ObjectContext ***c_ret);
	virtual void ClearSearch();
	virtual int ChangeContext(int x,int y,LaxInterfaces::ObjectContext **oc);
	virtual int ChangeContext(LaxInterfaces::ObjectContext *oc);
	
	virtual const char *SetViewMode(int m,int page);
	virtual int ViewMode(int *page);
	virtual int PlopData(LaxInterfaces::SomeData *ndata,char nearmouse=0);
	virtual void postmessage(const char *mes);
	virtual int DeleteObject();
	virtual int ObjectMove(LaxInterfaces::SomeData *d);
	virtual int CirculateObject(int dir, int i,int objOrSelection);
	virtual int validContext(VObjContext *oc);
	virtual void clearCurobj();
	virtual int locateObject(LaxInterfaces::SomeData *d,FieldPlace &place);
	virtual int n() { if (spread) return 2; return 1; }
	virtual Laxkit::anObject *object_e(int i);
	virtual int curobjPage();

	friend class ViewWindow;
	friend class GroupInterface;
};

//------------------------------- ViewWindow ---------------------------
class ViewWindow : public LaxInterfaces::ViewerWindow
{
 protected:
	void setup();
	Laxkit::NumInputSlider *pagenumber;
	Laxkit::NumInputSlider *var1, *var2, *var3;
	Laxkit::Button *pageclips;
	Laxkit::ColorBox *colorbox;
	Laxkit::SliderPopup *toolselector;
	Laxkit::anXWindow *rulercornerbutton;
 public:
	Project *project;
	Document *doc;

	ViewWindow(Document *newdoc);
	ViewWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						Document *newdoc);
	virtual ~ViewWindow();
	virtual const char *whattype() { return "ViewWindow"; }
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int FocusOn(const Laxkit::FocusChangeData *e);

	virtual int SelectTool(int id);
	virtual void updateContext(int messagetoo);
	virtual void updateProjectStatus();
	virtual void SetParentTitle(const char *str);
	virtual void setCurdoc(Document *newdoc);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

#endif

