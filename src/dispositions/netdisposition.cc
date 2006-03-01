/**************** dispositions/netdisposition.cc *********************/

#include "disposition.h"
#include <lax/lists.cc>
#include <lax/interfaces/pathinterface.h>
#include <lax/refcounter.h>

using namespace Laxkit;

extern RefCounter<SomeData> datastack;
extern RefCounter<anObject> objectstack;

//----------------------------- NetDisposition --------------------------

/*! \class NetDisposition
 * \brief Uses a Net object to define the pages and spread.
 *
 * The PaperLayout() returns the same as the PageLayout, with the important
 * addition of the extra matrix. This matrix can be adjusted by the user
 * to potentially make the net blow up way beyond the paper size, so as
 * to make tiles of it.
 */
//class NetDisposition : public Disposition
//{
// public:
//	Net *net;
//
//	NetDisposition(const char *nsname, Net *newnet=NULL);
//	virtual ~NetDisposition();
//	virtual Style *duplicate(Style *s=NULL);
//	
////	virtual int SetPageLikeThis(PageStyle *npage); // copies pagestyle, doesnt transfer pointer
//	virtual int SetPaperSize(PaperType *npaper); // set paperstyle, and compute page size
////	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
//	
//	virtual Laxkit::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
//	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
////	virtual int SyncPages(Document *doc,int start,int n);
//
//	virtual Laxkit::SomeData *GetPage(int pagenum,int local); // return outline of page in page coords
//
//	virtual Spread *GetLittleSpread(int whichpage); 
////	virtual Spread *SingleLayout(int whichpage); 
//	virtual Spread *PageLayout(int whichpage); 
//	virtual Spread *PaperLayout(int whichpaper);
//	virtual DoubleBBox *GetDefaultPageSize(DoubleBBox *bbox=NULL);
//	virtual int *PrintingPapers(int frompage,int topage);
//
//	virtual int NumPapers(int npapers);
//	virtual int NumPages(int npages);
//	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
//};

//! Constructor.
/*! Default Style constructor sets styledef, basedon to NULL. Any dispositions that are 
 *  explicitly based on another disposition must set up the proper styledef and basedon
 *  themselves. The standard built in dispositions all act autonomously, meaning they each
 *  completely define their own StyleDef, and are not based on another NetDisposition.
 */
NetDisposition::NetDisposition(const char *nsname, Net *newnet)
	: Disposition(nsname)
{ 
	net=newnet;
	*** setup default paperstyle and pagestyle
}
 
//! Deletes net.
NetDisposition::~NetDisposition()
{
	if (net) delete net;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
int Singles::SetPaperSize(PaperType *npaper)
{
	if (!npaper) return 1;
	if (paperstyle) delete paperstyle;
	paperstyle=(PaperType *)npaper->duplicate();
	
	setPage();
	return 0;
}

//! Using the paperstyle, create a new default pagestyle.
void Singles::setPage()
{
	if (!paperstyle) return;
	if (pagestyle) delete pagestyle;
	pagestyle=new RectPageStyle(RECTPAGE_LRTB);
	pagestyle->width=(paperstyle->w()-insetl-insetr)/tilex;
	pagestyle->height=(paperstyle->h()-insett-insetb)/tiley;
}

//! Just makes sure that s can be cast to NetDisposition. If yes, return s, if no, return NULL.
Style *NetDisposition::duplicate(Style *s)//s=NULL
{
	NetDisposition *d;
	if (s==NULL) d=new NetDisposition();
	else d=dynamic_cast<NetDisposition *>(s);
	if (!d) return NULL;
	
	 // copy net
	d->net=net->duplicate();
		
	return s;
}


//! Return a box describing a good scratchboard size for pagelayout (page==1) or paper layout (page==0).
/*! Default is to return bounds 3 times the paper or page size, with the paper/page centered.
 *
 * Place results in bbox if bbox!=NULL. If bbox==NULL, then create a new DoubleBBox and return that.
 */
Laxkit::DoubleBBox *NetDisposition::GoodWorkspaceSize(int page,Laxkit::DoubleBBox *bbox)//page=1
{
	if (page==0 || page==1) {
		if (!bbox) bbox=new DoubleBBox();
		bbox->setbounds(-paperstyle->width,2*paperstyle->width,-paperstyle->height,2*paperstyle->height);
	} else return NULL;
	return bbox;
}

//! Store a duplicate of the given PaperStyle.
/*! This deletes any old paperstyle. 
 */
int NetDisposition::SetPaperSize(PaperType *npaper)
{
	Disposition::SetPaperSize(npaper);
	
	 // revamp default pagestyle, just adjusts the size to be same as paper
	pagestyle->width=paperstyle->w();
	pagestyle->width=paperstyle->h();
		
	return 0;
}


/*! \fn Page **NetDisposition::CreatePages(PageStyle *pagestyle=NULL)
 * \brief Create the required pages.
 *
 * If pagestyle is not NULL, then this style is to be preferred over
 * the internal page style(?!!?!***)
 */
Page **NetDisposition::CreatePages(PageStyle *pagestyle=NULL)
{
	if (numpages==0) return NULL;
	Page **pages=new Page*[numpages+1];
	int c;
	PageStyle *ps=(PageStyle *)(thispagestyle?thispagestyle:pagestyle)->duplicate();
	for (c=0; c<numpages; c++) {
		 // pagestyle is passed to Page, not duplicated.
		 // There it is checkout'ed from objectstack.
		pages[c]=new Page(ps,0,c); 
	}
	pages[c]=NULL;
	return pages;
}


/*! \fn SomeData *NetDisposition::GetPage(int pagenum,int local)
 * \brief Return outline of page in paper coords. Origin is page origin.
 *
 * This is a no frills outline, used primarily to check where the mouse
 * is clicked down on.
 * If local==1 then return a new local SomeData. Otherwise return a
 * datastack object. In this case, the item should be guaranteed to be pushed
 * already, but not checked out.
 */
Laxkit::SomeData *NetDisposition::GetPage(int pagenum,int local)
{
	PathsData *newpath=new PathsData();
	flatpoint p[net->faces[c]->pn];
	for (int c=0; c<net->faces[c]->pn; c++) {
		p[c]=net->p[net->faces[c]->p[c]];
		***transform point to proper page coords
	}
	for (int c=0; c<net->faces[c]->pn; c++) newpath->append(p[c].x,p[c].y);
	//newpath->FindBBox();
	if (local==0) datastack.push(newpath,1,newpath->object_id,1);
	return newpath;
}

//! Just returns PageLayout(whichpage).
Spread *NetDisposition::GetLittleSpread(int whichpage)
{ return PageLayout(whichpage); }


/*! \fn Spread *NetDisposition::PageLayout(int whichpage)
 * \brief Returns a page view spread that contains whichpage, in viewer coords.
 *
 * whichpage starts at 0.
 * Derived classes must fill the spread with a path, and the PageLocation stack.
 * The path holds the outline of the spread, and the PageLocation stack holds
 * transforms to get from the overall coords to each page's coords.
 */
Spread *NetDisposition::PageLayout(int whichpage)
{
	if (!net) return NULL;
	int c;
	
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	int firstpage=(whichpage/net->nf)*net->nf;
	PathsData *newpath;
	for (int c=0; c<net->nf; c++) {
		newpath=new PathsData();
		***
		spread->pagestack.push(new PageLocation(firstpage+c,NULL,ntrans,1,NULL));
	}

	--------------
	 // define max/min points
	spread->minimum=flatpoint(paperstyle->w()/5,paperstyle->h()/2);
	spread->maximum=flatpoint(paperstyle->w()*4/5,paperstyle->h()/2);

	 // fill spread with paper and page outline
	spread->path=(SomeData *)newpath;
	spread->pathislocal=1;
	 // make the page outline
	newpath->pushEmpty(); //*** later be a certain linestyle
	newpath->appendRect(insetl,insetb, paperstyle->w()-insetl-insetr,paperstyle->h()-insett-insetb);
	
	 // setup spread->pagestack
	 // page width/height must map to proper area on page.
	PathsData *ntrans=new PathsData();
	newpath->appendRect(0,0, pagestyle->w(),pagestyle->h());
	newpath->FindBBox();
	ntrans->origin(flatpoint(insetl,insetb));
	spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans,1,NULL));
		
}

