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
// Copyright (C) 2013,2017 by Tom Lechner
//


#include <dlfcn.h>

#include "plugin.h"
#include "../language.h"

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

/*! You will need to PluginBase::Initialize() and otherwise dec_count the returned plugin.
 * See LaidoutApp::InitializePlugins() for more.
 */
PluginBase *LoadPlugin(const char *path_to_plugin, Laxkit::ErrorLog &log)
{
	PluginBase *plugin = NULL;
	void *handle = NULL;

	//DBG cerr <<"loading plugin "<<path_to_plugin<<"..."<<endl;

	try {

		handle = dlopen(path_to_plugin, RTLD_LAZY);
		//handle = dlopen(path_to_plugin, RTLD_NOW);
		//handle = dlopen(path_to_plugin, RTLD_LAZY|RTLD_GLOBAL);

		//DBG cerr <<"dl opened..."<<endl;

		if (!handle) throw 1;

//		for (int c=0; c<plugins.n; c++) {
//			if (plugins.e[c]->handle == handle) return -1;
//		}


		GetPluginFunc *GetPlugin; //dl_iterate_phdr
		GetPlugin = (GetPluginFunc*)dlsym(handle, "GetPlugin");
		if (!GetPlugin) throw 4;

		//DBG cerr <<"dl GetPlugin found..."<<endl;

		plugin = GetPlugin();
		if (!plugin) throw 5;

		if (isblank(plugin->PluginName())) throw 6;
		//.... laidout checks for plugin in same name, reject if found

		plugin->handle = handle;

	} catch(int error) {
		char *err = newstr(dlerror());

		if (plugin) plugin->dec_count();
		if (handle) { dlclose(handle); handle = NULL; }

		if (err) {
			cerr << "some kind of dl error: "<<error<<", "<<err << endl;
			log.AddMessage(0, path_to_plugin,_("Plugin"), err, Laxkit::ERROR_Fail);
			delete[] err;

		} else {
			const char *msg = _("Badly formed plugin!");
			if (error == 1) msg = _("Could not load plugin!");
			log.AddMessage(0, path_to_plugin, _("Plugin"), msg, Laxkit::ERROR_Fail);

			cerr << "plugin loading error: "<<error<<endl;
		}
		return NULL;
	}

	makestr(plugin->filepath, path_to_plugin);

	//DBG cerr <<"Found plugin!"<<endl;
	//DBG cerr <<"  Name:        "<< plugin->PluginName()  << endl;
	//DBG cerr <<"  Version:     "<< plugin->Version()<<endl;
	//DBG cerr <<"  Description: "<< plugin->Description() << endl;
	//DBG cerr <<"  Author:      "<< plugin->Author()      << endl;
	//DBG cerr <<"  ReleaseDate: "<< plugin->ReleaseDate() << endl;
	//DBG cerr <<"  License:     "<< plugin->License()     << endl;
	//DBG if (plugin->OtherMeta()) const_cast<LaxFiles::Attribute*>(plugin->OtherMeta())->dump_out_full(stderr, 2);

	return plugin;
}

/*! Call this when you are all done with the plugin, meaning
 * ALL references to things the plugin uses have been deallocated.
 * delete is called on plugin, and dlclose() for its dl handle after delete completes.
 */
int DeletePlugin(PluginBase *plugin)
{
	void *handle = plugin->handle;
	delete plugin;
	dlclose(handle);
	return 0;
}

//int LaidoutApp::UnLoad(PluginBase *plugin)
//{
//	// Remove all object refs belonging to this plugin
//	***
//
//	plugin->Unload();
//}


//------------------------------ PluginBase ------------------------------

/*! \class PluginBase
 *
 * Base class for any plugins. C++ plugins are dynamically loaded, and 
 * will define a function
 * <tt>PluginBase *GetPlugin()</tt>
 * that returns a PluginBase object. Once returned, its Initialize() function
 * is called, which is supposed to set up everything it needs. This may
 * change in the future, as this is not terribly good security wise.
 */


/*! Reminder: you should not initialize anything important in the constructor.
 * Instead do that in Initialize().
 */
PluginBase::PluginBase()
{
	filepath = NULL;
	handle = NULL;
	initialized = 0;
}

PluginBase::~PluginBase()
{
	//DBG cerr << "destructor plugin: "<<filepath<<endl;
	delete[] filepath;
	//if (handle) dlclose(handle); <- note: can't do this here, since *this is allocated within the dl!
}


/*! Default just set initialized to 1.
 */
int PluginBase::Initialize()
{
	if (initialized) return 0;
	initialized = 1;
	return 0;
}

/*! This should be the reverse of Initialize().
 */
void PluginBase::Finalize()
{
}



} // namespace Laidout

