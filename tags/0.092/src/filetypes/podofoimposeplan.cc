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
// Copyright (C) 2010 by Tom Lechner
//


#include "podofoimposeplan.h"
#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../utils.h"

#include <lax/transformmath.h>

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



//--------------------------------- install Podofoimpose filter

//! Tells the Laidout application that there's a new filter in town.
void installPodofoFilter()
{
	PodofooutFilter *podofoout=new PodofooutFilter;
	podofoout->GetStyleDef();
	laidout->PushExportFilter(podofoout);
}


//------------------------------------ PodofoExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Style *newPodofoExportConfig(StyleDef*)
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Podofoimpose PLAN"))
			d->filter=laidout->exportfilters.e[c];
	}
	return d;
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
StyleDef *PodofooutFilter::GetStyleDef()
{
	StyleDef *styledef;
	styledef=stylemanager.FindDef("PodofoExportConfig");
	if (styledef) return styledef; 

	styledef=makeStyleDef();
	makestr(styledef->name,"PodofoExportConfig");
	makestr(styledef->Name,_("Podofo Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a Podofoimpose PLAN file for external impositioning."));
	styledef->newfunc=newPodofoExportConfig;

	stylemanager.AddStyleDef(styledef);
	styledef->dec_count();

	return styledef;
}

//! Internal function to dump out the obj if it is an ImageData.
/*! \todo deal with SomeDataRef
 * \todo *** test EpsData out
 */
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
 * error_ret is appended to if possible.
 *    
 * \todo Need to implement something for printer marks
 */
int PodofooutFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;
	
	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	Group *limbo  =out->limbo;
	PaperGroup *papergroup=out->papergroup;
	if (!filename) filename=out->filename;
	double scale  =1; // ***

	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		if (error_ret) appendline(*error_ret,_("Nothing to export!"));
		return 1;
	}

	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			if (error_ret) appendline(*error_ret,_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".plan");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,error_ret);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		if (error_ret) appendline(*error_ret,_("Cannot open file for writing."));
		delete[] file;
		return 3;
	}

	setlocale(LC_ALL,"C");

	 //figure out paper size and orientation
	int c;

	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	double paperwidth,paperheight;
	paperwidth=papergroup->papers.e[0]->box->paperstyle->width;
	paperheight=papergroup->papers.e[0]->box->paperstyle->height;

	
	 // write out header
	if (!doc) {
		 //If no doc, then we are plastering the same limbo data across many papers
		int i=laidout->project->limbos.findindex(limbo);
		if (i>=0)fprintf(f," # Limbo %d data\n",i);
		else fprintf(f," # Limbo data\n");
	
		fprintf(f,"$PageWidth=%.10g\n",paperwidth);
		fprintf(f,"$PageHeight=%.10g\n",paperheight);
		fprintf(f,"$ScaleFactor=%.10g\n\n",scale);

	} else {
		fprintf(f," # %s\n",doc->imposition->BriefDescription());
		fprintf(f," # %d papers\n\n",start-end+1);

		double dx,dy;
		doc->imposition->GetDimensions(0,&dx,&dy);
		fprintf(f,"$PageWidth=%.10g\n",dx*72);
		fprintf(f,"$PageHeight=%.10g\n",dy*72);
		fprintf(f,"$ScaleFactor=%.10g\n\n",scale);
	}

	 // Write out paper spreads....
	Spread *spread=NULL;
	double m[6],mm[6],mmm[6];
	int p,c2,pg;
	int papernumber=0;

	for (c=start; c<=end; c++) {
		if (doc) spread=doc->imposition->Layout(layout,c);
		for (p=0; p<papergroup->papers.n; p++) {

			 //for plans, transforms are only 2 deep: 1 for the paper, 1 for the page

			 // **** if there are any printer marks, they would be applied by outputting a special
			 // pdf, which is then stamped onto final pdf from podofoimpose

			if (!spread) {
				 //we are only putting out a group, so just write out the group transform
				transform_set(mm,72,0,0,72,0,0);
				transform_invert(mmm,papergroup->papers.e[p]->m());
				transform_mult(m,mmm,mm);
				podofodumppage(f,m,1,papernumber);

			} else {				
				 // for each page in spread..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
					pg=spread->pagestack.e[c2]->index;
					if (pg>=doc->pages.n) continue;
					
					 //1. set up paper transform
					transform_set(mm,72,0,0,72,0,0);
					transform_invert(mmm,papergroup->papers.e[p]->m());
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





////! Export a plan file that can be fed into podofoimpose for external pdf impositioning.
///*! \ingroup extras
// *
// * Warning! this will clobber anything already in file!!
// *
// * thimanypages is how many pages to pretend we have for purposes of the plan file.
// * This may be different than the number of pages we actually have.
// * If thismanypages<1, then use numdocpages instead.
// *
// * scale is any further scaling to perform on pages. If working on a totally internal
// * Laidout document, 1 is preferred. You might want it different if you are artificially
// * acting on an external pdf, for instance.
// *
// * Returns 0 for success, or nonzero for error.
// */
//int ExportPlan(Imposition *imposition, const char *file, int thismanypages, double scale)
//{ ************************************************
//	FILE *f=fopen(file,"w");
//	if (!f) return 1;
//
//	Imposition *imp=(Imposition*)imposition->duplicate();
//
//	if (thismanypages<1) thismanypages=imp->NumPages();
//	imp->NumPages(thismanypages);
//
//	if (scale<=0) scale=1;
//
//	// podofoimpose plan files have format:
//	// $PageWidth=612   <-- in points, 1=1/72 inch
//	// $PageHeight=792
//	// $ScaleFactor=1   <-- only one scale factor is allowed for all pages
//	//  # any number of page placement statements:
//	// source page; target page; rotation; horizontal translation; vertical translation; 
//	//
//	// Rotation is applied around the lower left corner of a page, after h and v translation
//	//
//	// A plan file has looping possibilities, but they are not so very useful for signatures as
//	// defined here, with arbitrary folds.
//
//	int i;
//	double rot,dx,dy;
//	Spread *spread;
//
//	fprintf(f," # %s\n",imp->BriefDescription());
//	fprintf(f," # %d pages\n\n",imp->NumPages());
//
//	imp->GetDimensions(0,&dx,&dy);
//	fprintf(f,"$PageWidth=%.10g\n",dx*72);
//	fprintf(f,"$PageHeight=%.10g\n",dy*72);
//	fprintf(f,"$ScaleFactor=%.10g\n\n",scale);
//
//	for (int c=0; c<imp->NumSpreads(PAPERLAYOUT); c++) {
//		spread=imp->Layout(PAPERLAYOUT,c);
//
//		fprintf(f," # paper spread #%d\n",c+1);
//		for (int c2=0; c2<spread->pagestack.n; c2++) {
//			i=spread->pagestack.e[c2]->index;
//			if (i<0 || i>=thismanypages) continue;
//
//			rot=atan2(spread->pagestack.e[c2]->outline->m(1),spread->pagestack.e[c2]->outline->m(0));
//			dx=spread->pagestack.e[c2]->outline->m(4);
//			dy=spread->pagestack.e[c2]->outline->m(5);
//
//			fprintf(f,"%d; %d; %.10g; %.10g; %.10g;\n",
//						i,   //source page
//						c,   //target page
//						rot, //rotation
//						dx,  //horizontal displacement
//						dy   //vertical displacement
//				   );
//		}
//		delete spread;
//	}
//
//	fclose(f);
//	delete imp;
//
//	return 0;
//}

