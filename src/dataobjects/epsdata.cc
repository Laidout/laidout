//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include <lax/fileutils.h>
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
 */
int EpsData::LoadImage(const char *fname, const char *npreview, int maxpw, int maxph, char del)
{
	FILE *f=fopen(fname,"r");
	if (!f) return -1;
	
	char *preview=NULL;
	int c,depth,width,height;

	 // puts the eps BoundingBox into this::DoubleBBox
	c=scaninEPS(f,this,&title,&creationdate,&preview,&depth,&width,&height);
	fclose(f);
	
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
		DBG if (error) cout <<"EPS gs preview generation returned with error: "<<error<<endl;
		if (c) {
			if (error) delete[] error;
			return -3;
		}
	} else if (preview) {
		 // install preview image from EPSI data
		imlibimage=EpsPreviewToImlib(preview,width,height,depth);
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

