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
#include <lax/transformmath.h>

#include <lax/interfaces/somedataref.h>
#include "drawdata.h"
#include "laidout.h"

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 

//! Push axes and transform by m, draw data, pop axes.
/*! \ingroup objects
 * *** uh, this is unnecessary? the other one does this already....
 * should it not??
 */
void DrawData(Displayer *dp,double *m,SomeData *data,anObject *a1,anObject *a2,unsigned int flags)
{
	dp->PushAndNewTransform(m);
	DrawData(dp,data,a1,a2,flags);
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
void DrawData(Displayer *dp,SomeData *data,anObject *a1,anObject *a2,unsigned int flags)
{
	dp->PushAndNewTransform(data->m()); // insert transform first
	if (flags&DRAW_AXES) dp->drawaxes();
	if (flags&DRAW_BOX && data->validbounds()) {
		dp->NewFG(128,128,128);
		//DBG cout<<" drawing obj "<<data->object_id<<":"<<data->minx<<' '<<data->maxx<<' '<<data->miny<<' '<<data->maxy<<endl;
		dp->drawrline(flatpoint(data->minx,data->miny),flatpoint(data->maxx,data->miny));
		dp->drawrline(flatpoint(data->maxx,data->miny),flatpoint(data->maxx,data->maxy));
		dp->drawrline(flatpoint(data->maxx,data->maxy),flatpoint(data->minx,data->maxy));
		dp->drawrline(flatpoint(data->minx,data->maxy),flatpoint(data->minx,data->miny));
	}
	if (dynamic_cast<Group *>(data)) { // Is a layer or a group
		 //*** perhaps this should rather check whattype==Group?
		Group *g=dynamic_cast<Group *>(data);
		for (int c=0; c<g->n(); c++) DrawData(dp,g->e(c),a1,a2,flags);
		dp->PopAxes();
		return;
	} else if (!strcmp(data->whattype(),"SomeDataRef")) {
		data=((SomeDataRef *)data)->thedata;
		if (data) DrawData(dp,data,a1,a2,flags);
		dp->PopAxes();
		return;
	} 
	 // find interface in interfacepool
	int c;
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

//! Return a local instance of the give type of data (has count of one).
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


//! Return a list of points corresponding to the area.
/*! \ingroup objects
 * Converts a a group of PathsData, a SomeDataRef to a PathsData, 
 * or a single PathsData to a poly-line list of flatpoints. The union
 * of the returned lists is the area corresponding to outline.
 * If extra_m is not NULL, then apply this transform to the points.
 *
 * Non-PathsData elements in a group does not break the finding.
 * Those extra objects are just ignored.
 *
 * Returns the number of single paths interpreted, or negative number for error.
 *
 * If iscontinuing!=0, then ***.
 *
 * \todo *** currently, uses all points (vertex and control points)
 * in the paths as a polyline, not as the full curvy business 
 * that PathsData are capable of. when ps output of paths is 
 * actually more implemented, this will change..
 */
Region GetRegionFromPaths(LaxInterfaces::SomeData *outline, double *extra_m)
{
	PathsData *path=dynamic_cast<PathsData *>(outline);

	 //If is not a path, but is a reference to a path
	if (!path && dynamic_cast<SomeDataRef *>(outline)) {
		SomeDataRef *ref;
		 // skip all nested SomeDataRefs
		do {
			ref=dynamic_cast<SomeDataRef *>(outline);
			if (ref) outline=ref->thedata;
		} while (ref);
		if (outline) path=dynamic_cast<PathsData *>(outline);
	}

	int n=0; //the number of objects interpreted and that have non-empty paths
	Region region=0,region2=0,region3=0; //in Xlib currently, Regions are pointers...
	
	 // If is not a path, and is not a ref to a path, but is a group,
	 // then check its elements 
	if (!path && dynamic_cast<Group *>(outline)) {
		Group *g=dynamic_cast<Group *>(outline);
		SomeData *d;
		double m[6];
		for (int c=0; c<g->n(); c++) {
			d=g->e(c);
			 //add transform of group element
			if (extra_m) transform_mult(m,d->m(),extra_m);
			else transform_copy(m,d->m());
			region2=GetRegionFromPaths(d,m);
			if (!XEmptyRegion(region2)) {
				if (!region3) region3=XCreateRegion();
				if (!region)   region=XCreateRegion();
				XUnionRegion(region2,region,region3);
				if (region) { XDestroyRegion(region); region=0; }
				XDestroyRegion(region2); region2=0;
				region=region3;
				region3=NULL;
			}
		}
	}
	
	if (!path) {
		if (!region) region=XCreateRegion();
		return region;
	}
	
	 // finally append to clip path
	Coordinate *start,*p;
	int np,maxp=0;
	XPoint *points=NULL;
	flatpoint pp;
	int c,c2;
	for (c=0; c<path->paths.n; c++) {
		np=0;
		start=p=path->paths.e[c]->path;
		if (!p) continue;
		do { p=p->next; np++; } while (p && p!=start);
		if (p==start) { // only include closed paths
			if (np>maxp) {
				if (points) delete[] points; 
				maxp=np;
				points=new XPoint[maxp];
			}
			n++;
			c2=0;
			do {
				if (extra_m) pp=transform_point(extra_m,p->p());
					else pp=p->p();
				points[c2].x=(int)pp.x;
				points[c2].y=(int)pp.y;
					
				p=p->next;	
				c2++;
			} while (p && p!=start);
			if (!region)   region=XCreateRegion();
			region2=XPolygonRegion(points,np,WindingRule);
			if (!region3) region3=XCreateRegion();
			XUnionRegion(region,region2,region3);
			XDestroyRegion(region);
			XDestroyRegion(region2); region2=0;
			region=region3;
			region3=NULL;
		}
	}
	if (points) delete[] points;
	
	if (!region) region=XCreateRegion();
	return region;
}

////! Create an X region from one or more closed paths.
///*! This region can then become a window's clipping area
// * with a call to XSetRegion().
// */
//int GetRegionFromPaths(SomeData *outline,double *extra_m,Region *region)
//{
//	PtrStack<flatpoint> list(2);
//	GetAreaPath(outline,extra_m,&list,NULL);
//	if (!list.n) return 0;
//	Region r2=XCreateRegion(),r3;
//	int maxp=list.e[0]->***;
//	XPoint *points=new XPoint[maxp];
//	int c,c2;
//	for (c=0; c<list.n; c++) {
//		if (list.e[c]->*** >maxp) {
//			delete[] points;
//			points=new XPoint[maxp];
//		}
//		for (c2=0; c2<***; c2++) {
//			points[c2].x=(int)list.e[c][c2].x;
//			points[c2].y=(int)list.e[c][c2].y;
//		}
//		r3=XPolygonRegion(points,***,WindingRule);
//		XUnionRegion(r2,r3,r1);
//		XDestroyRegion(r2);
//		XDestroyRegion(r3);
//		r2=r1;
//	}
//	*region=r1;
//	return 0;
//}
//
////! Return a list of points corresponding to the area.
///*! \ingroup objects
// * Converts a a group of PathsData, a SomeDataRef to a PathsData, 
// * or a single PathsData to a poly-line list of flatpoints. The union
// * of the returned lists is the area corresponding to outline.
// * If extra_m is not NULL, then apply this transform to the points.
// *
// * Non-PathsData elements in a group does not break the finding.
// * Those extra objects are just ignored.
// *
// * Returns the number of single paths interpreted, or negative number for error.
// *
// * If iscontinuing!=0, then ***.
// *
// * \todo *** currently, uses all points (vertex and control points)
// * in the paths as a polyline, not as the full curvy business 
// * that PathsData are capable of. when ps output of paths is 
// * actually more implemented, this will change..
// */
//int GetAreaPath(LaxInterfaces::SomeData *outline, 
//		double *extra_m,
//		PtrStack<flatpoint> **list,
//		int *n_ret)//iscontinuing=0
//{***
//	if (!list) return -1;
//	if (!*list) *list=new PtrStack<flatpoint>(2);
//
//	PathsData *path=dynamic_cast<PathsData *>(outline);
//
//	 //If is not a path, but is a reference to a path
//	if (!path && dynamic_cast<SomeDataRef *>(outline)) {
//		SomeDataRef *ref;
//		 // skip all nested SomeDataRefs
//		do {
//			ref=dynamic_cast<SomeDataRef *>(outline);
//			if (ref) outline=ref->thedata;
//		} while (ref);
//		if (outline) path=dynamic_cast<PathsData *>(outline);
//	}
//
//	int n=0; //the number of objects interpreted and that have non-empty paths
//	
//	 // If is not a path, and is not a ref to a path, but is a group,
//	 // then check its elements 
//	if (!path && dynamic_cast<Group *>(outline)) {
//		***
//		Group *g=dynamic_cast<Group *>(outline);
//		SomeData *d;
//		double m[6];
//		for (int c=0; c<g->n(); c++) {
//			d=g->e(c);
//			 //add transform of group element
//			GetAreaPath(d,d->m(),list,*** n_ret);
//		}
//	}
//	
//	if (!path) return n;
//	
//	 // finally append to clip path
//	Coordinate *start,*p;
//	for (int c=0; c<path->paths.n; c++) {
//		start=p=path->paths.e[c]->path;
//		if (!p) continue;
//		do { p=p->next; } while (p && p!=start);
//		if (p==start) { // only include closed paths
//			n++;
//			do {
//				if (extra_m) pp=transform_point(extra_m,p->p());
//					else pp=p->p();
//				**** add pp to list
//				p=p->next;	
//			} while (p && p!=start);
//			list.push(plist.extractArray
//		}
//	}
//	
//	if (n && !iscontinuing) fprintf(f,"clip\n");
//	return n;
//}
