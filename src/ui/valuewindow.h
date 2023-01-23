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
// Copyright (C) 2019 by Tom Lechner
//
//
#ifndef VALUEWINDOW_H
#define VALUEWINDOW_H

#include <lax/rowframe.h>
#include <lax/scrolledwindow.h>

#include "../calculator/values.h"


namespace Laidout {

//----------------------------- ValueWindow -------------------------------

class ValueWindow : public Laxkit::ScrolledWindow, public Laxkit::SquishyBox
{
  protected:
	Value *value;
	Laxkit::RowFrame *rowframe;
	bool initialized;

	virtual void Initialize(const char *prevpath, Value *val, ObjectDef *mainDef, const char *pathOverride);
	void Send();

  public:
	ValueWindow(Laxkit::anXWindow *prnt, const char *nname, const char *ntitle, unsigned long nowner, const char *mes, Value *nvalue);
	virtual ~ValueWindow();
	virtual const char *whattype() { return "ValueWindow"; }
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int init();

	 //i/o
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

	virtual void syncWindows();
	virtual void Initialize();
};


} //namespace Laidout

#endif

