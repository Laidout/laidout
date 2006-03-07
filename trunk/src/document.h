//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef DOCUMENT_H
#define DOCUMENT_H

class Document;
#include "styles.h"
#include "disposition.h"

//------------------------- DocumentStyle ------------------------------------

class DocumentStyle : public Style
{
 public:
	Disposition *disposition;
	DocumentStyle(Disposition *disp);
	virtual ~DocumentStyle();
	virtual Style *duplicate(Style *s=NULL);
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


//------------------------- Document ------------------------------------

enum  LaidoutSaveFormat {
	Save_Normal,
	Save_PPT,
	Save_PS,
	Save_EPS,
	Save_PDF,
	Save_HTML,
	Save_SVG,
	Save_Scribus,
};

class Document : public ObjectContainer, public LaxFiles::DumpUtility
{
 public:
	DocumentStyle *docstyle;
	char *saveas;
	
	Laxkit::PtrStack<Page> pages;
	int curpage;
	int numn;
	char **notesorscripts;
	clock_t modtime;

	Document(const char *filename);
	Document(DocumentStyle *stuff=NULL,const char *filename=NULL);
	virtual ~Document();
	virtual const char *Name();
	virtual int Name(const char *nname);
	virtual Page *Curpage();

	virtual void clear();
	virtual int NewPages(int starting,int n);
	
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual int Load(const char *file);
	virtual int Save(LaidoutSaveFormat format=Save_Normal);
	
	virtual int n() { return pages.n; }
	virtual Laxkit::anObject *object_e(int i) 
		{ if (i>=0 && i<pages.n) return (anObject *)(pages.e[i]); return NULL; }
//	int SaveAs(char *newfile,int format=1); // format: binary, xml, pdata
//	int New();
//	int Print();
};


//------------------------- Project ------------------------------------

class Project
{
 public:
//	StyleManager styles;
	Laxkit::PtrStack<Document> docs;
	Page scratchboard;
	Laxkit::PtrStack<char> project_notes;

	Project();
	virtual ~Project();
};

#endif

