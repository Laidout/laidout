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
#ifndef EPSDATA_H
#define EPSDATA_H

#include <lax/interfaces/imageinterface.h>


namespace Laidout {


//-------------------------------- EpsData ----------------------------------
class EpsData : public LaxInterfaces::ImageData
{
 public:
	char *title, *creationdate, *resources;
	EpsData(const char *nfilename=NULL, const char *npreview=NULL, 
			  int maxpx=0, int maxpy=0, char delpreview=0);
	virtual ~EpsData();
	virtual const char *whattype() { return "EpsData"; }
	virtual int LoadImage(const char *fname, const char *npreview=NULL, int maxpx=0, int maxpy=0, bool fit=0,int index=0);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


} //namespace Laidout

#endif



