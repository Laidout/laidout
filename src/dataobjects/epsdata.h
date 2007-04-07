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
#ifndef EPSDATA_H
#define EPSDATA_H

#include <lax/interfaces/imageinterface.h>

//-------------------------------- EpsData ----------------------------------
class EpsData : public LaxInterfaces::ImageData
{
 public:
	char *title, *creationdate, *resources;
	EpsData(const char *nfilename=NULL, const char *npreview=NULL, 
			  int maxpx=0, int maxpy=0, char delpreview=1);
	virtual ~EpsData();
	virtual const char *whattype() { return "EpsData"; }
	virtual int LoadImage(const char *fname, const char *npreview=NULL, int maxpx=0, int maxpy=0, char del=0);
};

//-------------------------------- EpsInterface ----------------------------------
class EpsInterface : public LaxInterfaces::ImageInterface
{
 public:
	EpsInterface(int nid,Laxkit::Displayer *ndp);
	virtual const char *whattype() { return "EpsInterface"; }
	virtual const char *whatdatatype() { return "EpsData"; }
	virtual int draws(const char *what);
	virtual LaxInterfaces::ImageData *newData();
	virtual int Refresh();
};

#endif



