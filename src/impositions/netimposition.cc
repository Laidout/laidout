//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2013 by Tom Lechner
//


#include "../language.h"
#include "netimposition.h"
#include "simplenet.h"
#include "box.h"
#include "accordion.h"
#include "../core/stylemanager.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/transformmath.h>

 //built in nets:
 /*! \todo this should have a much more automated mechanism... */
#include "dodecahedron.h"
#include "box.h"

#include <lax/debug.h>

using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;
using namespace Polyptych;
 


namespace Laidout {


//----------------------------- NetImposition --------------------------

 //convenience defines for net->info
#define NETIMP_Internal          1
#define NETIMP_AlreadyScaled (1<<1)

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
 * This class interprets Net::info as follows. info&1 means that the net was
 * internally generated, so that when saving the file, it saves only the class
 * of imposition, perhaps with minimal other information, rather than the whole
 * imposition data. info&2 means that the net has already been scaled to
 * a paper size, so when a user changes the papergroup, the net will not
 * be scaled again, so as not to screw up all the positioning done with
 * the initial scaling.
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
/*! \var double NetImposition::scalefromnet
 * \brief Any final scaling factor from the net to spread coordinates.
 *
 * Abstract nets like polyhedra, nets, and spreads can all have their own scaling.
 * Usually abstract nets use the same distance as nets, but to scale nets onto
 * paper, it is convenient to have a single final scaling factor handy, 
 * and independent from any one Net or AbstractNet.
 */


NetImposition::NetImposition()
  : Imposition(_("NetImposition"))
{
	DBGL("imposition netimposition init")

	briefdesc        = nullptr;
	scalefromnet     = 1;
	maptoabstractnet = 0;
	pagestyle        = nullptr;
	netisbuiltin     = 0;
	abstractnet      = nullptr;
	printnet         = 1;
	line_fold        = nullptr;
	line_peak        = nullptr;
	line_valley      = nullptr;
	line_cut         = nullptr;
	line_soft_cut    = nullptr;
	line_debug       = nullptr;

	// setup default paperstyle
	PaperStyle *paperstyle = dynamic_cast<PaperStyle *>(stylemanager.FindDef("defaultpapersize"));
	if (paperstyle) paperstyle = static_cast<PaperStyle *>(paperstyle->duplicate());
	else paperstyle = new PaperStyle("letter",8.5,11.0,0,300,"in");
	Imposition::SetPaperSize(paperstyle);
	paperstyle->dec_count();

	objectdef = stylemanager.FindDef("NetImposition");
	if (objectdef) objectdef->inc_count(); 
	else {
		objectdef = makeObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef,0);
	}

	// set up default spread line styles
	line_fold = new LineStyle();
	line_fold->Colorf(.6,.6,.6,1.);
	line_fold->widthtype = 0;
	line_fold->width = 2; //screen px
	line_fold->Id("fold");
	double d[2] { .05,.05 };
	line_fold->Dashes(d, 2, 0);
	line_fold->use_dashes = true;

	line_peak = new LineStyle();
	line_peak->Colorf(.6,.6,.6,1.);
	line_peak->widthtype = 0;
	line_peak->width = 2; //screen px
	line_peak->Id("fold_peak");
	d[0] = .15; d[1] = .05;
	line_peak->Dashes(d, 2, 0);
	line_peak->use_dashes = true;

	line_valley = new LineStyle();
	line_valley->Colorf(.6,.6,.6,1.);
	line_valley->widthtype = 0;
	line_valley->width = 2; //screen px
	line_valley->Id("fold_valley");
	d[0] = .02; d[1] = .05;
	line_valley->Dashes(d, 2, 0);
	line_valley->use_dashes = true;

	line_cut = new LineStyle();
	line_cut->Colorf(1.,0.,0.,1.);
	line_cut->widthtype = 0;
	line_cut->width = 2; //screen px
	line_cut->Id("cut");

	line_soft_cut = new LineStyle();
	line_soft_cut->Colorf(0.,1.,0.,1.);
	line_soft_cut->widthtype = 0;
	line_soft_cut->width = 2; //screen px
	line_soft_cut->Id("soft_cut");

	line_debug = new LineStyle();
	line_debug->Colorf(1.,0.,1.,1.);
	line_debug->widthtype = 0;
	line_debug->width = 1; //screen px
	line_debug->Id("debug");
}

/*! This converts the papergroup into a BasicNet. papergroup itself is NOT stored or referenced anywhere.
 */
