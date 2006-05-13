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
/**************** printing/pscolorpatch.cc ******************/

#include "pscolorpatch.h"

using namespace Laxkit;
using namespace LaxInterfaces;

//#include <iostream>
//using namespace std;
//#define DBG 

#define LBLT 0
#define LTRT 1
#define RTRB 2
#define RBLB 3
#define LTLB 4
#define LBRB 5
#define RBRT 6 
#define RTLT 7

void psContinueColorPatch(FILE *f,ColorPatchData *g, int flag,int o, int r,int c);

//! Output postscript for a ColorPatchData. 
/*! \ingroup postscript
 *
 * Currently, the ColorPatchData objects exist only in grids, so this function
 * figures out the proper listing for the DataSource tag of the tensor product 
 * patch shading dictionary.
 *
 * The strategy is do each patch horizontally to the right, then move one down, then do
 * the next row travelling to the left, and so on.
 *
 */
void psColorPatch(FILE *f,ColorPatchData *g)
{
	int r,c,          // row and column of a patch, not of the coords, which are *3
		rows,columns, // the number of patches
		xs,ys;        //g->xsize and ysize
	xs=g->xsize;
	ys=g->ysize;
	rows=g->ysize/3;
	columns=g->xsize/3;
	r=0;
	
	fprintf(f,
			"<<"
			"    /ShadingType  7\n"
			"    /ColorSpace  /DeviceRGB\n"
			"    /DataSource  [\n");
	
	 // install first patch
	psContinueColorPatch(f,g, 0,LBLT, 0,0);

	 // handle single column case separately
	if (columns==1) {
		r=1;
		if (r<rows) {
			psContinueColorPatch(f,g, 3,RTLT, r,c);
			r++;
			while (r<rows) {
				if (r%2) psContinueColorPatch(f,g, 2,RTLT, r,c);
					else psContinueColorPatch(f,g, 2,LTRT, r,c);
				r++;
			}
		}
	}
	 
	 // general case
	c=1;
	while (r<rows) {

		 // add patches left to right
		while (c<columns) {
			if (c%2) psContinueColorPatch(f,g, 2,LTLB, r,c);
				else psContinueColorPatch(f,g, 2,LBLT, r,c);
			c++;
		}
		r++;

		 // if more rows, add the next row, travelling right to left
		if (r<rows) {
			c--;
			 // add connection downward, and the patch immediately
			 // to the left of it.
			if (c%2) {
				psContinueColorPatch(f,g, 1,LTRT, r,c);
				c--;
				if (c>=0) psContinueColorPatch(f,g, 3,RBRT, r,c);
				c--;
			} else {
				psContinueColorPatch(f,g, 3,RTLT, r,c);
				c--;
				if (c>=0) { psContinueColorPatch(f,g, 1,RTRB, r,c); c--; }
				c--;
			}

			 // continue adding patches right to left
			while (c>=0) {
				  // add patches leftward
				if (c%2) psContinueColorPatch(f,g, 2,RTRB, r,c);
					else psContinueColorPatch(f,g, 2,RBRT, r,c);
				 c--;
			}
			r++;
			 // add connection downward, and the patch to the right
			if (r<rows) {
				c++;
				 // 0 is always even, so only one kind of extention here
				psContinueColorPatch(f,g, 3,LTRT, r,c);
				c++; // c will be 1 here, and columns we already know is > 1
				psContinueColorPatch(f,g, 1,LTLB, r,c);
				c++;
			}
		}
	}
	fprintf(f,"    ]\n"     // end of DataSource, final part of dict, so do shfill..
			">> shfill\n\n");
}

