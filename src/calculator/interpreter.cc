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




//----------------------------InitInterpreters()-----------------------------------------


RefPtrStack<LaidoutCalculator> interpreters;

...LaidoutApp::close() {
	for (int c=0; c<interpreters.n; c++) {
		interpreters.e[c]->CloseInterpreter();
	}
	interpreters.flush();
}


//! Initialize and install built in interpreters
int LaidoutApp::InitInterpreters()
{
	int numadded = 0;

	 //add default calculator
	LaidoutCalculator *calc = new LaidoutCalculator();
	if (AddInterpreter(calc) == 0) numadded++;


	 //add Python
	PythonInterpreter *python = new PythonInterpreter();
	if (AddInterpreter(python) == 0) numadded++;


	return numadded;
}

/*! Takes count of i.
 */
int LaidoutApp::AddInterpreter(Interpreter *i)
{
	if (!i) return 1;
	interpreters.push(i);
	i->dec_count();
	return 0;
}

/*! Takes count of i.
 */
int LaidoutApp::RemoveInterpreter(Interpreter *i)
{
	if (!i) return 1;
	interpreters.remove(interpreters.findindex(i));
	return 0;
}



class LaidoutCalculator : public Interpreter
{  .... };




//---------------------------------- Interpreter ---------------------------------------
/*! \class Interpreter
 * \brief Base class for all extra interpreter language bindings.
 */

Interpreter::Interpreter()
{
	source_plugin = NULL;
	runstate = 0;
}

Interpreter::~Interpreter()
{
}


void Interpreter::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext)
{
	Attribute att;
	dump_out_atts(&att, what, savecontext);
	att.dump_out(f,indent);
}


/*! If run in separate thread, intepreter->In() would be called, then
 * laidout would continue on its way, until a message comes saying
 * that the script is all done. In the meantime, this function could
 * be called during execution to kill bad scripts.
 *
 * Default here is just to set the (hopefully atomic) flag runstate to -1, which interpreters should
 * respond to and shut down appropriately.
 *
 * ( *** more thought is needed here!!)
 */
void Interpreter::Kill()
{
	runstate = -1;
}




} // namespace Laidout

