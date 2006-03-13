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
#ifndef IMPOSITIONINST_H
#define IMPOSITIONINST_H

#include "imposition.h"

//------------------------ Singles ---------------------------

class Singles : public Imposition
{
  public:
	double insetl,insetr,insett,insetb;
	int tilex,tiley; 
	Singles();
	virtual ~Singles() {}
	virtual StyleDef *makeStyleDef();
	
//	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
	virtual Laxkit::SomeData *GetPage(int pagenum,int local); // return outline of page in paper coords
	virtual Spread *GetLittleSpread(int whichpage); 
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPaperSize(PaperType *npaper);
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

//------------------------ Double Sided Singles ---------------------------

class DoubleSidedSingles : public Singles
{
 public:
	int isvertical,isleft;
	DoubleSidedSingles();
//	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
	virtual StyleDef *makeStyleDef();
	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Style *duplicate(Style *s=NULL);
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

////------------------------ Booklet ---------------------------
//
class BookletImposition : public DoubleSidedSingles
{
 public:
	double creep;  // booklet.5
	unsigned long covercolor; // booklet.13
	unsigned long bodycolor; // booklet.14
	BookletImposition();
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	
//	virtual SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
	virtual Spread *PaperLayout(int whichpaper);
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


////------------------------ Basic Book ---------------------------
//
//class BasicBook : public Imposition
//{
// public:
//	int numsections;
//	int paperspersection; // 0 does not mean 0. 0 means infinity (or MAX_INT).
//	int creeppersection; 
//	int insetl,insetr,insett,insetb;
//	int tilex,tiley;
//	unsigned long bodycolor;
//	
//	char specifycover;
//	int spinewidth;
//	unsigned long covercolor;
//	PaperStyle coverpaper;
//	PageStyle coverpage;
//	virtual Style *duplicate(Style *s=NULL);
//	
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

////---------------------------------- CompositeImposition ----------------------------
//****
//
//



#endif

