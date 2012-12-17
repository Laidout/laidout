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
// Copyright (C) 2009-2012 by Tom Lechner
//


#include "values.h"
#include "../styles.h"
#include "stylemanager.h"

#include <lax/strmanip.h>
#include <lax/refptrstack.cc>

#include <cctype>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <iostream>

#define DBG
using namespace std;


using namespace LaxFiles;



namespace Laidout {


//NOTES:
// definetype polyhedron { v=set of vector, f=set of set of int, e=set of array[2]int }
// names: v={vector},f={dummy1},e={array[2]int},  dummy1=set of int 
//
// paragraph:
// 	first indent
// 	before lead
// 	after lead
// 	middle lead
// 	left indent
// 	right indent
//  




//----------------------------- typedef docs -----------------------------

/*! \typedef Style *(*NewStyleFunc)(ObjectDef *def)
 * \ingroup stylesandstyledefs
 * \brief These are in ObjectDef to aid in creation of new Style instances by StyleManager.
 */

/*! \ingroup stylesandstyledefs
 * Names for the ElementType enum.
 */
const char *element_TypeNames(int type)
{
	if (type==Element_Any) return "any";
	if (type==Element_None) return "none";
	if (type==Element_Set) return "set";
	if (type==Element_Object) return "object";
	if (type==Element_Int) return "int";
	if (type==Element_Real) return "real";
	if (type==Element_String) return "string";
	if (type==Element_Fields) return "fields";
	if (type==Element_Flatvector) return "flatvector";
	if (type==Element_Spacevector) return "spacevector";
	if (type==Element_File) return "file";
	if (type==Element_Flag) return "flags";
	if (type==Element_Enum) return "enum";
	if (type==Element_EnumVal) return "enumval";
	if (type==Element_Color) return "color";
	if (type==Element_Date) return "date";
	if (type==Element_Time) return "time";
	if (type==Element_Boolean) return "boolean";
	if (type==Element_Complex) return "complex";
	if (type==Element_Function) return "function";
	return "";
}


//------------------------------ ObjectDef --------------------------------------------

/*! \enum ElementType 
 * \ingroup stylesandstyledefs
 * \brief Says what a ObjectDef element is.
 */
	
/*! \class ObjectDef
 * \ingroup stylesandstyledefs
 * \brief The definition of the elements of a Value.
 *
 * \todo *** haven't worked on this code in quite a while, perhaps it could be somehow
 * combined with the Laxkit::Attribute type of thing? but for that, still need to reason
 * out a consistent Attribute definition standard...
 * 
 * These things are used to automatically create edit dialogs for any data. It also aids in
 * defining names and what they are for an interpreter. Ideally there should be only one instance
 * of a ObjectDef per type of style held in a style manager. The actual styles will have pointer
 * references to it.
 *
 * \todo have dialog format hints
 * \todo should have dynamic default values...
 * 
 *  example: 
 *  <pre>
 *   name        = spacevector    <-- name as it appears in an interpreter 
 *   Name        = Space Vector   <-- name as it appears as a dialog label
 *   description = A three dimensional vector  <-- description for some in-program help system or tooltip
 *    3 subfields:
 *    x,y,z all real
 * 
 *   Might be displayed like this:
 *   Space Vector: x= _12__  y= __1__  z= ___0_
 * 
 *   The name would be used in some command line interpreter.
 *   The Name would be used as shown in an edit dialog.
 *   The description would be used in some sort of help system
 *  </pre>
 *
 */
/*! \var ElementType ObjectDef::format
 * \brief What is the nature of *this.
 * 
 *   The format of the value of this ObjectDef. It can be any id that is defined in the stylemanager.
 *   These formats other than Element_Fields imply that *this is a single unit.
 *   Element_Fields implies that there are further subcomponents.
 */
/*! \var char *ObjectDef::name
 *  \brief Basically a class name, meant to be seen in the interpreter.
 */
/*! \var char *ObjectDef::Name
 *  \brief Basically a human readable version of the class name.
 *
 *  This is used as a label in automatically created edit windows.
 */
/*! \var char *ObjectDef::extends
 *  \brief The ObjectDef::name of which ObjectDef this one extends.
 *  
 *  If there is an extends, the index of all the fields starts with 0, which
 *	is the very first field in the very first base styledef.
 */
/*! \var ObjectDef *ObjectDef::extendsdef
 *  \brief Which ObjectDef this one extends.
 *  
 *	This is a pointer to a def in a StyleManager. ObjectDef looks up extends in
 *	the stylemanager to get the appropriate reference during the constructor.
 *  
 *  If there is an extends, the index of all the fields starts with 0, which
 *	is the very first field in the very first base styledef.
 */
/*! \var NewStyleFunc ObjectDef::newfunc
 * \brief Default constructor when the ObjectDef represents an object.
 *
 * For full constructor for objects, use stylefunc.
 */
/*! \var StyleFunc ObjectDef::stylefunc
 * \brief Callable function for the ObjectDef.
 *
 * If the styledef represents an object, then stylefunc is a full constructor,
 * to which you can supply parameters.
 *
 * If the styledef is a function, then this is what is called as the function.
 */


//! Constructor.
ObjectDef::ObjectDef(const char *nextends, //!< Which ObjectDef does this one extend
			const char *nname, //!< The name that would be used in the interpreter
			const char *nName, //!< A basic title, most likely an input label
			const char *ndesc,  //!< Description, newlines ok.
			ElementType fmt,     //!< Format of this ObjectDef
			const char *nrange,    //!< String showing range of allowed values
			const char *newdefval,   //!< Default value for the style
			Laxkit::RefPtrStack<ObjectDef> *nfields, //!< ObjectDef for the subfields or enum values.
			unsigned int fflags,       //!< New flags
			NewStyleFunc nnewfunc,    //!< Default creation function
			StyleFunc    nstylefunc)  //!< Full Function 
  : suggestions(2)
{
	newfunc=nnewfunc;
	stylefunc=nstylefunc;
	range=defaultvalue=extends=name=Name=description=NULL;

	makestr(extends,nextends);
	if (extends) {
		extendsdef=stylemanager.FindDef(extends); // must look up extends and inc_count()
		if (extendsdef) extendsdef->inc_count();
	} else extendsdef=NULL;
	
	makestr(name,nname);
	makestr(Name,nName);
	makestr(description,ndesc);
	makestr(range,nrange);
	makestr(defaultvalue,newdefval);
	
	fields=nfields;
	format=fmt;
	flags=fflags;
}

//! Delete the various strings, and styledef->dec_count().
ObjectDef::~ObjectDef()
{
	DBG cerr <<"ObjectDef \""<<name<<"\" destructor"<<endl;
	
	if (extends)      delete[] extends;
	if (name)         delete[] name;
	if (Name)         delete[] Name;
	if (description)  delete[] description;
	if (range)        delete[] range;
	if (defaultvalue) delete[] defaultvalue;
	
	if (extendsdef) {
		DBG cerr <<" extended: "<<extendsdef->name<<endl;
		extendsdef->dec_count();
	} else {
		DBG cerr <<"------------no extends"<<endl;
	}

	if (fields) {
		DBG cerr <<"---Delete fields stack"<<endl;
		delete fields;
		fields=NULL;
	}
}

//! Write out the stuff inside. 
/*! If this styledef extends another, this does not write out the whole
 * def of that, only the name element of it.
 */
void ObjectDef::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		//output something like:
		// fieldname example #description of field
		return;
	}

	if (name) fprintf(f,"%sname %s\n",spc,name);
	if (Name) fprintf(f,"%sName %s\n",spc,Name);
	if (description) fprintf(f,"%sdescription %s\n",spc,description);
	if (extends) fprintf(f,"%sextends %s\n",spc,extends);
	fprintf(f,"%sflags %u\n",spc,flags);
	fprintf(f,"%sformat %s\n",spc,element_TypeNames(format));

	if (fields) {
		for (int c=0; c<fields->n; c++) {
			fprintf(f,"%sfield\n",spc);
			fields->e[c]->dump_out(f,indent+2,0,context);
		}
	}
}

