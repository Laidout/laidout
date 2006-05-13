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
/**************** impositions/netimposition.cc *********************/

#include "netimposition.h"
#include "dodecahedron.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/refcounter.h>
#include <lax/transformmath.h>

using namespace Laxkit;
using namespace LaxInterfaces;
using namespace LaxFiles;
 
#include <iostream>
using namespace std;
#define DBG 

extern RefCounter<anObject> objectstack;

//----------------------------- NetImposition --------------------------

/*! \class NetImposition
 * \brief Uses a Net object to define the pages and spread.
 *
 * The PaperLayout() returns the same as the PageLayout, with the important
 * addition of the extra matrix. This matrix can be adjusted by the user
 * to potentially make the net blow up way beyond the paper size, so as
 * to make tiles of it.
 *
 * The net's faces should be defined such that the inside is to the left 
 * as you traverse the points, and when you draw on the normal math plane
 * which has positive x to the right, and positive y is up.
 */
//class NetImposition : public Imposition
//{
// public:
//	Net *net;
//	int netisbuiltin;
//	int printnet;
//
//	NetImposition(Net *newnet=NULL);
//	virtual ~NetImposition();
//	virtual Style *duplicate(Style *s=NULL);
//	
////	virtual int SetPageLikeThis(PageStyle *npage); // copies pagestyle, doesnt transfer pointer
//	virtual int SetPaperSize(PaperType *npaper); // set paperstyle, and compute page size
////	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
//	
//	virtual LaxInterfaces::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
//	virtual Page **CreatePages(PageStyle *thispagestyle=NULL); // create necessary pages based on default pagestyle
////	virtual int SyncPages(Document *doc,int start,int n);
//
//	virtual LaxInterfaces::SomeData *GetPage(int pagenum,int local); // return outline of page in page coords
//
//	virtual Spread *GetLittleSpread(int whichpage); 
////	virtual Spread *SingleLayout(int whichpage); 
//	virtual Spread *PageLayout(int whichpage); 
//	virtual Spread *PaperLayout(int whichpaper);
//	virtual DoubleBBox *GetDefaultPageSize(DoubleBBox *bbox=NULL);
//	virtual int *PrintingPapers(int frompage,int topage);
//
////	virtual int NumPapers(int npapers);
////	virtual int NumPages(int npages);
//	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
//
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//	
//	virtual int SetNet(const char *nettype);
//	virtual int SetNet(Net *newnet);
//	virtual void setPage();
//};

//! Constructor. Transfers newnet pointer, does not duplicate.
/*!  Default is to have a dodecahedron.
 */
NetImposition::NetImposition(Net *newnet)
	: Imposition("Net")
{ 
	 // setup default paperstyle and pagestyle
	paperstyle=new PaperType("letter",8.5,11.0,0,300);//***should be global default
	
	net=NULL;
	if (!newnet) {
		SetNet(makeDodecahedronNet(paperstyle->w(),paperstyle->h()));
		netisbuiltin=1; //this line must be after SetNet
	} else {
		SetNet(newnet);
		netisbuiltin=0;
	}

	printnet=1;
}
 
//! Deletes net.
NetImposition::~NetImposition()
{
	if (net) delete net;
}

//! Sets net to a builtin net.
/*! Can be:
 * <pre>
 *  dodecahedron
 * </pre>
 */
int NetImposition::SetNet(const char *nettype)
{
	if (!strcmp(nettype,"dodecahedron")) {
		SetNet(makeDodecahedronNet(paperstyle->w(),paperstyle->h()));
		netisbuiltin=1;
		return 0;	
	}
	return 1;
}


/*! Warning: transfers pointer, does not make duplicate.
 * Deletes old net.
 *
 * Return 0 if changed, 1 if not.
 */
int NetImposition::SetNet(Net *newnet)
{
	if (!newnet) return 1;
	if (net && net!=newnet) delete net;
	net=newnet;
	netisbuiltin=0;
	
	 // fit to page
	SomeData page;
	page.maxx=paperstyle->w();
	page.maxy=paperstyle->h();
	net->FitToData(&page,page.maxx*.05);
	setPage();

	return 0;
}

//! Using the paperstyle, create a new default pagestyle.
void NetImposition::setPage()
{
	if (!paperstyle) return;
	if (pagestyle) delete pagestyle;
	pagestyle=new PageStyle();
	if (!net || !net->nf) return;

	 //find generic bounds of a face
	 //  find transform from net point to paper face coords
	DoubleBBox b;
	double m[6],mm[6];

	net->basisOfFace(0,m,0); // so m: (face) -> (net)
	transform_invert(mm,m);  //   mm: (net) -> (face)

	NetFace *f=&net->faces[0];
	for (int c=0; c<f->np; c++) {
		b.addtobounds(transform_point(mm,net->points[f->points[c]]));
	}
	pagestyle->width= b.maxx-b.minx;
	pagestyle->height=b.maxy-b.miny;
}

