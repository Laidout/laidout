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
// Copyright (c) 2004-2007 Tom Lechner
//


#include "../language.h"
#include "../laidout.h"
#include "psout.h"

#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/somedataref.h>
#include <lax/transformmath.h>
#include <lax/fileutils.h>

#include "../version.h"
#include "../core/utils.h"
#include "psfilters.h"
#include "../filetypes/filefilters.h"
#include "../core/laidoutdefs.h"

#include "psgradient.h"
#include "psimage.h"
#include "psimagepatch.h"
#include "pscolorpatch.h"
#include "pspathsdata.h"
#include "pseps.h"

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxInterfaces;



namespace Laidout {




/*! \defgroup postscript Postscript
 *
 * Various things to help output Postscript language level 3 files.
 *
 * While outputting ps, functions can access the current transformation matrix
 * as seen by the postscript interpreter using psCTM(). This is useful, for intstance,
 * for effects where something must be generated at the paper's dpi like an
 * ImagePatch. Also can be used to aid setup for the kind of linewidth that is supposed 
 * to be relative to the paper, rather than the ctm.
 *
 * \todo *** could have a psSpreadOut function to do a 1-off of any old spread, optionally
 *   fit to a supplied box
 * \todo there is way too much code duplication between psout and epsout... should combine the two
 * \todo should probably isolate the psCtm() related functions to be more useful to filters
 */


//--------------------  ps CTM helpers --------------------------

static double psdpi=1;

//! This should be what is the current paper's preferred dpi.
/*! \ingroup postscript */
double psDpi()
{ return psdpi; }

//! Set what should be what is the current paper's preferred dpi.
/*! \ingroup postscript */
double psDpi(double n)
{ return psdpi=n; }

static double *psctm=NULL;
PtrStack<double> psctms(2);

//! New ps ctm=m*oldctm.
/*! \ingroup postscript */
void psConcat(const double *m)
{
	double *mm=transform_mult(NULL,m,psctm);
	delete[] psctm;
	psctm=mm;
	DBG cerr << "ctm concat: "; dumpctm(psctm);
}

//! Initialize to identity and return ctm.
/*! \ingroup postscript 
 * If psctm==NULL, then create new. This should be done, for instance, by
 * any output stuff like pdf that makes use of psctm, which is static to psout.cc.
 *
 * Also flushes psctms stack.
 */
double *psCtmInit()
{
	psctms.flush();
	psctm=transform_identity(psctm);
	return psctm;
}

//! New ps ctm=m*oldctm.
/*! \ingroup postscript */
void psConcat(double a,double b,double c,double d,double e,double f)
{
	double m[6];
	transform_set(m,a,b,c,d,e,f);
	double *mm=transform_mult(NULL,m,psctm);
	delete[] psctm;
	psctm=mm;
}

//! Return the current ps ctm.
/*! \ingroup postscript */
double *psCTM() 
{ return psctm; }

//! Push ctm on the ps ctm stack.
/*! \ingroup postscript */
void psPushCtm()
{
	psctms.push(psctm);
	psctm=transform_identity(NULL);
	transform_copy(psctm,psctms.e[psctms.n-1]);
}

//! Pop 1 off the ps ctm stack.
/*! \ingroup postscript */
void psPopCtm()
{
	delete[] psctm;
	psctm=psctms.pop();
	if (!psctm) psctm=transform_identity(NULL); //*** this is an error to be here!
}

//! Flush the running stack of ps ctms.
/*! \ingroup postscript */
void psFlushCtms()
{ psctms.flush(); }



//----------------------------- ps out ---------------------------------------

//! Internal function to dump out the obj in postscript. Called by psout().
/*! \ingroup postscript
 * It is assumed that the transform of the object is applied here, rather than
 * before this function is called.
 *
 * Should be able to handle gradients, bez color patches, paths, and images
 * without significant problems, EXCEPT for the lack of decent transparency handling.
 *
 * \todo *** must be able to do color management
 * \todo *** need integration of global units, assumes inches now. generally must work
 *    out what shall be the default working units...
 */
void psdumpobj(FILE *f,LaxInterfaces::SomeData *obj)
{
	if (!obj) return;
	
	 // push axes
	psPushCtm();
	psConcat(obj->m());
	fprintf(f,"gsave\n"
			  "[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
	
	if (!strcmp(obj->whattype(),"Group")) {
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) psdumpobj(f,g->e(c)); 
		
	} else if (!strcmp(obj->whattype(),"ImagePatchData")) {
		psImagePatch(f,dynamic_cast<ImagePatchData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"ImageData")) {
		psImage(f,dynamic_cast<ImageData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"GradientData")) {
		psGradient(f,dynamic_cast<GradientData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		psColorPatch(f,dynamic_cast<ColorPatchData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"PathsData")) {
		psPathsData(f,dynamic_cast<PathsData *>(obj));

	} else if (!strcmp(obj->whattype(),"EpsData")) {
		psEps(f,dynamic_cast<EpsData *>(obj));

	}
	
	 // pop axes
	fprintf(f,"grestore\n\n");
	psPopCtm();
}

//! Output a postscript clipping path from outline.
/*! \ingroup postscript
 * outline can be a group of PathsData, a SomeDataRef to a PathsData, 
 * or a single PathsData.
 *
 * Non-PathsData elements in a group does not break the printing.
 * Those extra objects are just ignored.
 *
 * Returns the number of single paths interpreted.
 *
 * If iscontinuing!=0, then doesn't write 'clip' at the end.
 *
 * \todo *** currently, uses all points (vertex and control points)
 * in the paths as a polyline, not as the full curvy business 
 * that PathsData are capable of. when ps output of paths is 
 * actually more implemented, this will change..
 */
int psSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing)//iscontinuing=0
{
	PathsData *path=dynamic_cast<PathsData *>(outline);

	 //If is not a path, but is a reference to a path
	if (!path && dynamic_cast<SomeDataRef *>(outline)) {
		SomeDataRef *ref;
		 // skip all nested SomeDataRefs
		do {
			ref=dynamic_cast<SomeDataRef *>(outline);
			if (ref) outline=ref->thedata;
		} while (ref);
		if (outline) path=dynamic_cast<PathsData *>(outline);
	}

	int n=0; //the number of objects interpreted
	
	 // If is not a path, and is not a ref to a path, but is a group,
	 // then check that its elements 
	if (!path && dynamic_cast<Group *>(outline)) {
		Group *g=dynamic_cast<Group *>(outline);
		SomeData *d;
		double m[6];
		for (int c=0; c<g->n(); c++) {
			d=g->e(c);
			 //add transform of group element
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					d->m(0), d->m(1), d->m(2), d->m(3), d->m(4), d->m(5)); 
			n+=psSetClipToPath(f,g->e(c),1);
			transform_invert(m,d->m());
			 //reverse the transform
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
		}
	}
	
