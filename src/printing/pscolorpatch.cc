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
/**************** printing/pscolorpatch.cc ******************/

#include "pscolorpatch.h"

using namespace Laxkit;
using namespace LaxInterfaces;

//#ifndef HIDEGARBAGE
//#include <iostream>
//using namespace std;
//#define DBG 
//#else
//#define DBG //
//#endif


//! Output postscript for a ColorPatchData. *** fix me!
/*! 
 * \todo *** right now, only handles non-subdivided patches...
 *
 * Currently, the ColorPatchData objects exist only in grids, so this function
 * figures out the proper listing for the DataSource tag of the tensor product 
 * patch shading dictionary.
 *
 * In postscript, each patch connects thusly:
 * <pre>
 *  6   7   8   9
 *  5   14  15  10  <-- edge flag==3
 *  4   13  12  11
 * (3   2   1   0)
 *  |   |   |   |
 *  0   11  10  9 -- (3)  4   5   6
 *  1   12  15  8 -- (2)  13  14  7  <-- edge flag==2
 *  2   13  14  7 -- (1)  12  15  8
 *  3   4   5   6 -- (0)  11  10  9
 *  |   |   |   |
 * (0   1   2   3)
 *  11  12  13  4   <-- edge flag==1
 *  10  15  14  5  
 *  9   8   7   6
 *
 *  Colors for the central patch are listed 
 *  (color at 0) - (at 3) - (at 6) - (at 9)
 * </pre>
 * The DataSource is by patch in order of coordinate, then color, then coord,
 * and so on: fxyxyxyxyxy...cccc...fxyxy...ccfxyxy....
 */
void psColorPatch(FILE *f,ColorPatchData *g)
{
	fprintf(f,
			"<<"
			"    /ShadingType  7\n"
			"    /ColorSpace  /DeviceRGB\n"
			"    /DataSource  [\n"
			"      0               %%  edge flag, 0==start a new patch\n"
			"    %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 0 1 2 3
			"    %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 4 5 6 7
			"    %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 8 9 10 11
			"    %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 12 13 14 15
			"    %.10g %.10g %.10g   %.10g %.10g %.10g \n" // c1:0 1 1 c2: 0.5 0 1
			"    %.10g %.10g %.10g   %.10g %.10g %.10g \n" // c3:0.75 0 0  c4:1 1 0.5
			"    ]\n"
			">> shfill\n",
			     //points
				g->points[0].x,  g->points[0].y,
				g->points[4].x,  g->points[4].y,
				g->points[8].x,  g->points[8].y,
				g->points[12].x, g->points[12].y,
				g->points[13].x, g->points[13].y,
				g->points[14].x, g->points[14].y,
				g->points[15].x, g->points[15].y,
				g->points[11].x, g->points[11].y,
				g->points[7].x,  g->points[7].y,
				g->points[3].x,  g->points[3].y,
				g->points[2].x,  g->points[2].y,
				g->points[1].x,  g->points[1].y,
				g->points[5].x,  g->points[5].y,
				g->points[9].x,  g->points[9].y,
				g->points[10].x, g->points[10].y,
				g->points[6].x,  g->points[6].y,
				 //colors
				g->colors[0].red/256.0, g->colors[0].green/256.0, g->colors[0].blue/256.0,
				g->colors[2].red/256.0, g->colors[2].green/256.0, g->colors[2].blue/256.0,
				g->colors[3].red/256.0, g->colors[3].green/256.0, g->colors[3].blue/256.0,
				g->colors[1].red/256.0, g->colors[1].green/256.0, g->colors[1].blue/256.0
			);
//		printf(f,
//			"<<"
//			"    /ShadingType  7"
//			"    /ColorSpace  /DeviceRGB"
//			"    /DataSource  ["
//			"        0                %  edge flag, 0==start a new patch"
//			"    *** 0 1 2 3"
//			"    *** 4 5 6 7"
//			"    *** 8 9 10 11"
//			"    *** 12 13 14 15"
//			"    *** c1:0 1 1 c2: 0.5 0 1 c3:0.75 0 0  c4:1 1 0.5"
//			"    ***   ...etc"
//			"    ]"
//			">> shfill");

}