/*! \todo *** imp me!!
 */
void ObjectDef::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	cout<<" *** imp me! ObjectDef::dump_in_atts(Attribute *att,int flag,context)"<<endl;
	
}

//! Push an Element_EnumVal on an Element_enum.
/*! Return 0 for value pushed or nonzero for error. It is an error to push an
 * enum value on anything but an enum.
 */
int ObjectDef::pushEnumValue(const char *str, const char *Str, const char *dsc)
{
	if (format!=Element_Enum) return 1;
	return push(str,Str,dsc, 
				Element_EnumVal, NULL,NULL,
				(unsigned int)0, NULL,NULL);
}

//! Create an enum and pass in all the enum values here.
/*! The '...' are all const char *, in groups of 3, with a single NULL after the last
 * description.
 *
 * - the enum value scripting name,
 * - the enum value translated, human readable name
 * - the description of the value
 *
 * For instance, if you are adding 2 enum values, you must supply 6 const char * values, followed
 * by a single NULL, or all hell will break loose.
 */
int ObjectDef::pushEnum(const char *nname,const char *nName,const char *ndesc,
					 const char *newdefval,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc,
					 ...)
{
	ObjectDef *e=new ObjectDef(NULL,nname,nName,ndesc,
							 Element_Enum, NULL, newdefval, 
							 NULL, 0,
							 nnewfunc, nstylefunc);
	va_list ap;
	va_start(ap, nstylefunc);
	const char *v1,*v2,*v3;
	while (1) {
		v1=va_arg(ap,const char *);
		if (v1==NULL) break;
		v2=va_arg(ap,const char *);
		v3=va_arg(ap,const char *);

		e->pushEnumValue(v1,v2,v3);
	}
	va_end(ap);

	int c=push(e);
	if (c<0) delete e;
	return c;
}