static char ro[][16]={{3,2,1,0,0,0,0,1,2,3,3,3,2,1,1,2},
					  {0,0,0,0,1,2,3,3,3,3,2,1,1,1,2,2},
					  {0,1,2,3,3,3,3,2,1,0,0,0,1,2,2,1},
					  {3,3,3,3,2,1,0,0,0,0,1,2,2,2,1,1},
					  {0,1,2,3,3,3,3,2,1,0,0,0,1,2,2,1},
					  {3,3,3,3,2,1,0,0,0,0,1,2,2,2,1,1},
					  {3,2,1,0,0,0,0,1,2,3,3,3,2,1,1,2},
					  {0,0,0,0,1,2,3,3,3,3,2,1,1,1,2,2}},
			co[][16]={{0,0,0,0,1,2,3,3,3,3,2,1,1,1,2,2},
					  {0,1,2,3,3,3,3,2,1,0,0,0,1,2,2,1},
					  {3,3,3,3,2,1,0,0,0,0,1,2,2,2,1,1},
					  {3,2,1,0,0,0,0,1,2,3,3,3,2,1,1,2},
					  {0,0,0,0,1,2,3,3,3,3,2,1,1,1,2,2},
					  {0,1,2,3,3,3,3,2,1,0,0,0,1,2,2,1},
					  {3,3,3,3,2,1,0,0,0,0,1,2,2,2,1,1},
					  {3,2,1,0,0,0,0,1,2,3,3,3,2,1,1,2}};

