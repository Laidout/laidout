/**************** disposition.cc *********************/

#include "disposition.h"
#include <lax/lists.cc>
#include <lax/interfaces/pathinterface.h>
#include <lax/refcounter.h>

using namespace Laxkit;

extern RefCounter<SomeData> datastack;
extern RefCounter<anObject> objectstack;

// TODO
//
// Perhaps divide Disposition to:
// Disposition
// 	DispositionPart: pages 1..50,52..60,62..100
// 	DispositionPart: pages 51,52,61,62
// This allows different page sizes for the same document.. useful for cover/book body, or non-reg polyhedra
// 
//***need a special mechanism for multiple pages on viewport, so PaperLayout/PageLayout can return just what's in viewport???
//  	(like the infinite scroll in acrobat for instance)
// *** need to think how to have master pages
//
//
// *****
// Necessary disposition styles to build in:
// Singles
// 		just one sheet at a time, 1 page==1 sheet
// Facing Singles
// 		Like singles, but is assumed double sided, displays facing pages, can adjust inner/outer margins
// One Fold --- for books: 
// 		multiple sections of signatures that have one fold down the middle, and any number tiling across page
// 		so after cutting, each sheet has 4 pages on it. A signature in this case means a stack of
// 		sheets, not just once huge sheet folded multiple times. So for instance, 1 signature making
// 		n pages (n%4==0) has the top sheet with pages 1,2,n-1,n, and the bottom sheet has 
// 		pages n/2-1,n/2,n/2+1,n/2+2. Either horizontal (like a book) or vertical (like a calendar).
//

//------------------------- PageLocation --------------------------------------

/*! \class PageLocation
 * \brief Disposition classes would derive PageLocation classes from this.
 *
 *  This is a shell around a page, providing a convenience for placement on
 *  the screen and also on a paper. Provides an initial transformation from
 *  paper coordinates to page coordinates in the transform member, which also
 *  should hold the bounding box of the page in paper or view coordinates. 
 *  index is the index of the page in the relevant Document->pages stack.
 *  
 *  As for other page related metrics, the page outline is kept in a
 *  Spread. The thumbnail (if available), margin path, and page number are
 *  all kept by Page.
 *
 *  The page is assumed to be nonlocal, but the transform Is assumed to be local, and
 *  thus will be delete'd in the PageLocation destructor.
 *  ***not sure what's up with attributes yet, if anything....
 */
/*! \var int PageLocation::index
 * \brief the page index (starting at 0) of the page in the relevant page stack.
 */
/*! \var SomeData *PageLocation::outline
 * \brief Holds the transform to get to the page, and also the outline of the page.
 *
 * It is typically a PathsData, but the actual form of it is up to the Disposition.
 */
//class PageLocation : public Laxkit::anObject
//{	
// public:
//	PageLocation(int ni,Page *npage,Laxkit::SomeData *trans,int outlineislocal,void **natts);
//	~PageLocation();
//	int index;
//	Page *page;
//	Laxkit::SomeData *outline;
//	int outlineislocal;
//	void **attributes;
//};

//! Constructor, just copies over pointers.
/*! The page and attributes are assumed to not be owned locally. If local==0, then
 * outline is assumed to be a datastack object, and will be checked out here, and checked in
 * on destruction. If local==1, then outline is local, and will be delete'd in the destructor.
 * If local is any other value, no action is taken on outline in the destructor.
 */
PageLocation::PageLocation(int ni,Page *npage,Laxkit::SomeData *poutline,int local,void **natts)
{
	index=ni;
	page=npage;
	outline=poutline;
	outlineislocal=local;
	if (local==0) datastack.push(poutline,1,poutline->object_id,1);
	attributes=natts;
}

//! Page, attributes not delete'd, see constructor for how outline is dealt with.
/*! *** what about attributes?? gotta figure out wtf those are....
 */
PageLocation::~PageLocation()
{
	if (outlineislocal==0) datastack.checkin(outline);
	else if (outlineislocal==1) delete outline;
}

typedef PtrStack<PageLocation> PageLocStack;

