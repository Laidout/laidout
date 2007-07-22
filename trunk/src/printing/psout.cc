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
// Copyright (c) 2004-2007 Tom Lechner
//


#include "../language.h"
#include "psout.h"
#include "../version.h"

#include "psfilters.h"
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/somedataref.h>
#include <lax/transformmath.h>
#include <lax/fileutils.h>

#include "psgradient.h"
#include "psimage.h"
#include "psimagepatch.h"
#include "pscolorpatch.h"
#include "pspathsdata.h"
#include "pseps.h"
#include "../filetypes/filefilters.h"
#include "../laidoutdefs.h"


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxInterfaces;




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
void psConcat(double *m)
{
	double *mm=transform_mult(NULL,m,psctm);
	delete[] psctm;
	psctm=mm;
	DBG cerr << "ctm: "; dumpctm(psctm);
}

//! Initialize to identity and return ctm.
/*! \ingroup postscript 
 * If psctm==NULL, then create new. This should be done, for instance, by
 * any output stuff like pdf that makes use of psctm, which is static to psout.cc.
 */
double *psCtmInit()
{
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

//! Print a postscript file of doc to already open f.
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
 * \todo for EPS that include specific resources, extensions, or language level, these must be
 *   mentioned in the postscript file's comments...
 * \todo *** fix non-paper layout media/paper type
 */
int psout(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->filename;

	if (!doc || !doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paper) {
		if (error_ret) *error_ret=newstr(_("Nothing to export!"));
		return 1;
	}
	
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			DBG cerr <<"**** cannot save, null filename, doc->saveas is null."<<endl;
			
			if (error_ret) *error_ret=newstr(_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".ps");
	} else file=newstr(filename);

	f=fopen(file,"w");
	if (!f) {
		DBG cerr <<"**** cannot save, "<<file<<" cannot be opened for writing."<<endl;

		delete[] file;
		if (error_ret) *error_ret=newstr(_("Error opening file for writing."));
		return 3;
	}

	if (start<0) start=0;
	else if (start>=doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	if (end<start) end=start;
	else if (end>=doc->docstyle->imposition->NumSpreads(layout))
		end=doc->docstyle->imposition->NumSpreads(layout)-1;


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

	 //figure out paper orientation
	int landscape=0;
	int c;
	if (layout==PAPERLAYOUT) {
		landscape=((doc->docstyle->imposition->paper->paperstyle->flags&1)?1:0);
	} 
	
	 // initialize outside accessible ctm
	psctms.flush();
	psctm=transform_identity(psctm);
	DBG cerr <<"=================== start printing "<<start<<" to "<<end<<" ====================\n";
	
	 // print out header
	fprintf (f,
			"%%!PS-Adobe-3.0\n"
			"%%%%Orientation: ");
	fprintf (f,"%s\n",(landscape?"Landscape":"Portrait"));
	fprintf(f,"%%%%Pages: %d\n",end-start+1);
	time_t t=time(NULL);
	fprintf(f,"%%%%PageOrder: Ascend\n"
			  "%%%%CreationDate: %s" //ctime puts a terminating newline
			  "%%%%Creator: Laidout %s\n"
			  "%%%%For: whoever \n",
			  		ctime(&t),LAIDOUT_VERSION);
	if (layout==PAPERLAYOUT) {
		fprintf(f,"%%%%DocumentMedia: %s %.10g %.10g 75 white ( )\n", //75 g/m^2 = 20lb * 3.76 g/lb/m^2
				doc->docstyle->imposition->paper->paperstyle->name, 
				72*doc->docstyle->imposition->paper->paperstyle->width,  //width and height ignoring landscape/portrait
				72*doc->docstyle->imposition->paper->paperstyle->height);
	} else {
		//PaperStyle *paper=doc->docstyle->imposition->Paper(layout)); 
		//paper->width
		//paper->height
		//paper->dec_count();
		fprintf(f,"%%%%DocumentMedia: %s %.10g %.10g 75 white ( )\n", //75 g/m^2 = 20lb * 3.76 g/lb/m^2
				doc->docstyle->imposition->paper->paperstyle->name, 
				72*doc->docstyle->imposition->paper->paperstyle->width,  //width and height ignoring landscape/portrait
				72*doc->docstyle->imposition->paper->paperstyle->height);
		//***this should be specific to layout?
	}
	fprintf(f,"%%%%EndComments\n");
			

	 //---------------------------Defaults
	fprintf(f,"%%%%BeginDefaults\n"
			  "%%%%PageMedia: %s\n",doc->docstyle->imposition->paper->paperstyle->name);
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
	Spread *spread;
	double m[6];
	int c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page;
	char *desc;
	for (c=start; c<=end; c++) {
		spread=doc->docstyle->imposition->Layout(layout,c);
		desc=spread->pagesFromSpreadDesc(doc);
			
	     //print (postscript) paper header
		if (desc) {
			fprintf(f, "%%%%Page: %s %d\n", desc,c-start+1);//Page label (ordinal starting at 1)
			delete[] desc;
		} else fprintf(f, "%%%%Page: %d %d\n", c+1,c-start+1);//Page label (ordinal starting at 1)
		fprintf(f, "save\n");
		fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
		psConcat(72.,0.,0.,72.,0.,0.);
		if (doc->docstyle->imposition->paper->paperstyle->flags&1) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->imposition->paper->paperstyle->width);
			psConcat(0.,1.,-1.,0., doc->docstyle->imposition->paper->paperstyle->width,0.);
		}
		
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			fprintf(f," .01 setlinewidth\n");
			//DBG cerr <<"marks data:\n";
			//DBG spread->marks->dump_out(stderr,2,0);
			psdumpobj(f,spread->marks);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			psDpi(doc->docstyle->imposition->paper->paperstyle->dpi);
			
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
		
		 // print out paper footer
		fprintf(f,"\n"
				  "restore\n"
				  "\n"
				  "showpage\n"
				  "\n");		
	}

	 //print out footer
	fprintf(f, "\n%%%%Trailer\n");
	fprintf(f, "\n%%%%EOF\n");

	DBG cerr <<"=================== end printing ========================\n";

	fclose(f);
	delete[] file;

	return 0;
}

