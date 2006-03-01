/*************** dispositioninst.cc ********************/

#include "dispositioninst.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>
#include <lax/refcounter.h>

using namespace Laxkit;
using namespace LaxFiles;

extern RefCounter<anObject> objectstack;
extern RefCounter<SomeData> datastack;

/*! \file 
 * <pre>
 *  This file defines the standard dispositions:
 *   Singles
 *   DoubleSidedSingles
 *   BookletDisposition 
 *   BasicBookDisposition ***todo
 *   CompositeDisposition ***todo
 *
 *  Some other useful ones defined elsewhere are:
 *   Dodecahedron         ***todo (in dodecahedron.cc) 
 *   NetDisposition       ***todo (dispositions/netdisposition.cc)
 * 
 * </pre>
 *
 * \todo *** implement the CompositeDisposition, and figure out good way
 * to integrate into whole system.. that involves clearing up relationship
 * between Disposition, DocStyle, and Document classes.
 * 
 * \todo *** implement BasicBookDisposition
 *
 */



//-------------------------- Singles ---------------------------------------------

/*! \class Singles
 * \brief For single page per sheet, not meant to be next to other pages.
 *
 * The pages can be inset a certain amount from each edge, specified by inset[lrtb].
 */
//class Singles : public Disposition
//{
//  public:
//	double insetl,insetr,insett,insetb;
//	int tilex,tiley;
//	Singles();
//	virtual ~Singles() {}
//	virtual StyleDef *makeStyleDef();
//	
////	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
////	virtual SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
//	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
//	virtual SomeData *GetPage(int pagenum, int local); // return outline of page in paper coords
//	virtual Spread *GetLittleSpread(int whichpage); 
//	virtual Spread *PageLayout(int whichpage); 
//	virtual Spread *PaperLayout(int whichpaper);
//	virtual DoubleBBox *GetDefaultPageSize(DoubleBBox *bbox=NULL);
//	virtual int *PrintingPapers(int frompage,int topage);
//	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//	virtual Style *duplicate(Style *s=NULL);
//	virtual int SetPaperSize(PaperType *npaper);
//	virtual void setPage();
//
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

//Style(StyleDef *sdef,Style *bsdon,const char *nstn)
Singles::Singles() : Disposition("Singles")
{ 
	insetl=insetr=insett=insetb=0;
	tilex=tiley=1;
	//paperstyle=*** global default paper style;
	paperstyle=new PaperType("letter",8.5,11.0,0,300);//***
	setPage();
}

////! Return the default page style for that page.
///*! Default is to return pagestyle->duplicate().
// */
//PageStyle *Singles::GetPageStyle(int pagenum)
//{
//	if (pagestyle) return (PageStyle *)pagestyle->duplicate();
//	return NULL;
//}

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

/*! Define from Attribute.
 *
 * Only implements the standard pagestyle. gotta think about how to 
 * handle exotic page shapes.. (meaning non-rectangular).. Also,
 * 
 * \todo *** should load the default PaperStyle.. right now just inits
 * to letter if defaultpaperstyle attribute found, then dumps in that
 * paper style. see todos in Page also.....
 */
void Singles::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
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
			if (pagestyle) delete pagestyle;
			pagestyle=new PageStyle();//***
			pagestyle->dump_in_atts(att->attributes.e[c]);
		} else if (!strcmp(name,"defaultpaperstyle")) {
			if (paperstyle) delete paperstyle;
			paperstyle=new PaperType("Letter",8.5,11,0,300);//***
			paperstyle->dump_in_atts(att->attributes.e[c]);
		}
	}
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
void Singles::dump_out(FILE *f,int indent)
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
		pagestyle->dump_out(f,indent+2);
	}
	if (paperstyle) {
		fprintf(f,"%sdefaultpaperstyle\n",spc);
		paperstyle->dump_out(f,indent+2);
	}
}

//! Duplicate this, or fill in this attributes.
Style *Singles::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new Singles();
	else if (!dynamic_cast<Singles *>(s)) return NULL;
	Singles *sn=dynamic_cast<Singles *>(s);
	if (!sn) return NULL;
	sn->insetl=insetl;
	sn->insetr=insetr;
	sn->insett=insett;
	sn->insetb=insetb;
	sn->tilex=tilex;
	sn->tiley=tiley;
	return Disposition::duplicate(s);  
}

//! ***imp me! Make an instance of the Singles disposition styledef
/*  Required by Style, this defines the various names for fields relevant to Singles,
 *  basically just the inset[lrtb], plus the standard Disposition npages and npapers.
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in the style def manager *** whatever and wherever that is!!!
 */