//----------------------- Spread -------------------------------
/*! \class Spread
 * \brief A generic container for the various things that Disposition returns.
 *
 * This class gets used by other classes, and those other classes are 
 * responsible for maintaining Spread's components.
 *
 * *** should put the page outline in the pagelocation stack?? maybe?
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
/*! \var Laxkit::SomeData *Spread::path
 * This is the outline of whatever is relevant. The Spread class is used as a return type
 * for many functions in Disposition, so the actual meaning of path depends 
 * on which Disposition function was called. Could be a single page, page spread,
 * or paper spread, etc.
 */
/*! \var int Spread::pathislocal
 * \brief Delete path if pathislocal==1, otherwise datastack.checkin(path).
 */
/*! \var int Spread::marksarelocal
 * \brief Delete marks if marksarelocal==1, otherwise datastack.checkin(marks).
 */
//class Spread : public Laxkit::anObject
//{
// public:
//	unsigned int mask; // which of path,min,max,pages is defined
//	unsigned int style; // says what is the type of thing this spread refers to. See Disposition.
//	Laxkit::SomeData *path;
//	int pathislocal;
//	Laxkit::SomeData *marks;
//	int marksarelocal;
//	flatpoint minimum,maximum; //are in path coordinates, useful for littlespreads in Spread editor
//	
//	Laxkit::PtrStack<PageLocation> pagestack;
//
//	Spread();
//	virtual ~Spread();
//	virtual int *pagesFromSpread();
//};

//! Basic init, set all to 0.
Spread::Spread()
{
	mask=style=0;
	path=marks=NULL;
	pathislocal=marksarelocal=0;
}

//! Deletes path and marks if local==1, otherwise call datastack.checkin(path and marks).
Spread::~Spread()
{
	if (path) {
		if (pathislocal) delete path; else datastack.checkin(path);
	}
	if (marks) {
		if (marksarelocal) delete marks; else datastack.checkin(marks);
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
			if (c2==list.n) { list.push(i,c2); break; }
		}
	}
cout <<"pagesfromSpread list: ";
for (c=0; c<list.n; c++) cout <<list.e[c]<<' '; 
cout <<endl;
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
cout <<"pagesfromSpread list2: ";
for (c=0; c<list2.n; c++) cout <<list2.e[c]<<' ';
cout <<endl;
	return list2.extractArray();
}

//----------------------------- Disposition --------------------------

