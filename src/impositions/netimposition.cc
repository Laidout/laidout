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


#include "../language.h"
#include "netimposition.h"
#include "simplenet.h"
#include "box.h"
#include "stylemanager.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/transformmath.h>
#include <lax/refptrstack.cc>

 //built in nets:
 /*! \todo this should have a much more automated mechanism... */
#include "dodecahedron.h"
#include "box.h"

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
 * Each NetImposition holds any number of Net objects that can be used or
 * rearranged according to the actual imposition's requirements. Each Net
 * is (or at least can be) an instance of abstractnet if given, which contains
 * information about how a net's faces connect to other faces in an overall
 * spread. This is useful mostly for 3 dimensional objects that are unwrapped
 * into a flat net. Another parallel is to booklet spreads, where each page
 * wraps over onto adjacent pages.
 *
 * There can be any number of nets. They are usually derived directly from
 * a base AbstractNet. Each net can be active or inactive. When inactive, they
 * are accessible only from the imposition editor, and are ignored when it
 * comes to actually working with page data.
 *
 * \todo needs a little more work to keep track of kinds of pages, especially
 *   when creating getting pagestyles for them.. if pagestyle starts having
 *   outlines, this will become important
 * \todo the net imposition should be able to display pages as they look when
 *   folded over.. So a 3 page spread should be able to be previewed, and the paper
 *   must be able to be positioned independently of the net
 */
/*! \var int NetImposition::maptoabstractnet
 * \brief How document pages map to net faces.
 *
 * If maptoabstractnet==0, then abstract net faces map to net faces in blocks 
 * of n faces, where n is the sum of faces in each net that
 * is tagged as active. Say you have 3 active nets, containing 5, 7, and 9 
 * actual faces, a total of 21 faces. Document page 3 will map to the first net,
 * index 3. Document page 10 will map to the second net, with index (10-5)=5.
 * Document page index 21 will map to the first net again, with index 0.
 *
 * If maptoabstractnet!=0, then document pages map to abstractnet faces, and
 * net faces link back accordingly. This lets you have a polyhedron, say, that
 * has 100 faces, but you are only concerned with nets that have only some of those
 * faces. The proper document pages will be mapped to the proper places in the nets.
 * Beware that in this case, if your nets do not cover all the faces in an abstract 
 * net, then you might never see some of the document pages.
 */
/*! \var int NetImposition::printnet
 * \brief Whether to draw an outline of the net when exporting via PaperLayout.
 */


//! Constructor. Transfers newnet pointer, does not duplicate.
/*!  Default is to have a dodecahedron.
 *
 * If newnet is not NULL, then its count is incremented (in SetNet()).
 */
NetImposition::NetImposition(Net *newnet)
	: Imposition("Net")
{ 
	maptoabstractnet=0;
	pagestyle=NULL;
	netisbuiltin=0;
	abstractnet=NULL;
	printnet=1;

	 // setup default paperstyle
	PaperStyle *paperstyle=dynamic_cast<PaperStyle *>(stylemanager.FindStyle("defaultpapersize"));
	if (paperstyle) paperstyle=static_cast<PaperStyle *>(paperstyle->duplicate());
	else paperstyle=new PaperStyle("letter",8.5,11.0,0,300);
	Imposition::SetPaperSize(paperstyle);
	paperstyle->dec_count();

	DBG cerr <<"   net 1"<<endl;
	
	
	//***can styledef exist already? possible made by a superclass?
	styledef=stylemanager.FindDef("NetImposition");
	if (styledef) styledef->inc_count(); 
	else {
		styledef=makeStyleDef();
		if (styledef) stylemanager.AddStyleDef(styledef);
	}

//	if (!newnet) {
//		newnet=makeDodecahedronNet(paper->media.maxx,paper->media.maxy);
//		SetNet(newnet);
//		newnet->info=-1; //***-1 here means net is a built in net
//		newnet->dec_count();
//	} else {
//		SetNet(newnet);
//		newnet->info=0; //***0 here means net is not a built in net
//	}

	
	DBG cerr <<"imposition netimposition init"<<endl;
}
 
