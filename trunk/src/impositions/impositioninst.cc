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
/*************** impositioninst.cc ********************/

#include "impositioninst.h"
#include "stylemanager.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>

using namespace Laxkit;
using namespace LaxInterfaces;
using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 

/*! \file 
 * <pre>
 *  This file defines the standard impositions:
 *   Singles
 *   DoubleSidedSingles
 *   BookletImposition 
 *   BasicBookImposition ***todo
 *   CompositeImposition ***todo
 *
 *  Some other useful ones defined elsewhere are:
 *   NetImposition       netimposition.cc
 *   WhatupImposition    ***todo, holds another imposition, but
 *   						prints multiple papers on a single paper,
 *   						or allows different papers to map to the
 *   						same paper, like posterizing
 * 
 * </pre>
 *
 * \todo *** implement the CompositeImposition, and figure out good way
 *   to integrate into whole system.. that involves clearing up relationship
 *   between Imposition, DocStyle, and Document classes.
 * \todo *** implement BasicBookImposition, WhatupImposition
 * \todo ***** work out dumping in and out of default pagestyles..
 */



//-------------------------- Singles ---------------------------------------------

/*! \class Singles
 * \brief For single page per sheet, not meant to be next to other pages.
 *
 * The pages can be inset a certain amount from each edge, specified by inset[lrtb].
 *
 * \todo imp Spread::spreadtype and PageStyle::pagetype
 */



//Style(StyleDef *sdef,Style *bsdon,const char *nstn)
/*! \todo *** need a global default paper style..
 */
Singles::Singles() : Imposition("Singles")
{ 
	insetl=insetr=insett=insetb=0;
	tilex=tiley=1;
	//paperstyle=*** global default paper style;
	paperstyle=new PaperStyle("letter",8.5,11.0,0,300);//***

			
	pagestyle=NULL;
	setPage();

	//***
	if (!stylemanager.FindDef("PageStyle")) {
		StyleDef *sdef=pagestyle->makeStyleDef();
		stylemanager.AddStyleDef(sdef);
	}
}

//! Calls pagestyle->dec_count().
Singles::~Singles()
{
	pagestyle->dec_count();
}

//! Return the default page style for that page.
/*! Default is to pagestyle->inc_count() then return pagestyle.
 */
PageStyle *Singles::GetPageStyle(int pagenum,int local)
{
	if (!pagestyle) setPage();
	if (!pagestyle) return NULL;
	if (local) return (PageStyle *)pagestyle->duplicate();
	pagestyle->inc_count();
	return pagestyle;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
/*! Return 0 success, nonzero error.
 */
int Singles::SetPaperSize(PaperStyle *npaper)
{
	if (Imposition::SetPaperSize(npaper)) return 1;
	setPage();
	return 0;
}

//! Using the paperstyle, create a new default pagestyle.
/*! dec_count() on old.
 */
void Singles::setPage()
{
	if (!paperstyle) return;
	if (pagestyle) pagestyle->dec_count();
	
	pagestyle=new RectPageStyle(RECTPAGE_LRTB);
	pagestyle->width=(paperstyle->w()-insetl-insetr)/tilex;
	pagestyle->height=(paperstyle->h()-insett-insetb)/tiley;
}

/*! Define from Attribute.
 *
 * Expects defaultpagestyle to either not exist, or be a RectPageStyle
 * with RECTPAGE_LRTB.
 * 
 * Loads the default PaperStyle.. right now just inits
 * to letter if defaultpaperstyle attribute found, then dumps in that
 * paper style. see todos in Page also.....
 */
void Singles::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	int pages=-1;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"insetl")) {
			DoubleAttribute(value,&insetl);
		} else if (!strcmp(name,"insetr")) {
			DoubleAttribute(value,&insetr);
		} else if (!strcmp(name,"insett")) {
			DoubleAttribute(value,&insett);
		} else if (!strcmp(name,"insetb")) {
			DoubleAttribute(value,&insetb);
		} else if (!strcmp(name,"tilex")) {
			IntAttribute(value,&tilex);
		} else if (!strcmp(name,"tiley")) {
			IntAttribute(value,&tiley);
		} else if (!strcmp(name,"numpages")) {
			IntAttribute(value,&numpages);
			if (numpages<0) numpages=0;
		} else if (!strcmp(name,"defaultpagestyle")) {
			pages=c;
			if (pagestyle) pagestyle->dec_count();
			pagestyle=new RectPageStyle(RECTPAGE_LRTB);
			pagestyle->dump_in_atts(att->attributes.e[c]);
		} else if (!strcmp(name,"defaultpaperstyle")) {
			if (paperstyle) delete paperstyle;
			paperstyle=new PaperStyle("Letter",8.5,11,0,300);//***should be global def
			paperstyle->dump_in_atts(att->attributes.e[c]);
		}
	}
	if (pages<0) setPage();
}

