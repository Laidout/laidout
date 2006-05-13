//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef PAGE_H
#define PAGE_H

#include <lax/anobject.h>
#include <lax/interfaces/imageinterface.h>
#include <X11/Xlib.h>
#include <Imlib2.h>

#include "group.h"
#include "styles.h"


//---------------------------- PageStyle ---------------------------------

#define MARGINS_CLIP       (1<<0)
#define PAGE_CLIPS         (1<<1)
#define FACING_PAGES_BLEED (1<<2)

class PageStyle : public Style
{
 public:
	unsigned int flags; // marginsclip,facingpagesbleed;
	double width,height; // these are to be considered the bounding box for non-rectangular pages
	PageStyle() { flags=0; }
	virtual StyleDef *makeStyleDef();
	virtual const char *whattype() { return "PageStyle"; }
	virtual Style *duplicate(Style *s=NULL);
	virtual double w() { return width; }
	virtual double h() { return height; }
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
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
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


//---------------------------- Page ---------------------------------

class Page : public ObjectContainer
{
 public:
	int labeltype;
	int pagenumber;
	LaxInterfaces::ImageData *thumbnail;
	clock_t thumbmodtime,modtime;
	Laxkit::PtrStack<Group> layers;
	PageStyle *pagestyle;
	int psislocal;
	Page(PageStyle *npagestyle=NULL,int pslocal=1,int num=-1); 
	virtual ~Page(); 
	virtual const char *whattype() { return "Page"; }
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual LaxInterfaces::ImageData *Thumbnail();
	virtual int InstallPageStyle(PageStyle *pstyle,int islocal=1);
	virtual int n() { return layers.n; }
	virtual Laxkit::anObject *object_e(int i) 
		{ if (i>=0 && i<layers.n) return (anObject *)(layers.e[i]); return NULL; }
};


#endif