NetImposition::~NetImposition()
{
	if (abstractnet) abstractnet->dec_count();
	//nets.flush();
}

//! Sets net to a builtin net.
/*! Can be:
 * <pre>
 *  dodecahedron
 *  box  4 5 10
 * </pre>
 * box is special in that you can provide dimensions of the box. If no dimensions
 * are provided, then a cube is assumed.
 *
 * Returns 0 for success or nonzero error.
 */
int NetImposition::SetNet(const char *nettype)
{
	if (!strcmp(nettype,"dodecahedron")) {
		Net *newnet=makeDodecahedronNet(paper->media.maxx,paper->media.maxy); //1 count
		int c=SetNet(newnet); //adds a count
		newnet->dec_count(); //remove creation count
		netisbuiltin=1;
		return c;	
	} else if (!strcmp(nettype,"box")) {
		Net *newnet=makeBox(nettype,0,0,0); 
		int c=SetNet(newnet); //adds a count
		newnet->dec_count(); //remove creation count
		netisbuiltin=1;
		return c;	
	}
	return 1;
}

/*! Warning: transfers pointer, does not make duplicate.
 * Flush the nets stack, and inc_count() on the new net.
 *
 * Sets the net then fits to paper.
 *
 * Return 0 if changed, 1 if not.
 *
 * \todo what about when newnet is in the nets stack already?
 */
int NetImposition::SetNet(Net *newnet)
{
	if (!newnet) return 1;
	newnet->info=0;//***tag that says net is not built in?? used?
	nets.flush();
	nets.push(newnet);//adds a count
	
	if (abstractnet) abstractnet->dec_count();
	abstractnet=newnet->basenet;
	if (abstractnet) abstractnet->inc_count();

	 //// fit to page
	setPage();

	return 0;
}

//! Using a presumably new paper, scale the net to fit the new paper.
/*! Based on new paper also set up a default page style, if possible.
 *
 * NOTE: *** currently ignores page style... in fact does nothing at all
 *
 * \todo *** figure out if this is useful or not..
 */
void NetImposition::setPage()
{
	if (!nets.n) return;
//	SomeData page;
//	page.minx=paper->media.minx;
//	page.miny=paper->media.miny;
//	page.maxx=paper->media.maxx;
//	page.maxy=paper->media.maxy;
//	for (int c=0; c<nets.n; c++) nets.e[c]->FitToData(&page,page.maxx*.05);
}

//! The newfunc for NetImposition instances.
Style *NewNetImposition(StyleDef *def)
{ 
	NetImposition *n=new NetImposition;
	n->styledef=def;
	return n;
}

//! Return a new EnumStyle instance that has Dodecahedron listed.
/*! \todo this has to be automated....
 */
Style *CreateNetListEnum(StyleDef *sd)
{
	EnumStyle *e=new EnumStyle;
	e->add("Dodecahedron",0);
	e->add("Box",1);
	return e;
}

//! Make an instance of the NetImposition styledef.
/* ...
 */
