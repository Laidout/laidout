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
#ifndef EXTRAS_H
#define EXTRAS_H

#include "laidout.h"
#include <lax/interfaces/imageinterface.h>

int dumpImages(Document *doc, int startpage, const char *pathtoimagedir, int imagesperpage=1, int ddpi=150);
int dumpImages(Document *doc, int startpage, const char **imagefiles, int nimages, int imagesperpage=1, int ddpi=150);
int dumpImages(Document *doc, int startpage, LaxInterfaces::ImageData **images, int nimages, int imagesperpage=1, int ddpi=150);

#endif
