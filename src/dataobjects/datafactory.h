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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef DATAFACTORY_H
#define DATAFACTORY_H


#include <lax/interfaces/somedatafactory.h>
#include <lax/resources.h>

namespace Laidout {



enum LaidoutDataObjects {
	LO_MYSTERYDATA = LaxInterfaces::LAX_DATA_MAX,
	LO_PLAINTEXT,

	LO_DATA_MAX
};

void InitializeDataFactory();

void InitializeResourceManager(Laxkit::ResourceManager *resourcemanager);



} //namespace Laidout

#endif