//! Push def without fields. If pushing this new field onto fields fails, return -1, else the new field's index.
int ObjectDef::push(const char *nname,const char *nName,const char *ndesc,
			ElementType fformat,const char *nrange, const char *newdefval,unsigned int fflags,
			NewStyleFunc nnewfunc,
		 	StyleFunc nstylefunc)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,
								  ndesc,fformat,nrange,newdefval,
								  NULL,fflags,nnewfunc);
	int c=push(newdef);
	newdef->dec_count();
	return c;
}

//! Push def with fields. If pushing this new field onto fields fails, return 1, else 0
/*! Note that the counts for the subfields are not incremented further.
 */
int ObjectDef::push(const char *nname,const char *nName,const char *ndesc,
		ElementType fformat,const char *nrange, const char *newdefval,
		Laxkit::RefPtrStack<ObjectDef> *nfields,unsigned int fflags,
		NewStyleFunc nnewfunc,
		StyleFunc nstylefunc)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,ndesc,fformat,nrange,newdefval,nfields,fflags,nnewfunc);
	int c=push(newdef);
	newdef->dec_count();
	return c;
}

//! Push newfield onto fields as not local. Its count is incremented.
/*! Returns whatever RefPtrStack::push returns.
 * 
 * Only use pop() when removing elements, as that pops from the stack, then calls
 * its dec_count() function, rather than just deleting.
 */
int ObjectDef::push(ObjectDef *newfield)
{
	if (!newfield) return 1;
	if (!fields) fields=new Laxkit::RefPtrStack<ObjectDef>;
	return fields->push(newfield);
}

//! Convenience function to push a parameter field to previously added def.
/*! So, this will push a field onto fields->e[fields->n-1].
 * Aids in easier creation of function definitions.
 *
 * If there is no last item in fields, then create a new fields object. Note that
 * future pushParameter() calls in this case will add to that one, not to *this.
 */
int ObjectDef::pushParameter(const char *nname,const char *nName,const char *ndesc,
			ElementType fformat,const char *nrange, const char *newdefval)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,
								  ndesc,fformat,nrange,newdefval,
								  NULL,0,NULL);
	int c;
	if (!fields || !fields->n) c=push(newdef);
	else c=fields->e[fields->n-1]->push(newdef);
	newdef->dec_count();
	return c;
}

