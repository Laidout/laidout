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
// Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//
#ifndef PAPERSIZES_H
#define PAPERSIZES_H


#include <lax/refptrstack.h>
#include <lax/utf8string.h>
#include <lax/menuinfo.h>
#include <lax/interfaces/somedata.h>

#include "../calculator/values.h"
#include "../dataobjects/objectcontainer.h"
#include "../dataobjects/drawableobject.h"



namespace Laidout {


//--------------------------------- BoxTypesEnum --------------------------------
/*! Usually, 
 * MediaBox >= PrintableBox >= BleedBox >= TrimBox >= ArtBox.
 * Laidout does not have a CropBox per se. The PrintableBox is meant to 
 * correspond to the printable area of particular printers.
 * 
 * Note that currently, only MediaBox is used.
 */
enum BoxTypes {
	NoBox        = 0,
	MediaBox     = (1<<0), // paper size
	PrintableBox = (1<<1), // area that a printer can physically print to
	BleedBox     = (1<<2), // area outside trimbox to print, usually 3-5mm
	TrimBox      = (1<<3), // area after bleed trimming, final page size
	MarginBox    = (1<<4), // editing guides. Laidout specific, not part of PDF spec.
	ArtBox       = (1<<5), // guide for area of interest on page
	// CropBox      = (1<<6), // area that should be shown in an editor window
	AllBoxes     = ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5))
};


//------------------------------------- PaperStyle --------------------------------------

class PaperStyle : public Value, public FunctionEvaluator
{
 public:
	char *name;
	double width,height;
	double dpi;
	char *defaultunits;
	bool is_landscape;
	bool favorite;
	Laxkit::ScreenColor color_hint;
	ValueHash meta; // like stock, thickness, glossiness, other printer specific settings

	PaperStyle(const char *nname = nullptr);
	PaperStyle(const char *nname, double ww, double hh, bool nis_landscape, double ndpi,const char *defunits);
	virtual ~PaperStyle();
	virtual double w() { if (landscape()) return height; else return width; }
	virtual double h() { if (landscape()) return width; else return height; }
	virtual double w(double v) { if (landscape()) height = v; else width = v; return w(); }
	virtual double h(double v) { if (landscape()) width = v; else height = v; return h(); }
	virtual bool landscape() { return is_landscape; }
	virtual bool landscape(bool l)
		{ is_landscape = l; return is_landscape; }
	virtual int SetFromString(const char *nname);

	//from Value:
	virtual Value *duplicateValue();
	virtual ObjectDef *makeObjectDef();
	virtual int type();
	virtual int getValueStr(char *buffer,int len);
	virtual Value *dereference(const char *extstring, int len);
	virtual int Evaluate(const char *function,int len, ValueHash *context, ValueHash *pp, CalcSettings *settings,
                         Value **value_ret, Laxkit::ErrorLog *log);
	virtual int IsMatch(double w, double h, double epsilon = 0);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
};


//----------------------------- Paper Helper Funcs --------------------------------------
PaperStyle *GetNamedPaper(double width, double height, int *is_landscape_ret, int startfrom, int *index_ret, double epsilon);
PaperStyle *GetPaperFromName(const char *name);
Laxkit::PtrStack<PaperStyle> *GetBuiltinPaperSizes(Laxkit::PtrStack<PaperStyle> *papers);
Laxkit::MenuInfo *BuildGroupedPaperMenu(Laxkit::MenuInfo *menu, int item_info, bool include_custom, bool include_whatever);


//------------------------------------- PaperBox --------------------------------------
class PaperBox :  public Laxkit::anObject
{
 public:
	int which; // see BoxTypes
	PaperStyle *paperstyle;
	Laxkit::DoubleBBox media, printable;
	Laxkit::Insets bleed, trim, art, margin;
	PaperBox(PaperStyle *paper, bool absorb_count);
	virtual ~PaperBox();
	virtual bool landscape();
	virtual bool landscape(bool l);
	virtual int Set(PaperStyle *paper);
	virtual anObject *duplicate();

	bool Has(BoxTypes type) { return (which & type) != 0; }
};


//------------------------------------- PaperBoxData --------------------------------------
class PaperBoxData : public LaxInterfaces::SomeData
{
 public:
	Laxkit::Utf8String label;
	ValueHash properties;
	Laxkit::ScreenColor color, outlinecolor;
	PaperBox *box;
	int index, index_back;
	// unsigned int which; //unused?

	PaperBoxData(PaperBox *paper);
	virtual ~PaperBoxData();
	virtual LaxInterfaces::SomeData *duplicateData(LaxInterfaces::SomeData *dup);
	virtual const char *whattype() { return "PaperBoxData"; }
	virtual void FindBBox();

	virtual double w() { if (box && box->paperstyle) return box->paperstyle->w(); else return 0; }
	virtual double h() { if (box && box->paperstyle) return box->paperstyle->h(); else return 0; }
};


//------------------------------------- PaperGroup --------------------------------------
class PaperGroup : virtual public Laxkit::Resourceable, virtual public ObjectContainer, virtual public Laxkit::DumpUtility
{
 public:
	char *name;
	char *Name;
	char locked;
	Laxkit::RefPtrStack<PaperBoxData> papers;

	DrawableObject objs;

	PaperGroup();
	PaperGroup(PaperBoxData *boxdata);
	PaperGroup(PaperStyle *paperstyle);
	virtual ~PaperGroup();
	virtual const char *whattype() { return "PaperGroup"; }
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual anObject *duplicate();

	virtual int AddPaper(double w,double h,double offsetx,double offsety);
	virtual int AddPaper(const char *nme,double w,double h,const double *m, const char *label);
	virtual double OutlineColor(double r,double g,double b);
	virtual PaperStyle *GetBasePaper(int index);
	virtual int FindPaperBBox(Laxkit::DoubleBBox *box_ret, double *extram = nullptr);

	virtual int n(); //this is the number of papergroup.objs, NOT the number of papers
	virtual Laxkit::anObject *object_e(int i);
	virtual const char *object_e_name(int i);
	virtual const double *object_transform(int i);
};


} // namespace Laidout

#endif

