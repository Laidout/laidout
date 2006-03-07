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
/***************** drawdata.cc **********************/

//------------------------- General Drawing Object Utilities --------------------------------

/*! \defgroup objects Miscellaneous Object Utilities
 *
 * Here are various utilities to deal with creation, deletion, and drawing of various
 * kinds of objects.
 * There are functions to variously create and draw anything anywhere (as appropriate).
 */


#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
//#include <lax/interfaces/gradientinterface.h>

#include <lax/interfaces/somedataref.h>
#include "drawdata.h"
#include "laidout.h"
using namespace Laxkit;

//! Push axes and transform by m, draw data, pop axes.
/*! \ingroup objects
 * *** uh, this is unnecessary? the other one does this already....
 * should it not??
 */
void DrawData(Displayer *dp,double *m,SomeData *data,anObject *a1,anObject *a2)
{
	dp->PushAndNewTransform(m);
	DrawData(dp,data,a1,a2);
	dp->PopAxes();
}

//! Draw data using the transform of the data....
/*! \ingroup objects
 * Assumes dp.Updates(0) has already been called, and the transform
 * has been set appropriately. This steps through any groups, and looks
 * up an appropriate interface from laidout->interfacepool to draw the data.
 *
 * Note that for groups, a1 and a2 are passed along to all the group members..
 */
void DrawData(Displayer *dp,SomeData *data,anObject *a1,anObject *a2)
{
	dp->PushAndNewTransform(data->m()); // insert transform first
	if (dynamic_cast<Group *>(data)) { // Is a layer or a group
		 //*** perhaps this should rather check whattype==Group?
		Group *g=dynamic_cast<Group *>(data);
		dp->drawaxes();
		for (int c=0; c<g->n(); c++) DrawData(dp,g->e(c),a1,a2);
		dp->PopAxes();
		return;
	} else if (!strcmp(data->whattype(),"SomeDataRef")) {
		data=((SomeDataRef *)data)->thedata;
	} 
	 // find interface in interfacepool
	int c;
	dp->drawaxes();
	for (c=0; c<laidout->interfacepool.n; c++) {
		if (laidout->interfacepool.e[c]->draws(data->whattype())) break;
	}
	if (c<laidout->interfacepool.n) {
		 // draw it, *** this is a little clunky, perhaps have only InterfaceWithDps?
		InterfaceWithDp *idp=dynamic_cast<InterfaceWithDp *>(laidout->interfacepool.e[c]);
		if (idp) idp->DrawDataDp(dp,data,a1,a2);
		else laidout->interfacepool.e[c]->DrawData(data,a1,a2);
	}
	dp->PopAxes();
}

//! Return a local instance of the give type of data.
/*! \ingroup objects
 * This text should be the same as is returned by the object's whattype() function.
 *
 * Currently recognized data are:
 * - SomeData
 * - ImageData
 * - PathsData
 * - GradientData
 * - ColorPatchData
 *
 *   \todo *** there needs to be a mechanism to automate this so new types can be
 *   added on the fly:
 *     objectfactory->new(SOMEDATA_ID);
 *     objectfactory->addtype(cchar *type,newDataFunc,int id);
 *     objectfactory->remove(id/"sometype");
 */
SomeData *newObject(const char *thetype)
{
	if (!strcmp(thetype,"SomeData")) return new SomeData();
	if (!strcmp(thetype,"Group")) return new SomeData();
	if (!strcmp(thetype,"ImageData")) return new ImageData();
	if (!strcmp(thetype,"PathsData")) return new PathsData();
	if (!strcmp(thetype,"GradientData")) return new GradientData();
	if (!strcmp(thetype,"ColorPatchData")) return new ColorPatchData();
	return NULL;
}

//! Return whether all corners of bbox have nonzero winding numbers for points.
/*! \ingroup objects
 * Assumes that you want a closed path, so it automatically checks
 * the segment [firstpoint,lastpoint].
 */
int boxisin(flatpoint *points, int n,DoubleBBox *bbox)
{
	if (!pointisin(points,n,flatpoint(bbox->minx,bbox->miny))) return 0;
	if (!pointisin(points,n,flatpoint(bbox->maxx,bbox->miny))) return 0;
	if (!pointisin(points,n,flatpoint(bbox->maxx,bbox->maxy))) return 0;
	if (!pointisin(points,n,flatpoint(bbox->minx,bbox->maxy))) return 0;
	return 1;
}

//! Return the winding number for p within points. 
/*! \ingroup objects
 * Assumes that you want a closed path, so it automatically checks
 * the segment [firstpoint,lastpoint].
 */
int pointisin(flatpoint *points, int n,flatpoint p)
{
	int w=0;
	flatpoint t1,t2,v;
	t1=points[0]-p;
	double tyx;
	for (int c=1; c<=n; t1=t2, c++) {
		if (c==n) t2=points[0]-p;
			else t2=points[c]-p;
		if (t1.x<0 && t2.x<0) continue;
		if (t1.x>=0 && t2.x>=0) {
			if (t1.y>0 && t2.y<=0) { w++; continue; }
			else if (t1.y<=0 && t2.y>0) { w--; continue; }
		}
		if (!(t1.y>0 && t2.y<=0 || t1.y<=0 && t2.y>0)) continue;
		v=t2-t1;
		tyx=t1.y/t1.x;
		if (t1.x<=0) { // note that this block looks identical to next block
			if (t1.y>0) { //-+ to +-
				if (v.y/v.x>=tyx) w++;
				continue;
			} else { //-- to ++
				if (v.y/v.x<tyx) w--;
				continue;
			}
		} else {
			if (t1.y>0) { //++ to --
				if (v.y/v.x>=tyx) w++;
				continue;
			} else { //+- to -+
				if (v.y/v.x<tyx) w--;
				continue;
			}
		}
	}
	return w;
}

