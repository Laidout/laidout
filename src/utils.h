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
#include <lax/attributes.h>

const char *Untitled_name();
FILE *open_laidout_file_to_read(const char *file,const char *what,char **error_ret);
FILE *open_file_for_reading(const char *file,char **error_ret);
FILE *open_file_for_writing(const char *file,int nooverwrite,char **error_ret);
char *previewFileName(const char *file, const char *nametemplate);

int laidout_file_type(const char *file, const char *minversion, const char *maxversion, char **actual_version,
					  const char *typ, char **actual_type);
int laidout_version_check(const char *version, const char *minversion, const char *maxversion);

const char *IdentifyFile(const char *file, char **version1, char **version2);
int isOffFile(const char *file);
int isEpsFile(const char *file,float *psversion, float *epsversion);
int is_bitmap_image(const char *file);

#endif

