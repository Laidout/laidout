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
// Copyright (C) 2007 by Tom Lechner
//

#ifndef UTILS_H
#define UTILS_H

#include <cstdio>

int laidout_file_type(const char *file, char *minversion, char *maxversion, char *typ);
int laidout_version_check(const char *version, const char *minversion, const char *maxversion);
FILE *open_file_to_read(const char *file,const char *what,char **error_ret);


#endif