	if (!path) return n;
	
	 // finally append to clip path
	Coordinate *start,*p;
	for (int c=0; c<path->paths.n; c++) {
		start=p=path->paths.e[c]->path;
		if (!p) continue;
		do { p=p->next; } while (p && p!=start);
		if (p==start) { // only include closed paths
			n++;
			p=start;
			do {
				fprintf(f,"%.10g %.10g ",p->x(),p->y());
				if (p==start) fprintf(f,"moveto\n");
				else fprintf(f,"lineto\n");
				p=p->next;	
			} while (p && p!=start);
			fprintf(f,"closepath\n");
		}
	}
	
	if (n && !iscontinuing) {
		//fprintf(f,".1 setlinewidth stroke\n");
		fprintf(f,"clip\n");
	}
	return n;
}

//! Output a postscript file.
/*! \ingroup postscript
 * Does not open or close f. This sets up the ctm as accessible through psCTM(),
 * and flushes the ctm stack.
 *
 * Return 0 for no errors, nonzero for errors.
 * 
 * \todo *** this does not currently handle pages that bleed their contents
 *   onto other pages correctly. it bleeds here by paper spread, rather than page spread
 * \todo *** ps doc tag For: ????
 * \todo *** for tiled pages, or multiples of same object each instance is
 *   rendered independently right now. should make a function to display such
 *   things, thus reduce ps file size substantially..
 * \todo for embedded EPS that include specific resources, extensions, or language level, these must be
 *   mentioned in the postscript file's comments...
 * \todo *** fix non-paper layout media/paper type
 * \todo DocumentMedia comment must be enhanced. Types of media are according to each paper
 *   in papergroup
 */
