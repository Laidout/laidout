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



/*! \todo *** rethink styledef assignment..
 */
Singles::Singles() : Imposition("Singles")
{ 
	insetl=insetr=insett=insetb=0;
	tilex=tiley=1;
	pagestyle=NULL;

	PaperStyle *paperstyle=dynamic_cast<PaperStyle *>(stylemanager.FindStyle("defaultpapersize"));
	if (paperstyle) paperstyle=static_cast<PaperStyle *>(paperstyle->duplicate());
	else paperstyle=new PaperStyle("letter",8.5,11.0,0,300);
	SetPaperSize(paperstyle);
	paperstyle->dec_count();
			
	//pagestyle=NULL;
	//setPage();***<--called from SetPaperSize

	styledef=stylemanager.FindDef("Singles");
	if (styledef) styledef->inc_count(); 
	else {
		styledef=makeStyleDef();
		if (styledef) stylemanager.AddStyleDef(styledef);
		 // so this new styledef should have a count of 2. The destructor removes
		 // 1 count, and the stylemanager should remove the other
	}
	
	DBG cerr <<"imposition singles init"<<endl;
}

//! Calls pagestyle->dec_count().
Singles::~Singles()
{
	DBG cerr <<"--Singles destructor"<<endl;
	pagestyle->dec_count();
}

//! Using the paperstyle, create a new default pagestyle.
/*! dec_count() on old.
 */
void Singles::setPage()
{
	if (!paper) return;
	if (pagestyle) pagestyle->dec_count();
	
	pagestyle=new RectPageStyle(RECTPAGE_LRTB);
	pagestyle->width=(paper->media.maxx-insetl-insetr)/tilex;
	pagestyle->height=(paper->media.maxy-insett-insetb)/tiley;
	pagestyle->pagetype=0;
}

//! Return the default page style for that page.
/*! Default is to pagestyle->inc_count() then return pagestyle.
 */
PageStyle *Singles::GetPageStyle(int pagenum,int local)
{
	if (!pagestyle) setPage();
	if (!pagestyle) return NULL;
	if (local) {
		PageStyle *ps=(PageStyle *)pagestyle->duplicate();
		ps->flags|=PAGESTYLE_AUTONOMOUS;
		return ps;
	}
	pagestyle->inc_count();
	return pagestyle;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer tranfser.
/*! Calls Imposition::SetPaperSize(npaper), then setPage().
 *
 * Return 0 success, nonzero error.
 */
int Singles::SetPaperSize(PaperStyle *npaper)
{
	if (Imposition::SetPaperSize(npaper)) return 1;
	setPage();
	return 0;
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
void Singles::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
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
			pagestyle->dump_in_atts(att->attributes.e[c],flag,context);
		} else if (!strcmp(name,"defaultpaperstyle")) {
			PaperStyle *paperstyle;
			paperstyle=new PaperStyle("Letter",8.5,11,0,300);//***should be global def
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			SetPaperSize(paperstyle);
			paperstyle->dec_count();
		} else if (!strcmp(name,"defaultpapers")) {
			if (papergroup) papergroup->dec_count();
			papergroup=new PaperGroup;
			papergroup->dump_in_atts(att->attributes.e[c],flag,context);
			if (papergroup->papers.n) {
				if (paper) paper->dec_count();
				paper=papergroup->papers.e[0]->box;
				paper->inc_count();
			}
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
 *
 * If what==-1, dump out a pseudocode mockup of the file format.
 *
 * \todo *** finish what==-1
 */
void Singles::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s#insets are regions of a paper not taken up by the page\n",spc);
		fprintf(f,"%sinsetl 0   #The left inset from the side of a paper\n",spc);
		fprintf(f,"%sinsetr 0   #The right inset from the side of a paper\n",spc);
		fprintf(f,"%sinsett 0   #The top inset from the side of a paper\n",spc);
		fprintf(f,"%sinsetb 0   #The bottom inset from the side of a paper\n",spc);
		fprintf(f,"%stilex 1    #number of times to tile the page horizontally\n",spc);
		fprintf(f,"%stiley 1    #number of times to tile the page vertically\n",spc);
		fprintf(f,"%snumpages 3 #number of pages in the document. This is ignored on readin\n",spc);
		fprintf(f,"%sdefaultpapers #default paper group\n",spc);
		papergroup->dump_out(f,indent+2,-1,NULL);
		fprintf(f,"%sdefaultpagestyle #default page style\n",spc);
		pagestyle->dump_out(f,indent+2,-1,NULL);
		return;
	}
	fprintf(f,"%sinsetl %.10g\n",spc,insetl);
	fprintf(f,"%sinsetr %.10g\n",spc,insetr);
	fprintf(f,"%sinsett %.10g\n",spc,insett);
	fprintf(f,"%sinsetb %.10g\n",spc,insetb);
	fprintf(f,"%stilex %d\n",spc,tilex);
	fprintf(f,"%stiley %d\n",spc,tiley);
	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
	if (pagestyle) {
		fprintf(f,"%sdefaultpagestyle\n",spc);
		pagestyle->dump_out(f,indent+2,0,context);
	}
	if (papergroup) {
		fprintf(f,"%sdefaultpapers\n",spc);
		papergroup->dump_out(f,indent+2,0,context);
	}
}

