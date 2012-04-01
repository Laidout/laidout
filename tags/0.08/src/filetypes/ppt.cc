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
// Copyright (C) 2004-2007 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"
#include "../utils.h"
#include "../printing/psout.h"
#include "ppt.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


//--------------------------------- install Passepartout filter

//! Tells the Laidout application that there's a new filter in town.
void installPptFilter()
{
	PptoutFilter *pptout=new PptoutFilter;
	laidout->exportfilters.push(pptout);
	
	//PptinFilter *pptin=new PptinFilter;
	//laidout->importfilters.push(pptin);
}



//------------------------------- PptoutFilter --------------------------------------
/*! \class PptoutFilter
 * \brief Output filter for Passepartout files.
 */

PptoutFilter::PptoutFilter()
{
	flags=FILTER_MULTIPAGE;
}

const char *PptoutFilter::VersionName()
{
	return _("Passepartout");
}

//! Internal function to dump out the obj if it is an ImageData.
/*! \todo deal with SomeDataRef
 */
static void pptdumpobj(FILE *f,double *mm,SomeData *obj,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (!strcmp(obj->whattype(),"Group")) {
		Group *g;
		g=dynamic_cast<Group *>(obj);
		if (!g || !g->n()) return;

		double m[6];
		if (mm) transform_mult(m,g->m(),mm);
		else transform_copy(m,g->m());
		
		fprintf(f,"%s<frame type=\"group\" transform=\"%.10g %.10g %.10g %.10g %.10g %.10g\" >\n",
				spc, m[0], m[1], m[2], m[3], m[4], m[5]);
		transform_identity(m);
		for (int c=0; c<g->n(); c++) pptdumpobj(f,m,g->e(c),indent+2);
		fprintf(f,"%s</frame>\n",spc);
	} else if (!strcmp(obj->whattype(),"ImageData")) {
		ImageData *img;
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;

		double m[6];
		if (mm) transform_mult(m,img->m(),mm);
		else transform_copy(m,img->m());
		
		char *bname=basename(img->filename); // Warning! This assumes the GNU basename, which does
											 // not modify the string.
		fprintf(f,"%s<frame name=\"Raster %s\" matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
				spc, bname, m[0], m[1], m[2], m[3], m[4], m[5]);
		fprintf(f,"lock=\"false\" flowaround=\"false\" obstaclemargin=\"0\" type=\"raster\" file=\"%s\" />\n",
				img->filename);
	}
}

static const char *pptpaper[12]= {
		"A0",
		"A1",
		"A2",
		"A3",
		"A4",
		"A5",
		"A6",
		"Executive",
		"Legal",
		"Letter",
		"Tabloid/Ledger",
		NULL
	};

//! Save the document as a Passepartout file.
/*! This only saves groups and images, and the page size and orientation.
 *
 * If the paper name is not recognized as a Passepartout paper name, which are
 * A0-A6, Executive (7.25 x 10.5in), Legal, Letter, and Tabloid/Ledger, then
 * Letter is used.
 *
 * error_ret is appended to if possible.
 *    
 * \todo if unknown paper, should really use some default paper size, if it is valid, 
 *   and then otherwise "Letter", or choose a size that is big enough to hold the spreads
 * \todo for singles, should figure out what paper size to export as..
 */
int PptoutFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
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
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			if (error_ret) appendline(*error_ret,_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".ppt");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,error_ret);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}
	
	 //figure out paper size and orientation
	const char *papersize=NULL, *landscape=NULL;
	int c;
	const char **tmp;
	double paperwidth; //,paperheight;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	 // The ps Orientation comment determines how onscreen viewers will show 
	 // pages. This can be overridden by the %%PageOrientation: comment
	landscape=(papergroup->papers.e[0]->box->paperstyle->flags&1)?"true":"false";
	paperwidth=papergroup->papers.e[0]->box->paperstyle->width;
	 //match the first paper name with a passepartout paper name
	for (c=0, tmp=pptpaper; *tmp; c++, tmp++) {
		if (!strcmp(papergroup->papers.e[0]->box->paperstyle->name,*tmp)) break;
	}
	if (*tmp) papersize=*tmp;
	else if (!strcmp(papergroup->papers.e[0]->box->paperstyle->name,"Tabloid")) {
		papersize="Tabloid/Ledger";
		if (landscape[0]=='t') landscape="false"; else landscape="true";
	} else if (!strcmp(papergroup->papers.e[0]->box->paperstyle->name,"Ledger")) {
		papersize="Tabloid/Ledger";
	} else papersize="Letter";
	
	 // write out header
	fprintf(f,"<?xml version=\"1.0\"?>\n");
	fprintf(f,"<document paper_name=\"%s\" doublesided=\"false\" landscape=\"%s\" first_page_num=\"%d\">\n",
				papersize, landscape, start);
	
	 // Write out paper spreads....
	Spread *spread=NULL;
	Group *g=NULL;
	double m[6],mm[6],mmm[6];
	int p,c2,l,pg,c3;
	
	transform_set(mm,72,0,0,72,0,0);
	psCtmInit();
	for (c=start; c<=end; c++) {
		if (doc) spread=doc->docstyle->imposition->Layout(layout,c);
		for (p=0; p<papergroup->papers.n; p++) {
			fprintf(f,"  <page>\n");

			 //begin paper contents
			//fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
			//psConcat(72.,0.,0.,72.,0.,0.);
			//if (plandscape) {
			//	//fprintf(f,"%.10g 0 translate\n90 rotate\n",paperwidth);
			//	psConcat(0.,1.,-1.,0., paperwidth,0.);
			//}

			psPushCtm();
			transform_invert(mmm,papergroup->papers.e[p]->m());
			transform_mult(m,mmm,mm);
			fprintf(f,"  <frame type=\"group\" transform=\"%.10g %.10g %.10g %.10g %.10g %.10g\" >\n",
				      m[0], m[1], m[2], m[3], m[4], m[5]);
			psConcat(m);

			if (limbo && limbo->n()) {
				//*** if limbo bbox inside paper bbox? could loop in limbo objs for more specific check
				pptdumpobj(f,NULL,limbo,4);
			}

			if (spread) {
				// // print out printer marks
				//if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
				//	//fprintf(f," .01 setlinewidth\n");
				//	//DBG cerr <<"marks data:\n";
				//	//DBG spread->marks->dump_out(stderr,2,0);
				//	pptdumpobj(f,m,spread->marks,4);
				//}
				
				 // for each page in spread..
				for (c2=0; c2<spread->pagestack.n; c2++) {
					pg=spread->pagestack.e[c2]->index;
					if (pg>=doc->pages.n) continue;
					
					 // for each layer on the page..
					for (l=0; l<doc->pages[pg]->layers.n(); l++) {

						 // for each object in layer
						g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							transform_copy(mmm,spread->pagestack.e[c2]->outline->m());
							pptdumpobj(f,mmm,g->e(c3),4);
						}
					}
				}

			}
			psPopCtm(); //remove papergroup->paper transform
			fprintf(f,"  </frame>\n");
			fprintf(f,"  </page>\n");
		}
		if (spread) { delete spread; spread=NULL; }
	}
		
	 // write out footer
	fprintf(f,"</document>\n");
	
	fclose(f);
	delete[] file;
	return 0;
	
}

