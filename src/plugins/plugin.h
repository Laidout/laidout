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
#include <lax/newwindowobject.h>
//#include "../calculator/values.h"
//#include "../filetypes/filefilters.h"
#include "../laidout.h"


namespace Laidout {

class PluginBase : public Laxkit::anObject
{
  public:
	enum PluginBaseContents {
		PLUG_Panes           = (1<<0),
		PLUG_ImageImporters  = (1<<1),
		PLUG_TextImporters   = (1<<2),
		PLUG_ImportFilters   = (1<<3),
		PLUG_ExportFilters   = (1<<4),
		PLUG_DrawableObjects = (1<<5),
		PLUG_Tools           = (1<<6),
		PLUG_Configs         = (1<<7),
		PLUG_Resources       = (1<<8),
		PLUG_Impositions     = (1<<9),
		PLUG_Actions         = (1<<10),
		PLUG_Interpreters    = (1<<11),
		PLUG_CalcModules     = (1<<12),
		PLUG_MAX
	};

	virtual const char *Id_str() { return object_idstr; }
	virtual unsigned long Id() { return object_id; }

	virtual const char *PluginName()  = 0;
	virtual const char *Version()     = 0;
	virtual const char *Description() = 0;
	virtual const char *Author()      = 0;
	virtual const char *ReleaseDate() = 0;
	virtual const char *License()     = 0;
	virtual const LaxFiles::Attribute *OtherMeta() = 0;

	virtual unsigned long WhatYouGot() = 0; //or'd list of PluginBaseContents

	 //return NULL terminated list
	virtual Laxkit::NewWindowObject    **WindowPanes()       { return NULL; }
	virtual ImportFilter               **ImportFilters()     { return NULL; }
	virtual ExportFilter               **ExportFilters()     { return NULL; }
	virtual Laxkit::ResourceType       **Tools();
	virtual Imposition                 **Impositions()       { return NULL; }
	virtual DrawableObject             **ObjectInstances()   { return NULL; } //like scrapbook items
	virtual CalculatorModule           **CalculatorModules() { return NULL; }
	virtual Interpreter                **Interpreters()      { return NULL; } //like python. selecting "Run" of a PlainTextObject uses these
	virtual Laxkit::Resource           **ResourceInstances() { return NULL; }

	//virtual ImageImportFilter        **ImageImportFilters() { return NULL; }
	//virtual LaidoutAction            **Actions() { return NULL; }//maybe scripted code actions to be available for key bindings in windows
	//virtual Config                   **Configs() { return NULL; } //tool settings and global Laidout config

	
	PluginBase();
	virtual ~PluginBase();
	virtual const char *whattype() { return "PluginBase"; }
};


} //namespace Laiodut


#endif

