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
#include <lax/interfaces/imagepatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/somedataref.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../printing/psout.h"
#include "pdf.h"
#include "../impositions/impositioninst.h"
#include "../utils.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;

/*! \file 
 * Pdf export code.
 */

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
 * \todo implement difference between 1.3 and 1.4! Currently will only do 1.4.
 */
/*! \var int PdfExportFilter::pdf_version
 * \brief 4 for 1.4, 3 for 1.3.
 */


/*! which==3 is 1.3, 4 is 1.4. However, only 1.4 is implemented.
 */
PdfExportFilter::PdfExportFilter(int which)
{
	pdf_version=4;
//	if (which==4) pdf_version=4; //1.4
//			 else pdf_version=3; //1.3
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
static int o=1;//***DBG

/*! \class PdfObjInfo
 * \brief Temporary class to hold info about pdf objects during export.
 */
class PdfObjInfo 
{
 public:
	int i;//***DBG
	unsigned long byteoffset;
	char inuse; //'n' or 'f'
	long number;
	int generation;
	PdfObjInfo *next;
	char *data;//optional for writing out
	long len; //length of data, just in case data has bytes with 0 value
	PdfObjInfo();
	virtual ~PdfObjInfo();
};

PdfObjInfo::PdfObjInfo()
	 : byteoffset(0), inuse('n'), number(0), generation(0), next(NULL), data(NULL), len(0)
{
	i=o++; 
	DBG cerr<<"creating PdfObjInfo "<<i<<"..."<<endl;
}
PdfObjInfo::~PdfObjInfo()
{
	DBG cerr<<"delete PdfObjInfo i="<<i<<", number="<<number<<"..."<<endl;
	if (next) delete next; 

}
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
	PdfPageInfo() { pagelabel=NULL; }
	virtual ~PdfPageInfo();
};

PdfPageInfo::~PdfPageInfo()
{
	DBG cerr<<"   delete PdfPageInfo i="<<i<<", number="<<number<<"..."<<endl;
	if (pagelabel) delete[] pagelabel; 
}

//----------------forward declarations

static void pdfColorPatch(FILE *f, PdfObjInfo *&obj, char *&stream, int &objectcount,
				  Attribute &resources, ColorPatchData *g);
static void pdfImage(FILE *f, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
					 LaxInterfaces::ImageData *img, char *&error_ret,int &warning);
static void pdfImagePatch(FILE *f, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
					 LaxInterfaces::ImagePatchData *img, char *&error_ret,int &warning);
static void pdfGradient(FILE *f, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
						LaxInterfaces::GradientData *g, char *&error_ret,int &warning);


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
			  "%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
				object->m(0), object->m(1), object->m(2), object->m(3), object->m(4), object->m(5)); 
	appendstr(stream,scratch);
	
	if (!strcmp(object->whattype(),"Group")) {
		Group *g=dynamic_cast<Group *>(object);
		for (int c=0; c<g->n(); c++) 
			pdfdumpobj(f,obj,stream,objectcount,resources,g->e(c),error_ret,warning);
		
//	} else if (!strcmp(object->whattype(),"PathsData")) {
//		PathsData *path=dynamic_cast<PathsData *>(object);
//		if (path) {
//		}
//
	} else if (!strcmp(object->whattype(),"ImagePatchData")) {
		pdfImagePatch(f,obj,stream,objectcount,resources,
				dynamic_cast<ImagePatchData *>(object), error_ret,warning);
		
	} else if (!strcmp(object->whattype(),"ImageData")) {
		pdfImage(f,obj,stream,objectcount,resources,dynamic_cast<ImageData *>(object), error_ret,warning);
		
	} else if (!strcmp(object->whattype(),"ColorPatchData")) {
		pdfColorPatch(f,obj,stream,objectcount,resources,dynamic_cast<ColorPatchData *>(object));
		
	} else if (!strcmp(object->whattype(),"GradientData")) {
		pdfGradient(f,obj,stream,objectcount,resources,dynamic_cast<GradientData *>(object), error_ret,warning);
		
	} else {
		sprintf(scratch,_("Warning: Cannot export %s to Pdf"),object->whattype());
		appendstr(error_ret,scratch);
		warning++;

	}
	
	 // pop axes
	appendstr(stream,"Q\n");
	psPopCtm();
}

