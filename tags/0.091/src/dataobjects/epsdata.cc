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
// Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/fileutils.h>
#include <lax/strmanip.h>
#include <lax/laximages-imlib.h>
#include "epsdata.h"
#include "../printing/epsutils.h"
#include "../configured.h"

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxFiles;
using namespace Laxkit;

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

//! Return new EpsInterface.
/*! If dup!=NULL and it cannot be cast to EpsInterface, then return NULL.
 */
LaxInterfaces::anInterface *EpsInterface::duplicate(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=new EpsInterface(id,NULL);
	else if (!dynamic_cast<EpsInterface *>(dup)) return NULL;
	return ImageInterface::duplicate(dup);
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
void EpsData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
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
				matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
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
void EpsData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
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
	if (fname && (!filename || (filename && strcmp(fname,filename)))) 
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
 * \todo ***rework so as not to depend on imlib
 */
int EpsData::LoadImage(const char *fname, const char *npreview, int maxpw, int maxph, char del)
{
	FILE *f=fopen(fname,"r");
	if (!f) return -1;
	
	char *preview=NULL;
	int c,depth,width,height;

	makestr(filename,fname);
	makestr(previewfile,npreview);

	 // puts the eps BoundingBox into this::DoubleBBox
	setlocale(LC_ALL,"C");
	c=scaninEPS(f,this,&title,&creationdate,&preview,&depth,&width,&height);
	fclose(f);
	setlocale(LC_ALL,"");
	
	if (c!=0) return -2;
	
	 // set up the preview if any
	if (image) { image->dec_count(); image=NULL; }
	
	Imlib_Image imlibimage=NULL;
	if (file_exists(npreview,1,NULL)) {
		// do nothing if preview file already exists..
		//*** perhaps optionally regenerate?
	} else if (strcmp("",GHOSTSCRIPT_BIN)) {
		 // call ghostscript from command line, save preview ***where?
		char *error=NULL;
		
		c=WriteEpsPreviewAsPng(GHOSTSCRIPT_BIN,
						 fname, width, height,
						 npreview, maxpw, maxph,
						 &error);
		DBG if (error) cerr <<"EPS gs preview generation returned with error: "<<error<<endl;
		if (c) {
			if (error) delete[] error;
			return -3;
		}
	} else if (preview) {
		 // install preview image from EPSI data
		imlibimage=imlib_create_image(width,height);
		DATA32 *data=imlib_image_get_data();
		EpsPreviewToARGB((unsigned char *)data,preview,width,height,depth);
		imlib_image_put_back_data(data);
		//if (imlibimage) { use this as preview image for eps, set up below.... }
	}

	 // now set this->image to have the generated preview as the main image
	image=new LaxImlibImage(npreview,imlibimage);
	
	return 0;
}


//-------------------------------- EpsInterface ----------------------------------
/*! \class EpsInterface
 * \brief Interface to manipulate placement of eps files.
 *
 * If there is an epsi style preview in the eps, then that is what is put
 * on screen. Otherwise, the title/file/date are put on.
 */


EpsInterface::EpsInterface(int nid,Laxkit::Displayer *ndp)
	: ImageInterface(nid,ndp)
{
}

//! Return whether this interface can draw the given type of object.
/*! \todo should redo this to be more easily expandable for other
 *    image types. Each spunky new image type might have many idiosyncracies (like EPS),
 *    so each added image type would need to define:
 *     an import filter, returning an ImageData pointer,
 *     output functions for various types: bitmap, ps, etc.,
 *     Refresh() extras,
 *     additional controls if any including extra StyleDef elements.
 *   could have an interface shell, whose purpose is to have one tool icon, but
 *   several sub tools that it dispatches events to...
 *   
 */
int EpsInterface::draws(const char *what)
{
	//if (!strcmp(what,"ImageData") || !strcmp("EpsData")) return 1;
	if (!strcmp(what,"EpsData")) return 1;
	return 0;		
}

//! Redefine ImageInterface::newData() to never create a random new eps. These can only be imported.
LaxInterfaces::ImageData *EpsInterface::newData()
{
	return NULL;
}

//! Draw title or filename if no preview data.
int EpsInterface::Refresh()
{
	int c=ImageInterface::Refresh();
	if (c!=0) return c;

	 // draw title or filename...
	if (!data->image && data->filename) {
		flatpoint fp=dp->realtoscreen(flatpoint((data->maxx+data->minx)/2,(data->maxy+data->miny)));
		dp->textout((int)fp.x,(int)fp.y,data->filename,0);
	}
	return 0;
}