/*! \class Disposition
 * \brief The abstract base class for disposition styles.
 *
 * The Disposition class is meant to describe a single style of chopping
 * and folding of same sized pieces of paper. The individual papers can
 * be of different colors, but must be the same size. Something like a book where
 * the cover would be printed on a piece of paper with a different size then the
 * body papers would not be all done in a single Disposition. That would be a ProjectStyle
 * with 2 Disposition classes used.
 * 
 * Currently the built in Disposition styles are single pages,
 * singles meant as double sided, such as would be stapled in the
 * corner or along the edge, the slightly more versatile booklet
 * format where the papers are folded at the spine, and the super-duper
 * BasicBookDisposition, comprised of multiple sections, each of which have 
 * one fold down the middle. Section there means basically the same
 * as "signature". Also, the basic book disposition works together with a
 * BookCover disposition, which is basically 4 pages, and the spine.
 *
 * The disposition's name is stored in stylename inherited from Style.
 *
 * ****** NOTE that numpages SHOULD be the actual number of pages in a document, but the
 * document can add or remove pages whenever it wants, so care must be taken so that numpages
 * is the same as document->pages.n.... This is ugly!! maybe have disposition point to a doc??
 *
 * \todo *** the handling of pagestyle needs to be cleaned up still.. loading often 
 * installs an improper pagestyle.
 */
 /*! \var int Disposition::numpapers
  * \brief The number of papers to set for the document.
  */
 /*! \var int Disposition::numpages
  * \brief The number of pages to set for the document.
  */
 /*! \var PaperType *Disposition::paperstyle
  * \brief A local instance of the type of paper to print on.
  */
 /*! \var PageStyle *Disposition::pagestyle
  * \brief A local instance of the default page style.
  */
 /*! \fn Laxkit::SomeData *Disposition::GetPrinterMarks(int papernum=-1)
  * \brief Return the printer marks for paper papernum in paper coordinates.
  *
  * Default is to return NULL.
  * This is usually a group of SomeData.
  */
 /*! \fn Page **Disposition::CreatePages(PageStyle *pagestyle=NULL)
  * \brief Create the required pages.
  *
  * Derived class must define this function.
  * If pagestyle is not NULL, then this style is to be preferred over
  * the internal page style(?!!?!***remove this? just assume default pagestyle?)
  */
 /*! \fn SomeData *Disposition::GetPage(int pagenum,int local)
  * \brief Return outline of page in page coords. Origin is page origin.
  *
  * This returns a no frills outline, used primarily to check where the mouse
  * is clicked down on.
  * If local==1 then return a new local SomeData. Otherwise return a
  * datastack object. In this case, the item should be guaranteed to be pushed
  * already, but not checked out.
  * 
  * Derived class must define this function.
  */
 /*! \fn Spread *Disposition::GetLittleSpread(int whichpage)
  * \brief Returns outlines of pages in page view, in viewer coords,
  * 
  * Mainly for use in the spread editor, so it would have little folded corners
  * and such, perhaps a thumbnail also. This spread is the one that
  * includes whichpage. 
  *
  * This spread should correspond to the PageLayout for the same page. It might be
  * augmented to contiain little dogeared conrners, for instance. Also, it really
  * should contain a continuous range of pages. Might trip up the spread editor
  * otherwise.
  *
  * Derived class must define this function.
  *
  * \todo *** this needs to modified so that -1 returns NULL, -2 returns a generic
  * page to be used in the SpreadEditor, and -3 returns a generic page layout spread
  * for the SpreadEditor. Those are used as temp page holders when pages are pushed
  * into limbo in the editor.
  */
 /*! \fn Spread *Disposition::PageLayout(int whichpage)
  * \brief Returns a page view spread that contains whichpage, in viewer coords.
  *
  * whichpage starts at 0.
  * Derived classes must fill the spread with a path, and the PageLocation stack.
  * The path holds the outline of the spread, and the PageLocation stack holds
  * transforms to get from the overall coords to each page's coords.
  *
  * Derived class must define this function.
  */
 /*! \fn Spread *Disposition::PaperLayout(int whichpaper)
  * \brief Returns a paper view spread that contains whichpaper, in paper coords.
  *
  * whichpaper starts at 0.
  * Derived class must define this function.
  */
 /*! \fn Laxkit::DoubleBBox *Disposition::GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL)
  * \brief Returns the bounding box in paper units for the default page size.
  * 
  * If bbox is not NULL, then put the info in the supplied bbox. Otherwise
  * return a new DoubleBBox.
  *
  * The orientation of the box is determined internally to the Disposition,
  * and accessed through the other functions here. 
  * minx==miny==0 which is the lower left corner of the page. This function
  * is useful mainly for speedy layout functions.
  * 
  * Derived class must define this function.
  */
 /*! \fn int *Disposition::PrintingPapers(int frompage,int topage)
  * \brief Return a specially formatted list of papers needed to print the range of pages.
  *
  * It is a -2 terminated int[] of papers needed to print [frompage,topage].
  * A range of papers is specified using 2 consecutive numbers. Single papers are
  * indiciated by a single number followed by -1. For example, a sequence { 1,5, 7,-1,10,-1,-2}  
  * means papers from 1 to 5 (inclusive), plus papers 7 and 10.
  */
 /*! \fn int Disposition::GetPagesNeeded(int npapers)
  * \brief Return the number of pages required to fill npapers number of papers.
  */
 /*! \fn int Disposition::GetPapersNeeded(int npages)
  * \brief Return the number of pages required to fill npapers of papers.
  */
 /*! \fn int Disposition::PaperFromPage(int pagenumber)
  * \brief Return the (first) paper index number that contains page index pagenumber
  */
