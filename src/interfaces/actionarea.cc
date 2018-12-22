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
// Copyright (C) 2012-2013 by Tom Lechner
//

#include <lax/strmanip.h>
#include "actionarea.h"

#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;


namespace Laidout {


//------------------------------------- ActionArea --------------------------------------


/*! \class ActionArea
 *
 * This defines simple areas on screen for various interaction purposes.
 * It consists of an outline, a hotspot, tooltip, and tags for whether the outline is to be rendered
 * in screen coordinates or real coordinates.
 */
/*! \var int ActionArea::offset
 * hotspot is at this location.
 */
/*! \var int ActionArea::hotspot
 * \brief Defines a point of focus of the area, in same coordinates as points.
 *
 * Say an area defines an arrow, the hotspot would be the point of the arrow.
 * When you call Position(double,double,int) to set the area's position, you are
 * actually setting the position of the hotspot.
 */


ActionArea::ActionArea(int what,int ntype,const char *txt,const char *ntooltip,
					   int isreal,int isvisible,unsigned long ncolor,int ncategory,
					   unsigned long ntcolor)
{
	text=newstr(txt);
	tip=newstr(ntooltip);
	outline=NULL;
	npoints=0;
	visible=isvisible;
	real=isreal;
	action=what;
	type=ntype;
	hidden=0;
	mode=0;
	category=ncategory;
	xdirection=flatpoint(1,0);
	usetail=0;

	color=ncolor;
	color_text=ntcolor;
}

ActionArea::~ActionArea()
{
	if (outline) delete[] outline;
	if (tip) delete[] tip;
	if (text) delete[] text;
}


flatpoint ActionArea::Center()
{
	if (maxx<minx || maxy<miny) return flatpoint(minx,miny);
	return flatpoint((minx+maxx)/2,(miny+maxy)/2);
}


/*! Just returns offset.
 */
flatpoint ActionArea::Position()
{ return offset; }

//! Change the position of the area, where pos==offset+hotspot.
/*! If which&1, adjust x. If which&2, adjust y.
 *
 * To be clear, this will make the x and/or y coordinate of the area's hotspot be
 * at the given (x,y).
 */
void ActionArea::Position(double x,double y,int which)
{
	if (which&1) offset.x=x;
	if (which&2) offset.y=y;
}

//! Return if point is within the outline (If outline is not NULL), or within the ActionArea DoubleBBox bounds otherwise.
/*! No point transformation is done. It is assumed that x and y are transformed already to the proper coordinate space.
 * This is to say that the calling code must account for this->real.
 *
 * If real==2, then (x,y) is assumed to have been transformed to point coordinates
 */
int ActionArea::PointIn(double x,double y)
{
	if (real!=2) { x-=offset.x;  y-=offset.y; }
	if (real==1 || real==2) { x+=hotspot.x; y+=hotspot.y; }

	if (npoints) return point_is_in(flatpoint(x,y),outline,npoints);
	return x>=minx && x<=maxx && y>=miny && y<=maxy;
}

void ActionArea::SetRect(double x,double y,double w,double h)
{
	if (outline && npoints<4) { delete[] outline; outline=NULL; }
	if (!outline) outline=new flatpoint[4];

	npoints=4;
	outline[0]=flatpoint(x,y);
	outline[1]=flatpoint(x+w,y);
	outline[2]=flatpoint(x+w,y+h);
	outline[3]=flatpoint(x,y+h);
	FindBBox();
}

//! Create outline points.
/*! If takethem!=0, then make outline=pts, and that array will be delete[]'d in the destructor.
 * Otherise, the points are copied into a new array.
 *
 * If pts==NULL, but n>0, then allocate (or reallocate) the array.
 * It is presumed that the calling code will then adjust the points.
 *
 * outline will be reallocated only if n>npoints.
 *
 * Please note that this function does not set the min/max bounds. You can use FindBBox for that.
 */
flatpoint *ActionArea::Points(flatpoint *pts, int n, int takethem)
{
	if (n<=0) return NULL;

	if (!pts) {
		if (n>npoints && outline) { delete[] outline; outline=NULL; }
		if (!outline) outline=new flatpoint[n];

	} else if (takethem) {
		delete[] outline;
		outline=pts;

	} else {
		if (n>npoints && outline) { delete[] outline; outline=NULL; }
		outline=new flatpoint[n];
		//memcpy(outline,pts,n*sizeof(flatpoint));
		for (int c=0; c<n; c++) outline[c] = pts[c];
	}

	npoints=n;

	return outline;
}

//! Make the bounds be the actual bounds of outline, in outline space.
/*! If there are no points in outline, then do nothing.
 */
void ActionArea::FindBBox()
{
	if (!npoints) return;

	DoubleBBox::clear(); //makes bounding box invalid
	for (int c=0; c<npoints; c++) addtobounds(outline[c]);
}


} //namespace Laidout

