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
// Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef PAGE_H
#define PAGE_H

#include <lax/anobject.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <X11/Xlib.h>
#include <Imlib2.h>

#include "dataobjects/group.h"
#include "styles.h"


//---------------------------- PageBleed ---------------------------------

class PageBleed
{
 public:
	int index;
	int hasbleeds;
	double matrix[6];
	PageBleed(int i, const double *m);
};

//---------------------------- PageStyle ---------------------------------

#define MARGINS_CLIP         (1<<0)
#define PAGE_CLIPS           (1<<1)
#define FACING_PAGES_BLEED   (1<<2)
#define PAGESTYLE_AUTONOMOUS (1<<3)
#define DONT_SHOW_PAGE       (1<<4)

class PageStyle : public Style
{
 public:
	unsigned int flags; // marginsclip,facingpagesbleed;
	int pagetype;
	double width,height; // these are to be considered the bounding box for non-rectangular pages

	LaxInterfaces::PathsData *outline, *margin;

	PageStyle(); 
	virtual ~PageStyle();
	virtual StyleDef *makeStyleDef();
	virtual const char *whattype() { return "PageStyle"; }
	virtual Style *duplicate(Style *s=NULL);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	virtual double w() { return width; }
	virtual double h() { return height; }
	virtual int set(const char *flag, int newstate);
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
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


//---------------------------- Page ---------------------------------

class Page : public ObjectContainer
{
 public:
	 //page attributes
	int labeltype;
	char *label;
	int pagenumber;
	PageStyle *pagestyle;
	int psislocal;

	 //page preview thumbnail
	LaxInterfaces::ImageData *thumbnail;
	clock_t thumbmodtime,modtime;

	 //page contents
	Group layers;
	Laxkit::PtrStack<PageBleed> pagebleeds;

	Page(PageStyle *npagestyle=NULL,int pslocal=1,int num=-1); 
	virtual ~Page(); 
	virtual const char *whattype() { return "Page"; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxInterfaces::ImageData *Thumbnail();
	virtual int InstallPageStyle(PageStyle *pstyle,int islocal=1);

	virtual int n() { return layers.n(); }
	virtual Group *e(int i) { return dynamic_cast<Group *>(layers.e(i)); }
	virtual Laxkit::anObject *object_e(int i) { return layers.object_e(i); }
	virtual const double *object_transform(int i) { return NULL; }
	virtual const char *object_e_name(int i) { return NULL; }
};


#endif

