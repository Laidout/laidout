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
// Copyright (C) 2019 by Tom Lechner
//
#ifndef PLUGINWINDOW_H
#define PLUGINWINDOW_H


#include <lax/rowframe.h>


namespace Laidout {


class PluginWindow : public Laxkit::RowFrame
{

  public:
 	PluginWindow(Laxkit::anXWindow *parnt);
	virtual ~PluginWindow();
	virtual const char *whattype() { return "PluginWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual int send();
};


} //namespace Laidout


#endif