StyleDef *Singles::makeStyleDef()
{
	return NULL;
}

//! Create necessary pages based on default pagestyle
/*! Currently returns NULL terminated list of pages.
 *
 * *** is thispagestyle really necessary???
 */
Page **Singles::CreatePages(PageStyle *thispagestyle)//thispagestyle=NULL
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

//! Return outline of page in page coords. 
SomeData *Singles::GetPage(int pagenum,int local)
{
	PathsData *newpath=new PathsData();
	newpath->appendRect(0,0,pagestyle->w(),pagestyle->h());
	newpath->maxx=pagestyle->w();
	newpath->maxy=pagestyle->h();
	if (local==0) datastack.push(newpath,1,newpath->object_id,1);
	return newpath;
}

//! Return a spread for use in the spread editor.
/*! This just returns a normal PageLayout. Perhaps some other
 * time I will make it so there are little dogeared corners. Hoo boy!
 */
Spread *Singles::GetLittleSpread(int whichpage)
{
	return PageLayout(whichpage);
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
 *
 * \todo *** tiling and cut marks are not functional yet
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
	 // make the page outline
	newpath->pushEmpty(); //*** later be a certain linestyle
	newpath->appendRect(insetl,insetb, paperstyle->w()-insetl-insetr,paperstyle->h()-insett-insetb);
	
	 // setup spread->pagestack
	 // page width/height must map to proper area on page.
	PathsData *ntrans=new PathsData();
	ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
	ntrans->FindBBox();
	ntrans->origin(flatpoint(insetl,insetb));
	spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans,1,NULL));
		
	 // make printer marks if necessary
	 //*** make this more responsible lengths:
	if (insetr>0 || insetl>0 || insett>0 || insetb>0) {
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

//! Page size is the paper size minus the inset values.
Laxkit::DoubleBBox *Singles::GetDefaultPageSize(Laxkit::DoubleBBox *bbox) 
{
	if (!paperstyle) return NULL;
	if (!bbox) bbox=new DoubleBBox;
	bbox->minx=0;
	bbox->miny=0;
	bbox->maxx=(paperstyle->w()-insetl-insetr)/tilex;
	bbox->maxy=(paperstyle->h()-insett-insetb)/tiley;
	return bbox;
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


//-------------------------------- DoubleSidedSingles ------------------------------------------------------

/*! \class DoubleSidedSingles
 * \brief For 1 page per sheet, arranged so the pages are to be placed next to each other, first page is like the cover.
 *
 * Please note that Singles::inset* that DoubleSidedSingles inherits are not margins.
 * The insets refer to portions of the paper that would later be chopped off, and are the 
 * same for each page, whether the page is on the left or the right.
 *
 * ***TODO: isvertical is not implemented
 */
/*! \var int DoubleSidedSingles::isvertical
 * \brief Nonzero if pages are top and bottom, rather than left and right.
 */
//class DoubleSidedSingles : public Singles
//{
// public:
//	int isvertical;
//	DoubleSidedSingles();
////	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
//	virtual StyleDef *makeStyleDef();
//	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
//	virtual Spread *PageLayout(int whichpage); 
//	virtual Spread *PaperLayout(int whichpaper);
//	virtual Style *duplicate(Style *s=NULL);
//	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
//	virtual void setPage();
//
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};
//
//Uses these functions from Singles:
//	virtual Spread *GetLittleSpread(int whichpage); 
//use singles: SomeData *DoubleSidedSingles::GetPage(int pagenum,int local)
//use singles: DoubleBBox *DoubleSidedSingles::GetDefaultPageSize(DoubleBBox *bbox) 
//	virtual int *PrintingPapers(int frompage,int topage);
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
////	virtual SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords


//! Set isverticla=0, call setpage().
DoubleSidedSingles::DoubleSidedSingles()
{
	isvertical=0;
	setPage();
	
	 // make style instance name "Double Sided Singles"  perhaps remove the spaces??
	makestr(stylename,"Double Sided Singles");
} 

//! Using the paperstyle and isvertical, create a new default pagestyle.
void DoubleSidedSingles::setPage()
{
	if (pagestyle) delete pagestyle;
	pagestyle=new RectPageStyle((isvertical?RECTPAGE_LRIO:RECTPAGE_IOTB));
	pagestyle->width=(paperstyle->w()-insetl-insetr)/tilex;
	pagestyle->height=(paperstyle->h()-insett-insetb)/tiley;
}

/*! *** must figure out best way to sync up pagestyles...
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
	}
	Singles::dump_in_atts(att);
}

/*! Write out isvertical, then Singles::dump_out.
 */
void DoubleSidedSingles::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (isvertical) fprintf(f,"%sisvertical\n",spc);
	Singles::dump_out(f,indent);
}

