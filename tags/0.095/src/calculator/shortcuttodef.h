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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef SHORTCUTTODEF_H
#define SHORTCUTTODEF_H


#include <lax/shortcuts.h>
#include "values.h"

namespace Laidout {

//Convenience glue for Laxkit shortcuts of windows and interfaces:
ObjectDef *ShortcutsToObjectDef(Laxkit::ShortcutHandler *sc, ObjectDef *def);

} // namespace Laidout

#endif