/*! \fn Spread *NetDisposition::PaperLayout(int whichpaper)
 * \brief Returns same as PageLayout, but with the paper outline included(??).
 */
Spread *NetDisposition::PaperLayout(int whichpaper)
{
	if (!net) return NULL;
	int c;
	Spread *spread=new PageLayout(whichpaper*net->nf);
	spread->style=SPREAD_PAPER;

	 // make the paper outline
	((PathsData *)(spread->outline))->appendRect(0,0,paperstyle->w(),paperstyle->h());

	return spread;
}

//! Returns the bounding box in paper units for the default page size.
/*! 
 * If bbox is not NULL, then put the info in the supplied bbox. Otherwise
 * return a new DoubleBBox.
 *
 * The orientation of the box is determined internally to the NetDisposition,
 * and accessed through the other functions here. 
 * minx==miny==0 which is the lower left corner of the page. This function
 * is useful mainly for speedy layout functions.
 *
 * \todo *** is this function actaully used anywhere? anyway it's broken here,
 * just returns bounds of paper.
 */
DoubleBBox *NetDisposition::GetDefaultPageSize(DoubleBBox *bbox=NULL)
{
	if (!paperstyle) return NULL;
	if (!bbox) bbox=new DoubleBBox;
	bbox->minx=0;
	bbox->miny=0;
	bbox->maxx=(paperstyle->w());
	bbox->maxy=(paperstyle->h());
	return bbox;
}

/*!\brief Return a specially formatted list of papers needed to print the range of pages.
 *
 * It is a -2 terminated int[] of papers needed to print [frompage,topage].
 * A range of papers is specified using 2 consecutive numbers. Single papers are
 * indiciated by a single number followed by -1. For example, a sequence { 1,5, 7,-1,10,-1,-2}  
 * means papers from 1 to 5 (inclusive), plus papers 7 and 10.
 */
int *NetDisposition::PrintingPapers(int frompage,int topage)
{
	int fp,tp;
	if (topage<frompage) { tp=topage; topage=frompage; frompage=tp; }
	fp=frompage/net->nf;
	tp=topage/net->nf;
	if (fp==tp) {
		int *i=new int[3];
		i[0]=fp;
		i[1]=-1;
		i[2]=-2;
		return i;
	}
	int *i=new int[3];
	i[0]=fp;
	i[1]=tp;
	i[2]=-2;
	return i;
}


//! Returns pagenumber/net->nf.
int NetDisposition::PaperFromPage(int pagenumber)
{
	if (!net) return 0;
	return pagenumber/net->nf;
}

//! Returns npapers*net->nf.
int NetDisposition::GetPagesNeeded(int npapers)
{
	if (!net) return 0;
	return npapers*net->nf;
}

//! Returns (npages-1)/net->nf+1.
int NetDisposition::GetPapersNeeded(int npages)
{
	if (!net) return 0;
	return (npages-1)/net->nf+1;
}

