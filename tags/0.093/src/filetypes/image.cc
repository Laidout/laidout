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
#include "../stylemanager.h"
#include "../printing/psout.h"
#include "image.h"
#include "../impositions/singles.h"

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
	imageout->GetStyleDef();
	laidout->PushExportFilter(imageout);
}


//----------------------------- ImageExportConfig -----------------------------
/*! \class ImageExportConfig
 * \brief Holds extra config for image export.
 *
 * \todo currently no extra settings, but could be image type, and background color to use, or transparency...
 */
class ImageExportConfig : public DocumentExportConfig
{
 public:
	//char *imagetype;
	//int use_transparent_bkgd;
	ImageExportConfig();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//! Set the filter to the Image export filter stored in the laidout object.
ImageExportConfig::ImageExportConfig()
{
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Image")) {
			filter=laidout->exportfilters.e[c];
			break; 
		}
	}
}

void ImageExportConfig::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	DocumentExportConfig::dump_out(f,indent,what,context);
}

void ImageExportConfig::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	DocumentExportConfig::dump_in_atts(att,flag,context);
}


//------------------------------------ ImageExportFilter ----------------------------------
	
/*! \class ImageExportFilter
 * \brief Filter for exporting pages to images via ghostscript.
 *
 * Right now, just exports through pngalpha with ghostscript.
 *
 * Uses ImageExportConfig.
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


Style *newImageExportConfig(StyleDef*)
{
	return new ImageExportConfig;
}

/*! \todo *** this needs a going over for safety's sake!! track down ref counting
 */
int createImageExportConfig(ValueHash *context,ValueHash *parameters,Value **v_ret,ErrorLog &log)
{
	ImageExportConfig *d=new ImageExportConfig;

	ValueHash *pp=parameters;
	if (pp==NULL) pp=new ValueHash;

	Value *vv=NULL, *v=NULL;
	vv=new ObjectValue(d);
	pp->push("exportconfig",vv);

	int status=createExportConfig(context,pp,&v,log);
	if (status==0 && v && v->type()==VALUE_Object) d=dynamic_cast<ImageExportConfig *>(((ObjectValue *)v)->object);

	if (d) for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Image")) {
			d->filter=laidout->exportfilters.e[c];
			break;
		}
	}
	*v_ret=v;

	if (pp!=parameters) delete pp;

	return 0;
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 *
 * \todo implement background color. currently defaults to transparent
 */
StyleDef *ImageExportFilter::GetStyleDef()
{
	StyleDef *styledef;
	styledef=stylemanager.FindDef("ImageExportConfig");
	if (styledef) return styledef; 

	styledef=makeStyleDef();
	makestr(styledef->name,"ImageExportConfig");
	makestr(styledef->Name,_("Image Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a png."));
	styledef->newfunc=newImageExportConfig;
	styledef->stylefunc=createImageExportConfig;

	// *** push any other settings:
	// *** image format
	// *** background color

	stylemanager.AddStyleDef(styledef);
	styledef->dec_count();

	return styledef;
}




//! Save the document as png files with transparency.
/*! This currently uses the postscript filter to make a temporary postscript file,
 * then uses ghostscript to translate that to images.
 *
 * Return 0 for success, or nonzero error. Possible errors are error and nothing written,
 * and corrupted file possibly written.
 * 
 * Currently uses a DocumentExportConfig for context, but ultimately should use 
 * an ImageExportConfig.
 *
 * \todo must figure out if gs can directly process pdf 1.4 with transparency. If it can, then
 *   when full transparency is implemented, that will be the preferred method, that is until
 *   laidout buffer rendering is capable enough in its own right.
 * \todo output file names messed up when papergroup has more than one paper, plus starting number
 *   not accurate...
 */
int ImageExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	//ImageExportConfig *iout=dynamic_cast<ImageExportConfig *>(context);
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	Document *doc =out->doc;
	//int start     =out->start;
	//int end       =out->end;
	//int layout    =out->layout;
	if (!filename) filename=out->tofiles;
	if (!filename) filename="temp-output#.eps";
	
	 //we must have something to export...
	if (!doc && !out->limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}
	
	const char *gspath=laidout->binary("gs");
	if (!gspath) {
		log.AddMessage(_("Currently need Ghostscript to output to image files."),ERROR_Fail);
		return 2;
	} 

	char *filetemplate=NULL;
	if (!filename) {
		if (!doc || isblank(doc->saveas)) {
			DBG cerr <<"**** cannot save, doc->saveas is null."<<endl;
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 3;
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
		log.AddMessage(_("Error exporting to image."),ERROR_Fail);
		return 4;
	}
	fclose(f);
	if (psout(tmp,context,log)) {
		log.AddMessage(_("Error exporting to image."),ERROR_Fail);
		return 5;
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
	dpi=doc->imposition->paper->paperstyle->dpi;
	
	char const * arglist[10];
	char str1[20];
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
		execv(gspath,(char * const *)arglist);
		cout <<"*** error running exec:"<<endl;
		for (int c=0; c<8; c++) cout <<arglist[c]<<" ";
		cout <<endl;
		//error=newstr("Error trying to run Ghostscript.");//this has no effect
		exit(1);//exit child thread
	} 
	 //continuing in parent thread
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
		log.AddMessage(error,ERROR_Fail);
		delete[] error;
		return -3;
	}

	return 0;
	
}

