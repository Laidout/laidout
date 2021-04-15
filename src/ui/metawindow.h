//
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
// Copyright (C) 2018 by Tom Lechner
//
//
#ifndef METAWINDOW_H
#define METAWINDOW_H

#include <lax/rowframe.h>
#include <lax/dump.h>


namespace Laidout {

//----------------------------- MetaWindow -------------------------------

enum MetaFlags {
	META_As_Is = (1<<16)
};

class MetaWindow : public Laxkit::RowFrame
{
  protected:
	int addpoint;
	unsigned long dialog_style;

  public:
	LaxFiles::AttributeObject *meta;

	MetaWindow(anXWindow *prnt, const char *nname,const char *ntitle,unsigned long nstyle, unsigned long nowner, const char *msg,
					  LaxFiles::AttributeObject *nMeta);
	virtual ~MetaWindow();
	const char *whattype() { return "MetaWindow"; }

    virtual int init();
	virtual void Send();
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual void AddVariable(const char *name, const char *value, bool syncToo);

	 //i/o
	//virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
	//virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} //namespace Laidout

#endif