StyleDef *NetImposition::makeStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,
			"NetImposition",
			_("Net"),
			_("Imposition of a fairly arbitrary net"),
			Element_Fields,
			NULL,NULL,
			NULL,
			0, //new flags
			NewNetImposition);

	sd->push("printnet",
			_("Print Net"),
			_("Whether to show the net outline when printing out a document."),
			Element_Boolean,
			NULL, "1",
			0,0);
	sd->push("net",
			_("Net"),
			_("What kind of net is the imposition using"),
			Element_Enum,
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
	if (d->nets.n) d->nets.flush();
	for (int c=0; c<nets.n; c++) {
		d->nets.push(nets.e[c]->duplicate(),1);
	}
		
	return s;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
int NetImposition::SetPaperSize(PaperStyle *npaper)
{
	if (Imposition::SetPaperSize(npaper)) return 1;
	setPage();
	return 0;
}

/*! Returns a new pagestyle for that page.
 *
 * \todo warning: potential cast problems with Imposition::GetPageOutline(), which
 *   returns a SomeData... should probably make that function return a path object of some kind.
 */
PageStyle *NetImposition::GetPageStyle(int pagenum,int local)
{
	PageStyle *ps=new PageStyle();
	ps->flags|=PAGESTYLE_AUTONOMOUS;
	ps->outline=dynamic_cast<PathsData *>(GetPageOutline(pagenum,0));
	ps->margin=ps->outline;	ps->outline->inc_count();
	return ps;
}

//! Return the number of faces making one set of nets.
/*! If maptoabstractnet==1 then this will be AbstractNet::NumFaces().
 *
 * Otherwise, and also if there is no abstract net,
 * then it is the sum of the number of faces in each active net that are tagged
 * with FACE_Actual. Note that there may be a ton of FACE_Potential faces
 * in the net.
 */
int NetImposition::numActiveFaces()
{
	if (maptoabstractnet==1 && abstractnet) return abstractnet->NumFaces();

	int n=0;
	for (int c=0; c<nets.n; c++) {
		if (!nets.e[c]->active) continue;
		for (int c2=0; c2<nets.e[c]->faces.n; c2++) 
			if (nets.e[c]->faces.e[c2]->tag==FACE_Actual) n++;
	}
	return n;
}

//! Return the sum of number of active nets.
/*! Note that if a net has only potential faces, it still counts as an active net.
 * Really, nets should never have only potential faces.
 */
int NetImposition::numActiveNets()
{
	int n=0;
	for (int c=0; c<nets.n; c++) if (nets.e[c]->active) n++;
	return n;
}

/*! \fn Page **NetImposition::CreatePages()
 * \brief Create the required pages, with appropriate PageStyle objects attached.
 */
Page **NetImposition::CreatePages()
{
	if (!nets.n || numActiveFaces()==0) return NULL;

	int c=0;
	Page **pages=new Page*[numpages+1]; //assumes numpages set properly
	PageStyle *ps;
	for (c=0; c<numpages; c++) {
		 // pagestyle is passed to Page where its count is inc'd..
		ps=GetPageStyle(c,0); //count of 1
		pages[c]=new Page(ps,0,c);//adds count to ps
		ps->dec_count(); //remove extra count
	}

	pages[c]=NULL;
	return pages;
}


//! Return outline of page in paper coords. Origin is page origin.
/*!
 * This is a no frills outline, used primarily to check where the mouse
 * is clicked down on.
 * If local==1 then return a new local SomeData. Otherwise return a
 * counted object. In this case, the item should be guaranteed to have
 * a reference count tick that refers to the returned pointer.
 *
 * This currently always creates a new PathsData. In the future when
 * pagetype is implemented more effectively, there will likely be a few outlines
 * laying around, and a link to them would be returned.
 *
 * NOTE: The correct matrix for placement in a net is NOT returned with the path.
 */
LaxInterfaces::SomeData *NetImposition::GetPageOutline(int pagenum,int local)
{//**********************************
	if (!nets.n) return NULL;
	//if (!doc || pagenum<0 || pagenum>=doc->pages.n) return NULL; ***
	if (pagenum<0 || pagenum>=numpages) return NULL;

	int isbez=0,
		n=0;
	flatpoint *pts=NULL;
	NetFace *face=NULL;
	if (maptoabstractnet && abstractnet) {
		 //existing nets may or may not have the face defined, so it is safer,
		 //though sometimes slower, to grab from the abstract net
		face=abstractnet->GetFace(pagenum%abstractnet->NumFaces());
		isbez=face->getOutline(&n,&pts,0);
		delete face;
	} else {
		 //the face is in a net, page numbers are mapped by active nets and FACE_Actual faces

		 //map document page number to a particular net and net face index
		int neti, netfacei;
		netfacei=pagenum%numActiveFaces();
		for (neti=0; neti<nets.n; neti++) {
			if (nets.e[neti]->active) {
				if (netfacei-nets.e[neti]->faces.n<0) break;
				netfacei-=nets.e[neti]->faces.n;
			}
		}
		DBG if (neti==nets.n) cerr <<"*** Bad news: page index not mapped to any net face"<<endl;

		isbez=nets.e[neti]->faces.e[netfacei]->getOutline(&n,&pts,0);
	}

	PathsData *newpath=new PathsData(); //count==1
	unsigned long flag=(isbez==2 ? POINT_TONEXT : POINT_VERTEX);
	for (int c=0; c<n; c++) {
		newpath->append(pts[c].x,pts[c].y,flag);
		if (isbez==2) {
			if (flag==POINT_TONEXT) flag=POINT_VERTEX;
			else if (flag==POINT_VERTEX) flag=POINT_TOPREV;
			else flag=POINT_TONEXT;
		}
	}
	newpath->close();
	newpath->FindBBox();
	delete[] pts;

	return newpath;
}


//------------------- Layout generating functions


//! Added "Singles with Adjacent" to layout types.
Spread *NetImposition::Layout(int layout,int which)
{
	if (layout==SINGLES_WITH_ADJACENT_LAYOUT) return SingleLayoutWithAdjacent(which);
	return Imposition::Layout(layout,which);
}

//! Added "Singles With Adjacent" to layout types.
int NetImposition::NumLayouts()
{ return 4; }

//! Added "Singles With Adjacent" to layout types.
const char *NetImposition::LayoutName(int layout)
{
	if (layout==SINGLES_WITH_ADJACENT_LAYOUT) return _("Singles With Adjacent");
	return Imposition::LayoutName(layout);
}


//! Return spread corresponding to document page whichpage.
Spread *NetImposition::SingleLayout(int whichpage)
{
	return Imposition::SingleLayout(whichpage);
}

/*! \fn Spread *NetImposition::PageLayout(int whichspread)
 * \brief Returns a page view spread that contains whichspread, in viewer coords.
 *
 * whichpage starts at 0.
 * Derived classes must fill the spread with a path, and the PageLocation stack.
 * The path holds the outline of the spread, and the PageLocation stack holds
 * transforms to get from the overall coords to each page's coords.
 *
 * \todo if no abstractnet, should unwrap based on an existing net as possible.
 *   currently in that case just returns SingleLayout()
 */
Spread *NetImposition::SingleLayoutWithAdjacent(int whichpage)
{
	if (!abstractnet) return SingleLayout(whichpage);
	
	 //start new net from the given page, first map the page
	int netpage=whichpage%abstractnet->NumFaces();
	Net newnet;
	newnet.Basenet(abstractnet);
	newnet.Anchor(netpage);
	
	 //unwrap all edges adjacent to that.
	newnet.Unwrap(0,-1);

	Spread *spread=GenerateSpread(NULL, &newnet,
								  (whichpage/numActiveFaces())*numActiveFaces());
	spread->style=SINGLES_WITH_ADJACENT_LAYOUT;
	
	return spread;
}

//! Append to or create a new spread using the provided net.
/*! \todo warning: potential cast problems with PageLocation::outline...
 *  \todo max and mxn points are messed up
 */
Spread *NetImposition::GenerateSpread(Spread *spread, Net *net, int pageoffset)
{
	DBG cerr <<"-- NetImposition::GenerateSpread--"<<endl;

	if (!spread) spread=new Spread();
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	 // fill pagestack
	PathsData *spreadpath=dynamic_cast<PathsData *>(spread->path);
	DBG if (!spreadpath && spread->path) cerr <<"**** error!!! wrong type for net spread path!"<<endl;
	if (!spreadpath) { 
		spreadpath=new PathsData;
		spread->path=spreadpath;
	}

	 // build lines...
	NetLine *l=NULL;
	Path *path=NULL;
	for (int c=0; c<net->lines.n; c++) {
		l=new NetLine;
		*l=*net->lines.e[c];
		path=new Path(l->points,l->linestyle,0);
		l->points=NULL;
		delete l;
		spreadpath->paths.push(path,1);
	}
	spreadpath->FindBBox();

	 //construct faces
	int page;
	PathsData *newpath=NULL;
	int n, isbez;
	flatpoint *p=NULL;
	NetFace *netface;
	unsigned long flag;
	for (int c=0; c<net->faces.n; c++) {
		netface=net->faces.e[c];
		if (netface->tag!=FACE_Actual) continue;

		page=pageoffset+net->faces.e[c]->original;
		if (page>=numpages) page=-1;

		newpath=new PathsData;
		isbez=netface->getOutline(&n, &p, 0);
		flag=(isbez?POINT_TONEXT:POINT_VERTEX);
		for (int c2=0; c2<n; c2++) {
			newpath->append(p[c2].x,p[c2].y,flag);
			if (isbez==2) {
				if (flag==POINT_TONEXT) flag=POINT_VERTEX;
				else if (flag==POINT_VERTEX) flag=POINT_TOPREV;
				else flag=POINT_TONEXT;
			}
		}
		newpath->close();
		newpath->FindBBox();
		delete[] p;

		if (netface->matrix) transform_copy(newpath->m(), netface->matrix);
		spread->pagestack.push(new PageLocation(page,NULL,newpath)); //incs newpath count
		newpath->dec_count();//remove extra count
	}


	 // define max/min points
	if (spread->pagestack.n) newpath=dynamic_cast<PathsData *>(spread->pagestack.e[0]->outline);
		else newpath=dynamic_cast<PathsData *>(spread->path);
	spread->minimum=transform_point(newpath->m(),
			flatpoint(newpath->minx,newpath->miny+(newpath->maxy-newpath->miny)/2));
	spread->maximum=transform_point(newpath->m(),
			flatpoint(newpath->maxx,newpath->miny+(newpath->maxy-newpath->miny)/2));

	return spread;
}

/*! \fn Spread *NetImposition::PageLayout(int whichspread)
 * \brief Returns a page view spread that contains whichspread, in viewer coords.
 *
 * All the active nets are piled on top of each other to make a single page layout
 * spread.
 */
Spread *NetImposition::PageLayout(int whichspread)
{
	if (!nets.n) return NULL;
	if (numActiveNets()==0) return NULL;

	int c,
		numactive=numActiveFaces();

	Spread *spread=NULL;
	for (c=0; c<nets.n; c++) {
		if (nets.e[c]->active) {
			spread=GenerateSpread(spread,nets.e[c],whichspread*numactive);
		}
	}

	spread->style=PAGELAYOUT;
	
	return spread;

}

//! Returns same as PageLayout, but with the papergroup slapped on as well.
/*! \todo really, could narrow down so that each paper of a papergroup gets
 *    its own paper spread?
 */
Spread *NetImposition::PaperLayout(int whichpaper)
{
	if (!nets.n) return NULL;

	Spread *spread=PageLayout(whichpaper);
	spread->style=SPREAD_PAPER;

	PathsData *path=static_cast<PathsData *>(spread->path);//this was a non-local PathsData obj

	 // put a reference to the outline in marks if printnet
	if (printnet) {
		spread->mask|=SPREAD_PRINTERMARKS;
		spread->marks=path;
		path->inc_count();
	}
	
	if (papergroup) {
		 //there is no need to actually draw the paper, since that is done by the viewer code
		spread->papergroup=papergroup;
		spread->papergroup->inc_count();
	}

	return spread;
}

//! Redefined to also return for SINGLES_WITH_ADJACENT_LAYOUT.
/*! SINGLES_WITH_ADJACENT_LAYOUT returns the same as SINGLESLAYOUT.
 */
int NetImposition::NumSpreads(int layout)
{
	if (layout==SINGLES_WITH_ADJACENT_LAYOUT) return NumSpreads(SINGLELAYOUT);
	return Imposition::NumSpreads(layout);
}

//! Returns pagenumber/numActiveFaces().
int NetImposition::PaperFromPage(int pagenumber)
{
	int n=numActiveFaces();
	if (!n) return 0;
	return pagenumber/n;
}

int NetImposition::SpreadFromPage(int layout, int pagenumber)
{
	if (layout==SINGLELAYOUT) return pagenumber;
	if (layout==PAGELAYOUT) return SpreadFromPage(pagenumber);
	if (layout==PAPERLAYOUT) return PaperFromPage(pagenumber);
	return pagenumber; //SINGLES_WITH_ADJACENT_LAYOUT
}

//! Returns pagenumber/numActiveFaces().
int NetImposition::SpreadFromPage(int pagenumber)
{
	int n=numActiveFaces();
	if (!n) return 0;
	return pagenumber/n;
}


//! From this many papers, how many pages are needed? 
/*! Returns npapers*net->nf.
 */
int NetImposition::GetPagesNeeded(int npapers)
{
	return npapers*numActiveFaces();
}

//! How many papers are needed to contain this many pages?
/*! Returns (npages-1)/numActiveFaces()+1.
 */
int NetImposition::GetPapersNeeded(int npages)
{
	return (npages-1)/numActiveFaces()+1;
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
 *
 * \todo is this function useful any longer?
 */
int *NetImposition::PrintingPapers(int frompage,int topage)
{//***
	return NULL;
//	int fp,tp;
//	if (topage<frompage) { tp=topage; topage=frompage; frompage=tp; }
//	fp=frompage/nets.e[curnet]->faces.n;
//	tp=topage/nets.e[curnet]->faces.n;
//	if (fp==tp) {
//		int *i=new int[3];
//		i[0]=fp;
//		i[1]=-1;
//		i[2]=-2;
//		return i;
//	}
//	int *i=new int[3];
//	i[0]=fp;
//	i[1]=tp;
//	i[2]=-2;
//	return i;
}

//! Bit of a cop-out currently, just returns page%numActiveFaces().
/*! \todo *** need more redundancy checking among the faces somehow...
 *     for archimedean solids, for instance, there will be 2 or 3
 *     different kinds of faces...
 */
int NetImposition::PageType(int page)
{
	if (numActiveFaces()) return page%numActiveFaces();
	return -1;
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
 *
 * \todo *** dump_out what==-1 with list of built in net types
 */
void NetImposition::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%snumpages 3      #number of pages in the document. This is ignored on readin\n",spc);
		fprintf(f,"%sprintnet yes     #whether the net lines get printed out with the page data\n",spc);
		fprintf(f,"%ssimplenet         #this is the same as using: abstractnet simple\n",spc);
		fprintf(f,"%s                  #It is a basic net definition roughly equivalent to an OFF file\n",spc);
		fprintf(f,"%sabstractnet type  #type can be \"file\" or \"Polyhedron\" or \"simple\".\n",spc);
		fprintf(f,"%sabstractnet file  #this block demonstrates abstract nets based on files.\n",spc);
		fprintf(f,"%s  filename /path/to/it  #This is used when the abstract net has not been\n",spc);
		fprintf(f,"%s                        #modified since being loaded from the file.\n",spc);
		fprintf(f,"%sabstractnet Polyhedron  #this block demonstrates Polyhedron abstract nets.\n",spc);
		Polyhedron p;
		p.dump_out(f,indent+2,-1,NULL);//**** if abstract nets get automated, this must change...

		fprintf(f,"%snet NetType   #There can be none or more nets. If there is no abstractnet, then\n",spc);
		fprintf(f,"%s              #there must be one or more nets. Nets are usually different arrangements\n",spc);
		fprintf(f,"%s              #of an abstract net.\n",spc);
		if (nets.n) nets.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			Net n;
			n.dump_out(f,indent+2,-1,NULL);
		}
		//fprintf(f,"%sdefaultpaperstyle #default paper style\n",spc);
		//***paperstyle->dump_out(f,indent+2,-1,NULL);
		return;
	}


	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
//	if (paper) { ***
//		fprintf(f,"%sdefaultpaperstyle\n",spc);
//		paperstyle->dump_out(f,indent+2,0,context);
//	}
	if (printnet) fprintf(f,"%sprintnet\n",spc);
		else fprintf(f,"%sprintnet false\n",spc);

	if (abstractnet) {
		if (!strcmp(abstractnet->whattype(),"SimpleNet")) {
			fprintf(f,"%ssimplenet\n",spc);
			abstractnet->dump_out(f,indent+2,0,context);
		} else if (!strcmp(abstractnet->whattype(),"Polyhedron") && abstractnet->Modified()) {
			fprintf(f,"%sabstractnet Polyhedron\n",spc);
			abstractnet->dump_out(f,indent+2,0,context);
		} else if (abstractnet->Filename()) {
			fprintf(f,"%sabstractnet file\n",spc);
			//fprintf(f,"%s  filetype %s\n",spc,abstractnet->Filetype());
			fprintf(f,"%s  filename %s\n",spc,abstractnet->Filename());
		}
	}

	if (nets.n) {
		for (int c=0; c<nets.n; c++) {
			fprintf(f,"%snet",spc);

			if (nets.e[c]->info==-1) fprintf(f," %s\n",nets.e[c]->whatshape());
			else {
				fprintf(f,"\n");
				nets.e[c]->dump_out(f,indent+2,0,context); // dump out a custom net
			}
		}
	}
}