//! Output a pdf clipping path from outline.
/*! 
 * outline can be a group of PathsData, a SomeDataRef to a PathsData, 
 * or a single PathsData.
 *
 * Non-PathsData elements in a group does not break the printing.
 * Those extra objects are just ignored.
 *
 * Returns the number of single paths interpreted.
 *
 * If iscontinuing!=0, then doesn't install path yet.
 *
 * \todo *** currently, uses all points (vertex and control points)
 *   in the paths as a polyline, not as the full curvy business 
 *   that PathsData are capable of. when pdf output of paths is 
 *   actually more implemented, this will change..
 */
int pdfSetClipToPath(char *&stream,LaxInterfaces::SomeData *outline,int iscontinuing)//iscontinuing=0
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
	char scratch[200];
	
	 // If is not a path, and is not a ref to a path, but is a group,
	 // then check that its elements 
	if (!path && dynamic_cast<Group *>(outline)) {
		Group *g=dynamic_cast<Group *>(outline);
		SomeData *d;
		double m[6];
		for (int c=0; c<g->n(); c++) {
			d=g->e(c);
			 //add transform of group element
			sprintf(scratch,"%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
					d->m(0), d->m(1), d->m(2), d->m(3), d->m(4), d->m(5)); 
			appendstr(stream,scratch);
			n+=pdfSetClipToPath(stream,g->e(c),1);
			transform_invert(m,d->m());
			 //reverse the transform
			sprintf(stream,"%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			appendstr(stream,scratch);
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
				sprintf(scratch,"%.10f %.10f %s\n",
						p->x(),p->y(), (p==start?"m":"l"));
				appendstr(stream,scratch);
				p=p->next;	
			} while (p && p!=start);
			appendstr(stream,"W n\n");
		}
	}
	
//	if (n && !iscontinuing) {
//		appendstr(stream,"W n\n");
//	}
	return n;
}

//--------------------------------------- PDF Out ------------------------------------

//! Save the document as PDF.
/*! This does not export EpsData.
 * Files are not checked for existence. They are clobbered if they already exist, and are writable.
 *
 * Return 0 for success, 1 for error and nothing written, 2 for error, and corrupted file possibly written.
 * 2 is mainly for debugging purposes, and will be perhaps be removed in the future.
 *
 * error_ret is appended to if possible.
 */
int PdfExportFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
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
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paper)...
		if (error_ret) appendline(*error_ret,_("Nothing to export!"));
		return 1;
	}
	
	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (!doc || isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			if (error_ret) appendstr(*error_ret,_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".ps");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,error_ret);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}


	
	DBG cerr <<"=================== start pdf out "<<start<<" to "<<end<<", papers:"
	DBG      <<papergroup->papers.n<<" ====================\n";

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
	PdfObjInfo *objs=new PdfObjInfo, //head of all pdf objects
			   *obj=NULL;            //temp object pointer
	obj=objs;
	obj->inuse='f';
	obj->number=0; 
	obj->generation=65535;
	int objcount=1;
	
	 // print out header
	if (pdf_version==4) fprintf (f,"%%PDF-1.4\n");
	else fprintf (f,"%%PDF-1.3\n");
	fprintf(f,"%%\xff\xff\n"); //binary file indicator

	
	 //figure out paper orientation
	int landscape=0;
	double paperwidth; //,paperheight;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	 // The ps Orientation comment determines how onscreen viewers will show 
	 // pages. This can be overridden by the %%PageOrientation: comment
	landscape=(papergroup->papers.e[0]->box->paperstyle->flags&1)?1:0;
	paperwidth=papergroup->papers.e[0]->box->paperstyle->width;


	 //object numbers of various dictionaries
	int pages=-1;        //Pages dictionary
	int outlines=-1;     //Outlines dictionary
	int pagelabels=-1;   //PageLabels dictionary
	long doccatalog=-1;  //document's Catalog
	long infodict=-1;    //document's info dict
	
	PdfPageInfo *pageobj=NULL,   //temp pointer
				*pageobjs=NULL;  //points to first page dict
	double m[6];
	Page *page=NULL;   //temp pointer
	char *stream=NULL; //page stream
	char scratch[300]; //temp buffer
	int pgindex;  //convenience variable
	char *desc=NULL;
	int plandscape;
	int p;
	
	 // find basic pdf page info, and generate content streams.
	 // Actual page objects are written out after the contents of all the pages have been processed.
	for (int c=start; c<=end; c++) {
			
		if (spread) { delete spread; spread=NULL; }
		if (doc) spread=doc->docstyle->imposition->Layout(layout,c);
		if (spread) desc=spread->pagesFromSpreadDesc(doc);
		else desc=limbo->id?newstr(limbo->id):NULL;

		for (p=0; p<papergroup->papers.n; p++) {
			if (!pageobjs) {
				pageobjs=pageobj=new PdfPageInfo;
			} else {
				pageobj->next=new PdfPageInfo;
				pageobj=(PdfPageInfo *)pageobj->next;
			}
			plandscape=(papergroup->papers.e[p]->box->paperstyle->flags&1)?1:0;

			pageobj->pagelabel=newstr(desc);//***should be specific to spread/paper
			pageobj->bbox.setbounds(0,
									papergroup->papers.e[p]->box->paperstyle->width,
									0,
									papergroup->papers.e[p]->box->paperstyle->height);

			 //set initial transform: convert from inches and map to paper in papergroup
			//transform_set(m,1,0,0,1,0,0);
			appendstr(stream,"q\n"
							 "72 0 0 72 0 0 cm\n"); // convert from inches
			psConcat(72.,0.,0.,72.,0.,0.);

			 //adjust for landscape
			if (plandscape) {
				 // paperstyle->width 0 translate   90 rotate  
				sprintf(scratch,"0 1 -1 0 %.10f 0 cm\n",paperwidth);
				appendstr(stream,scratch);
				psConcat(0.,1.,-1.,0., paperwidth,0.);
			}

			 //apply papergroup->paper transform
			transform_invert(m,papergroup->papers.e[p]->m());
			sprintf(scratch,"%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			appendstr(stream,scratch);
			psConcat(m);
			
			 //write out limbo object if any
			if (limbo && limbo->n()) {
				pdfdumpobj(f,obj,stream,objcount,pageobj->resources,limbo,*error_ret,warning);
			}

			if (spread) {
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
					sprintf(scratch,"%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
							m[0], m[1], m[2], m[3], m[4], m[5]); 
					appendstr(stream,scratch);
					psConcat(m);

					 // set clipping region
					DBG cerr <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
					if (page->pagestyle->flags&PAGE_CLIPS) {
						pdfSetClipToPath(stream,spread->pagestack.e[c2]->outline,0);
					} 
						
					 // for each layer on the page..
					for (l=0; l<page->layers.n(); l++) {
						pdfdumpobj(f,obj,stream,objcount,pageobj->resources,page->layers.e(l),*error_ret,warning);
					}

					appendstr(stream,"Q\n"); //pop ctm
					psPopCtm();
				}
			}

			 // print out paper footer
			appendstr(stream,"Q\n"); //pop ctm
			psPopCtm();



			 // pdfdumpobj() outputs objects relevant to the stream. Now dump out this
			 // page's content stream XObject to an object:
			obj->next=new PdfObjInfo;
			obj=obj->next;
			obj->number=objcount++;
			obj->byteoffset=ftell(f);
			fprintf(f,"%ld 0 obj\n"
					  "<< /Length %u >>\n"
					  "stream\n",
						obj->number, strlen(stream));
			fwrite(stream,1,strlen(stream),f); //write(obj->data,1,obj->len,f);
			fprintf(f,"\nendstream\n"
					  "endobj\n");
			delete[] stream; stream=NULL;

			pageobj->contents=obj->number;
			//pageobj gets its own number and byte offset later
		}
		if (spread) { delete spread; spread=NULL; }
		if (desc) delete[] desc;
	}

	
	
	 // write out pdf /Page dicts, which do not have their object number or offsets yet.
	int numpages=(end-start+1)*papergroup->papers.n;
	pages=objcount + numpages; //object number of parent Pages dict
	pageobj=pageobjs;
	obj->next=pageobj;
	obj=obj->next; //both obj and pageobj now point to first page object
	for (int c=0; c<numpages; c++) {
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
		fprintf(f,"  /Contents %d 0 R\n",pageobj->contents); //not req, but of course necessary if stuff on page
		if (landscape) {//***ignores per page landscape
			fprintf(f,"  /MediaBox [%f %f %f %f]\n",
					pageobj->bbox.miny*72, pageobjs->bbox.minx*72,
					pageobj->bbox.maxy*72, pageobjs->bbox.maxx*72);
			fprintf(f,"  /Rotate 90\n");   //number of 90 increments to rotate clockwise
		} else {
			fprintf(f,"  /MediaBox [%f %f %f %f]\n",
					pageobj->bbox.minx*72, pageobjs->bbox.miny*72,
					pageobj->bbox.maxx*72, pageobjs->bbox.maxy*72);
			fprintf(f,"  /Rotate 0\n");   //number of 90 increments to rotate clockwise
		}

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
		//fprintf(f,"  /PZ %.10f\n");
		//fprintf(f,"  /SeparationInfo << >>\n");
		fprintf(f,">>\nendobj\n"); 

		if (pageobj->next) obj=pageobj->next;
		pageobj=(PdfPageInfo *)pageobj->next;
	}
	//obj should now point to the final page object, and pageobj should be NULL
	
	 //write out top /Pages page tree node
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
		 //pages is not PdfPageInfo, so final obj makes pageobj==NULL
	}
	fprintf(f,"]\n");
	fprintf(f,"  /Count %d",numpages);
	//can also include (from Page dict): /Resources, /MediaBox, /CropBox, and /Rotate
	fprintf(f,">>\nendobj\n"); 
	
		
	 // write out Outlines
	//outlines=objcount++;
	//***
	
	
	 // write out PageLabels
