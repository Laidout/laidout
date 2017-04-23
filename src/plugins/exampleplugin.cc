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
// Copyright (C) 2017 by Tom Lechner
//


#include "../language.h"
#include "exampleplugin.h"


#include <iostream>
#define DBG


using namespace std;


namespace ExamplePluginNS {

/*! class ExamplePlugin
 *
 * Something really cool and revolutionary.
 */

ExamplePlugin::ExamplePlugin()
{
}

ExamplePlugin::~ExamplePlugin()
{
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

const char *ExamplePlugin::PluginName()
{
	return _("Example Plugin");
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
	return "2017 04 15";
}

const char *ExamplePlugin::License()
{
	return "LGPL3";
}

//const LaxFiles::Attribute *ExamplePlugin::OtherMeta()
//{
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



} //namespace ExamplePluginNS

Laidout::PluginBase *GetPlugin()
{
	return new ExamplePluginNS::ExamplePlugin();
}



