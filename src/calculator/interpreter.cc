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


//------------------------------------PythonInterpreter--------------------------------------
class PythonInterpreter : public Interpreter
{
 public:
	
	const char *Id() { return "python"; }
	const char *Name() { return "Python"; }
	const char *Description() { return _("The python interpreter. See python.org for more."); }
	const char *Version() { return "1.0"; }

	virtual int InitInterpreter();
	virtual int CloseInterpreter();

	int In(const char *input_text, char **result_ret, char **error_ret) = 0;
	char *GetLastResult() = 0;
	char *GetError(int *input_index_ret) = 0;
	void ClearError() = 0;
};

//! Return 0 for interpreter ready to go, or nonzero for invalid, and cannot run.
int PythonInterpreter::InitInterpreter(const char *programname)
{
	//Py_SetProgramName(programname); //optional
	Py_Initialize();

	***setup the Laidout module

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



//---------------------------------- Interpreter ---------------------------------------
/*! \class Interpreter
 * \brief Base class for all extra interpreter language bindings.
 */
class Interpreter : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	Plugin *source_plugin; //if the interpreter came from a plugin

	const char *Id() = 0;
	const char *Name() = 0;
	const char *Description() = 0;
	const char *Version() = 0;
	
	Interpreter();
	virtual ~Interpreter();
	virtual int InitInterpreter() = 0;
	virtual int CloseInterpreter() = 0;

	void KillScript(); //run script in separate thread, so, In would be called, then
					   //laidout would continue on its way, until a message comes saying
					   //that the script is all done. In the meantime, this function could
					   //be called during execution to kill bad scripts
					   //***probably that level of care should be entirely within the
					   //laidout code that uses interpreters, not the interpreter itself

	 //return status: 0 success, -1 success with warnings, 1 fatal error
	int In(const char *input_text, char **result_ret, char **error_ret) = 0;
	char *GetLastResult() = 0;
	char *GetError(int *input_index_ret) = 0;
	void ClearError() = 0;

	dump_out() {}     //for optionally saving any initialization state or history 
	dump_in_atts() {}
};



} // namespace Laidout

