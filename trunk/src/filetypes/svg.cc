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
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "svg.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



//--------------------------------- install SVG filter

//! Tells the Laidout application that there's a new filter in town.
void installSvgFilter()
{
	SvgOutputFilter *svgout=new SvgOutputFilter;
	laidout->exportfilters.push(svgout);
	
	//SvgInputFilter *svgin=new SvgInputFilter;
	//laidout->importfilters(svgin);
}


//------------------------------------ SvgOutputFilter ----------------------------------
	
/*! \class SvgOutputFilter
 * \brief Filter for exporting SVG 1.0.
 */


const char *SvgOutputFilter::VersionName()
{
	return _("Svg 1.0");
}

////--------------------------------*************
//class SvgFilterConfig
//{
// public:
//	char untranslatables;   //whether laidout objects not suitable for svg should be ignored, rasterized, or approximated
//	char dont_clobber_file;
//	char plain;             //akin to inkscapes plain vs. inkscape svg?
//	char preserveunknown;   //any unknown attributes should be attached as "metadata" to the object in question on readin
//							//these would potentially be written back out on an svg export?
//							
//	StyleDef *OutputStyleDef(); //for auto config dialog creation
//	StyleDef *InputStyleDef();
//};
////--------------------------------


//! Save the document as SVG.
/*! This only saves images, groups, linear and radial gradients, and the page size and orientation.
 * Files are not checked for existence. They are clobbered if they already exist, and are writable.
 *
 * Return 0 for success, 1 for error and nothing written, 2 for error, and corrupted file possibly written.
 * 2 is mainly for debugging purposes, and will be perhaps be removed in the future.
 * 
 * \todo *** should have option of rasterizing or approximating the things not supported in svg, such 
 *    as patch gradients
 */
int SvgOutputFilter::Out(const char *filename, Laxkit::anObject *context, char *&error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(config);
	if (!outconfig) return 1;
	doc     =out->doc;
	start   =out->start;
	end     =out->end;
	layout  =out->layout;
	filename=out->filename;
	
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paperstyle) return 1;
	
	FILE *f=NULL;
	char *file;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			DBG cout <<"**** cannot save, doc->saveas is null."<<endl;
			error_ret=newstr(_("Cannot save to SVG without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".svg");
	} else file=newstr(filename);
	
	f=fopen(file,"w");
	delete[] file; file=NULL;

	if (!f) {
		DBG cout <<"**** cannot save, doc->saveas cannot be opened for writing."<<endl;
		error_ret=newstr(_("Error opening file for writing."));
		return 3;
	}

	int warning=0;
	Spread *spread;
	Group *g;
	double m[6];
	int c,c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);

	if (start<0) start=0;
	else if (start>doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	spread=doc->docstyle->imposition->Layout(layout,start);
	
	
	 // write out header
	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	fprintf(f,"<!-- Created with Laidout, http://www.laidout.org -->\n");
	fprintf(f,"<svg \n"
			  "     xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
			  "     xmlns=\"http://www.w3.org/2000/svg\"\n"
			  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
			  "     version=\"1.0\"\n");
	fprintf(f,"     width=\"%fin***inches by default?\"\n", spread->outline->maxx-spread->outline->minx);
	fprintf(f,"     height=\"%fin\"\n", spread->outline->maxy-spread->outline->miny);
	fprintf(f,"   >\n");
			
	 //write out global defs section
	fprintf(f,"  <defs>\n");
	//*************** gradients and such
	fprintf(f,"  </defs>\n");
			
	
	 // Write out spread....
	fprintf(f,"  <g>\n");

	 // for each page in spread..
	for (c2=0; c2<spread->pagestack.n; c2++) {
		pg=spread->pagestack.e[c2]->index;
		if (pg<0 || pg>=doc->pages.n) continue;
		 // for each layer on the page..
		for (l=0; l<doc->pages[pg]->layers.n(); l++) {
			 // for each object in layer
			g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
			for (c3=0; c3<g->n(); c3++) {
				transform_copy(m,spread->pagestack.e[c2]->outline->m());
				svgdumpobj(f,m,g->e(c3),error_ret,warning);
			}
		}
	}

	delete spread;
	fprintf(f,"  </g>\n");
//-------or----------adapt to multipagesvg with page and pagesets
//	 // Write out spread....
//	fprintf(f,"  <g>\n");
//	*********
//	for (c=0; c<doc->docstyle->imposition->numpapers; c++) {
//		fprintf(f,"  <page>\n");
//		spread=doc->docstyle->imposition->PaperLayout(c);
//		 // for each page in paper layout..
//		for (c2=0; c2<spread->pagestack.n; c2++) {
//			pg=spread->pagestack.e[c2]->index;
//			if (pg>=doc->pages.n) continue;
//			 // for each layer on the page..
//			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
//				 // for each object in layer
//				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
//				for (c3=0; c3<g->n(); c3++) {
//					transform_copy(m,spread->pagestack.e[c2]->outline->m());
//					svgdumpobj(f,m,g->e(c3));
//				}
//			}
//		}
//
//		delete spread;
//		fprintf(f,"  </page>\n");
//	}
//	fprintf(f,"  </g>\n");
//--------------------
		
	 // write out footer
	fprintf(f,"</svg>\n");
	
	fclose(f);
	return 0;
	
}

