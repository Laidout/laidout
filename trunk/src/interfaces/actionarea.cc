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
// Copyright (C) 2012 by Tom Lechner
//

#include <lax/strmanip.h>
#include "actionarea.h"

using namespace Laxkit;


namespace Laidout {


//------------------------------------- ActionArea --------------------------------------
//typedef void RenderActionAreaFunc(Laxkit::Displayer *disp,ActionArea *overlay);


/*! \class ActionArea
 * \brief This can define areas on screen for various purposes.
 */
/*! \var int ActionArea::offset
 * \brief The outline is at location offset+(the points in outline).
 */
/*! \var int ActionArea::hotspot
 * \brief Defines a point of focus of the area.
 *
 * Say an area defines an arrow, the hotspot would be the point of the arrow.
 * When you call Position(double,double,int) to set the area's position, you are
 * actually setting the position of the hotspot.
 */


ActionArea::ActionArea(int what,int ntype,const char *txt,const char *ntooltip,
					   int isreal,int isvisible,unsigned long ncolor,int ncategory)
{
	text=newstr(txt);
	tip=newstr(ntooltip);
	outline=NULL;
	npoints=0;
	visible=isvisible;
	real=isreal;
	color=ncolor;
	action=what;
	type=ntype;
	hidden=0;
	category=ncategory;
}

ActionArea::~ActionArea()
{
	if (outline) delete[] outline;
	if (tip) delete[] tip;
	if (text) delete[] text;
}

//! Change the position of the area, where pos==offset+hotspot.
/*! If which&1, adjust x. If which&2, adjust y.
 *
 * To be clear, this will make the x and/or y coordinate of the area's hotspot be
 * at the given (x,y).
 */
void ActionArea::Position(double x,double y,int which)
{
	if (which&1) offset.x=x-hotspot.x;
	if (which&2) offset.y=y-hotspot.y;
}

//! Return if point is within the outline (If outline is not NULL), or within the ActionArea DoubleBBox bounds otherwise.
/*! No point transformation is done. If real==1, it is assumed x,y are real coordinates, and otherwise it is
 * assumed they are screen coordinates.
 */
int ActionArea::PointIn(double x,double y)
{
	x-=offset.x; y-=offset.y;
	if (npoints) return point_is_in(flatpoint(x,y),outline,npoints);
	return x>=minx && x<=maxx && y>=miny && y<=maxy;
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
		memcpy(outline,pts,sizeof(flatpoint));
	}

	npoints=n;

	return outline;
}

//! Make the bounds be the actual bounds of outline.
/*! If there are no points in outline, then do nothing.
 */
void ActionArea::FindBBox()
{
	if (!npoints) return;

	clear();
	for (int c=0; c<npoints; c++) addtobounds(outline[c]);
}


} //namespace Laidout

