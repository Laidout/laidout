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
// Copyright (C) 2004-2010 by Tom Lechner
//



#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/interfaces/somedatafactory.h>

#include <lax/lists.cc>

#include "page.h"
#include "drawdata.h"
#include "stylemanager.h"
#include "language.h"

using namespace LaxFiles;
using namespace LaxInterfaces;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


//----------------------------------- PageBleed ----------------------------------------
/*! \class PageBleed
 * \brief Simple class to keep track of how pages bleed onto each other.
 *
 * index is a Document page index, and matrix is the transform by which coordinates from
 * that page are transformed to the current page.
 */

PageBleed::PageBleed(int i, const double *m)
{
	index=i;
	transform_copy(matrix,m);
	hasbleeds=0;
}

//----------------------------------- PageStyles ----------------------------------------

/*! \class PageStyle
 * \brief Basic Page style.
 *
 * PageStyle objects are supposed to be basically independent of particular page numbers. They
 * are maintained by whatever imposition is in charge. You might have 1000 pages, but only
 * a couple page types, so the page style with margin and boundary information can be shared
 * across many pages.
 *
 * flags can indicate whether margins clip, and whether contents from other pages are allowed to bleed onto this one.
 *
 * min_x, min_y, width and height indicate the bounding box for the page in page coordinates. For rectangular pages,
 * this is just the page outline, and otherwise is just the bbox, not the outline.
 * 
 * \todo *** integrate pagetype
 */
/*! \var int PageStyle::pagetype
 * \brief A number given by an imposition saying what type of pagestyle this is.
 *
 * \todo ***this variable has potential, but is barely used currently.
 */
/*! \var double PageStyle::min_x
 * Minimum x value of outline.
 */
/*! \var double PageStyle::min_y
 * Minimum y value of outline.
 */
/*! \var double PageStyle::width
 * \brief The width of the bounding box of the page.
 */
/*! \var double PageStyle::height
 * \brief The height of the bounding box of the page.
 */
/*! \var unsigned int PageStyle::flags
 *  Can be or'd combination of:
 * 
 * <pre>
 *  MARGINS_CLIP         (1<<0)
 *  PAGE_CLIPS           (1<<1)
 *  FACING_PAGES_BLEED   (1<<2)
 *  PAGESTYLE_AUTONOMOUS (1<<3) <-- set if the style instance is NOT an
 *                                  imposition default, ie it is a custom style
 * </pre>
 */
PageStyle::PageStyle()
{
	outline=margin=NULL;
	pagetype=0;
	min_x=min_y=width=height=0;
	flags=0; 
}

PageStyle::~PageStyle()
{
	if (outline) outline->dec_count();
	if (margin)  margin->dec_count();
}

//! Toggle a flag (-1) or set on (1) or off (0).
/*! Return the flag if it is set afterwards.*** beware int vs. uint
 */
int PageStyle::set(const char *flag, int newstate)
{
	if (!strcmp(flag,"marginsclip")) {
		if (newstate==-1) flags^=MARGINS_CLIP;
		else if (newstate==0) flags&=~MARGINS_CLIP;
		else if (newstate==1) flags|=MARGINS_CLIP;
		return flags&MARGINS_CLIP;

	} else if (!strcmp(flag,"pageclips")) {
		if (newstate==-1) flags^=PAGE_CLIPS;
		else if (newstate==0) flags&=~PAGE_CLIPS;
		else if (newstate==1) flags|=PAGE_CLIPS;
		return flags&PAGE_CLIPS;

	} else if (!strcmp(flag,"facingpagesbleed")) {
		if (newstate==-1) flags^=FACING_PAGES_BLEED;
		else if (newstate==0) flags&=~FACING_PAGES_BLEED;
		else if (newstate==1) flags|=FACING_PAGES_BLEED;
		return flags&FACING_PAGES_BLEED;
	}
	return -1;
}

/*! Recognizes 'pageclips', 'marginsclip', 'facingpagesbleed', 'width', and 'height'.
 * Discards all else.
 */