/*! Writes out something like:
 * <pre>
 *  insetl 0
 *  insetr 0
 *  insett 0
 *  insetb 0
 *  tilex  1
 *  tiley  1
 *  numpages 10
 *  defaultpagestyle 
 *    ...
 *  defaultpaperstyle
 *    ...
 * </pre>
 */
void Singles::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f,"%sinsetl %.10g\n",spc,insetl);
	fprintf(f,"%sinsetr %.10g\n",spc,insetr);
	fprintf(f,"%sinsett %.10g\n",spc,insett);
	fprintf(f,"%sinsetb %.10g\n",spc,insetb);
	fprintf(f,"%stilex %d\n",spc,tilex);
	fprintf(f,"%stiley %d\n",spc,tiley);
	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
	if (pagestyle) {
		fprintf(f,"%sdefaultpagestyle\n",spc);
		pagestyle->dump_out(f,indent+2,0);
	}
	if (paperstyle) {
		fprintf(f,"%sdefaultpaperstyle\n",spc);
		paperstyle->dump_out(f,indent+2,0);
	}
}

//! Duplicate this, or fill in this attributes.
Style *Singles::duplicate(Style *s)//s=NULL
{
	Singles *sn;
	if (s==NULL) {
		//***stylemanager.newStyle("Singles");
		s=sn=new Singles();
		if (styledef) styledef->inc_count();
		sn->styledef=styledef;
		if (pagestyle) {
			sn->pagestyle->dec_count();
			sn->pagestyle=(RectPageStyle*)pagestyle->duplicate();
		}
	} else sn=dynamic_cast<Singles *>(s);
	if (!sn) return NULL;
	sn->insetl=insetl;
	sn->insetr=insetr;
	sn->insett=insett;
	sn->insetb=insetb;
	sn->tilex=tilex;
	sn->tiley=tiley;
	return Imposition::duplicate(s);  
}

//! ***imp me! Make an instance of the Singles imposition styledef
/*  Required by Style, this defines the various names for fields relevant to Singles,
 *  basically just the inset[lrtb], plus the standard Imposition npages and npapers.
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in the style def manager *** whatever and wherever that is!!!
 */
StyleDef *Singles::makeStyleDef()
{
	return NULL;

//	//StyleDef(const char *nname,const char *nName,const char *ntp, const char *ndesc,unsigned int fflags=STYLEDEF_CAPPED);
//	StyleDef *sd=new StyleDef(NULL,"singlesstyle","Singles",
//			"Imposition of single pages","Imposition of single pages",STYLEDEF_FIELDS);
//
//	//int StyleDef::push(const char *nfield,const char *ttip,const char *ndesc,StyleDef *nfields,unsigned int fflags);
//	**** creationfunc...
//	sd->push("insetl",
//			"Left Inset",
//			"How much a page is inset in a paper on the left",
//			NULL,
//			STYLEDEF_REAL,0);
//	sd->push("insetr",
//			"Right Inset",
//			"How much a page is inset in a paper on the right",
//			NULL,
//			STYLEDEF_REAL,0);
//	sd->push("insett",
//			"Top Inset",
//			"How much a page is inset in a paper from the top",
//			NULL,
//			STYLEDEF_REAL,0);
//	sd->push("insetb",
//			"Bottom Inset",
//			"How much a page is inset in a paper from the bottom",
//			NULL,
//			STYLEDEF_REAL,0);
//	sd->push("tilex",
//			"Tile X",
//			"How many to tile horizontally",
//			NULL,
//			STYLEDEF_INT,0);
//	sd->push("tiley",
//			"Tile Y",
//			"How many to tile vertically",
//			NULL,
//			STYLEDEF_INT,0);
//	return sd;
}

//! Create necessary pages based on default pagestyle.
/*! Currently returns NULL terminated list of pages.
 */
Page **Singles::CreatePages()
{
	if (numpages==0) return NULL;
	Page **pages=new Page*[numpages+1];
	int c;
	PageStyle *ps;
	for (c=0; c<numpages; c++) {
		ps=GetPageStyle(c,0);
		 // pagestyle is passed to Page, not duplicated.
		 // There its count is inc'd.
		pages[c]=new Page(ps,0,c); 
		ps->dec_count(); //remove extra count
	}
	pages[c]=NULL;
	return pages;
}

