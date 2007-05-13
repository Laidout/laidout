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
// Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef FILETYPES_SCRIBUS_H
#define FILETYPES_SCRIBUS_H


****************** NOT ACTIVE ******************************

#include "../document.h"


int scribusout(const char *scribusversion, Document *doc,const char *filename,	int layout,int start,int end);
Document *scribusin(const char *file,Document *doc,int startpage);


#endif




