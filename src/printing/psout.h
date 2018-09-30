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
// Copyright (c) 2004-2007 Tom Lechner
//
#ifndef PSOUT_H
#define PSOUT_H

#include "../document.h"
#include <cstdio>


namespace Laidout {


double psDpi();
double psDpi(double n);

void psConcat(const double *m);
void psConcat(double a,double b,double c,double d,double e,double f);
double *psCtmInit();
double *psCTM();
void psPushCtm();
void psPopCtm();
void psFlushCtms();

void psdumpobj(FILE *f,LaxInterfaces::SomeData *obj);
int psSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing=0);
int  psout(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);
int epsout(const char *filename, Laxkit::anObject *context, Laxkit::ErrorLog &log);

} // namespace Laidout

#endif


