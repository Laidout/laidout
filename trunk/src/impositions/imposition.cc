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


#include "../language.h"
#include "imposition.h"
#include "utils.h"

#include <lax/misc.h>
#include <lax/lists.cc>
#include <lax/interfaces/pathinterface.h>
#include <lax/transformmath.h>

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



namespace Laidout {



//------------------------- PageLocation --------------------------------------

/*! \class PageLocation
 * \brief Imposition classes would derive PageLocation classes from this.
 *
 *  This is a shell around a page, providing a convenience for placement on
 *  the screen and also on a paper. Provides an initial transformation from
 *  paper coordinates to page coordinates in the outline member, which also
 *  should hold the bounding box of the page in paper or view coordinates, and
 *  the path of the page itself, which gets used to check for points being inside
 *  the page. index is the index of the page in the relevant Document->pages stack.
 *  
 *  As for other page related metrics, the page spread outline is kept in a
 *  Spread. The thumbnail (if available), margin path, and page number are
 *  all kept by Page.
 *
 *  The page is assumed to be nonlocal, but the outline can be local, and
 *  thus can be delete'd in the PageLocation destructor. If not local, then dec_count() is
 *  called on it.
 */
/*! \var int PageLocation::index
 * \brief the page index (starting at 0) of the page in the relevant page stack.
 */
/*! \var SomeData *PageLocation::outline
 * \brief Holds the transform to get to the page, and also the outline of the page.
 *
 * It is typically a PathsData, but the actual form of it is up to the Imposition.
 * After applying outline->m(), coordinates are assumed to be in paper coordinates.
 */
/*! \var int PageLocation::info
 * \brief Used by SpreadEditor to keep track of pages that are temporarily away from main page list.
 */


//! Constructor, just copies over pointers.
/*! The page is assumed to not be owned locally. The count of poutline will be incremented here,
 * and decremented in the destructor.
 */
PageLocation::PageLocation(int ni,Page *npage,LaxInterfaces::SomeData *poutline)
{
	info=0;
	index=ni;
	page=npage;
	outline=poutline;
	if (outline) poutline->inc_count();
}

//! Page not delete'd, see constructor for how outline is dealt with.
PageLocation::~PageLocation()
{
	if (outline) outline->dec_count();
}

//----------------------- Spread -------------------------------
/*! \class Spread
 * \brief A generic container for the various things that Imposition returns.
 *
 * This class gets used by other classes, and those other classes are 
 * responsible for maintaining Spread's components.
 *
 * A Spread can contain a relevant PaperGroup, a drawing of the spread (path),
 * additional printing marks that are not associated with specific pages,
 * locations of what might be considered minimum and maximum points, for use
 * in a SpreadEditor for connecting spreads. Also contains a stack of actual
 * pages in the spread, which in turn contain page outlines.
 *
 * The type of thing the spread represents is held in mask, which are or'd
 * values from the following defines.
 * \code
 *  // these go in style, they say what the various members represent
 *  // The whole paper, with page outlines and pagestack also
 * #define SPREAD_PAPER            (1<<0)
 *  // Only the paper outline
 * #define SPREAD_PAPER_OUTLINE    (1<<1)
 *  
 *  // A whole page spread
 * #define SPREAD_PAGE             (1<<2)
 *  // Only a single page outline
 * #define SPREAD_PAGE_OUTLINE     (1<<3)
 *  // Only an outline made by "margins", whatever those are.
 * #define SPREAD_MARGIN_OUTLINE   (1<<4)
 * 
 *  // A small page spread for use in the spread editor
 * #define SPREAD_LITTLESPREAD     (1<<5)
 *  // The printer marks as seen in a paperview
 * #define SPREAD_PRINTERMARKS     (1<<6)
 * 
 *
 *  // these go in mask, they are which members have meaningful values for 
 *  // the type of thing indicated in style
 * #define SPREAD_PATH             (1<<7)
 * #define SPREAD_MINIMUM          (1<<8)
 * #define SPREAD_MAXIMUM          (1<<9)
 * #define SPREAD_PAGES            (1<<10)
 * \endcode
 */
/*! \var flatpoint Spread::minimum
 * This is useful in the spread editor. It is the coordinate on the spread in
 * spread coordinates corresponding to the page with the lowest
 * page number. In the editor, each spread points to the next spread, and
 * this is the point that gets pointed to.
 */
/*! \var flatpoint Spread::maximum
 * This is useful in the spread editor. It is the coordinate 
 * on the spread in spread coordinates corresponding
 * to the page with the highest page number. In the editor, 
 * each spread points to the next spread, and
 * this is the point from which the arrow to the next spread starts.
 */
/*! \var LaxInterfaces::SomeData *Spread::path
 * This is the outline of whatever is relevant. The Spread class is used as a return type
 * for many functions in Imposition, so the actual meaning of path depends 
 * on which Imposition function was called. Could be a single page, page spread,
 * or paper spread, etc.
 *
 * In any case, coordinates after path->m() applied are paper coordinates.
 */
/*! \var int Spread::spreadtype
 * \brief What is the shape of this spread.
 *
 * This value is used by SpreadEditors to only generate a new spread if the current
 * one is the wrong shape.
 */
/*! \var PaperGroup *Spread::papergroup
 * \brief The default paper group to use with the spread.
 */


//! Basic init, set all to 0.
Spread::Spread()
{
	papergroup=NULL;
	mask=0;
	spreadtype=0;
	style=0;
	path=marks=NULL;
	obj_flags|=OBJ_Unselectable|OBJ_Zone;
}

//! Dec count of path and marks.
Spread::~Spread()
{
	if (papergroup) papergroup->dec_count();
	if (path) path->dec_count();
	if (marks) marks->dec_count();
	pagestack.flush();
}

int Spread::n()
{ return 2; }

Laxkit::anObject *Spread::object_e(int i)
{
	if (i==0) return &pagestack;
	if (i==1) return marks;
	return NULL;
}

const char *Spread::object_e_name(int i)
{
	if (i==0) return "pagestack";
	if (i==1) return "marks";
	return NULL;
}

const double *Spread::object_transform(int i)
{
	if (i==0) return NULL;
	if (i==1 && marks) return marks->m();
	return NULL;
}


//! Return a new char[] string like "2-4,6-8" corresponding to what pages are in the spread.
/*! This retrieves the Page::label from the pages in doc.
 * The returned string is used, for instance, when printing to postscript as the
 * paper label.
 *
 * This assumes that doc exists. In the future it could be made to try to use whatever
 * page might be in the pagestack if doc==NULL. That would help adapt to Whatever/limbo
 * spreads.
 */
char *Spread::pagesFromSpreadDesc(Document *doc)
{
	int *pages=pagesFromSpread();
	int c=0;
	char *desc=NULL,*label;
	while (pages[c]!=-2) {
		 //single page or first page of range:
		if (pages[c]>=0 && pages[c]<doc->pages.n) label=doc->pages.e[pages[c]]->label; 
			else label=NULL;
		appendstr(desc,(label?label:"?"));
				
		 // page range
		if (pages[c+1]>=0) {
			if (pages[c+1]>=0 && pages[c+1]<doc->pages.n) label=doc->pages.e[pages[c+1]]->label; 
				else label=NULL;
			appendstr(desc,"-");
			appendstr(desc,(label?label:"?"));
		}
		if (pages[c+2]!=-2) appendstr(desc,",");
		c+=2;
	}
	delete[] pages;
	return desc;
}

//! Find what pages are in the spread.
/*! Returns an int[] such that a range of pages is indicated by 2 consecutive nonnegative
 * numbers, single pages are indicated by the page number followed by a -1. The whole array
 * is terminated with a -2.
 */
int *Spread::pagesFromSpread()
{
	int c,c2,i;
	NumStack<int> list,list2;
	for (c=0; c<pagestack.n(); c++) {
		i=pagestack.e[c]->index;
		if (!list.n) list.push(i);
		else {
			for (c2=0; c2<list.n; c2++) { // pushnodup, sorting in process
				if (i<list.e[c2]) { list.push(i,c2); break; }
				if (i==list.e[c2]) break;
			}
			if (c2==list.n) { list.push(i,c2); }
		}
	}
	DBG cerr <<"pagesfromSpread list: ";
	DBG for (c=0; c<list.n; c++) cerr <<list.e[c]<<' '; 
	DBG cerr <<endl;
	
	 //now list holds a monotonically increasing list of pages. 
	 //now crunch down ranges..
	for (c=0; c<list.n; c++) {
		i=list.e[c];
		list2.push(i);
		for (c2=c+1; c2!=list.n && list.e[c2]==i+1; c2++) { i=list.e[c2]; }
		if (c2==c+1) list2.push(-1); // was single page
		else list2.push(list.e[c2-1]); //range
		c=c2-1;
	}
	list2.push(-2);

	DBG cerr <<"pagesfromSpread list2: ";
	DBG for (c=0; c<list2.n; c++) cerr <<list2.e[c]<<' ';
	DBG cerr <<endl;

	return list2.extractArray();
}

//! Return the index in pagestack of the page corresponding to the given docpage index.
/*! If page not found, then return -1.
 */
int Spread::PagestackIndex(int docpage)
{
	for (int c=0; c<pagestack.n(); c++) {
		if (pagestack.e[c]->index==docpage) return c;
	}
	return -1;
}

//----------------------------- Imposition --------------------------

/*! \class Imposition
 * \brief The abstract base class for imposition styles.
 *
 * The Imposition class is meant to describe a single style of chopping
 * and folding of same sized pieces of paper. The individual papers can
 * be of different colors (***see todos), but must be the same size. Something like a book where
 * the cover would be printed on a piece of paper with a different size then the
 * body papers would not be all done in a single Imposition. That would be a ***??
 * with 2 Imposition classes used (???).
 * 
 * Currently the built in Imposition styles are single pages, singles meant as double sided,
 * such as would be stapled in the corner or along the edge, the slightly more versatile booklet
 * format where the papers are folded at the spine,
 *  \todo the super-duper BasicBookImposition, comprised of multiple sections, each of which have 
 *    one fold down the middle. Section there means basically the same
 *    as "signature". Also, the basic book imposition works together with a
 *    BookCover imposition, which is basically 4 pages, and the spine.
 *  \todo and the WhatupImposition, than allows n-up printing of any other Imposition.
 *
 * \todo needs to be a standard to be duplex aware..
 * \todo ***finishing imping built in impositions
 *
 * The imposition's name is stored in stylename inherited from Style.
 *
 * NOTE that numpages and numpapers SHOULD be consistent with the actual number
 * of pages in a document, but the document can add or remove pages whenever it wants, so
 * care must be taken, especially in Document, to maintain sanity
 * This is ugly!! maybe have imposition point to a doc??
 *
 * \todo *** need to think about printer marks and paper colors. they are very haphazard, and 
 *   built in to the PaperLayout() functions currently. need some standard to allow for
 *   different papers previewing differently to each other, and also to have different printer
 *   marks, but still be meaningful after a paper/pagesize changes.
 */
/*! \var int Imposition::numpapers
 * \brief The number of papers available.
 */
/*! \var int Imposition::numpages
 * \brief The number of pages available.
 */
/*! \var PaperGroup *Imposition::papergroup
 * \brief The group of papers to print spreads on.
 *
 * By convention, this will be applied to the PAPERLAYOUT spread type, but really that is
 * up to the imposition instance. When printing and displaying, this papergroup might be
 * overridden, for instance when a user wants to tile for posters, but the same spread will be used.
 */
/*! \var PaperBox *Imposition::paper
 * \brief The base type of paper to print on.
 *
 * This is a convenience pointer to the first box of papergroup (if any), or to a generic
 * paper to base impositions on. It usually points to a component of papergroup.
 */
/*! \fn LaxInterfaces::SomeData *Imposition::GetPrinterMarks(int papernum=-1)
 * \brief Return the printer marks for paper papernum in paper coordinates.
 *
 * Default is to return NULL.
 * This is usually a group of SomeData.
 */
/*! \fn Page **Imposition::CreatePages(int npages)
 * \brief Create the required pages. If npages>0, then assume the document wants this many pages.
 *
 * Derived class must define this function.
 */
/*! \fn SomeData *Imposition::GetPageOutline(int pagenum,int local)
 * \brief Return outline of page in page coords. Origin is page origin.
 *
 * This returns a no frills outline, used primarily to check where the mouse
 * is clicked down on. If local==1 then return a new local SomeData. Otherwise 
 * a counted object. In this case, the item's count will have 1 added that refers to
 * the returned pointer.
 * 
 * Derived class must define this function.
 */
/*! \fn Spread *Imposition::GetLittleSpread(int whichspread)
 * \brief Returns a page spread with outlines of pages in page view, in viewer coords,
 * 
 * Mainly for use in the spread editor, so it would have little folded corners
 * and such, perhaps a thumbnail also (maybe?).
 *
 * This spread should correspond to the PageLayout for the same page. Particularly,
 * it should be scaled the same. It might be
 * augmented to contain little dogeared conrners, for instance. Also, it really
 * should contain a continuous range of pages. Might trip up the spread editor
 * otherwise.
 *
 * The default is to return PageLayout(whichspread).
 *
 * \todo *** this needs to modified so that -1 returns NULL, -2 returns a generic
 * page to be used in the SpreadEditor, and -3 returns a generic page layout spread
 * for the SpreadEditor. Those are used as temp page holders when pages are pushed
 * into limbo in the editor. either that or need to have standardized way to know
 * how many different kinds of spreads an imposition can make... more thought required
 * about this.
 */
/*! \fn Spread *Imposition::PageLayout(int whichspread)
 * \brief Returns the whichspread'th page view spread, in viewer coords.
 *
 * whichspread starts at 0.
 * Derived classes must fill the spread with a path, and the PageLocation stack.
 * The path holds the outline of the spread, and the PageLocation stack holds
 * transforms to get from the overall coords to each page's coords.
 *
 * Derived class must define this function.
 */
/*! \fn Spread *Imposition::PaperLayout(int whichpaper)
 * \brief Returns a paper view spread that contains whichpaper, in paper coords.
 *
 * whichpaper starts at 0.
 * Derived class must define this function.
 */
/*! \fn int *Imposition::PrintingPapers(int frompage,int topage)
 * \brief Return a specially formatted list of papers needed to print the range of pages.
 *
 * It is a -2 terminated int[] of papers needed to print [frompage,topage].
 * A range of papers is specified using 2 consecutive numbers. Single papers are
 * indiciated by a single number followed by -1. For example, a sequence { 1,5, 7,-1,10,-1,-2}  
 * means papers from 1 to 5 (inclusive), plus papers 7 and 10.
 */
/*! \fn int Imposition::GetPagesNeeded(int npapers)
 * \brief Return the number of pages required to fill npapers number of papers.
 */
/*! \fn int Imposition::GetPapersNeeded(int npages)
 * \brief Return the number of papers required to hold npages of pages.
 */
/*! \fn int Imposition::GetSpreadsNeeded(int npages)
 * \brief Return the number of spreads required to hold npages of pages.
 */
/*! \fn int Imposition::PaperFromPage(int pagenumber)
 * \brief Return the (first) paper index number that contains page index pagenumber in doc->pages.
 */
/*! \fn int Imposition::SpreadFromPage(int pagenumber)
 * \brief Return the (first) spread index number that contains page index pagenumber in doc->pages.
 */
/*! \fn int Imposition::SpreadFromPage(int layout,int pagenumber)
 * \brief Return the (first) spread index number of type layout that contains page index pagenumber in doc->pages.
 */
/*! \fn int Imposition::NumPapers()
 * \brief Return the the number of papers the imposition thinks there are.
 */
/*! \fn int Imposition::NumPages()
 * \brief Return the the number of pages the imposition thinks there are.
 */
/*! \fn int Imposition::NumSpreads()
 * \brief Return the the number of page spreads the imposition thinks there are.
 *
 * This should be the main reader spreads, not printer spreads, and not any other strange
 * assortment of spread.
 *
 * Usually, this will be such that each spread contains a continuous range of document pages,
 * that continues the index count from the previous spread. For instance, in booklets,
 * the first spread would have document page 0, the next spread has doc pages 1 and 2, 
 * the next has 3 and 4, and so on. This is very different from paper based spreads or other
 * custom net spreads that may have any unordered selection of document pages.
 */
/*! \fn int Imposition::NumPageTypes()
 * \brief Return the number of types of pages.
 *
 * For instance, booklets have 2 types. Horizontal booklets have Left and right,
 * vertical have top and bottom.
 */
/*! \fn const char *Imposition::PageTypeName(int pagetype)
 * \brief Return a name for the pagetype (as returned by PageType().
 */
/*! \fn int Imposition::PageType(int page)
 * \brief Return the type of page this page index requires.
 *
 * This value makes sense only to the type of imposition, but will be a number in
 * the range [0..NumPageTypes()-1]. It is used to ensure that
 * pages have the proper PageStyle, like left page versus right page.
 *
 * page==-1 means return the number of different page types.
 */
/*! \fn int Imposition::SpreadType(int spread)
 * \brief Return the type of spread this spread index requires.
 *
 * This value makes sense only to the type of imposition. It is used to ensure that
 * spreads in a SpreadEditor have the proper shape, like single page versus 
 * left and right page.
 *
 * page==-1 means return the number of different page spread types.
 */
/*! \fn PageStyle *Imposition::GetPageStyle(int pagenum,int local)
 * \brief Return the default page style for that page.
 *
 * This function should only return NULL for page out of bounds.
 * 
 * The calling code need not increment the count of the returned style.
 * It is incremented here. If local==1 then create a PageStyle that is
 * a duplicate (with a count of 1) of the default page style of that page.
 * 
 * Derived class must define this function.
 *
 * This function is used internally by impositions to set up pages.
 */
/*! \fn const char *Imposition::BriefDescription()
 * \brief One line description, for use in a new document dialog.
 *
 * For signatures, this would be something like "2 fold, 8 page signature"
 * This exists to try to avoid unnecessarily large dialogs and still have it easy
 * to navigate to a particular imposition you want.
 */
/*! \fn void Imposition::GetDimensions(int paperorpage, double *x, double *y)
 * \brief Return default paper dimensions if paperorpage==0, or page dimensions for paperorpage==1.
 */



//! Constructor.
/*! Default Style constructor sets styledef, basedon to NULL. Any impositions that are 
 *  explicitly based on another imposition must set up the proper styledef and basedon
 *  themselves. The standard built in impositions all act autonomously, meaning they each
 *  completely define their own StyleDef, and are not based on another Imposition.
 *
 *  Otherwise, Imposition subclasses must establish paperstyle, and usually also their
 *  own pagestyles based on the paperstyle. It is assumed that the numpages, numpapers,
 *  are set soon after creation by the code that creates the instance
 *  in the first place.
 */
Imposition::Imposition(const char *nsname)
	: Style (NULL,NULL,nsname)
{ 
	doc=NULL;
	paper=NULL; 
	papergroup=NULL;
	numpages=numpapers=0; 
	
	DBG cerr <<"imposition base class init for object "<<object_id<<endl;
}

/*! Does paperstyle->dec_count().
 * \todo figure out how doc should be handled, and whether it belongs in this class.
 */
Imposition::~Imposition()
{
	if (paper) paper->dec_count();
	if (papergroup) papergroup->dec_count();

	DBG cerr <<"imposition base class destructor for object "<<object_id<<endl;
}

//! Return an imposition specific tool for use with the given layout type
/*! Default is to return NULL.
 */
LaxInterfaces::anInterface *Imposition::Interface(int layouttype)
{
	return NULL;
}


//! Return a box describing a good scratchboard size for this imposition.
/*! Default is to return bounds 3 times the paper size wide, and twice the height.
 *
 * Place results in bbox if bbox!=NULL. If bbox==NULL, then create a new DoubleBBox and return that.
 */
Laxkit::DoubleBBox *Imposition::GoodWorkspaceSize(Laxkit::DoubleBBox *bbox)
{
	if (!bbox) bbox=new DoubleBBox();
	else bbox->clear();

	if (papergroup) {
		for (int c=0; c<papergroup->papers.n; c++) {
			bbox->addtobounds(papergroup->papers.e[c]);
		}
	} else if (paper) {
		bbox->setbounds(&paper->media);
	} else {
		bbox->setbounds(0,1,0,1);
	}

	 //add a bit of a buffer
	double w=bbox->maxx-bbox->minx;
	double h=bbox->maxy-bbox->miny;
	bbox->minx-=w;
	bbox->maxx+=w;
	bbox->miny-=h/2;
	bbox->maxy+=h/2;

	return bbox;
}

//! Just makes sure that s can be cast to Imposition. If yes, return s, if no, return NULL.
/*! Note that this does not duplicate the paperstyle. The builtin
 * impositions create their own copy of the default papersize in their
 * constructors.
 */
Style *Imposition::duplicate(Style *s)//s=NULL
{
	if (s==NULL) return NULL; // Imposition is abstract
	Imposition *d=dynamic_cast<Imposition *>(s);
	if (!d) return NULL;
	return s;
}

//! Ensure that each page has a proper pagestyle and bleed information.
/*! This is called when pages are added or removed. It replaces the pagestyle for
 *  each page with the pagestyle returned by GetPageStyle(c,0).
 *
 *  When the pagestyle is a custom style, then the PageStyle::flags are preserved,
 *  and applied to a new duplicate of the actual default pagestyle.
 *  This preserves whether page clips and facing pages bleed.
 *
 * \todo *** for more complicated pagestyles, this will forget any
 *   custom changes it may have had, which may or may not be good. ultimately
 *   for arbitrary foldouts, this might be important. There is the PageStyle::pagetype
 *   element that can be used to preserve the basic kind of thing....
 */
int Imposition::SyncPageStyles(Document *doc,int start,int n)
{
	if (numpages==0 || numpapers==0) {
		NumPages(doc->pages.n);
	}
	if (n==0) return 0;
	if (n<0) n=doc->pages.n;
	unsigned int oldflags=0;
	PageStyle *temppagestyle;
	for (int c=start; c<doc->pages.n && c<start+n; c++) {
		temppagestyle=GetPageStyle(c,0); //the default style with increased count

		if (doc->pages.e[c]->pagestyle!=temppagestyle) {
			if (doc->pages.e[c]->pagestyle) {
				oldflags=doc->pages.e[c]->pagestyle->flags;
				if (oldflags!=temppagestyle->flags) {
					PageStyle *ttt=temppagestyle;
					temppagestyle=static_cast<PageStyle *>(temppagestyle->duplicate());
					ttt->dec_count();
					temppagestyle->flags=oldflags;
				}
			}
		}
		doc->pages.e[c]->InstallPageStyle(temppagestyle);//adds 1 count
		temppagestyle->dec_count();
	}
	return 0;
}

//! This incs count of ngroup and sets papergroup to it, dec_counting the old group if any.
/*! 
 * Derived classes are responsible for setting PageStyle objects to appropriate
 * values in response to the new papersize.
 *
 * Return 0 success, nonzero error.
 */
int Imposition::SetPaperGroup(PaperGroup *ngroup)
{
	if (!ngroup) return 1;
	if (papergroup) papergroup->dec_count();
	papergroup=ngroup;
	if (papergroup) papergroup->inc_count();
	if (papergroup->papers.n) {
		if (paper) paper->dec_count();
		paper=papergroup->papers.e[0]->box;
		paper->inc_count();
	}
	return 0;
}

//! Default is to duplicate npaper and base a brand new papergroup on the duplicate.
/*! This is a convenience function to simply alter a paper size an imposition
 * goes by. The newly created paper group will work from a copy of npaper,
 * not from a link to it.
 *
 * Derived classes are responsible for setting PageStyle objects to appropriate
 * values in response to the new paper size.
 *
 * Return 0 success, nonzero error.
 */
int Imposition::SetPaperSize(PaperStyle *npaper)
{
	if (!npaper) return 1;

	PaperStyle *newpaper=(PaperStyle *)npaper->duplicate();
	if (paper) paper->dec_count();
	paper=new PaperBox(newpaper);
	newpaper->dec_count();
	PaperBoxData *newboxdata=new PaperBoxData(paper);

	if (papergroup) papergroup->dec_count();
	papergroup=new PaperGroup;
	papergroup->papers.push(newboxdata);
	papergroup->OutlineColor(65535,0,0); //default to red papergroup
	newboxdata->dec_count();

	return 0;
}

//! Return the number of spreads of type layout.
/*! Please note the number returned for LITTLESPREADLAYOUT and PAGELAYOUT is just NumPages(), the same as for SINGLELAYOUT.
 *
 * \todo Ultimately, this will replace the other NumPapers(), NumPages(), etc.
 *    it is much more adaptible for nets right now, just relays based on 
 *    PAGELAYOUT, PAPERLAYOUT, SINGLESLAYOUT
 */
int Imposition::NumSpreads(int layout)
{
	if (layout==PAPERLAYOUT) return NumPapers();
	if (layout==PAGELAYOUT) return NumPages();
	if (layout==SINGLELAYOUT) return NumPages();
	if (layout==LITTLESPREADLAYOUT) return NumPages();
	return 0;
}

//! Set the number of papers to npapers, and set numpages as appropriate.
/*! Default is to set numpapers=npapers, 
 *  numpages=GetPagesNeeded(numpapers), and 
 *  
 * Does not check to make sure npapers is a valid number for any document in question.
 * 
 * Returns the new value of numpapers.
 */
int Imposition::NumPapers(int npapers)
{
	numpapers=npapers;
	numpages=GetPagesNeeded(numpapers);
	return numpapers;
}

//! Set the number of pages to npage, and set numpapers as appropriate.
/*! Default is to set numpagess=npages, 
 * numpapers=GetPapersNeeded(numpapers), and 
 * 
 * Does not check to make sure npages is a valid number.
 *
 * Returns the new value of numpages.
 */
int Imposition::NumPages(int npages)
{
	numpages=npages;
	numpapers=GetPapersNeeded(numpages);
	return numpages;
}

//! Return the which'th spread of type layout.
/*! \todo Ultimately, this will replace the other PaperLayout(), PageLayout(), etc.
 *    it is much more adaptible for nets right now, just relays based on 
 *    PAGELAYOUT, PAPERLAYOUT, SINGLESLAYOUT, LITTLESPREADLAYOUT
 */
Spread *Imposition::Layout(int layout,int which)
{
	if (layout==PAPERLAYOUT) return PaperLayout(which);
	if (layout==PAGELAYOUT) return PageLayout(which);
	if (layout==SINGLELAYOUT) return SingleLayout(which);
	if (layout==LITTLESPREADLAYOUT) return GetLittleSpread(which);
	return NULL;
}

//! Return the number of different kinds of layouts the imposition can provide.
/*! The default is to return 3: singles, pages, and papers.
 *
 * You can find the names for the layouts by calling LayoutName(int) and passing
 * numbers in the range [0..NumLayouts()-1].
 */
int Imposition::NumLayoutTypes()
{
	return 3;
}

//! Default is to return NULL.
/*! When a page has its own margins, outside of which objects usually shouldn't go,
 * this function is used to get that shape. The origin of the returned path 
 * is the origin of the page, not necessarily a corner of the margin outline.
 */
SomeData *Imposition::GetPageMarginOutline(int pagenum,int local)
{ return NULL; }

//! Return the name of layout, or NULL if layout not recognized.
/*! Default is to return "Papers" for 2, "Pages" for 1, or "Singles" for 0.
 * Note that layout is a straight index, not an arbitrary id number. It should be
 * in the range [0..NumLayouts()-1].
 *
 * Note that it returns const char[], not a new char[].
 */
const char *Imposition::LayoutName(int layout)
{
	if (layout==PAPERLAYOUT) return _("Papers");
	if (layout==PAGELAYOUT) return _("Pages");
	if (layout==SINGLELAYOUT) return _("Singles");
	if (layout==LITTLESPREADLAYOUT) return _("Little Spreads");
	return NULL;
}

//! Return a spread corresponding to the single whichpage.
/*! The path created here is one path for the page, which is found with 
 * a call to GetPageOutline(whichpage,0).
 * The bounds of the page are put in spread->path. Usually, exotic impositions 
 * with strange page shapes need to only redefine GetPageOutline() for this 
 * function to work right.
 *
 * The spread->pagestack elements holds the outline, and the page index.
 * The spread->path is the same object as spread->pagestack.e[0]->outline.
 */
Spread *Imposition::SingleLayout(int whichpage)
{
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	 // Get the page outline. It will be a counted object with 1 count for path pointer.
	spread->path=GetPageOutline(whichpage,0);
	spread->path->setIdentity(); // clear any transform
	
	 // define maximum/minimum points 
	double x,y,w,h;
	x=spread->path->minx;
	y=spread->path->miny;
	w=spread->path->maxx-spread->path->minx;
	h=spread->path->maxy-spread->path->miny;
	spread->minimum=flatpoint(x+w/4,   y+h/2);
	spread->maximum=flatpoint(x+w*3/4, y+h/2);

	 // setup spread->pagestack with the single page.
	 // page width/height must map to proper area on page.
	spread->pagestack.push(new PageLocation(whichpage,NULL,spread->path)); //(index,page,somedata)
	//at this point spread->path has additional count of 2, 1 for pagestack.e[0]->outline
	//and 1 for spread->path

	return spread;
}

//---------------------------------- ImpositionResource ----------------------------------

/*! \class ImpositionResource
 * \brief Info about how to create new Imposition instances.
 */
/*! \var char *ImpositionResource::styledef
 * \brief StyleDef name for the type of Imposition this is.
 */


/*! If local!=0, then delete the Attribute in the destructor.
 */
ImpositionResource::ImpositionResource(const char *sdef,const char *nname, const char *file, const char *desc,
					   				   LaxFiles::Attribute *conf,int local)
{
	styledef=newstr(sdef);
	name=newstr(nname);
	impositionfile=newstr(file);
	description=newstr(desc);
	config=conf;
	configislocal=local;
}

ImpositionResource::~ImpositionResource()
{
	if (styledef) delete[] styledef;
	if (name) delete[] name;
	if (impositionfile) delete[] impositionfile;
	if (description) delete[] description;
	if (configislocal && config) delete config;
}

//! Create a new imposition instance based on this resource.
/*! If the resource has non-null impositionfile, then that file is read in,
 * and the proper type of Imposition is created.
 * Otherwise, if the resource has styledef, then that type of Imposition is created.
 *
 * Then, if there is a config, then those attributes are applied.
 *
 * \todo should probably have an error return.
 */
Imposition *ImpositionResource::Create()
{
	Imposition *imp=NULL;
	if (impositionfile) {
		FILE *f=open_laidout_file_to_read(impositionfile,"Imposition",NULL);
		if (!f) return NULL; //file was bad
	
		LaxFiles::Attribute att;
		att.dump_in(f,0,NULL);
		if (att.attributes.n!=0 && !strcmp(att.attributes.e[0]->name,"type")) {
			imp=newImpositionByType(att.attributes.e[0]->value);
			if (imp) imp->dump_in_atts(&att,0,NULL);
		}

		fclose(f);

	} else if (styledef) {
		imp=newImpositionByType(styledef);

	} else return NULL; //no file and no styledef!

	if (config && imp) imp->dump_in_atts(config,0,NULL);
	return imp;
}

} // namespace Laidout

