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
#ifndef DOCUMENT_H
#define DOCUMENT_H

class Document;
#include "styles.h"
#include "impositions/imposition.h"
#include "laidoutdefs.h"



enum PageLabelType {
	Numbers_Default,
	Numbers_Arabic,
	Numbers_Roman,
	Numbers_Roman_cap,
	Numbers_abc,
	Numbers_ABC,
	Numbers_max
};

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


//------------------------- DocumentStyle ------------------------------------

class DocumentStyle : public Style
{
 public:
	Imposition *imposition;
	DocumentStyle(Imposition *imp);
	virtual ~DocumentStyle();
	virtual Style *duplicate(Style *s=NULL);
	virtual StyleDef* makeStyleDef();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//---------------------------- PageRange ---------------------------------------

class PageRange : public LaxFiles::DumpUtility
{
 public:
	char *name;
	int impositiongroup;
	int start,end,offset;
	char *labelbase;
	int labeltype,decreasing;
	PageRange(const char *newbase="#",int ltype=Numbers_Default);
	virtual ~PageRange();
	char *GetLabel(int i);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//------------------------- Document ------------------------------------

class Document : public ObjectContainer, public LaxFiles::DumpUtility, public Laxkit::RefCounted
{
 public:
	DocumentStyle *docstyle;
	char *saveas;
	char *name;
	
	LaxFiles::Attribute metadata;
	LaxFiles::Attribute iohints;

	Laxkit::PtrStack<Page> pages;
	Laxkit::PtrStack<PageRange> pageranges;
	int curpage;
	clock_t modtime;

	Document(const char *filename);
	Document(DocumentStyle *stuff=NULL,const char *filename=NULL);
	virtual ~Document();
	virtual const char *Saveas();
	virtual int Saveas(const char *nsaveas);
	virtual const char *Name(int withsaveas);
	virtual int Name(const char *nname);
	virtual void clear();

	virtual Page *Curpage();
	virtual int NewPages(int starting,int n);
	virtual int RemovePages(int start,int n);
	virtual int SyncPages(int start,int n);
	virtual int ReImpose(Imposition *newimp);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual int Load(const char *file,char **error_ret);
	virtual int Save(int includelimbos,int includewindows,char **error_ret);
	
	virtual Spread *GetLayout(int type, int index);
	
	virtual int n() { return pages.n; }
	virtual Laxkit::anObject *object_e(int i) 
		{ if (i>=0 && i<pages.n) return (anObject *)(pages.e[i]); return NULL; }
	virtual int GroupItems(FieldPlace whatlevel, int *items);
	virtual int UnGroup(FieldPlace which);

//	int SaveAs(char *newfile,int format=1); // format: binary, xml, pdata
//	int New();
//	int Print();
};


#endif

