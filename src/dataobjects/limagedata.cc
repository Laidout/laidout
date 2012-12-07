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

#include "limagedata.h"
#include "datafactory.h"


//------------------------------- LImageData ---------------------------------------
/*! \class LImageData
 * \brief Redefined LaxInterfaces::ImageData.
 */

LImageData::LImageData(LaxInterfaces::SomeData *refobj)
{
	//importer=NULL;
	//sourcefile=NULL;
}

LImageData::~LImageData()
{
	//if (importer) importer->dec_count();
	//if (sourcefile) delete[] sourcefile;
}

	

void LImageData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		DrawableObject::dump_out(f,indent,what,context);
		fprintf(f,"%sconfig\n",spc);
		ImageData::dump_out(f,indent+2,what,context);
		return;
	}

	DrawableObject::dump_out(f,indent,what,context);
	fprintf(f,"%sconfig\n",spc);
	ImageData::dump_out(f,indent+2,what,context);
}

/*! If no "config" element, then it is assumed there are no DrawableObject fields.
 */
void LImageData::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	DrawableObject::dump_in_atts(att,flag,context);
	int foundconfig=0;
	for (int c=0; c<att->attributes.n; c++) {
		if (!strcmp(att->attributes.e[c]->name,"config")) {
			foundconfig=1;
			ImageData::dump_in_atts(att->attributes.e[c],flag,context);
		}
	}
	if (!foundconfig) ImageData::dump_in_atts(att,flag,context);
}

LaxInterfaces::SomeData *LImageData::duplicate(LaxInterfaces::SomeData *dup)
{
	if (dup && !dynamic_cast<LImageData*>(dup)) return NULL; //wrong type for referencc object!
	if (!dup) dup=LaxInterfaces::somedatafactory->newObject("ImageData");
	ImageData::duplicate(dup);
	DrawableObject::duplicate(dup);
	return dup;
}

