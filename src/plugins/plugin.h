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
// Copyright (C) 2013-2017 by Tom Lechner
//
#ifndef PLUGIN_H
#define PLUGIN_H


#include <lax/anobject.h>
#include <lax/attributes.h>
#include <lax/errorlog.h>

//#include "../laidout.h"


namespace Laidout {


//------------------------- PluginBase --------------------------------------
	
class PluginBase : public Laxkit::anObject
{
  protected:

  public:
	char *filepath;
	void *handle;
	int initialized;

	//friend class LaidoutApp;


	enum PluginBaseContents {
		PLUGIN_Panes           = (1<<0),
		PLUGIN_ImageImporters  = (1<<1),
		PLUGIN_TextImporters   = (1<<2),
		PLUGIN_ImportFilters   = (1<<3),
		PLUGIN_ExportFilters   = (1<<4),
		PLUGIN_DrawableObjects = (1<<5),
		PLUGIN_Tools           = (1<<6),
		PLUGIN_Configs         = (1<<7),
		PLUGIN_Resources       = (1<<8),
		PLUGIN_Impositions     = (1<<9),
		PLUGIN_Actions         = (1<<10),
		PLUGIN_Interpreters    = (1<<11),
		PLUGIN_Nodes           = (1<<12),
		PLUGIN_CalcModules     = (1<<13),
		PLUGIN_MAX
	};
	

	PluginBase();
	virtual ~PluginBase();
	virtual const char *whattype() { return "PluginBase"; }

	virtual const char *PluginName()  = 0; //not localized
	virtual const char *Name()        = 0; //localized
	virtual const char *Version()     = 0;
	virtual const char *Description() = 0;
	virtual const char *Author()      = 0;
	virtual const char *ReleaseDate() = 0;
	virtual const char *License()     = 0;
	virtual const Laxkit::Attribute *OtherMeta() { return NULL; }

	virtual unsigned long WhatYouGot() = 0; //or'd list of PluginBaseContents

	virtual int Initialize(); //install stuff
	virtual char *DefaultConfigPath(bool include_file);
	virtual void Finalize();
};


//------------------------- LoadPlugin --------------------------------------

PluginBase *LoadPlugin(const char *path_to_plugin, Laxkit::ErrorLog &log);
int DeletePlugin(PluginBase *plugin);


} //namespace Laidout


#endif