/*! Output:
 * <pre>
 *  numpages 4
 *  defaultpaperstyle
 *    ...
 *  net Dodecahedron # <-- this is type, not instance name of net
 *    name "A dodecahedron"
 * </pre>
 * See also Net::dump_out().
 */
void NetImposition::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
	if (paperstyle) {
		fprintf(f,"%sdefaultpaperstyle\n",spc);
		paperstyle->dump_out(f,indent+2,0);
	}
	if (printnet) fprintf(f,"%sprintnet\n",spc);
		else fprintf(f,"%sprintnet false\n",spc);
	if (net) {
		fprintf(f,"%snet",spc);
		if (netisbuiltin) fprintf(f," %s\n",net->whatshape());
		else {
			fprintf(f,"\n");
			net->dump_out(f,indent+2,0); // dump out a custom net
		}
	}
}

void NetImposition::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	char *name,*value;
	Net *tempnet=NULL;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"numpages")) {
			IntAttribute(value,&numpages);
			if (numpages<0) numpages=0;
		} else if (!strcmp(name,"defaultpaperstyle")) {
			if (paperstyle) delete paperstyle;
			paperstyle=new PaperType("Letter",8.5,11,0,300);//***
			paperstyle->dump_in_atts(att->attributes.e[c]);
		} else if (!strcmp(name,"net")) {
			if (value && strcmp(value,"")) { // is a built in
				SetNet(value);
			} else {
				tempnet=new Net();
				tempnet->dump_in_atts(att->attributes.e[c]);
				SetNet(tempnet);
				//DBG cout <<"-----------after dump_in net and set----------"<<endl;
				//DBG net->dump_out(stdout,2,0);
				//DBG cout <<"-----------end netimpos..----------"<<endl;
			}
		} else if (!strcmp(name,"printnet")) {
			printnet=BooleanAttribute(value);
		}
	}
	setPage();
}

