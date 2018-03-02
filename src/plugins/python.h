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
#ifndef LAIDOUT_PLUGIN_PYTHON_H
#define LAIDOUT_PLUGIN_PYTHON_H


//note, this needs to go before various standard headers
#include <Python.h>


#include "../calculator/interpreter.h"
#include "../language.h"


extern "C" Laidout::PluginBase *GetPlugin();


namespace Laidout {
namespace PythonNS {


//---------------------------------- PythonInterpreter ---------------------------------------

class PythonInterpreter : public Interpreter
{
  protected:
	static int interpreter_count;
	static int gil_init;
	static int initialized;
	static PyGILState_STATE gil_state;

	char *last_message;

  public:
	const char *Id()          { return "Python"; }
	const char *Name()        { return _("Python"); }
	const char *Version()     { return "0.1"; }
	const char *Description();

	PythonInterpreter();
	virtual ~PythonInterpreter();
	virtual int InitInterpreter();
	virtual int CloseInterpreter();

	 //return status: 0 success, -1 success with warnings, 1 fatal error
//	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//			                                     Value **value_ret, Laxkit::ErrorLog *log);
	virtual char *In(const char *in, int *return_type);
	virtual int Evaluate(const char *in, int len, Value **value_ret, Laxkit::ErrorLog *log);

	virtual const char *Message();
	virtual void ClearError();

	 //dumping in and out history
    //virtual void       dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

};


//---------------------------------- PythonPlugin ---------------------------------------

class PythonPlugin : public PluginBase
{
  private:
	PythonInterpreter *python;

  public:
	PythonPlugin();
	virtual ~PythonPlugin();

	virtual const char *PluginName()  { return "Python"; }
	virtual const char *Name()        { return _("Python"); }
	virtual const char *Version()     { return "0.1"; }
	virtual const char *Description() { return _("Plugin for python interpreter."); }
	virtual const char *Author()      { return "Laidout"; }
	virtual const char *ReleaseDate() { return "2018"; }
	virtual const char *License()     { return "GPL"; }
	virtual const LaxFiles::Attribute *OtherMeta() { return NULL; }

	virtual unsigned long WhatYouGot(); //or'd list of PluginBaseContents

	virtual int Initialize(); //install stuff
	virtual void Finalize();

};


} //namespace PythonNS
} // namespace Laidout


#endif