int psout(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	Document *doc =out->doc;
	int layout    =out->layout;
	Group *limbo  =out->limbo;
	if (!filename) filename=out->filename;

	 //we must have something to export...
	if (!doc && !limbo) {
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}


	 //figure out paper size and orientation
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	 // The ps Orientation comment determines how onscreen viewers will show 
	 // pages. This can be overridden by the %%PageOrientation: comment
	int landscape = 0, plandscape = 0;
	double paperwidth = 0, paperheight = 0;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	PaperGroup *papergroup = out->papergroup;
	Spread *spread = nullptr;
	if (!papergroup) {
		int pg = out->range.Start();
		spread = doc->imposition->Layout(out->layout,pg);
		papergroup = spread->papergroup;
	}
	if (papergroup) {
		PaperStyle *defaultpaper = nullptr;
		defaultpaper = papergroup->papers.e[0]->box->paperstyle;
		landscape   = defaultpaper->landscape();
		paperwidth  = defaultpaper->width;
		paperheight = defaultpaper->height;
	} else if (spread) {
		paperwidth  = spread->path->boxwidth();
		paperheight = spread->path->boxheight();
	} else if (out->limbo) {
		paperwidth  = out->limbo->boxwidth();
		paperheight = out->limbo->boxheight();
	}
	if (spread) { delete spread; spread = nullptr; }
	papergroup = out->papergroup;

	if (paperwidth <= 0 || paperheight <= 0) {
		DBG cerr <<" bad bounds, aborting. w x h: "<<paperwidth <<" x "<<paperheight<<endl;
		log.AddError(_("Bad bounds for export!"));
		return 4;
	}

	int totalnumpages = out->NumOutputAreas();


	// if (!papergroup) {
	// 	log.AddError(_("There must be a Paper Group for this filter."),ERROR_Fail);
	// 	return 2;
	// }

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
		appendstr(file,".ps");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,&log);
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}

	setlocale(LC_ALL,"C");


	 //Basically, postscript documents following the ps document structure 
	 //guidelines are structured like this:
	 //-----------------header--------------
	 // %!PS-Adobe-3.0
	 // %%Pages: 3
	 // ...more header comments until a line not beginning with '%' or line with %%EndComments
	 // %%EndComments
	 // --------(epsi preview data goes here)-------
	 // ---------------procedure setup----------
	 // %%BeginProlog
	 // ...
	 // %%BeginResource
	 // ...
	 // %%EndResource
	 // %%EndProlog
	 // -----------doc setup--------------
	 // %%BeginSetup
	 // ...
	 // %%EndSetup
	 // ---------------pages--------------------
	 // %%Page:
	 // %%BeginPageSetup
	 // ...
	 // %%EndPageSetup
	 // ...
	 // %%PageTrailer
	 // %%Trailer
	 // ...
	 // %%EOF


	
	int c;
	
	 // initialize outside accessible ctm
	psctms.flush();
	psctm=transform_identity(psctm);
	DBG cerr <<"=================== start printing "<<out->range.ToString(true, false, false)<<" ====================\n";
	
	 // print out header
	fprintf (f,"%%!PS-Adobe-3.0\n"
			   "%%%%Orientation: ");
	fprintf (f,"%s\n",(landscape?"Landscape":"Portrait"));
	fprintf(f,"%%%%Pages: %d\n", totalnumpages);
	time_t t=time(NULL);
	fprintf(f,"%%%%PageOrder: Ascend\n"
			  "%%%%CreationDate: %s" //ctime puts a terminating newline
			  "%%%%Creator: Laidout %s\n"
			  "%%%%For: whoever \n",
			  		ctime(&t),LAIDOUT_VERSION);

	 //%%DocumentMedia: list...
	//*****uses only the first paper of papergroup
	 //%%DocumentMedia: tag width-ps-units height weight color type
	 //%%+ tag width height weight color type
	 //%%+ ...list of used media in order of most used first
	 //Each page refers to a given media with: %%PageMedia: tag
	fprintf(f,"%%%%DocumentMedia: %s %.10g %.10g 75 white ( )\n", //75 g/m^2 = 20lb * 3.76 g/lb/m^2 
			"Paper", //papergroup->papers.e[0]->box->paperstyle->name, 
			72 * paperwidth, //papergroup->papers.e[0]->box->paperstyle->width,  //width and height ignoring landscape/portrait
			72 * paperheight //papergroup->papers.e[0]->box->paperstyle->height
			);
	fprintf(f,"%%%%EndComments\n");
			

	 //---------------------------Defaults
	fprintf(f,"%%%%BeginDefaults\n"
			  "%%%%PageMedia: %s\n", "Paper" /*papergroup->papers.e[0]->box->paperstyle->name*/);//***
	fprintf(f,"%%%%EndDefaults\n"
			  "\n");
			  
	 //*** Extensions are general header comment. If an EPS has extensions, they must
	 //be listed here also:
	 //%%Extensions: DPS|CMYK|Composite|FileSystem  <-- seems to be more a concern for LL1 docs only?


	 //---------------------------Prolog, for procedure sets
	fprintf(f,"%%%%BeginProlog\n");
	//if (doc->hasEPS()) { *** including these defs always isn't harmful, and much easier to implement..
	
 		 // if an EPS has extra resources, they are mentioned here in the prolog(?)
		//***
		//%%BeginResource: procsetname
		//...
		//%%EndResource
		 
		 //define functions to simplify inclusion of EPS files
		fprintf(f,"/BeginEPS {\n"
			  "  /starting_state save def\n"
			  "  /dict_count countdictstack def \n"
			  "  /op_count count 1 sub def\n"
			  "  userdict begin\n"
			  "  /showpage { } def\n"
			  "  0 setgray     0 setlinecap     1 setlinewidth\n"
			  "  0 setlinejoin 10 setmiterlimit [ ] 0 setdash   newpath\n"
			  "  /languagelevel where\n"
			  "  { pop languagelevel 1 ne\n"
			  "    { false setoverprint  false setstrokeadjust\n"
			  "    } if\n"
			  "  } if\n"
			  "} bind def\n");
		fprintf(f,"/EndEPS {\n"
			  "  count op_count sub { pop } repeat\n"
			  "  countdictstack dict_count sub { end } repeat\n"
			  "  starting_state restore\n"
			  "} bind def\n");
	//}
			  
	fprintf(f,"%%%%EndProlog\n"
			  "\n"
			  "%%%%BeginSetup\n"
			  "%%%%EndSetup\n"
			  "\n");
	
	 // Write out paper spreads....
	double m[6];
	int c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page=NULL;
	char *desc=NULL;
	int p;
	int cur_page_index = 1;
	double dpi = 150;
	if (doc && doc->imposition && doc->imposition->paper && doc->imposition->paper->paperstyle)
		dpi = doc->imposition->paper->paperstyle->dpi;

	for (c = out->range.Start(); c >= 0; c = out->range.Next()) {
		 //get spread if any
		if (doc) spread=doc->imposition->Layout(layout,c);
		else spread=NULL;
		 
		 //get paper description
		if (spread) desc=spread->pagesFromSpreadDesc(doc);
		else desc=limbo->id?newstr(limbo->id):NULL;
		prependstr(desc,"(");
		appendstr(desc,")");

		papergroup = out->papergroup;
		if (!papergroup && spread) papergroup = spread->papergroup;
			
		for (p=0; p<(papergroup ? papergroup->papers.n : 1); p++) {
			DBG cerr<<"Printing paper "<<p<<"..."<<endl;
			if (papergroup) {
				plandscape = papergroup->papers.e[p]->box->paperstyle->landscape();
				paperwidth = papergroup->papers.e[p]->box->paperstyle->width;
			}

			 //print (postscript) page header
			 //%%Page label (ordinal starting at 1)
			if (desc) {
				fprintf(f, "%%%%Page: %s %d\n", desc, cur_page_index);
			} else fprintf(f, "%%%%Page: %d %d\n", 
					cur_page_index, cur_page_index);
			cur_page_index++;

			// //paper media
			//if (this paper media != default paper media) {
			//	fprintf(f,"%%%%PageMedia: $s\n",papergroup->papers.e[p]->box->name ???);     ***OR***
			//	fprintf(f,"%%%%PageMedia: $s\n",papergroup->papers.e[p]->box->paperstyle ???);***
			//}

			//%%BeginPageSetup
			if (plandscape!=landscape) {
				fprintf(f,"%%%%BeginPageSetup\n");
				fprintf(f,"%%%%PageOrientation: %s\n",((!landscape)?"Landscape":"Portrait"));
				fprintf(f,"%%%%EndPageSetup\n");
			}
			//%%EndPageSetup

			 //begin paper contents
			fprintf(f, "save\n");
			fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
			psConcat(72.,0.,0.,72.,0.,0.);
			if (plandscape) {
				fprintf(f,"%.10g 0 translate\n90 rotate\n",paperwidth);
				psConcat(0.,1.,-1.,0., paperwidth,0.);
			}

			fprintf(f,"gsave\n");
			psPushCtm();
			if (papergroup) transform_invert(m,papergroup->papers.e[p]->m());
			else transform_identity(m);
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			psConcat(m);

			if (limbo && limbo->n()) {
				//*** if limbo bbox inside paper bbox? could loop in limbo objs for more specific check
				psdumpobj(f,limbo);
				//----OR-----
				//transform by limbo
				//for (l=0; l<limbo->n(); l++) {
				//	if (***inbounds) psdumpobj(f,limbo->e(l));
				//}
			}

			if (papergroup && papergroup->objs.n()) {
				psdumpobj(f,&papergroup->objs);
			}

			
			if (spread) {
				 // print out printer marks
				if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
					//fprintf(f," .01 setlinewidth\n");
					//DBG cerr <<"marks data:\n";
					//DBG spread->marks->dump_out(stderr,2,0);
					psdumpobj(f,spread->marks);
				}
				
				 // for each page in spread..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
					psDpi(dpi);
					
					pg=spread->pagestack.e[c2]->index;
					if (pg<0 || pg>=doc->pages.n) continue;
					page=doc->pages.e[pg];
					
					 // transform to page
					fprintf(f,"gsave\n");
					psPushCtm();
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
							m[0], m[1], m[2], m[3], m[4], m[5]); 
					psConcat(m);

					 // set clipping region
					DBG cerr <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
					if (page->pagestyle->flags&PAGE_CLIPS) {
						DBG cerr <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" clips"<<endl;
						psSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
					} else {
						DBG cerr <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" does not clip"<<endl;
					}
						
					 // for each layer on the page..
					for (l=0; l<page->layers.n(); l++) {
						psdumpobj(f,page->layers.e(l));
					}
					fprintf(f,"grestore\n");
					psPopCtm();
				}

			}
			fprintf(f,"grestore\n"); //from papergroup->paper transform
			psPopCtm();

			 // print out paper footer
			fprintf(f,"\n"
					  "restore\n" //from page start
					  "\n"
					  "showpage\n"
					  "\n");		
			DBG cerr<<"Done printing paper "<<p<<"."<<endl;
		}
		if (spread) { delete spread; spread=NULL; }
		delete[] desc; desc=NULL;
	}

	 //print out footer
	fprintf(f, "\n%%%%Trailer\n");
	fprintf(f, "\n%%%%EOF\n");

	DBG cerr <<"=================== end printing ps ========================\n";

	 //clean up
	fclose(f);
	delete[] file;
	//papergroup->dec_count();
	setlocale(LC_ALL,"");

	return 0;
}

