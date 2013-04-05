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
#ifndef GUIDES_H
#define GUIDES_H


#include <lax/anobject.h>
#include <lax/dump.h>


namespace Laidout {


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
	Laxkit::ScreenColor color, hcolor;
    flatpoint p, p2;
    int anchor_type; //alignment point in bounding box,
                     //or absolute coordinate
                     //or segment
                     //or infinite line
    PointAnchor(const char *nname, int type, flatpoint pp1,flatpoint pp2);
    virtual ~PointAnchor();
    virtual const char *whattype() { return "PointAnchor"; }
    virtual void Set(const char *nname, int type, flatpoint pp1,flatpoint pp2);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


} // namespace Laidout

#endif