//! Function to dump out obj as svg.
/*! Return nonzero for fatal errors encountered, else 0.
 */
int svgdumpobj(FILE *f,double *mm,SomeData *obj,char &*error_ret,int &warning)
{
	if (!strcmp(obj->whattype(),"Group")) {
		***
		double m[6];
		fprintf(f,"    <g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" ",
				 m[0]*72, m[1]*72, m[2]*72, m[3]*72, m[4]*72, m[5]*72);
	} else if (!strcmp(obj->whattype(),"GradientData")) {
		***
	} else if (!strcmp(obj->whattype(),"EpsData")) {
		appendstr(error_ret,_("Cannot import Eps objects into svg.\n"));
		warning++;
		
	} else if (!strcmp(obj->whattype(),"ImageData")) {
		***
		ImageData *img;
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;

		double m[6];
		transform_mult(m,img->m(),mm);
		
		char *bname=basename(img->filename); // Warning! This assumes the GNU basename, which does
											 // not modify the string.
		fprintf(f,"    <frame name=\"Raster %s\" matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
				bname, m[0]*72, m[1]*72, m[2]*72, m[3]*72, m[4]*72, m[5]*72);
		fprintf(f,"lock=\"false\" flowaround=\"false\" obstaclemargin=\"0\" type=\"raster\" file=\"%s\" />\n",
				img->filename);
		
	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		appendstr(error_ret,_("Cannot import Color Patch objects into svg.\n"));
		warning++;
		
	} else if (!strcmp(obj->whattype(),"ImagePatchData")) {
		appendstr(error_ret,_("Cannot import Image Patch objects into svg.\n"));
		warning++;
	}
}


////------------------------------------ SvgInputFilter ----------------------------------
///*! \class SvgInputFileFilter
// * \brief Filter to, amazingly enough, import svg files.
// */
//
//class SvgInputFilter
//{
// public:
//	virtual ~FileFilter() {}
//	virtual const char *Author() = 0;
//	virtual const char *FilterVersion() = 0;
//	
//	virtual const char *DefaultExtension() { return "svg"; }
//	virtual const char *Format() = 0;
//	virtual const char **FormatVersions(int *n) = 0;
//	virtual const char *VersionName(const char *version) = 0;
//	virtual const char *FilterClass() = 0;
//
//	virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
//	
//	
//	virtual const char *FileType(const char *first100bytes) = 0;
//	virtual int In(const char *file, Laxkit::anObject *context) = 0;
//};
//
//
//
////-----------------------------------------------------------
//
////! Import an SVG file.
///*! If doc!=NULL, then import the svg file to Document starting at page startpage.
// * Otherwise, create a brand new Singles based document.
// *
// * Does no check on the file to ensure that it is in fact an svg file.
// *
// * It will be a file something like:
// * <pre>
// *   ??????
// * </pre>
// *
// * \todo ***** finish imp me!
// * \todo there should be a way to preserve any elements that laidout doesn't understand, so
// *   when outputting as svg, these elements would be written back out maybe...
// */
//Document *svgin(const char *file,Document *doc,int startpage,char **error_ret)
//{
//	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
//	if (!att) {
//		if (error_ret) error_ret=newstr(_("Could not open file for reading."));
//		return NULL;
//	}
//	
//	int c;
//	Attribute *svgdoc=att->find("svg"),
//			  *page, *frame, *a;
//	if (!svgdoc) {
//		delete att; 
//		if (error_ret) error_ret=newstr(_("Could not open file for reading."));
//		return NULL; 
//	}
//	
//	 //figure out the paper size, orientation
//	a=svgdoc->find("width");  //8.5in
//	a=svgdoc->find("height"); //11in...
//
//	 // create doc if not exist already with specified dimensions
//	a=svgdoc->find("paper_name");
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
//	
//	 //figure out orientation
//	int landscape;
//	a=svgdoc->find("landscape");
//	if (a) landscape=BooleanAttribute(a->value);
//	else landscape=0;
//	
//	
//	 // read in defs, which normally includes gradients and other globals...
//	a=svgdoc->find("defs");
//	
//
//	
//	 // read in pages
//	int pagenum=0;
//	svgdoc=pptdoc->find("contents");
//	if (!svgdoc) { delete att; return NULL; }
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
//	for (c=0; c<svgdoc->attributes.n; c++) {
//		if (!strcmp(svgdoc->attributes.e[c]->name,"page")) {
//			if (pagenum>doc->pages.n) doc->NewPages(-1,1);
//			page=svgdoc->attributes.e[c];
//			for (int c2=0; c2<page->attributes.n; c2++) {
//				if (!strcmp(page->attributes.e[c]->name,"frame")) {
//					frame=page->attributes.e[c];
//					a=frame->find("file");
//					t=frame->find("type");
//					n=frame->find("name");
//					m=frame->find("matrix");
//					if (a && a->value && t && !strcmp(t->value,"raster")) {
//						img=load_image(a->value);
//						if (img) {
//							image=new ImageData;
//							if (n) image->SetDescription(n->value);
//							image->SetImage(img);
//							if (m) DoubleListAttribute(m->value,M,6,NULL);
//							dynamic_cast<Group *>(doc->pages.e[pagenum]->layers.e(0))->push(image,0);
//							image->dec_count();
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