//! The element is popped off the fields stack, then thatelement->dec_count() is called.
/*! Returns 1 if item is removed, else 0.
 */
int ObjectDef::pop(int fieldindex)
{
	if (!fields || !fields->n || fieldindex<0 || fieldindex>=fields->n) return 0;
	ObjectDef *d=fields->pop(fieldindex);
	if (!d) return 0;
	d->dec_count();
	return 1;
}


 //! Returns the number of upper most fields that this styledef contains.
 /*! If this styledef is an extension of another, then the number returned is
  * the total number of fields defined in *this plus all the upper fields in
  * the extended styledef(s). Each styledef simply adds fields->n if fields
  * exists, or 0 if fields does not exists and extendsdef exists, or 1 if neither fields 
  * nor extendsdef exist. 
  *
  * A special exception is when format==Element_Enum. In that case, fields would
  * contain the possible enum values, but the enum as a whole acts like a single
  * number, so 1 is added, rather than fields->n. If the enum is an extension of
  * some other enum, then this styledef adds 0 to the count.
  * 
  * Consider a ObjectDef vector(x,y,z), Now say you define a ObjectDef called zenith
  * which is a vector with x,y,z. You want the normal x,y,z definitions, but the
  * head you want to now be "zenith" rather than "vector", So you extend "vector",
  * and do not define any new fields. Viola!
  *
  */
int ObjectDef::getNumFields()
{
	int n=0;

	if (format==Element_Enum) {
		if (!extendsdef) n++;
	} else {
		if (fields && fields->n) n+=fields->n; 
			else if (!extendsdef) n++;
		if (extendsdef) n+=extendsdef->getNumFields();
	}
	return n; 
}

//! Return various information about particular fields.
/*! Given the index of a desired field, this looks up which ObjectDef actually has the text
 * and sets the pointers to point to there. Nothing is done if the particular pointer is NULL.
 *
 * def_ret is the ObjectDef of index. This can be used to then read off enum names, for instance.
 *
 * If index==-1, then the info from *this is provided, rather than from a fields stack.
 *
 * Returns 0 success, 1 error.
 */
int ObjectDef::getInfo(int index,
						const char **nm,
						const char **Nm,
						const char **desc,
						const char **rng,
						const char **defv,
						ElementType *fmt,
						int *objtype,
						ObjectDef **def_ret)
{
	ObjectDef *def=NULL;
	index=findActualDef(index,&def);
	if (!def) return 1; // otherwise index should be a valid value in fields, or refer to this
	if (index==-1) {
		if (nm) *nm=def->name;
		if (Nm) *Nm=def->Name;
		if (desc) *desc=def->description;
		if (rng) *rng=def->range;
		if (defv) *defv=def->defaultvalue;
		if (fmt) *fmt=def->format;
		if (objtype) *objtype=def->fieldsformat;
		if (def_ret) *def_ret=def;
		return 0;
	}
	if (nm) *nm=def->fields->e[index]->name;
	if (Nm) *Nm=def->fields->e[index]->Name;
	if (desc) *desc=def->fields->e[index]->description;
	if (rng) *rng=def->fields->e[index]->range;
	if (defv) *defv=def->fields->e[index]->defaultvalue;
	if (fmt) *fmt=def->fields->e[index]->format;
	if (objtype) *objtype=def->fields->e[index]->fieldsformat;
	if (def_ret) *def_ret=def->fields->e[index];
	return 0;
}

//! Return the ObjectDef corresponding to the field at index.
/*! If the element at index does not have subfields or is not an enum,
 * then return NULL.
 */
ObjectDef *ObjectDef::getField(int index)
{
	ObjectDef *def=NULL;
	index=findActualDef(index, &def);
	if (index<0 || !def) return NULL;;
	return def->fields->e[index];
}

