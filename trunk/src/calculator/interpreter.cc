//
// $Id$
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
// Copyright (C) 2009 by Tom Lechner
//



********this file is not active yet!!!*****************




namespace Laidout {


LaidoutApp::AddInterpreter(Interpreter *i);
LaidoutApp::RemoveInterpreter(Interpreter *i);

 //in LaidoutApp:
...init() {
	interpreter->InitInterpreter();
}

...close() {
	interpreter->CloseInterpreter();
}



//----------------------------InitInterpreters()-----------------------------------------




//! Initialize and install built in interpreters
int InitInterpreters()
{
	int numadded=0;

	LaidoutCalculator *calc=new LaidoutCalculator();
	laidout->AddInterpreter(calc);
	numadded++;

	PythonInterpreter *python=new PythonInterpreter();
	laidout->AddInterpreter(python);
	numadded++;

	return numadded;
}



class LaidoutCalculator : public Interpreter
{  .... };




//---------------------------------- Interpreter ---------------------------------------
/*! \class Interpreter
 * \brief Base class for all extra interpreter language bindings.
 */

Interpreter::Interpreter()
{
	source_plugin=NULL;
}

Interpreter::~Interpreter()
{
	if (source_plugin) source_plugin->dec_count();
}


void Interpreter::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext)
{
	Attribute att;
	dump_out_atts(&att, what, savecontext);
	att.dump_out(f,indent);
}



//------------------------------------PythonInterpreter--------------------------------------
class PythonInterpreter : public Interpreter
{
	int gil_state;
	int gil_init;

  public:

	PythonInterpreter();
	virtual ~PythonInterpreter();

	const char *Id() { return "python"; }
	const char *Name() { return "Python"; }
	const char *Description() { return _("A Python interpreter. See python.org for more."); }
	const char *Version() { return "0.1"; }

	virtual int InitInterpreter();
	virtual int CloseInterpreter();

	int In(const char *input_text, char **result_ret, char **error_ret) = 0;
	char *GetLastResult() = 0;
	char *GetError(int *input_index_ret) = 0;
	void ClearError() = 0;
};

PythonInterpreter::PythonInterpreter()
{
	gil_init=0;
	gil_state=-1;
}

PythonInterpreter::~PythonInterpreter()
{
}

//! Return 0 for interpreter ready to go, or nonzero for invalid, and cannot run.
int PythonInterpreter::InitInterpreter(const char *programname)
{
	//Py_SetProgramName(programname); //optional
	Py_Initialize();




	**** beware threads!!! put this somewhere relevant...
	if (!gil_init) {
		gil_init = 1;
		PyEval_InitThreads();
		PyEval_SaveThread();
	}
	gil_state = PyGILState_Ensure();
	// Call Python/C API functions...    
	PyGILState_Release(gil_state);

			

	 //setup the Laidout module
	*** create LaidoutMethods
	Py_InitModule("Laidout",LaidoutMethods);


//----defining python methods:
//static PyMethodDef EmbMethods[] = {
//    {"numargs",       //name of method
//      emb_numargs,    //c function
//       METH_VARARGS,  //meaning a function like: PyObject* emb_numargs(PyObject *self, PyObject *args) {...}
//      "Return the number of arguments received by the process." //documentation
//    },
//    {NULL, NULL, 0, NULL}
//};
//
//
//static PyObject* emb_numargs(PyObject *self, PyObject *args)
//{
//    if(!PyArg_ParseTuple(args, ":numargs"))
//        return NULL;
//    return Py_BuildValue("i", numargs);
//}


}

int PythonInterpreter::CloseInterpreter()
{
	Py_Finalize();
}

/*! \todo *** should be able to run in sub thread, to be able to kill the script..
 */
int PythonInterpreter::In(const char *input_text, char **result_ret, char **error_ret)
{
	PyObject *globals, *locals; **** 

	PyRun_SimpleString(input_text);
	PyObject *stringresult=PyRun_Simple(input_text,0,globals,locals);
}

char *PythonInterpreter::GetLastResult()
{
}

char *PythonInterpreter::GetError(int *input_index_ret)
{
}

void PythonInterpreter::ClearError()
{
}







} // namespace Laidout

