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


#include "singles.h"
#include "stylemanager.h"
#include "language.h"
#include <lax/interfaces/pathinterface.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>

using namespace Laxkit;
using namespace LaxInterfaces;
using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 



namespace Laidout {



//-------------------------- Singles ---------------------------------------------

/*! \class Singles
 * \brief For single page per sheet, not meant to be next to other pages.
 *
 * The pages can be inset a certain amount from each edge, specified by inset[lrtb].
 *
 * \todo imp Spread::spreadtype and PageStyle::pagetype
 */



/*! \todo *** rethink styledef assignment..
 */
Singles::Singles() : Imposition(_("Singles"))
{ 
	insetleft=insetright=insettop=insetbottom=0;
	marginleft=marginright=margintop=marginbottom=0;
	tilex=tiley=1;
	gapx=gapy=0;
	pagestyle=NULL;

	PaperStyle *paperstyle=dynamic_cast<PaperStyle *>(stylemanager.FindDef("defaultpapersize"));
	if (paperstyle) paperstyle=static_cast<PaperStyle *>(paperstyle->duplicate());
	else paperstyle=new PaperStyle("letter",8.5,11.0,0,300,"in");
	SetPaperSize(paperstyle);
	paperstyle->dec_count();
			
	//pagestyle=NULL;
	//setPage();***<--called from SetPaperSize

	styledef=stylemanager.FindDef("Singles");
	if (styledef) styledef->inc_count(); 
	else {
		styledef=makeStyleDef();
		if (styledef) stylemanager.AddObjectDef(styledef,1);
		 // so this new styledef should have a count of 2. The Style destructor removes
		 // 1 count, and the stylemanager should remove the other
	}
	
	DBG cerr <<"imposition singles init"<<endl;
}

//! Calls pagestyle->dec_count().
Singles::~Singles()
{
	DBG cerr <<"--Singles destructor"<<endl;
	pagestyle->dec_count();
}

//! Static imposition resource creation function.
/*! Returns NULL terminated list of default resources.
 */
ImpositionResource **Singles::getDefaultResources()
{
	ImpositionResource **r=new ImpositionResource*[2];
	r[0]=new ImpositionResource("Singles",
								  _("Singles"),
								  NULL,
								  _("One sided single sheets"),
								  NULL,0);
	r[1]=NULL;
	return r;
}

//! Return paper dimensions (which==0) or page dimensions (which!=0).
void Singles::GetDimensions(int which, double *x, double *y)
{
	if (which==0) {
		*x=papergroup->papers.e[0]->box->paperstyle->w();
		*y=papergroup->papers.e[0]->box->paperstyle->h();
	}

	*x=pagestyle->w();
	*y=pagestyle->h();
}

//! Just return "Singles".
const char *Singles::BriefDescription()
{
	return _("Singles");
}

//! Using the paperstyle, create a new default pagestyle.
/*! dec_count() on old.
 */
void Singles::setPage()
{
	if (!paper) return;
	double oldl=0, oldr=0, oldt=0, oldb=0;
	if (pagestyle) {
		oldl=pagestyle->ml;
		oldr=pagestyle->mr;
		oldt=pagestyle->mt;
		oldb=pagestyle->mb;
		pagestyle->dec_count();
	}
	
	pagestyle=new RectPageStyle(RECTPAGE_LRTB);
	pagestyle->width=(paper->media.maxx-insetleft-insetright)/tilex;
	pagestyle->height=(paper->media.maxy-insettop-insetbottom)/tiley;
	pagestyle->pagetype=0;
	pagestyle->ml=oldl;
	pagestyle->mr=oldr;
	pagestyle->mt=oldt;
	pagestyle->mb=oldb;
}

//! Return the default page style for that page.
/*! Default is to pagestyle->inc_count() then return pagestyle.
 */
PageStyle *Singles::GetPageStyle(int pagenum,int local)
{
	if (!pagestyle) setPage();
	if (!pagestyle) return NULL;
	if (local) {
		PageStyle *ps=(PageStyle *)pagestyle->duplicate();
		ps->flags|=PAGESTYLE_AUTONOMOUS;
		return ps;
	}
	pagestyle->inc_count();
	return pagestyle;
}

//! Set paper size, also reset the pagestyle. Duplicates npaper, not pointer tranfser.
/*! Calls Imposition::SetPaperSize(npaper), then setPage().
 *
 * Return 0 success, nonzero error.
 */
int Singles::SetPaperSize(PaperStyle *npaper)
{
	if (Imposition::SetPaperSize(npaper)) return 1;
	setPage();
	return 0;
}

//! Set default left, right, top, bottom margins.
/*! Return 0 for success, or nonzero error.
 */
int Singles::SetDefaultMargins(double l,double r,double t,double b)
{
	if (!pagestyle) return 1;
	pagestyle->ml=l;
	pagestyle->mr=r;
	pagestyle->mt=t;
	pagestyle->mb=b;

	return 0;
}

/*! Define from Attribute.
 *
 * Expects defaultpagestyle to either not exist, or be a RectPageStyle
 * with RECTPAGE_LRTB.
 * 
 * Loads the default PaperStyle.. right now just inits
 * to letter if defaultpaperstyle attribute found, then dumps in that
 * paper style. see todos in Page also.....
 */
void Singles::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	int pages=-1;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"insetleft")) {
			DoubleAttribute(value,&insetleft);
		} else if (!strcmp(name,"insetright")) {
			DoubleAttribute(value,&insetright);
		} else if (!strcmp(name,"insettop")) {
			DoubleAttribute(value,&insettop);
		} else if (!strcmp(name,"insetbottom")) {
			DoubleAttribute(value,&insetbottom);
		} else if (!strcmp(name,"marginleft")) {
			DoubleAttribute(value,&marginleft);
		} else if (!strcmp(name,"marginright")) {
			DoubleAttribute(value,&marginright);
		} else if (!strcmp(name,"margintop")) {
			DoubleAttribute(value,&margintop);
		} else if (!strcmp(name,"marginbottom")) {
			DoubleAttribute(value,&marginbottom);
		} else if (!strcmp(name,"gapx")) {
			DoubleAttribute(value,&gapx);
		} else if (!strcmp(name,"gapy")) {
			DoubleAttribute(value,&gapy);
		} else if (!strcmp(name,"tilex")) {
			IntAttribute(value,&tilex);
		} else if (!strcmp(name,"tiley")) {
			IntAttribute(value,&tiley);
		} else if (!strcmp(name,"numpages")) {
			IntAttribute(value,&numpages);
			if (numpages<0) numpages=0;
		} else if (!strcmp(name,"defaultpagestyle")) {
			pages=c;
			if (pagestyle) pagestyle->dec_count();
			pagestyle=new RectPageStyle(RECTPAGE_LRTB);
			pagestyle->dump_in_atts(att->attributes.e[c],flag,context);
		} else if (!strcmp(name,"defaultpaperstyle")) {
			PaperStyle *paperstyle;
			paperstyle=new PaperStyle("Letter",8.5,11,0,300,"in");//***should be global def
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			SetPaperSize(paperstyle);
			paperstyle->dec_count();
		} else if (!strcmp(name,"defaultpapers")) {
			if (papergroup) papergroup->dec_count();
			papergroup=new PaperGroup;
			papergroup->dump_in_atts(att->attributes.e[c],flag,context);
			if (papergroup->papers.n) {
				if (paper) paper->dec_count();
				paper=papergroup->papers.e[0]->box;
				paper->inc_count();
			}
		}
	}
	if (pages<0) setPage();
}

