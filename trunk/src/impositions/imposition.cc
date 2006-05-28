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
/**************** imposition.cc *********************/

#include "imposition.h"
#include <lax/lists.cc>
#include <lax/interfaces/pathinterface.h>
#include <lax/transformmath.h>

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



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
 */
//class PageLocation : public Laxkit::anObject
//{	
// public:
//	PageLocation(int ni,Page *npage,LaxInterfaces::SomeData *trans,int outlineislocal);
//	~PageLocation();
//	int index;
//	Page *page;
//	LaxInterfaces::SomeData *outline;
//	int outlineislocal;
//};

//! Constructor, just copies over pointers.
/*! The page and attributes are assumed to not be owned locally. If local==0, then
 * outline is assumed to be managed with its count, which will be incremented here, and decremented in
 * the destructor. If local==1, then outline is local, and will be delete'd in the destructor.
 * If local is any other value, no action is taken on outline in the destructor.
 */
PageLocation::PageLocation(int ni,Page *npage,LaxInterfaces::SomeData *poutline,int local)
{
	index=ni;
	page=npage;
	outline=poutline;
	outlineislocal=local;
	if (local==0) poutline->inc_count();
}

//! Page, attributes not delete'd, see constructor for how outline is dealt with.
PageLocation::~PageLocation()
{
	if (outlineislocal==0) outline->dec_count();
	else if (outlineislocal==1) delete outline;
}

//----------------------- Spread -------------------------------
/*! \class Spread
 * \brief A generic container for the various things that Imposition returns.
 *
 * This class gets used by other classes, and those other classes are 
 * responsible for maintaining Spread's components.
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
 */
/*! \var int Spread::pathislocal
 * \brief Delete path if pathislocal==1, otherwise path->dec_count().
 */
/*! \var int Spread::marksarelocal
 * \brief Delete marks if marksarelocal==1, otherwise marks->dec_count().
 */
/*! \var int Spread::spreadtype
 * \brief What is the shape of this spread.
 *
 * This value is used by SpreadEditors to only generate a new spread if the current
 * one is the wrong shape.
 */


//! Basic init, set all to 0.
Spread::Spread()
{
	mask=style=0;
	path=marks=NULL;
	pathislocal=marksarelocal=0;
}