//! Print out eps files with filenames based on fname for papers or pages in doc.
/*! \ingroup postscript
 * This sets up the ctm as accessible through psCTM(),
 * and flushes the ctm stack.
 *
 * Return 0 for no errors, nonzero for error. If return >0 then the number is which
 * page failed to be output (start+1 means first page attempted).
 * 
 * The directory part of fname (if any) should already be a valid directory.
 * The function returns immediately when it cannot open the file to write. It does
 * not try to figure out why.
 * 
 * \todo *** this does not currently handle pages that bleed their contents
 *   onto other pages correctly. it bleeds here by paper spread, rather than page spread
 * \todo *** for tiled pages, or multiples of same object each instance is
 *   rendered independently right now. should make a function to display such
 *   things, thus reduce ps file size substantially..
 * \todo *** bounding box should more accurately reflect the drawn extent.. just does paper bounds here
 * \todo *** doing page layouts is potentially broken for impositions that do not provide page layouts 
 *   with pages that increase one by one: [1,2]->[3,4]->etc. No good: [1,3]->[2,4]
 */
int epsout(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	 //set up config
	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->tofiles;
	if (!filename) filename="output#.eps";

	if (!doc || !doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paper) {
		if (error_ret) *error_ret=newstr(_("Nothing to export!"));
		return 1;
	}
	
	if (start<0) start=0;
	else if (start>=doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	if (end<start) end=start;
	else if (end>=doc->docstyle->imposition->NumSpreads(layout))
		end=doc->docstyle->imposition->NumSpreads(layout)-1;


	 //figure out base filename
	char *filebase=LaxFiles::make_filename_base(filename);
	char epsfilename[strlen(filebase)+10];

	DBG cerr <<"=================== start printing eps "<<start<<" to "<<end<<" ====================\n";
		
	 // Write out paper spreads....
	Spread *spread;
	DoubleBBox bbox;
	double m[6];
	int c,c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page;
	FILE *f;
	for (c=start; c<=end; c++) {
		sprintf(epsfilename,filebase,c);
		f=fopen(epsfilename,"w");
		if (!f) {
			if (error_ret) {
				*error_ret=new char[100];
				sprintf(*error_ret,_("Error opening %s for writing during EPS out."),epsfilename);
			}
			delete[] filebase;
			return c+1;
		}
		
		 // initialize outside accessible ctm
		psctms.flush();
		psctm=transform_identity(psctm);

		 // Find bbox
		spread=doc->docstyle->imposition->Layout(layout,c);
		bbox.clear();
		bbox.addtobounds(spread->path);
		
		 // print out header
		fprintf (f,
				"%%!PS-Adobe-3.0 EPSF-3.0\n"
				"%%%%BoundingBox: %d %d %d %d\n",
				  (int)(spread->path->minx*72), (int)(spread->path->miny*72),
				  (int)(spread->path->maxx*72), (int)(spread->path->maxy*72));
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
		fprintf(f, "%%%%Page: %d 1\n", c+1);//%%Page (label) (ordinal starting at 1)
		fprintf(f, "save\n");
		fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
		psConcat(72.,0.,0.,72.,0.,0.);
		if (layout==PAPERLAYOUT && doc->docstyle->imposition->paper->paperstyle->flags&1) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->imposition->paper->paperstyle->width);
			psConcat(0.,1.,-1.,0., doc->docstyle->imposition->paper->paperstyle->width,0.);
		}
		
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			fprintf(f," .01 setlinewidth\n");
			//DBG cerr <<"marks data:\n";
			//DBG spread->marks->dump_out(stderr,2,0);
			psdumpobj(f,spread->marks);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			psDpi(doc->docstyle->imposition->paper->paperstyle->dpi);
			
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
		
		 // print out paper footer
		fprintf(f,"\n"
				  "restore\n"
				  "\n");		
		
		 //print out footer
		fprintf(f, "\n%%%%Trailer\n");
		fprintf(f, "\n%%%%EOF\n");

		fclose(f);

	}

	DBG cerr <<"=================== end printing eps ========================\n";

	return 0;
}