/*! Writes out something like:
 * <pre>
 *  insetleft 0
 *  insetright 0
 *  insettop 0
 *  insetbottom 0
 *  tilex  1
 *  tiley  1
 *  numpages 10
 *  defaultpagestyle 
 *    ...
 *  defaultpaperstyle
 *    ...
 * </pre>
 *
 * If what==-1, dump out a pseudocode mockup of the file format.
 *
 * \todo *** finish what==-1
 */
void Singles::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname \"Some name\"   #this can be any string you want.\n",spc);
		fprintf(f,"%sdescription  Single sheets  #Some description beyond just a name.\n",spc);
		fprintf(f,"%s                            #name and description are used by the imposition resource mechanism\n",spc);
		fprintf(f,"%s#insets are regions of a paper not taken up by the page\n",spc);
		fprintf(f,"%sinsetl 0   #The left inset from the side of a paper\n",spc);
		fprintf(f,"%sinsetr 0   #The right inset from the side of a paper\n",spc);
		fprintf(f,"%sinsett 0   #The top inset from the side of a paper\n",spc);
		fprintf(f,"%sinsetb 0   #The bottom inset from the side of a paper\n",spc);
		fprintf(f,"%sgapx 0     #Gap between tiles horizontally\n",spc);
		fprintf(f,"%sgapy 0     #Gap between tiles vertically\n",spc);
		fprintf(f,"%stilex 1    #number of times to tile the page horizontally\n",spc);
		fprintf(f,"%stiley 1    #number of times to tile the page vertically\n",spc);
		fprintf(f,"%smarginl 0   #The default left page margin\n",spc);
		fprintf(f,"%smarginr 0   #The default right page margin\n",spc);
		fprintf(f,"%smargint 0   #The default top page margin\n",spc);
		fprintf(f,"%smarginb 0   #The default bottom page margin\n",spc);
		fprintf(f,"%snumpages 3 #number of pages in the document. This is ignored on readin\n",spc);
		fprintf(f,"%sdefaultpapers #default paper group\n",spc);
		papergroup->dump_out(f,indent+2,-1,NULL);
		fprintf(f,"%sdefaultpagestyle #default page style\n",spc);
		pagestyle->dump_out(f,indent+2,-1,NULL);
		return;
	}
	fprintf(f,"%smarginl %.10g\n",spc,marginleft);
	fprintf(f,"%smarginr %.10g\n",spc,marginright);
	fprintf(f,"%smargint %.10g\n",spc,margintop);
	fprintf(f,"%smarginb %.10g\n",spc,marginbottom);
	fprintf(f,"%sinsetl %.10g\n",spc,insetleft);
	fprintf(f,"%sinsetr %.10g\n",spc,insetright);
	fprintf(f,"%sinsett %.10g\n",spc,insettop);
	fprintf(f,"%sinsetb %.10g\n",spc,insetbottom);
	fprintf(f,"%stilex %d\n",spc,tilex);
	fprintf(f,"%stiley %d\n",spc,tiley);
	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);
	if (pagestyle) {
		fprintf(f,"%sdefaultpagestyle\n",spc);
		pagestyle->dump_out(f,indent+2,0,context);
	}
	if (papergroup) {
		fprintf(f,"%sdefaultpapers\n",spc);
		papergroup->dump_out(f,indent+2,0,context);
	}
}

