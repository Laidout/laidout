//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
#ifndef INTERFACES_H
#define INTERFACES_H

#include <lax/lists.h>
#include <lax/interfaces/aninterface.h>

void PushBuiltinPathops();

Laxkit::PtrStack<LaxInterfaces::anInterface> *
GetBuiltinInterfaces(Laxkit::PtrStack<LaxInterfaces::anInterface> *existingpool); //existingpool=NULL

#endif
	