//! Just makes sure that s can be cast to NetImposition. If yes, return s, if no, return NULL.
Style *NetImposition::duplicate(Style *s)//s=NULL
{
	NetImposition *d;
	if (s==NULL) s=d=new NetImposition();
	else d=dynamic_cast<NetImposition *>(s);
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
Laxkit::DoubleBBox *NetImposition::GoodWorkspaceSize(int page,Laxkit::DoubleBBox *bbox)//page=1
{
	if (page==0 || page==1) {
		if (!bbox) bbox=new DoubleBBox();
		bbox->setbounds(-paperstyle->width,2*paperstyle->width,-paperstyle->height,2*paperstyle->height);
	} else return NULL;
	return bbox;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
int NetImposition::SetPaperSize(PaperType *npaper)
{
	if (!npaper) return 1;
	if (paperstyle) delete paperstyle;
	paperstyle=(PaperType *)npaper->duplicate();
	
	setPage();
	return 0;
}

/*! \fn Page **NetImposition::CreatePages(PageStyle *thispagestyle=NULL)
 * \brief Create the required pages.
 *
 * If thispagestyle is not NULL, then this style is to be preferred over
 * the internal page style(?!!?!***)
 */
Page **NetImposition::CreatePages(PageStyle *thispagestyle)//pagestyle=NULL
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


/*! \fn SomeData *NetImposition::GetPage(int pagenum,int local)
 * \brief Return outline of page in paper coords. Origin is page origin.
 *
 * This is a no frills outline, used primarily to check where the mouse
 * is clicked down on.
 * If local==1 then return a new local SomeData. Otherwise return a
 * counted object. In this case, the item should be guaranteed to have
 * a reference count of one that refers to the returned pointer.
 */
LaxInterfaces::SomeData *NetImposition::GetPage(int pagenum,int local)
{
	if (pagenum<0 || pagenum>=numpages) return NULL;
	int pg=pagenum%net->nf;
	PathsData *newpath=new PathsData(); //count==1
	
	 // find transform from paper coords to page coords 
	 //  (this here is specifically (paper face coords)->(paper),
	 //   which is slightly different from real page coords. Assumption here
	 //   is that page lengths are the same as paper lengths)
	double m[6],mm[6];
	net->basisOfFace(pg,m,1);

	transform_copy(newpath->m(),m); // newpath->m: (face) -> (paper)
	
	transform_invert(m,newpath->m()); // m: (paper) -> (face)
	transform_mult(mm,net->m(),m); // so this is (net point)->(paper)->(paper face)
	
	flatpoint p[net->faces[pg].np];
	//DBG cout <<"NetImposition::GetPage:\n";
	for (int c=0; c<net->faces[pg].np; c++) {
		p[c]=transform_point(mm,net->points[net->faces[pg].points[c]]);
		//DBG cout <<"  p"<<c<<": "<<p[c].x<<","<<p[c].y<<endl;
	}
	for (int c=0; c<net->faces[pg].np; c++) newpath->append(p[c].x,p[c].y);
	newpath->close();
	newpath->FindBBox();

	return newpath;
}

//! Just returns PageLayout(whichpage).
Spread *NetImposition::GetLittleSpread(int whichpage)
{ return PageLayout(whichpage); }


/*! \fn Spread *NetImposition::PageLayout(int whichpage)
 * \brief Returns a page view spread that contains whichpage, in viewer coords.
 *
 * whichpage starts at 0.
 * Derived classes must fill the spread with a path, and the PageLocation stack.
 * The path holds the outline of the spread, and the PageLocation stack holds
 * transforms to get from the overall coords to each page's coords.
 */
Spread *NetImposition::PageLayout(int whichpage)
{
	if (!net) return NULL;
	
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	int firstpage=(whichpage/net->nf)*net->nf;

	 // fill pagestack
	SomeData *newpath;
	int c;
	for (int c=0; c<net->nf; c++) {
		if (firstpage+c>=numpages) break;
		newpath=GetPage(c,1); // transformed page, local copy
		spread->pagestack.push(new PageLocation(firstpage+c,NULL,newpath,1,NULL));
	}

	 // fill spread with page outline
	PathsData *npath=new PathsData();
	int c2;
	for (c=0; c<net->nl; c++) {
		npath->pushEmpty();
		for (c2=0; c2<net->lines[c].np; c2++) {
			npath->append(transform_point(net->m(),net->points[net->lines[c].points[c2]]));
			if (net->lines[c].isclosed) npath->close();
		}
	}
	npath->FindBBox();
	spread->path=npath;
	spread->pathislocal=1;
	
	 // define max/min points
	if (spread->pagestack.n) newpath=spread->pagestack.e[0]->outline;
		else newpath=npath;
	spread->minimum=transform_point(newpath->m(),
			flatpoint(newpath->minx,newpath->miny+(newpath->maxy-newpath->miny)/2));
	spread->maximum=transform_point(newpath->m(),
			flatpoint(newpath->maxx,newpath->miny+(newpath->maxy-newpath->miny)/2));

	return spread;
}

//! Returns same as PageLayout, but with the paper outline included.
/*! \todo *** spread->marks (the printer marks) should optionally hold
 * the outline of the net, according to value of printnet.
 */
Spread *NetImposition::PaperLayout(int whichpaper)
{
	if (!net) return NULL;
	Spread *spread=PageLayout(whichpaper*net->nf);
	spread->style=SPREAD_PAPER;

	 // make the paper outline
	PathsData *path=static_cast<PathsData *>(spread->path);//this was a local PathsData obj

	 // put a reference to the outline in marks if printnet
	if (printnet) {
		spread->mask|=SPREAD_PRINTERMARKS;
		spread->marks=path;
		spread->marksarelocal=0;
		//path->inc_count(); <-- shouldn't have to do this
	}
	
	Group *g=new Group;
	spread->path=static_cast<SomeData *>(g);
	PathsData *path2=new PathsData();
	g->push(path2,0);
	g->push(path,0);
	path->pushEmpty(0);
	path->appendRect(0,0,paperstyle->w(),paperstyle->h(),0);
	g->FindBBox();

	return spread;
}

//! Returns the bounding box in paper units for the default page size.
/*! 
 * If bbox is not NULL, then put the info in the supplied bbox. Otherwise
 * return a new DoubleBBox.
 *
 * The orientation of the box is determined internally to the NetImposition,
 * and accessed through the other functions here. 
 * minx==miny==0 which is the lower left corner of the page. This function
 * is useful mainly for speedy layout functions.
 *
 * \todo *** is this function actaully used anywhere? anyway it's broken here,
 * just returns bounds of paper.
 */
DoubleBBox *NetImposition::GetDefaultPageSize(Laxkit::DoubleBBox *bbox)//box=NULL
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
int *NetImposition::PrintingPapers(int frompage,int topage)
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
int NetImposition::PaperFromPage(int pagenumber)
{
	if (!net) return 0;
	return pagenumber/net->nf;
}

//! Returns npapers*net->nf.
int NetImposition::GetPagesNeeded(int npapers)
{
	if (!net) return 0;
	return npapers*net->nf;
}

//! Returns (npages-1)/net->nf+1.
int NetImposition::GetPapersNeeded(int npages)
{
	if (!net) return 0;
	return (npages-1)/net->nf+1;
}

