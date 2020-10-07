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
// Copyright (c) 2004-2013 Tom Lechner
//
#ifndef VIEWWINDOW_H
#define VIEWWINDOW_H


#include <lax/interfaces/viewerwindow.h>
#include <lax/numslider.h>
#include <lax/sliderpopup.h>
#include <lax/lineedit.h>
#include <lax/colorbox.h>
#include <lax/button.h>

#include "../core/document.h"



namespace Laidout {

class Project;

//------------------------------- VObjContext ---------------------------
class VObjContext : public LaxInterfaces::ObjectContext
{
  public:
	FieldPlace context;
	VObjContext() { obj=NULL; }
	VObjContext(const VObjContext &oc);
	virtual ~VObjContext();
	virtual int isequal(const ObjectContext *oc);
	virtual int operator==(const ObjectContext &oc) { return isequal(&oc)==3; }
	virtual VObjContext &operator=(const VObjContext &oc);
	virtual int Set(ObjectContext *oc);
	virtual int set(LaxInterfaces::SomeData *nobj, int n, ...);
	virtual void clear();
	virtual void ClearTop();
	virtual void clearToPage();
	virtual LaxInterfaces::ObjectContext *duplicate();
	virtual int Up();

	virtual void push(int i,int where=-1);
	virtual int pop(int where=-1);
	virtual int level()  { if (obj) return context.n()-2; else return context.n()-1; }
	
	virtual int spread() { return context.e(0); }
	virtual int spreadpage() { if (context.e(0)==1 && context.e(1)==0) return context.e(2); else return -1; }
	virtual int layer()  { if (context.n()>3 && context.e(0)==1 && context.e(1)==0) return context.e(3); else return -1; }
	virtual int layeri() { if (context.n()>4 && context.e(0)==1) return context.e(4); else return -1; }
	virtual int limboi() { if (context.n()>1 && context.e(0)==0) return context.e(1); else return -1; }
	virtual int paperi() { if (context.n()>1 && context.e(0)==2) return context.e(1); else return -1; }
};

std::ostream &operator<<(std::ostream &os, VObjContext const &o);


//------------------------------- LaidoutViewport ---------------------------
class LaidoutViewport : public LaxInterfaces::ViewportWindow,
						virtual public ObjectContainer,
						virtual public Value,
						virtual public FunctionEvaluator
{
	char lfirsttime;
  protected:
	int fakepointer;   //***for lack of screencast recorder for multipointer
	flatpoint fakepos; //***for lack of screencast recorder for multipointer

	unsigned int drawflags;
	int viewportmode;
	int viewmode;
	int searchmode,searchcriteria;
	int showstate;
	int transformlevel;
	double ectm[6];
	Group *limbo;
	Laxkit::anXWindow *findwindow;

	char *pageviewlabel;

	virtual void setupthings(int tospread=-1,int topage=-1);
	virtual void UpdateMarkers();
	virtual void setCurobj(VObjContext *voc);
	virtual void findAny(int searcharea=0);
	virtual int nextObject(VObjContext *oc,int inc=0);
	virtual void transformToContext(double *m,FieldPlace &place,int invert, int depth);

	virtual int PerformAction(int action);

  public:
	 // these all have to refer to proper values in each other!
	Document *doc;
	Spread *spread;
	PaperGroup *papergroup;
	int spreadi;
	Page *curpage;
	 // these shadow viewport window variables of the same name but diff. type
	VObjContext curobj,firstobj,foundobj,foundtypeobj;

	int current_edit_area;
	Laxkit::LaxImage *edit_area_icon;

	LaidoutViewport(Document *newdoc);
	virtual ~LaidoutViewport();
	virtual const char *whattype() { return "LaidoutViewport"; }
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual void Refresh();
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int FocusOn(const Laxkit::FocusChangeData *e);
	virtual bool DndWillAcceptDrop(int x, int y, const char *action, Laxkit::IntRectangle &rect, char **types, int *type_ret, anXWindow **child_ret);
	virtual int selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which);

