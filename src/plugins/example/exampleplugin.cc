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
// Copyright (C) 2019 by Your Name Here
//


#include "../language.h"
#include "exampleplugin.h"


#include <iostream>
#define DBG


using namespace std;


/* Called by dlsym from Laidout. Return a PluginBase object.
 */
Laidout::PluginBase *GetPlugin()
{
	return new ExamplePluginNS::ExamplePlugin();
}



namespace ExamplePluginNS {

/*! \class ExamplePlugin
 *
 * Something really cool and revolutionary.
 */

ExamplePlugin::ExamplePlugin()
{
	//in general, you should not initialize anything other than simple types in or really do anything in
	//constructor, as accidentally loading or destroying the same thing twice will likely produce strange errors.
	//Instead do important initialization in Initialize(), and destroy those things in Finalize().
}

ExamplePlugin::~ExamplePlugin()
{
}

/*! This needs to be a unique, non-localized name of the plugin.
 * Laidout will not load this plugin in another with the same PluginName().
 */
const char *ExamplePlugin::PluginName()
{
	return "Example Plugin";
}

const char *ExamplePlugin::Name()
{
	return _("Example Plugin");
}

unsigned long ExamplePlugin::WhatYouGot()
{
	 //or'd list of PluginBase::PluginBaseContents
	return 0
		//| PLUGIN_Panes
		//| PLUGIN_ImageImporters
		//| PLUGIN_TextImporters
		//| PLUGIN_ImportFilters
		//| PLUGIN_ExportFilters
		//| PLUGIN_DrawableObjects
		//| PLUGIN_Tools
		//| PLUGIN_Configs
		//| PLUGIN_Resources
		//| PLUGIN_Impositions
		//| PLUGIN_Actions
		//| PLUGIN_Interpreters
		//| PLUGIN_Nodes
		//| PLUGIN_CalcModules
	  ;
}

const char *ExamplePlugin::Version()
{
	return "1.0";
}

/*! Return localized description.
 */
const char *ExamplePlugin::Description()
{
	return _("A good time to be had by all.");
}

const char *ExamplePlugin::Author()
{
	return "I. M. Sunshine";
}

const char *ExamplePlugin::ReleaseDate()
{
	return "2020 02 29";
}

const char *ExamplePlugin::License()
{
	return "LGPL3";
}

//const LaxFiles::Attribute *ExamplePlugin::OtherMeta()
//{ ***
//	return NULL;
//
//	//------------
//	//Attribute *att = new Attribute;
//	//att.push("important_info", "yes indeed");
//	//return att;
//}

/*! Install stuff.
 */
int ExamplePlugin::Initialize()
{
	if (initialized) return 0;

	initialized = 1;
	// do stuff
	DBG cerr << "ExamplePlugin initializing!"<<endl;

	return 0;
}

/*! Reverse of Initialize().
 */
void ExamplePlugin::Finalize()
{
	DBG cerr << "ExamplePlugin finalizing!"<<endl;
}



} //namespace ExamplePluginNS


