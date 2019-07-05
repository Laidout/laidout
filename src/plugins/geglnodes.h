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
// Copyright (C) 2017 by Tom Lechner
//
#ifndef PLUGINS_GEGL_NODES_H
#define PLUGINS_GEGL_NODES_H

#include <gegl.h>

#include "plugin.h"


extern "C" Laidout::PluginBase *GetPlugin();


namespace Laidout {
namespace GeglNodesPluginNS {


//-------------------------------- GeglUser --------------------------

class GeglUser : public Laidout::NodeBase
{
  public:
	GeglUser() {}
	virtual ~GeglUser() {};
	virtual GeglNode *GetGeglNode() = 0;
	virtual int UpdatePreview();
};


//-------------------------------- GeglLaidoutNode --------------------------

class GeglLaidoutNode : public GeglUser
{
  protected:
	Laxkit::MenuItem *op;
	int IsSaveNode();
	int AutoProcess();

  public:
	static GeglNode *masternode;
	static Laxkit::SingletonKeeper op_menu;

	char *operation;
	GeglNode *gegl;

	GeglLaidoutNode(const char *oper);
	GeglLaidoutNode(GeglNode *node);
	virtual ~GeglLaidoutNode();
	virtual NodeBase *Duplicate();
	virtual GeglNode *GetGeglNode();

	virtual int SetOperation(const char *oper);
	virtual int UpdateProperties();
	virtual int Update();
	virtual int UpdatePreview();
	virtual int Disconnected(NodeConnection *connection, bool from_will_be_replaced, bool to_will_be_replaced);
	virtual int Connected(NodeConnection *connection);
	virtual int SetPropertyFromAtt(const char *propname, LaxFiles::Attribute *att);
	virtual int GetRect(Laxkit::DoubleBBox &box);
};


//-------------------------------- GeglNodesPlugin --------------------------

class GeglNodesPlugin : public Laidout::PluginBase
{
  public:
	GeglNodesPlugin();
	virtual ~GeglNodesPlugin();

	virtual unsigned long WhatYouGot(); //or'd list of PluginBase::PluginBaseContents

	virtual const char *PluginName();
	virtual const char *Name();
	virtual const char *Version();
	virtual const char *Description();
	virtual const char *Author();
	virtual const char *ReleaseDate();
	virtual const char *License();
	//virtual const LaxFiles::Attribute *OtherMeta();

	virtual int Initialize(); //install stuff
	virtual void Finalize();
};


} //namespace GeglNodesPluginNS
} //namespace Laidout


#endif

