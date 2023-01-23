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
#include <lax/utf8string.h>

#include "../calculator/values.h"
#include "../core/plaintext.h"

namespace Laidout {

//----------------------------- AddonAction -------------------------------

class AddonAction : public Laxkit::anObject, public Laxkit::DumpUtility, public SimpleFunctionEvaluator
{
  protected:
	Laxkit::Utf8String label; //human readable for menu

  public:
  	ObjectDef *action_definition; //optional extra information about parameters and action purpose
	ValueHash *parameters;
	PlainText *script; //should be script -or- function, if both defined, use function
	ObjectFunc function;

	AddonAction();
	virtual ~AddonAction();
	const char *whattype() { return "AddonAction"; }

	virtual const char *Name() { return Id(); }
	virtual const char *Label();
	virtual ValueHash *Config(); //return the parameters object
	virtual int SetConfig(ValueHash *config, bool link_values); //new parameters object returned maybe after a dialog configures Config()
	virtual LaxInterfaces::anInterface *Interface(); // use this interface to edit parameters

	 //i/o
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

    virtual int RunAction(ValueHash *context, Value **value_ret, Laxkit::ErrorLog *log);

};


} //namespace Laidout

#endif

