//
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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef POLYPTYCHWINDOW_H
#define POLYPTYCHWINDOW_H

#include <lax/rowframe.h>
#include "../configured.h"
#include "netimposition.h"



namespace Laidout {


#ifndef LAIDOUT_NOGL

class PolyptychWindow : public Laxkit::RowFrame
{
  public:
	Laxkit::anXWindow *hwindow;
  public:
	PolyptychWindow(NetImposition *imp, Laxkit::anXWindow *parnt,unsigned long owner,const char *sendmes);
	virtual ~PolyptychWindow();

	virtual const char *whattype() { return "PolyptychWindow"; }
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	NetImposition *getImposition();
	virtual int setImposition(NetImposition *imp);
	virtual int sendNewImposition();
};

#endif

} // namespace Laidout

#endif

