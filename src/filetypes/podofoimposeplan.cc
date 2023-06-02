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
// Copyright (C) 2010-2011 by Tom Lechner
//


#include "podofoimposeplan.h"
#include "../language.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../core/utils.h"

#include <lax/transformmath.h>

#include <iostream>
#define DBG 


using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {



//--------------------------------- install Podofoimpose filter

//! Tells the Laidout application that there's a new filter in town.
void installPodofoFilter()
{
	PodofooutFilter *podofoout=new PodofooutFilter;
	podofoout->GetObjectDef();
	laidout->PushExportFilter(podofoout);
}


//------------------------------------ PodofoExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newPodofoExportConfig()
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Podofoimpose PLAN"))
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}


//------------------------------- PodofooutFilter --------------------------------------
/*! \class PodofooutFilter
 * \brief Output filter for Podofo files.
 */

PodofooutFilter::PodofooutFilter()
{
	flags=FILTER_MULTIPAGE;
}

const char *PodofooutFilter::VersionName()
{
	return _("Podofoimpose PLAN");
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *PodofooutFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("PodofoExportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"PodofoExportConfig");
	makestr(styledef->Name,_("Podofo Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a Podofoimpose PLAN file for external impositioning."));
	styledef->newfunc=newPodofoExportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}

static void podofodumppage(FILE *f,const double *mm,int source,int target)
{
	double rot=atan2(mm[1],mm[0]);
	double dx=mm[4];
	double dy=mm[5];

	fprintf(f,"%d; %d; %.10g; %.10g; %.10g;\n",
				source+1,   //source page
				target+1,   //target page
				rot, //rotation
				dx*72,  //horizontal displacement
				dy*72   //vertical displacement
		   );
}

//! Save the document as a Podofoimpose PLAN file.
/*! No actual objects are exported, only page positions. podofoimpose acts on external pdf files, and
 * arranges according to the PLAN.
 *
 * \todo Need to implement something for printer marks
 */
int PodofooutFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;
	
	Document *doc = out->doc;
	int layout    = out->layout;
	Group *limbo  = out->limbo;
	if (!filename) filename=out->filename;
	double scale  = 1; // ***

	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}

	if (!doc && !out->papergroup) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Export needs either a document or a custom Paper Group!"),ERROR_Fail);
		return 2;
	}

	 //we must be able to open the export file location...
	FILE *f = nullptr;
	char *file = nullptr;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".plan");
	} else file=newstr(filename);

	f = open_file_for_writing(file,0,&log);
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		log.AddMessage(_("Cannot open file for writing."),ERROR_Fail);
		delete[] file;
		return 3;
	}

	setlocale(LC_ALL,"C");

	 //figure out paper size and orientation

	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	double paperwidth, paperheight;
	PaperGroup *papergroup = out->papergroup;
	if (!papergroup) {
		PaperStyle *paper = doc->imposition->GetDefaultPaper();
		paperwidth  = paper->width;
		paperheight = paper->height;
	} else {
		paperwidth  = papergroup->papers.e[0]->box->paperstyle->width;
		paperheight = papergroup->papers.e[0]->box->paperstyle->height;
	}

	
	 // write out header
	if (!doc) {
		 //If no doc, then we are plastering the same limbo data across many papers
		int i = laidout->project->limbos.findindex(limbo);
		if (i >= 0)fprintf(f," # Limbo %d data\n",i);
		else fprintf(f," # Limbo data\n");
	
		fprintf(f,"$PageWidth=%.10g\n",paperwidth);
		fprintf(f,"$PageHeight=%.10g\n",paperheight);
		fprintf(f,"$ScaleFactor=%.10g\n\n",scale);

	} else {
		fprintf(f," # %s\n",doc->imposition->BriefDescription());
		fprintf(f," # %d papers\n\n",out->range.NumInRanges());

		double dx,dy;
		doc->imposition->GetDefaultPaperDimensions(&dx,&dy);
		fprintf(f,"$PageWidth=%.10g\n",dx*72);
		fprintf(f,"$PageHeight=%.10g\n",dy*72);
		fprintf(f,"$ScaleFactor=%.10g\n\n",scale);
	}

	 // Write out paper spreads....
	Spread *spread = nullptr;
	double m[6], mm[6], mmm[6];
	int p, c2, pg;
	int papernumber = 0;

	// if (out->evenodd == DocumentExportConfig::Even || out->evenodd == DocumentExportConfig::Odd) {
	// 	cerr <<"Tell devs to implement even odd for podofo export!"<<endl;
	// 	log.AddMessage(_("Cannot open file for writing."),ERROR_Fail);
	// 	delete[] file;
	// 	return 3;
	// }

	for (int c = out->range.Start(); c >= 0; c = out->range.Next()) {
		if (doc) spread = doc->imposition->Layout(layout,c);
		if (!papergroup && spread) papergroup = spread->papergroup;

		for (p = 0; p < (papergroup ? papergroup->papers.n : 1); p++) {

			// for plans, transforms are only 2 deep: 1 for the paper, 1 for the page

			// **** if there are any printer marks, they would be applied by outputting a special
			// pdf, which is then stamped onto final pdf from podofoimpose

			if (!spread) {
				 //we are only putting out a custom PaperGroup, so just write out the group transform
				transform_set(mm,72,0,0,72,0,0);
				transform_invert(mmm,papergroup->papers.e[p]->m());
				transform_mult(m,mmm,mm);
				podofodumppage(f,m,1,papernumber);

			} else {				
				 // for each page in spread..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
					pg = spread->pagestack.e[c2]->index;
					if (pg >= doc->pages.n) continue;
					
					 //1. set up paper transform
					transform_set(mm,72,0,0,72,0,0);
					if (papergroup) {
						transform_invert(mmm,papergroup->papers.e[p]->m());
					} else {
						transform_identity(mmm);
					}
					transform_mult(m,mmm,mm);

					 //2. apply page transform
					transform_mult(mmm,m,spread->pagestack.e[c2]->outline->m());

					podofodumppage(f,mmm,pg,papernumber);
				}
			}

			papernumber++;
		}
		if (spread) { delete spread; spread=NULL; }
	}
		
	
	fclose(f);
	setlocale(LC_ALL,"");
	delete[] file;
	return 0;
}




} // namespace Laidout

