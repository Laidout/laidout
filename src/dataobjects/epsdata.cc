//
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
// Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/fileutils.h>
#include <lax/strmanip.h>
#include <lax/laximages.h>
#include "epsdata.h"
#include "../laidout.h"
#include "../printing/epsutils.h"
#include "../configured.h"
#include "../language.h"

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;


namespace Laidout {


//-------------------------------- EpsData ----------------------------------
/*! \class EpsData
 * \brief Class to hold, of all things, EPS files.
 *
 * This class can scan in an eps file, and attempt to extract the
 * preview image if present.
 */


EpsData::EpsData(const char *nfilename, const char *npreview, 
			  int maxpx, int maxpy, char delpreview)
	: ImageData(NULL)
{
	resources=title=creationdate=NULL;
	LoadImage(nfilename,npreview,maxpx, maxpy, delpreview);
}

EpsData::~EpsData()
{
	if (title) delete[] title;
	if (resources) delete[] resources;
	if (creationdate) delete[] creationdate;
}

/*! 
 * Default dump for an EpsData. The bounding box is saved, but beware that the actual
 * bounding box in the eps may have different values. This helps compensate
 * for broken images when source files are moved around: you still save the working dimensions.
 * On a dump in, if the image exists, then the dimensions at time of load are used,
 * rather than what was saved in the file.
 *
 * Dumps:
 * <pre>
 *  minx 34
 *  miny 50
 *  maxx 100
 *  maxy 200
 *  filename filename.eps
 *  previewfile .filename-preview.png
 *  description "Blah blah blah"
 * </pre>
 * If previewfile is not an absolute path, then it is relative to filename.
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped. This makes it very easy
 * for programs to keep track of their file formats, that is, when the programmers remember to
 * update this code as change happens.
 * Otherwise dumps out in indented data format as above.
 */
void EpsData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sfilename /path/to/file\n",spc);
		fprintf(f,"%spreviewfile /path/to/preview/file  #if not absolute, is relative to filename\n",spc);
		fprintf(f,"%sminx 50  #the bounding box, which should be same as the %%%%BoundingBox comment in the EPS\n",spc);
		fprintf(f,"%sminy 50  \n",spc);
		fprintf(f,"%smaxx 100  \n",spc);
		fprintf(f,"%smaxy 200  \n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #affine transform to apply to the eps\n",spc);
		fprintf(f,"%sdescription \"Blah blah\" #a description of the eps \n",spc);
		return;
	}
	
	if (filename) fprintf(f,"%sfilename \"%s\"\n",spc,filename);
	if (previewfile && !(previewflag&1)) fprintf(f,"%spreviewfile \"%s\"\n",spc,previewfile);
	fprintf(f,"%sminx %.10g\n",spc,minx);
 	fprintf(f,"%sminy %.10g\n",spc,miny);
	fprintf(f,"%smaxx %.10g\n",spc,maxx);
	fprintf(f,"%smaxy %.10g\n",spc,maxy);
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m(0),m(1),m(2),m(3),m(4),m(5));
	if (description) {
		fprintf(f,"%sdescription",spc);
		dump_out_value(f,indent+2,description);
	}
}

/*! When the image listed in the attribute cannot be loaded,
 * image is set to NULL, and the width and height attributes
 * are used if present. If the image can be loaded, then width and
 * height as given in the file are curretly ignored, and the actual pixel 
 * width and height of the image are used instead.
 */
void EpsData::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *name,*value;
	minx=miny=0;
	maxx=maxy=-1;
	char *fname=NULL,*pname=NULL;
	double x1,x2,y1,y2;
	x1=x2=y1=y2=0;
	previewflag=(previewflag&~1);
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			double mm[6];
			if (DoubleListAttribute(value,mm,6)==6) m(mm);
		} else if (!strcmp(name,"description")) {
			makestr(description,value);
		} else if (!strcmp(name,"filename")) {
			fname=value;
		} else if (!strcmp(name,"previewfile")) {
			pname=value;
		} else if (!strcmp(name,"minx")) {
			DoubleAttribute(value,&x1);
		} else if (!strcmp(name,"miny")) {
			DoubleAttribute(value,&y1);
		} else if (!strcmp(name,"maxx")) {
			DoubleAttribute(value,&x2);
		} else if (!strcmp(name,"maxy")) {
			DoubleAttribute(value,&y2);
		}
	}
	 // if filename is given, and old file is NULL, or is different...
	if (fname && (!filename || (filename && strcmp(fname,filename)))) {
		//if (pname) previewflag|=1;
		 // load an image with existing preview, do not destroy that preview when
		 // image is destroyed:
		if (LoadImage(fname,pname,0,0,0)) {
			 // error loading image, so use the above w,h
			minx=x1;
			miny=y1;
			maxx=x2;
			maxy=y2;
		}
	}
}

/*! Return 0 for success, nonzero error.
 *
 * Import the file by opening, then using scaninEPS() to read in the relevant information.
 *
 * Also set up the preview if any. If the file is EPSI, then the preview present in that is
 * read in. Otherwise, if there is known Ghostscript executable, then Laidout uses that to
 * generate a preview png with transparency.
 * 
 * \todo in general must figure out a decent way to deal with errors while loading images,
 *    for instance.
 * \todo del is ignored
 */
int EpsData::LoadImage(const char *fname, const char *npreview, int maxpw, int maxph, char del)
{
	FILE *f = fopen(fname,"r");
	if (!f) return -1;
	
	char *preview=NULL;
	int c,depth,width,height;

	makestr(filename,fname);
	makestr(previewfile,npreview);

	 // puts the eps BoundingBox into this::DoubleBBox
	setlocale(LC_ALL,"C");
	c = scaninEPS(f,this,&title,&creationdate,&preview,&depth,&width,&height);
	fclose(f);
	setlocale(LC_ALL,"");
	
	if (c != 0) return -2;
	
	 // set up the preview if any
	if (image) { image->dec_count(); image=NULL; } //clear main image in any

	LaxImage *eimage = NULL;
	if (file_exists(npreview,1,NULL)) {
		// do nothing if preview file already exists..
		//*** perhaps optionally regenerate?

	} else if (laidout->prefs.FindExternalTool("Misc:gs") != nullptr) {
		 // call ghostscript from command line, save preview ***where?
		char *error = nullptr;
		
		ExternalTool *gs_tool = laidout->prefs.FindExternalTool("Misc:gs");
		if (!gs_tool->Valid()) {
			makestr(error, _("External tool Misc:gs needs to be configured to a ghostscript executable"));
			c = 1;

		} else {
			c = WriteEpsPreviewAsPng(gs_tool->binary_path,
							 fname, width, height,
							 npreview, maxpw, maxph,
							 &error);
			DBG if (error) cerr <<"EPS gs preview generation returned with error: "<<error<<endl;
		}
		if (c) {
			if (error) delete[] error;
			return -3;
		}

	} else if (preview) {
		 // install preview image from EPSI data
		eimage = ImageLoader::NewImage(width,height);
		unsigned char *data = eimage->getImageBuffer();
		EpsPreviewToARGB((unsigned char *)data, preview, width,height, depth);
		eimage->doneWithBuffer(data);
	}

	 // now set this->image to have the generated preview as the main image
	image = eimage;
	
	return 0;
}



} //namespace Laidout