//! From a given total index, return the actual ObjectDef the index lies in, and the index within the returned def.
/*! Does not consider subfields, only top level fields.
 *
 * Say *this (call it def1) has 5 fields, and extends def2 which has 4 fields,
 * which extends def3 which has 2 fields.
 * Then findActualDef(10,&def_ret) will return 4 with def_ret==def1.
 * findActualDef(1,&def_ret) will return 1 with def_ret==def3.
 * findActualDef(4,&def_ret) will return 2 with def_ret==def2.
 *
 * On out of bounds, -1 is returned, and def_ret is set to NULL.
 */ 	
int ObjectDef::findActualDef(int index,ObjectDef **def_ret)
{
	if (index<0) { *def_ret=NULL; return -1; }
	
	 // if enum, or this is single unit (not extending anything): index must be 0
	if (index==0 && (format==Element_Enum || (!extendsdef && (!fields || !fields->n)))) {
		*def_ret=this;
		return 0;
	}
	 // else there should be fields somewhere
	int n=0;
	if (extendsdef) {
		n=extendsdef->getNumFields(); //counts all fields in extensions
		if (index<n) { // index lies in extendsdef somewhere
			return extendsdef->findActualDef(index,def_ret);
		} 	
	}
	
	 // index puts it in *this.
	index-=n;
	 // index>=0 at this point implies that this has fields, assuming original index is valid
	if (!fields || !fields->n || index<0 || index>=fields->n)  { //error
		*def_ret=NULL;
		return -1;
	}
	*def_ret=this;
	return index;
}

//! Find the index of the field named fname. Doesn't look in subfields.
/*! Compares the chars in fname up to the first '.'. It assumes that fname
 *  does not contain whitespace or start with a '.'. 
 *  Updates next to point to char after the first '.', unless field is not found
 *  in which case next is set to fname, and -1 is returned.
 *  On success, the field index is returned (field index starting at 0). 
 *  If the field string is a number (ie "34.2...") the number is parsed
 *  and returned without bounds checking (though it must be non-negative). This 
 *  number passing is to accommodate sets with variable numbers of
 *  elements, where the number of elements is held in Object, not ObjectDef.
 *
 *  If fname has something like "34ddfdsa.33" then -1 is returned and 
 *  next is set to fname changed. Field names must be all digits, or [a-zA-Z0-9-_] and
 *  start with a letter.
 */
int ObjectDef::findfield(char *fname,char **next) // next=NULL
{
	int n;
	if (extendsdef) {
		n=extendsdef->findfield(fname,next);
		if (n>=0) return n;
	}
		
	char *nxt;
	 // make n the index of the first '.' or end of string
	n=strchrnul(fname,'.')-fname;
	if (n==0) n=strlen(fname);

	 // check for number first: "34.blah.blah"
	if (isdigit(fname[0])) {
		int nn=strtol(fname,&nxt,10); 
		if (nxt!=fname) {
			if (nxt-fname!=n) { if (next) *next=fname; return -1; } // was "34asdfasd" or some such bad input
			if (next) *next=fname + n + (fname[n]=='.'?1:0);
			if (nn>0) return nn;
		}
	} else if (fields) { // else check for field: "blah.3.blah..."
		for (int c=0; c<fields->n; c++) {
			if (!strncmp(fname,fields->e[c]->name,n) && fields->e[c]->name[n]=='\0') {
				if (next) *next=fname + n + (fname[n]=='.'?1:0);
				if (extendsdef) return extendsdef->getNumFields()+c;
				return c;
			}
		}
	}
	return -1;
}




//---------------------------------------- Values --------------------------------------
/*! \class ValueHash
 * \brief Class to aid parsing of functions.
 *
 * Used in LaidoutCalculator.
 */


ValueHash::ValueHash()
	: keys(2)
{}

ValueHash::~ValueHash()
{
	DBG values.flush(); //this should happen automatically anyway
}

int ValueHash::push(const char *name,int i)
{
	keys.push(newstr(name));
	Value *v=new IntValue(i);
	int c=values.push(v);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,double d)
{
	keys.push(newstr(name));
	Value *v=new DoubleValue(d);
	int c=values.push(v);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,const char *value)
{
	keys.push(newstr(name));
	Value *v=new StringValue(value);
	int c=values.push(v);
	v->dec_count();
	return c;
}

