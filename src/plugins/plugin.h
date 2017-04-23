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
#ifndef PLUGIN_H
#define PLUGIN_H


#include <lax/anobject.h>
//#include <lax/newwindowobject.h>
//#include "../calculator/values.h"
//#include "../filetypes/filefilters.h"
//#include "../laidout.h"


namespace Laidout {


class PluginBase : public Laxkit::anObject
{
  protected:

  public:
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

	virtual const char *PluginName()  = 0;
	virtual const char *Version()     = 0;
	virtual const char *Description() = 0; //localized
	virtual const char *Author()      = 0;
	virtual const char *ReleaseDate() = 0;
	virtual const char *License()     = 0;
	//virtual const LaxFiles::Attribute *OtherMeta() = 0;

	virtual unsigned long WhatYouGot() = 0; //or'd list of PluginBaseContents

	virtual int Initialize(); //install stuff

	 //return NULL terminated list
	//virtual Laxkit::NewWindowObject    **WindowPanes()       { return NULL; }
	//virtual ImportFilter               **ImportFilters()     { return NULL; }
	//virtual ExportFilter               **ExportFilters()     { return NULL; }
	//virtual Laxkit::ResourceType       **Tools();
	//virtual Imposition                 **Impositions()       { return NULL; }
	//virtual DrawableObject             **ObjectInstances()   { return NULL; } //like scrapbook items
	//virtual CalculatorModule           **CalculatorModules() { return NULL; }
	//virtual Interpreter                **Interpreters()      { return NULL; } //like python. selecting "Run" of a PlainTextObject uses these
	//virtual Laxkit::Resource           **ResourceInstances() { return NULL; }

	//virtual ImageImportFilter        **ImageImportFilters() { return NULL; }
	//virtual LaidoutAction            **Actions() { return NULL; }//maybe scripted code actions to be available for key bindings in windows
	//virtual Config                   **Configs() { return NULL; } //tool settings and global Laidout config


};

PluginBase *LoadPlugin(const char *path_to_plugin);

} //namespace Laiodut


#endif

