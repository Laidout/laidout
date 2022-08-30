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
// Copyright (C) 2017 by Tom Lechner
//
//
#ifndef ADDONACTION_H
#define ADDONACTION_H

#include <lax/interfaces/aninterface.h>
#include <lax/anobject.h>
#include <lax/dump.h>

#include "../calculator/values.h"
#include "../core/plaintext.h"

namespace Laidout {

//----------------------------- AddonAction -------------------------------

class AddonAction : public Laxkit::anObject, public LaxFiles::DumpUtility, public SimpleFunctionEvaluator
{
  protected:

  public:
	char *label; //human readable for menu

	ValueHash *parameters;
	PlainText *script; //should be script -or- function
	ObjectFunc function;

	AddonAction();
	virtual ~AddonAction();
	const char *whattype() { return "AddonAction"; }

	virtual const char *Name() { return Id(); }
	virtual const char *Label() = 0;
	virtual ValueHash *Config() = 0;
	virtual int SetConfig(ValueHash *config) = 0; //new parameters object returned maybe after a dialog configures Config()
	virtual LaxInterfaces::anInterface *Interface() = 0; //if a link back to the action, then use this interface to edit parameters

	 //i/o
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

    virtual int RunAction(ValueHash *context, Value **value_ret, Laxkit::ErrorLog *log);

};


} //namespace Laidout

#endif

