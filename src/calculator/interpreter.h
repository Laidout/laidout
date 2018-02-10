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
// Copyright (C) 2009-15, 2018 by Tom Lechner
//
#ifndef LAIDOUT_INTERPRETER_H
#define LAIDOUT_INTERPRETER_H


#include <lax/anobject.h>
#include <lax/dump.h>

#include "values.h"
#include "../plugins/plugin.h"


namespace Laidout {


//---------------------------------- Interpreter ---------------------------------------
/*! \class Interpreter
 * \brief Base class for all extra interpreter language bindings.
 */
class Interpreter : public Laxkit::anObject, public LaxFiles::DumpUtility
{
  protected:
	int runstate; //0 is ok, nonzero is it needs to be killed if in mid-process

  public:
	PluginBase *source_plugin; //if the interpreter came from a plugin

	virtual const char *Id()          = 0;
	virtual const char *Name()        = 0;
	virtual const char *Description() = 0;
	virtual const char *Version()     = 0;

	Interpreter();
	virtual ~Interpreter();
	virtual int InitInterpreter()  = 0;
	virtual int CloseInterpreter() = 0;

	virtual void Kill();

	 //return status: 0 success, -1 success with warnings, 1 fatal error
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
			                         Value **value_ret, Laxkit::ErrorLog *log) = 0;

	virtual const char *Message() = 0;
	virtual void ClearError() = 0;

	 //dumping in and out history
    virtual void       dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context) = 0;
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context) =0;

};



} // namespace Laidout


#endif