//! Return outline of page in page coords. 
SomeData *Singles::GetPage(int pagenum,int local)
{
	PathsData *newpath=new PathsData();//count==1
	newpath->appendRect(0,0,pagestyle->w(),pagestyle->h());
	newpath->maxx=pagestyle->w();
	newpath->maxy=pagestyle->h();
	//nothing special is done when local==0
	return newpath;
}

//! Return the single page.
/*! The path created here is one path for the page.
 * The bounds of the page are put in spread->path.
 *
 * The spread->pagestack elements hold only the transform.
 * They do not also have the outlines.
 */
Spread *Singles::PageLayout(int whichpage)
{
	return SingleLayout(whichpage);
}

//! Return a paper spread with 1 page on it, using the inset values.
/*! The path created here is one path for the paper, and another for the possibly inset page.
 */
Spread *Singles::PaperLayout(int whichpaper)
{
	Spread *spread=new Spread();
	spread->style=SPREAD_PAPER;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;
	
	 // define max/min points
	spread->minimum=flatpoint(paperstyle->w()/5,paperstyle->h()/2);
	spread->maximum=flatpoint(paperstyle->w()*4/5,paperstyle->h()/2);

	 // fill spread with paper and page outline
	PathsData *newpath=new PathsData();
	spread->path=(SomeData *)newpath;
	spread->pathislocal=1;
	
	 // make the paper outline
	newpath->appendRect(0,0,paperstyle->w(),paperstyle->h());
	
	 // make the outline around the inset, then lines to demarcate the tiles
	 // there are tilex*tiley pages, all pointing to the same page data
	newpath->pushEmpty(); // later could have a certain linestyle
	newpath->appendRect(insetl,insetb, paperstyle->w()-insetl-insetr,paperstyle->h()-insett-insetb);
	int x,y;
	for (x=1; x<tilex; x++) {
		newpath->pushEmpty();
		newpath->append(insetl+x*(paperstyle->w()-insetr-insetl)/tilex, insett);
		newpath->append(insetl+x*(paperstyle->w()-insetr-insetl)/tilex, insetb);
	}
	for (y=1; y<tiley; y++) {
		newpath->pushEmpty();
		newpath->append(insetl, insetb+y*(paperstyle->h()-insetb-insett)/tiley);
		newpath->append(insetr, insetb+y*(paperstyle->h()-insetb-insett)/tiley);
	}
	
	 // setup spread->pagestack
	 // page width/height must map to proper area on page.
	 // makes rects with local origin in ll corner
	PathsData *ntrans;
	for (x=0; x<tilex; x++) {
		for (y=0; y<tiley; y++) {
			ntrans=new PathsData();
			ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
			ntrans->FindBBox();
			ntrans->origin(flatpoint(insetl+x*(paperstyle->w()-insetr-insetl)/tilex,
									 insetb+y*(paperstyle->h()-insett-insetb)/tiley));
			spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans,1));
		}
	}
	
		
	 // make printer marks if necessary
	 //*** make this more responsible lengths:
	if (insetr>0 || insetl>0 || insett>0 || insetb>0) {
		spread->mask|=SPREAD_PRINTERMARKS;
		PathsData *marks=new PathsData();
		if (insetl>0) {
			marks->pushEmpty();
			marks->append(0,        paperstyle->h()-insett);
			marks->append(insetl*.9,paperstyle->h()-insett);
			marks->pushEmpty();
			marks->append(0,        insetb);
			marks->append(insetl*.9,insetb);
		}
		if (insetr>0) {
			marks->pushEmpty();
			marks->append(paperstyle->w(),          paperstyle->h()-insett);
			marks->append(paperstyle->w()-.9*insetr,paperstyle->h()-insett);
			marks->pushEmpty();
			marks->append(paperstyle->w(),          insetb);
			marks->append(paperstyle->w()-.9*insetr,insetb);
		}
		if (insetb>0) {
			marks->pushEmpty();
			marks->append(insetl,0);
			marks->append(insetl,.9*insetb);
			marks->pushEmpty();
			marks->append(paperstyle->w()-insetr,0);
			marks->append(paperstyle->w()-insetr,.9*insetb);
		}
		if (insett>0) {
			marks->pushEmpty();
			marks->append(insetl,paperstyle->h());
			marks->append(insetl,paperstyle->h()-.9*insett);
			marks->pushEmpty();
			marks->append(paperstyle->w()-insetr,paperstyle->h());
			marks->append(paperstyle->w()-insetr,paperstyle->h()-.9*insett);
		}
		spread->marks=marks;
		spread->marksarelocal=1;
	}

	 
	return spread;
}

//! Returns { frompage,topage, singlepage,-1,anothersinglepage,-1,-2 }
int *Singles::PrintingPapers(int frompage,int topage)
{
	if (frompage<topage) {
		int t=frompage;
		frompage=topage;
		topage=t;
	}
	int *blah=new int[3];
	blah[0]=frompage;
	blah[1]=topage;
	blah[2]=-2;
	return blah;
}

