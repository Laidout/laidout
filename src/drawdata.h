//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef DRAWDATA_H
#define DRAWDATA_H

#include <lax/displayer.h>
#include <lax/interfaces/somedata.h>

void DrawData(Laxkit::Displayer *dp,double *m,Laxkit::SomeData *data,Laxkit::anObject *a1,Laxkit::anObject *a2);
void DrawData(Laxkit::Displayer *dp,Laxkit::SomeData *data,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL);
Laxkit::SomeData *newObject(const char *thetype);
int pointisin(flatpoint *points, int n,flatpoint p);
int boxisin(flatpoint *points, int n,Laxkit::DoubleBBox *bbox);

#endif