void PageStyle::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	flags=0;
	min_x=min_y=0;
	width=height=0;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"marginsclip")) {
			if (BooleanAttribute(value)) flags|=MARGINS_CLIP;

		} else if (!strcmp(name,"pageclips")) {
			//cout <<"*****adding page clips"<<endl;
			if (BooleanAttribute(value)) flags|=PAGE_CLIPS;

		} else if (!strcmp(name,"facingpagesbleed")) {
			if (BooleanAttribute(value)) flags|=FACING_PAGES_BLEED;

		} else if (!strcmp(name,"min_x")) {
			DoubleAttribute(value,&min_x);

		} else if (!strcmp(name,"min_y")) {
			DoubleAttribute(value,&min_y);

		} else if (!strcmp(name,"width")) {
			DoubleAttribute(value,&width);

		} else if (!strcmp(name,"height")) {
			DoubleAttribute(value,&height);

		} else { 
			DBG cerr <<"PageStyle dump_in:*** unknown attribute!!"<<endl;
		}
	}
}

/*! Write out flags, width, height.
 * can be:
 * <pre>
 *  width 8.5
 *  height 11
 *  marginsclip
 *  pageclips
 *  facingpagesbleed
 * </pre>
 */
void PageStyle::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%swidth 8.5    #overrides the default page width\n",spc);
		fprintf(f,"%sheight 11    #overrides the default page height\n",spc);
		fprintf(f,"%smin_x 0      #overrides the default min point\n",spc);
		fprintf(f,"%smin_y 0      #overrides the default min point\n",spc);
		fprintf(f,"%smarginsclip  #whether the margins clip page contents\n",spc);
		fprintf(f,"%spageclips    #whether the page outline clips page contents\n",spc);
		fprintf(f,"%sfacingpagesbleed  #whether facing pages are allowed to bleed onto this one\n",spc);
		return;
	}

	fprintf(f,"%smin_x %.10g\n",spc,minx());
	fprintf(f,"%smin_y %.10g\n",spc,miny());
	fprintf(f,"%swidth %.10g\n",spc,w());
	fprintf(f,"%sheight %.10g\n",spc,h());
	if (flags&MARGINS_CLIP) fprintf(f,"%smarginsclip\n",spc);
	if (flags&PAGE_CLIPS) fprintf(f,"%spageclips\n",spc);
	if (flags&FACING_PAGES_BLEED) fprintf(f,"%sfacingpagesbleed\n",spc);
}

//! Creates brand new object if s==NULL. Copies over width, height, and flags.
Style *PageStyle::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new PageStyle();
	else s=dynamic_cast<PageStyle *>(s);
	PageStyle *ps=dynamic_cast<PageStyle *>(s);
	if (!ps) return NULL;

	ps->flags=flags;
	ps->width=width;
	ps->height=height;
	ps->min_x=min_x;
	ps->min_y=min_y;
	if (margin)  ps->margin  = dynamic_cast<PathsData*>(margin ->duplicate(NULL));
	if (outline) ps->outline = dynamic_cast<PathsData*>(outline->duplicate(NULL));

	return s;
}