//! Just return pagenumber, since 1 page==1 paper
int Singles::PaperFromPage(int pagenumber) // the paper number containing page pagenumber
{ return pagenumber; }

//! Is singles, so 1 paper=1 page
int Singles::GetPagesNeeded(int npapers) 
{ return npapers; }

//! Is singles, so 1 page=1 paper
int Singles::GetPapersNeeded(int npages) 
{ return npages; } 

//! Is singles, so 1 page=1 spread
int Singles::GetSpreadsNeeded(int npages)
{ return npages; } 

int Singles::PageType(int page)
{ return 0; }

int Singles::SpreadType(int spread)
{ return 0; }


//-------------------------------- DoubleSidedSingles ------------------------------------------------------

/*! \class DoubleSidedSingles
 * \brief For 1 page per sheet, arranged so the pages are to be placed next to each other, first page is like the cover.
 *
 * Please note that Singles::inset* that DoubleSidedSingles inherits are not margins.
 * The insets refer to portions of the paper that would later be chopped off, and are the 
 * same for each page, whether the page is on the left or the right.
 *
 * \todo *** it is imperative to be able to modify whether the first page is a left page
 * or a right page: (isleft is tag for that, currently ignored.. finish me!)
 */
/*! \var int DoubleSidedSingles::isvertical
 * \brief Nonzero if pages are top and bottom, rather than left and right.
 */
/*! \var int DoubleSidedSingles::isleft
 * \brief 1 if the first page is not by itself.
 *
 * Spreads would go lr-lr-lr for isleft==1.\n
 * Spreads would go  r-lr-lr for isleft==0.
 */
/*! \var RectPageStyle *DoubleSidedSingles::pagestyler
 * \brief The style of a page on the right or bottom. Singles::pagestyle is left or top.
 */


//! Set isvertical=0, call setpage().
DoubleSidedSingles::DoubleSidedSingles()
{
	isvertical=0;
	isleft=0;
	setPage();
	
	 // make style instance name "Double Sided Singles"  perhaps remove the spaces??
	makestr(stylename,"Double Sided Singles");

	styledef=stylemanager.FindDef("Double Sided Singles");
} 

//! Using the paperstyle and isvertical, create new default pagestyles.
void DoubleSidedSingles::setPage()
{
	if (pagestyle) pagestyle->dec_count();
	pagestyle=new RectPageStyle((isvertical?(RECTPAGE_LRIO|RECTPAGE_LEFTPAGE):(RECTPAGE_IOTB|RECTPAGE_TOPPAGE)));
	pagestyle->pagetype=getUniqueNumber();			

	if (pagestyler) pagestyler->dec_count();
	pagestyler=new RectPageStyle((isvertical?(RECTPAGE_LRIO|RECTPAGE_RIGHTPAGE):(RECTPAGE_IOTB|RECTPAGE_BOTTOMPAGE)));
	pagestyler->pagetype=getUniqueNumber();			
				
	pagestyler->width= pagestyle->width =(paperstyle->w()-insetl-insetr)/tilex;
	pagestyler->height=pagestyle->height=(paperstyle->h()-insett-insetb)/tiley;
}

//! Return the default page style for that page.
/*! Default is to pagestyle->inc_count() then return pagestyle.
 */
PageStyle *DoubleSidedSingles::GetPageStyle(int pagenum,int local)
{
	if (!pagestyle || !pagestyler) setPage();
	if (!pagestyle || !pagestyler) return NULL;
	PageStyle *p;
	int c=PageType(pagenum);
	if (c==1 || c==2) p=pagestyle; else p=pagestyler;
	if (local) return (PageStyle *)p->duplicate();
	p->inc_count();
	return p;
}

/*! \todo *** must figure out best way to sync up pagestyles...
 */
void DoubleSidedSingles::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"isvertical")) {
			isvertical=BooleanAttribute(value);
		}
		if (!strcmp(name,"isleft") || !strcmp(name,"istop")) {
			isleft=BooleanAttribute(value);
		} else if (!strcmp(name,"defaultpagestyler")) {
			//pages=c;
			if (pagestyler) pagestyler->dec_count();
			pagestyler=new RectPageStyle();
			pagestyler->dump_in_atts(att->attributes.e[c]);
		}
	}
	Singles::dump_in_atts(att);
}

/*! Write out isvertical, then Singles::dump_out.
 */