//! Duplicate this, or fill in this attributes.
Style *Singles::duplicate(Style *s)//s=NULL
{
	Singles *sn;
	if (s==NULL) {
		//***stylemanager.newStyle("Singles");
		s=sn=new Singles();
	} else sn=dynamic_cast<Singles *>(s);
	if (!sn) return NULL;
	if (styledef) {
		styledef->inc_count();
		if (sn->styledef) sn->styledef->dec_count();
		sn->styledef=styledef;
	}
	if (pagestyle) {
		if (sn->pagestyle) sn->pagestyle->dec_count();
		pagestyle->inc_count();
		sn->pagestyle=pagestyle;
	}
	sn->marginleft=marginleft;
	sn->marginright=marginright;
	sn->margintop=margintop;
	sn->marginbottom=marginbottom;
	sn->insetleft=insetleft;
	sn->insetright=insetright;
	sn->insettop=insettop;
	sn->insetbottom=insetbottom;
	sn->tilex=tilex;
	sn->tiley=tiley;
	return Imposition::duplicate(s);  
}

//! The newfunc for Singles instances.
Value *NewSingles(StyleDef *def)
{ 
	Singles *s=new Singles;
	ObjectValue *v=new ObjectValue(s);
	s->styledef=def;
	s->dec_count();
	return v;
}

