//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2007 by Tom Lechner
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
/**************** printing/pspathsdata.cc ******************/

#include "pspathsdata.h"

using namespace Laxkit;
using namespace LaxInterfaces;

//! Output postscript for a PathsData. *** fix me!
/*! 
 * \todo *** right now, only handles polylines, ignores subops.
 * \todo *** draws with default color, ignores linestyle
 */
void psPathsData(FILE *f,PathsData *path)
{
	Coordinate *p,*start;
	flatpoint pp;
	int n;
	for (int c=0; c<path->paths.n; c++) {
		p=start=path->paths.e[c]->path;
		if (!p) continue;
		n=0; //number of points seen
		do {
			pp=p->p();
			n++;
			fprintf(f,"%.10g %.10g ",pp.x,pp.y);
			if (n==1) fprintf(f,"moveto\n");
			else fprintf(f,"lineto\n");
			p=p->next;
		} while (p && p!=start);
		if (p==start) fprintf(f,"closepath ");
		fprintf(f,"stroke\n");
	}
}

