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
// Copyright (C) 2016 by Tom Lechner
//

#include "objectfilter.h"

#include <lax/refptrstack.cc>


namespace Laidout {



//----------------------------- ObjectFilter ---------------------------------
/*! \class ObjectFilter
 * \brief Class that modifies any DrawableObject somehow.
 *
 * This could be blur, contrast, saturation, distort, etc. 
 *
 * This could also be a adapted to be a dynamic
 * filter that depends on some resource, such as a global integer resource
 * representing the current frame, that might
 * adjust an object's matrix based on keyframes, for instance.
 *
 * Every object can have any number of filters applied to it. Filters behave in
 * a manner similar to svg filters. They can specify the input source(s), and output target,
 * which can then be the input of another filter.
 *
 * \todo it would be nice to support all the built in svg filters, and additionally
 *   image warping as a filter.
 */


ObjectFilter::ObjectFilter()
{
	filtername=NULL;
}

ObjectFilter::~ObjectFilter()
{
	if (filtername) delete[] filtername;
}


} //namespace Laidout