void DoubleSidedSingles::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (isvertical) fprintf(f,"%sisvertical\n",spc);
	if (isleft)
		if (isvertical) fprintf(f,"%sistop\n",spc);
		else fprintf(f,"%sisleft\n",spc);
	if (pagestyler) {
		fprintf(f,"%sdefaultpagestyler\n",spc);
		pagestyler->dump_out(f,indent+2,0);
	}
	Singles::dump_out(f,indent,0);
}

//! Copies over isvertical.
Style *DoubleSidedSingles::duplicate(Style *s)//s=NULL
{
	DoubleSidedSingles *ds;
	if (s==NULL) s=ds=new DoubleSidedSingles();
	else {
		ds=dynamic_cast<DoubleSidedSingles *>(s);
		if (!ds) return NULL;
	}
	if (!ds) return NULL;
	ds->isvertical=isvertical;
	ds->isleft=isleft;
	return Singles::duplicate(s);  
}

//! ***imp me! Make an instance of the DoubleSidedSingles imposition styledef
/*  Required by Style, this defines the various names for fields relevant to DoubleSidedSingles.
 *  Basically just the inset[lrtb], plus the standard Imposition npages and npapers,
 *  and isvertical (which is a flag to say whether this is a booklet or a calendar).
 *
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in the style def manager *** whatever and wherever that is!!!
 */
StyleDef *DoubleSidedSingles::makeStyleDef()
{
	return NULL;
//	//StyleDef(const char *nname,const char *nName,const char *ntp, const char *ndesc,unsigned int fflags=STYLEDEF_CAPPED);
//	StyleDef *sd=new StyleDef(stylemanager.FindDef("Singles"),"doublesidetsingles","Double Sided Singles",
//			"Imposition of single pages meant to be next to each other",
//			"Imposition of single pages meant to be next to each other",
//			STYLEDEF_FIELDS);
//	***

}

//! Return a box describing a good scratchboard size for paper layout.
/*! Default is to return bounds 4 times the page size or 3 times the paper size.
 *
 * Place results in bbox if bbox!=NULL. If bbox==NULL, then create a new DoubleBBox and return that.
 */
Laxkit::DoubleBBox *DoubleSidedSingles::GoodWorkspaceSize(Laxkit::DoubleBBox *bbox)//page=1
{
	if (paperstyle) {
		if (!bbox) bbox=new DoubleBBox();
		if (isvertical) bbox->setbounds(-paperstyle->w(),2*paperstyle->w(),-paperstyle->h(),3*paperstyle->h());
		else bbox->setbounds(-paperstyle->w(),3*paperstyle->w(),-paperstyle->h(),2*paperstyle->h());
	} else return NULL;
	return bbox;
}

//! Create necessary pages based on default pagestyle
/*! Currently returns NULL terminated list of pages.
 */
Page **DoubleSidedSingles::CreatePages()
{
	if (numpages==0) return NULL;
	if (!pagestyle || !pagestyler) return NULL;
	
	Page **newpages=new Page*[numpages+1];
	int c;
	for (c=0; c<numpages; c++) {
		newpages[c]=new Page(((c+isleft)%2==0?pagestyler:pagestyle),0,c); // this checksout left or right
	}
	newpages[c]=NULL;
	return newpages;
}


//! Return a page spread based on page index (starting from 0) whichpage.
/*! The path created here is one path for the page(s), and another for between pages.
 * The bounds of the spread are put in spread->path.
 * The first page has only itself (if !isleft).
 * If the last page (numbered from 0) is odd, it also has only itself.
 * All other spreads have 2 pages.
 *
 * The spread->pagestack elements hold only the transform.
 * They do not also have the outlines.
 */