//	if (doc && doc->pageranges.n) {
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
	const char *title=NULL;
	if (doc) { if (!isblank(doc->Name(0))) title=doc->Name(0); }
	if (!title) title=papergroup->Name;
	if (!title) title=papergroup->name;
	if (title) fprintf(f,"  /Title (%s)\n",title); //***warning, does not sanity check the string
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
	
	DBG cerr <<"feof:"<<feof(f)<<"  ferror:"<<ferror(f)<<endl;
	
	 //write xref table
	long xrefpos=ftell(f);
	int count=0;
	for (obj=objs; obj; obj=obj->next) count++; //should be same as objcount
	DBG cerr <<"objcount:"<<objcount<<"  should == count:"<<count<<endl;

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

	fclose(f);


	 //clean up
	DBG cerr <<"done writing pdf, cleaning up.."<<endl;
	obj=objs;
	while (obj) {
		cerr <<"PbfObjInfo i="<<obj->i<<"  num="<<obj->number<<endl;
		obj=obj->next;
	}
	 //*** double check that this is all that needs cleanup:
	if (objs) delete objs;

	DBG cerr <<"=================== end pdf out ========================\n";

	return 0;
}

//------------------------------- Color Patch Out ---------------------------------

#define LBLT 0
#define LTRT 1
#define RTRB 2
#define RBLB 3
#define LTLB 4
#define LBRB 5
#define RBRT 6 
#define RTLT 7

extern char ro[][16], co[][16];

/*! Write out coordinates for pdf color patchs which require the coordinates 
 * to be scaled so that the range [0..65535] is scaled to the objects bounding box.
 * This writes out 2 bytes per val.
 */
static void writeout(char *stream,int &len,int val)
{
	if (val<0) val=0;
	else if (val>65535) val=65535;
	stream[len++]=(val&0xff00)>>8;
	stream[len++]=val&0xff;
}

/*! Add to stream, using 16 bit coordintates, 8 bit color, and 8 bits for the flag.
 * The coordinates are scaled so that the range [0..65535] is scaled to the objects bounding box.
 *
 * \todo for transparency soft masks, need a version that only appends alpha, not the colors
 */
