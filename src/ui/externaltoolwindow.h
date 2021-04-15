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
// Copyright (C) 2021 by Tom Lechner
//
#ifndef EXTERNALTOOLWINDOW_H
#define EXTERNALTOOLWINDOW_H


#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include <lax/buttondowninfo.h>

#include "../core/externaltools.h"


namespace Laidout {


class ExternalToolManagerWindow : public Laxkit::RowFrame
{
	double main_w, main_h;
	Laxkit::SquishyBox *main_box;
	int hover_cat;
	int hover_item;
	int hover_action;
	int lb_action;
	Laxkit::ButtonDownInfo buttondown;

	void GetMainExtent();
	int scan(int x, int y, int *hitem, int *haction);

  public:
 	ExternalToolManagerWindow(Laxkit::anXWindow *parnt, unsigned long nowner, const char *mes);
	virtual ~ExternalToolManagerWindow();
	virtual const char *whattype() { return "ExternalToolManagerWindow"; }
	// virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	

	virtual int send();
};


class ExternalToolWindow : public Laxkit::RowFrame
{
	Laxkit::LineInput *name, *commandid, *path, *parameters, *description, *website;
	ExternalTool *tool;
	
	bool ValidityCheck();
	
  public:
 	ExternalToolWindow(Laxkit::anXWindow *parnt, ExternalTool *etool, unsigned long nowner, const char *mes);
	virtual ~ExternalToolWindow();
	virtual const char *whattype() { return "ExternalToolWindow"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual int send();
};


} //namespace Laidout


#endif

