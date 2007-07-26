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
#include "../printing/psout.h"
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
	//PdfExportFilter *pdfout=new PdfExportFilter(3);
	//laidout->exportfilters.push(pdfout);
	
	PdfExportFilter *pdfout=new PdfExportFilter(4);
	laidout->exportfilters.push(pdfout);
	
	//PdfInputFilter *pdfin=new PdfInputFilter;
	//laidout->importfilters(pdfin);
}


//------------------------------------ PdfExportFilter ----------------------------------
	
/*! \class PdfExportFilter
 * \brief Filter for exporting PDF 1.3 or 1.4.
 *
 * \todo implement difference between 1.3 and 1.4!
 */
/*! \var int PdfExportFilter::pdf_version
 * \brief 4 for 1.4, 3 for 1.3.
 */


PdfExportFilter::PdfExportFilter(int which)
{
	if (which==4) pdf_version=4; //1.4
			 else pdf_version=3; //1.3
	flags=FILTER_MULTIPAGE;
}

const char *PdfExportFilter::Version()
{
	if (pdf_version==4) return "1.4";
	return "1.3";
}

const char *PdfExportFilter::VersionName()
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

//-------------------------------------- pdf out -------------------------------------------

//---------------------------- PdfObjInfo
/*! \class PdfObjInfo
 * \brief Temporary class to hold info about pdf objects during export.
 */
class PdfObjInfo 
{
 public:
	unsigned long byteoffset;
	char inuse; //'n' or 'f'
	long number;
	int generation;
	PdfObjInfo *next;
	char *data;//optional for writing out
	long len; //length of data, just in case data has bytes with 0 value
	PdfObjInfo() 
	 : byteoffset(0), inuse('n'), number(0), generation(0), next(NULL), data(NULL), len(0)
	 {}
	virtual ~PdfObjInfo() { if (next) delete next; }
};

//---------------------------- PdfPageInfo
/*! \class PdfPageInfo
 * \brief Temporary class to hold info about pdf page objects during export.
 */
class PdfPageInfo : public PdfObjInfo
{
 public:
	int contents;
	DoubleBBox bbox;
	char *pagelabel;
	Attribute resources;
	PdfPageInfo() { next=NULL; pagelabel=NULL; }
	virtual ~PdfPageInfo();
};

PdfPageInfo::~PdfPageInfo()
{
	if (pagelabel) delete[] pagelabel; 
}

