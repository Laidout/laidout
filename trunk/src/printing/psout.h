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
#ifndef PSOUT_H
#define PSOUT_H

#include "../document.h"
#include <cstdio>

void psdumpobj(FILE *f,LaxInterfaces::SomeData *obj);
int psout(FILE *f,Document *doc);
int psout(Document *doc,const char *file=NULL);
int psSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing=0);

#endif