//! The newfunc for PageStyle instances.
Value *NewPageStyle()
{
	PageStyle *d=new PageStyle;
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

 //! Return a pointer to a new local ObjectDef class with the PageStyle description.
ObjectDef *PageStyle::makeObjectDef()
{
	ObjectDef *sd=stylemanager.FindDef("PageStyle");
	if (sd) {
		sd->inc_count();
		return sd;
	}

	//ObjectDef(const char *nname,const char *nName,const char *ntp, const char *ndesc,unsigned int fflags=STYLEDEF_CAPPED);
	sd=new ObjectDef(NULL,"PageStyle",
			_("Generic Page"),
			_("A page"),
			"class",
			NULL,NULL);

	//int ObjectDef::push(name,Name,ttip,ndesc,format,range,val,flags,newfunc);
	sd->newfunc=NewPageStyle;
	sd->push("marginsclip",
			_("Margins Clip"),
			_("Whether a page's margins clip the contents"),
			"boolean", NULL,"0",
			0,
			NULL);
	sd->push("pageclips",
			_("Page Clips"),
			_("Whether a page's outline clips the contents"),
			"boolean", NULL,"0",
			0,
			NULL);
	sd->push("facingpagesbleed",
			_("Facing Pages Bleed"),
			_("Whether nearby pages are allowed to bleed onto this page"),
			"boolean", NULL,"0",
			0,
			NULL);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}

//--------------------------------- RectPageStyle ---------------------------------------


/*! \class RectPageStyle
 * \brief Further info for rectangular pages
 *
 * Holds additional left, right, top, bottom margin insets, and
 * also whether the page is next to another, and so would have inside/outside
 * margins rather than left/right or top/bottom
 */
/*! \var unsigned int RectPageStyle::recttype
 * \code
 *   // left-right-top-bottom margins "rectpagestyle"
 *  #define RECTPAGE_LRTB      1
 *   // inside-outside-top-bottom (for booklet sort of style) "facingrectstyle"
 *  #define RECTPAGE_IOTB      2
 *   // left-right-inside-outside (for calendar sort of style) "topfacingrectstyle"
 *  #define RECTPAGE_LRIO      4
 *  #define RECTPAGE_LEFTPAGE  8
 *  #define RECTPAGE_RIGHTPAGE 16
 * \endcode
 */



//! Constructor, create new rectangular page style.
/*! ntype is RECTPAGE_LRTB for stand alone pages,
 * RECTPAGE_IOTB for a booklet sort of arrangement,
 * or RECTPAGE_LRIO for a calendar sort of arrangment.
 */
RectPageStyle::RectPageStyle(unsigned int ntype,double l,double r,double t,double b)
{
	recttype=ntype;
	ml=l;
	mr=r;
	mt=t;
	mb=b; //*** these would later start out at global default page margins.
}

/*! Recognizes  margin[lrtb], leftpage, rightpage, lrtb, iotb, lrio,
 * 'marginsclip', 'facingpagesbleed', 'width', and 'height'.
 * Discards all else.
 */
void RectPageStyle::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	ml=mr=mt=mb=0;
	recttype=0;
	char *name,*value;
	PageStyle::dump_in_atts(att,flag,context);
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"marginl")) {
			DoubleAttribute(value,&ml);
		} else if (!strcmp(name,"marginr")) {
			DoubleAttribute(value,&mr);
		} else if (!strcmp(name,"margint")) {
			DoubleAttribute(value,&mt);
		} else if (!strcmp(name,"marginb")) {
			DoubleAttribute(value,&mb);
		} else if (!strcmp(name,"leftpage") || !strcmp(name,"toppage")) {
			recttype=(recttype&~(RECTPAGE_LEFTPAGE|RECTPAGE_RIGHTPAGE))|RECTPAGE_LEFTPAGE;
		} else if (!strcmp(name,"rightpage") || !strcmp(name,"bottompage")) {
			recttype=(recttype&~(RECTPAGE_LEFTPAGE|RECTPAGE_RIGHTPAGE))|RECTPAGE_RIGHTPAGE;
		} else if (!strcmp(name,"lrtb")) {
			recttype=(recttype&~(RECTPAGE_LRTB|RECTPAGE_IOTB|RECTPAGE_LRIO))|RECTPAGE_LRTB;
		} else if (!strcmp(name,"iotb")) {
			recttype=(recttype&~(RECTPAGE_LRTB|RECTPAGE_IOTB|RECTPAGE_LRIO))|RECTPAGE_IOTB;
		} else if (!strcmp(name,"lrio")) {
			recttype=(recttype&~(RECTPAGE_LRTB|RECTPAGE_IOTB|RECTPAGE_LRIO))|RECTPAGE_LRIO;
		} else { 
			DBG cerr <<"PageStyle dump_in:*** unknown attribute!!"<<endl;
		}
	}
}