//! Create an ObjectValue with obj, and push.
/*! Increments obj count. */
int ValueHash::pushObject(const char *name,Laxkit::anObject *obj)
{
	keys.push(newstr(name));
	Value *v=new ObjectValue(obj);
	int c=values.push(v);
	v->dec_count();
	return c;
}

/*! Increments count on v. */
int ValueHash::push(const char *name,Value *v)
{
	keys.push(newstr(name));
	return values.push(v);
}

/*! Return 0 for success, nonzero for no can do. */
int ValueHash::remove(int i)
{
	if (i<0 || i>=keys.n) return 1;
	keys.remove(i);
	values.remove(i);
	return 0;
}

int ValueHash::n()
{ return keys.n; }

//! Return the Value with index i, or NULL if i is out of bounds.
Value *ValueHash::e(int i)
{
	if (i<0 || i>keys.n) return NULL;
	return values.e[i];
}

void ValueHash::swap(int i1, int i2)
{
	if (i1<0 || i1>keys.n || i2<0 || i2>keys.n) return;
	keys.swap(i1,i2);
	values.swap(i1,i2);
}

//! Return name of key at index i.
const char *ValueHash::key(int i)
{
	if (i<0 || i>=keys.n) return NULL;
	return keys.e[i];
}

//! Return the value at index i.
Value *ValueHash::value(int i)
{
	if (i<0 || i>=keys.n) return NULL;
	return values.e[i];
}

//! Rename key at index i.
void ValueHash::renameKey(int i,const char *newname)
{
	if (i<0 || i>=keys.n) return;
	makestr(keys.e[i],newname);
}

/*! Return 0 for success, or nonzero for error.
 * Increments count of newv.
 */
int ValueHash::set(const char *key, Value *newv)
{
	int i=findIndex(key);
	return set(i,newv);
}

//! Set the value of an existing key to newv.
/*! Return 0 for success, or nonzero for error.
 * Increments count of newv.
 */
int ValueHash::set(int which, Value *newv)
{
	if (which<0 || which>=keys.n) return 1;
	values.e[which]->dec_count();
	newv->inc_count();
	values.e[which]=newv;
	return 0;
}

//! Return the index corresponding to name, or -1 if not found.
int ValueHash::findIndex(const char *name,int len)
{
	for (int c=0; c<keys.n; c++) {
		if (len<0 && !strcmp(name,keys.e[c])) return c;
		if (len>0 && !strncmp(name,keys.e[c],len)) return c;
	}
	return -1;
}

Value *ValueHash::find(const char *name)
{
	for (int c=0; c<keys.n; c++) {
		if (!strcmp(name,keys.e[c])) return values.e[c];
	}
	return NULL;
}

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not an IntValue, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * No cast conversion is done between int and real.
 */
long ValueHash::findInt(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	IntValue *i=dynamic_cast<IntValue*>(values.e[which]);
	if (!i) { if (error_ret) *error_ret=2; return 0; }
	if (error_ret) *error_ret=0;
	return i->i;
}

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a DoubleValue, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * No cast conversion is done between int and real. Use findIntOrDouble() if
 * you don't care about the difference.
 */
double ValueHash::findDouble(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	DoubleValue *d=dynamic_cast<DoubleValue*>(values.e[which]);
	if (!d) { if (error_ret) *error_ret=2; return 0; }
	if (error_ret) *error_ret=0;
	return d->d;
}

//! Return a double value from an IntValue or a DoubleValue.
/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a DoubleValue, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * No cast conversion is done between int and real.
 */
double ValueHash::findIntOrDouble(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }

	DoubleValue *d=dynamic_cast<DoubleValue*>(values.e[which]);
	if (d) {
		if (error_ret) *error_ret=0;
		return d->d;
	}
	IntValue *i=dynamic_cast<IntValue*>(values.e[which]);
	if (i) {
		if (error_ret) *error_ret=0;
		return i->i;
	}

	if (error_ret) *error_ret=2; //for not found
	return 0;
}

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a StringValue, then sets *error_ret=2.
 * Otherwise set to 0.
 */
