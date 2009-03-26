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
// Copyright (C) 2009 by Tom Lechner
//


********this file is not active yet!!!*****************

/*! \ingroup plugins
 *
 * Plugins can provide:
 * 
 * main window panes
 * commands (possible return value, optional parameter dialog)
 * random dialogs (command with optional context)
 * image import filters
 * import filters
 * export filters
 * interfaces and possibly new data types
 * impositions
 * resources, like page sizes, palettes, imposition instance templates, etc
 * icons/menu items
 *  particular templates/types of data, master pages for instance
 * interpreters (python, yacas, octave, tex/latex?)
 * 
 * 
 */


class PluginBase
{
 public:
	const char *PluginName() = 0;
	const char *Description() = 0;
	const char *Author() = 0;
	const char *License() = 0;
	const char *Version() = 0;
	unsigned long WhatYouGot();

	LaidoutDialog     **WindowPanes();
	LaidoutAction     **Actions();
	ImageImportFilter **ImageImportFilters();
	ImportFilter      **ImportFilters();
	ExportFilter      **ExportFilters();
	InterfaceWithDp   **Tools();
	Imposition        **Impositions();
	Resource          **ResourceInstances();
	DrawableObject    **ObjectInstances();
	Config            **Configs();
	Interpreter       **Interpreters();
	
	PluginBase();
	virtual ~PluginBase();
};



