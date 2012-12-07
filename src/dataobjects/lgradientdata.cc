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

#include "lgradientdata.h"
#include "datafactory.h"




/*! \class LGradientData 
 * \brief Subclassing LaxInterfaces::GradientData
 */



LGradientData::LGradientData(LaxInterfaces::SomeData *refobj)
{
}

LGradientData::~LGradientData()
{
}

void LGradientData::FindBBox()
{
	GradientData::FindBBox();
}

/*! Provide final pointin() definition.
 */
int LGradientData::pointin(flatpoint pp,int pin)
{
	return GradientData::pointin(pp,pin);
}

void LGradientData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		GradientData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	GradientData::dump_out(f,indent+2,what,context);
}

void LGradientData::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			GradientData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) GradientData::dump_in_atts(att,flag,context);
}

LaxInterfaces::SomeData *LGradientData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LGradientData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=LaxInterfaces::somedatafactory->newObject("GradientData");
	GradientData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}


