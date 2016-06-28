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
// Copyright (c) 2004-2010 Tom Lechner
//
#ifndef DRAWDATA_H
#define DRAWDATA_H

#include <lax/displayer.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/interfacemanager.h>


namespace Laidout {

enum ViewDrawFlags {
	DRAW_AXES  = (1<<(LaxInterfaces::InterfaceManager::DRAW_MAX+1)),
	DRAW_BOX   = (1<<(LaxInterfaces::InterfaceManager::DRAW_MAX+2)),
	DRAW_HIRES = (1<<(LaxInterfaces::InterfaceManager::DRAW_MAX+3)),
};

//void DrawData(Laxkit::Displayer *dp,double *m,LaxInterfaces::SomeData *data,
				//Laxkit::anObject *a1,Laxkit::anObject *a2,unsigned int flags=0);
void DrawDataStraight(Laxkit::Displayer *dp,LaxInterfaces::SomeData *data,
				Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,unsigned int flags=0);
void DrawData(Laxkit::Displayer *dp,LaxInterfaces::SomeData *data,
				Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,unsigned int flags=0);
LaxInterfaces::SomeData *newObject(const char *thetype);
int boxisin(flatpoint *points, int n,Laxkit::DoubleBBox *bbox);


} // namespace Laidout

#endif

