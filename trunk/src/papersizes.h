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
// Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef PAPERSIZES_H
#define PAPERSIZES_H


#include <lax/refptrstack.h>

#include "styles.h"
#include <lax/interfaces/somedata.h>


//--------------------------------- BoxTypesEnum --------------------------------
enum BoxTypes {
	NoBox        =0,
	MediaBox     =(1<<0),
	ArtBox       =(1<<1),
	TrimBox      =(1<<2),
	PrintableBox =(1<<3),
	BleedBox     =(1<<4),
};


//------------------------------------- PaperStyle --------------------------------------
class PaperStyle : public Style
{
 public:
	int color_red,color_green,color_blue;
	char *name;
	double width,height;
	int dpi;
	unsigned int flags; //1=landscape !(&1)=portrait
	PaperStyle(const char *nname,double ww,double hh,unsigned int nflags,int ndpi);
	virtual double w() { if (flags&1) return height; else return width; }
	virtual double h() { if (flags&1) return width; else return height; }
	virtual Style *duplicate(Style *s=NULL);

	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
};


//----------------------------- GetBuiltInPaperSizes() --------------------------------------
Laxkit::PtrStack<PaperStyle> *GetBuiltinPaperSizes(Laxkit::PtrStack<PaperStyle> *papers);


//------------------------------------- PaperBox --------------------------------------
class PaperBox : public Laxkit::RefCounted
{
 public:
	int which;
	PaperStyle *paperstyle;
	Laxkit::DoubleBBox media, printable, bleed, trim, crop, art;
	PaperBox(PaperStyle *paper);
	virtual ~PaperBox();
};


//------------------------------------- PaperBoxData --------------------------------------
class PaperBoxData : public LaxInterfaces::SomeData
{
 public:
	PaperBox *box;
	int index;
	unsigned int which;
	PaperBoxData(PaperBox *paper);
	virtual ~PaperBoxData();
};


//------------------------------------- PaperGroup --------------------------------------
class PaperGroup : public Laxkit::anObject, public Laxkit::RefCounted, public LaxFiles::DumpUtility
{
 public:
	char *name;
	char *Name;
	Laxkit::RefPtrStack<PaperBoxData> papers;
	Laxkit::anObject *owner;

	PaperGroup();
	virtual ~PaperGroup();
	virtual void dump_out(FILE *f,int indent,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
};



#endif

