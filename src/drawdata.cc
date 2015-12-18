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
// Copyright (c) 2004-2011 Tom Lechner
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
#include <lax/interfaces/somedatafactory.h>
#include "drawdata.h"
#include "dataobjects/mysterydata.h"
#include "laidout.h"
#include "language.h"
//#include "dataobjects/datafactory.h"


// DBG !!!!!
#include <lax/displayer-cairo.h>


using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {

//! Push axes and transform by m, draw data, pop axes.
/*! \ingroup objects
 * *** uh, this is unnecessary? the other one does this already....
 * should it not??
 */
//void DrawData(Displayer *dp,double *m,SomeData *data,anObject *a1,anObject *a2,unsigned int flags)
//{
//	dp->PushAndNewTransform(m);
//	DrawData(dp,data,a1,a2,flags);
//	dp->PopAxes();
//}

//! Just like DrawData(), but don't push data matrix.
void DrawDataStraight(Displayer *dp,SomeData *data,anObject *a1,anObject *a2,unsigned int flags)
{
	//DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(dp);
    //DBG if (ddp && ddp->GetCairo()) cerr <<" DrawDataStraight for "<<data->Id()<<", cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;


	if (flags&DRAW_AXES) dp->drawaxes();
	if (flags&DRAW_BOX && data->validbounds()) {
		dp->NewFG(128,128,128);
		//DBG cerr<<" drawing obj "<<data->object_id<<":"<<data->minx<<' '<<data->maxx<<' '<<data->miny<<' '<<data->maxy<<endl;
		dp->drawline(flatpoint(data->minx,data->miny),flatpoint(data->maxx,data->miny));
		dp->drawline(flatpoint(data->maxx,data->miny),flatpoint(data->maxx,data->maxy));
		dp->drawline(flatpoint(data->maxx,data->maxy),flatpoint(data->minx,data->maxy));
		dp->drawline(flatpoint(data->minx,data->maxy),flatpoint(data->minx,data->miny));
	}

	if (dynamic_cast<DrawableObject*>(data) && dynamic_cast<DrawableObject*>(data)->n()) {
		DrawableObject *g=dynamic_cast<DrawableObject *>(data);
		for (int c=0; c<g->n(); c++) DrawData(dp,g->e(c),a1,a2,flags);

		if (!strcmp(data->whattype(),"Group")) {
			// Is explicitly a layer or a group, so we are done drawing!
			return;
		}
	}
	
	 //special treatment for clones
	if (!strcmp(data->whattype(),"SomeDataRef")) {
		SomeDataRef *ref=dynamic_cast<SomeDataRef *>(data);
		data=ref->thedata;
		if (data) {
			//dp->PushAndNewTransform(ref->m()); // insert transform first
			DrawDataStraight(dp,data,a1,a2,flags);
			//dp->PopAxes();
		}
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
		 // draw it
		interf->DrawDataDp(dp,data,a1,a2);
		
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
				dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
				dp->LineWidthScreen(1);
				dp->drawbez(mdata->outline,mdata->numpoints/3,
							mdata->outline[0]==mdata->outline[mdata->numpoints-1],0);
			}
		
		} else {
			flatpoint fp;
			fp=dp->realtoscreen(flatpoint((data->maxx+data->minx)/2,(data->maxy+data->miny)/2));
			dp->DrawScreen();
			dp->textout((int)fp.x,(int)fp.y,_("unknown"),-1);
			dp->DrawReal();
		}


		 //draw box around it
		dp->NewFG(0,0,255);
		dp->LineAttributes(-1,LineSolid,CapButt,JoinMiter);
		flatpoint ul=dp->realtoscreen(flatpoint(data->minx,data->miny)), 
				  ur=dp->realtoscreen(flatpoint(data->maxx,data->miny)), 
				  ll=dp->realtoscreen(flatpoint(data->minx,data->maxy)), 
				  lr=dp->realtoscreen(flatpoint(data->maxx,data->maxy));
		dp->DrawScreen();
		dp->LineWidthScreen(2);
		dp->drawline(ul,ur);
		dp->drawline(ur,lr);
		dp->drawline(lr,ll);
		dp->drawline(ll,ul);
		dp->DrawReal();
	}
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
	DrawDataStraight(dp,data,a1,a2,flags);
	dp->PopAxes();
}

//! Return a local instance of the give type of data (has count of one).
/*! \ingroup objects
 * This text should be the same as is returned by the object's whattype() function.
 *
 * See src/dataobjects/datafactory.cc for currently allowed object types.
 */
SomeData *newObject(const char *thetype)
{
	return dynamic_cast<SomeData*>(somedatafactory()->NewObject(thetype));
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

			dp->DrawScreen();
			dp->Clip(points,np,1);
			dp->DrawReal();
			n++;
		}
	}
	if (points) delete[] points;
	
	return n;
}



} // namespace Laidout