/*! Write out marginsl/r/t/b, leftpage or rightpage, lrtb, iotb, lrio,
 * and call PageStyle::dump_out for flags, width, height.
 *
 * \todo when reading in, must remember to let some features be overridden,
 *   but lrtb, iotb, etc are always set by the imposition
 */
void RectPageStyle::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%smarginl 0  #amount in from the left or outside for a margin\n",spc);
		fprintf(f,"%smarginr 0  #amount in from the right or inside for a margin\n",spc);
		fprintf(f,"%smargint 0  #amount in from the top   for a margin\n",spc);
		fprintf(f,"%smarginb 0  #amount in from the bottom for a margin\n",spc);
		fprintf(f,"%stoppage    #page is tagged as being on the top in a spread\n", spc);
		fprintf(f,"%sbottompage #page is tagged as being on the bottom in a spread\n", spc);
		fprintf(f,"%sleftpage   #page is tagged as being on the left in a spread\n", spc);
		fprintf(f,"%srightpage  #page is tagged as being on the right in a spread\n", spc);
		fprintf(f,"%slrtb       #tag that page has left, right, top, and bottom margins\n",spc);
		fprintf(f,"%siotb       #tag that page has inside, outside, top, and bottom margins\n",spc);
		fprintf(f,"%slrio       #tag that page has left, right, inside, and outside margins\n",spc);
		return;
	}
	fprintf(f,"%smarginl %.10g\n",spc,ml);
	fprintf(f,"%smarginr %.10g\n",spc,mr);
	fprintf(f,"%smargint %.10g\n",spc,mt);
	fprintf(f,"%smarginb %.10g\n",spc,mb);
	if (recttype&RECTPAGE_LEFTPAGE) {
		if (recttype&RECTPAGE_LRIO) fprintf(f,"%stoppage\n", spc);
		else fprintf(f,"%sleftpage\n", spc);
	}
	if (recttype&RECTPAGE_RIGHTPAGE) {
		if (recttype&RECTPAGE_LRIO) fprintf(f,"%sbottompage\n", spc);
		else fprintf(f,"%srightpage\n",spc);
	}
	if (recttype&RECTPAGE_LRTB) fprintf(f,"%slrtb\n",spc);
	if (recttype&RECTPAGE_IOTB) fprintf(f,"%siotb\n",spc);
	if (recttype&RECTPAGE_LRIO) fprintf(f,"%slrio\n",spc);
	PageStyle::dump_out(f,indent,0,context);
}

//! Copy over ml,mr,mt,mb,recttype.
Style *RectPageStyle::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new RectPageStyle(recttype);
	RectPageStyle *blah=dynamic_cast<RectPageStyle *>(s);
	if (!blah) return NULL;
	blah->recttype=recttype;
	blah->ml=ml;
	blah->mr=mr;
	blah->mt=mt;
	blah->mb=mb;
	return PageStyle::duplicate(s);
}

//! The newfunc for PageStyle instances.
/*! \ingroup stylesandstyledefs
 */
