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


#include <cups/cups.h>
#include <sys/wait.h>

#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>

#include "../language.h"
#include "../laidout.h"
#include "../printing/psout.h"
#include "image.h"
#include "../impositions/impositioninst.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



//--------------------------------- install Image filter

//! Tells the Laidout application that there's a new filter in town.
void installImageFilter()
{
	ImageExportFilter *imageout=new ImageExportFilter;
	laidout->exportfilters.push(imageout);
}


//------------------------------------ ImageExportFilter ----------------------------------
	
/*! \class ImageExportFilter
 * \brief Filter for exporting pages to images via ghostscript.
 *
 * Right now, just exports through pngalpha with ghostscript.
 */


ImageExportFilter::ImageExportFilter()
{
	flags=FILTER_MANY_FILES;
}

/*! \todo if other image formats get implemented, then this would return
 *    the proper extension for that image type
 */
const char *ImageExportFilter::DefaultExtension()
{
	return "png";
}

const char *ImageExportFilter::VersionName()
{
	return _("Image");
}



////--------------------------------*************
//class ImageFilterConfig
//{
// public:
//	char *imagetype;
//	int use_transparency;
//	StyleDef *OutputStyleDef(); //for auto config dialog creation
//	StyleDef *InputStyleDef();
//};
////--------------------------------


//! Save the document as png files with transparency.
/*! This currently uses the postscript filter to make a temporary postscript file,
 * then uses ghostscript to translate that to images.
 *
 * Return 0 for success, 1 for error and nothing written, 2 for error, and corrupted file possibly written.
 * 2 is mainly for debugging purposes, and will be perhaps be removed in the future.
 * 
 * \todo must figure out if gs can directly process pdf 1.4 with transparency. If it can, then
 *   when full transparency is implemented, that will be the preferred method, that is until
 *   laidout buffer rendering is capable enough in its own right.
 */
int ImageExportFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	//int start     =out->start;
	//int end       =out->end;
	//int layout    =out->layout;
	if (!filename) filename=out->tofiles;
	if (!filename) filename="output#.eps";
	
	if (!doc->docstyle || !doc->docstyle->imposition 
			|| !doc->docstyle->imposition->paper->paperstyle) return 1;
	
	const char *gspath=laidout->binary("gs");
	if (!gspath) {
		if (error_ret) *error_ret=newstr(_("Currently need Ghostscript to output to image files."));
		return 1;
	} 

	char *filetemplate=NULL;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			DBG cerr <<"**** cannot save, doc->saveas is null."<<endl;
			*error_ret=newstr(_("Cannot save without a filename."));
			return 2;
		}
		filetemplate=newstr(doc->saveas);
		appendstr(filetemplate,"%d.png");
	}
	
	 //write out the document to a temporary postscript file
	char tmp[256];
	cupsTempFile2(tmp,sizeof(tmp));
	DBG cerr <<"attempting to write temp file for image out: "<<tmp<<endl;
	FILE *f=fopen(tmp,"w");
	if (!f) {
		if (error_ret) *error_ret=newstr(_("Error exporting to image."));
		return 1;
	}
	fclose(f);
	if (psout(tmp,context,error_ret)) {
		if (error_ret) appendstr(*error_ret,_("Export to image failed."));
		return 1;
	}


	 //---------run a gs shell command
	
	 //figure out decent preview size. If maxw and/or maxh are greater than 0, then
	 //generate preview via:
 	 //* gs -dNOPAUSE -sDEVICE=pngalpha -sOutputFile=temp%02d.png
	 //     -dBATCH -r(resolution) whatever.ps
	//cout <<"*** fix export image dpi!!"<<endl;
	double dpi;
	//dpi=maxw ? maxw*72./epsw : 200;
	//t  =maxh*72./epsh;
	//if (maxh && t && t<dpi) dpi=t;
	//dpi=150;//***
	dpi=doc->docstyle->imposition->paper->paperstyle->dpi;
	
	char *arglist[10], str1[20];
	arglist[0]=const_cast<char *>(gspath);
	arglist[1]="-dNOPAUSE";
	arglist[2]="-dBATCH";
	arglist[3]="-dEPSCrop";
	arglist[4]="-sDEVICE=pngalpha";

	sprintf(str1,"-r%f",dpi);
	arglist[5]=str1;

	if (!filetemplate) filetemplate=LaxFiles::make_filename_base(filename);
	prependstr(filetemplate,"-sOutputFile=");
	arglist[6]=filetemplate;

	arglist[7]=tmp; //input file
	arglist[8]=NULL;
		 
	DBG cerr <<"Exporting images via:    "<<gspath<<' ';
	DBG for (int c=1; c<8; c++) cerr <<arglist[c]<<' ';
	DBG cerr <<endl;
	
	char *error=NULL;
	pid_t child=fork();
	if (child==0) { // is child
		execv(gspath,arglist);
		cout <<"*** error in exec!"<<endl;
		error=newstr("Error trying to run Ghostscript.");
		exit(1);
	} 
	int status;
	waitpid(child,&status,0);
	if (!WIFEXITED(status)) {
		DBG cerr <<"*** error in child process, not returned normally!"<<endl;
		error=newstr(_("Ghostscript interrupted from making image."));
	} else if (WEXITSTATUS(status)!=0) {
		DBG cerr <<"*** ghostscript returned error while trying to make preview"<<endl;
		error=newstr(_("Ghostscript had error while making image."));
	}

	delete[] filetemplate;
	unlink(tmp);

	if (error) {
		if (error_ret) appendline(*error_ret,error);
		delete[] error;
		return -3;
	}

	return 0;
	
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
