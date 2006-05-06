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

double psDpi();
double psDpi(double n);

void psConcat(double *m);
void psConcat(double a,double b,double c,double d,double e,double f);
double *psCTM();
void psPushCtm();
void psPopCtm();
void psFlushCtms();

void psdumpobj(FILE *f,LaxInterfaces::SomeData *obj);
int psSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing=0);
int psout(FILE *f,Document *doc,int start=-1,int end=-1,unsigned int flags=0);
int psout(Document *doc,const char *file=NULL);
int epsout(const char *fname,Document *doc,int start,int end,
		int layouttype,unsigned int flags);

#endif


