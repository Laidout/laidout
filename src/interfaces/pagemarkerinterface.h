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
//    Copyright (C) 2014 by Tom Lechner
//
#ifndef _LAX_PAGEMARKERINTERFACE_H
#define _LAX_PAGEMARKERINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include "../core/page.h"

namespace Laidout { 



class PageMarkerInterface : public LaxInterfaces::anInterface
{
  protected:
	int showdecs;
	bool shownumbers;
	int mode;
	Laxkit::flatpoint boxoffset;
	double boxw,boxh;
	int hover, hoveri;
	double uiscale;

	Laxkit::ShortcutHandler *sc;

	class PageMarkerInterfaceNode {
	  public:
		Page *page;
		Laxkit::flatpoint origin;
		double scaling;
		int linetype;
		PageMarkerInterfaceNode(Page *npage, Laxkit::flatpoint npos, double nscaling, int nlinetype);
		~PageMarkerInterfaceNode();
	};
	Laxkit::PtrStack<PageMarkerInterfaceNode> pages;
	int curpage;

	Laxkit::PtrStack<Laxkit::ScreenColor> colors;
	Laxkit::NumStack<int> shapes;


	virtual int NearestColor(Laxkit::ScreenColor *color);
	virtual int scan(int x,int y, int &index);
	virtual int send();


  public:

	PageMarkerInterface(LaxInterfaces::anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~PageMarkerInterface();
	virtual LaxInterfaces::anInterface *duplicateInterface(LaxInterfaces::anInterface *dup);
	virtual const char *IconId() { return "PageMarker"; }
	const char *Name();
	const char *whattype() { return "PageMarkerInterface"; }
	const char *whatdatatype();
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(LaxInterfaces::SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	//virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual int AddPage(Page *page, Laxkit::flatpoint pos, double scaling, int nlinetype);
	virtual int UpdatePage(Page *page, Laxkit::flatpoint pos, double scaling, int nlinetype);
	virtual void ClearPages();
	virtual int UpdateCurpage(Page *page);
	virtual Laxkit::ScreenColor NextColor(Laxkit::ScreenColor &color);
	virtual Laxkit::ScreenColor PreviousColor(Laxkit::ScreenColor &color);
};

} // namespace Laidout

#endif