//! Duplicate this, or fill in this attributes.
Style *Singles::duplicate(Style *s)//s=NULL
{
	Singles *sn;
	if (s==NULL) {
		//***stylemanager.newStyle("Singles");
		s=sn=new Singles();
	} else sn=dynamic_cast<Singles *>(s);
	if (!sn) return NULL;
	if (styledef) {
		styledef->inc_count();
		if (sn->styledef) sn->styledef->dec_count();
		sn->styledef=styledef;
	}
	if (pagestyle) {
		if (sn->pagestyle) sn->pagestyle->dec_count();
		pagestyle->inc_count();
		sn->pagestyle=pagestyle;
	}
	sn->insetl=insetl;
	sn->insetr=insetr;
	sn->insett=insett;
	sn->insetb=insetb;
	sn->tilex=tilex;
	sn->tiley=tiley;
	return Imposition::duplicate(s);  
}

//! The newfunc for Singles instances.
Style *NewSingles(StyleDef *def)
{ 
	Singles *s=new Singles;
	s->styledef=def;
	return s;
}

//! Make an instance of the Singles imposition styledef.
/*  Required by Style, this defines the various names for fields relevant to Singles,
 *  basically just the inset[lrtb], plus the standard Imposition npages and npapers.
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in a StyleManager.
 *
 *  Returns a new StyleDef with a count of 1.
 */
