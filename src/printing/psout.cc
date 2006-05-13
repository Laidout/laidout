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
/********** psout.cc ****************/


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
	//DBG cout << "ctm: "; dumpctm(psctm);
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
		
	} else if (dynamic_cast<PathsData *>(obj)) {
		psPathsData(f,dynamic_cast<PathsData *>(obj));

	}
	
	 // pop axes
	fprintf(f,"grestore\n\n");
	psPopCtm();
}

//! Open file, or 'output.ps' if file==NULL, and output postscript for doc via psout(FILE*,Document*).
/*! \ingroup postscript
 * \todo *** does not sanity checking on file, nor check for previous existence of file
 *
 * Return 1 if not saved, 0 if saved.
 */
int psout(Document *doc,const char *file)//file=NULL
{
	if (file==NULL) file="output.ps";

	FILE *f=fopen(file,"w");
	if (!f) return 1;
	int c=psout(f,doc);
	fclose(f);

	if (c) return 1;
	return 0;
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
 */
int psout(FILE *f,Document *doc,int start,int end,unsigned int flags)
{
	if (!f || !doc) return 1;
	if (start<0) start=0;
	if (end<0 || end>=doc->docstyle->imposition->numpapers) 
		end=doc->docstyle->imposition->numpapers-1;

	 // initialize outside accessible ctm
	psctms.flush();
	psctm=transform_identity(psctm);
	//DBG cout <<"=================== start printing "<<start<<" to "<<end<<" ====================\n";
	
	 // print out header
	fprintf (f,
			"%%!PS-Adobe-3.0\n"
			"%%%%Orientation: ");
	fprintf (f,"%s\n",(doc->docstyle->imposition->paperstyle->flags&1)?"Landscape":"Portrait");
	fprintf(f,"%%%%Pages: %d\n",end-start+1);
	time_t t=time(NULL);
	fprintf(f,"%%%%PageOrder: Ascend\n"
			  "%%%%CreationDate: %s\n"
			  "%%%%Creator: Laidout %s\n"
			  "%%%%For: whoever (i686, Linux 2.6.15-1-k7)\n",
			  		ctime(&t),LAIDOUT_VERSION);
	fprintf(f,"%%%%DocumentMedia: %s %.10g %.10g 75 white ( )\n", //75 g/m^2 = 20lb * 3.76 g/lb/m^2
				doc->docstyle->imposition->paperstyle->name, 
				72*doc->docstyle->imposition->paperstyle->width,  //width and height ignoring landscape/portrait
				72*doc->docstyle->imposition->paperstyle->height);
	fprintf(f,"%%%%EndComments\n"
			  "%%%%BeginDefaults\n"
			  "%%%%PageMedia: %s\n",doc->docstyle->imposition->paperstyle->name);
	fprintf(f,"%%%%EndDefaults\n"
			  "\n"
			  "%%%%BeginProlog\n"
			  "%%%%EndProlog\n"
			  "\n"
			  "%%%%BeginSetup\n"
			  "%%%%EndSetup\n"
			  "\n");
	
	 // Write out paper spreads....
	Spread *spread;
	double m[6];
	int c,c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page;
	for (c=start; c<=end; c++) {
	     //print paper header
		fprintf(f, "%%%%Page: %d %d\n", c+1,c-start+1);//Page label (ordinal starting at 1)
		fprintf(f, "save\n");
		fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
		psConcat(72.,0.,0.,72.,0.,0.);
		if (doc->docstyle->imposition->paperstyle->flags&1) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->imposition->paperstyle->width);
			psConcat(0.,1.,-1.,0., doc->docstyle->imposition->paperstyle->width,0.);
		}
		
		spread=doc->docstyle->imposition->PaperLayout(c);
		
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			fprintf(f," .01 setlinewidth\n");
			////DBG cout <<"marks data:\n";
			////DBG spread->marks->dump_out(stdout,2,0);
			psdumpobj(f,spread->marks);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			psDpi(doc->docstyle->imposition->paperstyle->dpi);
			
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
			//DBG cout <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				//DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" clips"<<endl;
				psSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} else {
				//DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" does not clip"<<endl;
			}
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n; l++) {
				psdumpobj(f,page->layers.e[l]);
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
	fprintf(f, "\n%%%%EOF\n");

	//DBG cout <<"=================== end printing ========================\n";

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
int epsout(const char *fname,Document *doc,int start,int end,
		int layouttype,unsigned int flags)
{
	if (!doc) return 1;
	if (start<0) start=0;
	if (layouttype==PAPERLAYOUT) {
		if (end<0 || end>=doc->docstyle->imposition->numpapers) 
			end=doc->docstyle->imposition->numpapers-1;
	} else if (layouttype==PAGELAYOUT) {
		if (end<0 || end>=doc->pages.n) 
			end=doc->pages.n-1;
	} else { // singles
		if (end<0 || end>=doc->pages.n) 
			end=doc->pages.n-1;
	}

	 //figure out base filename
	char *filebase=LaxFiles::make_filename_base(fname);
	char filename[strlen(filebase)+10];

	//DBG cout <<"=================== start printing eps "<<start<<" to "<<end<<" ====================\n";
		
	 // Write out paper spreads....
	Spread *spread;
	DoubleBBox bbox;
	double m[6];
	int c,c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page;
	FILE *f;
	for (c=start; c<=end; c++) {
		sprintf(filename,filebase,c+1);
		f=fopen(filename,"w");
		if (!f) {
			delete[] filebase;
			return c+1;
		}
		
		 // initialize outside accessible ctm
		psctms.flush();
		psctm=transform_identity(psctm);

		 // Find bbox
		if (layouttype==SINGLELAYOUT) spread=doc->docstyle->imposition->SingleLayout(c);
		else if (layouttype==PAGELAYOUT) spread=doc->docstyle->imposition->PageLayout(c);
		else if (layouttype==PAPERLAYOUT) spread=doc->docstyle->imposition->PaperLayout(c);
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
	
	     //print paper header
		fprintf(f, "%%%%Page: %d 1\n", c+1);//Page label (ordinal starting at 1)
		fprintf(f, "save\n");
		fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
		psConcat(72.,0.,0.,72.,0.,0.);
		if (doc->docstyle->imposition->paperstyle->flags&1) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->imposition->paperstyle->width);
			psConcat(0.,1.,-1.,0., doc->docstyle->imposition->paperstyle->width,0.);
		}
		
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			fprintf(f," .01 setlinewidth\n");
			////DBG cout <<"marks data:\n";
			////DBG spread->marks->dump_out(stdout,2,0);
			psdumpobj(f,spread->marks);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			psDpi(doc->docstyle->imposition->paperstyle->dpi);
			
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
			//DBG cout <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				//DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" clips"<<endl;
				psSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} else {
				//DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" does not clip"<<endl;
			}
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n; l++) {
				psdumpobj(f,page->layers.e[l]);
			}
			fprintf(f,"grestore\n");
			psPopCtm();
		}

		//**** ugly workaround for page spreads, need perhaps a numpagespreads var in imposition
		if (layouttype==PAGELAYOUT) 
			for (c2=0; c2<spread->pagestack.n; c2++)
				if (spread->pagestack.e[c2]->index>c) c=spread->pagestack.e[c2]->index;
			
		delete spread;
		
		 // print out paper footer
		fprintf(f,"\n"
				  "restore\n"
				  "\n");		
		
		 //print out footer
		fprintf(f, "\n%%%%EOF\n");

		fclose(f);

	}

	//DBG cout <<"=================== end printing eps ========================\n";

	return 0;
}