//! Continue writing out the 'DataSource' element of a bez color patch shading dictionary.
/*! \ingroup postscript
 *
 * The orientation can be 0 to 7. 0 is map the points so that they travel the
 * patch from the left-bottom to the left-top (LBLT) and so on. The orientation is one of:
 * <pre>
 *   LBLT == 0
 *   LTRT == 1
 *   RTRB == 2
 *   RBLB == 3
 *   LTLB == 4
 *   LBRB == 5
 *   RBRT == 6 
 *   RTLT == 7
 * </pre>
 * 
 * 
 * A starting patch has flag==0. In postscript, each further patch connects thusly:
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
void psContinueColorPatch(FILE *f,ColorPatchData *g,
						int flag, //!< The edge flag
						int o,    //!< orientation of the section
						int r,    //!< Row of upper corner (patch index, not coord index)
						int c)    //!< Column of upper corner (patch index, not coord index)
{
	fprintf(f,"      %d\n",flag);

	r*=3; 
	c*=3;
	int xs=g->xsize, i=r*g->xsize+c,
		cx=g->xsize/3+1, ci=r/3*cx+c/3,
		c3=ci + (ro[o][6]?1:0)*cx + (co[o][6]?1:0),
		c4=ci + (ro[o][9]?1:0)*cx + (co[o][9]?1:0);
	if (flag==0) {
		int c1=ci + (ro[o][0]?1:0)*cx + (co[o][0]?1:0),
			c2=ci + (ro[o][3]?1:0)*cx + (co[o][3]?1:0);

		fprintf(f,
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 0 1 2 3
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 4 5 6 7
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 8 9 10 11
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 12 13 14 15
			"      %.10g %.10g %.10g   %.10g %.10g %.10g \n" // c1:0 1 1 c2: 0.5 0 1
			"      %.10g %.10g %.10g   %.10g %.10g %.10g \n", // c3:0.75 0 0  c4:1 1 0.5
			     //points
				g->points[i+ro[o][ 0]*xs+co[o][ 0]].x, g->points[i+ro[o][ 0]*xs+co[o][ 0]].y,
				g->points[i+ro[o][ 1]*xs+co[o][ 1]].x, g->points[i+ro[o][ 1]*xs+co[o][ 1]].y,
				g->points[i+ro[o][ 2]*xs+co[o][ 2]].x, g->points[i+ro[o][ 2]*xs+co[o][ 2]].y,
				g->points[i+ro[o][ 3]*xs+co[o][ 3]].x, g->points[i+ro[o][ 3]*xs+co[o][ 3]].y,
				g->points[i+ro[o][ 4]*xs+co[o][ 4]].x, g->points[i+ro[o][ 4]*xs+co[o][ 4]].y,
				g->points[i+ro[o][ 5]*xs+co[o][ 5]].x, g->points[i+ro[o][ 5]*xs+co[o][ 5]].y,
				g->points[i+ro[o][ 6]*xs+co[o][ 6]].x, g->points[i+ro[o][ 6]*xs+co[o][ 6]].y,
				g->points[i+ro[o][ 7]*xs+co[o][ 7]].x, g->points[i+ro[o][ 7]*xs+co[o][ 7]].y,
				g->points[i+ro[o][ 8]*xs+co[o][ 8]].x, g->points[i+ro[o][ 8]*xs+co[o][ 8]].y,
				g->points[i+ro[o][ 9]*xs+co[o][ 9]].x, g->points[i+ro[o][ 9]*xs+co[o][ 9]].y,
				g->points[i+ro[o][10]*xs+co[o][10]].x, g->points[i+ro[o][10]*xs+co[o][10]].y,
				g->points[i+ro[o][11]*xs+co[o][11]].x, g->points[i+ro[o][11]*xs+co[o][11]].y,
				g->points[i+ro[o][12]*xs+co[o][12]].x, g->points[i+ro[o][12]*xs+co[o][12]].y,
				g->points[i+ro[o][13]*xs+co[o][13]].x, g->points[i+ro[o][13]*xs+co[o][13]].y,
				g->points[i+ro[o][14]*xs+co[o][14]].x, g->points[i+ro[o][14]*xs+co[o][14]].y,
				g->points[i+ro[o][15]*xs+co[o][15]].x, g->points[i+ro[o][15]*xs+co[o][15]].y,
				 //colors
				g->colors[c1].red/256.0, g->colors[c1].green/256.0, g->colors[c1].blue/256.0,
				g->colors[c2].red/256.0, g->colors[c2].green/256.0, g->colors[c2].blue/256.0,
				g->colors[c3].red/256.0, g->colors[c3].green/256.0, g->colors[c3].blue/256.0,
				g->colors[c4].red/256.0, g->colors[c4].green/256.0, g->colors[c4].blue/256.0);
	} else {
		fprintf(f,
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 4 5 6 7
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 8 9 10 11
			"      %.10g %.10g  %.10g %.10g  %.10g %.10g  %.10g %.10g \n" // 12 13 14 15
			"      %.10g %.10g %.10g   %.10g %.10g %.10g \n", // c3:0.75 0 0  c4:1 1 0.5
			     //points
				g->points[i+ro[o][ 4]*xs+co[o][ 4]].x, g->points[i+ro[o][ 4]*xs+co[o][ 4]].y,
				g->points[i+ro[o][ 5]*xs+co[o][ 5]].x, g->points[i+ro[o][ 5]*xs+co[o][ 5]].y,
				g->points[i+ro[o][ 6]*xs+co[o][ 6]].x, g->points[i+ro[o][ 6]*xs+co[o][ 6]].y,
				g->points[i+ro[o][ 7]*xs+co[o][ 7]].x, g->points[i+ro[o][ 7]*xs+co[o][ 7]].y,
				g->points[i+ro[o][ 8]*xs+co[o][ 8]].x, g->points[i+ro[o][ 8]*xs+co[o][ 8]].y,
				g->points[i+ro[o][ 9]*xs+co[o][ 9]].x, g->points[i+ro[o][ 9]*xs+co[o][ 9]].y,
				g->points[i+ro[o][10]*xs+co[o][10]].x, g->points[i+ro[o][10]*xs+co[o][10]].y,
				g->points[i+ro[o][11]*xs+co[o][11]].x, g->points[i+ro[o][11]*xs+co[o][11]].y,
				g->points[i+ro[o][12]*xs+co[o][12]].x, g->points[i+ro[o][12]*xs+co[o][12]].y,
				g->points[i+ro[o][13]*xs+co[o][13]].x, g->points[i+ro[o][13]*xs+co[o][13]].y,
				g->points[i+ro[o][14]*xs+co[o][14]].x, g->points[i+ro[o][14]*xs+co[o][14]].y,
				g->points[i+ro[o][15]*xs+co[o][15]].x, g->points[i+ro[o][15]*xs+co[o][15]].y,
				 //colors
				g->colors[c3].red/256.0, g->colors[c3].green/256.0, g->colors[c3].blue/256.0,
				g->colors[c4].red/256.0, g->colors[c4].green/256.0, g->colors[c4].blue/256.0);
	}
}
