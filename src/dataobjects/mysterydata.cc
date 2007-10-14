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
// Copyright (C) 2007 by Tom Lechner
//

//--------------------------------------- MysteryData ---------------------------------

#include <lax/strmanip.h>
#include "mysterydata.h"

using namespace LaxFiles;

//namespace Laidout {

/*! \class MysteryData
 * \brief Holds fragments of objects of an imported file.
 *
 * This class facilitates preserving data from files of non-native formats, so that
 * when a Laidout document is exported to the same format, those objects that Laidout
 * doesn't understand can still be exported. Other formats will generally ignore the
 * MysteryData.
 */


MysteryData::MysteryData(const char *gen)
{
	generator=newstr(gen);
}

MysteryData::~MysteryData()
{
	if (generator) delete[] generator;
}

//! Move the att to this->attributes.
/*! This object takes possession of att. The calling code should not delete att.
 * If this->attributes existed, it is deleted.
 *
 * If att==NULL, then this->attributes becomes NULL.
 */
int MysteryData::installAtts(LaxFiles::Attribute *att)
{
	if (attributes) delete attributes;
	attributes=att;
	return 0;
}


//} //namespace Laidout

