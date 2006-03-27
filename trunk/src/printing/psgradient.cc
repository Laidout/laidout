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
	if (g->style&GRADIENT_RADIAL) {
		cout <<" *** GradientData radial ps out not implemented! "<<endl;
	} else {
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
				"    /ShadingType  2\n"
				"    /ColorSpace  /DeviceRGB\n"
				"    /Coords [ %.10g 0 %.10g 0]\n",
				  g->colors.e[0]->t, g->colors.e[g->colors.n-1]->t);
		fprintf(f,
				"    /BBox   [ %.10g %.10g %.10g %.10g ]\n",//[l b r t]
				  g->colors.e[0]->t, -fabs(g->v), g->colors.e[g->colors.n-1]->t, fabs(g->v));
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
}
