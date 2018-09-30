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
// Copyright (C) 2004-2007 by Tom Lechner
//

#include "psgradient.h"

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



namespace Laidout {


//! Output postscript for a GradientData. 
/*! \ingroup postscript
 */
void psGradient(FILE *f,GradientData *g)
{
	 // Radial vs. axial gradients are virtually identical.
	 // They differ in the Coords member: [x0 y0 r0 x1 y1 r1] vs. [x0 y0 x1 y1] 
	 // And shading dict type 3 vs. 2
	 
	int c;
	double clen=g->colors[g->colors.n-1]->t-g->colors[0]->t;
	fprintf(f,"\n\n");
	 //individual functions
	for (c=1; c<g->colors.n; c++) {
		fprintf(f,"/gradientf%d <<\n"
				  "  /FunctionType 2\n"
				  "  /Domain [0 1]\n"
				  "  /C0 [%.10g %.10g %.10g]\n"
				  "  /C1 [%.10g %.10g %.10g]\n"
				  "  /N 1\n"
				  ">> def\n\n",
					c, 
					g->colors.e[c-1]->color.red/65535.0, 
					g->colors.e[c-1]->color.green/65535.0,
					g->colors.e[c-1]->color.blue/65535.0, 
					g->colors.e[c  ]->color.red/65535.0,
					g->colors.e[c  ]->color.green/65535.0, 
					g->colors.e[c  ]->color.blue/65535.0
				);
	}

	 //shading dictionary
	fprintf(f,
			"<<\n"
			"    /ShadingType  %d\n"
			"    /ColorSpace  /DeviceRGB\n",
			  (g->style&GRADIENT_RADIAL)?3:2);
	if (g->style&GRADIENT_RADIAL) fprintf(f,"    /Coords [ %.10g 0 %.10g %.10g 0 %.10g ]\n",
			  g->p1, fabs(g->r1), //x0, r0
			  g->p2, fabs(g->r2)); //x1, r1
	else fprintf(f,"    /Coords [ %.10g 0 %.10g 0]\n",
			  g->p1, g->p2);

	fprintf(f,
			"    /BBox   [ %.10g %.10g %.10g %.10g ]\n",//[l b r t]
			  g->minx, g->miny, g->maxx, g->maxy);
	fprintf(f,
			"    /Function <<\n"
			"      /FunctionType 3\n"
			"      /Functions [");
	for (c=1; c<g->colors.n; c++) fprintf(f,"gradientf%d ",c);
	fprintf(f,
			"]\n"
			"      /Domain [0 1]\n"
			"      /Bounds [");
	for (c=1; c<g->colors.n-1; c++) fprintf(f,"%.10g ", (g->colors.e[c]->t-g->colors.e[0]->t)/clen);
	fprintf(f,
			"]\n"
			"      /Encode [");
	for (c=1; c<g->colors.n; c++) fprintf(f,"0 1 ");
	fprintf(f,
			"]\n"
			"    >>\n"
			">> shfill\n"
		);
}


} // namespace Laidout