/*! Clear the imposition, then read in a new one.
 *
 * \todo implement clear()!! Currently assumes that it is blank.
 */
void NetImposition::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
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
			//*** ignoring defaultpaperstyle.....
			//***if (paperstyle) delete paperstyle;
			//paperstyle=new PaperStyle("Letter",8.5,11,0,300);//***
			//paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
		} else if (!strcmp(name,"net")) {
			if (!isblank(value)) { // is a built in
				SetNet(value);
			} else {
				tempnet=new Net();
				tempnet->dump_in_atts(att->attributes.e[c],flag,context);
				SetNet(tempnet);
				tempnet->dec_count();
			}
		} else if (!strcmp(name,"printnet")) {
			printnet=BooleanAttribute(value);
		} else if (!strcmp(name,"simplenet")
					|| (!strcmp(name,"abstractnet") && value && !strcmp(value,"simple"))) {
			if (abstractnet) abstractnet->dec_count();
			abstractnet=new SimpleNet();
			abstractnet->dump_in_atts(att->attributes.e[c],flag,context);
			maptoabstractnet=1;
		} else if (!strcmp(name,"abstractnet")) {
			 // read in abstract net based on value type
			if (!strcmp(value,"file")) {
				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
					name= att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;
					if (strcmp(name,"filename")) continue;
					if (isblank(value)) continue;
					if (abstractnet) abstractnet->dec_count();
					abstractnet=AbstractNetFromFile(value);
				}
			} else if (!strcmp(value,"Polyhedron")) {
				Polyhedron *poly=new Polyhedron;
				poly->dump_in_atts(att->attributes.e[c],flag,context);
				if (abstractnet) abstractnet->dec_count();
				abstractnet=poly;
			}
		}
	}
	setPage();
}

//! From a file, try to read in an abstract net.
/*! Currently this can only be a simple net, or a polyhedron.
 *
 * \todo *** finish me!!
 */
AbstractNet *NetImposition::AbstractNetFromFile(const char *filename)
{
	Polyhedron *poly=new Polyhedron;
	if (poly->dumpInFile(filename,NULL)==0) {
		 //successfully loaded a polyhedron
		return poly;
	}
	delete poly;

	//***try to load a simple net...
	return NULL;
}
