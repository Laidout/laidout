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
#ifndef PAPERSIZES_H
#define PAPERSIZES_H


#define LAX_LISTS_H_ONLY
#include <lax/lists.h>
#undef LAX_LISTS_H_ONLY

#include "styles.h"

class PaperType : public Style
{
 public:
	char *name;
	double width,height;
	int dpi;
	unsigned int flags; //1=landscape !(&1)=portrait
	PaperType(const char *nname,double ww,double hh,unsigned int f,int ndpi);
	virtual double w() { if (flags&1) return height; else return width; }
	virtual double h() { if (flags&1) return width; else return height; }
	virtual Style *duplicate(Style *s=NULL);

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

Laxkit::PtrStack<PaperType> *GetBuiltinPaperSizes(Laxkit::PtrStack<PaperType> *papers);


#endif