//class Disposition : public Style
//{
//  public:
//	int numpapers; // ".0"
//	int numpages; // ".1"
//	PaperType *paperstyle; // ".2" is the default paper style, assumed to be local
//	PageStyle *pagestyle; // ".3" is the default page style, assumed to be local
//
//	Disposition(const char *nsname);
//	virtual ~Disposition() {}
//	virtual Style *duplicate(Style *s=NULL);
//	
//	virtual int SetPageLikeThis(PageStyle *npage); // copies pagestyle, doesnt transfer pointer
//	virtual int SetPaperSize(PaperType *npaper); // set paperstyle, and compute page size
//	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
//	
////	virtual void AdjustPages(Page **pages) {} // when changing page size and atts, return bases for the new pages
//	virtual Laxkit::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
//	virtual Page **CreatePages(PageStyle *pagestyle=NULL) = 0; // create necessary pages based on default pagestyle
//	virtual int SyncPages(Document *doc,int start,int n);
//
//	virtual Laxkit::SomeData *GetPaper(int papernum,int local); // return outline of paper in paper coords
//	virtual Laxkit::SomeData *GetPage(int pagenum,int local) = 0; // return outline of page in page coords
//
//	virtual Spread *GetLittleSpread(int whichpage) = 0; 
//	virtual Spread *SingleLayout(int whichpage); 
//	virtual Spread *PageLayout(int whichpage) = 0; 
//	virtual Spread *PaperLayout(int whichpaper) = 0;
//	virtual DoubleBBox *GetDefaultPageSize(DoubleBBox *bbox=NULL) = 0;
//	virtual int *PrintingPapers(int frompage,int topage) = 0;
//
//	virtual int NumPapers(int npapers);
//	virtual int NumPages(int npages);
//	virtual int PaperFromPage(int pagenumber) = 0; // the paper number containing page pagenumber
//	virtual int GetPagesNeeded(int npapers) = 0; // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages) = 0; // how many papers needed to contain n pages
//	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
//};

//! Constructor.
/*! Default Style constructor sets styledef, basedon to NULL. Any dispositions that are 
 *  explicitly based on another disposition must set up the proper styledef and basedon
 *  themselves. The standard built in dispositions all act autonomously, meaning they each
 *  completely define their own StyleDef, and are not based on another Disposition.
 */
Disposition::Disposition(const char *nsname)
	: Style (NULL,NULL,nsname)
{ 
	pagestyle=NULL; 
	paperstyle=NULL; 
	numpages=numpapers=0; 
}


//! Return a box describing a good scratchboard size for pagelayout (page==1) or paper layout (page==0).
/*! Default is to return bounds 3 times the paper or page size, with the paper/page centered.
 *
 * Place results in bbox if bbox!=NULL. If bbox==NULL, then create a new DoubleBBox and return that.
 *
 * *** should probably include single page as well as pagelayout....
 */
Laxkit::DoubleBBox *Disposition::GoodWorkspaceSize(int page,Laxkit::DoubleBBox *bbox)//page=1
{
	if (page==1 && pagestyle) {
		if (!bbox) bbox=new DoubleBBox();
		bbox->setbounds(-pagestyle->width,2*pagestyle->width,-pagestyle->height,2*pagestyle->height);
	} else if (page==0 && paperstyle) {
		if (!bbox) bbox=new DoubleBBox();
		bbox->setbounds(-pagestyle->width,2*pagestyle->width,-pagestyle->height,2*pagestyle->height);
	} else return NULL;
	return bbox;
}

//! Just makes sure that s can be cast to Disposition. If yes, return s, if no, return NULL.
Style *Disposition::duplicate(Style *s)//s=NULL
{
	if (s==NULL) return NULL; // Disposition is abstract
	Disposition *d=dynamic_cast<Disposition *>(s);
	if (!d) return NULL;
	return s;
}

//! Ensure that each page has the proper pagestyle.
/*! Default is insert whatever GetPageStyle() returns, 
 * replacing whatever pagestyle was already there. This returned pagestyle 
 * is checked out from objectstack.
 *
 * Default here is to remove the old pagestyle referenced by the page, and replace
 * it with the default. This might be bad later on, when the pagestyle can be
 * made a custom appearance.
 */