Spread *DoubleSidedSingles::PageLayout(int whichspread)
{
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_MINIMUM|SPREAD_MAXIMUM;
	int whichpage=whichspread*2-isleft,
		left=((whichpage+1)/2)*2-1+isleft,
		right=left+1;

	 // fill spread path with 2 page box
	PathsData *newpath=new PathsData();
	newpath->maxx=(isvertical?1:2)*pagestyle->w();
	newpath->maxy=(isvertical?2:1)*pagestyle->h();
	spread->path=(SomeData *)newpath;
	spread->pathislocal=1;

	if (whichpage==0 || whichpage==numpages-1 && whichpage%2==1) {
		 // first and possibly last are just single pages, so just have single box
		if (whichpage==0) newpath->appendRect(pagestyle->width,0,pagestyle->width,pagestyle->height);
		else newpath->appendRect(0,0,pagestyle->width,pagestyle->height);
	} else {
		 // 2 lines, 1 for 2 pages, and line down the middle
		newpath->appendRect(0,0,2*pagestyle->width,pagestyle->height);
		newpath->pushEmpty();
		newpath->append(pagestyle->width,0);
		newpath->append(pagestyle->width,pagestyle->height);
		newpath->FindBBox();
	}

	 // setup spread->pagestack with the single pages.
	 // page width/height must map to proper area on page.
	 //*** maybe keep around a copy of the outline, then checkin in destructor rather
	 //than GetPage here?
	SomeData *noutline=GetPage(0,0); //this checks out the outline
	Group *g=new Group;
	g->push(noutline,0); // this checks it out again..
	g->FindBBox();
	noutline->dec_count(); // This removes the extra unnecessary tick

	 // left page
	if (left>=0) {
		spread->pagestack.push(new PageLocation(left,NULL,g,1));
		g=NULL;
		spread->minimum=flatpoint(pagestyle->w()/5,pagestyle->h()/2);
	} else {
		spread->minimum=flatpoint(pagestyle->w()*6/5,pagestyle->h()/2);
	}

	 // right page
	 //**** NOTE the index might be > pages.n... but numpages should be the correct value.... 
	 //**** bit of an ugly disparity, storing what should be the same data in two places.....
	if (right<numpages) {
		if (!g) {
			g=new Group();
			g->push(noutline,0); // this checks out outline..
		}
		g->m()[4]+=pagestyle->width;
		g->FindBBox();
		spread->pagestack.push(new PageLocation(right,NULL,g,1));
		spread->maximum=flatpoint(pagestyle->w()*9/5,pagestyle->h()/2);
	} else {
		spread->maximum=flatpoint(pagestyle->w()*4/5,pagestyle->h()/2);
	}

	return spread;
}

//! Return a paper spread with 1 page on it
/*! The path created here is one path for the paper, and another for the possibly inset page.
 * 
 * \todo *** must modify the singles layout to swap the l/r (or t/b) insets for facing pages
 */
Spread *DoubleSidedSingles::PaperLayout(int whichpaper)
{
	//*** must modify the singles layout to swap the l/r (or t/b) insets for facing pages
	return Singles::PaperLayout(whichpaper);
}

//! Return (npages-isleft)/2+1.
int DoubleSidedSingles::GetSpreadsNeeded(int npages)
{ return (npages-isleft)/2+1; } 


/*! 0=right
 *  1=left
 *  2=top
 *  3=bottom
 */
int DoubleSidedSingles::PageType(int page)
{ 
	int left=((page+1)/2)*2-1+isleft;
	if (left==page && isvertical) return 2;
	if (left==page) return 1;
	if (isvertical) return 3;
	return 0; 
	
}

/*! 0=right only
 *  1=left only
 *  2=top only
 *  3=bottom only
 *  4=l + r
 *  5=t + b
 */
int DoubleSidedSingles::SpreadType(int spread)
{
	int page=spread*2-isleft,left,right;
	if (page<0) page=0;
	left=((page+1)/2)*2-1+isleft,
	right=left+1;
	if (left==page)
		if (right<NumPages()) 
			if (isvertical) return 5;
			else return 4;
		else if (isvertical) return 2;
		else return 1;
			
	if (left>=0) 
		if (isvertical) return 5;
		else return 4;
	if (isvertical) return 3;
	return 0;
}

//---------------------------------- BookletImposition -----------------------------------------

/*! \class BookletImposition
 * \brief A imposition made of one bunch of papers folded down the middle.
 *
 * The papers can be folded vertically in the middle like a book or folded horizontally like
 * a calendar. Also, the whole shebang can be tiled across the paper so that when
 * printing, you would cut along the tile lines, then fold the result.
 *
 * The first paper (which holds the first 2 and last 2 pages) can optionally
 * have a different color than the body.
 *
 * DoubleSidedSingles::isleft is not used.
 *
 * \todo *** tiling and cut marks are not functional yet
 */


//! Constructor, init new variables, make style name="Booklet"
BookletImposition::BookletImposition()
{
	creep=0;
	covercolor=bodycolor=~0;
	makestr(stylename,"Booklet");
	setPage();
}

//! Using the paperstyle and isvertical, create a new default pagestyle.
void BookletImposition::setPage()
{
	if (pagestyle) delete pagestyle;
	pagestyle=new RectPageStyle((isvertical?RECTPAGE_LRIO:RECTPAGE_IOTB));
	pagestyle->width=(paperstyle->w()-insetl-insetr)/tilex;
	pagestyle->height=(paperstyle->h()-insett-insetb)/tiley;
	if (isvertical) pagestyle->height/=2;
	else pagestyle->width/=2;
}

