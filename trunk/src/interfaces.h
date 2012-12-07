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
// Copyright (C) 2004-2009 by Tom Lechner
//
#ifndef INTERFACES_H
#define INTERFACES_H

#include <lax/lists.h>
#include <lax/interfaces/aninterface.h>


namespace Laidout {


void PushBuiltinPathops();

Laxkit::RefPtrStack<LaxInterfaces::anInterface> *
GetBuiltinInterfaces(Laxkit::RefPtrStack<LaxInterfaces::anInterface> *existingpool); //existingpool=NULL

} //namespace Laidout

#endif
	