//! Return a ValueObject with a SignatureImposition.
/*! This does not throw an error for having an incomplete set of parameters.
 * It just fills what's given.
 */
int createSingles(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=NULL;
		log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 1;
	}

	Singles *imp=new Singles();

	char error[100];
	int err=0;
	try {
		int i, e;
		double d;

		 //---insetleft
		d=parameters->findDouble("insetleft",-1,&e);
		if (e==0) imp->insetleft=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insetleft"); throw error; }

		 //---insetright
		d=parameters->findDouble("insetright",-1,&e);
		if (e==0) imp->insetright=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insetright"); throw error; }

		 //---insettop
		d=parameters->findDouble("insettop",-1,&e);
		if (e==0) imp->insettop=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insettop"); throw error; }

		 //---insetbottom
		d=parameters->findDouble("insetbottom",-1,&e);
		if (e==0) imp->insetbottom=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"insetbottom"); throw error; }

		 //---marginleft
		d=parameters->findDouble("marginleft",-1,&e);
		if (e==0) imp->marginleft=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"marginleft"); throw error; }

		 //---marginright
		d=parameters->findDouble("marginright",-1,&e);
		if (e==0) imp->marginright=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"marginright"); throw error; }

		 //---margintop
		d=parameters->findDouble("margintop",-1,&e);
		if (e==0) imp->margintop=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"margintop"); throw error; }

		 //---marginbottom
		d=parameters->findDouble("marginbottom",-1,&e);
		if (e==0) imp->marginbottom=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"marginbottom"); throw error; }

		 //---tilegapx
		d=parameters->findDouble("gapx",-1,&e);
		if (e==0) imp->gapx=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"gapx"); throw error; }

		 //---tilegapy
		d=parameters->findDouble("gapy",-1,&e);
		if (e==0) imp->gapy=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"gapy"); throw error; }

		 //---tilex
		i=parameters->findInt("tilex",-1,&e);
		if (e==0) imp->tilex=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"tilex"); throw error; }

		 //---tiley
		i=parameters->findInt("tiley",-1,&e);
		if (e==0) imp->tiley=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"tiley"); throw error; }


	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}

	if (value_ret && err==0) {
		*value_ret=new ObjectValue(imp);
	}
	if (imp) imp->dec_count();

	return err;
}

StyleDef *Singles::makeStyleDef()
{
	return makeSinglesStyleDef();
}

//! Make an instance of the Singles imposition styledef.
/*  Required by Style, this defines the various names for fields relevant to Singles,
 *  basically just the inset[lrtb], plus the standard Imposition npages and npapers.
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  styledefs stored in a StyleManager.
 *
 *  Returns a new StyleDef with a count of 1.
 */