//! Copies over isvertical.
Style *DoubleSidedSingles::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new DoubleSidedSingles();
	else if (!dynamic_cast<DoubleSidedSingles *>(s)) return NULL;
	DoubleSidedSingles *ds=dynamic_cast<DoubleSidedSingles *>(s);
	if (!ds) return NULL;
	ds->isvertical=isvertical;
	return Singles::duplicate(s);  
}

//! ***imp me! Make an instance of the DoubleSidedSingles disposition styledef
/*  Required by Style, this defines the various names for fields relevant to DoubleSidedSingles.
 *  Basically just the inset[lrtb], plus the standard Disposition npages and npapers,
 *  and isvertical (which is a flag to say whether this is a booklet or a calendar).
 *
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in the style def manager *** whatever and wherever that is!!!
 */
StyleDef *DoubleSidedSingles::makeStyleDef()
{
	return NULL;
}

//! Return a box describing a good scratchboard size for pagelayout (page==1) or paper layout (page==0).
/*! Default is to return bounds 4 times the page size or 3 times the paper size, with the paper/page centered.
 *
 * Place results in bbox if bbox!=NULL. If bbox==NULL, then create a new DoubleBBox and return that.
 */
Laxkit::DoubleBBox *DoubleSidedSingles::GoodWorkspaceSize(int page,Laxkit::DoubleBBox *bbox)//page=1
{
	if (page==1 && pagestyle) {
		if (!bbox) bbox=new DoubleBBox();
		bbox->setbounds(-pagestyle->w(),3*pagestyle->w(),-pagestyle->h(),3*pagestyle->h());
	} else if (page==0 && paperstyle) {
		if (!bbox) bbox=new DoubleBBox();
		bbox->setbounds(-paperstyle->w(),2*paperstyle->w(),-paperstyle->h(),2*paperstyle->h());
	} else return NULL;
	return bbox;
}

//! Create necessary pages based on default pagestyle
/*! Currently returns NULL terminated list of pages.
 *
 * *** is thispagestyle really necessary??? It is ignored here,
 * and left/right RectPageStyles are used.
 *
 * *** each page is created with its own RectPageStyle, there is mondo duplication
 * here, must implement the style manager, and have the page have only references to it.
 *
 * *** a bit here is totally unsatisfactory: Every page in a spread will possibly have
 * a different configuration of margins. A simple pagestyle like RectPageStyle will
 * keep track of whether the page is a left or right page, but how to get the real
 * l/r/t/b margins on the left or the right page? whose responsiblity is that?
 */
Page **DoubleSidedSingles::CreatePages(PageStyle *thispagestyle)//thispagestyle=NULL
{
	if (numpages==0) return NULL;
	if (!pagestyle && !thispagestyle) return NULL;
	if (!thispagestyle) thispagestyle=pagestyle;
	if (!thispagestyle) return NULL;
	
	RectPageStyle *rps=dynamic_cast<RectPageStyle *>(thispagestyle); //*** do this for Singles also?
	if (!rps) return NULL;
	RectPageStyle *left,*right;
	left= new RectPageStyle(RECTPAGE_LEFTPAGE| (isvertical?RECTPAGE_IOTB:RECTPAGE_LRIO),rps->ml,rps->mr,rps->mt,rps->mb),
	right=new RectPageStyle(RECTPAGE_RIGHTPAGE|(isvertical?RECTPAGE_IOTB:RECTPAGE_LRIO),rps->ml,rps->mr,rps->mt,rps->mb);
	right->width =left->width =rps->w();
	right->height=left->height=rps->h();
	right->flags =left->flags =rps->flags;
	objectstack.push(left ,1,getUniqueNumber(),1);
	objectstack.push(right,1,getUniqueNumber(),1);
	Page **newpages=new Page*[numpages+1];
	int c;
	for (c=0; c<numpages; c++) {
		newpages[c]=new Page((c%2==0?right:left),0,c); // this checksout left or right
	}
	newpages[c]=NULL;
	objectstack.checkin(left);
	objectstack.checkin(right);
	return newpages;
}


