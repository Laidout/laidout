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
// Copyright (C) 2007-2015 by Tom Lechner
//
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/imagepatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/textonpathinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>

#include <lax/lists.cc>

#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../printing/psout.h"
#include "pdf.h"
#include "../impositions/singles.h"
#include "../utils.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


namespace Laidout {


/*! \file 
 * Pdf import and export code.
 */

//------------------------- PdfImportFilter -----------------------------

/*! \class PdfImportFilter
 *
 * Use MysteryData, with nativeid == page number, and preview as the page preview, generated
 * by ghostscript.
 */
const char *PdfImportFilter::VersionName()
{
	return _("Pdf *");
}

ObjectDef *PdfImportFilter::GetObjectDef()
{
	return NULL; // ***
}

const char *PdfImportFilter::FileType(const char *first100bytes)
{
	if (strncmp(first100bytes,"%PDF-",5)) return NULL;
	if (!strncmp(first100bytes+5,"1.0",3)) return "1.0";
	if (!strncmp(first100bytes+5,"1.1",3)) return "1.1";
	if (!strncmp(first100bytes+5,"1.2",3)) return "1.2";
	if (!strncmp(first100bytes+5,"1.3",3)) return "1.3";
	if (!strncmp(first100bytes+5,"1.4",3)) return "1.4";
	if (!strncmp(first100bytes+5,"1.5",3)) return "1.5";
	if (!strncmp(first100bytes+5,"1.6",3)) return "1.6";
	if (!strncmp(first100bytes+5,"1.7",3)) return "1.7";
	if (!strncmp(first100bytes+5,"1.8",3)) return "1.8";
	if (!strncmp(first100bytes+5,"1.9",3)) return "1.9";
	return NULL;
}

int PdfImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log)
{
	// ***
	// find number of pages
	// generate previews for pages via ghostscript, install with MysteryData
	return 1;
}


//--------------------------------- install PDF filter

//! Tells the Laidout application that there's a new filter in town.
void installPdfFilter()
{
	PdfExportFilter *pdfout=new PdfExportFilter(4);
	pdfout->GetObjectDef();
	laidout->PushExportFilter(pdfout);
	
	//PdfInputFilter *pdfin=new PdfInputFilter;
	//laidout->importfilters(pdfin);
}

//------------------------------------ PdfExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
/*! \todo Options:
 * 		compression rating
 *		compression type for image data, lossy (dctdecode) or lessless (zip?)
 */
Value *newPdfExportConfig()
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Pdf")) 
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
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


//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *PdfExportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("PdfExportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"PdfExportConfig");
	makestr(styledef->Name,_("Pdf Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a pdf."));
	styledef->newfunc=newPdfExportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}



////--------------------------------*************
//class PdfFilterConfig
//{
// public:
//	char untranslatables;   //whether laidout objects not suitable for pdf should be ignored, rasterized, or approximated
//	char preserveunknown;   //any unknown attributes should be attached as "metadata" to the object in question on readin
//							//these would potentially be written back out on an pdf export?
//							
//	ObjectDef *OutputObjectDef(); //for auto config dialog creation
//	ObjectDef *InputObjectDef();
//};
////--------------------------------

//-------------------------------------- pdf out -------------------------------------------

//---------------------------- PdfObjInfo
static int o=1;//***DBG

double current_dpi = 300;

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
	unsigned long lo_object_id;
	anObject *lo_object;

	PdfObjInfo();
	virtual ~PdfObjInfo();
};

PdfObjInfo::PdfObjInfo()
	 : byteoffset(0), inuse('n'), number(0), generation(0), next(NULL), data(NULL), len(0)
{
	i=o++; 
	lo_object_id=0;
	lo_object=NULL;
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
	int rotation;
	int landscape;

	PdfPageInfo() { pagelabel=NULL; rotation=0; landscape=0; }
	virtual ~PdfPageInfo();
};

PdfPageInfo::~PdfPageInfo()
{
	DBG cerr<<"   delete PdfPageInfo i="<<i<<", number="<<number<<"..."<<endl;
	if (pagelabel) delete[] pagelabel; 
}

//----------------forward declarations

static void pdfColorPatch(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount,
				  Attribute &resources, ColorPatchData *g, ErrorLog &log,int &warning, DocumentExportConfig *config);
static void pdfImage(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
					 LaxInterfaces::ImageData *img, ErrorLog &log,int &warning, DocumentExportConfig *config);
static void pdfImagePatch(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
					 LaxInterfaces::ImagePatchData *img, ErrorLog &log,int &warning, DocumentExportConfig *config);
static void pdfGradient(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
						LaxInterfaces::GradientData *g, ErrorLog &log,int &warning, DocumentExportConfig *config);
static void pdfPaths(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
						LaxInterfaces::PathsData *g, ErrorLog &log,int &warning, DocumentExportConfig *config);
static void pdfCaption(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
						LaxInterfaces::CaptionData *g, ErrorLog &log,int &warning, DocumentExportConfig *config);
static void pdfTextOnPath(FILE *f, PdfObjInfo *objs, PdfObjInfo *&obj, char *&stream, int &objectcount, Attribute &resources,
						LaxInterfaces::TextOnPath *g, ErrorLog &log,int &warning, DocumentExportConfig *config);


//-------------------------------- pdfdumpobj

//! Internal function to dump out the object in PDF. Called by pdfout().
/*! \ingroup pdf
 * It is assumed that the transform of the object is applied here, rather than
 * before this function is called.
 *
 * Should be able to handle gradients, bez color patches, paths, and images
 * without significant problems, EXCEPT for the lack of decent transparency handling.
 *
 * New XObjects are created and written to f as needed, and pushed onto obj, which is assumed
 * to be the uppermost xobject already written to f. objs is the first.
 *
 * Besides XObjects, drawing commands are appended to stream, which is written out as an XObject
 * elsewhere, when a page is finished being processed.
 *
 * \todo *** must be able to do color management
 * \todo *** need integration of global units, assumes inches now. generally must work
 *    out what shall be the default working units...
 */
