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
#ifndef PSIMAGE_H
#define PSIMAGE_H

#include <lax/interfaces/imageinterface.h>
#include <cstdio>

void psImage(FILE *f,LaxInterfaces::ImageData *i);
void psImage_masked_interleave1(FILE *f,LaxInterfaces::ImageData *img);
void psImage_masked_interleave2(FILE *f,LaxInterfaces::ImageData *img);
void psImage_103(FILE *f,LaxInterfaces::ImageData *img);

#endif
