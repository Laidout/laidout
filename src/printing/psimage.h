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
// Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef PSIMAGE_H
#define PSIMAGE_H

#include <lax/interfaces/imageinterface.h>
#include <cstdio>


namespace Laidout {


int psImage(FILE *f,LaxInterfaces::ImageData *i);
int psImage_masked_interleave1(FILE *f, unsigned char *buf, int width, int height);
int psImage_103(FILE *f, unsigned char *buf, int width, int height);


} // namespace Laidout
#endif

