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
// Copyright (C) 2010,2011 by Tom Lechner
//
#ifndef PODOFOIMPOSEPLAN_H
#define PODOFOIMPOSEPLAN_H

#include "../impositions/imposition.h"
#include "filefilters.h"
#include "../version.h"



namespace Laidout {


void installPodofoFilter();

//------------------------------------- PodofooutFilter -----------------------------------
class PodofooutFilter : public ExportFilter
{
 protected:
 public:
	PodofooutFilter();
	virtual const char *Author() { return "Laidout"; }
	virtual const char *FilterVersion() { return LAIDOUT_VERSION; }
	
	virtual const char *Format() { return "Podofoimpose PLAN"; }
	virtual const char *DefaultExtension() { return "plan"; }
	virtual const char *Version() { return "0.7"; }
	virtual const char *VersionName();
	virtual const char *FilterClass() { return "document"; }
	virtual ObjectDef *GetObjectDef();
	
	virtual int Out(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);
};


} // namespace Laidout

#endif

