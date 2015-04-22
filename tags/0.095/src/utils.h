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
// Copyright (C) 2007,2010 by Tom Lechner
//

#ifndef UTILS_H
#define UTILS_H

#include <lax/errorlog.h>
#include <lax/attributes.h>


namespace Laidout {


//----------------------------------- unique name functions ------------------------------
const char *Untitled_name();


//--------------------------------- number range naming helpers --------------------------
char *make_labelbase_for_printf(const char *f,int *len);
char *letter_numeral(int i,char cap);
char *roman_numeral(int i,char cap);


//----------------------------------- File i/o helpers ---------------------------------------------
FILE *open_laidout_file_to_read(const char *file,const char *what,Laxkit::ErrorLog *log, bool warn_if_fail=true);
FILE *open_file_for_reading(const char *file,Laxkit::ErrorLog *log);
FILE *open_file_for_writing(const char *file,int nooverwrite,Laxkit::ErrorLog *log);
int resource_name_and_desc(FILE *f,char **name, char **desc);
char *previewFileName(const char *file, const char *nametemplate);

int laidout_file_type(const char *file, const char *minversion, const char *maxversion, char **actual_version,
					  const char *typ, char **actual_type);
int laidout_version_check(const char *version, const char *minversion, const char *maxversion);

//-----------------------------System Resources-----------------------------------------
char *get_system_default_paper(const char *file="/etc/papersize");

//------------------------------ File identification functions -------------------------------
const char *IdentifyFile(const char *file, char **version1, char **version2);
int isOffFile(const char *file);
int isPdfFile(const char *file,float *pdfversion);
int isEpsFile(const char *file,float *psversion, float *epsversion);
int isScribusFile(const char *file);
int isJpg(const char *file);
int is_bitmap_image(const char *file);


} // namespace Laidout

#endif