//! Return a page spread based on page index (starting from 0) whichpage.
/*! The path created here is one path for the page(s), and another for the middle.
 * The bounds of the spread are put in spread->path.
 * The first page has only itself (*** this should be modifiable!!).
 * If the last page (numbered from 0) is odd, it also has only itself.
 * All other spreads have 2 pages.
 *
 * The spread->pagestack elements hold only the transform.
 * They do not also have the outlines.
 */
Spread *DoubleSidedSingles::PageLayout(int whichpage)
{
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_MINIMUM|SPREAD_MAXIMUM;
	int left=((whichpage+1)/2)*2-1,
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
	datastack.checkin(noutline); // This removes the extra unnecessary tick

	 // left page
	if (left>=0) {
		spread->pagestack.push(new PageLocation(left,NULL,g,1,NULL));
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
		spread->pagestack.push(new PageLocation(right,NULL,g,1,NULL));
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

//---------------------------------- BookletDisposition -----------------------------------------

/*! \class BookletDisposition
 * \brief A disposition made of one bunch of papers folded down the middle.
 *
 * The papers can be folded vertically in the middle like a book or folded horizontally like
 * a calendar. Also, the whole shebang can be tiled across the paper so that when
 * printing, you would cut along the tile lines, then fold the result.
 *
 * The first paper (which holds the first 2 and last 2 pages) can optionally
 * have a different color than the body.
 *
 * \todo *** tiling and cut marks are not functional yet
 */
//class BookletDisposition : public DoubleSidedSingles
//{
// public:
//	double creep;  // booklet.5
//	unsigned long covercolor; // booklet.13
//	unsigned long bodycolor; // booklet.14
//	BookletDisposition();
//	virtual StyleDef *makeStyleDef();
//	virtual Style *duplicate(Style *s=NULL);
//	
////	virtual SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
//	virtual Spread *PaperLayout(int whichpaper);
//	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
//	virtual int *PrintingPapers(int frompage,int topage);
//	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//	virtual void setPage();
//
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};
//doesn't reimp from Singles/DoubleSidedSingles:
//	virtual SomeData *GetPage(int pagenum,int local); // return outline of page in paper coords
//doesn't reimp from DoubleSidedSingles:
//	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
//	virtual Spread *GetLittleSpread(int whichpage); 
//	virtual Spread *PageLayout(int whichpage); 

//! Constructor, init new variables, make style name="Booklet"
BookletDisposition::BookletDisposition()
{
	creep=0;
	covercolor=bodycolor=~0;
	makestr(stylename,"Booklet");
	setPage();
}

//! Using the paperstyle and isvertical, create a new default pagestyle.
void BookletDisposition::setPage()
{
	if (pagestyle) delete pagestyle;
	pagestyle=new RectPageStyle((isvertical?RECTPAGE_LRIO:RECTPAGE_IOTB));
	pagestyle->width=(paperstyle->w()-insetl-insetr)/tilex;
	pagestyle->height=(paperstyle->h()-insett-insetb)/tiley;
	if (isvertical) pagestyle->height/=2;
	else pagestyle->width/=2;
}

//! Read creep, body and covercolor, then DoubleSidedSigles::dump_in_atts().
void BookletDisposition::dump_in_atts(LaxFiles::Attribute *att)
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
}

/*! Something like:
 *  <pre>
 *    creep .05
 *    bodycolor 0xffffffff
 *    colorcolor 0xffffffff
 *    ...DoubleSidedSingles stuff..
 *  </pre>
 */
void BookletDisposition::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f,"%screep %.10g\n",spc,creep);
	fprintf(f,"%sbodycolor 0x%.6lx\n",spc,bodycolor);
	fprintf(f,"%scovercolor 0x%.6lx\n",spc,covercolor);
	DoubleSidedSingles::dump_out(f,indent);
}

//! Duplicate. Copies over creep, tilex, tily, bodycolor, covercolor.
Style *BookletDisposition::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new BookletDisposition();
	else if (!dynamic_cast<BookletDisposition *>(s)) return NULL;
	BookletDisposition *b=dynamic_cast<BookletDisposition *>(s);
	if (!b) return NULL;
	b->creep=creep;
	b->tilex=tilex;
	b->tiley=tiley;
	b->bodycolor=bodycolor;
	b->covercolor=covercolor;
	return DoubleSidedSingles::duplicate(s);  
}

//! ***imp me!
StyleDef *BookletDisposition::makeStyleDef()
{//***
	return NULL;
}