NetImposition::NetImposition(PaperGroup *papergroup)
  : NetImposition()
{
	if (!papergroup || !papergroup->papers.n) return;

	BasicNet *net = new BasicNet();
	for (int c=0; c<papergroup->papers.n; c++) {
		PaperBoxData *paper = papergroup->papers.e[c];

		NetFace *face = new NetFace();
		face->original = c;
		face->tag = FACE_Actual;
		net->push(face, 1);

		flatpoint p = paper->transformPoint(flatpoint(paper->minx, paper->miny));
		NetFaceEdge *edge = new NetFaceEdge();
		edge->points = new Coordinate(p);
		face->edges.push(edge, 1);

		p = paper->transformPoint(flatpoint(paper->maxx, paper->miny));
		edge = new NetFaceEdge();
		edge->points = new Coordinate(p);
		face->edges.push(edge, 1);

		p = paper->transformPoint(flatpoint(paper->maxx, paper->maxy));
		edge = new NetFaceEdge();
		edge->points = new Coordinate(p);
		face->edges.push(edge, 1);

		p = paper->transformPoint(flatpoint(paper->minx, paper->maxy));
		edge = new NetFaceEdge();
		edge->points = new Coordinate(p);
		face->edges.push(edge, 1);
	}

	Net *nnet = new Net();
	nnet->Basenet(net);
	net->dec_count();
	nnet->TotalUnwrap(true);
	nnet->rebuildLines();
	SetNet(nnet);
	nnet->dec_count();
}

//! Constructor. Transfers newnet pointer, does not duplicate.
/*!  Default is to have a dodecahedron.
 *
 * If newnet is not nullptr, then its count is incremented (in SetNet()).
 */
NetImposition::NetImposition(Net *newnet)
  : NetImposition()
{ 
	if (newnet) SetNet(newnet);
}

NetImposition::~NetImposition()
{
	if (briefdesc) delete[] briefdesc;
	if (abstractnet)   abstractnet  ->dec_count();
	if (line_fold)     line_fold    ->dec_count();
	if (line_peak)     line_peak    ->dec_count();
	if (line_valley)   line_valley  ->dec_count();
	if (line_cut)      line_cut     ->dec_count();
	if (line_soft_cut) line_soft_cut->dec_count();
	if (line_debug)    line_debug   ->dec_count();
}

ImpositionInterface *NetImposition::Interface()
{
	return nullptr;
}

//! Static imposition resource creation function.
ImpositionResource **NetImposition::getDefaultResources()
{
	ObjectDef *accordions = Accordion::GetPresets();
	int num_accordions = accordions->getNumEnumFields() - 1; // minus one to take out "unknown"

	ImpositionResource **r = new ImpositionResource*[3 + num_accordions];
	Attribute *att;
	
	att = new Attribute;
	att->push("net","cube");
	r[1] = new ImpositionResource("NetImposition",     //objectdef
								  _("Cube"),         //name
								  nullptr,              //file
								  _("6 squares"),
								  "Cube",
								  att,1);
	att = new Attribute;
	att->push("net","dodecahedron");
	r[0] = new ImpositionResource("NetImposition",     //objectdef
								  _("Dodecahedron"), //name
								  nullptr,              //file
								  _("12 pentagons"),
								  "Dodecahedron",
								  att,1);
	
	// add accordions
	for (int c = 0; c < num_accordions; c++) {
		ObjectDef *def = accordions->getField(c+1); //skip "unknown"
		int _id = 0;
		accordions->getEnumInfo(c+1, nullptr, nullptr, nullptr, &_id);
		Accordion::AccordionPresets id = (Accordion::AccordionPresets)_id;
		int p1 = 0, p2 = 0;
		int num_p = Accordion::NumParams(id, &p1, &p2);

		att = new Attribute();
		Attribute *att2 = att->pushSubAtt("generator", "accordion");
		att2->push("variant", def->name);
		Utf8String str("params=%d, p1:int=%d, p2:int=%d", num_p, p1, p2);
		att2->push("uihint", str.c_str());
		att2->push("p1", p1);
		att2->push("p2", p2);
		r[2 + c] = new ImpositionResource("NetImposition",     //objectdef
								  def->Name,
								  nullptr,
								  def->description,
								  def->name,
								  att,1);
	}

	r[2 + num_accordions] = nullptr;
	return r;
}

//! Return what type of thing this is.
/*! If there is an abstract net, then return abstractnet->NetName().
 * Otherwise, return "Net".
 */
const char *NetImposition::NetImpositionName()
{
	if (abstractnet && abstractnet->NetName()) return abstractnet->NetName();
	return _("Net");
}

	
void NetImposition::GetDefaultPaperDimensions(double *x, double *y)
{
	*x = papergroup->papers.e[0]->box->paperstyle->w();
	*y = papergroup->papers.e[0]->box->paperstyle->h();
}

void NetImposition::GetDefaultPageDimensions(double *x, double *y)
{
	PageStyle *p = GetPageStyle(0,1);
	*x = p->w();
	*y = p->h();
	p->dec_count();
}


//! Return something "Dodecahedron, 12 page net".
/*! If briefdesc!=nullptr, then return that. Otherwise make briefdesc
 * reflect teh current net info.
 */