StyleDef *Singles::makeStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,"Singles","Singles",
			"Imposition of single pages",
			"Imposition of single pages",
			Element_Fields,
			NULL,NULL,
			NULL,
			0, //new flags
			NewSingles);

	sd->push("insetl",
			"Left Inset",
			"How much a page is inset in a paper on the left",
			NULL,
			Element_Real,
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("insetr",
			"Right Inset",
			"How much a page is inset in a paper on the right",
			NULL,
			Element_Real,
			NULL,
			"0",
			0,NULL);
	sd->push("insett",
			"Top Inset",
			"How much a page is inset in a paper from the top",
			NULL,
			Element_Real,
			NULL,
			"0",
			0,0);
	sd->push("insetb",
			"Bottom Inset",
			"How much a page is inset in a paper from the bottom",
			NULL,
			Element_Real,
			NULL,
			"0",
			0,0);
	sd->push("tilex",
			"Tile X",
			"How many to tile horizontally",
			NULL,
			Element_Int,
			NULL,
			"1",
			0,0);
	sd->push("tiley",
			"Tile Y",
			"How many to tile vertically",
			NULL,
			Element_Int,
			NULL,
			"1",
			0,0);
	return sd;
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
SomeData *Singles::GetPageOutline(int pagenum,int local)
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

	if (papergroup) {
		spread->papergroup=papergroup;
		spread->papergroup->inc_count();
	}
	
	 // define max/min points
	spread->minimum=flatpoint(paper->media.maxx/5,  paper->media.maxy/2);
	spread->maximum=flatpoint(paper->media.maxx*4/5,paper->media.maxy/2);

	 // fill spread with paper and page outline
	PathsData *newpath=new PathsData();
	spread->path=(SomeData *)newpath;
	
	 // make the paper outline
	newpath->appendRect(0,0,paper->media.maxx,paper->media.maxy);
	
	 // make the outline around the inset, then lines to demarcate the tiles
	 // there are tilex*tiley pages, all pointing to the same page data
	newpath->pushEmpty(); // later could have a certain linestyle
	newpath->appendRect(insetl,insetb, paper->media.maxx-insetl-insetr,paper->media.maxy-insett-insetb);
	int x,y;
	for (x=1; x<tilex; x++) {
		newpath->pushEmpty();
		newpath->append(insetl+x*(paper->media.maxx-insetr-insetl)/tilex, insett);
		newpath->append(insetl+x*(paper->media.maxx-insetr-insetl)/tilex, insetb);
	}
	for (y=1; y<tiley; y++) {
		newpath->pushEmpty();
		newpath->append(insetl, insetb+y*(paper->media.maxy-insetb-insett)/tiley);
		newpath->append(insetr, insetb+y*(paper->media.maxy-insetb-insett)/tiley);
	}
	
	 // setup spread->pagestack
	 // page width/height must map to proper area on page.
	 // makes rects with local origin in ll corner
	PathsData *ntrans;
	for (x=0; x<tilex; x++) {
		for (y=0; y<tiley; y++) {
			ntrans=new PathsData();//count of 1
			ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
			ntrans->FindBBox();
			ntrans->origin(flatpoint(insetl+x*(paper->media.maxx-insetr-insetl)/tilex,
									 insetb+y*(paper->media.maxy-insett-insetb)/tiley));
			spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans));//ntrans count++
			ntrans->dec_count();//remove extra count
		}
	}
	
		
	 // make printer marks if necessary
	 //*** make this more responsible lengths:
	if (insetr>0 || insetl>0 || insett>0 || insetb>0) {
		spread->mask|=SPREAD_PRINTERMARKS;
		PathsData *marks=new PathsData();
		if (insetl>0) {
			marks->pushEmpty();
			marks->append(0,        paper->media.maxy-insett);
			marks->append(insetl*.9,paper->media.maxy-insett);
			marks->pushEmpty();
			marks->append(0,        insetb);
			marks->append(insetl*.9,insetb);
		}
		if (insetr>0) {
			marks->pushEmpty();
			marks->append(paper->media.maxx,          paper->media.maxy-insett);
			marks->append(paper->media.maxx-.9*insetr,paper->media.maxy-insett);
			marks->pushEmpty();
			marks->append(paper->media.maxx,          insetb);
			marks->append(paper->media.maxx-.9*insetr,insetb);
		}
		if (insetb>0) {
			marks->pushEmpty();
			marks->append(insetl,0);
			marks->append(insetl,.9*insetb);
			marks->pushEmpty();
			marks->append(paper->media.maxx-insetr,0);
			marks->append(paper->media.maxx-insetr,.9*insetb);
		}
		if (insett>0) {
			marks->pushEmpty();
			marks->append(insetl,paper->media.maxy);
			marks->append(insetl,paper->media.maxy-.9*insett);
			marks->pushEmpty();
			marks->append(paper->media.maxx-insetr,paper->media.maxy);
			marks->append(paper->media.maxx-insetr,paper->media.maxy-.9*insett);
		}
		spread->marks=marks;
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
int Singles::PaperFromPage(int pagenumber)
{ return pagenumber; }

//! Just return pagenumber, since 1 page==1 paper
int Singles::SpreadFromPage(int pagenumber)
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

//! There is only one type of page, so return 0.
int Singles::PageType(int page)
{ return 0; }

//! There is only one type of spread, so return 0.
int Singles::SpreadType(int spread)
{ return 0; }


//-------------------------------- DoubleSidedSingles ------------------------------------------------------

/*! \class DoubleSidedSingles
 * \brief For 1 page per sheet, arranged so the pages are to be placed next to each other, first page is like the cover.
 *
 * Please note that Singles::inset* that DoubleSidedSingles inherits are not margins.
 * The insets refer to portions of the paper that would later be chopped off, and are the 
 * same for each page, whether the page is on the left or the right.
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
/*! \todo *** rethink styledef assignment..
 */
DoubleSidedSingles::DoubleSidedSingles()
{
	isvertical=0;
	isleft=0;
	pagestyler=NULL;
	setPage();
	
	 // make style instance name "Double Sided Singles"  perhaps remove the spaces??
	makestr(stylename,"Double Sided Singles");

	//**** this gets a little expensive, perhaps pass in a StyleDef in constructor?
	if (styledef) styledef->dec_count();//remove the one from Singles..
	styledef=stylemanager.FindDef("DoubleSidedSingles");
	if (styledef) styledef->inc_count(); 
	else {
		styledef=makeStyleDef();
		if (styledef) stylemanager.AddStyleDef(styledef);
	}
	
	DBG cerr <<"imposition doublesidedsingles init"<<endl;
} 

//! Calls pagestyler->dec_count().
DoubleSidedSingles::~DoubleSidedSingles()
{
	DBG cerr <<"--Double Sided Singles Singles destructor"<<endl;
	pagestyler->dec_count();
}

//! Using the paperstyle and isvertical, create new default pagestyles.
void DoubleSidedSingles::setPage()
{
	if (pagestyle) pagestyle->dec_count();
	pagestyle=new RectPageStyle((isvertical?(RECTPAGE_LRIO|RECTPAGE_LEFTPAGE):(RECTPAGE_IOTB|RECTPAGE_TOPPAGE)));
	pagestyle->pagetype=(isvertical?2:1);

	if (pagestyler) pagestyler->dec_count();
	pagestyler=new RectPageStyle((isvertical?(RECTPAGE_LRIO|RECTPAGE_RIGHTPAGE):(RECTPAGE_IOTB|RECTPAGE_BOTTOMPAGE)));
	pagestyler->pagetype=(isvertical?3:0);
				
	pagestyler->width= pagestyle->width =(paper->media.maxx-insetl-insetr)/tilex;
	pagestyler->height=pagestyle->height=(paper->media.maxy-insett-insetb)/tiley;
}

//! The newfunc for DoubleSidedSingles instances.
Style *NewDoubleSidedSingles(StyleDef *def)
{ 
	DoubleSidedSingles *s=new DoubleSidedSingles;
	s->styledef=def;
	return s;
}

//! Make an instance of the DoubleSidedSingles imposition styledef
/*  Required by Style, this defines the various names for fields relevant to DoubleSidedSingles.
 *  Basically just the inset[lrtb], plus the standard Imposition npages and npapers,
 *  and isvertical (which is a flag to say whether this is a booklet or a calendar).
 *
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in the style def manager *** whatever and wherever that is!!!
 */
StyleDef *DoubleSidedSingles::makeStyleDef()
{
	StyleDef *sd=new StyleDef("Singles","DoubleSidedSingles","Double Sided Singles",
			"Imposition of single pages meant to be next to each other",
			"Imposition of single pages meant to be next to each other",
			Element_Fields,NULL,NULL,
			NULL,0,NewDoubleSidedSingles);

	sd->push("isvertical",
			"Is Vertical",
			"Whether pages are arranged left and right, or top and bottom",
			"Whether pages are arranged left and right, or top and bottom",
			Element_Boolean,
			NULL,"0",
			0,0);
	sd->push("isleft",
			"First is on left",
			"Whether the first page is on the left/top, or right/bottom",
			NULL,
			Element_Boolean,
			NULL,"0",
			0,0);
	return sd;
}

//! Copies over isvertical and isleft.
Style *DoubleSidedSingles::duplicate(Style *s)//s=NULL
{
	DoubleSidedSingles *ds;
	if (s==NULL) s=ds=new DoubleSidedSingles();
	else {
		ds=dynamic_cast<DoubleSidedSingles *>(s);
	}
	if (!ds) return NULL;
	ds->isvertical=isvertical;
	ds->isleft=isleft;
	if (pagestyler) {
		if (ds->pagestyler) ds->pagestyler->dec_count();
		pagestyler->inc_count();
		ds->pagestyler=pagestyler;
	}
	return Singles::duplicate(s);  
}

//! Return the default page style for that page.
/*! Default is to pagestyle->inc_count() then return pagestyle (or pagestyler).
 * pagestyle or pagestyler is decided via a call to PageType(pagenum).
 */
PageStyle *DoubleSidedSingles::GetPageStyle(int pagenum,int local)
{
	if (!pagestyle || !pagestyler) setPage();
	if (!pagestyle || !pagestyler) return NULL;
	PageStyle *p;
	int c=PageType(pagenum);
	if (c==1 || c==2) p=pagestyle; else p=pagestyler;
	if (local) {
		PageStyle *ps=(PageStyle *)p->duplicate();
		ps->flags|=PAGESTYLE_AUTONOMOUS;
		return ps;
	}
	p->inc_count();
	return p;
}

/*! \todo *** must figure out best way to sync up pagestyles...
 */
void DoubleSidedSingles::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
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
			pagestyler->dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	Singles::dump_in_atts(att,flag,context);
}