const char *ValueHash::findString(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	StringValue *s=dynamic_cast<StringValue*>(values.e[which]);
	if (!s) { if (error_ret) *error_ret=2; return NULL; }
	if (error_ret) *error_ret=0;
	return s->str;
}

/*! Does not increment count of the object.
 *  If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not an ObjectValue, then sets *error_ret=2.
 * Otherwise set to 0.
 */
Laxkit::anObject *ValueHash::findObject(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }
	ObjectValue *o=dynamic_cast<ObjectValue*>(values.e[which]);
	if (!o) { if (error_ret) *error_ret=2; return NULL; }
	if (error_ret) *error_ret=0;
	return o->object;
}

//------------------------------------- Value ---------------------------------------
/*! \class Value
 * \brief Base class of internal scripting objects.
 *
 * Used in LaidoutCalculator.
 */

/*! \fn const char *Value::toCChar()
 *
 * Regenerate tempstr to be a cached string representation of the value.
 * When done, set modified to 0.
 */

Value::Value()
	: tempstr(NULL)
{
	modified=1;
}

Value::~Value()
{
	if (tempstr) delete[] tempstr;
}

//! Return a Value's string id, if any. NULL might be returned for unnamed values.
const char *Value::Id()
{ return NULL; }

//! Return the cached string value.
const char *Value::CChar()
{
	if (!tempstr || modified) return toCChar();
	return tempstr;
}

//! Return objectdef, calling makeObjectDef() if necessary.
ObjectDef *Value::GetObjectDef()
{
	if (!objectdef) objectdef=makeObjectDef();
	return objectdef;
}

//! Return the number of top fields in this Value.
/*! This would be redefined for Values that are variable length sets
 *  ObjectDef itself has no way of knowing how many fields there are
 *  and even what kind of fields they are in that case.
 *
 *  Returns objectdef->getNumFields(). Calls makeObjectDef() if necessary
 * 
 * Set and array values will have this redefined to return the number of objects in the set.
 */
int Value::getNumFields()
{
    ObjectDef *def=GetObjectDef();
	if (def) return def->getNumFields();
    return -1;
}

/*! Does not increment the def count. Calling code must do that if it uses it further.
 */
ObjectDef *Value::FieldInfo(int i)
{
    if (i<0 || i>=getNumFields()) return NULL;
    ObjectDef *def=GetObjectDef();
    if (!def) return NULL;
    return def->getField(i);
}

const char *Value::FieldName(int i)
{
    ObjectDef *def=GetObjectDef();
    if (!def) return NULL;
    if (i<0 || i>=getNumFields()) return NULL;
    def=def->getField(i);
    if (!def) return NULL;
    return def->name;
}


//----------------------------- SetValue ----------------------------------
/*! \class SetValue
 */

//! Push val, which increments its count.
/*! Return 0 for success or nonzero for error.
 */
int SetValue::Push(Value *v)
{
	if (!v) return 1;
	if (values.push(v)>=0) return 0;
	return 1;
}

const char *SetValue::toCChar()
{
	makestr(tempstr,"{");
	for (int c=0; c<values.n; c++) {
		appendstr(tempstr,values.e[c]->toCChar());
		if (c!=values.n-1) appendstr(tempstr,",");
	}
	appendstr(tempstr,"}");
	modified=0;
	return tempstr;
}

/*! Returns set with each element duplicate()'d.
 */
Value *SetValue::duplicate()
{
	SetValue *s=new SetValue;
	Value *v;
	for (int c=0; c<values.n; c++) {
		v=values.e[c]->duplicate();
		s->Push(v);
		v->dec_count();
	}
	return s;
}

int SetValue::getNumFields()
{
	return values.n;
}

ObjectDef *SetValue::FieldInfo(int i)
{
	if (i<0 || i>=values.n) return NULL;
	return values.e[i]->GetObjectDef();
}

