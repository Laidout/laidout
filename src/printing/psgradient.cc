//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include "psgradient.h"

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 


//! Output postscript for a GradientData. ***imp me!!
/*! 
 */
void psGradient(FILE *f,GradientData *g)
{
	 // Radial vs. axial gradients are virtually identical.
	 // They differ in the Coords memeber: [x0 y0 r0 x1 y1 r1] vx. [x0 y0 x1 y1] 
	 // And shading dict type 3 vs. 2
	 
	int c;
	double clen=g->colors[g->colors.n-1]->t-g->colors[0]->t;
	fprintf(f,"\n\n");
	for (c=1; c<g->colors.n; c++) {
		fprintf(f,"/gradientf%d <<\n"
				  "  /FunctionType 2\n"
				  "  /Domain [0 1]\n"
				  "  /C0 [%.10g %.10g %.10g]\n"
				  "  /C1 [%.10g %.10g %.10g]\n"
				  "  /N 1\n"
				  ">> def\n\n",
					c, 
					g->colors.e[c-1]->red/255.0, g->colors.e[c-1]->green/255.0, g->colors.e[c-1]->blue/255.0, 
					g->colors.e[c  ]->red/255.0, g->colors.e[c  ]->green/255.0, g->colors.e[c  ]->blue/255.0
				);
	}

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