static void pdfContinueColorPatch(char *&stream,
								  int &len,
								  int &maxlen,
								  ColorPatchData *g,
								  char flag, //!< The edge flag
								  int o,     //!< orientation of the section
								  int r,     //!< Row of upper corner (patch index, not coord index)
								  int c)     //!< Column of upper corner (patch index, not coord index)
{
	 //make sure stream has enough space allocated
	if (!stream) {
		stream=new char[1024];
		maxlen=1024;
		len=0;
	} else if (len+200>maxlen) {
		maxlen+=1024;
		char *nstream=new char[maxlen];
		memcpy(nstream,stream,len);
	}

	stream[len++]=flag;

	r*=3; 
	c*=3;
	int xs=g->xsize, 
		i=r*g->xsize+c,
		cx=g->xsize/3+1, ci=r/3*cx+c/3,
		c3=ci + (ro[o][6]?1:0)*cx + (co[o][6]?1:0),
		c4=ci + (ro[o][9]?1:0)*cx + (co[o][9]?1:0);

	 //write out coords first.
	double ax=65535/(g->maxx-g->minx),
		   bx=ax*g->minx,
		   ay=65535/(g->maxy-g->miny),
		   by=ay*g->miny;
	if (flag==0) {
		writeout(stream,len,(int)(g->points[i+ro[o][ 0]*xs+co[o][ 0]].x*ax-bx));
		writeout(stream,len,(int)(g->points[i+ro[o][ 0]*xs+co[o][ 0]].y*ay-by));
		writeout(stream,len,(int)(g->points[i+ro[o][ 1]*xs+co[o][ 1]].x*ax-bx));
		writeout(stream,len,(int)(g->points[i+ro[o][ 1]*xs+co[o][ 1]].y*ay-by));
		writeout(stream,len,(int)(g->points[i+ro[o][ 2]*xs+co[o][ 2]].x*ax-bx));
		writeout(stream,len,(int)(g->points[i+ro[o][ 2]*xs+co[o][ 2]].y*ay-by));
		writeout(stream,len,(int)(g->points[i+ro[o][ 3]*xs+co[o][ 3]].x*ax-bx));
		writeout(stream,len,(int)(g->points[i+ro[o][ 3]*xs+co[o][ 3]].y*ay-by));
	}
	for (int cc=4; cc<16; cc++) {
		writeout(stream,len,(int)(g->points[i+ro[o][cc]*xs+co[o][cc]].x*ax-bx));
		writeout(stream,len,(int)(g->points[i+ro[o][cc]*xs+co[o][cc]].y*ay-by));
	}

	 //write out colors
	if (flag==0) {
		int c1=ci + (ro[o][0]?1:0)*cx + (co[o][0]?1:0),
			c2=ci + (ro[o][3]?1:0)*cx + (co[o][3]?1:0);
		stream[len++]=g->colors[c1].red/256;
		stream[len++]=g->colors[c1].green/256;
		stream[len++]=g->colors[c1].blue/256;
		stream[len++]=g->colors[c2].red/256;
		stream[len++]=g->colors[c2].green/256;
		stream[len++]=g->colors[c2].blue/256;
	}
	stream[len++]=g->colors[c3].red/256;
	stream[len++]=g->colors[c3].green/256;
	stream[len++]=g->colors[c3].blue/256;
	stream[len++]=g->colors[c4].red/256;
	stream[len++]=g->colors[c4].green/256;
	stream[len++]=g->colors[c4].blue/256;
}


//! Output pdf shading dictionary for a ColorPatchData. 
/*! Currently, the ColorPatchData objects exist only in grids, so this function
 * figures out the proper listing for the DataSource tag of the tensor product 
 * patch shading dictionary.
 *
 * The strategy is do each patch horizontally to the right, then move one down, then do
 * the next row travelling to the left, and so on.
 */
