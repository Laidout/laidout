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
// Copyright (C) 2007 by Tom Lechner
//
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "pdf.h"
#include "../impositions/impositioninst.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



//--------------------------------- install PDF filter

//! Tells the Laidout application that there's a new filter in town.
void installPdfFilter()
{
	//PdfOutputFilter *pdfout=new PdfOutputFilter(3);
	//laidout->exportfilters.push(pdfout);
	
	pdfout=new PdfOutputFilter(4);
	laidout->exportfilters.push(pdfout);
	
	//PdfInputFilter *pdfin=new PdfInputFilter;
	//laidout->importfilters(pdfin);
}


//------------------------------------ PdfOutputFilter ----------------------------------
	
/*! \class PdfOutputFilter
 * \brief Filter for exporting PDF 1.3.
 *
 * \todo implement 1.4!
 */
/*! \var int PdfOutputFilter::pdf_version
 * \brief 4 for 1.4, 3 for 1.3.
 */


PdfOutputFilter::PdfOutputFilter(int which)
{
	if (which==4) pdf_version=1.4; else pdf_version=1.3;
	flags=FILTERS_MULTIPAGE;
}

const char *PdfOutputFilter::Version()
{
	if (pdf_version==4) return "1.4";
	return "1.3";
}

const char *PdfOutputFilter::VersionName()
{
	if (pdf_version==4) return _("Pdf 1.4");
	return _("Pdf 1.3");
}



////--------------------------------*************
//class PdfFilterConfig
//{
// public:
//	char untranslatables;   //whether laidout objects not suitable for pdf should be ignored, rasterized, or approximated
//	char preserveunknown;   //any unknown attributes should be attached as "metadata" to the object in question on readin
//							//these would potentially be written back out on an pdf export?
//							
//	StyleDef *OutputStyleDef(); //for auto config dialog creation
//	StyleDef *InputStyleDef();
//};
////--------------------------------

//----------------------------- pdf out 

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
void pdfdumpobj(FILE *f,
				char *&stream,
				int &objectcount,
				LaxInterfaces::SomeData *obj,
				char *&error_ret,int &warning)
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

//---------------------------- PdfObjData
class PdfObjData 
{
 public:
	unsigned long byteoffset;
	char inuse; //'n' or 'f'
	long number;
	int generation;
	PdfObjData *next;
	char *data;//optional for writing out
	long len; //length of data, just in case data has bytes with 0 value
	PdfObjData() 
	 : byteoffset(0), inuse('n'), number(0), generation(0), next(NULL), data(NULL), len(0)
	 {}
};

//---------------------------- PdfPageInfo
class PdfPageInfo : public PdfObjData
{
 public:
	PdfPageInfo *next;
	int contents;
	DoubleBBox bbox;
	char *pagelabel;
	char *resources;
	PdfPageInfo(int n) { next=NULL; pagelabel=NULL; objnum=n; }
	~PdfPageInfo() { if (next) delete next; if (pagelabel) delete[] pagelabel; }
}:


//! Save the document as PDF.
/*! This does not export EpsData.
 * Files are not checked for existence. They are clobbered if they already exist, and are writable.
 *
 * Return 0 for success, 1 for error and nothing written, 2 for error, and corrupted file possibly written.
 * 2 is mainly for debugging purposes, and will be perhaps be removed in the future.
 * 
 * \todo *** should have option of rasterizing or approximating the things not supported in pdf, such 
 *    as image patch objects
 */
int PdfOutputFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->filename;
	
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paperstyle) return 1;
	
	FILE *f=NULL;
	char *file;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			DBG cerr <<"**** cannot save, doc->saveas is null."<<endl;
			*error_ret=newstr(_("Cannot save to PDF without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".pdf");
	} else file=newstr(filename);
	
	f=fopen(file,"w");
	delete[] file; file=NULL;

	if (!f) {
		DBG cerr <<"**** cannot save, doc->saveas cannot be opened for writing."<<endl;
		*error_ret=newstr(_("Error opening file for writing."));
		return 3;
	}

	if (start<0) start=0;
	else if (start>doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	spread=doc->docstyle->imposition->Layout(layout,start);
	
	int warning=0;
	Spread *spread;
	Group *g;
	//int c;
	int c2,l,pg,c3;

	DBG cerr <<"=================== start pdf out "<<start<<" to "<<end<<" ====================\n";

	 // initialize outside accessible ctm
	psCtmInit();
	psctms.flush();

	 // a fresh PDF is:
	 //   header: %!PDF-1.4
	 //   body: a list of indirect objects
	 //   cross reference table
	 //   trailer
	
	
	 // Start the list of objects with the head of free objects, which
	 // has generation number of 65535. Its number is the object number of
	 // the next free object. Since this is a fresh pdf, there are no 
	 // other free objects.
	PdfObjData *objs=new PdfObjData,
			   *obje=objs,
			   *obj=objs;
	obj->inuse='f';
	obj->number=0; 
	obj->generation=65535;
	int objcount=1;
	
	 // print out header
	if (pdf_version==4) fprintf (f,"%%!PDF-1.4\n");
	else fprintf (f,"%%!PDF-1.3\n");
	fprintf(f,"%%%04x\n",0xffff); //binary file indicator

	
	 //figure out paper size
	int landscape=0;
	if (layout==PAPERLAYOUT) {
		landscape=doc->docstyle->imposition->paperstyle->flags&1;
	}

	 // find basic pdf page info, and generate content streams
	int page, pages;
	Spread *spread;
	PdfPageInfo *pageobjs,*pageobjsstart;
	PdfPageInfo *pg;
	double m[6];
	Page *page;
	char *stream=NULL;
	char scratch[300];
	for (int c=start; c<=end; c++) {
		pg=new PdfPageInfo;
		spread=doc->docstyle->imposition->Layout(layout,c);
		pg->pagelabel=spread->pagesFromSpreadDesc(doc);
		pg->bbox.setbounds(spread->outline);

		transform_set(m,1,0,0,1,0,0);
		appendstr(stream,"q\n"
				  		 "72 0 0 72 0 0 cm\n"); // convert from inches
		psConcat(72.,0.,0.,72.,0.,0.);
		if (landscape) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->imposition->paperstyle->width);
			sprintf(scratch,"0 1 -1 0 %.10g 0 cm\n",doc->docstyle->imposition->paperstyle->width);
			appendstr(stream,scratch);
			psConcat(0.,1.,-1.,0., doc->docstyle->imposition->paperstyle->width,0.);
		}
		
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			//DBG cerr <<"marks data:\n";
			//DBG spread->marks->dump_out(stderr,2,0);
			pdfdumpobj(f,stream,objcount,pageobj->resources,spread->marks,*error_ret,warning);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			psDpi(doc->docstyle->imposition->paperstyle->dpi);
			
			pgindex=spread->pagestack.e[c2]->index;
			if (pgindex<0 || pgindex>=doc->pages.n) continue;
			page=doc->pages.e[pgindex];
			
			 // transform to page
			appendstr(stream,"q\n"); //save ctm
			psPushCtm();
			transform_copy(m,spread->pagestack.e[c2]->outline->m());
			sprintf(scratch,"%.10g %.10g %.10g %.10g %.10g %.10g cm\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			appendstr(stream,stratch);
			psConcat(m);

			 // set clipping region
			DBG cerr <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				pdfSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} 
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n(); l++) {
				pdfdumpobj(f,stream,objcount,page->layers.e(l),*error_ret,warning);
			}

			appendstr(stream,"Q\n"); //pop ctm
			psPopCtm();
		}

		 // print out paper footer
		appendstr(stream,"Q\n"); //pop ctm
		psPopCtm();


		delete spread;

		 // dumpobj will have output objects relevant to the stream. Now dump out this
		 // page's content stream to an object:
		obj=new PdfObjData;
		obj->number=objcount++;
		obj->byteoffset=ftell(f);
		fprintf(f,"%d 0 obj\n"
				  "<< /Length %d >>\n"
				  "stream\n",
				  	obj->number,strlen(stream));
		fprintf(f,stream);  //write(obj->data,1,obj->len,f);
		fprintf(f,"\nendstream\n"
				  "endobj\n");
		delete[] stream;

		pg->contents=obj->number;
		//pg->number=objcount++;
		//pg->byteoffset=ftell(f);
		if (pageobjs) {
			pageobjs->next=pg;
			pageobjs=pageobjs->next;
		} else pageobjs=pageobjsstart=pg;
	}

	
	
	 // write out pdf /Page dicts, which do not have their object number yet
	pages=objcount + end-start+1;
	pageobj=pageobjsstart;
	obj->next=pageobj;
	for (int c=start; c<=end; c++) {
		pageobj->number=objcount++;
		pageobj->byteoffset=ftell(f);

		fprintf(f,"%d 0 obj\n",pageobj->number);
		 //required
		fprintf(f,"<<\n  /Type /Page\n");
		fprintf(f,"  /Parent %d 0 R\n",pages);
		 // would include referenced xobjects!!
		if (pageobj->resources) {
			fprintf(f,"  /Resources <<\n"
					  "%s",pageobj->resources);  //eg "/XObject << /X0 4 0 R"
			fprintf(f,"  >>\n"
		} else 	fprintf(f,"  /Resources << >>\n");
		fprintf(f,"  /MediaBox [%f %f %f %f]\n",
				pageobj->bbox.minx, pageobjs->miny,
				pageobj->bbox.maxx, pageobjs->maxy);
		fprintf(f,"  /Contents %d 0 R\n",pageobj->contents); //not req, but of course necessary if stuff on page
		//fprintf(f,"  /Rotate %d\n",number of 90 increments to rotate clockwise);
		fprintf(f,"  /Rotate 0\n");
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

		if (pageobj->next) obj=pageobj->next;
		pageobj=pageobj->next;
	}
	
	 //write out top /Pages page tree node
	objcount++;
	pageobjs=pageobjsstart;
	fprintf(f,"%d 0 obj\n",pages);
	fprintf(f,"<<\n  /Type /Pages\n");
	fprintf(f,"  /Kids [");
	while (pageobjs) {
		fprintf(f,"%d 0 R ",pageobjs->number);
		pageobjs=pageobjs->next;
	}
	fprintf(f,"]\n");
	fprintf(f,"  /Count %d",end-start+1);
	//can also include (from Page dict): /Resources, /MediaBox, /CropBox, and /Rotate
	fprintf(f,">>\nendobj\n"); 
	

	
		
	 // write out Outlines
	int outlines=-1;
	//int outlines=objcount++
	//***
	
	
	 // write out PageLabels
	int pagelabels=-1;
//	int pagelabels;
//	if (doc->pageranges.n) {
//		pagelabels=objcount++;	
//		//***
//		pagelabels=-1;
//	} else pagelabels=-1;

	
	 // write out Root doc catalog dict:
	 // this must be written after Pages and other items' object numbers figured out
	doccatalog=objcount++;
	obj->next=new PdfObjData;
	obj=obj->next;
	obj->number=doccatalog;
	obj->byteoffset=ftell(f);
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
	//infodict=-1;
	infodict=objcount++;
	time_t t=time(NULL);
	obj->next=new PdfObjData;
	obj=obj->next;
	obj->number=infodict;
	obj->byteoffset=ftell(f);
	fprintf(f,"%d 0 obj\n<<\n",infodict);
	fprintf(f,"  /Title (%s)\n",doc->Name(0)); //***warning, does not sanity check the string
	//fprintf(f,"  /Author (%s)\n",***);
	//fprintf(f,"  /Subject (%s)\n",***);
	//fprintf(f,"  /Keywords (%s)\n",***);
	//fprintf(f,"  /Creator (Laidout %s)\n",LAIDOUT_VERSION);
	//fprintf(f,"  /Producer (Laidout %s)\n",LAIDOUT_VERSION);
	//fprintf(f,"  /CreationDate (%s)\n",***);
	fprintf(f,"  /ModDate (%s)\n",ctime(&t));
	//fprintf(f,"  /Trapped /False\n");
	fprintf(f,">>\nendobj\n");
	
	
	 //write xref table
	long xrefpos=ftell(f);
	int count=0;
	PdfObjData *obj=objs;
	while (obj) { count++; obj=obj->objs; }
	fprintf("xref\n%d %d",0,count);
	obj=objs;
	while (obj) {
		fprintf(f,"%010lu %05d %c\n",obj->byteoffset,obj->generation,obj->inuse);
		obj=obj->next;
	}

	
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


	 //clean up
	//delete ***;
	if (objs) delete objs;
	delete pageobjs;

	DBG cerr <<"=================== end pdf out ========================\n";

	return 0;
	
	
}


