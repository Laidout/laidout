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
#ifndef DATAFACTORY_H
#define DATAFACTORY_H


#include <lax/interfaces/somedatafactory.h>


namespace Laidout {



const int LO_GROUP       =10000; //note: this must be greater than all Laxkit object ids
const int LO_MYSTERYDATA =10001;

void InitializeDataFactory();




} //namespace Laidout

#endif