static void pdfColorPatch(FILE *f,
				  PdfObjInfo *&obj,
				  char *&stream,
				  int &objectcount,
				  Attribute &resources,
				  ColorPatchData *g)
{
	//---------generate shading source stream
	int r,c,          // row and column of a patch, not of the coords, which are *3
		rows,columns, // the number of patches
		xs,ys;        //g->xsize and ysize
	xs=g->xsize;
	ys=g->ysize;
	rows=g->ysize/3;
	columns=g->xsize/3;
	r=0;

	int srcstreamlen=0, maxlen=0;
	char *srcstream=NULL;

	 // install first patch
	pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 0,LBLT, 0,0);

	 // handle single column case separately
	if (columns==1) {
		r=1;
		c=0;
		if (r<rows) {
			pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 3,RTLT, r,c);
			r++;
			while (r<rows) {
				if (r%2) pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 2,RTLT, r,c);
					else pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 2,LTRT, r,c);
				r++;
			}
		}
	}
	 
	 // general case
	c=1;
	while (r<rows) {

		 // add patches left to right
		while (c<columns) {
			if (c%2) pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 2,LTLB, r,c);
				else pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 2,LBLT, r,c);
			c++;
		}
		r++;

		 // if more rows, add the next row, travelling right to left
		if (r<rows) {
			c--;
			 // add connection downward, and the patch immediately
			 // to the left of it.
			if (c%2) {
				pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 1,LTRT, r,c);
				c--;
				if (c>=0) pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 3,RBRT, r,c);
				c--;
			} else {
				pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 3,RTLT, r,c);
				c--;
				if (c>=0) { pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 1,RTRB, r,c); }
				c--;
			}

			 // continue adding patches right to left
			while (c>=0) {
				  // add patches leftward
				if (c%2) pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 2,RTRB, r,c);
					else pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 2,RBRT, r,c);
				 c--;
			}
			r++;
			 // add connection downward, and the patch to the right
			if (r<rows) {
				c++;
				 // 0 is always even, so only one kind of extention here
				pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 3,LTRT, r,c);
				c++; // c will be 1 here, and columns we already know is > 1
				pdfContinueColorPatch(srcstream,srcstreamlen,maxlen, g, 1,LTLB, r,c);
				c++;
			}
		}
	}
	//----------done generating stream

	 // shading dict
	obj->next=new PdfObjInfo;
	obj=obj->next;
	obj->byteoffset=ftell(f);
	obj->number=objectcount++;
	int shadedict=obj->number;
	fprintf(f,"%ld 0 obj\n",obj->number);
	fprintf(f,"<<\n"
			  "  /ShadingType 7\n"
			  "  /ColorSpace  /DeviceRGB\n"
			  "  /BitsPerCoordinate 16\n" //coords in range [0..65535], which map by /Decode
			  "  /BitsPerComponent  8\n"
			  "  /BitsPerFlag       8\n"
			  "  /Decode     [%.10f %.10f %.10f %.10f 0 1 0 1 0 1]\n", //xxyy r g b
			  	g->minx,g->maxx,g->miny,g->maxy);
	fprintf(f,"  /Length      %d\n", srcstreamlen);
	fprintf(f,">>\n"
			  "stream\n");
	
	fwrite(srcstream,1,srcstreamlen,f);
	fprintf(f,"\nendstream\n"
			  "endobj\n");

	 //attach to content stream
	char scratch[50];
	sprintf(scratch,"/colorpatch%ld sh\n\n",g->object_id);
	appendstr(stream,scratch);

	 //Add shading function to resources
	Attribute *shading=resources.find("/Shading");
	sprintf(scratch,"/colorpatch%ld %d 0 R\n",g->object_id,shadedict);
	if (shading) {
		appendstr(shading->value,scratch);
	} else {
		resources.push("/Shading",scratch);
	}
}

//--------------------------------------- pdfImage() ----------------------------------------

//! Append an image to pdf export. 
/*! 
 * \todo *** the output should be tailored to a specified dpi, otherwise the output
 *   file will sometimes be quite enormous. Also, repeat images are outputted for each
 *   instance, which also potentially increases size a lot
 * \todo *** this still assumes a LaxImlibImage.
 * \todo image alternates?
 */