/*! Write out isvertical, then Singles::dump_out.
 *
 * If what==-1, dump out a pseudocode mockup of the file format.
 */
void DoubleSidedSingles::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		Singles::dump_out(f,indent,-1,NULL);
		fprintf(f,"%sdefaultpagestyler #default right or bottom page style\n",spc);
		fprintf(f,"%s  #(same kinds of attributes as the defaultpagestyle)\n",spc);
		fprintf(f,"%sisvertical  #whether the fold is up/down or the default left/right\n",spc);
		fprintf(f,"%sisleft      #whether page 0 is to be displayed by itself or to the left of page 1\n",spc);
		fprintf(f,"%sistop       #like isleft, but for isvertical impositioning\n",spc);
		return;
	}
	if (isvertical) fprintf(f,"%sisvertical\n",spc);
	if (isleft) {
		if (isvertical) fprintf(f,"%sistop\n",spc);
		else fprintf(f,"%sisleft\n",spc);
	}
	if (pagestyler) {
		fprintf(f,"%sdefaultpagestyler\n",spc);
		pagestyler->dump_out(f,indent+2,0,context);
	}
	Singles::dump_out(f,indent,0,context);
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
/*! The path created here is one path for the page(s), and another for the line
 * between the pages. The bounds and outline of the spread are put in spread->path.
 * The first page has only itself (if !isleft).
 * The last page (numbered from 0) it also has only itself (if num is odd and isleft is set).
 * All other spreads have 2 pages.
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
	 // this however assumes pagestyle and pagestyler are same dims!
	newpath->maxx=(isvertical?1:2)*pagestyle->w();
	newpath->maxy=(isvertical?2:1)*pagestyle->h();
	
	spread->path=(SomeData *)newpath;

	if (left<0 || right>=numpages) {
		 // first and possibly last are just single pages, so just have single box
		if (left<0) newpath->appendRect(pagestyler->w(),0,pagestyler->w(),pagestyler->h());
		else newpath->appendRect(0,0,pagestyle->w(),pagestyle->h());
	} else {
		 // 2 lines, 1 for 2 pages, and line down the middle
		newpath->appendRect(0,0,(isvertical?1:2)*pagestyle->w(),(isvertical?2:1)*pagestyle->h());
		newpath->pushEmpty();
		if (isvertical) {
			newpath->append(0,pagestyle->h());
			newpath->append(pagestyle->w(),pagestyle->h());
		} else {
			newpath->append(pagestyle->w(),0);
			newpath->append(pagestyle->w(),pagestyle->h());
		}
		newpath->FindBBox();
	}

	 // setup spread->pagestack with the single pages.
	 // page width/height must map to proper area on page.
	 //*** maybe keep around a copy of the outline, then checkin in destructor rather
	 //than GetPageOutline here?
	SomeData *noutline=GetPageOutline(0,0); //this has 1 count for noutline ptr
	Group *g=NULL;

	 // left page
	if (left>=0) {
		g=new Group;  // 1 count
		g->push(noutline,0); // this checks it out again.. noutline->count 2
		g->FindBBox();
		spread->pagestack.push(new PageLocation(left,NULL,g)); // incs count of g (to 2)
		g->dec_count(); // remove extra tick
		g=NULL;
		if (isvertical) {
			g->origin(flatpoint(0,pagestyle->h()));
			spread->minimum=flatpoint(pagestyle->w()/2,pagestyle->h()*9/5);
		} else spread->minimum=flatpoint(pagestyle->w()/5,pagestyle->h()/2);
	} else {
		if (isvertical) spread->minimum=flatpoint(pagestyle->w()/2,pagestyle->h()*4/5);
			else spread->minimum=flatpoint(pagestyle->w()*6/5,pagestyle->h()/2);
	}

	 // right page
	if (right<numpages) {
		g=new Group();
		g->push(noutline,0); // this incs count on outline..
		if (!isvertical) g->m()[4]+=pagestyle->w();
		g->FindBBox();
		spread->pagestack.push(new PageLocation(right,NULL,g)); //incs count of g
		g->dec_count(); //remove extra tick
		if (isvertical) spread->maximum=flatpoint(pagestyle->w()/2,pagestyle->h()/5);
		else spread->maximum=flatpoint(pagestyle->w()*9/5,pagestyle->h()/2);
	} else {
		if (isvertical) spread->maximum=flatpoint(pagestyle->w()/2,pagestyle->h()*6/5);
		else spread->maximum=flatpoint(pagestyle->w()*4/5,pagestyle->h()/2);
	}
	
	noutline->dec_count(); // This removes the extra unnecessary tick, has count==1 or 2 now

	return spread;
}

//! Return a paper spread with 1 page on it
/*! The path created here is one path for the paper, and another for the possibly inset page.
 *
 * Returns Singles::PaperLayout(whichpaper). Remeber that the insets are not the same thing as
 * margins.
 */
