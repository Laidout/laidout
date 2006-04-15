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
#ifndef PSFILTERS_H
#define PSFILTERS_H

#include <cstdio>

int Ascii85_out(std::FILE *f,unsigned char *in,int len,int puteod,int linewidth,int *curwidth=NULL);

#endif

