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


#include "python.h"
#include <lax/strmanip.h>

#include "../laidout.h"


using namespace Laxkit;
using namespace LaxFiles;

namespace Laidout {
namespace PythonNS {


//------------------------------------ Test --------------------------------------


static PyObject* Test_HelloYall(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":HelloYall"))
        return NULL;
    return PyLong_FromLong(1);
}

static PyMethodDef TestMethods[] = {
    {"HelloYall", Test_HelloYall, METH_VARARGS, "The start of something good"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef TestModule = {
    PyModuleDef_HEAD_INIT, "Test", NULL, -1, TestMethods,
    NULL, NULL, NULL, NULL
};

static PyObject* PyInit_Test(void)
{
    return PyModule_Create(&TestModule);
}


//------------------------------------PythonInterpreter--------------------------------------

/*! \class PythonInterpreter
 */

int PythonInterpreter::interpreter_count = 0;
int PythonInterpreter::gil_init = 0;
int PythonInterpreter::initialized = 0;
PyGILState_STATE PythonInterpreter::gil_state;

PythonInterpreter::PythonInterpreter()
{
	last_message = NULL;


	interpreter_count++;
	if (interpreter_count == 1) {
		 //first time Python initialization

		//Py_SetProgramName(programname); //optional
		//Py_SetProgramName("Laidout"); //optional
		Py_Initialize();
		if (Py_IsInitialized()) {
			initialized = 1;


			// **** beware threads!!! put this somewhere relevant...
//			if (!gil_init) {
//				gil_init = 1;
//				PyEval_InitThreads();
//				PyEval_SaveThread();
//			}
//
//			gil_state = PyGILState_Ensure();
//
//			// Call Python/C API functions...    
//			// ....
//
//			//finish thread
//			PyGILState_Release(gil_state);


			 //setup the Laidout module
			//*** create LaidoutMethods
			//Py_InitModule("Laidout",LaidoutMethods);
			PyImport_AppendInittab("Test", PyInit_Test);


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

	} //end initial Python setup

}

PythonInterpreter::~PythonInterpreter()
{
	interpreter_count--;
	if (interpreter_count == 0) {
		//Py_FinalizeEx(); //from 3.6
		Py_Finalize();
		gil_init = 0;
	}
}

const char *PythonInterpreter::Description()
{
	return _("A Python interpreter. See python.org for more.");
}

//! Return 0 for interpreter ready to go, or nonzero for invalid, and cannot run.
int PythonInterpreter::InitInterpreter()
{
	return 0;
}

int PythonInterpreter::CloseInterpreter()
{
	return 0;
}

LaxFiles::Attribute *PythonInterpreter::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{ // ***
	return att;
}

void PythonInterpreter::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ // ***
}

int PythonInterpreter::Evaluate(const char *in, int len, Value **value_ret, Laxkit::ErrorLog *log)
{
	PyObject *globals = PyDict_New();
	PyObject *locals  = PyDict_New();
	//int flags=0; //see python/code.h: CO_*

	//PyObject *result = PyRun_String(in, Py_single_input, globals, locals); //single statement (no return value)
	PyObject *result = PyRun_String(in, Py_eval_input, globals, locals);     //single expression
	//PyObject *result = PyRun_String(in, Py_file_input, globals, locals);   //lines of code

	if (PyErr_Occurred()) {
		PyObject *ptype = NULL,
				 *pvalue = NULL,     //pvalue contains error message
				 *ptraceback = NULL; //ptraceback contains stack snapshot and many other information

		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		//Get error message
		//char *msg = PyUnicode_AsUTF8(repr);
		//char *msg = PyBytes_AsString(pvalue);


		PyObject *repr = PyObject_Repr(pvalue);
		const char* msg = PyUnicode_AsUTF8(repr);

		makestr(last_message, msg);
		log->AddMessage(msg, ERROR_Fail);

		if (ptype)      Py_DECREF(ptype);
		if (pvalue)     Py_DECREF(pvalue);
		if (ptraceback) Py_DECREF(ptraceback);

		PyErr_Print(); //this clears, i think
		PyErr_Clear();
		Py_DECREF(repr);

	} else {
		PyObject *repr = PyObject_Repr(result);
		const char* s = PyUnicode_AsUTF8(repr);
		makestr(last_message, s);
		Py_DECREF(repr);
	}

	if (result) Py_DECREF(result);
	Py_XDECREF(globals);

	if (log->Errors()  ) return 1;
	if (log->Warnings()) return -1;
	return 0;
}

char *PythonInterpreter::In(const char *in, int *return_type)
{   
    makestr(last_message,NULL);

    Value *v=NULL;
    ErrorLog errorlog;

    int status = isblank(in) ? 0 : Evaluate(in,-1, &v, &errorlog);

    char *buffer=NULL;
    int len=0;
    if (v) {
         //assume success
        v->getValueStr(&buffer,&len,1);
        appendstr(last_message, buffer);
        v->dec_count();
        if (return_type) *return_type=1;

    } else if (status!=0) {
         //there was an error
        if (return_type) *return_type=0;
        if (errorlog.Total()) {
            for (int c=0; c<errorlog.Total(); c++) {
                appendline(last_message, errorlog.Message(c,NULL,NULL,NULL,NULL));
            }
        }
        return newstr(last_message);

    }

    if (last_message) return newstr(last_message);

    if (return_type) *return_type=-1;
    //return newstr(_("You are surrounded by twisty passages, all alike."));
    return newstr("");
}

const char *PythonInterpreter::Message()
{
	return last_message;
}

void PythonInterpreter::ClearError()
{
	makestr(last_message, NULL);
}


//---------------------------------- PythonPlugin ---------------------------------------

/*! \class PythonPlugin 
 */

PythonPlugin::PythonPlugin()
{
	python = NULL;
}

PythonPlugin::~PythonPlugin()
{
	if (python) python->dec_count();
}

int PythonPlugin::Initialize()
{
	//install a PythonInterpreter in laidout
	python = new PythonInterpreter;
	laidout->AddInterpreter(python, 0);
	return 0;
}

void PythonPlugin::Finalize()
{
	//remove a PythonInterpreter in laidout
	laidout->RemoveInterpreter(python);
}

unsigned long PythonPlugin::WhatYouGot()
{
	return PLUGIN_Interpreters;
}


} //namespace PythonNS
} // namespace Laidout


/*! dl entry point for laidout
 */
Laidout::PluginBase *GetPlugin()
{
	return new Laidout::PythonNS::PythonPlugin();
}

