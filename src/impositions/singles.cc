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
// Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/interfaces/pathinterface.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>

#include "../laidout.h"
#include "singles.h"
#include "../core/stylemanager.h"
#include "../language.h"


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {



//-------------------------- Singles ---------------------------------------------

/*! \class Singles
 * \brief For single pages not meant to be directly connected to other pages.
 */



Singles::Singles() : Imposition(_("Singles"))
{
	insetleft  = insetright  = insettop  = insetbottom  = 0;
	marginleft = marginright = margintop = marginbottom = 0;
	tilex = tiley = 1;
	gapx  = gapy  = 0;
	
	PaperStyle *paperstyle = laidout->GetDefaultPaper();
	if (paperstyle) paperstyle = static_cast<PaperStyle *>(paperstyle->duplicate());
	else paperstyle = new PaperStyle("letter", 8.5, 11.0, 0, 300, "in");
	SetPaperSize(paperstyle); //sets papergroup based on paperstyle
	paperstyle->dec_count();

	setPage();

	objectdef = stylemanager.FindDef("Singles");
	if (objectdef) objectdef->inc_count();
	else {
		objectdef = makeObjectDef();
		if (objectdef) stylemanager.AddObjectDef(objectdef, 1);
		// so this new objectdef should have a count of 2. The Style destructor removes
		// 1 count, and the stylemanager should remove the other
	}

	DBG cerr <<"imposition singles init"<<endl;
}

/*! Initialize with a particular "art board" like layout based on pgroup.
 */
Singles::Singles(PaperGroup *pgroup, bool absorb)
  : Singles()
{
	SetPaperGroup(pgroup); //inc's pgroup count
	if (pgroup && absorb) pgroup->dec_count();
}

Singles::~Singles()
{
	if (papergroup) papergroup->dec_count();
	if (custom_paper_packing) custom_paper_packing->dec_count();
	DBG cerr <<"--Singles destructor object "<<object_id<<endl;
}


//! Static imposition resource creation function.
/*! Returns NULL terminated list of default resources.
 */
ImpositionResource **Singles::getDefaultResources()
{
	ImpositionResource **r = new ImpositionResource*[2];
	r[0] = new ImpositionResource("Singles",
								  _("Singles"),
								  NULL,
								  _("Single pages per paper"),
								  "Singles",
								  NULL,0);
	r[1] = NULL;
	return r;
}


PaperGroup *Singles::GetPaperGroup(int layout, int index)
{
	if (layout == SINGLELAYOUT) return nullptr;

	return papergroup;
}

int Singles::SetPaperGroup(PaperGroup *ngroup)
{
	if (!ngroup) return 1;

	// Imposition::SetPaperGroup(ngroup);

	if (papergroup) papergroup->dec_count();
	papergroup = ngroup;
	if (papergroup) papergroup->inc_count();
	// if (papergroup->papers.n) {
	// 	if (paper) paper->dec_count();
	// 	paper = papergroup->papers.e[0]->box;
	// 	paper->inc_count();
	// }

	setPage();
	return 0;
}


/*! Default is to return papergroup->papers.e[0]->box->paperstyle, if it exists.
 * Returned value is an internal reference. If you use it much you must inc_count on it yourself.
 */
PaperStyle *Singles::GetDefaultPaper()
{
	if (papergroup
			&& papergroup->papers.n
			&& papergroup->papers.e[0]->box
			&& papergroup->papers.e[0]->box->paperstyle) 
		return papergroup->papers.e[0]->box->paperstyle;
	return nullptr;
}

//! Return default paper dimensions for informational purposes.
void Singles::GetDefaultPaperDimensions(double *x, double *y)
{
	*x = papergroup->papers.e[0]->box->paperstyle->w();
	*y = papergroup->papers.e[0]->box->paperstyle->h();
}

//! Return page dimensions for informational purposes.
void Singles::GetDefaultPageDimensions(double *x, double *y)
{
	*x = pagestyles.e[0]->w();
	*y = pagestyles.e[0]->h();
}

//! Just return "Singles".
const char *Singles::BriefDescription()
{
	return _("Singles");
}

void Singles::SetStylesFromPapergroup()
{
	for (int c = 0; c < papergroup->papers.n; c++) {
		PaperBoxData *data = papergroup->papers.e[c];
		PaperBox *box = data->box;

		if (c >= pagestyles.n) {
			RectPageStyle *p = new RectPageStyle();
			pagestyles.push(p);
			p->dec_count();
		}

		RectPageStyle *pagestyle = pagestyles.e[c];
		pagestyle->width  = data->w();
		pagestyle->height = data->h();
		if (box->Has(MarginBox)) {
			pagestyle->ml = box->margin.minx;
			pagestyle->mr = papergroup->papers.e[c]->w() - box->margin.maxx;
			pagestyle->mt = papergroup->papers.e[c]->h() - box->margin.maxy;
			pagestyle->mb = box->margin.miny;
		} else {
			pagestyle->ml = marginleft;
			pagestyle->mr = marginright;
			pagestyle->mt = margintop;
			pagestyle->mb = marginbottom;
		}

		pagestyle->pagetype = 0;
		pagestyle->RebuildOutline();
		pagestyle->RebuildMarginPath();
	}
}

void Singles::FixPageBleeds(int index, Page *page)
{
	if (!papergroup) return;

	int pindex = index % papergroup->papers.n;
	int group0 = (index / papergroup->papers.n) * papergroup->papers.n;
	int i = 0;

	PaperBoxData *paper = papergroup->papers.e[pindex];
	double p_inv[6];
	transform_invert(p_inv, paper->m());

	for (int c = 0; c < papergroup->papers.n; c++) {
		if (c == pindex) continue;

		//PaperBox *p = papergroup->papers.e[c]->box;
		PaperBoxData *pdata = papergroup->papers.e[c];

		PageBleed *bleed = nullptr;
		if (i < page->pagebleeds.n) bleed = page->pagebleeds.e[i];
		else {
			bleed = new PageBleed();
			page->pagebleeds.push(bleed);
		}

		bleed->index = group0 + c;
		bleed->page = (doc && bleed->index < doc->pages.n) ? doc->pages.e[bleed->index] : nullptr;
		transform_mult(bleed->matrix, pdata->m(), p_inv);

		i++;
	}

	while (page->pagebleeds.n > papergroup->papers.n-1)
		page->pagebleeds.remove(page->pagebleeds.n-1);
}

/*! Using the papergroup, create new pagestyle(s).
 *  Do nothing if papergroup is null.
 */
void Singles::setPage()
{
	if (!papergroup) {
		papergroup = new PaperGroup();
		double w = 0, h = 0;
		if (pagestyles.n) {

		}
		papergroup->AddPaper(w,h, 0,0);
	}

	for (int c = 0; c < papergroup->papers.n; c++) {
		PaperBox *box = papergroup->papers.e[c]->box;

		double oldl=0, oldr=0, oldt=0, oldb=0;
		double insetl = insetleft, insetr = insetright, insett = insettop, insetb = insetbottom;

		if (c < pagestyles.n) {
			oldl = pagestyles.e[c]->ml;
			oldr = pagestyles.e[c]->mr;
			oldt = pagestyles.e[c]->mt;
			oldb = pagestyles.e[c]->mb;
			pagestyles.e[c]->dec_count();
			pagestyles.e[c] = nullptr;
		} else {
			if (box->Has(MarginBox)) {
				oldl = box->margin.minx;
				oldr = papergroup->papers.e[c]->w() - box->margin.maxx;
				oldt = papergroup->papers.e[c]->h() - box->margin.maxy;
				oldb = box->margin.miny;
			} else {
				oldl = marginleft;
				oldr = marginright;
				oldt = margintop;
				oldb = marginbottom;
			}
		}

		if (box->Has(TrimBox)) {
			insetl = box->trim.minx;
			insetr = papergroup->papers.e[c]->w() - box->trim.maxx;
			insett = papergroup->papers.e[c]->h() - box->trim.maxy;
			insetb = box->trim.miny;
		}
		
		RectPageStyle *pagestyle = new RectPageStyle(RECTPAGE_LRTB);
		pagestyle->width  = box->media.maxx; // (box->media.maxx - insetl - insetr)/tilex;
		pagestyle->height = box->media.maxy; // (box->media.maxy - insett - insetb)/tiley;
		pagestyle->pagetype = 0;
		pagestyle->ml = oldl;
		pagestyle->mr = oldr;
		pagestyle->mt = oldt;
		pagestyle->mb = oldb;
		pagestyle->RebuildOutline();
		pagestyle->RebuildMarginPath();

		if (c < pagestyles.n) {
			pagestyles.e[c] = pagestyle;
		} else {
			pagestyles.push(pagestyle);
			pagestyle->dec_count();
		}

		// update paper
		papergroup->papers.e[c]->box->margin.minx = oldl;
		papergroup->papers.e[c]->box->margin.miny = oldb;
		papergroup->papers.e[c]->box->margin.maxx = papergroup->papers.e[c]->w() - oldr;
		papergroup->papers.e[c]->box->margin.maxy = papergroup->papers.e[c]->h() - oldt;

		papergroup->papers.e[c]->box->trim.minx = insetl;
		papergroup->papers.e[c]->box->trim.miny = insetb;
		papergroup->papers.e[c]->box->trim.maxx = papergroup->papers.e[c]->w() - insetr;
		papergroup->papers.e[c]->box->trim.maxy = papergroup->papers.e[c]->h() - insett;
	}
}

//! Return the default page style for that page.
/*! Default is to pagestyle->inc_count() then return pagestyle.
 */
PageStyle *Singles::GetPageStyle(int pagenum,int local)
{
	if (!papergroup) return nullptr;
	// if (!papergroup) {
	// 	if (!pagestyle) setPage();
	// 	if (!pagestyle) return nullptr;
	// 	if (local) {
	// 		PageStyle *ps = (PageStyle *)pagestyle->duplicate();
	// 		ps->flags |= PAGESTYLE_AUTONOMOUS;
	// 		return ps;
	// 	}
	// 	pagestyle->inc_count();
	// 	return pagestyle;
	// }

	// fill up pagestyles if necessary
	while (pagestyles.n < papergroup->papers.n) {
		RectPageStyle *rectstyle = new RectPageStyle();
		rectstyle->width  = papergroup->papers.e[papergroup->papers.n-1]->boxwidth();
		rectstyle->height = papergroup->papers.e[papergroup->papers.n-1]->boxheight();
		rectstyle->ml = marginleft  ;
		rectstyle->mr = marginright ;
		rectstyle->mt = margintop   ;
		rectstyle->mb = marginbottom;
		rectstyle->RebuildOutline();
		rectstyle->RebuildMarginPath();
		pagestyles.push(rectstyle);
		rectstyle->dec_count();
	}

	PageStyle *pstyle = pagestyles.e[pagenum % papergroup->papers.n];

	if (local) {
		PageStyle *ps = (PageStyle *)pstyle->duplicate();
		ps->flags |= PAGESTYLE_AUTONOMOUS;
		return ps;
	}

	pstyle->inc_count();
	return pstyle;
}

bool Singles::SetDefaultPageStyle(int index_in_spread, RectPageStyle *pstyle)
{
	if (papergroup && index_in_spread >= papergroup->papers.n) return false;
	if (!papergroup && index_in_spread > 0) return false;

	while (pagestyles.n < index_in_spread) pagestyles.push(nullptr);
	if (pagestyles.e[index_in_spread] != nullptr) {
		pstyle->inc_count();
		pagestyles.e[index_in_spread]->dec_count();
		pagestyles.e[index_in_spread] = pstyle;
	} else {
		pagestyles.e[index_in_spread] = pstyle;
		pstyle->inc_count();
	}

	if (papergroup) {
		PaperBoxData *data = papergroup->papers.e[index_in_spread];
		data->box->margin.setbounds(pstyle->ml, data->w() - pstyle->mr, pstyle->mb, data->h() - pstyle->mt);
	}

	return true;
}

/*! Set paper size, completely replacing previous papergroup with a single paper new size.
 * Duplicates npaper, not pointer tranfser.
 * 
 * Return 0 success, nonzero error.
 * 
 * Calls Imposition::SetPaperSize(npaper), then setPage().
 */
int Singles::SetPaperSize(PaperStyle *npaper)
{
 	if (!npaper) return 1;

	// if (Imposition::SetPaperSize(npaper)) return 1; // note this will REPLACE the old papergroup

	PaperStyle *newpaper = (PaperStyle *)npaper->duplicate();
	PaperBox *paper = new PaperBox(newpaper, true);
	PaperBoxData *newboxdata = new PaperBoxData(paper);
	paper->dec_count();

	if (papergroup) papergroup->dec_count();
	papergroup = new PaperGroup;
	papergroup->papers.push(newboxdata);
	papergroup->OutlineColor(1.0, 0, 0);  // default to red papergroup
	newboxdata->dec_count();

	setPage();
	return 0;
}

//! Set default left, right, top, bottom margins.
/*! Return 0 for success, or nonzero error.
 */
int Singles::SetDefaultMargins(double l,double r,double t,double b)
{
	marginleft  = l;
	marginright = r;
	margintop   = t;
	marginbottom= b;
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
void Singles::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"insetleft") || !strcmp(name,"insetl")) {
			DoubleAttribute(value,&insetleft);
		} else if (!strcmp(name,"insetright") || !strcmp(name,"insetr")) {
			DoubleAttribute(value,&insetright);
		} else if (!strcmp(name,"insettop") || !strcmp(name,"insett")) {
			DoubleAttribute(value,&insettop);
		} else if (!strcmp(name,"insetbottom") || !strcmp(name,"insetb")) {
			DoubleAttribute(value,&insetbottom);
		} else if (!strcmp(name,"marginleft") || !strcmp(name,"marginl")) {
			DoubleAttribute(value,&marginleft);
		} else if (!strcmp(name,"marginright") || !strcmp(name,"marginr")) {
			DoubleAttribute(value,&marginright);
		} else if (!strcmp(name,"margintop") || !strcmp(name,"margint")) {
			DoubleAttribute(value,&margintop);
		} else if (!strcmp(name,"marginbottom") || !strcmp(name,"marginb")) {
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

		// } else if (!strcmp(name,"defaultpagestyle")) {
		// 	if (pagestyle) pagestyle->dec_count();
		// 	pagestyle=new RectPageStyle(RECTPAGE_LRTB);
		// 	pagestyle->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"defaultpaperstyle")) {
			PaperStyle *paperstyle;
			paperstyle = new PaperStyle("Letter",8.5,11,0,300,"in");//TODO: ***should be global def
			paperstyle->dump_in_atts(att->attributes.e[c],flag,context);
			SetPaperSize(paperstyle);
			paperstyle->dec_count();

		} else if (!strcmp(name,"paper_layout") ||
				   !strcmp(name,"defaultpapers") // <- for backwards compat < 0.098
				  ) {
			if (papergroup) papergroup->dec_count();
			papergroup = new PaperGroup;
			papergroup->dump_in_atts(att->attributes.e[c],flag,context);
			// if (papergroup->papers.n) {
			// 	if (paper) paper->dec_count();
			// 	paper = papergroup->papers.e[0]->box;
			// 	paper->inc_count();
			// }
		}
	}

	setPage();
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
void Singles::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sname \"Some name\"   #this can be any string you want.\n",spc);
		fprintf(f,"%sdescription  Single sheets  #Some description beyond just a name.\n",spc);
		fprintf(f,"%s                            #name and description are used by the imposition resource mechanism\n",spc);
		fprintf(f,"%s#insets are regions of a paper not taken up by the page\n",spc);
		fprintf(f,"%sinsetleft   0   #The left inset from the side of a paper\n",spc);
		fprintf(f,"%sinsetright  0   #The right inset from the side of a paper\n",spc);
		fprintf(f,"%sinsettop    0   #The top inset from the side of a paper\n",spc);
		fprintf(f,"%sinsetbottom 0   #The bottom inset from the side of a paper\n",spc);
		fprintf(f,"%sgapx 0     #Gap between tiles horizontally\n",spc);
		fprintf(f,"%sgapy 0     #Gap between tiles vertically\n",spc);
		fprintf(f,"%stilex 1    #number of times to tile the page horizontally\n",spc);
		fprintf(f,"%stiley 1    #number of times to tile the page vertically\n",spc);
		fprintf(f,"%smarginlleft  0   #The default left page margin\n",spc);
		fprintf(f,"%smarginright  0   #The default right page margin\n",spc);
		fprintf(f,"%smargintop    0   #The default top page margin\n",spc);
		fprintf(f,"%smarginbottom 0   #The default bottom page margin\n",spc);
		fprintf(f,"%snumpages 3 #number of pages in the document. This is ignored on readin\n",spc);
		fprintf(f,"%spaper_layout  #optional definition of multiple pages per page spread\n", spc);
		papergroup->dump_out(f,indent+2,-1,NULL);
		//fprintf(f,"%sdefaultpagestyle #default page style\n",spc);
		//pagestyle->dump_out(f,indent+2,-1,NULL);
		return;
	}
	fprintf(f,"%smarginleft   %.10g\n",spc,marginleft);
	fprintf(f,"%smarginright  %.10g\n",spc,marginright);
	fprintf(f,"%smargintop    %.10g\n",spc,margintop);
	fprintf(f,"%smarginbottom %.10g\n",spc,marginbottom);
	fprintf(f,"%sinsetleft    %.10g\n",spc,insetleft);
	fprintf(f,"%sinsetright   %.10g\n",spc,insetright);
	fprintf(f,"%sinsettop     %.10g\n",spc,insettop);
	fprintf(f,"%sinsetbottom  %.10g\n",spc,insetbottom);
	fprintf(f,"%stilex %d\n",spc,tilex);
	fprintf(f,"%stiley %d\n",spc,tiley);

	if (numpages) fprintf(f,"%snumpages %d\n",spc,numpages);

	if (pagestyles.n) {
		//Attribute *att2 = att->pushSubAtt("pagestyles");
		fprintf(f,"%spagestyles\n",spc);
		for (int c=0; c < pagestyles.n; c++) {
			fprintf(f,"%s  pagestyle\n",spc);
			pagestyles.e[c]->dump_out(f,indent+4,0,context);
		}
	}

	if (papergroup) {
		fprintf(f,"%spaper_layout\n",spc);
		papergroup->dump_out(f,indent+2,0,context);
	}
}

