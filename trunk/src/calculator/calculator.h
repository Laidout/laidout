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

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "../styles.h"
#include "values.h"

//----------------------------- LaidoutCalculator -------------------------------------
class LaidoutCalculator : public Laxkit::RefCounted
{
 private:
	char *messagebuffer;
	char *dir;
	ValueHash *parse_parameters(StyleDef *def, const char *in, int len, char **error_pos_ret);
	ValueHash *build_context();
 protected:
	char decimal;
 public:
	LaidoutCalculator();
	virtual ~LaidoutCalculator();

	virtual char *In(const char *in);
	virtual int evaluate(const char *in, int len, Value **value_ret, int *error_pos, char **error_ret);
};

//------------------------------- parsing helpers ------------------------------------
LaxFiles::Attribute *parse_fields(LaxFiles::Attribute *Att, const char *str,char **end_ptr);
LaxFiles::Attribute *parse_a_field(LaxFiles::Attribute *Att, const char *str, char **end_ptr);
LaxFiles::Attribute *MapAttParameters(LaxFiles::Attribute *rawparams, StyleDef *def);


#endif

