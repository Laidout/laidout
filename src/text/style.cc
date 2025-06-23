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


#include "style.h"


namespace Laidout {
	

//----------------------------------- Style -----------------------------------
/*! \class Style
 * \brief Class to hold cascading styles.
 *
 * Style objects are either temporary objects embedded in StreamElement, or they are project resources.
 * A Style may be based on a "parent" style. In this case, the parent style must be a project resource.
 */

Style::Style()
{
    parent = nullptr;
}

Style::Style(const char *new_name) //todo: *** name of what? instance? style type?
{
    parent = nullptr;
    Id(new_name);
}

Style::~Style()
{
}

/*! Create and return a new Style cascaded upward from this.
 * Values in *this override values in parents.
 */
Style *Style::Collapse()
{
    Style *s = new Style();
    Style *f = this;
    while (f) {
        s->MergeFromMissing(f);
        f = f->parent;
    }
    return s;
}

/*! Adds to *this any key+value in s that are not already included in this->values. It does not replace values.
 * Note ONLY checks against *this, NOT this->parent or this->kids.
 * Return number of items added.
 *
 * So, say you have:
 *     s:
 *       bold: true
 *       color: red
 *     this: 
 *       bold: false
 *       width: 5
 *
 * you will end up with:
 *     this:
 *       bold: false
 *       width: 5
 *       color: red
 */
int Style::MergeFromMissing(Style *s)
{
    const char *str;
    int n = 0;
    for (int c = 0; c < s->n(); c++) {
        str = s->key(c);
        if (findIndex(str) >= 0) continue; //don't use if we have it already
        push(str, s->e(c));
        n++;
    }
    return n;
}

/*! Adds or replaces any key+value from s into this->values.
 * Note ONLY checks against *this, NOT this->parent or this->kids.
 * Return number of items added.
 *
 * So, say you have:
 *     s:
 *       bold: true
 *       color: red
 *     this: 
 *       bold: false
 *       width: 5
 *
 * you will end up with:
 *     this:
 *       bold: true
 *       width: 5
 *       color: red
 */
int Style::MergeFrom(Style *s)
{
    const char *str;
    int n = 0;
    for (int c=0; c<s->n(); c++) {
        str = s->key(c);
        push(str, s->value(c)); //adds or replaces
        n++;
    }
    return n;
}


void Style::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	ValueHash::dump_in_atts(att, flag, context);
}

Laxkit::Attribute *Style::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	return ValueHash::dump_out_atts(att, what, context);
}

void Style::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what,context);
	att.dump_out(f,indent);
}


} // namespace Laidout
