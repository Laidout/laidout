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
// Copyright (C) 2020 by Tom Lechner
//
//

#include "addonaction.h"



//for debugging: 
#include <iostream>
using namespace std;
#define DBG



namespace Laidout {

//----------------------------- AddonAction -------------------------------

/*! \class AddonAction
 *
 * Objectifying custom actions that run with specific parameters.
 * These can be accessed, for instance, via the "Miscellaneous actions" button in view window.
 */

AddonAction::AddonAction()
	: SimpleFunctionEvaluator(nullptr)
{
	action_definition = nullptr;
	parameters = nullptr;
	script     = nullptr;
}

AddonAction::~AddonAction()
{
	if (action_definition) action_definition->dec_count();
	if (parameters) parameters->dec_count();
	if (script) script->dec_count();
}

const char *AddonAction::Label()
{
	if (!label.IsEmpty()) return label.c_str();
	if (script) return script->Id();
	return Id();
}

ValueHash *AddonAction::Config()
{
	return parameters;
}

/*! New parameters object returned maybe after a dialog configures Config().
 * config does not have to correspond 1:1 to parameters. values in config are added to or replace
 * corresponding values in parameters.
 *
 * Return 1 for values changed/updated, else 0.
 */
int AddonAction::SetConfig(ValueHash *config, bool link_values)
{
	if (!parameters) parameters = new ValueHash();
	parameters->CopyFrom(config, link_values);
	return 1;
}

/*! Use this interface to edit parameters.
 */
LaxInterfaces::anInterface *AddonAction::Interface()
{
	return nullptr;
}


void AddonAction::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Laxkit::Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Laxkit::Attribute *AddonAction::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (what==-1) {
		if (!att) att = new Laxkit::Attribute;
		att->push("label", "Text for menu");
		att->push("parameters", nullptr, "(optional) Dictionary of parameters to use for this action");
		att->push("script", nullptr, "(optional) The script of this action. If the action is built in, no script is usually necessary.");

		return att;
	}

	if (!att) att = new Laxkit::Attribute;
	if (!label.IsEmpty()) att->push("label", label.c_str());

	if (parameters) {
		Laxkit::Attribute *att2 = att->pushSubAtt("parameters", nullptr, "Dictionary of parameters to use for this action");
		parameters->dump_out_atts(att2, what, savecontext);
	}

	if (script) {
		Laxkit::Attribute *att2 = att->pushSubAtt("script");
		script->dump_out_atts(att2, what, savecontext);
	}

	return att;
}

void AddonAction::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	char *name;
    char *value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"label")) {
        	label = value;
        
		} else if (!strcmp(name,"parameters")) {
			if (!parameters) parameters = new ValueHash();
			parameters->dump_in_atts(att->attributes.e[c], flag, context);

		} else if (!strcmp(name,"script")) {
			cerr << " *** TODO!!  AddonAction::dump_in_atts() script"<<endl;
		}
	} 
}

/*! This will call this->function(context, parameters, ...).
 */
int AddonAction::RunAction(ValueHash *context, Value **value_ret, Laxkit::ErrorLog *log)
{
	//runs this->function: typedef int (*ObjectFunc)(ValueHash *context, ValueHash *parameters, Value **value_ret, Laxkit::ErrorLog &log);

	return Evaluate("action",-1, context, parameters, NULL, value_ret, log);

	//-----
	// *** should use laidout->calculator to evaluate script with parameters as local variables
}


} // namespace Laidout

