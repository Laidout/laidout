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
// Copyright (C) 2009 by Tom Lechner
//
#ifndef OPENANDNEW_H
#define OPENANDNEW_H

#include "../calculator/values.h"


namespace Laidout {


ObjectDef *makeNewDocumentObjectDef();
int NewDocumentFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 Laxkit::ErrorLog &log);



ObjectDef *makeOpenObjectDef();
int OpenFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 Laxkit::ErrorLog &log);


} // namespace Laidout

#endif 