//! Returns object name, or NULL.
const char *SetValue::FieldName(int i)
{
	if (i<0 || i>=values.n) return NULL;
	return values.e[i]->Id();
}

ObjectDef *SetValue::makeObjectDef()
{
	return NULL;

	//push(value, position)
	//pop(position)
	//swap(p1,p2)
	//slide(p1,p2)  same as push(pop(p1),p2)
	//n
}


//--------------------------------- IntValue -----------------------------
const char *IntValue::toCChar()
{
	if (!tempstr) tempstr=new char[20];
	sprintf(tempstr,"%ld",i);
	modified=0;
	return tempstr;
}

Value *IntValue::duplicate()
{ return new IntValue(i); }

//--------------------------------- DoubleValue -----------------------------
const char *DoubleValue::toCChar()
{
	if (!tempstr) tempstr=new char[30];
	sprintf(tempstr,"%g",d);
	modified=0;
	return tempstr;
}

Value *DoubleValue::duplicate()
{ return new DoubleValue(d); }

//--------------------------------- FlatvectorValue -----------------------------
const char *FlatvectorValue::toCChar()
{
	if (!tempstr) tempstr=new char[40];
	sprintf(tempstr,"(%g,%g)",v.x,v.y);
	modified=0;
	return tempstr;
}

Value *FlatvectorValue::duplicate()
{ return new FlatvectorValue(v); }


//! Compare nonwhitespace until period with field, return 1 for yes, 0 for no.
/*! Return pointer to just after extension.
 */
int extequal(const char *str, const char *field, char **next_ret=NULL)
{
	unsigned int n=0;
	while (isalnum(str[n]) || str[n]=='_') n++;
	if (n!=strlen(field) || strncmp(str,field,n)!=0) {
		if (next_ret) *next_ret=NULL;
		return 0;
	}

	str+=n;
	if (next_ret) *next_ret=const_cast<char*>(str);
	return 1;
}

Value *FlatvectorValue::dereference(const char *extstring)
{
	if (extequal(extstring, "x")) return new DoubleValue(v.x);
	if (extequal(extstring, "y")) return new DoubleValue(v.y);
	return NULL;
}

//--------------------------------- SpacevectorValue -----------------------------
const char *SpacevectorValue::toCChar()
{
	if (!tempstr) tempstr=new char[60];
	sprintf(tempstr,"(%g,%g,%g)", v.x, v.y, v.z);
	modified=0;
	return tempstr;
}

Value *SpacevectorValue::duplicate()
{ return new SpacevectorValue(v); }

Value *SpacevectorValue::dereference(const char *extstring)
{
	if (extequal(extstring, "x")) return new DoubleValue(v.x);
	if (extequal(extstring, "y")) return new DoubleValue(v.y);
	if (extequal(extstring, "z")) return new DoubleValue(v.z);
	return NULL;
}

//--------------------------------- StringValue -----------------------------
//! Create a string value with the first len characters of s.
/*! If len<=0, then use strlen(s).
 */
StringValue::StringValue(const char *s, int len)
{ str=newnstr(s,len); }

const char *StringValue::toCChar()
{
	modified=0;
	return str;
}

Value *StringValue::duplicate()
{ return new StringValue(str); }


//--------------------------------- ObjectValue -----------------------------
/*! Will inc count of obj.
 */
ObjectValue::ObjectValue(anObject *obj)
{
	DBG cerr <<"ObjectValue creation.."<<endl;
	object=obj; 
	if (object) object->inc_count();
}

/*! Objects gets count decremented.
 */
ObjectValue::~ObjectValue()
{
	DBG cerr <<"ObjectValue destructor.."<<endl;
	if (object) object->dec_count();
}

const char *ObjectValue::toCChar()
{
	if (!object) return NULL;
	if (dynamic_cast<Style*>(object)) {
		Style *s=dynamic_cast<Style*>(object);
		if (s->Stylename()) return s->Stylename();
		return s->whattype();
	}
	modified=0;
	return "object(TODO!!)";
}

Value *ObjectValue::duplicate()
{ return new ObjectValue(object); }


} // namespace Laidout