	virtual int UseTheseRulers(Laxkit::RulerWindow *x,Laxkit::RulerWindow *y);
	virtual double *transformToContext(double *m,LaxInterfaces::ObjectContext *oc,int invert,int full);
	virtual void DrawSomeData(Laxkit::Displayer *ddp,LaxInterfaces::SomeData *ndata,
			                        Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual void DrawSomeData(LaxInterfaces::SomeData *ndata,
			                        Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);

	virtual int UseThisDoc(Document *ndoc);
	virtual int UseThisPaperGroup(PaperGroup *group);
	
	virtual int ApplyThis(Laxkit::anObject *thing,unsigned long mask);
	
	virtual flatpoint realtoscreen(flatpoint r);
	virtual flatpoint screentoreal(int x,int y);
	virtual double Getmag(int c=0);
	virtual double GetVMag(int x,int y);

	virtual const char *Pageviewlabel();
	virtual void Center(int w=0);
	virtual int NewData(LaxInterfaces::SomeData *d,LaxInterfaces::ObjectContext **oc, bool clear_selection=true);
	virtual int SelectPage(int i);
	virtual int SelectSpread(int i);
	virtual int NextSpread();
	virtual int PreviousSpread();
	virtual int CurrentSpread() { return spreadi; }
	
	virtual int ChangeObject(LaxInterfaces::ObjectContext *oc,int switchtool);
	virtual int SelectObject(int i);
	virtual int FindObject(int x,int y, const char *dtype, 
					LaxInterfaces::SomeData *exclude, int start,
					LaxInterfaces::ObjectContext **oc,
					int searcharea=0);
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
	virtual int DeleteObject(LaxInterfaces::ObjectContext *oc);
	virtual LaxInterfaces::ObjectContext *ObjectMoved(LaxInterfaces::ObjectContext *oc, int modifyoc);
	virtual int MoveObject(LaxInterfaces::ObjectContext *from, LaxInterfaces::ObjectContext *to);
	virtual int CirculateInLayer(int dir, int i,int objOrSelection);
	virtual bool IsValidContext(LaxInterfaces::ObjectContext *oc);
	virtual LaxInterfaces::SomeData *GetObject(LaxInterfaces::ObjectContext *oc);
	virtual LaxInterfaces::ObjectContext *CurrentContext();
	virtual int UpdateSelection(LaxInterfaces::Selection *sel);
	virtual int wipeContext();
	virtual void clearCurobj();
	virtual int locateObject(LaxInterfaces::SomeData *d,FieldPlace &place);
	virtual int curobjPage();
	virtual int isDefaultPapergroup(int yes_if_in_project);

	 //from objectcontainer
	virtual int n();
	virtual Laxkit::anObject *object_e(int i);
	virtual const char *object_e_name(int i);
	virtual const double *object_transform(int i);
	virtual int object_e_info(int i, const char **name, const char **Name, int *isarray);

	 //from Value
	virtual int type();
    virtual Value *duplicate();
	virtual ObjectDef *makeObjectDef();
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Value *dereference(const char *extstring, int len);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
	                     Value **value_ret, Laxkit::ErrorLog *log);

	 //for scripting:
	virtual ValueHash *build_context();

	 //for i/o
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);


	friend class ViewWindow;
	friend class GroupInterface;
};

//------------------------------- ViewWindow ---------------------------
class ViewWindow : public LaxInterfaces::ViewerWindow
{
  protected:
	void setup();
	Laxkit::NumSlider *pagenumber;
	Laxkit::Button *pageclips;
	Laxkit::ColorBox *colorbox;
	Laxkit::SliderPopup *toolselector;
	Laxkit::anXWindow *rulercornerbutton;

	char *tempstring;
	int initial_tool;

	int ClearTools(int except_this);

  public:
	Project *project;
	Document *doc;

	ViewWindow(Document *newdoc);
	ViewWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						Document *newdoc);
	virtual ~ViewWindow();
	virtual const char *whattype() { return "ViewWindow"; }
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int FocusOn(const Laxkit::FocusChangeData *e);

	virtual LaxInterfaces::anInterface *GetObjectTool();
	virtual int PerformAction(int action);
	virtual int SelectTool(int id);
	virtual int SelectToolFor(const char *datatype,LaxInterfaces::ObjectContext *oc=NULL);
	virtual void updateContext(int messagetoo);
	virtual void updateProjectStatus();
	virtual void SetParentTitle(const char *str);
	virtual void setCurdoc(Document *newdoc);
	virtual char *CurrentDirectory();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} // namespace Laidout

#endif

