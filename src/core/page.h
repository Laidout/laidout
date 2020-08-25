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
// Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef PAGE_H
#define PAGE_H

#include <lax/anobject.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/pathinterface.h>

#include "../dataobjects/group.h"
#include "../calculator/values.h"



namespace Laidout {


//---------------------------- PageBleed ---------------------------------

class Page;

class PageBleed
{
 public:
	int index;
	Page *page;
	int hasbleeds;
	double matrix[6];
	PageBleed(int doc_page_index, const double *m, Page *docpage);
};

//---------------------------- PageStyle ---------------------------------

#define MARGINS_CLIP         (1<<0)
#define PAGE_CLIPS           (1<<1)
#define FACING_PAGES_BLEED   (1<<2)
#define PAGESTYLE_AUTONOMOUS (1<<3)
#define DONT_SHOW_PAGE       (1<<4)

class PageStyle : public Value
{
 public:
	unsigned int flags; // marginsclip,facingpagesbleed;
	int pagetype;
	double min_x,min_y,width,height; // these are to be considered the bounding
									//box for non-rectangular pages. usually the
									//same as outline bbox

	LaxInterfaces::PathsData *outline, *margin;

	PageStyle(); 
	virtual ~PageStyle();
	virtual ObjectDef *makeObjectDef();
	virtual const char *whattype() { return "PageStyle"; }
	virtual Value *duplicate();
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	virtual double minx() { return min_x; }
	virtual double miny() { return min_y; }
	virtual double w() { return width; }
	virtual double h() { return height; }
	virtual int set(const char *flag, int newstate);
	virtual bool Flag(unsigned int which);
	virtual void Flag(unsigned int which, bool state);
};

//---------------------------- RectPageStyle ---------------------------------
 // left-right-top-bottom margins "rectpagestyle"
#define RECTPAGE_LRTB       (1<<0)
 // inside-outside-top-bottom (for booklet sort of style) "facingrectstyle"
#define RECTPAGE_IOTB       (1<<1)
 // left-right-inside-outside (for calendar sort of style) "topfacingrectstyle"
#define RECTPAGE_LRIO       (1<<2)

#define RECTPAGE_LEFTPAGE   (1<<3)
#define RECTPAGE_TOPPAGE    (1<<3)
#define RECTPAGE_RIGHTPAGE  (1<<4)
#define RECTPAGE_BOTTOMPAGE (1<<4)

class RectPageStyle : public PageStyle
{
 public:
	unsigned int recttype; //LRTB, IOTB, LRIO
	double ml,mr,mt,mb; // margins 
	RectPageStyle(unsigned int ntype=RECTPAGE_LRTB,double l=0,double r=0,double t=0,double b=0);
	virtual const char *whattype() { return "RectPageStyle"; }
	virtual ObjectDef *makeObjectDef();
	virtual Value *duplicate();
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//---------------------------- Page ---------------------------------

enum PageMarkerType {
	MARKER_Unmarked=0,
	MARKER_Circle,
	MARKER_Square,
	MARKER_Octagon,
	MARKER_TriangleUp,
	MARKER_Diamond,
	MARKER_MAX
};

class Page : public ObjectContainer
{
 public:
	 //page attributes
	int labeltype;
	Laxkit::ScreenColor labelcolor;
	char *label;
	int pagenumber;
	PageStyle *pagestyle;
	ValueHash properties;

	 //page preview thumbnail
	LaxInterfaces::ImageData *thumbnail;
	clock_t thumbmodtime,modtime;

	 //page contents
	DrawableObject anchors;
	Group layers;
	Laxkit::PtrStack<PageBleed> pagebleeds;

	//char *external_page_file;
	//int page_loaded; //-1 for not applicable, 0 for no, 1 for yes

	Page(PageStyle *npagestyle=NULL,int num=-1); 
	virtual ~Page(); 
	virtual const char *whattype() { return "Page"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxInterfaces::ImageData *Thumbnail();
	virtual Laxkit::LaxImage *RenderPage(int width, int height, Laxkit::LaxImage *existing, bool transparent);
	virtual int InstallPageStyle(PageStyle *pstyle, bool shift_within_margins);

	virtual int PushLayer(const char *layername, int where=-1);

	virtual int n() { return layers.n(); }
	virtual Group *e(int i) { return dynamic_cast<Group *>(layers.e(i)); }
	virtual Laxkit::anObject *object_e(int i) { return layers.object_e(i); }
	virtual const double *object_transform(int i) { return NULL; }
	virtual const char *object_e_name(int i);

	virtual void Touch(clock_t at_time=0);
	virtual void UpdateAnchored(Group *g);
	virtual int HasObjects();
};

} // namespace Laidout

#endif