Value *NewRectPageStyle()
{
	RectPageStyle *d=new RectPageStyle;
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

/*! \todo **** the newfunc is not quite right...
 */
ObjectDef *RectPageStyle::makeObjectDef() 
{
	const char *rpstype;
	if (recttype&RECTPAGE_IOTB)  rpstype="FacingRectPageStyle";
	else if (recttype&RECTPAGE_LRIO) rpstype="TopFacingRectPageStyle";
	else rpstype="RectPageStyle";

	ObjectDef *sd=stylemanager.FindDef(rpstype);
	if (sd) {
		sd->inc_count();
		return sd;
	}

	//else not found already, so create
	ObjectDef *psd=PageStyle::makeObjectDef();
	psd->dec_count(); //extraneous count, after this line should only have stylemanager's count

	if (recttype&RECTPAGE_IOTB) 
		sd=new ObjectDef(psd,rpstype,
						_("Rectangular Facing Page"),
						_("Rectangular Facing Page"),
						"class",NULL,NULL,
						NULL,
						0,
						NewRectPageStyle);
	else if (recttype&RECTPAGE_LRIO)
		sd=new ObjectDef(psd,rpstype,
						_("Rectangular Top Facing Page"),
						_("Rectangular Top Facing Page"),
						"class",NULL,NULL,
						NULL,0,NewRectPageStyle);
	else sd=new ObjectDef(psd,rpstype,
					_("Rectangular Page"),
					_("Rectangular Page"),
					"class",NULL,NULL,
					NULL,0,
					NewRectPageStyle);
	
	 // the left or inside
	if (recttype&RECTPAGE_LRIO) 
		sd->push("insidemargin",
			_("Inside Margin"),
			_("How much space to put on the inside of facing pages."),
			"real",
			NULL,NULL,
			0,
			NULL);
	else sd->push("leftmargin",
			_("Left Margin"),
			_("How much space to put in the left margin."),
			"real",
			NULL,NULL,
			0,
			NULL);
	
	 // right right or outside
	if (recttype&RECTPAGE_LRIO) 
		sd->push("outsidemargin",
			_("Outside Margin"),
			_("How much space to put on the outside of facing pages."),
			"real",
			NULL,NULL,
			0,
			NULL);
	else sd->push("rightmargin",
			_("Right Margin"),
			_("How much space to put in the right margin."),
			"real",
			NULL,NULL,
			0,
			NULL);

	 // the top or inside
	if (recttype&RECTPAGE_IOTB) 
		sd->push("insidemargin",
			_("Inside Margin"),
			_("How much space to put on the inside of facing pages."),
			"real",
			NULL,NULL,
			0,
			NULL);
	else sd->push("topmargin",
			_("Top Margin"),
			_("How much space to put in the top margin."),
			"real",
			NULL,NULL,
			0,
			NULL);

	 // the bottom or outside
	if (recttype&RECTPAGE_IOTB) 
		sd->push("outsidemargin",
			_("Outside Margin"),
			_("How much space to put on the outside of facing pages."),
			"real",
			NULL,NULL,
			0,
			NULL);
	else sd->push("bottommargin",
			_("Bottom Margin"),
			_("How much space to put in the bottom margin."),
			"real",
			NULL,NULL,
			0,
			NULL);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}
 
//---------------------------------- Page ----------------------------------------

/*! \class Page
 * \brief Holds page number, thumbnail, a pagestyle, and the page's layers
 *
 * Pages fit into designated portions of spreads and papers. The sequence
 * of pages can be easily rearranged, usually via the spread editor.
 *
 * The ObjectContainer part returns the individual layers of the page.
 */
/*! \var char *Page::label
 * \brief The label for a page.
 * 
 * This will usually be something like "2" or "23", but can also be "iii",
 * "IV", or even "C". See Document and PageRange for more details.
 */
/*! \var int Page::labeltype
 * \brief How to show the label in a SpreadEditor. See labelcolor and PageMarkerType.
 */
/*! \fn Group *Page::e(int i) 
 * \brief Return dynamic_cast<Group *>(layers.e(i)).
 */

//! Constructor, takes pointer, does not make copy of npagestyle, It deletes the pagestyle in destructor.
/*! If pslocal==0, then npagestyle->inc_count().
 *
 * Pushes 1 new Group onto layers stack.
 */
Page::Page(PageStyle *npagestyle,int num)
{
	label=NULL;
	thumbmodtime=0;
	modtime=times(NULL);
	pagestyle=npagestyle;
	if (pagestyle) pagestyle->inc_count();
	labeltype=MARKER_Circle;
	labelcolor.rgbf(1,1,1);
	thumbnail=NULL;
	pagenumber=num;

	 // initialize page contents to 1 empty layer.
	Group *g=new Group;
	g->Id("pagelayer");
	g->selectable=0;
	g->obj_flags=OBJ_Unselectable|OBJ_Zone; //force searches to not return return individual layers
	layers.push(g); //incs count
	g->dec_count();

	layers.selectable=0;
	layers.obj_flags=OBJ_Unselectable|OBJ_Zone; //force searches to not return return layers
	obj_flags=OBJ_Unselectable|OBJ_Zone; //force searches to not return return this
	layers.Id("pagegroup");
}

//! Destructor, destroys the thumbnail, and dec_counts pagestyle.
Page::~Page()
{
	DBG cerr <<"  Page destructor"<<endl;
	if (label) delete[] label;
	if (thumbnail) thumbnail->dec_count();
	if (pagestyle) pagestyle->dec_count();
	layers.flush();
}

/*! Return index of new layer.
 * If where<0 or where>=layers.n() push at end.
 */
int Page::PushLayer(const char *layername, int where)
{
	if (where<0 || where>=layers.n()) where=layers.n();

	Group *g = new Group;
	g->Id(layername ? layername : "pagelayer");
	g->selectable=0;
	g->obj_flags=OBJ_Unselectable|OBJ_Zone; //force searches to not return return individual layers
	layers.push(g,where); //incs count
	g->dec_count();

	return where;
}

const char *Page::object_e_name(int i)
{
	if (i<0 || i>layers.n()) return NULL;
	return layers.e(i)->Id();
}

//! Delete (or checkin) old, checkout new.
/*! If pstyle==NULL then still remove the old and make the pagestyle NULL.
 * This should later be corrected by imposition->SyncPages().
 *
 * Return 0 for success, nonzero for error.
 *
 * \todo if there is any custom margin information, it should be transferred...
 */
int Page::InstallPageStyle(PageStyle *pstyle, bool shift_within_margins)
{
	if (!pstyle) return 1;
	if (pstyle==pagestyle) return 0;

	flatpoint offset;
	if (pagestyle && shift_within_margins) {
		offset=   pstyle->margin->ReferencePoint(LAX_MIDDLE,true) 
			 - pagestyle->margin->ReferencePoint(LAX_MIDDLE,true);
	}

	if (pagestyle) pagestyle->dec_count();
	pagestyle=pstyle;
	if (pagestyle) pagestyle->inc_count();

	if (shift_within_margins && (offset.x!=0 || offset.y!=0)) {
		SomeData *o;
        DrawableObject *g;

		for (int c2=0; c2<layers.n(); c2++) {
            g=dynamic_cast<DrawableObject*>(layers.e(c2));
            if (g) for (int c3=0; c3<g->n(); c3++) {
              o=g->e(c3);
              o->origin(o->origin()+offset);
            }
        }

	}

	return 0;
}

//! Dump in page.
/*! Layers should have been flushed before coming here, and
 * pagestyle should have been set to the default page style for this page.
 *
 */
void Page::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"pagestyle")) {
			PageStyle *ps=NULL;

			const char *which=value;
			if (!which || !strcmp(value,"default")) which="PageStyle";
			if (!isblank(which)) {
				ObjectDef *def=stylemanager.FindDef(which);
				if (def) {
					Value *v=def->newObject(def);
					ps=dynamic_cast<PageStyle *>(v);
					if (!ps) {
						 //maybe it was embedded in an object value
						ObjectValue *vv=dynamic_cast<ObjectValue*>(v);
						if (vv) {
							ps=dynamic_cast<PageStyle *>(vv->object);
							if (ps) ps->inc_count();
						}
					}
					v->dec_count();
				}
			}

			if (ps) {
				ps->dump_in_atts(att->attributes.e[c],flag,context);
				ps->flags|=PAGESTYLE_AUTONOMOUS;
			}
			InstallPageStyle(ps, false);
			ps->dec_count();

		} else if (!strcmp(name,"layer")) {
			Group *g=new Group;
			makestr(g->object_idstr,"pagelayer");
			g->obj_flags|=OBJ_Unselectable|OBJ_Zone;
			g->dump_in_atts(att->attributes.e[c],flag,context);
			g->selectable=0;
			layers.push(g);
			g->dec_count();

		} else if (!strcmp(name,"labeltype")) {
			if (!isblank(value)) {
				if (!strcasecmp(value,"circle"))        labeltype=MARKER_Circle;
				else if (!strcasecmp(value,"square"))   labeltype=MARKER_Square;
				else if (!strcasecmp(value,"diamond"))  labeltype=MARKER_Diamond;
				else if (!strcasecmp(value,"triangle")) labeltype=MARKER_TriangleUp;
				else if (!strcasecmp(value,"octagon"))  labeltype=MARKER_Octagon;
				else IntAttribute(value,&labeltype);
			}

		} else if (!strcmp(name,"labelcolor")) {
			SimpleColorAttribute(value,NULL,&labelcolor, NULL);

		} else { 
			DBG cerr <<"Page dump_in:*** unknown attribute "<<(name?name:"(noname)")<<"!!"<<endl;
		}
	}
}