StyleDef *makeSinglesStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,"Singles",
			_("Singles"),
			_("Imposition of single pages"),
			"class",
			NULL,NULL,
			NULL,
			0, //new flags
			NewSingles,
			createSingles);

	sd->push("insetleft",
			_("Left Inset"),
			_("How much a page is inset in a paper on the left"),
			"real",
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("insetright",
			_("Right Inset"),
			_("How much a page is inset in a paper on the right"),
			"real",
			NULL,
			"0",
			0,NULL);
	sd->push("insettop",
			_("Top Inset"),
			_("How much a page is inset in a paper from the top"),
			"real",
			NULL,
			"0",
			0,0);
	sd->push("insetbottom",
			_("Bottom Inset"),
			_("How much a page is inset in a paper from the bottom"),
			"real",
			NULL,
			"0",
			0,0);
	sd->push("tilex",
			_("Tile X"),
			_("How many to tile horizontally"),
			"int",
			NULL,
			"1",
			0,0);
	sd->push("tiley",
			_("Tile Y"),
			_("How many to tile vertically"),
			"int",
			NULL,
			"1",
			0,0);
	sd->push("gapx",
			_("Horizontal gap"),
			_("Gap between tiles horizontally"),
			"real",
			NULL,
			"0",
			0,0);
	sd->push("gapy",
			_("Vertical gap"),
			_("Gap between tiles vertically"),
			"real",
			NULL,
			"0",
			0,0);
	sd->push("marginleft",
			_("Left Margin"),
			_("Default left page margin"),
			"real",
			NULL, //range
			"0",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("marginright",
			_("Right Margin"),
			_("Default right page margin"),
			"real",
			NULL,
			"0",
			0,NULL);
	sd->push("margintop",
			_("Top Margin"),
			_("Default top page margin"),
			"real",
			NULL,
			"0",
			0,0);
	sd->push("marginbottom",
			_("Bottom Margin"),
			_("Default bottom page margin"),
			"real",
			NULL,
			"0",
			0,0);

	return sd;
}

//! Create necessary pages based on default pagestyle.
/*! Currently returns NULL terminated list of pages.
 */
Page **Singles::CreatePages(int npages)
{
	if (npages>0) NumPages(npages);
	if (numpages==0) return NULL;

	Page **pages=new Page*[numpages+1];
	int c;
	PageStyle *ps;
	for (c=0; c<numpages; c++) {
		ps=GetPageStyle(c,0);
		 // pagestyle is passed to Page, not duplicated.
		 // There its count is inc'd.
		pages[c]=new Page(ps,c); 
		ps->dec_count(); //remove extra count
	}
	pages[c]=NULL;
	return pages;
}

//! Return outline of page in page coords. 
SomeData *Singles::GetPageOutline(int pagenum,int local)
{
	PathsData *newpath=new PathsData();//count==1
	newpath->appendRect(0,0,pagestyle->w(),pagestyle->h());
	newpath->maxx=pagestyle->w();
	newpath->maxy=pagestyle->h();
	//nothing special is done when local==0
	return newpath;
}

//! Return outline of page margin in page coords. 
/*! Calling code should call dec_count() on the returned object when it is no longer needed.
 *
 * If all the margins are 0, then NULL is returned.
 */
SomeData *Singles::GetPageMarginOutline(int pagenum,int local)
{
	 //return if no margins.
	if (pagestyle->ml==0 && pagestyle->mr==0 && pagestyle->mt==0 && pagestyle->mb==0) return NULL;

	PathsData *newpath=new PathsData();//count==1
	newpath->appendRect(pagestyle->ml,pagestyle->mb, 
						pagestyle->w()-pagestyle->mr-pagestyle->ml,pagestyle->h()-pagestyle->mt-pagestyle->mb);
	newpath->FindBBox();
	//nothing special is done when local==0
	return newpath;
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
 */
Spread *Singles::PaperLayout(int whichpaper)
{
	Spread *spread=new Spread();
	spread->spreadtype=1;
	spread->style=SPREAD_PAPER;
	spread->mask=SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;

	if (papergroup) {
		spread->papergroup=papergroup;
		spread->papergroup->inc_count();
	}
	
	 // define max/min points
	spread->minimum=flatpoint(paper->media.maxx/5,  paper->media.maxy/2);
	spread->maximum=flatpoint(paper->media.maxx*4/5,paper->media.maxy/2);

	 // fill spread with paper and page outline
	PathsData *newpath=new PathsData();
	spread->path=(SomeData *)newpath;
	
	 // make the outline around the inset, then lines to demarcate the tiles
	 // there are tilex*tiley pages, all pointing to the same page data
	newpath->pushEmpty(); // later could have a certain linestyle
	newpath->appendRect(insetleft,insetbottom, paper->media.maxx-insetleft-insetright,paper->media.maxy-insettop-insetbottom);
	int x,y;
	for (x=1; x<tilex; x++) {
		newpath->pushEmpty();
		newpath->append(insetleft+x*(paper->media.maxx-insetright-insetleft)/tilex, insettop);
		newpath->append(insetleft+x*(paper->media.maxx-insetright-insetleft)/tilex, insetbottom);
	}
	for (y=1; y<tiley; y++) {
		newpath->pushEmpty();
		newpath->append(insetleft, insetbottom+y*(paper->media.maxy-insetbottom-insettop)/tiley);
		newpath->append(insetright, insetbottom+y*(paper->media.maxy-insetbottom-insettop)/tiley);
	}
	
	 // setup spread->pagestack
	 // page width/height must map to proper area on page.
	 // makes rects with local origin in ll corner
	PathsData *ntrans;
	for (x=0; x<tilex; x++) {
		for (y=0; y<tiley; y++) {
			ntrans=new PathsData();//count of 1
			ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
			ntrans->FindBBox();
			ntrans->origin(flatpoint(insetleft+x*(paper->media.maxx-insetright-insetleft)/tilex,
									 insetbottom+y*(paper->media.maxy-insettop-insetbottom)/tiley));
			spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans));//ntrans count++
			ntrans->dec_count();//remove extra count
		}
	}
	
		
	 // make printer marks if necessary
	 //*** make this more responsible lengths:
	if (insetright>0 || insetleft>0 || insettop>0 || insetbottom>0) {
		spread->mask|=SPREAD_PRINTERMARKS;
		PathsData *marks=new PathsData();
		marks->flags|=SOMEDATA_LOCK_CONTENTS;
		if (insetleft>0) {
			marks->pushEmpty();
			marks->append(0,        paper->media.maxy-insettop);
			marks->append(insetleft*.9,paper->media.maxy-insettop);
			marks->pushEmpty();
			marks->append(0,        insetbottom);
			marks->append(insetleft*.9,insetbottom);
		}
		if (insetright>0) {
			marks->pushEmpty();
			marks->append(paper->media.maxx,          paper->media.maxy-insettop);
			marks->append(paper->media.maxx-.9*insetright,paper->media.maxy-insettop);
			marks->pushEmpty();
			marks->append(paper->media.maxx,          insetbottom);
			marks->append(paper->media.maxx-.9*insetright,insetbottom);
		}
		if (insetbottom>0) {
			marks->pushEmpty();
			marks->append(insetleft,0);
			marks->append(insetleft,.9*insetbottom);
			marks->pushEmpty();
			marks->append(paper->media.maxx-insetright,0);
			marks->append(paper->media.maxx-insetright,.9*insetbottom);
		}
		if (insettop>0) {
			marks->pushEmpty();
			marks->append(insetleft,paper->media.maxy);
			marks->append(insetleft,paper->media.maxy-.9*insettop);
			marks->pushEmpty();
			marks->append(paper->media.maxx-insetright,paper->media.maxy);
			marks->append(paper->media.maxx-insetright,paper->media.maxy-.9*insettop);
		}
		spread->marks=marks;
	}

	return spread;
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
int Singles::PaperFromPage(int pagenumber)
{ return pagenumber; }

//! Just return pagenumber, since 1 page==1 paper
int Singles::SpreadFromPage(int pagenumber)
{ return pagenumber; }

int Singles::SpreadFromPage(int layout, int pagenumber)
{
	if (layout==SINGLELAYOUT) return pagenumber;
	if (layout==PAGELAYOUT) return SpreadFromPage(pagenumber);
	return PaperFromPage(pagenumber); //paperlayout
}

//! Is singles, so 1 paper=1 page
int Singles::GetPagesNeeded(int npapers) 
{ return npapers; }

//! Is singles, so 1 page=1 paper
int Singles::GetPapersNeeded(int npages) 
{ return npages; } 

//! Is singles, so 1 page=1 spread
int Singles::GetSpreadsNeeded(int npages)
{ return npages; } 

int Singles::NumPageTypes()
{ return 1; }

//! Just return "Page".
const char *Singles::PageTypeName(int pagetype)
{ return pagetype==0 ? _("Page") : NULL; }

//! There is only one type of page, so return 0.
int Singles::PageType(int page)
{ return 0; }

//! There is only one type of spread, so return 0.
int Singles::SpreadType(int spread)
{ return 0; }


} // namespace Laidout

