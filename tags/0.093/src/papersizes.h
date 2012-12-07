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
// Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//
#ifndef PAPERSIZES_H
#define PAPERSIZES_H


#include <lax/refptrstack.h>

#include "styles.h"
#include "dataobjects/objectcontainer.h"
#include "dataobjects/group.h"
#include <lax/interfaces/somedata.h>


//--------------------------------- BoxTypesEnum --------------------------------
enum BoxTypes {
	NoBox        =0,
	MediaBox     =(1<<0),
	ArtBox       =(1<<1),
	TrimBox      =(1<<2),
	PrintableBox =(1<<3),
	BleedBox     =(1<<4),
	AllBoxes     =31
};


//------------------------------------- PaperStyle --------------------------------------
#define PAPERSTYLE_Landscape  1

class PaperStyle : public Style
{
 public:
	int color_red,color_green,color_blue;
	char *name;
	double width,height;
	double dpi;
	char *defaultunits;
	unsigned int flags; //1=landscape !(&1)=portrait
	PaperStyle();
	PaperStyle(const char *nname,double ww,double hh,unsigned int nflags,double ndpi,const char *defunits);
	virtual ~PaperStyle();
	virtual double w() { if (flags&1) return height; else return width; }
	virtual double h() { if (flags&1) return width; else return height; }
	virtual int landscape() { return flags&PAPERSTYLE_Landscape; }
	virtual int landscape(int l)
		{ if (l) flags|=PAPERSTYLE_Landscape; else flags&=~PAPERSTYLE_Landscape; return flags&PAPERSTYLE_Landscape; }
	virtual Style *duplicate(Style *s=NULL);
	virtual StyleDef *makeStyleDef();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};
StyleDef *makePaperStyleDef();


//----------------------------- GetBuiltInPaperSizes() --------------------------------------
Laxkit::PtrStack<PaperStyle> *GetBuiltinPaperSizes(Laxkit::PtrStack<PaperStyle> *papers);


//------------------------------------- PaperBox --------------------------------------
class PaperBox :  public Laxkit::anObject, public Laxkit::RefCounted
{
 public:
	int which;
	PaperStyle *paperstyle;
	Laxkit::DoubleBBox media, printable, bleed, trim, crop, art;
	PaperBox(PaperStyle *paper);
	virtual ~PaperBox();
	virtual int Set(PaperStyle *paper);
};


//------------------------------------- PaperBoxData --------------------------------------
class PaperBoxData : public LaxInterfaces::SomeData
{
 public:
	int red,green,blue;
	PaperBox *box;
	int index;
	unsigned int which;
	PaperBoxData(PaperBox *paper);
	virtual ~PaperBoxData();
	virtual const char *whattype() { return "PaperBoxData"; }
};


//------------------------------------- PaperGroup --------------------------------------
class PaperGroup : public ObjectContainer, public Laxkit::RefCounted, public LaxFiles::DumpUtility
{
 public:
	char *name;
	char *Name;
	char locked;
	Laxkit::RefPtrStack<PaperBoxData> papers;
	Laxkit::anObject *owner;

	Group objs;

	PaperGroup();
	PaperGroup(PaperBoxData *boxdata);
	PaperGroup(PaperStyle *paperstyle);
	virtual ~PaperGroup();
	virtual const char *whattype() { return "PaperGroup"; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	virtual int AddPaper(double w,double h,double offsetx,double offsety);
	virtual int AddPaper(const char *nme,double w,double h,const double *m);
	virtual int OutlineColor(int r,int g,int b);

	virtual int n();
	virtual Laxkit::anObject *object_e(int i);
	virtual const char *object_e_name(int i);
	virtual const double *object_transform(int i);
};



#endif
