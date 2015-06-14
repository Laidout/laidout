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


#include <lax/anobject.h>
#include <lax/dump.h>

namespace Laidout {


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

    virtual void       dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
    virtual Attribute *dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context) = 0;
    virtual void dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context) =0;

};



} // namespace Laidout