//! Read creep, body and covercolor, then DoubleSidedSigles::dump_in_atts().
void BookletImposition::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"creep")) {
			DoubleAttribute(value,&creep);
		} else if (!strcmp(name,"bodycolor")) {
			if (value) bodycolor=strtol(value,NULL,0);
		} else if (!strcmp(name,"covercolor")) {
			if (value) covercolor=strtol(value,NULL,0);
		}
	}
	DoubleSidedSingles::dump_in_atts(att);
	isleft=0;
}

/*! Something like:
 *  <pre>
 *    creep .05
 *    bodycolor 0xffffffff
 *    colorcolor 0xffffffff
 *    ...DoubleSidedSingles stuff..
 *  </pre>
 */
void BookletImposition::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f,"%screep %.10g\n",spc,creep);
	fprintf(f,"%sbodycolor 0x%.6lx\n",spc,bodycolor);
	fprintf(f,"%scovercolor 0x%.6lx\n",spc,covercolor);
	DoubleSidedSingles::dump_out(f,indent,0);
}

//! Duplicate. Copies over creep, tilex, tily, bodycolor, covercolor.
Style *BookletImposition::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new BookletImposition();
	else if (!dynamic_cast<BookletImposition *>(s)) return NULL;
	BookletImposition *b=dynamic_cast<BookletImposition *>(s);
	if (!b) return NULL;
	b->creep=creep;
	b->tilex=tilex;
	b->tiley=tiley;
	b->bodycolor=bodycolor;
	b->covercolor=covercolor;
	return DoubleSidedSingles::duplicate(s);  
}

//! ***imp me!
StyleDef *BookletImposition::makeStyleDef()
{//***
	return NULL;
}

//--- can use DoubleSidedSingles::CreatePages
//Page **BookletImposition::CreatePages(PageStyle *pagestyle=NULL)
//{ ***
//}

/*! Layout booklet with tiling.
 *
 * \todo *** use isvertical
 */
Spread *BookletImposition::PaperLayout(int whichpaper)
{//***
	if (!numpapers) return NULL;
	if (whichpaper<0 || whichpaper>=numpapers) whichpaper=0;

	 // grab singles, which draws a tiled page using the inset values,
	 // including printer marks, and max and min points.
	 // but the pagestack is incorrect. All but pagestack is ok
	Spread *spread=Singles::PaperLayout(whichpaper);

	 // fill pagestack, includes tiling
	spread->pagestack.flush();
	int x,y,lpg,rpg,npgs=GetPagesNeeded(numpapers);
	PathsData *ntrans;
	for (x=0; x<tilex; x++) {
		for (y=0; y<tiley; y++) {
			 //install 2 page cells for each tile, according to isvertical
			 //the left or top:
			if (whichpaper%2==1) { // odd numbered pages are always on left...
				 // make right and left side be correct page number
				lpg=whichpaper; 
				rpg=npgs-whichpaper-1; 
			} else {
				 // make right and left side be correct page number
				lpg=npgs-whichpaper-1;
				rpg=whichpaper;
			}
			
			if (lpg>=0 && lpg<numpages) {
				ntrans=new PathsData();
				ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
				ntrans->origin(flatpoint(insetl+x*(paperstyle->w()-insetr-insetl)/tilex,
										 insetb+y*(paperstyle->h()-insett-insetb)/tiley));
				spread->pagestack.push(new PageLocation(lpg,NULL,ntrans,1));
			}
			 //the right or bottom:
			if (rpg>=0 && rpg<numpages) {
				ntrans=new PathsData();
				ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
				ntrans->origin(flatpoint(isvertical?0:pagestyle->w(), isvertical?pagestyle->h():0)
							   + flatpoint(insetl+x*(paperstyle->w()-insetr-insetl)/tilex,
										   insetb+y*(paperstyle->h()-insett-insetb)/tiley));
				spread->pagestack.push(new PageLocation(rpg,NULL,ntrans,1));
			}
		}
	}

	return spread;
}

//! Returns { frompage,topage, singlepage,-1,anothersinglepage,-1,-2 }
int *BookletImposition::PrintingPapers(int frompage,int topage)
{
	int lcenterpage=numpages/2; // The left or upper page of the centerfold
	int *i=new int[3];
	if (topage<=lcenterpage) { // all pages left of fold
		i[0]=frompage;
		i[1]=topage;
		i[2]=-2;
		return i;
	} else if (frompage>lcenterpage) { // all pages right of fold
		i[0]=numpages-topage-1;
		i[1]=numpages-frompage-1;
		i[2]=-2;
		return i;
	} else { // page range straddles fold
		if (numpages-topage-1<frompage) frompage=numpages-topage-1;
		i[0]=frompage;
		i[1]=lcenterpage;
		i[2]=-2;
	}
	return i;
}