//! Deletes path and marks if local==1, otherwise call dec_count() for them.
Spread::~Spread()
{
	if (path) {
		if (pathislocal) delete path; else path->dec_count();
	}
	if (marks) {
		if (marksarelocal) delete marks; else marks->dec_count();
	}
	pagestack.flush();
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
	for (c=0; c<pagestack.n; c++) {
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
	DBG cout <<"pagesfromSpread list: ";
	DBG for (c=0; c<list.n; c++) cout <<list.e[c]<<' '; 
	DBG cout <<endl;
	
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

	DBG cout <<"pagesfromSpread list2: ";
	DBG for (c=0; c<list2.n; c++) cout <<list2.e[c]<<' ';
	DBG cout <<endl;

	return list2.extractArray();
}

//----------------------------- Imposition --------------------------

/*! \class Imposition
 * \brief The abstract base class for imposition styles.
 *
 * The Imposition class is meant to describe a single style of chopping
 * and folding of same sized pieces of paper. The individual papers can
 * be of different colors, but must be the same size. Something like a book where
 * the cover would be printed on a piece of paper with a different size then the
 * body papers would not be all done in a single Imposition. That would be a ProjectStyle
 * with 2 Imposition classes used (???).
 * 
 * It is the responsibility of the Imposition subclass to make sure numpapers, numspreads,
 * and numpages are all consistent with each other.
 * 
 * \todo needs to be a standard to be duplex aware..
 * \todo ***finishing imping built in impositions
 * 
 * Currently the built in Imposition styles are
 *  single pages,
 *  singles meant as double sided, such as would be stapled in the
 *    corner or along the edge,
 *  the slightly more versatile booklet format where the papers are folded at the spine,
 *  and the super-duper BasicBookImposition, comprised of multiple sections, each of which have 
 *    one fold down the middle. Section there means basically the same
 *    as "signature". Also, the basic book imposition works together with a
 *    BookCover imposition, which is basically 4 pages, and the spine.
 *
 * The imposition's name is stored in stylename inherited from Style.
 *
 * NOTE that numpages, numspreads, and numpapers SHOULD be consistent with the actual number
 * of pages in a document, but the document can add or remove pages whenever it wants, so
 * care must be taken, especially in Document, to maintain sanity
 * This is ugly!! maybe have imposition point to a doc??
 */
/*! \var int Imposition::numpapers
 * \brief The number of papers available.
 */
/*! \var int Imposition::numspreads
 * \brief The number of page spreads available.
 */
/*! \var int Imposition::numpages
 * \brief The number of pages available.
 */
/*! \var PaperStyle *Imposition::paperstyle
 * \brief A local instance of the type of paper to print on.
 */
/*! \fn LaxInterfaces::SomeData *Imposition::GetPrinterMarks(int papernum=-1)
 * \brief Return the printer marks for paper papernum in paper coordinates.
 *
 * Default is to return NULL.
 * This is usually a group of SomeData.
 */
/*! \fn Page **Imposition::CreatePages()
 * \brief Create the required pages.
 *
 * Derived class must define this function.
 */
/*! \fn SomeData *Imposition::GetPage(int pagenum,int local)
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
 * \brief Return the number of pages required to fill npapers of papers.
 */
/*! \fn int Imposition::PaperFromPage(int pagenumber)
 * \brief Return the (first) paper index number that contains page index pagenumber in doc->pages.
 */
/*! \fn int Imposition::NumPapers()
 * \brief Return the the number of papers the imposition thinks there are.
 */
/*! \fn int Imposition::NumPages()
 * \brief Return the the number of pages the imposition thinks there are.
 */
/*! \fn int Imposition::NumSpreads()
 * \brief Return the the number of spreads the imposition thinks there are.
 */
/*! \fn int Imposition::PageType(int page)
 * \brief Return the type of page this page index requires.
 *
 * This value makes sense only to the type of imposition. It is used to ensure that
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
 */



//! Constructor.
/*! Default Style constructor sets styledef, basedon to NULL. Any impositions that are 
 *  explicitly based on another imposition must set up the proper styledef and basedon
 *  themselves. The standard built in impositions all act autonomously, meaning they each
 *  completely define their own StyleDef, and are not based on another Imposition.
 */
Imposition::Imposition(const char *nsname)
	: Style (NULL,NULL,nsname)
{ 
	doc=NULL;
	paperstyle=NULL; 
	numpages=numspreads=numpapers=0; 
}


//! Return a box describing a good scratchboard size for pagelayout (page==1) or paper layout (page==0).
/*! Default is to return bounds 3 times the paper size.
 *
 * Place results in bbox if bbox!=NULL. If bbox==NULL, then create a new DoubleBBox and return that.
 */
Laxkit::DoubleBBox *Imposition::GoodWorkspaceSize(Laxkit::DoubleBBox *bbox)//page=1
{
	if (!paperstyle) return NULL;
	
	if (!bbox) bbox=new DoubleBBox();
	bbox->setbounds(-paperstyle->width,2*paperstyle->width,-paperstyle->height,2*paperstyle->height);
	return bbox;
}

//! Just makes sure that s can be cast to Imposition. If yes, return s, if no, return NULL.
Style *Imposition::duplicate(Style *s)//s=NULL
{
	if (s==NULL) return NULL; // Imposition is abstract
	Imposition *d=dynamic_cast<Imposition *>(s);
	if (!d) return NULL;
	return s;
}

//! Ensure that each page has a proper pagestyle.
/*! This is called when pages are added or removed.
 *
 *  When the pagestyle is a custom style, then the PageStyle::flags are preserved,
 *  and applied to a new duplicate of the actual default pagestyle.
 *  This preserves whether page clips and facing pages bleed.
 *
 * \todo *** perhaps break this down so there's a SyncPage()?
 */
int Imposition::SyncPages(Document *doc,int start,int n)
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
		doc->pages.e[c]->InstallPageStyle(temppagestyle,0);//adds 1 count
		temppagestyle->dec_count();
	}
	return 0;
}