//////------------------------------------- PptinFilter -----------------------------------
///*! \class PptinFilter 
// * \brief Passepartout input filter.
// */
//
//
//const char *PptinFilter::FileType(const char *first100bytes)
//{
//	return !strncmp(first100bytes,"<?xml version="1.0"?>\n<document",
//						   strlen("<?xml version="1.0"?>\n<document"))
//}
//
//const char **PptinFilter::FormatVersions(int *n)
//{
//	return (const char **){ "0.6", NULL };
//}
//
//
////! Import a Passepartout file.
///*! If doc!=NULL, then import the pptout files to Document starting at page startpage.
// * Otherwise, create a brand new Singles based document.
// *
// * Does no check on the file to ensure that it is in fact a Passepartout file.
// *
// * It will be a file something like:
// * <pre>
// * <?xml version="1.0"?>
// * <document paper_name="Letter" doublesided="true" landscape="false" first_page_num="1">
// *   <page>
// *      <frame name="Raster beetile-501x538.jpg"
// *             matrix="0.812233 0 0 0.884649 98.6048 243.107"
// *             lock="false"
// *             flowaround="false"
// *             obstaclemargin="0" 
// *             type="raster"
// *             file="beetile-501x538.jpg"/>
// *   </page>
// * </document>         
// * </pre>
// *
// * \todo ***** finish imp me!
// * \todo there should be a way to preserve any elements that laidout doesn't understand, so
// *   when outputting as ppt, these elements would be written back out maybe...
// */
//int PptinFilter::In(const char *file, Laxkit::anObject *context, char **error_ret)
//{
//	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
//	if (!att) return NULL;
//	
//	int c;
//	Attribute *pptdoc=att->find("document"),
//			  *page, *frame, *a;
//	if (!pptdoc) { delete att; return NULL; }
//	
//	 //figure out the paper size, orientation
//	a=pptdoc->find("paper_name");
//	PaperStyle *paper=NULL;
//	if (a) {
//		for (c=0; c<laidout->papersizes.n; c++)
//			if (!strcasecmp(laidout->papersizes.e[c]->name,a->value)) {
//				paper=laidout->papersizes.e[c];
//				break;
//			}
//	}
//	if (!paper) paper=laidout->papersizes.e[0];
//
//	 //figure out orientation
//	int landscape;
//	a=pptdoc->find("landscape");
//	if (a) landscape=BooleanAttribute(a->value);
//	else landscape=0;
//	
//	 // read in pages
//	int pagenum=0;
//	pptdoc=pptdoc->find("contents");
//	if (!pptdoc) { delete att; return NULL; }
//
//	 //create the document
//	if (!doc) {
//		Imposition *imp=new Singles;
//		imp->SetPaperSize(paper);
//		imp->paperstyle->flags=((imp->paperstyle->flags)&~1)|(landscape?1:0);
//		DocumentStyle *docstyle=new DocumentStyle(imp);
//		doc=new Document(docstyle,"untitled");//**** laidout should keep track of: untitled1, untitled2, ...
//	}
//
//	ImageData *image;
//	LaxImage *img=NULL;
//	Attribute *t,*n,*m;
//	double M[6];
//	
//	for (c=0; c<pptdoc->attributes.n; c++) {
//		if (!strcmp(pptdoc->attributes.e[c]->name,"page")) {
//			if (pagenum>doc->pages.n) doc->NewPages(-1,1);
//			page=pptdoc->attributes.e[c];
//			for (int c2=0; c2<page->attributes.n; c2++) {
//				if (!strcmp(page->attributes.e[c]->name,"frame")) {
//					frame=page->attributes.e[c];
//					a=frame->find("file");
//					t=frame->find("type");
//					n=frame->find("name");
//					m=frame->find("matrix");
//					if (a && a->value && t) {
//						if (!strcmp(t->value,"raster")) {
//							img=load_image(a->value);
//							if (img) {
//								image=new ImageData;
//								if (n) image->SetDescription(n->value);
//								image->SetImage(img);
//								if (m) DoubleListAttribute(m->value,M,6,NULL);
//								dynamic_cast<Group *>(doc->pages.e[pagenum]->layers.e(0))->push(image,0);
//								image->dec_count();
//							}
//						} else if (!strcmp(t->value,"group")) {
//							***
//							pptDumpInGroup(page->attributes.e[c]->attributes***
//						}
//					}
//				}
//			}
//			pagenum++;
//		}
//	}
//	
//	 //*** set up page labels for "first_page_num"
//	
//	 //establish doc in project
//	laidout->project->docs.push(doc);
//	laidout->app->addwindow(newHeadWindow(doc));
//	
//	delete att;
//	return doc;
//}


