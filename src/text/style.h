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
// Copyright (C) 2025-present by Tom Lechner
//
#ifndef _LO_STYLE_H
#define _LO_STYLE_H


#include <lax/resources.h>
#include "../calculator/values.h"


namespace Laidout {


//------------------------------ Style ---------------------------------

class Style : public ValueHash, public Laxkit::Resourceable
{
  public:
    Style *parent; // if non-null, this MUST be EITHER a project resource OR a temporary StreamElement owned Style

    Style();
    Style(const char *new_name);
    virtual ~Style();
    virtual const char *whattype() { return "Style"; }

    virtual Value *FindValue(int attribute_name);
    virtual int MergeFromMissing(Style *s); //only from s not in *this
    virtual int MergeFrom(Style *s); //all in s override *this
    virtual Style *Collapse();

	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,Laxkit::DumpContext *context);
};

} // namespace Laidout


#endif