//! Duplicate this, or fill in this attributes.
Value *Singles::duplicate()
{
	Singles *sn;
	sn = new Singles(dynamic_cast<PaperGroup*>(papergroup->duplicate(nullptr)), true);
	if (custom_paper_packing) {
		sn->custom_paper_packing = dynamic_cast<PaperGroup*>(custom_paper_packing->duplicate(nullptr));
	}

	if (objectdef) {
		objectdef->inc_count();
		if (sn->objectdef) sn->objectdef->dec_count();
		sn->objectdef=objectdef;
	}

	if (papergroup && papergroup->papers.n) {
		for (int c=0; c < papergroup->papers.n; c++) {
			if (c < sn->pagestyles.n && sn->pagestyles.e[c]) sn->pagestyles.e[c]->dec_count();

			RectPageStyle *dup = dynamic_cast<RectPageStyle*>(pagestyles.e[c]->duplicate());
			if (c < sn->pagestyles.n) sn->pagestyles.e[c] = dup;
			else {
				sn->pagestyles.push(dup);
				dup->dec_count();
			}
		}
	}

	sn->marginleft   = marginleft;
	sn->marginright  = marginright;
	sn->margintop    = margintop;
	sn->marginbottom = marginbottom;
	sn->insetleft    = insetleft;
	sn->insetright   = insetright;
	sn->insettop     = insettop;
	sn->insetbottom  = insetbottom;
	sn->tilex        = tilex;
	sn->tiley        = tiley;

	return sn;  
}

