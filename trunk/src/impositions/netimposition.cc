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
#include "stylemanager.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/transformmath.h>

using namespace Laxkit;
using namespace LaxInterfaces;
using namespace LaxFiles;
 
#include <iostream>
using namespace std;
#define DBG 


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
 *
 * \todo needs a little more work to keep track of kinds of pages, especially
 *   when creating getting pagestyles for them.. if pagestyle starts having
 *   outlines, this will become important
 */


//! Constructor. Transfers newnet pointer, does not duplicate.
/*!  Default is to have a dodecahedron.
 *
 * If newnet is not NULL, then its count is incremented (in SetNet()).
 */
NetImposition::NetImposition(Net *newnet)
	: Imposition("Net")
{ 
	 // setup default paperstyle and pagestyle
	paperstyle=dynamic_cast<PaperStyle *>(stylemanager.FindStyle("defaultpapersize"));
	if (paperstyle) paperstyle=static_cast<PaperStyle *>(paperstyle->duplicate());
	else paperstyle=new PaperStyle("letter",8.5,11.0,0,300);
	
	net=NULL;
	pagestyle=NULL;
	if (!newnet) {
		SetNet(makeDodecahedronNet(paperstyle->w(),paperstyle->h()));
		netisbuiltin=1; //this line must be after SetNet
	} else {
		SetNet(newnet);
		netisbuiltin=0;
	}

	printnet=1;
	
	styledef=stylemanager.FindDef("NetImposition");
	if (styledef) styledef->inc_count(); 
	else {
		styledef=makeStyleDef();
		if (styledef) stylemanager.AddStyleDef(styledef);
	}
}
 
//! Dec_count() of the net, and pagestyle.
NetImposition::~NetImposition()
{
	if (net) net->dec_count();
	pagestyle->dec_count();
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
		Net *newnet=makeDodecahedronNet(paperstyle->w(),paperstyle->h()); //1 count
		SetNet(newnet); //adds a count
		newnet->dec_count(); //remove extra count
		netisbuiltin=1;
		return 0;	
	}
	return 1;
}

/*! Warning: transfers pointer, does not make duplicate.
 * Dec_count() on the old net, and inc_count() on the new net.
 *
 * NOTE that if newnet happens to be the same as the current net, its
 * count is NOT incremented.
 *
 * Return 0 if changed, 1 if not.
 */
int NetImposition::SetNet(Net *newnet)
{
	if (!newnet) return 1;
	if (net && net!=newnet) net->dec_count();
	if (net!=newnet) newnet->inc_count();
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
	if (pagestyle) pagestyle->dec_count();
	pagestyle=new PageStyle();
	pagestyle->pagetype=0;
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

//! The newfunc for Singles instances.
Style *NewNetImposition(StyleDef *def)
{ 
	NetImposition *n=new NetImposition;
	n->styledef=def;
	return n;
}

//! Return a new EnumStyle instance that has Dodecahedron listed.
Style *CreateNetListEnum(StyleDef *sd)
{
	EnumStyle *e=new EnumStyle;
	e->add("Dodecahedron",0);
	//e->add("Cube",1);
	return e;
}

//! Make an instance of the NetImposition imposition styledef.
/* ...
 */
StyleDef *NetImposition::makeStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,"NetImposition","Net",
			"Imposition of a fairly arbitrary net",
			NULL,
			Element_Fields,
			NULL,NULL,
			NULL,
			0, //new flags
			NewNetImposition);

	sd->push("printnet",
			"Print Net",
			"Whether to show the net outline when printing out a document.",
			NULL,
			Element_Boolean,
			NULL, "1",
			0,0);
	sd->push("net",
			"Net",
			"What kind of net is the imposition using",
			NULL,
			Element_DynamicEnum,
			NULL, "0",
			0,0,CreateNetListEnum);
	return sd;
}

