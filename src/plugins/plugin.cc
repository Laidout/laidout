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


#include <dlfcn.h>

#include "plugin.h"

#include <lax/strmanip.h>


#include <iostream>
#define DBG

using namespace std;



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



//------------------------------ Plugin load/unload ------------------------------

typedef PluginBase *GetPluginFunc();

/*! Return 0 for successful load.
 * Return -1 for already loaded.
 * Return >0 for error, and not loaded.
 */
//int LaidoutApp::Load(const char *path_to_plugin)
//
/*! You will need to dec_count the returned plugin.
 */
PluginBase *LoadPlugin(const char *path_to_plugin)
{
	PluginBase *plugin = NULL;
	void *handle = NULL;

	try {
		//if (!IS_REG(file_exists(path_to_plugin, 1, NULL))) throw 1;

		 //check for plugin already exists
//		for (int c=0; c<plugins.n; c++) {
//			if (!strcmp(plugins.e[c]->Path(), path_to_plugin)) return -1;
//		}
	

		handle = dlopen(path_to_plugin, RTLD_LAZY);
		//handle = dlopen(path_to_plugin, RTLD_NOW);
		//handle = dlopen(path_to_plugin, RTLD_LAZY|RTLD_GLOBAL);

		DBG cerr <<"dl opened..."<<endl;

		if (!handle) throw 2;

//		for (int c=0; c<plugins.n; c++) {
//			if (plugins.e[c]->handle == handle) return -1;
//		}

		GetPluginFunc *GetPlugin; //dl_iterate_phdr
		GetPlugin = (GetPluginFunc*)dlsym(handle, "GetPlugin");
		if (!GetPlugin) throw 3;

		DBG cerr <<"dl GetPlugin found..."<<endl;

		plugin = GetPlugin();
		if (!plugin) throw 4;

		plugin->handle = handle;

		plugin->Initialize();
		//plugins.push(plugin);
		//plugin->dec_count();

	} catch(int error) {
		char *err = newstr(dlerror());

		if (plugin) plugin->dec_count();
		if (handle) dlclose(handle);

		if (err) {
			cerr << "some kind of error: "<<error<<", "<<err << endl;
			delete[] err;
		} else {
			cerr << "dl error: "<<error<<endl;
		}
		return NULL;
	}

	return plugin;
}

//int LaidoutApp::UnLoad(PluginBase *plugin)
//{
//	// Remove all object refs belonging to this plugin
//	***
//
//	plugin->Unload();
//}


//------------------------------ PluginBase ------------------------------

PluginBase::PluginBase()
{
	handle = NULL;
	initialized = 0;
}

PluginBase::~PluginBase()
{
	if (handle) dlclose(handle);
}


/*! Default just set initialized to 1.
 */
int PluginBase::Initialize()
{
	if (initialized) return 0;
	initialized = 1;
	return 0;
}



} // namespace Laidout