static void pdfImage(FILE *f,
					 PdfObjInfo *&obj,
					 char *&stream,
					 int &objectcount,
					 Attribute &resources,
					 LaxInterfaces::ImageData *img,
					 char *&error_ret,int &warning)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	if (!img || !img->image) return;

	LaxImlibImage *imlibimg=dynamic_cast<LaxImlibImage *>(img->image);
	if (!imlibimg) return;
	
	imlib_context_set_image(imlibimg->Image(1));
	int width=imlib_image_get_width(),
		height=imlib_image_get_height();
	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
	
	int softmask=-1;
	if (imlib_image_has_alpha()) { 
		 // softmask image XObject dict
		softmask=objectcount++;

		obj->next=new PdfObjInfo;
		obj=obj->next;
		obj->byteoffset=ftell(f);
		obj->number=softmask;
		fprintf(f,"%ld 0 obj\n",obj->number);
		fprintf(f,"<<\n"
				  "  /Type /XObject\n"
				  "  /Subtype /Image\n"
				  "  /Width  %d\n"
				  "  /Height %d\n",
				 width, height);
		fprintf(f,"  /ColorSpace  /DeviceGray\n"
				  "  /BitsPerComponent  8\n"
				  //"  /Intent \n" (opt 1.1) ignored
				  //"  /Decode     [0 1 0 1 0 1]\n", //(opt) r g b
				  //"  /Interpolate false\n" //(opt)
				  //"  /Matte *** \n" //(opt, 1.4), for use when img is pre-multiplied alpha
				  "  /Length %d\n",
				 width*height);
		fprintf(f,">>\n"
				  "stream\n");

		unsigned char a;
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				a=(buf[y*width+x]&(0xff000000))>>24;
				fprintf(f,"%c",a);
			}
		}
		fprintf(f,"\nendstream\n"
				  "endobj\n");
	}
	


	 // image XObject dict
	obj->next=new PdfObjInfo;
	obj=obj->next;
	obj->byteoffset=ftell(f);
	obj->number=objectcount++;
	int imagexobj=obj->number;
	fprintf(f,"%ld 0 obj\n",obj->number);
	fprintf(f,"<<\n"
			  "  /Type /XObject\n"
			  "  /Subtype /Image\n"
			  "  /Width  %d\n"
			  "  /Height %d\n",
			 width, height);
	fprintf(f,"  /ColorSpace  /DeviceRGB\n"
			  "  /BitsPerComponent  8\n");
			  //"  /Intent \n" (opt 1.1)
			  //"  /ImageMask false\n" // (opt)
			  //"  /Mask  ***\n"  //(opt, 1.3) image mask, stream or array of color keys
	if (softmask>0) 
		fprintf(f,"  /SMask %d 0 R\n", //(opt, 1.4) soft mask for image, overrides mask and gstate mask
				 softmask);
			  //"  /Decode     [0 1 0 1 0 1]\n", //(opt) r g b
			  //"  /Interpolate false\n" //(opt)
			  //"  /Alternates array\n"  //(opt 1.3) not allowed in imgs that are themselves alternates
			  //"  /Name  str\n" //(req 1.0, else opt)
			  //"  /StructParent ??\n"
			  //"  /ID...(opt1.3) \n  /OPI(opt1.2)\n"
			  //"  /Metadata stream\n"

	fprintf(f,"  /Length %d\n", 3*width*height);
	fprintf(f,">>\n"
			  "stream\n");

	unsigned char r,g,b;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			r=(buf[y*width+x]&(0xff0000))>>16;
			g=(buf[y*width+x]&(0xff00))>>8;
			b=buf[y*width+x]& 0xff;
			fprintf(f,"%c%c%c",r,g,b);
		}
	}
	fprintf(f,"\nendstream\n"
			  "endobj\n");

	imlibimg->doneForNow();

	 //attach to content stream
	char scratch[70];
	sprintf(scratch,"%.10f 0 0 %.10f 0 0 cm\n"
					"/image%ld Do\n\n",
				img->maxx,img->maxy,
				img->object_id);
	appendstr(stream,scratch);

	 //Add image XObject function to resources
	Attribute *xobject=resources.find("/XObject");
	sprintf(scratch,"/image%ld %d 0 R\n",img->object_id,imagexobj);
	if (xobject) {
		appendstr(xobject->value,scratch);
	} else {
		resources.push("/XObject",scratch);
	}
}

//--------------------------------------- pdfImagePatch() ----------------------------------------

//! Output pdf for an ImagePatchData. 
/*!
 * \todo *** this is in the serious hack stage
 */
