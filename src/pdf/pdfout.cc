//
// $Id: psout.cc 114 2006-08-17 18:48:34Z tomlechner $
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


#include "pdfout.h"
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




/*! \defgroup pdf Pdf
 *
 * Various things to help output PDF, currently only 1.4.
 *
 * While outputting pdf, functions can access the current transformation matrix
 * as kept up by the postscript helper functions such as psCTM(). This is useful, for intstance,
 * for effects where something must be generated at the paper's dpi like an
 * ImagePatch. Also can be used to aid setup for the kind of linewidth that is supposed 
 * to be relative to the paper, rather than the ctm.
 *
 * \todo *** could have a pdfSpreadOut function to do a 1-off of any old spread, optionally
 *   fit to a supplied box
 */


//----------------------------- pdf out ---------------------------------------

//! Internal function to dump out the obj in PDF. Called by pdfout().
/*! \ingroup pdf
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
void pdfdumpobj(FILE *f,LaxInterfaces::SomeData *obj)
{***
	if (!obj) return;
	
	 // push axes
	psPushCtm();
	psConcat(obj->m());
	fprintf(f,"gsave\n"
			  "[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
	
	if (!strcmp(obj->whattype(),"Group")) {
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) pdfdumpobj(f,g->e(c)); 
		
	} else if (!strcmp(obj->whattype(),"ImagePatchData")) {
		pdfImagePatch(f,dynamic_cast<ImagePatchData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"ImageData")) {
		pdfImage(f,dynamic_cast<ImageData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"GradientData")) {
		pdfGradient(f,dynamic_cast<GradientData *>(obj));
		
	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		pdfColorPatch(f,dynamic_cast<ColorPatchData *>(obj));
		
	} else if (dynamic_cast<PathsData *>(obj)) {
		pdfPathsData(f,dynamic_cast<PathsData *>(obj));

	}
	
	 // pop axes
	fprintf(f,"grestore\n\n");
	psPopCtm();
}

//! Open file, or 'output.pdf' if file==NULL, and output a PDF for doc via pdfout(FILE*,Document*).
/*! \ingroup pdf
 * \todo *** does not sanity checking on file, nor check for previous existence of file
 *
 * Return 1 if not saved, 0 if saved.
 */
int pdfout(Document *doc,const char *file)//file=NULL
{
	if (file==NULL) file="output.pdf";

	FILE *f=fopen(file,"w");
	if (!f) return 1;
	int c=pdfout(f,doc);
	fclose(f);

	if (c) return 1;
	return 0;
}

//! Output a postscript clipping path from outline.
/*! \ingroup pdf
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
 * that PathsData are capable of. when pdf output of paths is 
 * actually more implemented, this will change..
 */
int pdfSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing)//iscontinuing=0
{***
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

class PdfObjData 
{
 public:
	unsigned long byteoffset;
	char inuse; //'n' or 'f'
	long number;
	int generation;
	PdfObjData *next;
	char *data;//optional for writing out
	long len; //just in case data has bytes with 0 value
	PdfObjData() 
	 : byteoffset(0), inuse('n'), number(0), generation(0), next(NULL), data(NULL), len(0)
	 {}
};

//! Write out the xref table and trailer dictionary. Return byte offset of table.
/*! start is the starting number for this sequence of objects in this xref table.
 * start overrides whatever is in obj->number.
 */
long writeXrefTable(FILE *f,int start,PdfObjData *objs)
{
	long xrefpos=ftell(f);
	int count=0;
	PdfObjData *o=objs;
	while (o) { count++; o=o->objs; }
	fprintf("xref\n%d %d",start,count);
	while (objs) {
		fprintf(f,"%010lu %05d %c\n",objs->byteoffset,objs->generation,objs->inuse);
		objs=objs->next;
	}
}

//! Print a pdf file of doc to already open f.
/*! \ingroup pdf
 * Does not open or close f. This sets up the ctm as accessible through psCTM(),
 * and flushes the ctm stack.
 *
 * Return 0 for no errors, nonzero for errors.
 * 
 * \todo *** this does not currently handle pages that bleed their contents
 *   onto other pages correctly. it bleeds here by paper spread, rather than page spread
 * \todo *** for tiled pages, or multiples of same object each instance is
 *   rendered independently right now. should make a function to display such
 *   things, thus reduce file size substantially..
 * \todo *** trailer dict's encryption dict, ID, Prev (if necessary.. prev would only be
 *   if ever laidout allows importing and preserving pdf's, which it will not in the 
 *   forseeable future
 * \todo *** in future, might allow PDF-X, PDF 1.3, 1.5, etc..
 */
int pdfout(FILE *f,
		   Document *doc,
		   int format,        //!< Imposition type, ie SINGLELAYOUT, PAGELAYOUT, PAPERLAYOUT
		   int start,         //!< The starting unit in format, ie start page, or start paper
		   int end,           //!< The ending unit in format
		   unsigned int flags,
		   const char *pdfformat) //!< "1.4" is only one right now, so this value is ignored.
{***
	if (!f || !doc) return 1;
	if (start<0) start=0;
	else if (start>=doc->docstyle->imposition->NumSpreads(format)) return 1;
	if (end<0 || end>=doc->docstyle->imposition->NumSpreads(format)) 
		end=doc->docstyle->imposition->NumSpreads(format))-1;

	 // initialize outside accessible ctm
	psCtmInit();
	psctms.flush();
	DBG cout <<"=================== start pdf out "<<start<<" to "<<end<<" ====================\n";

	 // a PDF is:
	 //   header
	 //   body: a list of indirect objects
	 //   cross reference table
	 //   trailer
	
	
	 // Start the list of objects with the head of free objects, which
	 // has generation number of 65535. Its number is the object number of
	 // the next free object. Since this is a fresh pdf, there are no 
	 // other free objects.
	PdfObjData *objs=new PdfObjData, *obje=objs, *obj;
	objs->inuse='f';
	objs->number=0; 
	objs->generation=65535;
	int objcount=1;
	
	 // print out header
	fprintf (f,"%%!PDF-1.4\n");

	
	-----------
	struct PdfPageInfo
	{
		PdfPageInfo *next;
		int objnum,contents;
		DoubleBBox bbox;
		char *pagelabel;
		PdfPageInfo(int n) { next=NULL; pagelabel=NULL; objnum=n; }
		~PdfPageInfo() { if (next) delete next; if (pagelabel) delete[] pagelabel; }
	}:
	------------
	 // find basic pdf page info, and generate content stream
	int page, pages;
	Spread *spread;
	NumStack<int> pageobjs;
	PtrStack<char> pagelabeltext(2);
	for (int c=start; c<=end; c++) {
		page=objcount++;
		spread=doc->docstyle->imposition->Layout(format,c);
		desc=spread->pagesFromSpreadDesc(doc);
		
		pageobjs.push(page);
		pagelabeltext.push(desc);

		delete spread;
	}
	
	
	 // write out pdf /Page dicts
	--<<<<<<<<<<<--------------
	pages=objcount + end-start+1;
	for (int c=start; c<=end; c++) {
		fprintf(f,"%d 0 obj\n",objcount++);
		 //required
		fprintf(f,"<<\n  /Type /Page\n");
		fprintf(f,"  /Parent %d 0 R\n",pages);
		fprintf(f,"  /Resources << >>\n");
		fprintf(f,"  /MediaBox [llx lly urx ury]\n",***);
		fprintf(f,"  /Contents << >>\n",***); //not req, but of course necessary if stuff on page
		fprintf(f,"  /Rotate %d\n",number of 90 increments to rotate clockwise);
		 //the rest is optional
		//fprintf(f,"  /LastModified %s\n",lastmoddate);
		//fprintf(f,"  /CropBox [llx lly urx ury]\n");
		//fprintf(f,"  /BleedBox [llx lly urx ury]\n");
		//fprintf(f,"  /TrimBox [llx lly urx ury]\n");
		//fprintf(f,"  /ArtBox [llx lly urx ury]\n");
		//fprintf(f,"  /BoxColorInfo << >>\n");
		//fprintf(f,"  /Group << >>\n"); //group atts dict
		//fprintf(f,"  /Thumb << >>\n");
		//fprintf(f,"  /B << >>\n");
		//fprintf(f,"  /Dur << >>\n");
		//fprintf(f,"  /Trans << >>\n");
		//fprintf(f,"  /Annots << >>\n");
		//fprintf(f,"  /AA << >>\n");
		//fprintf(f,"  /Metadata << >>\n");
		//fprintf(f,"  /PieceInfo << >>\n");
		//fprintf(f,"  /StructParents << >>\n");
		//fprintf(f,"  /ID ()\n");
		//fprintf(f,"  /PZ %.10g\n");
		//fprintf(f,"  /SeparationInfo << >>\n");
		fprintf(f,">>\nendobj\n"); 
	}
	
	 //write out top /Pages page tree node
	objcount++;
	fprintf(f,"%d 0 obj\n",pages);
	fprintf(f,"<<\n  /Type /Pages\n");
	fprintf(f,"  /Kids [");
	for (c=objcount-(end-start+1); c<objcount; c++) fprintf(f,"%d 0 R  ",c);
	fprintf("]\n");
	fprintf(f,"  /Count %d",pageobjs.n); //this should be == end-start+1
	//can also include (from Page dict): /Resources, /MediaBox, /CropBox, and /Rotate
	fprintf(f,">>\nendobj\n"); 
	

	--==========--------------
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
	
	 // Write out paper spreads....
	Spread *spread;
	double m[6];
	int c,c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	Page *page;
	char *desc;
	for (c=start; c<=end; c++) {
		spread=doc->docstyle->imposition->Layout(format,c);
		desc=spread->pagesFromSpreadDesc(doc);
			
	     //print paper header
		if (desc) {
			fprintf(f, "%%%%Page: %s %d\n", desc,c-start+1);//Page label (ordinal starting at 1)
			delete[] desc;
		} else fprintf(f, "%%%%Page: %d %d\n", c+1,c-start+1);//Page label (ordinal starting at 1)
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
			//DBG cout <<"marks data:\n";
			//DBG spread->marks->dump_out(stdout,2,0);
			pdfdumpobj(f,spread->marks);
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
			DBG cout <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" clips"<<endl;
				psSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} else {
				DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" does not clip"<<endl;
			}
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n(); l++) {
				pdfdumpobj(f,page->layers.e(l));
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

	-------------------
	
		
	 // write out Outlines
	int outlines=-1;
	//int outlines=objcount++
	//***
	
	
	 // write out PageLabels
	int pagelabels;
	if (doc->pageranges.n) {
		pagelabels=objcount++;	
		//***
		pagelabels=-1;
	} else pagelabels=-1;

	
	 // write out Root doc catalog dict:
	 // this must be written after Pages and other items' object numbers figured out
	doccatalog=objcount++;
	obje->next=new PdfObjData;
	obje=obje->next;
	obje->number=doccatalog;
	obje->byteoffset=ftell(f);
	fprintf(f,"%d 0 obj\n<<\n",doccatalog);
	 //required fields
	fprintf(f,"  /Type /Catalog\n");
	fprintf(f,"  /Version /1.4\n");
	fprintf(f,"  /Pages %d 0 R\n",pages);
	 //the rest are optional
	if (pagelabels>0) fprintf(f,"  /PageLabels %d 0 R\n",pagelabels);
	if (outlines>0) {
		fprintf(f,"  /PageMode /UseOutlines\n");
		fprintf(f,"  /Outlines %d 0 R\n", outlines);
	}
//	if (!strcmp(doc->imposition->whattype(),"DoubleSidedSingles")) {
//		DoubleSidedSingles *dss=dynamic_cast<DoubleSidedSingles *>(doc->imposition);
//		if (dss->isvertical==0 && ***)
//		fprintf(f,"  /PageLayout /SinglePage\n");
//		fprintf(f,"  /PageLayout /OneColumn\n");
//		fprintf(f,"  /PageLayout /TwoColumnLeft\n");
//		fprintf(f,"  /PageLayout /TowColumnRight\n");
//	}
	//fprintf(f,"  /Names %d 0 R\n",     ***);
	//fprintf(f,"  /Dests %d 0 R\n",     ***);
	//fprintf(f,"  /Threads %d 0 R\n",   ***);
	//fprintf(f,"  /OpenAction %d 0 R\n",***);
	//fprintf(f,"  /AA %d 0 R\n",        ***);
	//fprintf(f,"  /URI %d 0 R\n",       ***);
	//fprintf(f,"  /AcroForm %d 0 R\n",  ***);
	//fprintf(f,"  /Metadata %d 0 R\n",  ***);
	//fprintf(f,"  /StructTreeRoot %d 0 R\n",***);
	//fprintf(f,"  /MarkInfo %d 0 R\n",  ***);
	//fprintf(f,"  /Lang (***)\n");
	//fprintf(f,"  /SpiderInfo %d 0 R\n",***);
	//fprintf(f,"  /OutputIntents %d 0 R\n",***);
	fprintf(f,">>\nendobj\n");

	
	 // write out doc info dict:
	infodict=-1;
	//infodict=objcount++;
	//obje->next=new PdfObjData;
	//obje=obje->next;
	//obje->number=infodict;
	//obje->byteoffset=ftell(f);
	//fprintf(f,"%d 0 obj\n<<\n",infodict);
	//fprintf(f,"  /Title (%s)\n",doc->Name()); //***warning, does not sanity check the string
	//fprintf(f,"  /Author (%s)\n",***);
	//fprintf(f,"  /Subject (%s)\n",***);
	//fprintf(f,"  /Keywords (%s)\n",***);
	//fprintf(f,"  /Creator (Laidout %s)\n",LAIDOUT_VERSION);
	//fprintf(f,"  /Producer (Laidout %s)\n",LAIDOUT_VERSION);
	//fprintf(f,"  /CreationDate (%s)\n",***);
	//fprintf(f,"  /ModDate (%s)\n",***);
	//fprintf(f,"  /Trapped /False\n");
	//fprintf(f,">>\nendobj\n");
	
	
	 //write xref
	long xrefpos=writeXrefTable(f,0,objs);

	
	 //write trailer dict, startxref, and EOF
	fprintf(f,"trailer\n<< /Size %d\n",totalobjs);
	fprintf(f,"    /Root %d 0 R\n", doccatalog);
	if (infodict>0) fprintf(f,"    /Info %d 0 R\n", infodict);
	
	//fprintf(f,"    /Encrypt %d***\n", encryption dict);
	//fprintf(f,"    /ID %d\n",      2 string id);
	//fprintf(f,"    /Prev %d\n",    previous_xref_section byte offset);
	
	fprintf(f,">>\n");
	fprintf(f,"startxref\n%ld\n",xrefpos);
	fprintf(f,"%%%%EOF\n");


	DBG cout <<"=================== end pdf out ========================\n";

	return 0;
}