//! If pagenumber<=numpapers return pagenumber, else return numpapers-(pagenumber-numpapers)-1.
int BookletImposition::PaperFromPage(int pagenumber)
{
	if (pagenumber+1<=numpapers) return pagenumber;
	return numpapers-(pagenumber-numpapers)-1;
}

//! Is (int((npapers-1)/2)+1)*4. This assumes actually printing double sided later on..
/*! The actual number of physical papers when printed double sided is half numpapers.
 */
int BookletImposition::GetPagesNeeded(int npapers)
{ return ((npapers-1)/2+1)*4; }

//! Get the number of papers needed to hold this many pages.  Is just (floor((npages-1)/4)+1)*2.
/*! The value for number of physical papers after printing out double sided is half the returned number.
 */
int BookletImposition::GetPapersNeeded(int npages)
{ return ((npages-1)/4+1)*2; }

//! Return (npages)/2+1.
int BookletImposition::GetSpreadsNeeded(int npages)
{ return (npages)/2+1; } 


////---------------------------------- CompositeImposition ----------------------------
//
// // class to enable certain page ranges to be administred by different impositions...
//class CompositeImposition : public Imposition
//{
// protected:
//	Imposition *impos;
//	Ranges ranges; 
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//}



////----------------------------------- BasicBook -------------------------------------------
///*! \class BasicBook
// * \brief A more general imposition geared more for books with several sections.
// *
// * A book in this case has muliple numbers of sections, and each section has a certain
// * number of pages. Each of these sections would then be folded and assembled back
// * to back to form the body pages and sewn onto binding tape, and fastened onto the
// * book cover. Alternately, the sections could just be chopped in half the result
// * perfect bound with the cover.
// *
// * The optional cover is mostly for perfect bound books. It will have different dimensions 
// * than the body page because of the extra thickness of the spine. Indeed the cover often 
// * times printed on a different size piece of paper to accomodate that. For instance, I often 
// * make the body pages legal size paper chopped in half, and print the covers on thick
// * tabloid size paper, then trim it all down after the cover is attached to the body.
// *
// * This imposition will automatically set up the cover paper to have the proper cut marks
// * to fit around the sections with the specified spine thickness. Specifically, this means
// * the vertical dimension of the coverpage will be the same as a body page, but the width
// * will be 2 pages plus the spine width.
// */
////************* should this be broken down into a Project, rather than complete Imposition????
////************* because it implies 2 different breakdowns of pages. Imposition should be one kind of breakdown
////class BasicBook : public Imposition
////{
//// public:
////	int numsections;
////	int paperspersection; // 0 does not mean 0. 0 means infinity (or MAX_INT).
////	int creeppersection; 
////	int insetl,insetr,insett,insetb;
////	int tilex,tiley;
////	unsigned long bodycolor;
////	
////	char specifycover;
////	int spinewidth;
////	unsigned long covercolor;
////	PaperStyle coverpaper;
////	PageStyle coverpage;
////	
////	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
////	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
////};
//
//BasicBook::BasicBook() : Style(NULL,NULL,"Basic Book")
//{ }
//
////! *** imp me!
//void BasicBook::dump_in_atts(Attribute *att)
//{***
//	if (!att) return;
//	char *name,*value;
//	for (int c=0; c<att->attributes.n; c++) {
//		name= att->attributes.e[c]->name;
//		value=att->attributes.e[c]->value;
//		if (!strcmp(name,"creep")) {
//			DoubleAttribute(value,&creep);
//		} else if (!strcmp(name,bodycolor)) {
//			if (value) bodycolor=strtol(value,NULL,0);
//		} else if (!strcmp(name,covercolor)) {
//			if (value) covercolor=strtol(value,NULL,0);
//		}
//	}
//	DoubleSidedSingles::dump_in_atts(att);
//}
//
////! Write out flags, width, height
//void BasicBook::dump_out(FILE *f,int indent,int what)
//{
//	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
//	fprintf(f,"%swidth %s\n",spc,w());
//	fprintf(f,"%sheight %s\n",spc,h());
//	if (flags&MARGINS_CLIP) fprintf(f,"%smarginsclip\n",spc);
//	if (flags&FACING_PAGES_BLEED) fprintf(f,"%sfacingpagesbleed\n",spc);
//}
//
//int BasicBook::GetPagesNeeded(int npapers)
//{ return npapers*4; }
//
////! Get the number of papers needed to hold this many pages.
///*! This uses the same sheets per section, and may imply a change
// *  in the number of sections actually used.
// */
//int BasicBook::GetPapersNeeded(int npages)
//{*** 
//	if (!paperspersection) return (npages-1)/4;
//	return ((npages-1)/4/paperspersection+1)*paperspersection; 
//}
//
//