//! Default is to delete old paperstyle and replace it with a duplicate of the given PaperStyle.
/*! Derived classes might react to getting
 * a new paper style by changing the pagestyle, for instance. This function basically
 * bypasses the Style methods of returning a FieldMask of other values that change.
 * So, this function should only be called when it is known the FieldMask feedback
 * is not needed. Otherwise one should call imposition.set("paperstyle",PaperStyle)
 * or whatever is appropriate for that imposition.
 *
 * Derived classes are responsible for setting the PageStyle to an appropriate
 * value in response to the new papersize.
 *
 * Return 0 success, nonzero error.
 */
int Imposition::SetPaperSize(PaperStyle *npaper)
{
	if (!npaper) return 1;
	PaperStyle *pp;
	pp=(PaperStyle *)npaper->duplicate();
	if (pp) {
		if (paperstyle) delete paperstyle;
		paperstyle=pp;
	}
	return 0;
}

//! Set the number of papers to npapers, and set numpages,numspreads as appropriate.
/*! Default is to set numpapers=npapers, and numpages=GetPagesNeeded(numpapers).
 * Does not check to make sure npapers is a valid number.
 *
 * Returns the new value of numpapers.
 */
int Imposition::NumPapers(int npapers)
{
	numpapers=npapers;
	numpages=GetPagesNeeded(numpapers);
	numspreads=GetSpreadsNeeded(numpages);
	return numpapers;
}

//! Set the number of pages to npage, and set numpapers,numspreads as appropriate.
/*! Default is to set numpagess=npages, and numpapers=GetPapersNeeded(numpapers).
 * Does not check to make sure npages is a valid number.
 *
 * Returns the new value of numpages.
 */
int Imposition::NumPages(int npages)
{
	numpages=npages;
	numpapers=GetPapersNeeded(numpages);
	numspreads=GetSpreadsNeeded(numpages);
	return numpages;
}

//! Return outline of paper in paper coords. Origin is paper origin.
/*! The default is to return a PathsData rectangle with width=paperstyle->w(),
 * and height=paperstyle->h(), and with the origin at the typical postscript
 * position of the lower left corner.
 *
 * This is a no frills outline, used primarily to check where the mouse
 * is clicked down on.
 * If local==1 then return a new local SomeData. Otherwise return a
 * counted object which might exist elsewhere already.
 * In this case, the item will have a count referring to
 * returend pointer. So if it is created here, and immediately
 * checked in, then it is removed from existence.
 */
SomeData *Imposition::GetPaper(int papernum,int local)
{
	PathsData *newpath=new PathsData();//count==1
	newpath->appendRect(0,0,paperstyle->w(),paperstyle->h());
	newpath->maxx=paperstyle->w();
	newpath->maxy=paperstyle->h();
	//nothing special is done when local==0
	return newpath;
}

//! Return a spread corresponding to the single whichpage.
/*! The path created here is one path for the page, which is found with 
 * a call to GetPage(whichpage,0).
 * The bounds of the page are put in spread->path. Usually, exotic impositions 
 * with strange page shapes need to only redefine GetPage() for this 
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
	spread->path=GetPage(whichpage,0);
	transform_identity(spread->path->m()); // clear any transform
	spread->pathislocal=0;
	
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
	spread->pagestack.push(new PageLocation(whichpage,NULL,spread->path,0)); //(index,page,somedata,local)
	//at this point spread->path has additional count of 2, 1 for pagestack.e[0]->outline
	//and 1 for spread->path

	return spread;
}
