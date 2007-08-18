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



#ifndef LANGUAGE_H
#define LANGUAGE_H


#if 1

 //yes gettext
#include <libintl.h>

#define _(str) gettext(str)
#define gettext_noop(str) str
#define N_(str) gettext_noop(str)
#else

 //no gettext
#define _(str) str
#define gettext_noop(str) str
#define N_(str) gettext_noop(str)

#endif


#endif