//! The newfunc for Singles instances.
Value *NewSingles()
{ 
	Singles *s = new Singles;
	return s;
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

ObjectDef *Singles::makeObjectDef()
{
	return makeSinglesObjectDef();
}

//! Make an instance of the Singles imposition objectdef.
/*  Required by Style, this defines the various names for fields relevant to Singles,
 *  basically just the inset[lrtb], plus the standard Imposition npages and npapers.
 *  Two of the fields would be the pagestyle and paperstyle. They have their own
 *  objectdefs stored in a StyleManager.
 *
 *  Returns a new ObjectDef with a count of 1.
 */
ObjectDef *makeSinglesObjectDef()
{
	ObjectDef *sd=new ObjectDef(NULL,"Singles",
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


//! Ensure that each page has a proper pagestyle and bleed information.
/*! This is called when pages are added or removed. It replaces the pagestyle for
 *  each page with the pagestyle returned by GetPageStyle(c,0).
 */
int Singles::SyncPageStyles(Document *doc,int start,int n, bool shift_within_margins)
{
	int status = Imposition::SyncPageStyles(doc,start,n, shift_within_margins);

	this->doc = doc;
	for (int c = start; c < doc->pages.n; c++) {
		FixPageBleeds(c,doc->pages.e[c]);
	}

	return status;
}


//! Return outline of page in page coords. 
SomeData *Singles::GetPageOutline(int pagenum,int local)
{
	PathsData *newpath = new PathsData();//count==1
	newpath->style |= PathsData::PATHS_Ignore_Weights;

	PageStyle *pstyle = GetPageStyle(pagenum, 0);

	newpath->appendRect(0,0,pstyle->w(),pstyle->h());
	newpath->maxx = pstyle->w();
	newpath->maxy = pstyle->h();
	pstyle->dec_count(); //note: GetPageStyle incs count of non-local
	
	return newpath;
}

//! Return outline of page margin in page coords. 
/*! Calling code should call dec_count() on the returned object when it is no longer needed.
 */
SomeData *Singles::GetPageMarginOutline(int pagenum,int local)
{
	if (!papergroup || !papergroup->papers.n) return nullptr;
	//if (!doc || pagenum >= doc->pages.n) return nullptr;

	int which = pagenum % papergroup->papers.n;

	if (pagestyles.n < papergroup->papers.n) SetStylesFromPapergroup();
	if (!pagestyles.e[which]->margin) pagestyles.e[which]->RebuildMarginPath();
	pagestyles.e[which]->margin->inc_count();
	return pagestyles.e[which]->margin;
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
	if (!papergroup || !papergroup->papers.n) return SingleLayout(whichpage);

	Spread *spread = new Spread();
	spread->spreadtype = 1;
	spread->style = SPREAD_PAGE;
	spread->mask = SPREAD_PATH|SPREAD_PAGES|SPREAD_MINIMUM|SPREAD_MAXIMUM;
	spread->spread_index = whichpage;

	spread->papergroup = papergroup;
	papergroup->inc_count();
	
	 // define max/min points for spread editor
	PaperBox *pp = papergroup->papers.e[0]->box;
	spread->minimum = flatpoint(pp->media.maxx/5,  pp->media.maxy/2);
	pp = papergroup->papers.e[papergroup->papers.n-1]->box;
	spread->maximum = flatpoint(pp->media.maxx*4/5,pp->media.maxy/2);

	 // fill spread with paper and page outline
	PathsData *newpath = new PathsData();
	newpath->style |= PathsData::PATHS_Ignore_Weights;
	spread->path = (SomeData *)newpath;
	
	int page_start = (whichpage / papergroup->papers.n) * papergroup->papers.n;

	for (int c=0; c<papergroup->papers.n; c++)
	{
		PaperBoxData *pbox = papergroup->papers.e[c];

		PathsData *ntrans = new PathsData();//count of 1
		ntrans->appendRect(&(pbox->box->media));
		ntrans->FindBBox();
		ntrans->origin(pbox->origin());
		SomeData *margin = pagestyles.e[c]->margin;
		// margin->origin(pbox->origin());
		spread->pagestack.push(new PageLocation(page_start + c, NULL, ntrans, margin)); //ntrans and margin get count++
		ntrans->dec_count();//remove extra count

		newpath->pushEmpty();
		newpath->append(pbox->transformPoint(flatpoint(pbox->box->media.minx, pbox->box->media.miny)));
		newpath->append(pbox->transformPoint(flatpoint(pbox->box->media.maxx, pbox->box->media.miny)));
		newpath->append(pbox->transformPoint(flatpoint(pbox->box->media.maxx, pbox->box->media.maxy)));
		newpath->append(pbox->transformPoint(flatpoint(pbox->box->media.minx, pbox->box->media.maxy)));
		newpath->close();
	}

	newpath->FindBBox();

	return spread;
}

//! Return a paper spread with 1 page on it, using the inset values.
/*! The path created here is one path for the paper, and another for the possibly inset page.
 */
Spread *Singles::PaperLayout(int whichpaper)
{
	Spread *spread = new Spread();
	spread->spreadtype = 1;
	spread->style = SPREAD_PAPER;
	spread->mask = SPREAD_PATH | SPREAD_PAGES | SPREAD_MINIMUM | SPREAD_MAXIMUM;
	spread->spread_index = whichpaper;

	int paperi = (papergroup ? whichpaper % papergroup->papers.n : 0);
	PaperBox *box = nullptr; //paper;

	if (papergroup) {
		spread->papergroup = new PaperGroup(papergroup->papers.e[paperi]->box->paperstyle);
		box = papergroup->papers.e[paperi]->box;
	}
	
	// define max/min points for spread editor
	spread->minimum = flatpoint(box->media.maxx/5,   box->media.maxy/2);
	spread->maximum = flatpoint(box->media.maxx*4/5, box->media.maxy/2);

	// fill spread with paper and page outline
	PathsData *newpath = new PathsData();
	newpath->style |= PathsData::PATHS_Ignore_Weights;
	spread->path = (SomeData *)newpath;
	
	// make the outline around the inset, then lines to demarcate the tiles
	// there are tilex*tiley pages, all pointing to the same page data
	newpath->pushEmpty(); // later could have a certain linestyle
	newpath->appendRect(insetleft,insetbottom, box->media.maxx-insetleft-insetright, box->media.maxy-insettop-insetbottom);
	int x,y;
	for (x=1; x<tilex; x++) {
		newpath->pushEmpty();
		newpath->append(insetleft+x*(box->media.maxx-insetright-insetleft)/tilex, insettop);
		newpath->append(insetleft+x*(box->media.maxx-insetright-insetleft)/tilex, insetbottom);
	}
	for (y=1; y<tiley; y++) {
		newpath->pushEmpty();
		newpath->append(insetleft,  insetbottom+y*(box->media.maxy-insetbottom-insettop)/tiley);
		newpath->append(insetright, insetbottom+y*(box->media.maxy-insetbottom-insettop)/tiley);
	}

	newpath->FindBBox();
	
	// setup spread->pagestack
	// page width/height must map to proper area on page.
	// makes rects with local origin in ll corner
	PathsData *ntrans;
	RectPageStyle *pagestyle = pagestyles.e[whichpaper % papergroup->papers.n];
	for (x=0; x<tilex; x++) {
		for (y=0; y<tiley; y++) {
			ntrans=new PathsData();//count of 1
			ntrans->appendRect(0,0, pagestyle->w(),pagestyle->h());
			ntrans->FindBBox();
			ntrans->origin(flatpoint(insetleft  +x*(box->media.maxx-insetright-insetleft)  /tilex,
									 insetbottom+y*(box->media.maxy-insettop  -insetbottom)/tiley));
			spread->pagestack.push(new PageLocation(whichpaper,NULL,ntrans));//ntrans count++
			ntrans->dec_count();//remove extra count
		}
	}
	
		
	// make printer marks if necessary
	//*** make this more responsible lengths:
	if (insetright>0 || insetleft>0 || insettop>0 || insetbottom>0) {
		spread->mask|=SPREAD_PRINTERMARKS;
		PathsData *marks=new PathsData();
		ScreenColor color(0.,0.,0.,1.);
		marks->line(2./72, -1, -1, &color);
		marks->Id(_("Cut marks"));
		marks->flags |= SOMEDATA_LOCK_CONTENTS|SOMEDATA_UNSELECTABLE;

		if (insetleft>0) {
			marks->pushEmpty();
			marks->append(0,        box->media.maxy-insettop);
			marks->append(insetleft*.9,box->media.maxy-insettop);
			marks->pushEmpty();
			marks->append(0,        insetbottom);
			marks->append(insetleft*.9,insetbottom);
		}
		if (insetright>0) {
			marks->pushEmpty();
			marks->append(box->media.maxx,          box->media.maxy-insettop);
			marks->append(box->media.maxx-.9*insetright,box->media.maxy-insettop);
			marks->pushEmpty();
			marks->append(box->media.maxx,          insetbottom);
			marks->append(box->media.maxx-.9*insetright,insetbottom);
		}
		if (insetbottom>0) {
			marks->pushEmpty();
			marks->append(insetleft,0);
			marks->append(insetleft,.9*insetbottom);
			marks->pushEmpty();
			marks->append(box->media.maxx-insetright,0);
			marks->append(box->media.maxx-insetright,.9*insetbottom);
		}
		if (insettop>0) {
			marks->pushEmpty();
			marks->append(insetleft,box->media.maxy);
			marks->append(insetleft,box->media.maxy-.9*insettop);
			marks->pushEmpty();
			marks->append(box->media.maxx-insetright,box->media.maxy);
			marks->append(box->media.maxx-insetright,box->media.maxy-.9*insettop);
		}

		marks->FindBBox();
		spread->marks = marks;
	}

	return spread;
}

//! Just return pagenumber, since 1 page==1 paper
int Singles::PaperFromPage(int pagenumber)
{
	return pagenumber;
}

int Singles::PapersPerPageSpread()
{
	if (!papergroup || papergroup->papers.n == 0) return 1;
	return papergroup->papers.n;
}

//! Return the page layout spread, which is either pagenumber, or if papergroup != null is pagenumber/(papers in papergroup).
int Singles::SpreadFromPage(int pagenumber)
{
	if (papergroup && papergroup->papers.n) return pagenumber / papergroup->papers.n;
	return pagenumber;
}

int Singles::SpreadFromPage(int layout, int pagenumber)
{
	if (layout == SINGLELAYOUT) return pagenumber;
	if (layout == PAGELAYOUT) return SpreadFromPage(pagenumber);
	return PaperFromPage(pagenumber); //paperlayout
}

//! Is singles, so 1 paper=1 page
int Singles::GetPagesNeeded(int npapers) 
{
	if (papergroup && papergroup->papers.n) return npapers * papergroup->papers.n;
	return npapers;
}

//! Is singles, so 1 page=1 paper
int Singles::GetPapersNeeded(int npages) 
{
	if (papergroup && papergroup->papers.n) return 1 + (npages-1) / papergroup->papers.n;
	return npages;
}

/*! Page spread is all papers in papergroup. */
int Singles::GetSpreadsNeeded(int npages)
{
	if (npages <= 0) return 0;
	if (papergroup && papergroup->papers.n) return 1 + (npages-1) / papergroup->papers.n;
	return npages;
} 

int Singles::GetNumInPaperGroupForSpread(int layout, int spread)
{
	if (layout == SINGLELAYOUT) return 1;
	return papergroup->papers.n;
}

int Singles::NumPageTypes()
{ 
	if (!papergroup) return 1;
	return papergroup->papers.n;
}

int Singles::NumPapers()
{
	return numpages;
	// if (!papergroup) return numpages;
	// return (numpages - 1) / papergroup->papers.n + 1;
}

int Singles::NumSpreads(int layout)
{
	if (layout == PAPERLAYOUT)        return NumPapers();
	if (layout == SINGLELAYOUT)       return NumPages();
	if (layout == LITTLESPREADLAYOUT) return NumPages();
	if (layout == PAGELAYOUT) {
		if (!papergroup) return numpages;
		return (numpages - 1) / papergroup->papers.n + 1;
	}
	return 0;
}

//! Just return "Page".
const char *Singles::PageTypeName(int pagetype)
{ return pagetype==0 ? _("Page") : NULL; }

//! There is only one type of page, so return 0.
int Singles::PageType(int page)
{
	if (!papergroup) return 0;
	return page % papergroup->papers.n;
}

//! There is only one type of spread, so return 0.
int Singles::SpreadType(int spread)
{ return 0; }


ImpositionInterface *Singles::Interface()
{
	return nullptr;
}

} // namespace Laidout