//-------------------------------- pdfdumpobj

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
				PdfObjInfo *&obj,
				char *&stream,
				int &objectcount,
				Attribute &resources,
				LaxInterfaces::SomeData *object,
				char *&error_ret,
				int &warning)
{
	if (!obj) return;
	
	 // push axes
	psPushCtm();
	psConcat(object->m());
	char scratch[100];
	sprintf(scratch,"q\n"
			  "%.10g %.10g %.10g %.10g %.10g %.10g cm\n ",
				object->m(0), object->m(1), object->m(2), object->m(3), object->m(4), object->m(5)); 
	appendstr(stream,scratch);
	
	if (!strcmp(object->whattype(),"Group")) {
		Group *g=dynamic_cast<Group *>(object);
		for (int c=0; c<g->n(); c++) 
			pdfdumpobj(f,obj,stream,objectcount,resources,g->e(c),error_ret,warning);
		
//	} else if (!strcmp(object->whattype(),"ImagePatchData")) {
//		pdfImagePatch(f,dynamic_cast<ImagePatchData *>(object));
//		
//	} else if (!strcmp(object->whattype(),"ImageData")) {
//		pdfImage(f,dynamic_cast<ImageData *>(object));
//		
//	} else if (!strcmp(object->whattype(),"PathsData")) {
//		PathsData *path=dynamic_cast<PathsData *>(object);
//		if (path) {
//		}
//
//	} else if (!strcmp(object->whattype(),"ColorPatchData")) {
//		pdfColorPatch(f,dynamic_cast<ColorPatchData *>(object));
//		
	} else if (!strcmp(object->whattype(),"GradientData")) {
		GradientData *g=dynamic_cast<GradientData *>(object);
		if (g) {
			 // 1.3 shading dictionaries, built with pattern dicts
			 // <<
			 //  /Type        /Pattern
			 //  /PatternType 2              for shading pattern
			 //  /Shading     dict or stream
			 //  /Matrix      [1 0 0 1 0 0]  (opt)
			 //  /ExtGState    dict           (opt)
			 // >> 
			 // ---OR---
			 // use sh operator which takes a shading dict: name sh, name in Shading element of resource dict
			 // <<
			 //  /ShadingType 1 function based
			 //  			  2 axial
			 //  			  3 radial
			 //  			  4 free triangle  (stream)
			 //  			  5 mesh triangle  (stream)
			 //  			  6 coons  (stream)
			 //  			  7 tensor  (stream)
			 //  /ColorSpace  name or array (req)
			 //  /Background array  (opt) not with sh, used with shadings as part of patterns
			 //  /BBox [l b r t] (opt)
			 //  /AntiAlias    boolean (opt) default false
			 //  ---------type 2:-----------
			 //  /Coords [x0 y0 x1 y1]
			 //  /Domain [t0 t1]   (opt, def: 0 1)
			 //  /Function function  (req) 1-in n-out, n==num color components, in==in /Domain
			 //  /Extend  [bool bool] (opt) whether to extend past start and end points
			 //  ---------type 3:-----------
			 //  /Coords [x0 y0 r0 x1 y1 r1]
			 //  /Domain [t0 t1]   (opt, def: 0 1)
			 //  /Function function  (req) 1-in n-out, n==num color components, in==in /Domain
			 //  /Extend  [bool bool] (opt) whether to extend past start and end points
			 // >>

			int c;
			double clen=g->colors[g->colors.n-1]->t-g->colors[0]->t;

			 // shading dict
			obj->next=new PdfObjInfo;
			obj=obj->next;
			obj->byteoffset=ftell(f);
			obj->number=objectcount++;
			int shadedict=obj->number;
			fprintf(f,"%ld 0 obj\n",obj->number);
			fprintf(f,"<<\n"
					  "  /ShadingType %d\n",(g->style&GRADIENT_RADIAL)?3:2);
			fprintf(f,"  /ColorSpace /DeviceRGB\n"
					  "    /BBox   [ %.10g %.10g %.10g %.10g ]\n", //[l b r t]
					    g->minx, g->miny, g->maxx, g->maxy);
			fprintf(f,"  /Domain [0 1]\n");

			 //Coords
			if (g->style&GRADIENT_RADIAL) fprintf(f,"  /Coords [ %.10g 0 %.10g %.10g 0 %.10g ]\n",
					  g->p1, fabs(g->r1), //x0, r0
					  g->p2, fabs(g->r2)); //x1, r1
			else fprintf(f,"  /Coords [ %.10g 0 %.10g 0]\n", g->p1, g->p2);
			fprintf(f,"  /Function %d 0 R\n"
					  ">>\n"
					  "endobj\n", objectcount);

			 //stitchting function
			obj->next=new PdfObjInfo;
			obj=obj->next;
			obj->byteoffset=ftell(f);
			obj->number=objectcount++;
			fprintf(f,"%ld 0 obj\n",obj->number);
			fprintf(f,"<<\n"
					  "  /FunctionType 3\n"
					  "  /Domain [0 1]\n"
					  "  /Encode [");
			for (c=1; c<g->colors.n; c++) fprintf(f,"0 1 ");
			fprintf(f,"]\n"
					  "  /Bounds [");
			for (c=1; c<g->colors.n-1; c++) fprintf(f,"%.10g ", (g->colors.e[c]->t-g->colors.e[0]->t)/clen);
			fprintf(f,
					"]\n");
			fprintf(f,"  /Functions [");
			for (c=0; c<g->colors.n-1; c++) fprintf(f,"%d 0 R  ",objectcount+c);
			fprintf(f,"]\n"
					  ">>\n"
					  "endobj\n");

			 //individual functions
			for (c=1; c<g->colors.n; c++) {
				obj->next=new PdfObjInfo;
				obj=obj->next;
				obj->byteoffset=ftell(f);
				obj->number=objectcount++;
				fprintf(f,"%ld 0 obj\n"
						  "<<\n"
						  "  /FunctionType 2\n"
						  "  /Domain [0 1]\n"
						  "  /C0 [%.10g %.10g %.10g]\n"
						  "  /C1 [%.10g %.10g %.10g]\n"
						  "  /N 1\n"
						  ">>\n"
						  "endobj\n",
							obj->number, 
							g->colors.e[c-1]->color.red/65535.0, 
							g->colors.e[c-1]->color.green/65535.0,
							g->colors.e[c-1]->color.blue/65535.0, 
							g->colors.e[c  ]->color.red/65535.0,
							g->colors.e[c  ]->color.green/65535.0, 
							g->colors.e[c  ]->color.blue/65535.0
						);
			}

			 //insert gradient to the stream
			sprintf(scratch,"/gradient%ld sh\n",g->object_id);
			appendstr(stream,scratch);

			 //Add shading function to resources
			Attribute *shading=resources.find("/Shading");
			sprintf(scratch,"/gradient%ld %d 0 R\n",g->object_id,shadedict);
			if (shading) {
				appendstr(shading->value,scratch);
			} else {
				resources.push("/Shading",scratch);
			}
		}
		
	} else {
		sprintf(scratch,_("Warning: Cannot export %s to Pdf"),object->whattype());
		appendstr(error_ret,scratch);
		warning++;

	}
	
	 // pop axes
	appendstr(stream,"Q\n");
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
{//***
	cout <<"*** must implement pdfSetClipToPath()!!"<<endl;
	return 0;
//	PathsData *path=dynamic_cast<PathsData *>(outline);
//
//	 //If is not a path, but is a reference to a path
//	if (!path && dynamic_cast<SomeDataRef *>(outline)) {
//		SomeDataRef *ref;
//		 // skip all nested SomeDataRefs
//		do {
//			ref=dynamic_cast<SomeDataRef *>(outline);
//			if (ref) outline=ref->thedata;
//		} while (ref);
//		if (outline) path=dynamic_cast<PathsData *>(outline);
//	}
//
//	int n=0; //the number of objects interpreted
//	
//	 // If is not a path, and is not a ref to a path, but is a group,
//	 // then check that its elements 
//	if (!path && dynamic_cast<Group *>(outline)) {
//		Group *g=dynamic_cast<Group *>(outline);
//		SomeData *d;
//		double m[6];
//		for (int c=0; c<g->n(); c++) {
//			d=g->e(c);
//			 //add transform of group element
//			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
//					d->m(0), d->m(1), d->m(2), d->m(3), d->m(4), d->m(5)); 
//			n+=psSetClipToPath(f,g->e(c),1);
//			transform_invert(m,d->m());
//			 //reverse the transform
//			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
//					m[0], m[1], m[2], m[3], m[4], m[5]); 
//		}
//	}
//	
//	if (!path) return n;
//	
//	 // finally append to clip path
//	Coordinate *start,*p;
//	for (int c=0; c<path->paths.n; c++) {
//		start=p=path->paths.e[c]->path;
//		if (!p) continue;
//		do { p=p->next; } while (p && p!=start);
//		if (p==start) { // only include closed paths
//			n++;
//			p=start;
//			do {
//				fprintf(f,"%.10g %.10g ",p->x(),p->y());
//				if (p==start) fprintf(f,"moveto\n");
//				else fprintf(f,"lineto\n");
//				p=p->next;	
//			} while (p && p!=start);
//			fprintf(f,"closepath\n");
//		}
//	}
//	
//	if (n && !iscontinuing) {
//		//fprintf(f,".1 setlinewidth stroke\n");
//		fprintf(f,"clip\n");
//	}
//	return n;
}

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
int PdfExportFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->filename;
	
	if (!doc->docstyle || !doc->docstyle->imposition 
			|| !doc->docstyle->imposition->paper 
			|| !doc->docstyle->imposition->paper->paperstyle) return 1;
	
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
	if (end<start) end=start;
	else if (end>doc->docstyle->imposition->NumSpreads(layout))
		end=doc->docstyle->imposition->NumSpreads(layout)-1;

	
	DBG cerr <<"=================== start pdf out "<<start<<" to "<<end<<" ====================\n";

	 // initialize outside accessible ctm
	psCtmInit();
	psFlushCtms();

	 // a fresh PDF is:
	 //   header: %PDF-1.4
	 //   body: a list of indirect objects
	 //   cross reference table
	 //   trailer
	
	
	int warning=0;
	Spread *spread=NULL;
	int c2,l;

	 // Start the list of objects with the head of free objects, which
	 // has generation number of 65535. Its number is the object number of
	 // the next free object. Since this is a fresh pdf, there are no 
	 // other free objects.
	PdfObjInfo *objs=new PdfObjInfo,
			   *obj=objs;
	obj->inuse='f';
	obj->number=0; 
	obj->generation=65535;
	int objcount=1;
	
	 // print out header
	if (pdf_version==4) fprintf (f,"%%PDF-1.4\n");
	else fprintf (f,"%%PDF-1.3\n");
	fprintf(f,"%%\xff\xff\n"); //binary file indicator

	
	 //figure out paper size
	int landscape=0;
	if (layout==PAPERLAYOUT) {
		landscape=doc->docstyle->imposition->paper->paperstyle->flags&1;
	}

	 //object numbers of various dictionaries
	int pages=-1;        //Pages dictionary
	int outlines=-1;     //Outlines dictionary
	int pagelabels=-1;   //PageLabels dictionary
	long doccatalog=-1;  //document's Catalog
	long infodict=-1;    //document's info dict
	
	PdfPageInfo *pageobj=NULL,   //temp pointer
				*pageobjs=NULL;
	double m[6];
	Page *page;   //temp pointer
	char *stream=NULL; //page stream
	char scratch[300]; //temp buffer
	int pgindex;  //convenience variable
	
	 // find basic pdf page info, and generate content streams.
	 // Actual page objects are written out after the contents of all the pages have been processed.
	for (int c=start; c<=end; c++) {
		if (!pageobj) {
			pageobj=pageobjs=new PdfPageInfo;
		} else {
			pageobj->next=new PdfPageInfo;
			pageobj=(PdfPageInfo *)pageobj->next;
		}
		spread=doc->docstyle->imposition->Layout(layout,c);
		pageobj->pagelabel=spread->pagesFromSpreadDesc(doc);
		pageobj->bbox.setbounds(spread->path);

		//transform_set(m,1,0,0,1,0,0);
		appendstr(stream,"q\n"
				  		 "72 0 0 72 0 0 cm\n"); // convert from inches
		psConcat(72.,0.,0.,72.,0.,0.);
		if (landscape) {
			 // paperstyle->width 0 translate   90 rotate  
			sprintf(scratch,"0 1 -1 0 %.10g 0 cm\n",doc->docstyle->imposition->paper->paperstyle->width);
			appendstr(stream,scratch);
			psConcat(0.,1.,-1.,0., doc->docstyle->imposition->paper->paperstyle->width,0.);
		}
		
		 // print out printer marks
		 // *** later this will be more like pdf printer mark annotations
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			pdfdumpobj(f,obj,stream,objcount,pageobj->resources,spread->marks,*error_ret,warning);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			psDpi(doc->docstyle->imposition->paper->paperstyle->dpi);
			
			pgindex=spread->pagestack.e[c2]->index;
			if (pgindex<0 || pgindex>=doc->pages.n) continue;
			page=doc->pages.e[pgindex];
			
			 // transform to page
			appendstr(stream,"q\n"); //save ctm
			psPushCtm();
			transform_copy(m,spread->pagestack.e[c2]->outline->m());
			sprintf(scratch,"%.10g %.10g %.10g %.10g %.10g %.10g cm\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			appendstr(stream,scratch);
			psConcat(m);

			 // set clipping region
			DBG cerr <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				pdfSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} 
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n(); l++) {
				pdfdumpobj(f,obj,stream,objcount,pageobj->resources,page->layers.e(l),*error_ret,warning);
			}

			appendstr(stream,"Q\n"); //pop ctm
			psPopCtm();
		}

		 // print out paper footer
		appendstr(stream,"Q\n"); //pop ctm
		psPopCtm();


		delete spread;

		 // pdfdumpobj() outputs objects relevant to the stream. Now dump out this
		 // page's content stream to an object:
		obj->next=new PdfObjInfo;
		obj=obj->next;
		obj->number=objcount++;
		obj->byteoffset=ftell(f);
		fprintf(f,"%ld 0 obj\n"
				  "<< /Length %u >>\n"
				  "stream\n",
				  	obj->number, strlen(stream));
		fprintf(f,stream);  //write(obj->data,1,obj->len,f);
		fprintf(f,"\nendstream\n"
				  "endobj\n");
		delete[] stream; stream=NULL;

		pageobj->contents=obj->number;
		//pageobj gets its own number and byte offset later
	}

	
	
	 // write out pdf /Page dicts, which do not have their object number or offsets yet
	pages=objcount + end-start+1;
	pageobj=pageobjs;
	obj->next=pageobj;
	obj=obj->next; //obj now points to first page object
	for (int c=start; c<=end; c++) {
		pageobj->number=objcount++;
		pageobj->byteoffset=ftell(f);

		fprintf(f,"%ld 0 obj\n",pageobj->number);
		 //required
		fprintf(f,"<<\n  /Type /Page\n");
		fprintf(f,"  /Parent %d 0 R\n",pages);
		 // would include referenced xobjects!!
		if (pageobj->resources.attributes.n) {
			fprintf(f,"  /Resources <<\n");
			for (int c2=0; c2<pageobj->resources.attributes.n; c2++) {
				fprintf(f,"    %s <<\n",pageobj->resources.attributes.e[c2]->name);  //eg "/XObject << /X0 4 0 R"
				fprintf(f,"      %s\n",pageobj->resources.attributes.e[c2]->value);
				fprintf(f,"    >>\n");
			}
			fprintf(f,"  >>\n");
		} else fprintf(f,"  /Resources << >>\n");
		fprintf(f,"  /MediaBox [%f %f %f %f]\n",
				pageobj->bbox.minx*72, pageobjs->bbox.miny*72,
				pageobj->bbox.maxx*72, pageobjs->bbox.maxy*72);
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
		pageobj=(PdfPageInfo *)pageobj->next;
	}
	//obj should now point to the final page object
	
	 //write out top /Pages page tree node ***add to obj->next?
	pages=objcount++;
	pageobj=pageobjs;
	obj->next=new PdfObjInfo;
	obj=obj->next;
	obj->byteoffset=ftell(f);
	obj->number=pages;
	fprintf(f,"%d 0 obj\n",pages);
	fprintf(f,"<<\n  /Type /Pages\n");
	fprintf(f,"  /Kids [");
	while (pageobj) {
		fprintf(f,"%ld 0 R ",pageobj->number);
		pageobj=dynamic_cast<PdfPageInfo *>(pageobj->next);
	}
	fprintf(f,"]\n");
	fprintf(f,"  /Count %d",end-start+1);
	//can also include (from Page dict): /Resources, /MediaBox, /CropBox, and /Rotate
	fprintf(f,">>\nendobj\n"); 
	

	
		
	 // write out Outlines
	//outlines=objcount++;
	//***
	
	
	 // write out PageLabels