//! Write out pagestyle and layers. Ignore pagenumber.
/*!
 * 
 * If what==-1, then output pseudocode mockup of file format.
 */
void Page::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%slabeltype circle          #Can be circle, square, diamond, triangle, octagon\n",spc);
		fprintf(f,"%slabelcolor rgbf(1.,1.,1.) #color to fill the labeltype\n",spc);
		fprintf(f,"\n%s#Pages contain layers which contain drawable objects.\n",spc);
		fprintf(f,"%s#Layers are really just Group objects whose parent is a page.\n",spc);
		fprintf(f,"%s#Each drawable object has a number of common options, then has object\n",spc);
		fprintf(f,"%s#specific values under \"config\".\n",spc);

		fprintf(f,"\n%slayer\n",spc);
		Group g;
		g.dump_out(f,indent+2,-1,context);


         //dump out each available object type...
        anObject *o;
		SomeData *d;
        const char *t;
		ObjectFactory *factory=somedatafactory();

        for (int c=0; c<factory->NumTypes(); c++) {
            t=factory->TypeStr(c);

            o=factory->NewObject(t);
			d=dynamic_cast<SomeData*>(o);
			if (d) {
            	fprintf(f,"%s    object %s\n",spc, t);
				d->dump_out(f,indent+6, -1, context);
				fprintf(f,"\n");
			}
            o->dec_count();

        }

		return;
	}
	
	//labelcolor.dump_out(f,indent,what,context);

	if (labeltype==MARKER_Circle) fprintf(f,"%slabeltype circle\n",spc);
	else if (labeltype==MARKER_Square)       fprintf(f,"%slabeltype square\n",spc);
	else if (labeltype==MARKER_Diamond)      fprintf(f,"%slabeltype diamond\n",spc);
	else if (labeltype==MARKER_TriangleUp)   fprintf(f,"%slabeltype triangle\n",spc);
	else if (labeltype==MARKER_Octagon)      fprintf(f,"%slabeltype octagon\n",spc);
	else fprintf(f,"%slabeltype %d\n",spc,labeltype);

	fprintf(f,"%slabelcolor rgbf(%.10g,%.10g,%.10g)\n",spc,
				labelcolor.red/65535., labelcolor.green/65535., labelcolor.blue/65535.);

	if (pagestyle && (pagestyle->flags&PAGESTYLE_AUTONOMOUS)) {
		fprintf(f,"%spagestyle %s\n",spc,pagestyle->whattype());
		pagestyle->dump_out(f,indent+2,0,context);
	}

	for (int c=0; c<layers.n(); c++) {
		fprintf(f,"%slayer %d\n",spc,c);
		layers.e(c)->dump_out(f,indent+2,0,context);
	}
}

