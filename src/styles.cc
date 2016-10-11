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
// Copyright (C) 2004-2012 by Tom Lechner
//

***
*** THIS FILE NO LONGER ACTIVE
***

#include <cctype>
#include <cstdlib>
#include <cstdarg>

#include <lax/strmanip.h>

#include "styles.h"
#include "stylemanager.h"

#include <lax/lists.cc>
#include <lax/refptrstack.cc>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;



namespace Laidout {



/*! \defgroup stylesandstyledefs Styles and ObjectDefs
 * 
 * -------- Styles and ObjectDefs -----------
 * 
 *  There are Values, Styles and ObjectDefs. ObjectDef has a map of the names
 *  of the style or value and of its fields. Value and Style have actual instances of the 
 *  various fields. Unlike Value, each Style can be based on some other Style. 
 *  
 *  For example you might have a character style \n
 *    c1 = italic/bookman/10pt/red/etc., basedon=NULL\n
 *  The ObjectDef has field names (say) ItalicFlag/FaceName/Size/Color/etc.
 *  Then say you want a style c2 exactly like c1, but bold too, then the only 
 *  data actually stored in c2 is the BoldFlag, a pointer to c1, and a pointer
 *  to the ObjectDef.
 *
 */






//------------------------------------- Style -------------------------------------------


/*! \class Style
 * \ingroup stylesandstyledefs
 * \brief Abstract base class for styles.
 * 
 *  class Style : public Laxkit::anObject, public LaxFiles::DumpUtility 
 * 
 *  Styles hold the actual values of a style definition found in a ObjectDef.
 *  Many styles are specialized enough to have their own derived Style classes,
 *  for instance when changing one value forces the change in another, it is 
 *  just easier to have a separate class to deal with it. Styles that do not
 *  need such specific checking can be a GenericStyle instance.
 *
 *  Please note that Style itself does not check for pointer consistency when
 *  reassigning, deleting, and creating new styles. that has to be implemented
 *  somewhere else (StyleManager, for instance).
 * 
 *  Styles are kind of an uber-Value. They are not Value themselves because 
 *  the Value setup doesn't need all the fancy basedon stuff,
 *  and also doesn't need the FieldMask feedback, which says what fields are
 *  changed when a field is assigned a new value.
 *
 *  Any given Style might be a derived class, and stores the fields in
 *  any old way, so any code that deals with Styles
 *  should always use the wrapper functions to get and set values. 
 *
 *  See GenericStyle for a kind of all-purpose flat Style, that is, it does not
 *  have any fancy hardcoded shortcuts.
 *
 *  Finally, derived classes should remember to define their own dump_out() and
 *  dump_in_atts(), required by class LaxFiles::DumpUtility.
 *
 *
 * <pre>
 *  ****IMPORTANT there must be an easy setup that allows a redefined Style to 
 *  **** easily change
 *  ****values when other values are changed, such as for constrained aspect ratios..
 *  **** some default way to set/access fields that uses all the FieldNode/ObjectDef mechanism. This would 
 *  		be beneficial for scripting interface
 * </pre>
 */
/*! \var char *Style::stylename
 * \brief The name of the style.
 * 
 * Note this is not necessarily a variable name as seen by an interpreter, 
 * but it is an instance name.
 * It would be included in a list of available styles, like "Bold Body", and the
 * ObjectDef name/Name might be "charstyle"/"Character Style"
 */
/*! \var ObjectDef *Style::styledef
 * \brief The ObjectDef that gives information about what the Style's fields are.
 *
 *  This points to an original ObjectDef that would be kept in a pool somewhere, and
 *  which outlasts the Style instances.
 */
/*! \var Style *Style::basedon
 * \brief The Style that this style is based on.
 *
 * Any fields that are not actually defined in this Style should be available in basedon,
 * or something basedon is based on.
 */
/*! \var FieldMask Style::fieldmask
 * \brief Mask of which values are defined in this Style, and would overwrite fields from basedon.
 *
 * Mask values refer only to the top level fields, not subfields.
 */
/*! \fn ObjectDef *Style::makeObjectDef()
 * \brief Construct and return an instance of the relevant ObjectDef.
 *
 * Default just returns NULL. This function should otherwise return a unique instance
 * of the relevant ObjectDef. Normally, this would then be added to the StyleManager,
 * and other Styles of the same kind would have a reference to that one, not a unique
 * def of their own.
 */
/*! \fn const char *Style::Stylename()
 * \brief Return const pointer to stylename.
 */

/*! \fn Style *Style::duplicate(Style *s)
 * \brief Abstract function. Derived classes should return a copy of the style.
 * 
 * This is really in lieu of copy constructor, since one never always
 * knows what is down in the style.. duplicate() must create a whole copy,
 * not just the elements that subclasses know how to copy over.
 *
 * Usually, s could  be a subclass. In that case, the subclass would have called
 * the based class duplicate() to fill in what the subclass didn't copy over.
 * Otherwise, passing s==NULL means the style should create a new copy of itself.
 */



//! Start up with blank slate. All null variables.
Style::Style()
{ 
	basedon=NULL; 
	styledef=NULL; 
	stylename=NULL; 
}

//! Set up with the given styledef, basedon and stylename.
Style::Style(ObjectDef *sdef,Style *bsdon,const char *nstn)
{ 
	styledef=sdef; 
	basedon=bsdon; 
	stylename=NULL; 
	makestr(stylename,nstn); 
}

//! Destructor, currently just deletes stylename. styledef and based on assumed non-local.
/*! \todo *** should implement some reference counting thing for Style and ObjectDef..
 */
Style::~Style()
{
	//DBG cerr <<"Style \""<<stylename<<"\" destructor"<<endl;
	if (stylename) delete[] stylename;
	if (styledef) styledef->dec_count();
}

//! Return styledef, call makeObjectDef() if necessary.
ObjectDef *Style::GetObjectDef()
{
	if (!styledef) styledef=makeObjectDef();
	return styledef;
}

//! Change stylename. Return 1 for changed, 0 for not.
int Style::Stylename(const char *nname)
{ makestr(stylename,nname); return 1; }


//! Return the number of top fields in this Style.
/*! This would be redefined for Styles that are variable length sets
 *  ObjectDef itself has no way of knowing how many fields there are
 *  and even what kind of fields they are in that case.
 *
 *  Returns styledef->getNumFields(). If styledef==NULL, then return -1;
 *  
 *  \todo *** so what? how is this used in conjunction with ObjectDef::getNumFields?
 */
int Style::getNumFields()
{ 
	if (styledef) return styledef->getNumFields(); 
	return -1;
}
	
/*! Does not increment the def count. Calling code must do that if it uses it further.
 */
ObjectDef *Style::FieldInfo(int i)
{
	if (i<0 || i>=getNumFields()) return NULL;
	ObjectDef *def=GetObjectDef();
	if (!def) return NULL;
	return def->getField(i);
}

const char *Style::FieldName(int i)
{
	ObjectDef *def=GetObjectDef();
	if (!def) return NULL;
	if (i<0 || i>=getNumFields()) return NULL;
	def=def->getField(i);
	if (!def) return NULL;
	return def->name;
}


//-------------------------------- EnumStyle -------------------------------------


/*! \class EnumStyle
 * \brief Convenience class to simplify creation of dynamically created lists in dialogs.
 */

EnumStyle::EnumStyle()
	: names(2)
{}

ObjectDef *EnumStyle::makeObjectDef()
{
	cout <<" *** imp me! EnumStyle::makeObjectDef"<<endl;
	return NULL; //****
}

/*! \todo ***imp me!
 */
Style *EnumStyle::duplicate(Style *s)
{
	cout <<" *** imp me! EnumStyle::duplicate"<<endl;
	return NULL;
}

//! Return the id of the new item.
/*! If nid==-1, then make the id 1 more than the maximum of any previous id.
 */
int EnumStyle::add(const char *nname,int nid)
{
	names.push(newstr(nname));
	if (nid<0) {
		for (int c=0; c<ids.n; c++) {
			if (ids.e[c]>nid) nid=ids.e[c]+1;
		}
		if (nid<1) nid=1;
	}
	ids.push(nid);
	return nid;
}

//! Return name for id. NULL if not found.
const char *EnumStyle::name(int Id)
{
	for (int c=0; c<ids.n; c++) {
		if (ids.e[c]==Id) return names.e[c];
	}
	return NULL;
}

//! Return id for name. -1 if not found.
int EnumStyle::id(const char *Name)
{
	for (int c=0; c<names.n; c++) {
		if (!strcmp(Name,names.e[c])) return ids.e[c];
	}
	return -1;
}

} // namespace Laidout