void pdfdumpobj(FILE *f,
				PdfObjInfo *objs, 
				PdfObjInfo *&obj,
				char *&stream,
				int &objectcount,
				Attribute &resources,
				LaxInterfaces::SomeData *object,
				ErrorLog &log,
				int &warning,
				DocumentExportConfig *config)
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
			pdfdumpobj(f,objs,obj,stream,objectcount,resources,g->e(c),log,warning,config);

	} else if (!strcmp(object->whattype(),"PathsData")) {
		pdfPaths(f,objs,obj,stream,objectcount,resources,
				dynamic_cast<PathsData *>(object), log,warning,config);

	} else if (!strcmp(object->whattype(),"ImagePatchData")) {
		pdfImagePatch(f,objs,obj,stream,objectcount,resources,
				dynamic_cast<ImagePatchData *>(object), log,warning,config);

	} else if (!strcmp(object->whattype(),"ImageData")) {
		pdfImage(f,objs,obj,stream,objectcount,resources,dynamic_cast<ImageData *>(object), log,warning,config);

	} else if (!strcmp(object->whattype(),"ColorPatchData")) {
		pdfColorPatch(f,objs,obj,stream,objectcount,resources,dynamic_cast<ColorPatchData *>(object), log,warning,config);

	} else if (!strcmp(object->whattype(),"GradientData")) {
		pdfGradient(f,objs,obj,stream,objectcount,resources,dynamic_cast<GradientData *>(object), log,warning,config);

	} else if (!strcmp(object->whattype(),"CaptionData")) {
		if (config->textaspaths) {
			CaptionData *text = dynamic_cast<CaptionData*>(object);
			SomeData *path = text->ConvertToPaths(false, NULL);
			pdfPaths(f,objs,obj,stream,objectcount,resources, dynamic_cast<PathsData *>(object), log,warning,config);
            path->dec_count();
		} else {
			pdfCaption(f,objs,obj,stream,objectcount,resources,dynamic_cast<CaptionData *>(object), log,warning,config);
		}

	} else if (!strcmp(object->whattype(),"TextOnPath")) {
		if (config->textaspaths) {
			TextOnPath *text = dynamic_cast<TextOnPath*>(object);
			SomeData *path = text->ConvertToPaths(false, NULL);
			pdfPaths(f,objs,obj,stream,objectcount,resources, dynamic_cast<PathsData *>(object), log,warning,config);
            path->dec_count();
		} else {
			pdfTextOnPath(f,objs,obj,stream,objectcount,resources,dynamic_cast<TextOnPath*>(object), log,warning,config);
		}

	} else {
		DrawableObject *dobj=dynamic_cast<DrawableObject*>(object);
        SomeData *dobje=NULL;
        if (dobj) dobje=dobj->EquivalentObject();

        if (dobje) {
            dobje->Id(object->Id());

			appendstr(stream,"Q\n");
			psPopCtm(); 
            pdfdumpobj(f,objs,obj,stream,objectcount,resources,dobje,log,warning,config);

            dobje->dec_count();

        } else {

            setlocale(LC_ALL,"");
            char buffer[strlen(_("Cannot export %s objects to pdf."))+strlen(object->whattype())+1];
            sprintf(buffer,_("Cannot export %s objects to pdf."),object->whattype());
            log.AddMessage(object->object_id,object->nameid,NULL, buffer,ERROR_Warning);
            setlocale(LC_ALL,"C");
            warning++;
        }

		return;

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
 */
int PdfExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *config=dynamic_cast<DocumentExportConfig *>(context);
	if (!config) return 1;

	Document *doc =config->doc;
	int start     =config->start;
	int end       =config->end;
	int layout    =config->layout;
	Group *limbo  =config->limbo;
	PaperGroup *papergroup=config->papergroup;
	if (!filename) filename=config->filename;
	
	if (config->reverse_order) { int temp=start; start=end; end=temp; }


	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}
	
	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (!doc || isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".pdf");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,&log);
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}


	setlocale(LC_ALL,"C");
	
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
	fprintf(f,"%%\xff\xff\xff\xff\n"); //4 byte binary file indicator

	
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
	int paperrotate;
	int p;
	
	 // find basic pdf page info, and generate content streams.
	 // Actual page objects are written out after the contents of all the pages have been processed.
	for (int c=start; (end>=start ? c<=end : c>=end); (end>=start ? c++ : c--)) {
		if (config->evenodd==DocumentExportConfig::Even && c%2==0) continue;
        if (config->evenodd==DocumentExportConfig::Odd && c%2==1) continue;
			
		if (spread) { delete spread; spread=NULL; }
		if (doc) spread=doc->imposition->Layout(layout,c);
		if (spread) desc=spread->pagesFromSpreadDesc(doc);
		else desc = limbo->Id() ? newstr(limbo->Id()) : NULL;

		for (p=0; p<papergroup->papers.n; p++) {
			if (!pageobjs) {
				pageobjs=pageobj=new PdfPageInfo;
			} else {
				pageobj->next=new PdfPageInfo;
				pageobj=(PdfPageInfo *)pageobj->next;
			}

			paperrotate=config->paperrotation;
			if (config->rotate180 && c%2==1) paperrotate+=180;
			if (paperrotate>=360) paperrotate-=360; 
			pageobj->rotation = paperrotate;
			pageobj->landscape=papergroup->papers.e[p]->box->paperstyle->landscape();

			pageobj->pagelabel=newstr(desc);//***should be specific to spread/paper
			//not we don't need to explicitly worry about landscape: papergroup->papers.e[p]->box->paperstyle->landscape();
			//since paper->w() and h() take it into account. paperrotate is something different
			pageobj->bbox.setbounds(0,
									papergroup->papers.e[p]->box->paperstyle->w(), //takes in to account paper landscape
									0,
									papergroup->papers.e[p]->box->paperstyle->h());

			 //set initial transform: convert from inches and map to paper in papergroup
			//transform_set(m,1,0,0,1,0,0);
			appendstr(stream,"q\n"
							 "72 0 0 72 0 0 cm\n"); // convert from inches
			psConcat(72.,0.,0.,72.,0.,0.);


			 //apply papergroup->paper transform
			transform_invert(m,papergroup->papers.e[p]->m());
			sprintf(scratch,"%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
			appendstr(stream,scratch);
			psConcat(m);
			
			 //write out limbo object if any
			if (limbo && limbo->n()) {
				pdfdumpobj(f,objs,obj,stream,objcount,pageobj->resources,limbo,log,warning,config);
			}

			 //write out any papergroup objects
			if (papergroup->objs.n()) {
				pdfdumpobj(f,objs,obj,stream,objcount,pageobj->resources,&papergroup->objs,log,warning,config);
			}

			if (spread) {
				 // print out printer marks
				 // *** later maybe this will be more like pdf printer mark annotations
				if ((spread->mask&SPREAD_PRINTERMARKS) && spread->marks) {
					pdfdumpobj(f,objs,obj,stream,objcount,pageobj->resources,spread->marks,log,warning,config);
				}
				
				 // for each paper in paper layout..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
					PaperStyle *defaultpaper=doc->imposition->GetDefaultPaper();
					psDpi(defaultpaper->dpi);
					
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
						pdfdumpobj(f,objs,obj,stream,objcount,pageobj->resources,page->layers.e(l),log,warning,config);
					}

					appendstr(stream,"Q\n"); //pop ctm, page transform
					psPopCtm();
				}
			}

			 // print out paper footer
			appendstr(stream,"Q\n"); //pop papergroup transform
			psPopCtm();
//			if (paperrotate>0) {
//				appendstr(stream,"Q\n"); //pop paper rotation transform
//				psPopCtm();
//			}
			//appendstr(stream,"Q\n"); //pop  pt to inches conversion (not really necessary
			//psPopCtm();



			 // pdfdumpobj() outputs objects relevant to the stream. Now dump out this
			 // page's content stream XObject to an object:
			obj->next=new PdfObjInfo;
			obj=obj->next;
			obj->number=objcount++;
			obj->byteoffset=ftell(f);
			fprintf(f,"%ld 0 obj\n"
					  "<< /Length %lu >>\n"
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
	//int numpages=(end-start+1)*papergroup->papers.n;
	int numpages=0;
	for (pageobj=pageobjs; pageobj; pageobj=(PdfPageInfo *)pageobj->next) numpages++;

	pages=objcount + numpages; //object number of parent Pages dict
	pageobj=pageobjs;
	obj->next=pageobj;
	obj=obj->next; //both obj and pageobj now point to first page object

	while (pageobj) {
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


		fprintf(f,"  /Rotate %d\n", pageobj->rotation);   //number of 90 increments to rotate clockwise
//		if (pageobj->rotation==90 || pageobj->rotation==270) {
//			fprintf(f,"  /MediaBox [%f %f %f %f]\n",
//					pageobj->bbox.miny*72, pageobjs->bbox.minx*72,
//					pageobj->bbox.maxy*72, pageobjs->bbox.maxx*72);
//		} else {
//			fprintf(f,"  /MediaBox [%f %f %f %f]\n",
//					pageobj->bbox.minx*72, pageobjs->bbox.miny*72,
//					pageobj->bbox.maxx*72, pageobjs->bbox.maxy*72);
//		}
		fprintf(f,"  /MediaBox [%f %f %f %f]\n",
				pageobj->bbox.minx*72, pageobjs->bbox.miny*72,
				pageobj->bbox.maxx*72, pageobjs->bbox.maxy*72);


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
//	if (is booklet type layout....) {
//		SignatureImposition *dss=dynamic_cast<SignatureImposition *>(doc->imposition);
//		if (dss->IsVertical() && ***)
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
	//fprintf(f,"  /Creator (Laidout %s)\n",LAIDOUT_VERSION);  //for pdf creators
	//fprintf(f,"  /Producer (Laidout %s)\n",LAIDOUT_VERSION); //for pdf convertors
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
		fprintf(f,"%010lu %05d %c \n",obj->byteoffset,obj->generation,obj->inuse);
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
	setlocale(LC_ALL,"");


	 //clean up
	DBG cerr <<"done writing pdf, cleaning up.."<<endl;
	obj=objs;
	while (obj) {
		DBG cerr <<"PbfObjInfo i="<<obj->i<<"  num="<<obj->number<<endl;
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
	} else if (len+400>maxlen) {
		maxlen+=1024;
		char *nstream=new char[maxlen];
		memcpy(nstream,stream,len);
		delete[] stream;
		stream=nstream;
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
				  PdfObjInfo *objs, 
				  PdfObjInfo *&obj,
				  char *&stream,
				  int &objectcount,
				  Attribute &resources,
				  ColorPatchData *g,
				  ErrorLog &log,int &warning, DocumentExportConfig *config)
{
	//---------generate shading source stream
	int r,c,          // row and column of a patch, not of the coords, which are *3
		rows,columns; // the number of patches
	//	xs,ys;        //g->xsize and ysize
	//xs=g->xsize;
	//ys=g->ysize;
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

/*! Assuming argb, return true if any a is not 255.
 */
bool image_has_alpha(const unsigned char *buf, int len)
{
	int i=3;
	for (int c=0; c<len; c++) {
	  if (buf[i]<255) return true;
	  i+=4;
	}

	return false;
}

void ascii_out(unsigned char *buf, int width, int height)
{
	double w=100;
	double h=w*height/width;

	int xx,yy,i;

	cerr <<"dump buf:"<<endl;
	for (int x=0; x<w; x++) {
	  for (int y=0; y<h; y++) {
		xx=x/w*width;
		yy=y/h*height;
		i=4*(yy*width + xx);
		cerr <<(char)(32+(buf[i]/255.)*94);
  	  }
	  cerr <<endl;
	}
	cerr <<endl;
}

//! Append an image to pdf export. 
/*! 
 * \todo *** the output should be tailored to a specified dpi, otherwise the output
 *   file will sometimes be quite enormous. Also, repeat images are outputted for each
 *   instance, which also potentially increases size a lot
 * \todo image alternates?
 * \todo DCTDecode for jpgs
 */
static void pdfImage(FILE *f,
					 PdfObjInfo *objs, 
					 PdfObjInfo *&obj,
					 char *&stream,
					 int &objectcount,
					 Attribute &resources,
					 LaxInterfaces::ImageData *img,
					 ErrorLog &log,int &warning, DocumentExportConfig *config)
{
	 // the image gets put in a postscript box with sides 1x1, and the matrix
	 // in the image is ??? so must set
	 // up proper transforms
	
	if (!img || !img->image) return;



	// *** search current PDfObjInfos for occurance of this image.
	//     if found, add reference to that, rather than add duplicate.
	//     WARNING! must be careful to include the found existing object
	//     in resources! resources is fresh for each page.
	PdfObjInfo *existing=objs;
	while (existing) {
		if (existing->lo_object_id==img->object_id) break;
		existing=existing->next;
	}

	int imagexobj=-1;
	if (existing) {
		imagexobj=existing->number;

	} else {
		LaxImage *image=img->image;

		int width =image->w();
		int height=image->h();
		unsigned char *buf=image->getImageBuffer(); // ARGB

		int softmask=-1;
		if (image_has_alpha(buf,width*height)) { 
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
			int i=3;
			for (int y=0; y<height; y++) {
				for (int x=0; x<width; x++) {
					a=buf[i];
					i+=4;
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
		obj->lo_object_id=img->object_id;
		imagexobj=obj->number;
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
		int i=0;
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				b=buf[i++];
				g=buf[i++];
				r=buf[i++];
				i++;
				fprintf(f,"%c%c%c",r,g,b);

				//DBG cerr <<(char)(32+(buf[i]/255.)*94);
			}
		}
		fprintf(f,"\nendstream\n"
				  "endobj\n");

		image->doneWithBuffer(buf);
	} //if !existing


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
					 	  PdfObjInfo *objs, 
						  PdfObjInfo *&obj,
						  char *&stream,
						  int &objectcount,
						  Attribute &resources,
						  LaxInterfaces::ImagePatchData *i,
						  ErrorLog &log,int &warning, DocumentExportConfig *config)
{
	 // make an ImageData covering the bounding box

	int width,height;

	flatpoint ul=transform_point(psCTM(),flatpoint(i->minx,i->miny)),
			  ur=transform_point(psCTM(),flatpoint(i->maxx,i->miny)),
			  ll=transform_point(psCTM(),flatpoint(i->minx,i->maxy));
			  //lr=transform_point(psCTM(),flatpoint(i->maxx,i->maxy));
	//DBG cerr <<"  ul: "<<ul.x<<','<<ul.y<<endl;
	//DBG cerr <<"  ur: "<<ur.x<<','<<ur.y<<endl;
	//DBG cerr <<"  ll: "<<ll.x<<','<<ll.y<<endl;
	//DBG cerr <<"  lr: "<<lr.x<<','<<lr.y<<endl;

	width= (int)(sqrt((ul-ur)*(ul-ur))/72*psDpi());
	height=(int)(sqrt((ul-ll)*(ul-ll))/72*psDpi());
	
	LaxImage *image = create_new_image(width,height);
	unsigned char *buffer=image->getImageBuffer();
	memset(buffer,0,width*height*4); // make whole transparent/black
	//memset(buf,0xff,width*height*4); // makes whole non-transparent/white
	
	 // create an image where the patch goes
	double m[6]; //takes points from i to buffer
	double a=(i->maxx-i->minx)/width,
		   d=(i->miny-i->maxy)/height;
	m[0]=1/a;
	m[1]=0;
	m[2]=0;
	m[3]=1/d;
	m[4]=-i->minx/a;
	m[5]=-i->maxy/d;
	i->renderToBuffer(buffer,width,height, 0,8,4);
	//imlib_image_flip_vertical();
	image->doneWithBuffer(buffer);
	ImageData img;
	img.SetImage(image, NULL);
	image->dec_count();

	 // set image transform
	double mm[6];
	transform_invert(mm,m);
	img.m(mm);
	img.Flip(0);
	//img.Flip(img.transformPoint(flatpoint(img.minx,(img.miny+img.maxy)/2)),
			 //img.transformPoint(flatpoint(img.maxx,(img.miny+img.maxy)/2)));

	 // push axes
	psPushCtm();
	psConcat(img.m());
	char scratch[100];
	sprintf(scratch,"q\n"
			  "%.10f %.10f %.10f %.10f %.10f %.10f cm\n ",
				img.m(0), img.m(1), img.m(2), img.m(3), img.m(4), img.m(5)); 
	appendstr(stream,scratch);
	
	pdfImage(f,objs,obj,stream,objectcount,resources,&img, log,warning,config);

	 // pop axes
	appendstr(stream,"Q\n");
	psPopCtm();
}

//--------------------------------------- pdfCaption() ----------------------------------------

//! Output pdf for a CaptionData. 
static void pdfTextOnPath(FILE *f,
					 	PdfObjInfo *objs, 
						PdfObjInfo *&obj,
						char *&stream,
						int &objectcount,
						Attribute &resources,
						LaxInterfaces::TextOnPath *text,
						ErrorLog &log,int &warning, DocumentExportConfig *config)
{
	if (!text) return;

	setlocale(LC_ALL,"");
	log.AddMessage(text->object_id, text->Id(), NULL, _("Unimplemented pdf textonpath out!"), ERROR_Warning);
	setlocale(LC_ALL,"C");
	warning++;
}

//--------------------------------------- pdfCaption() ----------------------------------------

//! Output pdf for a CaptionData. 
static void pdfCaption(FILE *f,
					 	PdfObjInfo *objs, 
						PdfObjInfo *&obj,
						char *&stream,
						int &objectcount,
						Attribute &resources,
						LaxInterfaces::CaptionData *caption,
						ErrorLog &log,int &warning, DocumentExportConfig *config)
{
	if (!caption) return;

	LaxFont *font=caption->font;

	char scratch[100];


	// search for existing font object 
	int fontdict=0;
	PdfObjInfo *existing=objs;
	while (existing) {
		if (existing->lo_object_id==font->object_id) break;
		existing=existing->next;
	}

	if (existing) {
		 //found existing!
		fontdict=obj->number;

	} else {
		 //Must create a new font object..
		obj->next=new PdfObjInfo;
		obj=obj->next;
		obj->byteoffset = ftell(f);
		obj->number = objectcount++;
		obj->lo_object_id = font->object_id;
		fontdict=obj->number;

		const char *file=font->FontFile();
		if (!S_ISREG(file_exists(file,1,NULL))) {
			 //can't find font file, just use Helvetica
			fprintf(f,"%ld 0 obj\n",obj->number);
			fprintf(f,"<<\n"
					  "  /Type /Font\n"
					  "  /Subtype /Type1\n");
			fprintf(f,"  /BaseFont /Helvetica\n");
			fprintf(f,">>\n");

            setlocale(LC_ALL,"");
            char buffer[strlen(_("Using Helvetica in place of mystery font %s."))+strlen(font->Family())+1];
            sprintf(buffer,_("Using Helvetica in place of mystery font %s."), font->Family());
            log.AddMessage(caption->object_id, caption->Id(), NULL, buffer, ERROR_Warning);
            setlocale(LC_ALL,"C");
            warning++;

		} else {
			 //need to create a font dict corresponding to font at that file
			int widths = obj->number+1;
			int fontdescriptor = obj->number+2;

			FT_Library *ft_library=anXApp::app->fontmanager->GetFreetypeLibrary();
			if (!ft_library) return; //this shouldn't happen!


			 //scan in from a freetype face
			FT_Face ft_face=NULL;
			FT_Error ft_error = FT_New_Face(*ft_library, font->FontFile(), 0, &ft_face);
			if (ft_error) {
				DBG cerr <<" ERROR loading "<<font->FontFile()<<" with FT_New_Face"<<endl;
				return;
			}

			 //find defined characters for font
			int code;
			int firstchar=0, lastchar=0;
			FT_UInt gindex=0;

			firstchar = lastchar = code = FT_Get_First_Char( ft_face, &gindex );

			while (gindex != 0) {
				//chars.push(code);
				code = FT_Get_Next_Char( ft_face, code, &gindex );
				if (gindex!=0) lastchar=code;
			}

			const char *fonttype="Type1";
			FontManager *fontmanager=GetDefaultFontManager();
			FontDialogFont *fontinfo=fontmanager->FindFontFromFile(file);
			if (fontinfo) {
				if (     !strcmp(fontmanager->GetTagName(fontinfo->format),"Type 1")) fonttype="Type1";
				else if (!strcmp(fontmanager->GetTagName(fontinfo->format),"TrueType")) fonttype="TrueType";
				else if (!strcmp(fontmanager->GetTagName(fontinfo->format),"CFF")) fonttype="TrueType"; // *** big assumption!!
			}

			fprintf(f,"%ld 0 obj\n",obj->number);
			fprintf(f,"<<\n"
					  "  /Type /Font\n"
					  "  /Subtype /%s\n", fonttype);
			fprintf(f,"  /BaseFont /%s\n", font->PostscriptName());

			fprintf(f,"  /FirstChar %d\n", firstchar);
			fprintf(f,"  /LastChar %d\n", lastchar);
			fprintf(f,"  /Widths %d 0 R\n", widths);
			fprintf(f,"  /FontDescriptor %d 0 R\n", fontdescriptor);
			//fprintf(f,"  /Encoding %d\n", ***); //optional, name or dict
			//fprintf(f,"  /ToUnicode %d\n", ***); //(Optional; PDF 1.2) A stream containing a CMap file that maps character codes to Unicode values
			fprintf(f,">>\n");


			 //Add widths array object.
			 // one value per character, LastChar-FirstChar+1 entries, in units 1/1000 of text unit
			obj->next=new PdfObjInfo;
			obj=obj->next;
			obj->byteoffset = ftell(f);
			obj->number = objectcount++;

			fprintf(f,"%ld 0 obj\n",obj->number);
			fprintf(f,"<<\n [ ");
			for (int c=firstchar; c<=lastchar; c++) {
				gindex = FT_Get_Char_Index(ft_face, c);
				if (gindex==0) {
					fprintf(f, "0 ");
					continue;
				}

				FT_Load_Glyph( ft_face, gindex, FT_LOAD_NO_SCALE );
				fprintf(f, "%d ", (int)ft_face->glyph->advance.x);
			}
			fprintf(f," ]\n");
			fprintf(f,">>\n");


			 //Add FontDescriptor object
			obj->next=new PdfObjInfo;
			obj=obj->next;
			obj->byteoffset = ftell(f);
			obj->number = objectcount++;

			fprintf(f,"%ld 0 obj\n",obj->number);
			fprintf(f,"<<\n"
					  "  /Type /FontDescriptor\n"
					  "  /FontName /%s\n", font->PostscriptName());
			// /FontFamily    optional, pdf 1.5
			// /FontStretch   optional, pdf 1.5, must be one of UltraCondensed , ExtraCondensed ,
			//                   Condensed , SemiCondensed , Normal , SemiExpanded , Expanded , ExtraExpanded or UltraExpanded
			// /FontWeight    optional, pdf 1.5, must be one of 100, 200, 300, 400, 500, 600, 700, 800, or 900. 400 is normal, 700 is bold

			// /Flags         required. Used as hints for possible substitution.
			//                   or'd Bits: 1=Fixed pitch, 2=serif, 3=symbolic (any chars outside Adobe standard latin)
			//                              4=script, 6=nonsymbolic (3 or 6 MUST be set), 7=italic, 17=all cap, 18=small cap, 19=force bold
			unsigned int flags=0;
			if (FT_IS_FIXED_WIDTH(ft_face)) flags|=(1<<1);
			if (ft_face->style_flags & FT_STYLE_FLAG_ITALIC) flags|=(1<<7);
			fprintf(f,"  /Flags %u\n", flags);

			// /FontBBox      required except for Type3 fonts
			fprintf(f,"  /Flags [ %d %d %d %d ]\n",
					(int)ft_face->bbox.xMin, (int)ft_face->bbox.yMin,
					(int)ft_face->bbox.xMax, (int)ft_face->bbox.yMax);

			// /ItalicAngle   required. degrees off vertical
			fprintf(f,"  /ItalicAngle 0\n"); // ***using 0 because all this meta gets me down

			// /Ascent        required except for Type3 fonts.
			fprintf(f,"  /Ascent %d\n", (int)ft_face->ascender);

			// /Descent       required except for Type3 fonts.
			fprintf(f,"  /Descent %d\n", (int)ft_face->descender);

			// /Leading       optional (default 0)
			if (ft_face->height!=ft_face->ascender-ft_face->descender)
				fprintf(f,"  /Leading %d\n", (int)(ft_face->height - (ft_face->ascender-ft_face->descender)));

			// /CapHeight     Required for fonts that have Latin characters, except for Type 3 fonts
			DBG cerr <<" Warning! using totally made up value for CapHeight in pdf out!!"<<endl;
			fprintf(f,"  /CapHeight 100\n"); // *** WARNING!! MADE UP VALUE!!

			// /XHeight       optional
			// /StemV         required except for Type3 fonts, thickness of vertical strokes
			DBG cerr <<" Warning! using totally made up value for StemV in pdf out!!"<<endl;
			fprintf(f,"  /StemV 100\n"); // *** WARNING!! MADE UP VALUE!!

			// /StemH         optional
			// /AvgWidth      (Optional) The average width of glyphs in the font. Default value: 0.
			// /MaxWidth      (Optional) The maximum width of glyphs in the font. Default value: 0.
			// /MissingWidth  (Optional) The width to use for chars not in widths array
			// /FontFile      (Optional) A stream containing a Type 1 font program (see Section 5.8, “Embedded Font Programs”).
			// /FontFile2     (Optional; PDF 1.1) A stream containing a TrueType font program (see Section 5.8, “Embedded Font Programs”).
			// /FontFile3     (Optional; PDF 1.2) A stream containing a font program whose format is specified by the 
			//                  Subtype entry in the stream dictionary (see Table 5.23 and implementation note 68 in Appendix H).
			//                  At most, only one of the FontFile , FontFile2 , and FontFile3 entries may be present.
			// /CharSet       (Optional; meaningful only in Type 1 fonts; PDF 1.1) An ascii or byte string listing the char-
			//					 acter names defined in a font subset. The names in this string must be in PDF
			// 					syntax—that is, each name preceded by a slash ( / ). The names can appear in
			// 					any order. The name . notdef should be omitted; it is assumed to exist in the
			// 					font subset. If this entry is absent, the only indication of a font subset is the
			// 					subset tag in the FontName entry (see Section 5.5.3, “Font Subsets”).

			fprintf(f,">>\n");


			if (ft_face) {
				FT_Done_Face(ft_face);
				ft_face=NULL;
			}
		}
	} //end creating new font dict



	 //append text object to stream
	sprintf(scratch,"%.10g %.10g %.10g rg\n",  //set fill color
				caption->red, caption->green, caption->blue);
	appendstr(stream,scratch);

	appendstr(stream, "BT\n");
	sprintf(scratch, " /font%ld %.10g Tf\n", caption->font->object_id, caption->fontsize);
	appendstr(stream, scratch);
	sprintf(scratch, " 1 0 0 -1 0 %.10g Tm\n", -caption->fontsize);
	appendstr(stream, scratch);
	
	int i1,i2;
	for (int c=0; c<caption->lines.n; c++) {
		sprintf(scratch, "%.10g %.10g Td\n", 0., -(c==0 ? 2 : 1)*caption->fontsize*caption->linespacing);
		appendstr(stream, scratch);

		appendstr(stream, "(");
		 //add a backslash to '(' and ')'
		i1=i2=0;
		while (caption->lines.e[c][i1]!='\0') {
			if (caption->lines.e[c][i1]=='(') scratch[i2++]='\\';
			else if (caption->lines.e[c][i1]==')') scratch[i2++]='\\';

			scratch[i2++]=caption->lines.e[c][i1++];
			if (i2>95 || caption->lines.e[c][i1]=='\0') {
				scratch[i2]='\0';
				appendstr(stream, scratch);
				i2=0;
			} 
		}
		//appendstr(stream, caption->lines.e[c]);
		appendstr(stream, ") Tj\n");
	}
	appendstr(stream, "ET\n");


	 //Add font to resources
	Attribute *fonts=resources.find("/Font");
	sprintf(scratch,"/font%ld %d 0 R\n",caption->font->object_id, fontdict);
	if (fonts) {
		if (!strstr(fonts->value, scratch)) appendstr(fonts->value,scratch);
	} else {
		resources.push("/Font",scratch);
	}
}


//--------------------------------------- pdfGradient() ----------------------------------------

//! Output pdf for a GradientData. 
static void pdfGradient(FILE *f,
					 	PdfObjInfo *objs, 
						PdfObjInfo *&obj,
						char *&stream,
						int &objectcount,
						Attribute &resources,
						LaxInterfaces::GradientData *g,
						ErrorLog &log,int &warning, DocumentExportConfig *config)
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


	 //stitching function object
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


//--------------------------------------- pdfPaths() ----------------------------------------

static int pdfaddpath(FILE *f,Coordinate *path, char *&stream);
static int pdfaddpath(FILE *f, flatpoint *points,int n, char *&stream);
static void pdfLineStyle(LineStyle *lstyle, char *&stream);



//! Output pdf for a PathsData. 
static void pdfPaths(FILE *f,
					 PdfObjInfo *objs, 
					 PdfObjInfo *&obj,
					 char *&stream,
					 int &objectcount,
					 Attribute &resources,
					 LaxInterfaces::PathsData *pdata,
					 ErrorLog &log,int &warning, DocumentExportConfig *config)
{
	if (!pdata) return;

	flatpoint pp;
	char buffer[255];

	if (pdata->paths.n==0) return; //ignore empty path objects

	LineStyle *lstyle=pdata->linestyle;
	if (lstyle && lstyle->hasStroke()==0) lstyle=NULL;
	FillStyle *fstyle=pdata->fillstyle;
	if (fstyle && fstyle->hasFill()==0) fstyle=NULL;
	if (!lstyle && !fstyle) return;


	int weighted=0;
	bool open=true;
	for (int c=0; c<pdata->paths.n; c++) {
		if (!pdata->paths.e[c]->path) continue;
		if (pdata->paths.e[c]->Weighted()) weighted++;
		if (pdata->paths.e[c]->IsClosed()) open=false;
	}

	if (!weighted) {
		 //plain, ordinary path with no offset and constant width

		Path *path;
		for (int c=0; c<pdata->paths.n; c++) {
			path=pdata->paths.e[c];
			if (!path->path) continue;

			pdfaddpath(f,path->path,stream);
		}

		pdfLineStyle(lstyle, stream);

		 //fill and/or stroke
		if (fstyle && fstyle->hasFill() && lstyle && lstyle->hasStroke()) {
			sprintf(buffer,"%.10g %.10g %.10g rg\n",  //set fill color
						fstyle->color.red/65535.,fstyle->color.green/65535.,fstyle->color.blue/65535.);
			appendstr(stream,buffer);

			if (fstyle->fillrule==LAXFILL_EvenOdd) appendstr(stream,"B*\n"); //fill and stroke
			else appendstr(stream,"B\n"); //fill and stroke


		} else if (fstyle && fstyle->hasFill()) {
			sprintf(buffer,"%.10g %.10g %.10g rg\n",  //set fill color
						fstyle->color.red/65535.,fstyle->color.green/65535.,fstyle->color.blue/65535.);
			appendstr(stream,buffer);

			if (fstyle->fillrule==LAXFILL_EvenOdd) appendstr(stream,"f*\n"); //fill only
			else appendstr(stream,"f\n"); //fill only

		} else if (lstyle && lstyle->hasStroke()) {
			appendstr(stream,"S\n"); //stroke only
		}


	} else {
		//at least one weighted path, need to fill separately from stroke, as stroke may
		//be all over the place


		if (fstyle && fstyle->hasFill() && !open) {
			 //---write style for fill within centercache.. no stroke to that, as we apply artificial stroke
			sprintf(buffer,"%.10g %.10g %.10g rg\n",  //set fill color
						fstyle->color.red/65535.,fstyle->color.green/65535.,fstyle->color.blue/65535.);
			appendstr(stream,buffer);

			Path *path;
			for (int c=0; c<pdata->paths.n; c++) {
				path=pdata->paths.e[c];
				if (path->needtorecache) path->UpdateCache();
				if (!path->path) continue;

				if (path->Weighted()) pdfaddpath(f,path->centercache.e,path->centercache.n, stream);
				else pdfaddpath(f,path->path, stream);
			}

			if (fstyle->fillrule==LAXFILL_EvenOdd) appendstr(stream,"f*\n"); //fill only
			else appendstr(stream,"f\n"); //fill only
		}

		if (lstyle) {
			pdfLineStyle(lstyle, stream);

			 //---write style: no linestyle, but fill style is based on linestyle
			FillStyle fillstyle;
			fillstyle.color=lstyle->color;

			sprintf(buffer,"%.10g %.10g %.10g rg\n",  //set fill color
						fillstyle.color.red/65535.,fillstyle.color.green/65535.,fillstyle.color.blue/65535.);
			appendstr(stream,buffer);


			 //add outlinecache...
			for (int c=0; c<pdata->paths.n; c++) {
				Path *path=pdata->paths.e[c];
				if (!path->path) continue;
				if (path->needtorecache) path->UpdateCache();

				pdfaddpath(f,path->outlinecache.e,path->outlinecache.n, stream);
			}

			appendstr(stream,"f*\n"); //evenodd fill only

		}
	} //end if weighted path
}

static void pdfLineStyle(LineStyle *lstyle, char *&stream)
{
	if (!lstyle) return;

	 //linecap
	if (lstyle->capstyle==CapButt) appendstr(stream,"0 J\n");
	else if (lstyle->capstyle==CapRound) appendstr(stream,"1 J\n");
	else if (lstyle->capstyle==CapProjecting) appendstr(stream,"2 J\n");

	 //linejoin
	if (lstyle->joinstyle==JoinMiter) appendstr(stream,"0 j\n");
	else if (lstyle->joinstyle==JoinRound) appendstr(stream,"1 j\n");
	else if (lstyle->joinstyle==JoinBevel) appendstr(stream,"2 j\n");

	//setmiterlimit
	//setstrokeadjust

	 //line width
	char buffer[255]; 
	sprintf(buffer," %.10g w\n",lstyle->width);
	appendstr(stream,buffer);

	 //dash pattern
	if (lstyle->dotdash==0 || lstyle->dotdash==~0)
		appendstr(stream," [] 0 d\n"); //clear dash array
	else {
		sprintf(buffer," [%.10g %.10g] 0 d\n",lstyle->width,2*lstyle->width); //set dash array
		appendstr(stream,buffer);
	}

	 //set stroke color
	sprintf(buffer,"%.10g %.10g %.10g RG\n",
				lstyle->color.red/65535.,lstyle->color.green/65535.,lstyle->color.blue/65535.);
	appendstr(stream,buffer); 
}


static int pdfaddpath(FILE *f,Coordinate *path, char *&stream)
{
	Coordinate *p,*p2,*start;
	p=start=path->firstPoint(1);
	if (!p) return 0;

	 //build the path to draw
	flatpoint c1,c2;
	int n=1; //number of points seen
	char buffer[255];

	sprintf(buffer,"%.10f %.10f m ",start->p().x,start->p().y);
	appendstr(stream,buffer);
	do { //one loop per vertex point
		p2=p->next; //p points to a vertex
		if (!p2) break;

		n++;

		//p2 now points to first Coordinate after the first vertex
		if (p2->flags&(POINT_TOPREV|POINT_TONEXT)) {
			 //we do have control points
			if (p2->flags&POINT_TOPREV) {
				c1=p2->p();
				p2=p2->next;
			} else c1=p->p();
			if (!p2) break;

			if (p2->flags&POINT_TONEXT) {
				c2=p2->p();
				p2=p2->next;
			} else { //otherwise, should be a vertex
				//p2=p2->next;
				c2=p2->p();
			}

			sprintf(buffer,"%.10f %.10f %.10f %.10f %.10f %.10f c\n",
					c1.x,c1.y,
					c2.x,c2.y,
					p2->p().x,p2->p().y);
			appendstr(stream,buffer);
		} else {
			 //we do not have control points, so is just a straight line segment
			sprintf(buffer,"%.10f %.10f l\n", p2->p().x,p2->p().y);
			appendstr(stream,buffer);
		}
		p=p2;
	} while (p && p->next && p!=start);
	if (p==start) appendstr(stream,"h ");

	return n;
}

static int pdfaddpath(FILE *f, flatpoint *points,int n, char *&stream)
{
	if (n<=0) return 0;

	 //build the path to draw
	char buffer[255];
	flatpoint c1,c2,p2;
	int np=1; //number of points seen
	bool onfirst=true;
	int ii=0;
	int ifirst=0;

	for (int i=ii; i<n; i++) {
		 //one loop per vertex point
		np++;

		if (onfirst) {
			onfirst=false;

			ifirst=i;
			while (i<n && (points[i].info&LINE_Bez)!=0 && (points[i].info&LINE_Vertex)==0) i++;
			sprintf(buffer,"%.10f %.10f m ",points[i].x,points[i].y);
			appendstr(stream,buffer);
			i++;
		}

		//i now points to first Coordinate after the first vertex
		if (points[i].info&LINE_Bez) {
			 //we do have control points
			 //by convention, there MUST be 2 cubic bezier controls
			c1=points[i];
			ii=-1;
			if (points[i].info&LINE_Open) ii=-2;
			else if (points[i].info&LINE_Closed) {
				ii=i;
				i=ifirst;
			} else i++;

			if (ii!=-2) {
				c2=points[i];

				if (points[i].info&LINE_Open) ii=-2;
				else if (points[i].info&LINE_Closed) {
					ii=i;
					i=ifirst;
				} else i++;

				if (ii!=-2) {
					p2=points[i];

					if (ii>=0) i=ii;

					sprintf(buffer,"%.10f %.10f %.10f %.10f %.10f %.10f c\n",
							c1.x,c1.y,
							c2.x,c2.y,
							p2.x,p2.y);
					appendstr(stream,buffer);
				}
			}
		} else {
			 //we do not have control points, so is just a straight line segment
			sprintf(buffer,"%.10f %.10f l\n", points[i].x,points[i].y);
			appendstr(stream,buffer);
			//i++;
		}

		if (points[i].info&LINE_Closed) {
			appendstr(stream,"h ");
			onfirst=true;
		} else if (points[i].info&LINE_Open) {
			onfirst=true;
		} 
	}

	return np;
}


} // namespace Laidout

