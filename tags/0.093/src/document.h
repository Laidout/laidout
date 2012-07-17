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
// Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef DOCUMENT_H
#define DOCUMENT_H

class Document;
#include "styles.h"
#include "impositions/imposition.h"
#include "laidoutdefs.h"
#include "spreadview.h"


class SpreadView;

enum  LaidoutSaveFormat {
	Save_Normal,
	Save_PPT,
	Save_PS,
	Save_EPS,
	Save_PDF_1_3,
	Save_PDF_1_4,
	Save_HTML,
	Save_SVG,
	Save_Scribus,
};


//---------------------------- PageRange ---------------------------------------

enum PageLabelType {
	Numbers_Default,
	Numbers_None, //sometimes you want some pages with no page numbers
	Numbers_Arabic,
	Numbers_Roman,
	Numbers_Roman_cap,
	Numbers_abc,
	Numbers_ABC,
	Numbers_MAX
};

const char *pageLabelTypeName(PageLabelType t);

class PageRange : public LaxFiles::DumpUtility
{
 public:
	char *name;
	int start,end, first;
	char *labelbase;
	int labeltype;
	int decreasing;
	Laxkit::ScreenColor color; //default display color for page range editor

	PageRange();
	PageRange(const char *nm, const char *base, int type, int s, int e, int f, int dec);
	virtual ~PageRange();
	char *GetLabel(int i);
	char *GetLabel(int i,int altfirst,int alttype);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//------------------------- Document ------------------------------------

class Document : public ObjectContainer, public Style
{
 public:
	char *saveas;
	char *name;
	
	Imposition *imposition;

	LaxFiles::Attribute metadata;
	LaxFiles::Attribute iohints;

	int curpage;
	Laxkit::PtrStack<Page> pages;
	Laxkit::PtrStack<PageRange> pageranges;
	Laxkit::RefPtrStack<SpreadView> spreadviews;

	clock_t modtime;

	Document(const char *filename);
	Document(Imposition *imp=NULL,const char *filename=NULL);
	virtual ~Document();
	virtual const char *Saveas();
	virtual int Saveas(const char *nsaveas);
	virtual const char *Name(int withsaveas);
	virtual int Name(const char *nname);
	virtual void clear();

	 //style functions
	virtual Style *duplicate(Style *s=NULL);
	virtual StyleDef* makeStyleDef();

	 //page and imposition management
	virtual Page *Curpage();
	virtual int NewPages(int starting,int n);
	virtual int RemovePages(int start,int n);
	virtual int SyncPages(int start,int n);
	virtual int ReImpose(Imposition *newimp,int scale_page_contents_to_fit);
	virtual Spread *GetLayout(int type, int index);

	virtual int ApplyPageRange(const char *name, int type, const char *base, int start, int end, int first, int dec);
	virtual void UpdateLabels(int whichrange);
	
	 //i/o
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual int Load(const char *file,ErrorLog &log);
	virtual int Save(int includelimbos,int includewindows,ErrorLog &log);
	
	
	 //object content
	virtual int n() { return pages.n; }
	virtual Laxkit::anObject *object_e(int i) 
		{ if (i>=0 && i<pages.n) return (anObject *)(pages.e[i]); return NULL; }
	virtual const char *object_e_name(int i) { return NULL; }
	virtual const double *object_transform(int i) { return NULL; }
	virtual int GroupItems(FieldPlace whatlevel, int *items);
	virtual int UnGroup(FieldPlace which);

//	int SaveAs(char *newfile,int format=1); // format: binary, xml, pdata
//	int New();
//	int Print();
};


#endif

