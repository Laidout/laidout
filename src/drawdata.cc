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
// Copyright (c) 2004-2010 Tom Lechner
//


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
#include <lax/interfaces/imagepatchinterface.h>
#include <lax/transformmath.h>

#include <lax/interfaces/somedataref.h>
#include "drawdata.h"
#include "dataobjects/epsdata.h"
#include "dataobjects/mysterydata.h"
#include "laidout.h"
#include "language.h"

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
 *
 * \todo currently this looks up which interface to draw an object with in LaidoutApp,
 *   but it should first check for suitable one in the relevant viewport.
 */
void DrawData(Displayer *dp,SomeData *data,anObject *a1,anObject *a2,unsigned int flags)
{
	dp->PushAndNewTransform(data->m()); // insert transform first
	if (flags&DRAW_AXES) dp->drawaxes();
	if (flags&DRAW_BOX && data->validbounds()) {
		dp->NewFG(128,128,128);
		DBG cerr<<" drawing obj "<<data->object_id<<":"<<data->minx<<' '<<data->maxx<<' '<<data->miny<<' '<<data->maxy<<endl;
		dp->drawline(flatpoint(data->minx,data->miny),flatpoint(data->maxx,data->miny));
		dp->drawline(flatpoint(data->maxx,data->miny),flatpoint(data->maxx,data->maxy));
		dp->drawline(flatpoint(data->maxx,data->maxy),flatpoint(data->minx,data->maxy));
		dp->drawline(flatpoint(data->minx,data->maxy),flatpoint(data->minx,data->miny));
	}
	if (!strcmp(data->whattype(),"Group")) { // Is a layer or a group
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
	anInterface *interf=NULL;
	//if (dp->GetXw()) ...
	// else:
	for (c=0; c<laidout->interfacepool.n; c++) {
		if (laidout->interfacepool.e[c]->draws(data->whattype())) {
			interf=laidout->interfacepool.e[c];
			break;
		}
	}
	if (interf) {
		 // draw it, *** this is a little clunky, perhaps have only InterfaceWithDps?
		InterfaceWithDp *idp=dynamic_cast<InterfaceWithDp *>(interf);
		if (idp) idp->DrawDataDp(dp,data,a1,a2);
		else laidout->interfacepool.e[c]->DrawData(data,a1,a2);
		
	} else {
		 //mystery data! might be actual MysteryData, or might be some data for which
		 //the interface is somehow unavailable

		 // draw question marks and name if any...
		MysteryData *mdata=dynamic_cast<MysteryData *>(data);
		if (mdata) {

			flatpoint fp;
			if (mdata->name || mdata->importer) {
				char str[(mdata->name?strlen(mdata->name):0)
						  +(mdata->importer?strlen(mdata->importer):0)+2];
				sprintf(str,"%s\n%s", mdata->importer?mdata->importer:"",
									  mdata->name?mdata->name:"");
				fp=dp->realtoscreen(flatpoint((mdata->maxx+mdata->minx)/2,(mdata->maxy+mdata->miny)/2));
				dp->DrawScreen();
				dp->textout((int)fp.x,(int)fp.y,str,-1);
				dp->DrawReal();
			}

			 //draw question marks in random spots
			for (int c=0; c<10; c++) {
				fp=dp->realtoscreen(flatpoint(mdata->minx+(mdata->maxx+mdata->minx)*((double)random()/RAND_MAX),
											  mdata->miny+(mdata->maxy+mdata->miny)*((double)random()/RAND_MAX)));
				dp->DrawScreen();
				dp->textout((int)fp.x,(int)fp.y,"?",1);
				dp->DrawReal();
			}

		 	 //draw outline if any
			if (mdata->numpoints) {
				dp->NewFG(0,0,0);
				dp->LineAttributes(1,LineSolid,CapButt,JoinMiter);
				dp->drawbez(mdata->outline,mdata->numpoints/3,
							mdata->outline[0]==mdata->outline[mdata->numpoints-1],0);
			}
		
		} else {
			flatpoint fp;
			fp=dp->realtoscreen(flatpoint((data->maxx+data->minx)/2,(data->maxy+data->miny)));
			dp->DrawScreen();
			dp->textout((int)fp.x,(int)fp.y,_("unknown"),-1);
			dp->DrawReal();
		}


		 //draw box around it
		dp->NewFG(0,0,255);
		dp->LineAttributes(2,LineSolid,CapButt,JoinMiter);
		flatpoint ul=dp->realtoscreen(flatpoint(data->minx,data->miny)), 
				  ur=dp->realtoscreen(flatpoint(data->maxx,data->miny)), 
				  ll=dp->realtoscreen(flatpoint(data->minx,data->maxy)), 
				  lr=dp->realtoscreen(flatpoint(data->maxx,data->maxy));
		dp->DrawScreen();
		dp->drawline(ul,ur);
		dp->drawline(ur,lr);
		dp->drawline(lr,ll);
		dp->drawline(ll,ul);
		dp->DrawReal();
	}

	dp->PopAxes();
}

//! Return a local instance of the give type of data (has count of one).
/*! \ingroup objects
 * This text should be the same as is returned by the object's whattype() function.
 *
 * Currently recognized data are:
 * - SomeData *** should remove this one?
 * - Group
 * - ImageData
 * - ImagePatchData
 * - PathsData
 * - GradientData
 * - ColorPatchData
 * - EpsData
 * - MysteryData
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
	if (!strcmp(thetype,"Group")) return new Group();
	if (!strcmp(thetype,"ImageData")) return new ImageData();
	if (!strcmp(thetype,"ImagePatchData")) return new ImagePatchData();
	if (!strcmp(thetype,"PathsData")) return new PathsData();
	if (!strcmp(thetype,"GradientData")) return new GradientData();
	if (!strcmp(thetype,"ColorPatchData")) return new ColorPatchData();
	if (!strcmp(thetype,"EpsData")) return new EpsData();
	if (!strcmp(thetype,"MysteryData")) return new MysteryData();
	return NULL;
}

//! Return whether all corners of bbox have nonzero winding numbers for points.
/*! \ingroup objects
 * Assumes that you want a closed path, so it automatically checks
 * the segment [firstpoint,lastpoint].
 */
int boxisin(flatpoint *points, int n,DoubleBBox *bbox)
{
	if (!point_is_in(flatpoint(bbox->minx,bbox->miny),points,n)) return 0;
	if (!point_is_in(flatpoint(bbox->maxx,bbox->miny),points,n)) return 0;
	if (!point_is_in(flatpoint(bbox->maxx,bbox->maxy),points,n)) return 0;
	if (!point_is_in(flatpoint(bbox->minx,bbox->maxy),points,n)) return 0;
	return 1;
}


//! Append clipping paths to dp.
/*! \ingroup objects
 * Converts a a group of PathsData, a SomeDataRef to a PathsData, 
 * or a single PathsData to a clipping path. The final region is just 
 * the union of all the paths there.
 *
 * Non-PathsData elements in a group does not break the finding.
 * Those extra objects are just ignored.
 *
 * Returns the number of single paths interpreted, or negative number for error.
 *
 * \todo *** currently, uses all points (vertex and control points)
 *   in the paths as a polyline, not as the full curvy business 
 *   that PathsData are capable of. when ps output of paths is 
 *   actually more implemented, this will change..
 * \todo this would be good to transplant into laxkit
 */
int SetClipFromPaths(Laxkit::Displayer *dp,LaxInterfaces::SomeData *outline, const double *extra_m)
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

			n+=SetClipFromPaths(dp,d,m);
		}
	}
	
	if (!path) {
		return n;
	}
	
	 // finally append to clip path
	Coordinate *start,*p;
	int np,maxp=0;
	flatpoint *points=NULL;
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
				points=new flatpoint[maxp];
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

			dp->Clip(points,np,1);
			n++;
		}
	}
	if (points) delete[] points;
	
	return n;
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