//! Update modtime to at_time. If at_time==0, then use current time.
void Page::Touch(clock_t at_time)
{
	if (at_time) modtime=at_time;
	else modtime=times(NULL);
}

//! Update the thumbnail if necessary and return it.
/*! Creates thumbnail only if thumbmodtime<modtime.
 * 
 * These are used notably in the SpreadEditor and SignatureInterface.
 *
 * Creates thumb images that are 200 pixels wide, and as many high as aspect ratio calls for.
 * 
 */
ImageData *Page::Thumbnail()
{
	if (!pagestyle) return NULL;
	if (thumbmodtime>modtime) return thumbnail;

	DoubleBBox bbox;
	if (pagestyle->outline) bbox=*(pagestyle->outline);
	else { bbox.maxx=pagestyle->w(); bbox.maxy=pagestyle->h(); }

	
	double w=bbox.maxx-bbox.minx,
		   h=bbox.maxy-bbox.miny;
	h=h*200./w;
	w=200.;
	DBG cerr <<"..----making thumbnail "<<w<<" x "<<h<<"  pgW,H:"<<pagestyle->w()<<','<<pagestyle->h()
	DBG 	<<"  bbox:"<<bbox.minx<<','<<bbox.maxx<<' '<<bbox.miny<<','<<bbox.maxy<<endl;

	if (!thumbnail) thumbnail=new ImageData(); 
	thumbnail->xaxis(flatpoint((bbox.maxx-bbox.minx)/w,0));
	thumbnail->yaxis(flatpoint(0,(bbox.maxx-bbox.minx)/w));
	thumbnail->origin(flatpoint(bbox.minx,bbox.miny));
	
	Displayer *dp=newDisplayer(NULL);
	dp->defaultRighthanded(true);
	dp->CreateSurface((int)w,(int)h);
	
	 // setup dp to have proper scaling...
	dp->NewTransform(1.,0.,0.,-1.,0.,0.);
	//dp->NewTransform(1.,0.,0.,1.,0.,0.);
	dp->SetSpace(bbox.minx,bbox.maxx, bbox.miny,bbox.maxy);
	dp->Center  (bbox.minx,bbox.maxx, bbox.miny,bbox.maxy);
		
	dp->NewBG(255,255,255); // *** this should be the paper color for paper the page is on...
	dp->NewFG(0,0,0,255);
	dp->ClearWindow();

	for (int c=0; c<layers.n(); c++) {
		//dp->PushAndNewTransform(layers.e[c]->m());
		DrawData(dp,layers.e(c));
		//dp->PopAxes();
	}
		
	LaxImage *img=dp->GetSurface();
	if (img) {
		thumbnail->SetImage(img,NULL); //*** must implement using diff size image than is in maxx,y
		img->dec_count();
	}
	
	DBG cerr <<"Thumbnail dump_out:"<<endl;
	DBG thumbnail->dump_out(stderr,2,0,NULL);
	DBG cerr <<"  minx "<<thumbnail->minx<<endl;
	DBG cerr <<"  maxx "<<thumbnail->maxx<<endl;
	DBG cerr <<"  miny "<<thumbnail->miny<<endl;
	DBG cerr <<"  maxy "<<thumbnail->maxy<<endl;
	//DBG save_image(img, "DBG.png", "png");

	DBG cerr <<"==--- Done Page::updating thumbnail.."<<endl;
	thumbmodtime=times(NULL);

	dp->EndDrawing();
	dp->dec_count();

	return thumbnail;
}

/*! Perform any AlignmentRule things in any object on the page.
 * Each is performed once in order the objects exist on the page.
 */
void Page::UpdateAnchored(Group *g)
{
	if (!g) g=&layers;

	DrawableObject *o;
	for (int c=0; c<g->n(); c++) {
		o=dynamic_cast<DrawableObject*>(g->e(c));
		if (!o) continue;
		if (o->parent_link) o->UpdateFromRules();

		if (o->kids.n) UpdateAnchored(o);
    } 
}


} // namespace Laidout

