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
// Copyright (C) 2018 by Tom Lechner
//
#ifndef NODEEDITOR_H
#define NODEEDITOR_H

#include <lax/interfaces/viewerwindow.h>

#include "nodeinterface.h"


namespace Laidout {

//----------------------------- NodeEditor -------------------------------

class NodeEditor : public LaxInterfaces::ViewerWindow
{
  protected:
	NodeInterface *tool;

	char *fileout, *outformat;
	bool pipeout;

	LaxFiles::Attribute passthrough;

  public:
	NodeEditor(Laxkit::anXWindow *parnt, const char *nname,const char *ntitle,
						unsigned long nowner, const char *mes,
						NodeGroup *nnodes,int absorb,
						const char *arg=NULL,
						NodeInterface *interface=NULL
						);
	virtual ~NodeEditor();
	virtual int init();
	virtual const char *whattype() { return "NodeEditor"; }
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual void send();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//----------------------------- Stand alone node editor -------------------------------

Laxkit::anXWindow *newNodeEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
								unsigned long nowner_id, const char *mes,
								NodeGroup *nnodes,int absorb,
								const char *editor_arg
								);



} //namespace Laidout

#endif