//	if (doc->pageranges.n) {
//		pagelabels=objcount++;	
//		//***
//		pagelabels=-1;
//	} else pagelabels=-1;

	
	 // write out Root doc catalog dict:
	 // this must be written after Pages and other items' object numbers figured out
	doccatalog=objcount++;
	obj->next=new PdfObjInfo;
	obj=obj->next;
	obj->number=doccatalog;
	obj->byteoffset=ftell(f);
	fprintf(f,"%ld 0 obj\n<<\n",doccatalog);
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
	infodict=objcount++;
	time_t t=time(NULL);
	obj->next=new PdfObjInfo;
	obj=obj->next;
	obj->number=infodict;
	obj->byteoffset=ftell(f);
	fprintf(f,"%ld 0 obj\n<<\n",infodict);
	if (!isblank(doc->Name(0))) fprintf(f,"  /Title (%s)\n",doc->Name(0)); //***warning, does not sanity check the string
	//fprintf(f,"  /Author (%s)\n",***);
	//fprintf(f,"  /Subject (%s)\n",***);
	//fprintf(f,"  /Keywords (%s)\n",***);
	//fprintf(f,"  /Creator (Laidout %s)\n",LAIDOUT_VERSION);
	//fprintf(f,"  /Producer (Laidout %s)\n",LAIDOUT_VERSION);
	//fprintf(f,"  /CreationDate (%s)\n",***);
	char *tmp=newstr(ctime(&t));
	tmp[strlen(tmp)-1]='\0';
	fprintf(f,"  /ModDate (%s)\n",tmp);
	delete[] tmp;
	//fprintf(f,"  /Trapped /False\n");
	fprintf(f,">>\nendobj\n");
	
	cout <<"feof:"<<feof(f)<<"  ferror:"<<ferror(f)<<endl;
	
	 //write xref table
	long xrefpos=ftell(f);
	int count=0;
	for (obj=objs; obj; obj=obj->next) count++;
	fprintf(f,"xref\n%d %d\n",0,count);
	for (obj=objs; obj; obj=obj->next) {
		fprintf(f,"%010lu %05d %c\n",obj->byteoffset,obj->generation,obj->inuse);
	}

	
	 //write trailer dict, startxref, and EOF
	fprintf(f,"trailer\n<< /Size %d\n",count);
	fprintf(f,"    /Root %ld 0 R\n", doccatalog);
	if (infodict>0) fprintf(f,"    /Info %ld 0 R\n", infodict);
	
	//fprintf(f,"    /Encrypt %d***\n", encryption dict);
	//fprintf(f,"    /ID %d\n",      2 string id);
	//fprintf(f,"    /Prev %d\n",    previous_xref_section byte offset);
	
	fprintf(f,">>\n");
	fprintf(f,"startxref\n%ld\n",xrefpos);
	fprintf(f,"%%%%EOF\n");


	 //clean up
	 //*** double check that this is all that needs cleanup:
	if (objs) delete objs;

	DBG cerr <<"=================== end pdf out ========================\n";

	return 0;
	
	
}