//! Print out eps files with filenames based on fname for papers or pages in doc.
/*! \ingroup postscript
 * This sets up the ctm as accessible through psCTM(),
 * and flushes the ctm stack.
 *
 * Return 0 for no errors, nonzero for error. 
 * 
 * \todo *** this does not currently handle pages that bleed their contents
 *   onto other pages correctly. it bleeds here by paper spread, rather than page spread
 * \todo *** for tiled pages, or multiples of same object each instance is
 *   rendered independently right now. should make a function to display such
 *   things, thus reduce ps file size substantially..
 * \todo *** bounding box should more accurately reflect the drawn extent.. just does paper bounds here
 */
int epsout(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *out = dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	 //set up config
	Document *doc =out->doc;
	int layout    =out->layout;
	Group *limbo  =out->limbo;
	char *file = nullptr;

	if (!filename) filename=out->filename;
	if (!filename) filename=out->tofiles;
	if (!filename) {
		if (!doc || isblank(doc->saveas)) { 
            log.AddMessage(_("Cannot save without a filename."),ERROR_Fail); 
            return 3; 
        } 
        file = newstr(doc->saveas);
        appendstr(file,".png");
		filename = file;
	}

	double dpi = 150;
	if (doc && doc->imposition && doc->imposition->paper && doc->imposition->paper->paperstyle)
		dpi = doc->imposition->paper->paperstyle->dpi;

	DBG cerr <<"=================== start printing eps "<<out->range.ToString(true, false, false)<<" ====================\n";
		
	Spread *spread = nullptr;
	DoubleBBox bbox;
	double m[6];
	int c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page;
	
	 // initialize outside accessible ctm
	psctms.flush();
	psctm=transform_identity(psctm);

	 // Find bbox
	 //*** note bbox is not used!!
	if (doc) spread = doc->imposition->Layout(layout,out->range.Start());
	bbox.ClearBBox();
	bbox.addtobounds(spread->path);
	

	// figure out paper size and orientation
	int landscape = 0;
	double paperwidth = 0, paperheight = 0;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	PaperGroup *papergroup = out->papergroup;
	if (!papergroup) {
		papergroup = spread->papergroup;
	}
	if (papergroup) {
		PaperStyle *defaultpaper = nullptr;
		defaultpaper = papergroup->papers.e[0]->box->paperstyle;
		landscape   = defaultpaper->landscape();
		paperwidth  = defaultpaper->width;
		paperheight = defaultpaper->height;
	} else if (spread) {
		paperwidth  = spread->path->boxwidth();
		paperheight = spread->path->boxheight();
	} else if (out->limbo) {
		paperwidth  = out->limbo->boxwidth();
		paperheight = out->limbo->boxheight();
	}
	papergroup = out->papergroup;

	if (paperwidth <= 0 || paperheight <= 0) {
		DBG cerr <<" bad bounds, aborting. w x h: "<<paperwidth <<" x "<<paperheight<<endl;
		log.AddError(_("Bad bounds for export!"));
		if (spread) { delete spread; spread = nullptr; }
		return 4;
	}


	FILE *f = open_file_for_writing(filename,0,&log);
	delete[] file;
	if (!f) {
		if (spread) { delete spread; spread = nullptr; }
		return 1;
	}
	
	setlocale(LC_ALL,"C");


	 // print out header
	fprintf(f, "%%!PS-Adobe-3.0 EPSF-3.0\n");
	fprintf(f,"%%%%BoundingBox: 0 0 %d %d\n",
			(int)(72 * paperwidth),
			(int)(72 * paperheight));

	fprintf(f,"%%%%Pages: 1\n");
	time_t t=time(NULL);
	fprintf(f,"%%%%CreationDate: %s\n"
			  "%%%%Creator: Laidout %s\n",
					ctime(&t),LAIDOUT_VERSION);

	fprintf(f,"%%%%EndComments\n");
	fprintf(f,"%%%%BeginProlog\n");
	//if (doc->hasEPS()) { *** including these defs always isn't harmful, and much easier to implement..
		 // if an EPS has extra resources, they are mentioned here in the prolog(?)
		//***
		//%%BeginResource: procsetname
		//...
		//%%EndResource
		 
		 //define functions to simplify inclusion of EPS files
		fprintf(f,"/BeginEPS {\n"
			  "  /starting_state save def\n"
			  "  /dict_count countdictstack def \n"
			  "  /op_count count 1 sub def\n"
			  "  userdict begin\n"
			  "  /showpage { } def\n"
			  "  0 setgray     0 setlinecap     1 setlinewidth\n"
			  "  0 setlinejoin 10 setmiterlimit [ ] 0 setdash   newpath\n"
			  "  /languagelevel where\n"
			  "  { pop languagelevel 1 ne\n"
			  "    { false setoverprint  false setstrokeadjust\n"
			  "    } if\n"
			  "  } if\n"
			  "} bind def\n");
		fprintf(f,"/EndEPS {\n"
			  "  count op_count sub { pop } repeat\n"
			  "  countdictstack dict_count sub { end } repeat\n"
			  "  starting_state restore\n"
			  "} bind def\n");
	//}
			  
	fprintf(f,"%%%%EndProlog\n");
		
	 //print paper header
	fprintf(f, "%%%%Page: %d 1\n", out->range.Start()+1);//%%Page (label) (ordinal starting at 1)

	 //begin paper contents
	fprintf(f, "save\n");
	fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
	psConcat(72.,0.,0.,72.,0.,0.);
	if (landscape) {
		fprintf(f,"%.10g 0 translate\n90 rotate\n",paperwidth);
		psConcat(0.,1.,-1.,0., paperwidth,0.);
	}

	fprintf(f,"gsave\n");
	psPushCtm();
	if (papergroup) transform_invert(m,papergroup->papers.e[0]->m());
	else transform_identity(m);
	fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
			m[0], m[1], m[2], m[3], m[4], m[5]); 
	psConcat(m);

	if (limbo && limbo->n()) {
		//*** if limbo bbox inside paper bbox? could loop in limbo objs for more specific check
		psdumpobj(f,limbo);
		//----OR-----
		//transform by limbo
		//for (l=0; l<limbo->n(); l++) {
		//	if (***inbounds) psdumpobj(f,limbo->e(l));
		//}
	}
			
	if (papergroup && papergroup->objs.n()) {
		psdumpobj(f,&papergroup->objs);
	}

	if (spread) {
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			fprintf(f," .01 setlinewidth\n");
			//DBG cerr <<"marks data:\n";
			//DBG spread->marks->dump_out(stderr,2,0);
			psdumpobj(f,spread->marks);
		}
	
		 // for each page in spread..
		for (c2=0; c2<spread->pagestack.n(); c2++) {
			psDpi(dpi);
			
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			page=doc->pages.e[pg];
			
			 // transform to page
			fprintf(f,"gsave\n");
			psPushCtm();
			transform_copy(m,spread->pagestack.e[c2]->outline->m());
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			psConcat(m);

			 // set clipping region
			DBG cerr <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				DBG cerr <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" clips"<<endl;
				psSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} else {
				DBG cerr <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" does not clip"<<endl;
			}
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n(); l++) {
				psdumpobj(f,page->layers.e(l));
			}
			fprintf(f,"grestore\n");
			psPopCtm();
		}

		delete spread;
		spread = nullptr;
	}
	fprintf(f,"grestore\n");//remove papergroup->paper transform
	psPopCtm();
	

	 // print out paper footer
	fprintf(f,"\n"
			  "restore\n"
			  "\n");		
	
	 //print out footer
	fprintf(f, "\n%%%%Trailer\n");
	fprintf(f, "\n%%%%EOF\n");

	fclose(f);
	setlocale(LC_ALL,"");


	DBG cerr <<"=================== end printing eps ========================\n";

	return 0;
}

} // namespace Laidout