const char *NetImposition::BriefDescription()
{
	if (briefdesc) return briefdesc;

	int n=numActiveFaces();
	delete[] briefdesc;
	briefdesc=new char[30+strlen(NetImpositionName())];
	sprintf(briefdesc,_("%s, %d face net"), NetImpositionName(), n);
	return briefdesc;
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
	if (!strcasecmp(nettype,"dodecahedron")) {
		Net *newnet=makeDodecahedronNet(paper->media.maxx,paper->media.maxy); //1 count
		int c=SetNet(newnet); //adds a count
		newnet->info|=NETIMP_Internal;
		newnet->dec_count(); //remove creation count
		netisbuiltin=1;
		makestr(briefdesc,_("Dodecahedron"));
		return c;

	} else if (strcasestr(nettype,"box")==nettype) {
		Net *newnet=makeBox(nettype,0,0,0); 
		int c=SetNet(newnet); //adds a count
		newnet->info|=NETIMP_Internal;
		newnet->dec_count(); //remove creation count
		netisbuiltin=1;
		makestr(briefdesc,newnet->basenet->NetName());
		return c;

	} else if (strcasestr(nettype,"cube")==nettype) {
		Net *newnet=makeBox(nettype,1,1,1);
		int c=SetNet(newnet); //adds a count
		newnet->info|=NETIMP_Internal;
		newnet->dec_count(); //remove creation count
		netisbuiltin=1;
		makestr(briefdesc,_("Cube"));
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
	newnet->info=0;//clears the info tag, means it is not internal, and needs to scale to fit paper
	nets.flush();
	nets.push(newnet);//adds a count
	
	if (abstractnet) abstractnet->dec_count();
	abstractnet=newnet->basenet;
	if (abstractnet) abstractnet->inc_count();

	 // fit to paper
	//newnet->rebuildLines();  <--shouldn't have to do this
	newnet->FindBBox();
	DBGL("NetImposition::SetNet()")
	setPage();

	return 0;
}

//! Using a presumably new paper, scale the net to fit the new paper.
/*! Based on new paper also set up a default page style, if possible.
 *
 * \todo *** figure out if this is useful or not..
 * \todo *** when there is more than one net, this scales them differently
 *   depending on how big they are... not a uniform scaling...
 */
void NetImposition::setPage()
{
	if (!nets.n) return;

	SomeData page;
	page.minx = paper->media.minx;
	page.miny = paper->media.miny;
	page.maxx = paper->media.maxx;
	page.maxy = paper->media.maxy;
	DBG cerr << "NetImposition paper minx,miny,maxx,maxy: "<<page.minx<<" "<<page.miny<<" "<<page.maxx<<" "<<page.maxy<<endl;
	DBG cerr << "   paper w,h: "<<paper->paperstyle->w()<<" "<<paper->paperstyle->h()<<endl;

	for (int c=0; c<nets.n; c++) {
		if (nets.e[c]->info & NETIMP_AlreadyScaled) continue;

		nets.e[c]->info |= NETIMP_AlreadyScaled;
		//double scalex = std::max();
		nets.e[c]->FitToData(&page, margin_is_percent ? (page.maxx-page.minx) * margin : margin, 1);
		scalefromnet = norm(nets.e[c]->xaxis());

		DBG cerr <<"new scalefromnet: "<<scalefromnet<<endl;

		DBG cerr <<"net "<<c<<" matrix: "<<nets.e[c]->m((int)0)<<", "
		DBG 	<<nets.e[c]->m(1)<<", "
		DBG 	<<nets.e[c]->m(2)<<", "
		DBG 	<<nets.e[c]->m(3)<<", "
		DBG 	<<nets.e[c]->m(4)<<", "
		DBG 	<<nets.e[c]->m(5)<<endl;
		DBG cerr <<"net "<<c<<" bounds xx-yy: "
		DBG 	<<nets.e[c]->minx<<", "
		DBG 	<<nets.e[c]->maxx<<", "
		DBG 	<<nets.e[c]->miny<<", "
		DBG 	<<nets.e[c]->maxy<<endl;
	}
}

//! The newfunc for NetImposition instances.
Value *NewNetImposition()
{ 
	NetImposition *n=new NetImposition;
	return n;
}

//! Make an instance of the NetImposition objectdef.
ObjectDef *makeNetImpositionObjectDef()
{
	ObjectDef *sd=new ObjectDef(nullptr,
			"NetImposition",
			_("Net"),
			_("Imposition of a fairly arbitrary net"),
			"class",
			nullptr,nullptr,
			nullptr,
			0, //new flags
			NewNetImposition);

	sd->push("printnet", _("Print Net"), _("Whether to show the net outline when printing out a document."),
			"boolean",
			nullptr, "1",
			0,0);

	sd->push("margin_is_percent", _("Margin is percent"), _("Whether the margin value is absolute (false) or a percent of paper width (true)."),
			"boolean",
			nullptr, "1",
			0,0);

	sd->push("margin", _("Margin"), _("Margin within which to place net."),
			"real",
			nullptr, ".05",
			0,0);

	sd->push("net", _("Net"),  _("What kind of net is the imposition using"),
			"enum",
			nullptr, "0",
			0,nullptr); // *** 0,0,CreateNetListEnum);
	DBG cerr << " *** need to make enum list work again in makeNetImpositionObjectDef()"<<endl;
	return sd;
}

ObjectDef *NetImposition::makeObjectDef()
{
	return makeNetImpositionObjectDef();
}

//! Copy over net and whether it is builtin..
Value *NetImposition::duplicate()
{
	NetImposition *d;
	d = new NetImposition();

	d->margin = margin;
	d->margin_is_percent = margin_is_percent;
	
	 // copy net
	if (d->nets.n) d->nets.flush();
	for (int c=0; c<nets.n; c++) {
		d->nets.push(nets.e[c]->duplicate(),1);
	}
		
	return d;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer transer.
int NetImposition::SetPaperSize(PaperStyle *npaper)
{
	DBG cerr << "NetImposition::SetPaperSize(PaperStyle *npaper)"<<endl;
	if (Imposition::SetPaperSize(npaper)) {
		DBG cerr <<"  ----> ret early"<<endl;
		return 1;
	}
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
	ps->flags |=PAGESTYLE_AUTONOMOUS;

	ps->outline=dynamic_cast<PathsData *>(GetPageOutline(pagenum,0));
	if (ps->outline) {
		ps->min_x  =ps->outline->minx;
		ps->min_y  =ps->outline->miny;
		ps->width  =ps->outline->maxx-ps->outline->minx;
		ps->height =ps->outline->maxy-ps->outline->miny;
	}
	ps->margin =ps->outline;	if (ps->outline) ps->outline->inc_count();

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


//! Return outline of page in page coords. Origin is page origin.
/*!
 * This is a no frills outline, used primarily to check where the mouse
 * is clicked down on.
 * If local==1 then return a new local SomeData, a duplicate, not a link.
 * Otherwise return a counted object which MAY be a link.
 * In either case, calling code must use dec_count() on the returned object.
 *
 * This currently always creates a new PathsData. In the future when
 * pagetype is implemented more effectively, there will likely be a few outlines
 * laying around, and a link to them would be returned.
 *
 * NOTE: The correct matrix for placement in a net is NOT returned with the path.
 * The path coordinates are in single page coordinates, and it has an identity matrix.
 */
LaxInterfaces::SomeData *NetImposition::GetPageOutline(int pagenum,int local)
{
	if (!nets.n) return nullptr;
	//if (!doc || pagenum<0 || pagenum>=doc->pages.n) return nullptr; ***
	if (pagenum<0 || pagenum>=numpages) return nullptr;

	int isbez=0;
	int n=0;
	flatpoint *pts=nullptr;
	if (maptoabstractnet && abstractnet) {
		 //existing nets may or may not have the face defined, so it is safer,
		 //though sometimes slower, to grab from the abstract net
		NetFace *face=abstractnet->GetFace(pagenum%abstractnet->NumFaces(),scalefromnet);
		isbez=face->getOutline(&n,&pts,0);
		delete face;

	} else {
		 //the face is in a net, page numbers are mapped by active nets and FACE_Actual faces

		 //map document page number to a particular net and net face index
		NetFace face;
		int neti, netfacei;
		netfacei=pagenum%numActiveFaces();
		for (neti=0; neti<nets.n; neti++) {
			if (nets.e[neti]->active) {
				if (netfacei-nets.e[neti]->faces.n<0) break;
				netfacei-=nets.e[neti]->faces.n;
			}
		}
		DBG if (neti==nets.n) cerr <<"*** Bad news: page index not mapped to any net face"<<endl;

		face=*nets.e[neti]->faces.e[netfacei];
		
		double scaling=norm(nets.e[neti]->xaxis()); //theoretically, this will be same as scalefromnet
		for (int c=0; c<face.edges.n; c++) {
			Coordinate *t,*coord=face.edges.e[c]->points;
			t=coord;
			do {
				t->x(scaling*t->x()); //to paper scale
				t->y(scaling*t->y()); //to paper scale
				t=t->next;
			} while (t && t!=coord);
		}

		isbez=face.getOutline(&n,&pts,0);
	}

	PathsData *newpath=new PathsData(); //count==1
	newpath->style |= PathsData::PATHS_Ignore_Weights;
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

//! Adds "Adjacent" to layout types.
int NetImposition::NumLayoutTypes()
{ return 4; }

//! Adds "Adjacent" to layout types.
const char *NetImposition::LayoutName(int layout)
{
	if (layout==SINGLES_WITH_ADJACENT_LAYOUT) return _("Adjacent");
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
	newnet.xaxis(newnet.xaxis()*scalefromnet);
	newnet.yaxis(newnet.yaxis()*scalefromnet);
	
	 //unwrap all edges adjacent to that.
	newnet.Unwrap(0,-1);

	Spread *spread=GenerateSpread(nullptr, &newnet,
								  (whichpage/numActiveFaces())*numActiveFaces());
	spread->style=SINGLES_WITH_ADJACENT_LAYOUT;
	
	return spread;
}

/*! Append to or create a new spread using the provided net.
 *
 *  \todo warning: potential cast problems with PageLocation::outline...
 *  \todo max and min points are messed up
 */
Spread *NetImposition::GenerateSpread(Spread *spread, //!< If not null, append to that one, else return new one.
									Net *net,
									int pageoffset) //!< Add this to any page indices in the new spread pages
{
	DBG cerr <<"-- NetImposition::GenerateSpread--"<<endl;
	DBG cerr <<"   net dump:"<<endl;
	DBG net->dump_out(stderr,0,0,nullptr);
	DBG cerr <<"-- end net dump"<<endl;

	if (!spread) spread = new Spread();
	spread->mask = SPREAD_PATH | SPREAD_PAGES | SPREAD_MINIMUM | SPREAD_MAXIMUM;

	// fill pagestack
	PathsData *soft_cuts = nullptr;
	PathsData *folds = nullptr;
	PathsData *peaks = nullptr;
	PathsData *valleys = nullptr;
	PathsData *hard_cuts = nullptr;
	PathsData *debugs = nullptr;
	if (spread->path) { spread->path->dec_count(); spread->path = nullptr; } // *** todo: does laidout ever repurpose already created spreads?
	Group *spread_paths = new Group;
	spread_paths->Id("spreadpath group");
	spread->path = spread_paths;

	// build lines...
	Path *path = nullptr;
	Coordinate *tt;
	for (int c=0; c<net->lines.n; c++) {
		NetLine *line = net->lines.e[c];
		if (!line->points) continue;

		LineStyle *ls = line->linestyle;
		if (!ls && net->lines.e[c]->style_hint == Net::EDGE_Fold)        ls = line_fold;
		if (!ls && net->lines.e[c]->style_hint == Net::EDGE_Fold_Peak)   ls = line_peak;
		if (!ls && net->lines.e[c]->style_hint == Net::EDGE_Fold_Valley) ls = line_valley;
		if (!ls && net->lines.e[c]->style_hint == Net::EDGE_Hard_Cut)    ls = line_cut;
		if (!ls && net->lines.e[c]->style_hint == Net::EDGE_Soft_Cut)    ls = line_soft_cut;
		if (!ls) ls = line_debug;

		path = new Path(line->points->duplicateAll(), ls);
		path->Id("subline path");

		//transform points out of net coords into paper coords
		tt = path->path;
		do {
			tt->p(transform_point(net->m(), tt->p()));
			tt = tt->next;
		} while (tt && tt != path->path);

		PathsData *pp = nullptr;
		if      (ls == line_fold)     { if (!folds)     { folds     = new PathsData(); folds    ->InstallLineStyle(line_fold);     } pp = folds;     }
		else if (ls == line_peak)     { if (!peaks)     { peaks     = new PathsData(); peaks    ->InstallLineStyle(line_peak);     } pp = peaks;     }
		else if (ls == line_valley)   { if (!valleys)   { valleys   = new PathsData(); valleys  ->InstallLineStyle(line_valley);   } pp = valleys;   }
		else if (ls == line_cut)      { if (!hard_cuts) { hard_cuts = new PathsData(); hard_cuts->InstallLineStyle(line_cut);      } pp = hard_cuts; }
		else if (ls == line_soft_cut) { if (!soft_cuts) { soft_cuts = new PathsData(); soft_cuts->InstallLineStyle(line_soft_cut); } pp = soft_cuts; }
		else if (ls == line_debug)    { if (!debugs)    { debugs    = new PathsData(); debugs   ->InstallLineStyle(line_debug);    } pp = debugs;    }
		pp->style |= PathsData::PATHS_Ignore_Weights;
		pp->paths.push(path);
		pp->Id("net subline");
		if (spread_paths->pushnodup(pp) == -1) pp->dec_count();
		path->dec_count();
	}
	if (folds)     folds    ->FindBBox();
	if (peaks)     peaks    ->FindBBox();
	if (valleys)   valleys  ->FindBBox();
	if (hard_cuts) hard_cuts->FindBBox();
	if (soft_cuts) soft_cuts->FindBBox();
	if (debugs)    debugs   ->FindBBox();
	spread_paths->FindBBox();

	//construct pagestack
	int page;
	PathsData *newpath = nullptr;
	int n, isbez;
	flatpoint *p = nullptr;
	NetFace *netface;
	unsigned long flag;
	double mm[6];
	flatpoint origin, xaxis;
	flatpoint pp;
	Affine facem;

	for (int c=0; c<net->faces.n; c++) {
		netface=net->faces.e[c];
		if (netface->tag!=FACE_Actual) continue;

		page=pageoffset+net->faces.e[c]->original;
		if (page>=numpages) page=-1;

		if (netface->matrix) {
			transform_mult(mm, netface->matrix, net->m());
		} else transform_copy(mm,net->m());

		newpath=new PathsData;
		isbez= (netface->getOutline(&n, &p, 0)==2);
		flag=(isbez?POINT_TONEXT:POINT_VERTEX);

		//------
		origin=transform_point(mm,flatpoint(0,0));
		xaxis=transform_point(mm,flatpoint(1,0))-origin;
		//------
		//if (isbez) {
		//	origin=transform_point(mm,p[1]);
		//	xaxis=transform_point(mm,p[4])-origin;
		//} else {
		//	origin=transform_point(mm,p[0]);
		//	xaxis=transform_point(mm,p[1])-origin;
		//}
		//---------
		xaxis.normalize();

		for (int c2=0; c2<n; c2++) {
			pp=transform_point(mm,p[c2]); //transform to paper coords
			newpath->append(pp.x,pp.y,flag);
			if (isbez) {
				if (flag==POINT_TONEXT) flag=POINT_VERTEX;
				else if (flag==POINT_VERTEX) flag=POINT_TOPREV;
				else flag=POINT_TONEXT;
			}
		}
		facem.setBasis(origin,xaxis,transpose(xaxis));
		newpath->MatchTransform(facem);
		newpath->close();
		newpath->FindBBox();
		newpath->origin(origin);
		delete[] p;

		spread->pagestack.push(new PageLocation(page,nullptr,newpath)); //incs newpath count
		newpath->dec_count();//remove extra count
	}


	 // define max/min points
	SomeData *pm = nullptr;
	if (spread->path) pm = spread->path;
	else if (spread->pagestack.n()) pm = spread->pagestack.e[0]->outline;
	spread->minimum=transform_point(pm->m(),
			flatpoint(pm->minx,pm->miny+(pm->maxy-pm->miny)/2));
	spread->maximum=transform_point(pm->m(),
			flatpoint(pm->maxx,pm->miny+(pm->maxy-pm->miny)/2));

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
	if (!nets.n) return nullptr;
	if (numActiveNets()==0) return nullptr;

	int c;
	int numactive=numActiveFaces();

	Spread *spread=nullptr;
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
	if (!nets.n) return nullptr;

	Spread *spread = PageLayout(whichpaper);
	spread->style = SPREAD_PAPER;

	//PathsData *path = dynamic_cast<PathsData *>(spread->path);//this was a non-local PathsData obj

	 // put a reference to the outline in marks if printnet
	if (printnet) {
		spread->mask |= SPREAD_PRINTERMARKS;
		spread->marks = spread->path;
		spread->path->inc_count();
		spread->marks->flags |= SOMEDATA_UNSELECTABLE;
		// ScreenColor color(.7,.7,.7, 1.);
		// dynamic_cast<PathsData *>(spread->marks)->line(-1,-1,-1, &color);
		// dynamic_cast<PathsData *>(spread->marks)->fill(nullptr);
	}
	
	if (papergroup) {
		 //there is no need to actually draw the paper, since that is done by the viewer code
		spread->papergroup = papergroup;
		spread->papergroup->inc_count();

		 //center spread in paper
		flatpoint center1(papergroup->papers.e[0]->box->paperstyle->w()/2,papergroup->papers.e[0]->box->paperstyle->h()/2);
		flatpoint center2((spread->path->maxx+spread->path->minx)/2, (spread->path->maxy+spread->path->miny)/2);
		spread->path->origin(spread->path->origin() + center1 - center2);

		for (int c = 0; c < spread->pagestack.n(); c++) {
			if (!spread->pagestack.e[c]->outline) continue;
			spread->pagestack.e[c]->outline->origin(spread->pagestack.e[c]->outline->origin()+ center1 - center2);
		}
	}

	return spread;
}

//! Return how many pages the current setup can hold.
/*! For instance, for a cube, if the document only has 3 pages,
 * 6 pages will still be returned.
 */
int NetImposition::NumPageSpreads()
{
	return GetPapersNeeded(Imposition::NumPages());
}

//! Set the known number of pages to npages.
int NetImposition::NumPages(int npages)
{
	numpages=npages;
	numpapers=GetPapersNeeded(npages);
	return numpages;
}

//! Redefined to also return for SINGLES_WITH_ADJACENT_LAYOUT.
/*! SINGLES_WITH_ADJACENT_LAYOUT returns the same as SINGLESLAYOUT.
 */
int NetImposition::NumSpreads(int layout)
{
	if (layout==SINGLES_WITH_ADJACENT_LAYOUT) return NumSpreads(SINGLELAYOUT);
	if (layout==PAGELAYOUT || layout==LITTLESPREADLAYOUT) return NumPageSpreads();
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
	int n=numActiveFaces();
	if (n==0) {
		DBG cerr <<"*****warning: no active faces in net!!"<<endl;
		return 0;
	}
	return (npages-1)/n+1;
}

//! Same as GetPapersNeeded().
int NetImposition::GetSpreadsNeeded(int npages)
{
	return GetPapersNeeded(npages);
}

int NetImposition::GetNumInPaperGroupForSpread(int layout, int spread)
{
	if (layout == SINGLELAYOUT) return 1;
	if (layout == PAGELAYOUT) return 1;
	if (layout == SINGLES_WITH_ADJACENT_LAYOUT) return 1;
	if (layout == PAPERLAYOUT) return papergroup->papers.n;
	return 0;
}

//! Assuming one page type per active face.
int NetImposition::NumPageTypes()
{
	return numActiveFaces();
}

//! Just return a string with the number.
/*! \todo i doubt it will matter, but this function is not threadsafe. the returned value
 *    should be used quickly, before anything else calls this function.
 */
const char *NetImposition::PageTypeName(int pagetype)
{
	if (pagetype<0 || pagetype>=numActiveFaces()) return nullptr;
	static char str[50];
	sprintf(str,"%d",pagetype);
	return str;
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
void NetImposition::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%snumpages 3      #number of pages in the document. This is ignored on readin\n",spc);
		fprintf(f,"%sprintnet yes     #whether the net lines get printed out with the page data\n",spc);
		fprintf(f,"%spaper letter      #Paper size to print on.\n",spc);
		fprintf(f,"%spapers           #Alternately, define a particular PaperGroup to use.\n",spc);
		fprintf(f,"%s  ...\n",spc);
		fprintf(f,"%sscalingfromnet 1  #any final scaling to apply to a net before mapping\n",spc);
		fprintf(f,"%s                  #  onto a spread\n",spc);
		fprintf(f,"%sabstractnet type  #type can be \"file\" or \"Polyhedron\" or \"simple\".\n",spc);
		fprintf(f,"%sabstractnet file  #this block demonstrates abstract nets based on files.\n",spc);
		fprintf(f,"%s  filename /path/to/it  #This is used when the abstract net has not been\n",spc);
		fprintf(f,"%s                        #modified since being loaded from the file.\n",spc);
		fprintf(f,"%ssimplenet         #this is the same as using: abstractnet simple\n",spc);
		fprintf(f,"%s                  #It is a basic net definition the same as a Polyhedron (below),\n",spc);
		fprintf(f,"%s                  #but using only vertices (with only 2-d vertices) and faces blocks.\n",spc);
		fprintf(f,"%sabstractnet Polyhedron  #this block demonstrates Polyhedron abstract nets.\n",spc);
		Polyhedron p;
		p.dump_out(f,indent+2,-1,nullptr);//**** if abstract nets get automated, this must change...

		fprintf(f,"%snet NetType   #There can be none or more nets. If there is no abstractnet, then\n",spc);
		fprintf(f,"%s              #there must be one or more nets. Nets are usually different arrangements\n",spc);
		fprintf(f,"%s              #of an abstract net.\n",spc);
		if (nets.n) nets.e[0]->dump_out(f,indent+2,-1,nullptr);
		else {
			Net n;
			n.dump_out(f,indent+2,-1,nullptr);
		}
		//fprintf(f,"%sdefaultpaperstyle #default paper style\n",spc);
		//***paperstyle->dump_out(f,indent+2,-1,nullptr);
		return;
	}


	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
//	if (paper) { ***
//		fprintf(f,"%sdefaultpaperstyle\n",spc);
//		paperstyle->dump_out(f,indent+2,0,context);
//	}
	if (printnet) fprintf(f,"%sprintnet\n",spc);
		else fprintf(f,"%sprintnet false\n",spc);

	fprintf(f, "%smargin %f\n",spc, margin);
	fprintf(f, "%smargin_is_percent %s\n",spc, margin_is_percent ? "yes" : "no");

	if (scalefromnet!=1) fprintf(f,"%sscalingfromnet %.10g\n",spc,scalefromnet);

	if (papergroup) {
		fprintf(f,"%spapers\n",spc);
		papergroup->dump_out(f,indent+2,0,context);
	}

	if (abstractnet) {
		if (!strcmp(abstractnet->whattype(),"SimpleNet")) {
			fprintf(f,"%ssimplenet\n",spc);
			abstractnet->dump_out(f,indent+2,0,context);

		} else if (!strcmp(abstractnet->whattype(),"Polyhedron")) { // && abstractnet->Modified()) {
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

			if (nets.e[c]->info&NETIMP_Internal) fprintf(f," %s\n",nets.e[c]->whatshape());
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
void NetImposition::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *name, *value;
	Net *tempnet = nullptr;
	int foundscaling = 0;
	bool is_accordion = false;
	int acc_1 = 0, acc_2 = 0;
	const char *acc_type = nullptr;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"numpages")) {
			IntAttribute(value,&numpages);
			if (numpages<0) numpages=0;

		} else if (!strcmp(name,"defaultpaperstyle")) {
			//*** ignoring defaultpaperstyle.....
			//***if (paperstyle) delete paperstyle;
			//paperstyle=new PaperStyle("Letter",8.5,11,0,300,"in");//***
			//paperstyle->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"net")) {
			if (!isblank(value)) { // is a built in
				SetNet(value);
			} else {
				tempnet=new Net();
				if (foundscaling) tempnet->info|=NETIMP_AlreadyScaled;
				tempnet->dump_in_atts(att->attributes.e[c],flag,context);
				if (tempnet->info&NETIMP_AlreadyScaled) tempnet->setIdentity();
				SetNet(tempnet);
				tempnet->dec_count();
			}
		} else if (!strcmp(name,"scalingfromnet")) {
			DoubleAttribute(value,&scalefromnet);
			if (scalefromnet<=0) scalefromnet=1;
			else foundscaling=1;

		} else if (!strcmp(name,"printnet")) {
			printnet=BooleanAttribute(value);

		} else if (!strcmp(name,"paper")) {
			PaperStyle *paperstyle=new PaperStyle(value);
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			SetPaperSize(paperstyle);
			paperstyle->dec_count();

		} else if (!strcmp(name,"margin")) {
			DoubleAttribute(value, &margin);

		} else if (!strcmp(name,"margin_is_percent")) {
			margin_is_percent = BooleanAttribute(value);

		} else if (!strcmp(name,"papers")) {
			if (papergroup) papergroup->dec_count();
			papergroup=new PaperGroup;
			papergroup->dump_in_atts(att->attributes.e[c],flag,context);
			if (papergroup->papers.n) {
				if (paper) paper->dec_count();
				paper=papergroup->papers.e[0]->box;
				paper->inc_count();
			}

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

		} else if (!strcmp(name,"generator")) {
			// this is abbreviated accordion def, like selected from newdoc dialog
			if (strEquals(value, "accordion", true)) {
				DBG att->attributes.e[c]->dump_out(stderr, 2);

				acc_type = att->attributes.e[c]->findValue("variant");
				acc_1 = att->attributes.e[c]->findLong("p1");
				acc_2 = att->attributes.e[c]->findLong("p2");
				is_accordion = true;
			}
		}
	}

	if (is_accordion) {
		ObjectDef *def = Accordion::GetPresets();
		ObjectDef *edef = def->FindDef(acc_type);
		int id = strtol(edef->defaultvalue, NULL, 10);
		Accordion::AccordionPresets preset = (Accordion::AccordionPresets) id;
		double width = 8.5;
		double height = 11;
		PaperStyle *paper = GetDefaultPaper();
		if (paper) {
			width  = paper->w();
			height = paper->h();
		}

		margin = 0;
		fit_paper_asymmetric = true;
		NetImposition *imp = Accordion::Build(preset, width, height, acc_1, acc_2, this);
		if (!imp) {
			DBG cerr << " *** error creating Accordion!!"<<endl;
		}
	}

	if (numActiveFaces() == 0 && abstractnet) {
		Net *n = new Net;
		if (foundscaling) n->info |= NETIMP_AlreadyScaled;
		n->Basenet(abstractnet);
		n->TotalUnwrap();
		n->active = 1;
		nets.push(n);
		n->dec_count();
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
	if (poly->dumpInFile(filename,nullptr)==0) {
		 //successfully loaded a polyhedron
		return poly;
	}
	delete poly;

	//***try to load a simple net...
	return nullptr;
}

//! Try to set up imposition based on file.
/*! File can be either a polyhedron file, net file.
 *
 * Return 0 for success, nonzero for failure.
 */
int NetImposition::SetNetFromFile(const char *file)
{
	AbstractNet *absnet=AbstractNetFromFile(file);
	if (absnet) {
		nets.flush();
		if (abstractnet) abstractnet->dec_count();
		abstractnet=absnet;
		makestr(briefdesc,nullptr);

		if (numActiveFaces()==0 && abstractnet) {
			Net *n=new Net;
			n->Basenet(abstractnet);
			n->TotalUnwrap();
			n->active=1;
			nets.push(n);
			n->dec_count();
		}
		return 0;
	}

	return 1;
}

} // namespace Laidout

