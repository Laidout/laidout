//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2025-present by Tom Lechner
//
#ifndef _LO_CSSPARSE_H
#define _LO_CSSPARSE_H


#include <cctype>

#include <lax/cssutils.h>
#include <lax/strmanip.h>
#include <lax/debug.h>
#include <lax/fontmanager.h>


namespace Laidout {
namespace CSS {



//--------------------------- text parsing utils ------------------------------

const char *ParseName(const char *start, int *n_ret, const char *extra);


//-------------------------------- css classes ----------------------------------------

class CSSParseCache
{
  public:
  	Laxkit::RefPtrStack<Laxkit::LaxFont> fonts;

};


class CSSSelector
{
  public:
    char *name = nullptr;
	char *pseudo_class = nullptr;
	char *pseudo_element = nullptr;

	// any descended, ie "E F"
	// direct child, ie "E > F"
	char qualifier = 0; // relative to previous selector in stack, can be ' ' or '>', or nul.
	char type = 0; // '.' == class type, '#' == id type, '>' == qualifier

	CSSSelector *next = nullptr;

    CSSSelector() {}
	~CSSSelector()
	{
		delete[] name;
		delete[] pseudo_class;
		delete[] pseudo_element;
		if (next) delete next;
	}

	bool IsType()      { return type != '.' && type != '#'; }
	bool IsClass()     { return type == '.'; }
	bool IsID()        { return type == '#'; }
	bool IsQualifier() { return type == '>'; }
};


//-------------------------------- css functions ----------------------------------------

Style *ProcessCSSFontFace(Style *style, Laxkit::Attribute *att, Laxkit::ErrorLog *log);
CSSSelector *ParseCSSSelector(const char *cssvalue, const char **endptr);
Laxkit::LaxFont *MatchCSSFont(const char *family_list, int italic, const char *variant, int weight, CSSParseCache *cache);
StreamElement *ParseCommonStyle(Laxkit::Attribute *att, StreamElement *current, Laxkit::ErrorLog *log);
Style *ProcessCSSBlock(Style *existing_style, const char *cssvalue, const char **error_pos_ret, Laxkit::ErrorLog &log);


} // namespace CSS
} // namespace Laidout

#endif
