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
// Copyright (C) 2013 by Tom Lechner
//


#include "shortcuttodef.h"
#include <lax/anxapp.h>
#include <lax/interfaces/aninterface.h>


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//------------------------------------ ShortcutEvaluator -----------------------------------------
/*! \class ShortcutEvaluator
 * \brief Class to simplify automatically including key shortcut actions as zero parameter functions for scripting.
 */
class ShortcutEvaluator : public FunctionEvaluator
{
  public:
	ShortcutEvaluator() {}
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log);
};

/*! "this" must be defined in context.
 */
int ShortcutEvaluator::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	Value *obj=context->find("this");
	if (!obj) return -1;

	ShortcutHandler *sc=NULL;
	anXWindow *window=dynamic_cast<anXWindow*>(obj);
	if (window) {
		sc=window->GetShortcuts();
	    int action=sc->FindActionNumber(func,len);
	    if (action>=0) {
			if (window->PerformAction(action)) return -1;
			return 0;
	    }

		return -1;
	}

	anInterface *interface=dynamic_cast<anInterface*>(obj);
	if (interface) {
		sc=interface->GetShortcuts();
	    int action=sc->FindActionNumber(func,len);
	    if (action>=0) {
			if (interface->PerformAction(action)) return -1;
			return 0;
	    }

		return -1;
	}

	return -1;
}

ShortcutEvaluator shortcutevaluator;


//! Convert Laxkit shortcut actions in sc into functions that take no parameters.
/*! def and sc must both exist already. Null is returned if not.
 *
 * It is assumed that at some point, the functions created here will be called as
 * member functions of an object of type def. Thus we do not have to give these
 * function definitions evaluators.
 *
 * The object must be derived from FunctionEvaluator, and is responsible for
 * using its shortcuthandler->FindActionNumber() with the given function name.
 *
 */
ObjectDef *ShortcutsToObjectDef(Laxkit::ShortcutHandler *sc, ObjectDef *def)
{
	//basically, you want behavior like:
	//
	//viewport.save()
	// --> viewport->PerformAction( viewport->sc->FindActionNumber("save") )


	if (!sc || !def) return NULL;

	WindowAction *action;
	for (int c=0; c<sc->NumActions(); c++) {
		action=sc->Action(c);

		def->pushFunction(action->name,action->name,action->description, &shortcutevaluator);
	}

	return def;
}


} // namespace Laidout

