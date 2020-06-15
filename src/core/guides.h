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
#ifndef GUIDES_H
#define GUIDES_H


#include <lax/anobject.h>
#include <lax/dump.h>
#include "../dataobjects/drawableobject.h"


namespace Laidout {


class DrawableObject;

//----------------------------------------- PointAnchor -------------------------------------
enum PointAnchorTypes {
    PANCHOR_BBox,
    PANCHOR_Absolute,
    PANCHOR_Segment,
    PANCHOR_Line,
    PANCHOR_MAX
};

class PointAnchor : public Laxkit::anObject,
					public LaxFiles::DumpUtility
{
  public:
    char *name;
	int id;

	Laxkit::anObject *owner;
	unsigned int owner_id;

	Laxkit::ScreenColor color, hcolor;
    flatpoint p, p2;
    int anchor_type; //alignment point in bounding box in owner,
                     //or absolute coordinate in owner
                     //or segment of absolute points in owner
                     //or infinite line

	PointAnchor();
    PointAnchor(const char *nname, int type, flatpoint pp1,flatpoint pp2,int nid);
    virtual ~PointAnchor();
    virtual const char *whattype() { return "PointAnchor"; }
    virtual void Set(const char *nname, int type, flatpoint pp1,flatpoint pp2,int nid);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//----------------------------------------- Guide -------------------------------------
class Guide : virtual public LaxInterfaces::SomeData
{
  public:
	Guide() {}
	virtual ~Guide() {}
};


//----------------------------------------- GridGuide -------------------------------------
class GridGuide : public Guide
{
  public:
	enum GridType {
		Grid,
		Horizontal,
		Vertical,
		Triangles,
		Hex,
		MAX
	};
	GridType            gridtype;
	bool                enabled;
	bool                visible;
	flatpoint           offset;
	flatvector          majordir, minordir;
	int                 spacingunits;
	double              xspacing, yspacing;
	Laxkit::ScreenColor minorcolor, majorcolor;
	int                 majorinterval;  // draw thick every this number
	bool                snaponlytovisible;

	GridGuide();
	virtual ~GridGuide();
	virtual const char *whattype() { return "GridGuide"; }
	virtual flatpoint Snap(flatpoint p);
	virtual int SetUnits(const char *units);
	virtual void dump_out(FILE *f, int indent, int what, LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att, int flag, LaxFiles::DumpContext *context);
};

} // namespace Laidout

#endif

