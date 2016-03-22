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


#include "plugin.h"


namespace Laidout {

/*! \ingroup plugins
 *
 * Plugins can provide:
 * 
 * main window panes
 * commands (possible return value, optional parameter dialog)
 * random dialogs (command with optional context)
 * image import filters (todo)
 * import filters
 * export filters
 * interfaces and possibly new data types
 * impositions
 * resources, like page sizes, palettes, imposition instance templates, etc
 * particular templates/types of data, master pages for instance
 * interpreters (python, yacas, octave, tex/latex?  -> investigate swig)
 * 
 */


PluginBase::PluginBase()
{
}

PluginBase::~PluginBase()
{
}


/*! Return a ResourceType with "tools" and "objects" resource groups.
 * If "tools" and "objects" don't exist, then it is assumed only tools are present.
 *
 * For tools, you must specify in what window types they should appear. Do this in
 * a "usein" attribute in resource->meta.
 */
Laxkit::ResourceType **PluginBase::Tools()
{
	return NULL;
}



} // namespace Laidout