//! Copy over net and whether it is builtin..
Style *NetImposition::duplicate(Style *s)//s=NULL
{
	NetImposition *d;
	if (s==NULL) s=d=new NetImposition();
	else d=dynamic_cast<NetImposition *>(s);
	if (!d) return NULL;
	
	 // copy net
	d->net=net->duplicate();
	d->netisbuiltin=netisbuiltin;
	d->setPage();
		
	return s;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
int NetImposition::SetPaperSize(PaperStyle *npaper)
{
	if (Imposition::SetPaperSize(npaper)) return 1;
	setPage();
	return 0;
}

/*! \todo *** fixme!! just returns whatever is in pagestyle, should be a special pagestyle for
 *    the face type...
 *
 * Local styles have count of 1, nonlocal increases the default's count by 1.
 */
PageStyle *NetImposition::GetPageStyle(int pagenum,int local)
{
	if (!pagestyle) return NULL;
	PageStyle *ps=NULL;
	if (local || PageType(pagenum)!=pagestyle->pagetype) {
		ps=(PageStyle *)pagestyle->duplicate();
		ps->pagetype=PageType(pagenum);
		ps->flags|=PAGESTYLE_AUTONOMOUS;
	} else {
		pagestyle->inc_count();
		ps=pagestyle;
	}
	return ps;
}


/*! \fn Page **NetImposition::CreatePages()
 * \brief Create the required pages.
 *
 * If thispagestyle is not NULL, then this style is to be preferred over
 * the internal page style(?!!?!***)
 *
 * \todo when pagestyle handling is ironed out more, this will have to be modified to
 *   not create local pagestyles.
 */
Page **NetImposition::CreatePages()
{
	if (numpages==0) return NULL;
	Page **pages=new Page*[numpages+1];
	int c;
	PageStyle *ps;
	for (c=0; c<numpages; c++) {
		 // pagestyle is passed to Page where its count is inc'd..
		ps=GetPageStyle(c,0); //count of 1
		pages[c]=new Page(ps,0,c);
		ps->dec_count(); //remove extra count
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
 * a reference count tick that refers to the returned pointer.
 *
 * This currently always creates a new PathsData. In the future when
 * pagetype is implemented more effectively, there will likely be a few outlines
 * laying around, and a link to them would be returned.
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
	DBG cout <<"NetImposition::GetPage:\n";
	for (int c=0; c<net->faces[pg].np; c++) {
		p[c]=transform_point(mm,net->points[net->faces[pg].points[c]]);
		DBG cout <<"  p"<<c<<": "<<p[c].x<<","<<p[c].y<<endl;
	}
	for (int c=0; c<net->faces[pg].np; c++) newpath->append(p[c].x,p[c].y);
	newpath->close();
	newpath->FindBBox();

	return newpath;
}


/*! \fn Spread *NetImposition::PageLayout(int whichspread)
 * \brief Returns a page view spread that contains whichspread, in viewer coords.
 *
 * whichpage starts at 0.
 * Derived classes must fill the spread with a path, and the PageLocation stack.
 * The path holds the outline of the spread, and the PageLocation stack holds
 * transforms to get from the overall coords to each page's coords.
 */
Spread *NetImposition::PageLayout(int whichspread)
{
	if (!net) return NULL;
	
	Spread *spread=new Spread();
	spread->style=SPREAD_PAGE;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	int firstpage=whichspread*net->nf;

	 // fill pagestack
	SomeData *newpath;
	int c;
	for (int c=0; c<net->nf; c++) {
		if (firstpage+c>=numpages) break;
		newpath=GetPage(c,0); // transformed page, count at least 1 more
		spread->pagestack.push(new PageLocation(firstpage+c,NULL,newpath,0)); //incs newpath count
		newpath->dec_count();//remove extra count
	}

	 // fill spread with net outline
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
	spread->pathislocal=0; //current npach count is 1, so this is ok
	
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
Spread *NetImposition::PaperLayout(int whichpaper)
{
	if (!net) return NULL;
	Spread *spread=PageLayout(whichpaper*net->nf);
	spread->style=SPREAD_PAPER;

	 // make the paper outline
	PathsData *path=static_cast<PathsData *>(spread->path);//this was a non-local PathsData obj

	 // put a reference to the outline in marks if printnet
	if (printnet) {
		spread->mask|=SPREAD_PRINTERMARKS;
		spread->marks=path;
		spread->marksarelocal=0;
		path->inc_count();
	}
	
	Group *g=new Group;
	spread->path=static_cast<SomeData *>(g);
	g->push(path,0);    //incs count: the net outline
	
	PathsData *path2=new PathsData();//the paper outline
	g->push(path2,0);   //incs count
	path2->dec_count(); //remove extra count
	path2->appendRect(0,0,paperstyle->w(),paperstyle->h(),0);

	g->FindBBox();

	return spread;
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

//! Same as GetPapersNeeded().
int NetImposition::GetSpreadsNeeded(int npages)
{
	return GetPapersNeeded(npages);
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

//! Bit of a cop-out currently, just returns page/net->nf.
/*! \todo *** need more redundancy checking among the faces somehow...
 */
int NetImposition::PageType(int page)
{
	return page/net->nf;
}

//! For now assuming only 1 spread type, which is same as paper spread. Returns 0.
int NetImposition::SpreadType(int spread)
{
	return 0;
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

void NetImposition::dump_in_atts(LaxFiles::Attribute *att,int flag)
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
			paperstyle=new PaperStyle("Letter",8.5,11,0,300);//***
			paperstyle->dump_in_atts(att->attributes.e[c],flag);
		} else if (!strcmp(name,"net")) {
			if (value && strcmp(value,"")) { // is a built in
				SetNet(value);
			} else {
				tempnet=new Net();
				tempnet->dump_in_atts(att->attributes.e[c],flag);
				SetNet(tempnet);
				DBG cout <<"-----------after dump_in net and set----------"<<endl;
				DBG net->dump_out(stdout,2,0);
				DBG cout <<"-----------end netimpos..----------"<<endl;
			}
		} else if (!strcmp(name,"printnet")) {
			printnet=BooleanAttribute(value);
		}
	}
	setPage();
}

