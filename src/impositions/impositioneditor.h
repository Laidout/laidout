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
// Copyright (C) 2013 by Tom Lechner
//
#ifndef IMPOSITIONEDITOR_H
#define IMPOSITIONEDITOR_H

#include <lax/interfaces/viewerwindow.h>
#include <lax/menubutton.h>

#include "imposition.h"

namespace Laidout {

//----------------------------- ImpositionEditor -------------------------------

class ImpositionEditor : public LaxInterfaces::ViewerWindow
{
  protected:
	ImpositionInterface *tool;
	ImpositionInterface *tool_signatures;
	ImpositionInterface *tool_net;
	ImpositionInterface *tool_singles;

	Laxkit::anXWindow *neteditor;
	
	int whichactive;
	Imposition *firstimp;
	Document *doc;
	
	char *imposeout, *imposeformat;
	int rescale_pages;
	bool show_splash = false;

	Laxkit::MenuButton *type_mbut = nullptr;

	void SetMenuButton(const char *text);

  public:
	ImpositionEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner, const char *mes,
						Document *doc,
						Imposition *imposition,
						PaperStyle *p,
						const char *imposearg = nullptr
						);
	virtual ~ImpositionEditor();
	virtual int init();
	virtual const char *whattype() { return "ImpositionEditor"; }
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual void send();
	virtual Imposition *impositionFromFile(const char *file);
	virtual int ChangeImposition(Imposition *newimp);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//----------------------------- generic Imposition Editor creator -------------------------------

Laxkit::anXWindow *newImpositionEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner_id, const char *mes,
						PaperStyle *papertype, const char *type, Imposition *imp, const char *imposearg,
						Document *doc);



} //namespace Laidout

#endif