static void pdfImagePatch(FILE *f,
						  PdfObjInfo *&obj,
						  char *&stream,
						  int &objectcount,
						  Attribute &resources,
						  LaxInterfaces::ImagePatchData *i,
						  char *&error_ret,int &warning)
{
	 // make an ImageData covering the bounding box

	Imlib_Image image;
	int width,height;

	flatpoint ul=transform_point(psCTM(),flatpoint(i->minx,i->miny)),
			  ur=transform_point(psCTM(),flatpoint(i->maxx,i->miny)),
			  ll=transform_point(psCTM(),flatpoint(i->minx,i->maxy)),
			  lr=transform_point(psCTM(),flatpoint(i->maxx,i->maxy));
	//DBG cerr <<"  ul: "<<ul.x<<','<<ul.y<<endl;
	//DBG cerr <<"  ur: "<<ur.x<<','<<ur.y<<endl;
	//DBG cerr <<"  ll: "<<ll.x<<','<<ll.y<<endl;
	//DBG cerr <<"  lr: "<<lr.x<<','<<lr.y<<endl;

	width= (int)(sqrt((ul-ur)*(ul-ur))/72*psDpi());
	height=(int)(sqrt((ul-ll)*(ul-ll))/72*psDpi());
	
	image=imlib_create_image(width,height);
	imlib_context_set_image(image);
	imlib_image_set_has_alpha(1);
	DATA32 *buf=imlib_image_get_data();
	memset(buf,0,width*height*4); // make whole transparent/black
	//memset(buf,0xff,width*height*4); // makes whole non-transparent/white
	
	 // create an image where the patch goes
	double m[6]; //takes points from i to buffer
	unsigned char *buffer;
	buffer=(unsigned char *) buf;
	double a=(i->maxx-i->minx)/width,
		   d=(i->miny-i->maxy)/height;
	m[0]=1/a;
	m[1]=0;
	m[2]=0;
	m[3]=1/d;
	m[4]=-i->minx/a;
	m[5]=-i->maxy/d;
	ImagePatchInterface imginterf(0,NULL);
	imginterf.data=i;
	imginterf.renderToBuffer(buffer,m,width,height);
	imginterf.data=NULL;
	imlib_image_put_back_data(buf);
	imlib_image_flip_vertical();
	ImageData img;
	LaxImage *limg=new LaxImlibImage(NULL,image);
	img.SetImage(limg);

	 // set image transform
	transform_invert(img.m(),m);

	 // push axes
	psPushCtm();
	psConcat(img.m());
	char scratch[100];
	sprintf(scratch,"q\n"
			  "%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
				img.m(0), img.m(1), img.m(2), img.m(3), img.m(4), img.m(5)); 
	appendstr(stream,scratch);
	
	pdfImage(f,obj,stream,objectcount,resources,&img, error_ret,warning);

	 // pop axes
	appendstr(stream,"Q\n");
	psPopCtm();
}

//--------------------------------------- pdfGradient() ----------------------------------------

//! Output pdf for a GradientData. 
static void pdfGradient(FILE *f,
						PdfObjInfo *&obj,
						char *&stream,
						int &objectcount,
						Attribute &resources,
						LaxInterfaces::GradientData *g,
						char *&error_ret,int &warning)
{
	if (!g) return;

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
	char scratch[100];

	 // shading dict object
	obj->next=new PdfObjInfo;
	obj=obj->next;
	obj->byteoffset=ftell(f);
	obj->number=objectcount++;
	int shadedict=obj->number;
	fprintf(f,"%ld 0 obj\n",obj->number);
	fprintf(f,"<<\n"
			  "  /ShadingType %d\n",(g->style&GRADIENT_RADIAL)?3:2);
	fprintf(f,"  /ColorSpace /DeviceRGB\n"
			  "    /BBox   [ %.10f %.10f %.10f %.10f ]\n", //[l b r t]
				g->minx, g->miny, g->maxx, g->maxy);
	fprintf(f,"  /Domain [0 1]\n");

	 //Coords of shading dict
	if (g->style&GRADIENT_RADIAL) fprintf(f,"  /Coords [ %.10f 0 %.10f %.10f 0 %.10f ]\n",
			  g->p1, fabs(g->r1), //x0, r0
			  g->p2, fabs(g->r2)); //x1, r1
	else fprintf(f,"  /Coords [ %.10f 0 %.10f 0]\n", g->p1, g->p2);
	fprintf(f,"  /Function %d 0 R\n"
			  ">>\n"
			  "endobj\n", objectcount); //end shading dict


	 //stitchting function object
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
	for (c=1; c<g->colors.n-1; c++) fprintf(f,"%.10f ", (g->colors.e[c]->t-g->colors.e[0]->t)/clen);
	fprintf(f,
			"]\n");
	fprintf(f,"  /Functions [");
	for (c=0; c<g->colors.n-1; c++) fprintf(f,"%d 0 R  ",objectcount+c);
	fprintf(f,"]\n"
			  ">>\n"
			  "endobj\n");


	 //individual function objects
	for (c=1; c<g->colors.n; c++) {
		obj->next=new PdfObjInfo;
		obj=obj->next;
		obj->byteoffset=ftell(f);
		obj->number=objectcount++;
		fprintf(f,"%ld 0 obj\n"
				  "<<\n"
				  "  /FunctionType 2\n"
				  "  /Domain [0 1]\n"
				  "  /C0 [%.10f %.10f %.10f]\n"
				  "  /C1 [%.10f %.10f %.10f]\n"
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