int Disposition::SyncPages(Document *doc,int start,int n)
{
	if (numpages==0 || numpapers==0) {
		NumPages(doc->pages.n);
	}
	if (n==0) return 0;
	if (n<0) n=doc->pages.n;
	PageStyle *temppagestyle;
	for (int c=start; c<doc->pages.n && c<start+n; c++) {
		temppagestyle=GetPageStyle(c);
		if (temppagestyle) {
			doc->pages.e[c]->InstallPageStyle(temppagestyle,0);
		} else {
			cout <<"*** this is error, should not be here, null pagestyle from GetPageStyle!!"<<endl;
		}
	}
	return 0;
}

//! Store a duplicate of the given PaperStyle.
/*! This deletes any old paperstyle. Derived classes might react to getting
 * a new paper style by changing the pagestyle, for instance. This function basically
 * bypasses the Style methods of returning a FieldMask of other values that change.
 * So, this function should only be called when it is known the FieldMask feedback
 * is not needed. Otherwise one should call disposition.set("paperstyle",PaperStyle)
 * or whatever is appropriate for that disposition
 *
 * Derived classes are responsible for setting the PageStyle to an appropriate
 * value in response to the new papersize.
 */
int Disposition::SetPaperSize(PaperType *npaper)
{
	if (!npaper) return 1;
	PaperType *pp;
	pp=(PaperType *)npaper->duplicate();
	if (pp) {
		if (paperstyle) delete paperstyle;
		paperstyle=pp;
	}
	return 0;
}

//! Store a duplicate of the given PageStyle.
/*! Default here is just to make a local copy of npage.
 */
int Disposition::SetPageLikeThis(PageStyle *npage)
{
	if (!npage) return 1;
	PageStyle *pg;
	pg=(PageStyle *)npage->duplicate();
	if (pg) {
		if (pagestyle) delete pagestyle;
		pagestyle=pg;
	}
	return 0;
}

//! Return the default page style for that page.
/*! Default is to return pagestyle->duplicate().
 */
PageStyle *Disposition::GetPageStyle(int pagenum)
{
	if (pagestyle) return (PageStyle *)pagestyle->duplicate();
	return NULL;
}

//! Set the number of papers to npapers, and set numpages as appropriate.
/*! Default is to set numpapers=npapers, and numpages=GetPagesNeeded(numpapers).
 * Does not check to make sure npapers is a valid number.
 *
 * Returns the new value of numpapers.
 */
int Disposition::NumPapers(int npapers)
{
	numpapers=npapers;
	numpages=GetPagesNeeded(numpapers);
	return numpapers;
}

//! Set the number of page to npage, and set numpapers as appropriate.
/*! Default is to set numpagess=npages, and numpapers=GetPapersNeeded(numpapers).
 * Does not check to make sure npages is a valid number.
 *
 * Returns the new value of numpages.
 */
int Disposition::NumPages(int npages)
{
	numpages=npages;
	numpapers=GetPapersNeeded(numpages);
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
 * datastack object. In this case, the item should be guaranteed to be pushed
 * already with an extra count. So if it is created here, then immediately
 * checked in, then it is removed from datastack.
 */
SomeData *Disposition::GetPaper(int papernum,int local)
{
	PathsData *newpath=new PathsData();
	newpath->appendRect(0,0,paperstyle->w(),paperstyle->h());
	newpath->maxx=paperstyle->w();
	newpath->maxy=paperstyle->h();
	if (local==0) datastack.push(newpath,1,newpath->object_id,1);
	return newpath;
}

//! Return a spread corresponding to the single whichpage.
/*! The path created here is one path for the page, which is found with 
 * a call to GetPage(whichpage,0).
 * The bounds of the page are put in spread->path. Usually, exotic dispositions 
 * with strange page shapes need to only redefine GetPage() for this 
 * function to work right.
 *
 * The spread->pagestack elements holds the outline, and the page index.
 */
Spread *Disposition::SingleLayout(int whichpage)
{
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	 // Get the page outline. It will be a datastack object with 1 count for path pointer.
	spread->path=GetPage(whichpage,0);
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
	spread->pagestack.push(new PageLocation(whichpage,NULL,spread->path,0,NULL)); //(index,page,somedata,local,atts)
	//at this point spread->path has a count of 2 in datastack, 1 for pagestack.e[0]->outline
	//and 1 for spread->path

	return spread;
}
