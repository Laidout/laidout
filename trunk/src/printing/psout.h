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
#ifndef PSOUT_H
#define PSOUT_H

#include "../document.h"
#include <cstdio>

void psdumpobj(FILE *f,LaxInterfaces::SomeData *obj);
int psout(FILE *f,Document *doc);
int psout(Document *doc,const char *file=NULL);
int psSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing=0);

#endif


