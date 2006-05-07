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

#include "epsdata.h"



//-------------------------------- EpsData ----------------------------------
/*! \class EpsData
 * \brief Class to hold, of all things, EPS files.
 *
 * This class can scan in an eps file, and attempt to extract the
 * preview image if present.
 *
 * *** this could be built on ImageData/ImageInterface
 */
class EpsData : public LaxInterfaces::ImageData
{
 public:
	char *filename, *title, *creationdate;
	Imlib_Image preview;
	virtual ~EpsData();
	virtual int SetFile(const char *file);
};

EpsData::~EpsData()
{
	if (filename) delete[] filename;
	if (title) delete[] title;
	if (creationdate) delete[] creationdate;
}

int EpsData::SetFile(const char *file)
{
	*** import the file...
	*** and set up the preview if any
}

//! Get the bounding box, and the preview, title and creation date (if present).
/*! \todo In future, must also be able to extract any resources that must go
 * at the top of an including ps file.
 */
int scaninEPS(FILE *f, Laxkit::DoubleBBox *bbox, char **title, char **date, 
		char **preview, int *depth, int *width, int *height)
{
	if (!f) return -1;
	
	char buf[1024];
	int c,off=0;
	int languagelevel;
	char *line;
	DoubleBBox bbox;
	
	while (1) {
		c=fread(buf+off,sizeof(char),1024-off,f); // c==0 is either error or eof
		if (!c) break;

		if (strncmp(buf,"%!PS-Adobe-3.0 EPSF-3.0") && strncmp(buf,"%!PS-Adobe-2.0 EPSF-2.0")) {
			*** not a readable eps
		}
		
		getaline(buf,1024,&line,f);
			
		while (line) {
			if (!strncmp(line,"%%BoundingBox:",14)) {
			} else if (!strncmp(line,"%%Title:",8)) {
			} else if (!strncmp(line,"%%CreationDate:",15)) {
			} else if (!strncmp(line,"%%BeginPreview:",15)) {
				p=splitonspace(line+15,&n);
				if (n<3) *** error!!;
				
				width=strtol(p[0],NULL,10);
				height=strtol(p[1],NULL,10);
				depth=strtol(p[2],NULL,10);
				*preview=new char[(width*depth/8+1)*height];
				***
			} else if (!strncmp(line,"%%Page:",7)) break;
			
			if (!getaline(buf,1024,&line,f)) break;
		}
	}
}

//-------------------------------- EpsInterface ----------------------------------
/*! \class EpsInterface
 * \brief Interface to manipulate placement of eps files.
 *
 * If there is an epsi style preview in the eps, then that is what is put
 * on screen. Otherwise, the title/file/date are put on.
 */
class EpsInterface : public LaxInterfaces::ImageInterface
{
};
