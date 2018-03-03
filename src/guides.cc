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

#include "guides.h"
#include <lax/strmanip.h>
#include <lax/units.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/pathinterface.h>


using namespace Laxkit;
//using namespace LaxInterfaces;


namespace Laidout {


//----------------------------------------- PointAnchor -------------------------------------

/*! \class PointAnchor 
 * A named anchor, optionally aligned to a bounding box, or absolute coordinates.
 * Can be a point, or a segment, or a line. See PointAnchorTypes.
 */

PointAnchor::PointAnchor()
{
	id=getUniqueNumber();
	name=NULL;
	anchor_type=PANCHOR_Absolute;
	owner=NULL;
}

PointAnchor::PointAnchor(const char *nname, int type, flatpoint pp1,flatpoint pp2, int nid)
{
	if (nid<=0) nid=getUniqueNumber();
	id=nid;
    name=newstr(nname);
    anchor_type=type;
    p=pp1;
    p2=pp2;
	color.rgb(0.,0.,1.);
	hcolor.rgb(1.,0.,0.);

	owner=NULL;
	owner_id=0;
}

PointAnchor::~PointAnchor()
{
	// *** assume mechanism in place to remove anchors before removing owner objects
    if (name) delete[] name;
}

/*! Sets id only when nid>=0. If nid==0, then set a new number for id.
 */
void PointAnchor::Set(const char *nname, int type, flatpoint pp1,flatpoint pp2, int nid)
{
	if (nid==0) nid=getUniqueNumber();
    makestr(name,nname);
    anchor_type=type;
    p=pp1;
    p2=pp2;
}

void PointAnchor::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to implement PathGuide::dump_out"<<endl;
}

void PointAnchor::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to implement PathGuide::dump_in_atts"<<endl;
}


//-------------------------------------- PathGuide -------------------------------------
/*! \class PathGuide
 * \brief Things snap to guides.
 *
 * Guides in most programs are either vertical or horizontal lines. Guides in Laidout
 * can be any arbitrary single path. PathGuide objects are used in a viewer to snap objects to them,
 * and they are also used as tabstops and margin indicators in text objects.
 * 
 * A guide can lock its rotation, scale, or position to be the constant relative to its containing
 * object, which might be an object, a page, a scratch space, or the viewport itself. 
 *
 * This class is for 1-dimensional guides. See class Arrangement for 2-d "guides".
 */
/*! \var unsigned int PathGuide::guidetype
 * \brief The type of guide.
 *
 * <pre>
 *  (1<<0) Lock orientation
 *  (1<<2) Repeat the path after endpoints, if any
 *  (1<<3) Extend flat lines with tangent at endpoints if any.
 *  (1<<4) attract objects horizontally
 *  (1<<5) attract objects vertically
 *  (1<<6) attract objects perpendicularly
 * </pre>
 */
class PathGuide : public LaxInterfaces::SomeData, public LaxFiles::DumpUtility
{
 public:
	unsigned int guidetype;
	char *name;
	LaxInterfaces::PathsData *guide;
	Laxkit::ScreenColor color, hcolor;

	PathGuide();
	virtual ~PathGuide();
	virtual const char *whattype() { return "PathGuide"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

PathGuide::PathGuide()
{
	name=NULL;
	guidetype=0;
	guide=NULL;
	color.rgb(0.,0.,1.);
	hcolor.rgb(1.,0.,0.);
}

PathGuide::~PathGuide()
{
	if (name) delete[] name;
	if (guide) guide->dec_count();
}

void PathGuide::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to implement PathGuide::dump_out"<<endl;
}

void PathGuide::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to implement PathGuide::dump_in_atts"<<endl;
}



//------------------------------------- Grid -----------------------------------------------------

/*! \class Grid
 * Info about a grid guide.
 */
class Grid : public LaxFiles::DumpUtility
{
  public:
	int gridtype; //0=rect, 1=triangles, 2=hex?, others?
	char enabled, visible;
	flatpoint offset;
	flatvector majordir, minordir;
	int spacingunits;
	double xspacing, yspacing;
	Laxkit::ScreenColor color, majorcolor;
	int majorinterval; //draw thick every this number

	Grid();
	virtual ~Grid();
	virtual const char *whattype() { return "Grid"; }
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

Grid::Grid()
  : majordir(1,0), minordir(0,1)
{
	gridtype=0;
	enabled=visible=1;
	xspacing=yspacing=.5;
	spacingunits=UNITS_Inches;

	color.rgb(0.,0.,1.);
	majorcolor.rgb(0.,0.,.8);
	majorinterval=5;
}

Grid::~Grid()
{}


void Grid::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to implement Grid::dump_out"<<endl;
}

void Grid::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to implement Grid::dump_in_atts"<<endl;
}


} // namespace Laidout