Spread *DoubleSidedSingles::PaperLayout(int whichpaper)
{
	return Singles::PaperLayout(whichpaper);
}

//! Return (pagenumber+1-isleft)/2.
int DoubleSidedSingles::SpreadFromPage(int pagenumber)
{ return (pagenumber+1-isleft)/2; }

//! Return (npages-isleft)/2+1.
int DoubleSidedSingles::GetSpreadsNeeded(int npages)
{ return (npages-isleft)/2+1; } 


/*! 
 * <pre>
 *  0=right
 *  1=left
 *  2=top
 *  3=bottom
 * </pre>
 */
int DoubleSidedSingles::PageType(int page)
{ 
	int left=((page+1)/2)*2-1+isleft;
	if (left==page && isvertical) return 2;
	if (left==page) return 1;
	if (isvertical) return 3;
	return 0; 
}

/*! 
 * <pre>
 *  0=right only
 *  1=left only
 *  2=top only
 *  3=bottom only
 *  4=l + r
 *  5=t + b
 * </pre>
 */
int DoubleSidedSingles::SpreadType(int spread)
{
	int page=spread*2-isleft,left,right;
	if (page<0) page=0;
	left=((page+1)/2)*2-1+isleft,
	right=left+1;
	if (left==page) {
		if (right<NumPages()) {
			if (isvertical) return 5;
			else return 4;
		} else if (isvertical) return 2;
		else return 1;
	}
			
	if (left>=0) {
		if (isvertical) return 5;
		else return 4;
	}
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
 * DoubleSidedSingles::isleft is not used. That is to say, no code should cause isleft in
 * a Booklet instance to be anything but 0.
 *
 * Also, see note in DoubleSidedSingles about Singles::GetPageOutline().
 *
 * \todo *** tiling and cut marks are not functional yet
 * \todo *** creep is not implemented
 */


//! Constructor, init new variables, make style name="Booklet"
/*! \todo *** rethink styledef assignment..
 */
BookletImposition::BookletImposition()
{
	creep=0;
	covercolor=bodycolor=~0;

	makestr(stylename,"Booklet");
	isleft=0;//isleft must always be 0.

	setPage();
	
	 // set up the styledef
	//**** this gets a little expensive, perhaps pass in a StyleDef in constructor?
	if (styledef) styledef->dec_count();//remove the one from Singles..
	styledef=stylemanager.FindDef("Booklet");
	if (styledef) styledef->inc_count(); 
	else {
		styledef=makeStyleDef();
		if (styledef) stylemanager.AddStyleDef(styledef);
	}
	
	DBG cerr <<"imposition booklet init"<<endl;
}

//! Using the paperstyle and isvertical, create a new default pagestyle.
void BookletImposition::setPage()
{
	DoubleSidedSingles::setPage();
	if (isvertical) {
		pagestyle ->height/=2;
		pagestyler->height/=2;
	} else {
		pagestyle ->width/=2;
		pagestyler->width/=2;
	}
	
}

//! The newfunc for Booklet instances.
Style *NewBookletFunc(StyleDef *def)
{ 
	BookletImposition *s=new BookletImposition;
	s->styledef=def;
	return s;
}

//! Extend Singles to include isvertical, creep, covercolor, and bodycolor.
StyleDef *BookletImposition::makeStyleDef()
{
	StyleDef *sd=new StyleDef("Singles","Booklet","Booklet",
			"Imposition for a stack of sheets, folded down the middle",
			NULL,
			Element_Fields,NULL,NULL,
			NULL,0,NewBookletFunc);

	sd->push("isvertical",
			"Is Vertical",
			"Whether pages are arranged left and right, or top and bottom",
			NULL,
			Element_Boolean,
			NULL,"0",
			0,NULL);
	sd->push("creep",
			"Creep",
			"The difference in distance from the middle page edge to the outer "
			   "page edge when the booklet is all folded up. (currently ignored!)",
			NULL,
			Element_Real,
			NULL,"0",
			0,NULL);
	sd->push("covercolor",
			"Cover Color",
			"The color of paper you are using for the cover. This translates to paper number 0 and 1.",
			NULL,
			Element_Color,
			NULL,"#ffffffff",
			0,NULL);
	sd->push("bodycolor",
			"Body Color",
			"The color of paper you are using for the body pages.",
			NULL,
			Element_Color,
			NULL,"#ffffffff",
			0,NULL);
	return sd;
}

//! Duplicate. Copies over creep, bodycolor, covercolor.
Style *BookletImposition::duplicate(Style *s)//s=NULL
{
	BookletImposition *b;
	if (s==NULL) s=b=new BookletImposition();
	else {
		b=dynamic_cast<BookletImposition *>(s);
	}
	if (!b) return NULL;
	b->creep=creep;
	b->bodycolor=bodycolor;
	b->covercolor=covercolor;
	return DoubleSidedSingles::duplicate(s);  
}

/*! Layout booklet with tiling.
 *
 * \todo *** use isvertical and creep
 */
Spread *BookletImposition::PaperLayout(int whichpaper)
{
	if (!numpapers) return NULL;
	if (whichpaper<0 || whichpaper>=numpapers) whichpaper=0;

	 // grab singles, which draws a tiled page using the inset values,
	 // including printer marks, and max and min points.
	 // but the pagestack is incorrect. All but pagestack is ok. It
	 // gets corrected below.
	Spread *spread=Singles::PaperLayout(whichpaper);

	 // fill pagestack, includes tiling
	spread->pagestack.flush();
	int x,y,left,right,npgs=GetPagesNeeded(numpapers);
	double dx,dy;
	PathsData *ntrans;
	for (x=0; x<tilex; x++) {
		for (y=0; y<tiley; y++) {
			dx=insetl+x*(paper->media.maxx-insetr-insetl)/tilex;
			dy=insetb+y*(paper->media.maxy-insett-insetb)/tiley;
			
			 //install 2 page cells for each tile, according to isvertical
			 //the left or top:
			if (whichpaper%2==1) { // odd numbered pages are always on left/top...
				 // make right and left side be correct page number
				left=whichpaper; 
				right=npgs-whichpaper-1; 
			} else {
				 // make right and left side be correct page number
				left=npgs-whichpaper-1;
				right=whichpaper;
			}
			
			if (left>=0 && left<numpages) {
				ntrans=new PathsData();
				ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
				ntrans->origin(flatpoint(dx,dy+(isvertical?pagestyle->h():0)));
				spread->pagestack.push(new PageLocation(left,NULL,ntrans));
			}
			 //the right or bottom:
			if (right>=0 && right<numpages) {
				ntrans=new PathsData();
				ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
				ntrans->origin(flatpoint(isvertical?0:pagestyle->w()+dx, dy));
				spread->pagestack.push(new PageLocation(right,NULL,ntrans));
			}
		}
	}

	return spread;
}

//! If pagenumber<=numpapers return pagenumber, else return numpapers-(pagenumber-numpapers)-1.
int BookletImposition::PaperFromPage(int pagenumber)
{
	if (pagenumber+1<=numpapers) return pagenumber;
	return numpapers-(pagenumber-numpapers)-1;
}

//! Return (pagenumber+1)/2.
int BookletImposition::SpreadFromPage(int pagenumber)
{ return (pagenumber+1)/2; }

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

/*! Something like:
 *  <pre>
 *    creep .05
 *    bodycolor 0xffffffff
 *    colorcolor 0xffffffff
 *    isvertical
 *    defaultpagestyler
 *    ...Singles stuff..
 *  </pre>
 *
 *  Note that DoubleSidedSingles::dump_out() is not called, so as to avoid outputting isleft.
 *  But Singles::dump_out() is called.
 *
 * If what==-1, dump out a pseudocode mockup of the file format.
 */
void BookletImposition::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		Singles::dump_out(f,indent,-1,NULL);
		fprintf(f,"%sdefaultpagestyler #default right or bottom page style\n",spc);
		fprintf(f,"%s  #(same kinds of attributes as the defaultpagestyle)\n",spc);
		fprintf(f,"%sisvertical  #whether the pages fold up/down or the default left/right\n",spc);
		fprintf(f,"%screep  0    #(todo) how much creep is introduced by the fold when all pages are folded over\n",spc);
		fprintf(f,"%sbodycolor  0xffffff  #(todo)8bit rgb color of the pages\n",spc);
		fprintf(f,"%scovercolor 0xffffff  #(todo)8bit rgb color of the pages\n",spc);
		return;
	}
	fprintf(f,"%screep %.10g\n",spc,creep);
	fprintf(f,"%sbodycolor 0x%.6lx\n",spc,bodycolor);
	fprintf(f,"%scovercolor 0x%.6lx\n",spc,covercolor);
	if (isvertical) fprintf(f,"%sisvertical\n",spc);
	if (pagestyler) {
		fprintf(f,"%sdefaultpagestyler\n",spc);
		pagestyler->dump_out(f,indent+2,0,context);
	}
	Singles::dump_out(f,indent,0,context);
}

//! Read creep, body and covercolor, then DoubleSidedSigles::dump_in_atts().
void BookletImposition::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
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
	DoubleSidedSingles::dump_in_atts(att,flag,context);
	isleft=0;
}


////---------------------------------- CompositeImposition ----------------------------
//
// // class to enable certain page ranges to be administred by different impositions...
//class CompositeImposition : public Imposition
//{
// protected:
//	Imposition *impos;
//	Ranges ranges; 
//	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
//	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
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
//	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
//	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
////};
//
//BasicBook::BasicBook() : Style(NULL,NULL,"Basic Book")
//{ }
//
////! *** imp me!
//void BasicBook::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
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
//	DoubleSidedSingles::dump_in_atts(att,flag,context);
//}
//
////! Write out flags, width, height
///*!
// * If what==-1, dump out a pseudocode mockup of the file format.
// */
//void BasicBook::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
//{
//	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
//	if (what==-1) {
//		***
//	}
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
