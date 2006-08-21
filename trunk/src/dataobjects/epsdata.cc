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
 */
//class EpsData : public LaxInterfaces::ImageData
//{
// public:
//	char *title, *creationdate;
//	EpsData();
//	virtual ~EpsData();
//	virtual int SetFile(const char *file);
//};

EpsData::EpsData()
	: ImageData(NULL);
{
	title=creationdate=NULL;
}

EpsData::~EpsData()
{
	if (title) delete[] title;
	if (creationdate) delete[] creationdate;
}

/*! Return 0 for success, nonzero error.
 *
 * import the file...
 * and set up the preview if any
 */
int EpsData::SetFile(const char *file)
{
	FILE *f=fopen(file,"r");
	if (!f) return -1;
	DoubleBBox bbox;
	char *preview=NULL;
	int c,d,w,h;
	c=scaninEPS(f,&bbox,&title,&creationdate,&preview,&d,&w,&h);
	if (c!=0) return -2;
	
	 // set up the preview if any
	if (imlibimage) {
		imlib_context_set_image(imlibimage);
		imlib_free_image();
	}
	imlibimage=imlib_image_create(w,h);
	imlib_context_set_image(imlibimage);
	DATA32 *buf=imlib_image_get_data_for_reading_only();
	int pos=0;
	unsigned char p[8];
	do {
		***
		***pixel=((unsigned char)preview[pos])<<(8-depth);
		buf[pos++]=buf[pos++]=buf[pos++]=pixel;
	} while (pos<w*h*4);
}

//----------- eps helper funcs:

//! Get the bounding box, and the preview, title and creation date (if present).
/*! \todo In future, must also be able to extract any resources that must go
 * at the top of an including ps file. Turns preview to a new'd char[], with same
 * depth and data as the preview if any. 
 *
 * Return -1 for not a readable EPS. -2 for error during some point in the reading.
 * 0 for success.
 *
 * \todo *** also need to set up any resources, etc...
 * \todo gs -dNOPAUSE -sDEVICE=pngalpha -sOutputFile=temp234234234.png -r(resolution) whatever.eps
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
	
	c=fread(buf+off,sizeof(char),1024-off,f); // c==0 is either error or eof
	if (!c) break;

	if (strncmp(buf,"%!PS-Adobe-3.0 EPSF-3.0",23) && strncmp(buf,"%!PS-Adobe-2.0 EPSF-2.0",23)) {
		 // not a readable eps
		return -1;
	}
	
	int pos,ppos=0,plen,llen;
	*preview=NULL;
		
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
			plen=(width*depth/8+1)*height;    //expected number of bytes of preview
			llen=width*depth/8+1; //num bytes per line
			*preview=new char[plen];
			while (1) {
				if (getaline(buf,1024,&line,f)<=0) break;
				if (!strncmp(line,"%%EndPreview")) break;
				pos=0;
				while (line[pos] && !isxdigit(line[pos])) pos++;
				(*preview)[ppos++]=line[pos++];
				if (ppos>plen) *** more data than expected... break;
			}
			if (ppos<plen) *** less data than expected...
		} else if (!strncmp(line,"%%Page:",7)) break;
		
		if (!getaline(buf,1024,&line,f)) break;
	}
}

//-------------------------------- EpsInterface ----------------------------------
/*! \class EpsInterface
 * \brief Interface to manipulate placement of eps files.
 *
 * If there is an epsi style preview in the eps, then that is what is put
 * on screen. Otherwise, the title/file/date are put on.
 */
//class EpsInterface : public LaxInterfaces::ImageInterface
//{
// public:
//	EpsInterface(int nid,Laxkit::Displayer *ndp);
//	const char *whattype() { return "EpsInterface"; }
//	const char *whatdatatype() { return "EpsData"; }
//	ImageData *newData();
//	int Refresh();
//};

EpsInterface::EpsInterface(int nid,Laxkit::Displayer *ndp)
	: ImageInterface(nid,ndp)
{
}

//! Redefine same from ImageInterface to never create a random new eps. These can only be imported.
ImageData *EpsInterface::newData()
{
	return NULL;
}

//! Draw title or filename if no preview data.
int EpsInterface::Refresh()
{
	int c=ImageInterface::Refresh();
	if (c!=0) return c;

	 // draw title or filename...
	if (!data->imlibimage && data->filename) {
		flatpoint fp=dp->realtoscreen(flatpoint((data->maxx+data->minx)/2,(data->maxy+data->miny)));
		dp->textout((int)fp.x,(int)fp.y,data->filename,0);
	}
	return 0;
}