//--- can use DoubleSidedSingles::CreatePages
//Page **BookletDisposition::CreatePages(PageStyle *pagestyle=NULL)
//{ ***
//}

/*! 
 * \todo ***Finish implementing me!! For inset, provide print cut marks about 1/8 inch from actual page outline.
 */
Spread *BookletDisposition::PaperLayout(int whichpaper)
{
	if (!numpapers) return NULL;
	if (whichpaper<0 || whichpaper>=numpapers) whichpaper=0;
	int nnp=numpages;
	numpages=100000000;
	Spread *spread=PageLayout(2); // force getting a double page
	numpages=nnp;
	
	int npgs=GetPagesNeeded(numpapers);
	 // Grab PageLayout spread and modify to paper by doing tiling(?!?!?!?!) and printer marks
	if (whichpaper%2==1) { // odd numbered pages are always on left...
		 // make right and left side be correct page number
		spread->pagestack.e[0]->index=whichpaper; 
		spread->pagestack.e[1]->index=npgs-whichpaper-1; 
	} else {
		 // make right and left side be correct page number
		spread->pagestack.e[0]->index=npgs-whichpaper-1;
		spread->pagestack.e[1]->index=whichpaper;
	}
	if (spread->pagestack.e[0]->index>=numpages) spread->pagestack.remove(0);
	if (spread->pagestack.e[spread->pagestack.n-1]->index>=numpages) spread->pagestack.remove();
	//*** add printer marks.....
	//*** add tiling!!!!
	
	 // define max/min points
	spread->minimum=flatpoint(paperstyle->w()/5,paperstyle->h()/2);
	spread->maximum=flatpoint(paperstyle->w()*4/5,paperstyle->h()/2);

	return spread;
}

/*! The portion allotted to the pages is the paper minus the insets.
 * Then this portion is broken up into the different tiles. If there is a vertical
 * fold like a book, then the page width is further divided by 2. If there is a horizontal
 * fold, then the height is divided by 2.
 */
Laxkit::DoubleBBox *BookletDisposition::GetDefaultPageSize(Laxkit::DoubleBBox *bbox)//bbox=NULL
{ 
	if (!paperstyle) return NULL;
	if (!bbox) bbox=new DoubleBBox;
	bbox->minx=0;
	bbox->miny=0;
	double w=paperstyle->w()-insetl-insetr,
	       h=paperstyle->h()-insett-insetb;
	if (isvertical) {
		w/=tilex;
		h/=tilex*2;
	} else {
		w/=tilex*2;
		h/=tiley;
	}
	bbox->maxx=w;
	bbox->maxy=h;
	return bbox;
}

//! Returns { frompage,topage, singlepage,-1,anothersinglepage,-1,-2 }
int *BookletDisposition::PrintingPapers(int frompage,int topage)
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
int BookletDisposition::PaperFromPage(int pagenumber)
{
	if (pagenumber+1<=numpapers) return pagenumber;
	return numpapers-(pagenumber-numpapers)-1;
}

//! Is (int((npapers-1)/2)+1)*4. This assumes actually printing double sided later on..
/*! The actual number of physical papers when printed double sided is half numpapers.
 */
int BookletDisposition::GetPagesNeeded(int npapers)
{ return ((npapers-1)/2+1)*4; }

//! Get the number of papers needed to hold this many pages.  Is just (floor((npages-1)/4)+1)*2.
/*! The value for number of physical papers after printing out double sided is half the returned number.
 */
int BookletDisposition::GetPapersNeeded(int npages)
{ return ((npages-1)/4+1)*2; }


////---------------------------------- CompositeDisposition ----------------------------
//
// // class to enable certain page ranges to be administred by different dispositions...
//class CompositeDisposition : public Disposition
//{
// protected:
//	Disposition *disps;
//	Ranges ranges; 
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//}



////----------------------------------- BasicBook -------------------------------------------
///*! \class BasicBook
// * \brief A more general disposition geared more for books with several sections.
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
// * This disposition will automatically set up the cover paper to have the proper cut marks
// * to fit around the sections with the specified spine thickness. Specifically, this means
// * the vertical dimension of the coverpage will be the same as a body page, but the width
// * will be 2 pages plus the spine width.
// */
////************* should this be broken down into a Project, rather than complete Disposition????
////************* because it implies 2 different breakdowns of pages. Disposition should be one kind of breakdown
////class BasicBook : public Disposition
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
//	virtual void dump_out(FILE *f,int indent);
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
//void BasicBook::dump_out(FILE *f,int indent)
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
