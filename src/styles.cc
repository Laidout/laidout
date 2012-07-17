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
// Copyright (C) 2004-2009 by Tom Lechner
//


#include <cctype>
#include <cstdlib>
#include <cstdarg>

#include <lax/strmanip.h>

#include "styles.h"
#include "stylemanager.h"

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;



/*! \defgroup stylesandstyledefs Styles and StyleDefs
 * 
 * -------- Styles and StyleDefs -----------
 * 
 *  there are Styles and StyleDefs. StyleDef has a map of the names
 *  of the style and of its fields. Style has actual instances of the 
 *  various fields. Also, each Style can be based on some other Style. 
 *  
 *  For example you might have a character style \n
 *    c1 = italic/bookman/10pt/red/etc., basedon=NULL\n
 *  The StyleDef has field names (say) ItalicFlag/FaceName/Size/Color/etc.
 *  Then say you want a style c2 exactly like c1, but bold too, then the only 
 *  data actually stored in c2 is the BoldFlag, a pointer to c1, and a pointer
 *  to the StyleDef.
 *
 */

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

/*! \typedef Style *(*NewStyleFunc)(StyleDef *def)
 * \ingroup stylesandstyledefs
 * \brief These are in StyleDef to aid in creation of new Style instances by StyleManager.
 */

/*! \ingroup stylesandstyledefs
 * Names for the ElementType enum.
 */
const char *element_TypeNames[16]={
	 	"any",
		"none",
		"int",
		"real",
		"string",
		"object", //call "fields" an "object"
		"boolean",
		"date",
		"file",
		"flag",
		"enum",
		"enumval",
		"color",
		"set",
		"function",
		NULL
	};


//------------------------------ StyleDef --------------------------------------------

/*! \enum ElementType 
 * \ingroup stylesandstyledefs
 * \brief Says what a StyleDef element is.
 */
	
/*! \class StyleDef
 * \ingroup stylesandstyledefs
 * \brief The definition of the elements of a Style.
 *
 * \todo *** haven't worked on this code in quite a while, perhaps it could be somehow
 * combined with the Laxkit::Attribute type of thing? but for that, still need to reason
 * out a consistent Attribute definition standard...
 * 
 * These things are used to automatically create edit dialogs for any data. It also aids in
 * defining names and what they are for an interpreter. Ideally there should be only one instance
 * of a StyleDef per type of style held in a style manager. The actual styles will have pointer
 * references to it. **** still need to work out good framework for this.
 *
 * \todo ***eventually include accepted functions, not just variables??
 * \todo Currently, this stuff is an internationalization nightmare (english only right now).. would have to integrate with
 *   gettext somehow (gotta read up on that!!!)
 * \todo Perhaps the fields stack can be migrated to Laxkit's RefPtrStack.
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
/*! \var ElementType StyleDef::format
 * \brief What is the nature of *this.
 * 
 *   The format of the value of this Styledef. It can be any id that is defined in the value Manager***whatever
 *   that is!!
 *   These formats other than Element_Fields imply that *this is a single unit.
 *   Element_Fields implies that there are further subcomponents.
 */
/*! \var char *StyleDef::name
 *  \brief Basically a class name, meant to be seen in the interpreter.
 */
/*! \var char *StyleDef::Name
 *  \brief Basically a human readable version of the class name.
 *
 *  This is used as a label in automatically created edit windows.
 */
/*! \var char *StyleDef::extends
 *  \brief The StyleDef::name of which StyleDef this one extends.
 *  
 *  If there is an extends, the index of all the fields starts with 0, which
 *	is the very first field in the very first base styledef.
 */
/*! \var StyleDef *StyleDef::extendsdef
 *  \brief Which StyleDef this one extends.
 *  
 *	This is a pointer to a def in a StyleManager. StyleDef looks up extends in
 *	the stylemanager to get the appropriate reference during the constructor.
 *  
 *  If there is an extends, the index of all the fields starts with 0, which
 *	is the very first field in the very first base styledef.
 */
/*! \var NewStyleFunc StyleDef::newfunc
 * \brief Default constructor when the StyleDef represents an object.
 *
 * For full constructor for objects, use stylefunc.
 */
/*! \var StyleFunc StyleDef::stylefunc
 * \brief Callable function for the StyleDef.
 *
 * If the styledef represents an object, then stylefunc is a full constructor,
 * to which you can supply parameters.
 *
 * If the styledef is a function, then this is what is called as the function.
 */


//! Constructor.
StyleDef::StyleDef(const char *nextends, //!< Which StyleDef does this one extend
			const char *nname, //!< The name that would be used in the interpreter
			const char *nName, //!< A basic title, most likely an input label
			const char *ndesc,  //!< Description, newlines ok.
			ElementType fmt,     //!< Format of this StyleDef
			const char *nrange,    //!< String showing range of allowed values
			const char *newdefval,   //!< Default value for the style
			Laxkit::PtrStack<StyleDef> *nfields, //!< StyleDef for the subfields or enum values.
			unsigned int fflags,       //!< New flags
			NewStyleFunc nnewfunc,    //!< Default creation function
			StyleFunc    nstylefunc)  //!< Full Function 
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
StyleDef::~StyleDef()
{
	//DBG cerr <<"StyleDef \""<<name<<"\" destructor"<<endl;
	
	if (extends)      delete[] extends;
	if (name)         delete[] name;
	if (Name)         delete[] Name;
	if (description)  delete[] description;
	if (range)        delete[] range;
	if (defaultvalue) delete[] defaultvalue;
	
	if (extendsdef) {
		//DBG cerr <<" extended: "<<extendsdef->name<<endl;
		extendsdef->dec_count();
	} else {
		//DBG cerr <<"------------no extends"<<endl;
	}

	if (fields) {
		//DBG cerr <<"---deleting styledef fields:"<<endl;
		for (int c=0; c<fields->n; c++) {
			//DBG cerr <<"----f number "<<c<<endl;
			fields->e[c]->dec_count();
		}
		//DBG cerr <<"---Delete fields stack"<<endl;
		delete fields;
		fields=NULL;
	}
}

//! Write out the stuff inside. 
/*! If this styledef extends another, this does not write out the whole
 * def of that, only the name element of it.
 */
void StyleDef::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (name) fprintf(f,"%sname %s\n",spc,name);
	if (Name) fprintf(f,"%sName %s\n",spc,Name);
	if (description) fprintf(f,"%sdescription %s\n",spc,description);
	if (extends) fprintf(f,"%sextends %s\n",spc,extends);
	fprintf(f,"%sflags %u\n",spc,flags);
	fprintf(f,"%sformat %s\n",spc,element_TypeNames[format]);//*** does this actually work right??

	if (fields) {
		for (int c=0; c<fields->n; c++) {
			fprintf(f,"%sfield\n",spc);
			fields->e[c]->dump_out(f,indent+2,0,context);
		}
	}
}

/*! \todo *** imp me!!
 */
void StyleDef::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	cout<<" *** imp me! StyleDef::dump_in_atts(Attribute *att,int flag,context)"<<endl;
	
}

//! Push an Element_EnumVal on an Element_enum.
/*! Return 0 for value pushed or nonzero for error. It is an error to push an
 * enum value on anything but an enum.
 */
int StyleDef::pushEnumValue(const char *str, const char *Str, const char *dsc)
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
int StyleDef::pushEnum(const char *nname,const char *nName,const char *ndesc,
					 const char *newdefval,
					 NewStyleFunc nnewfunc,
					 StyleFunc nstylefunc,
					 ...)
{
	StyleDef *e=new StyleDef(NULL,nname,nName,ndesc,
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
int StyleDef::push(const char *nname,const char *nName,const char *ndesc,
			ElementType fformat,const char *nrange, const char *newdefval,unsigned int fflags,
			NewStyleFunc nnewfunc,
		 	StyleFunc nstylefunc)
{
	StyleDef *newdef=new StyleDef(NULL,nname,nName,
								  ndesc,fformat,nrange,newdefval,
								  NULL,fflags,nnewfunc);
	int c=push(newdef);
	if (c<0) delete newdef;
	return c;
}

//! Push def with fields. If pushing this new field onto fields fails, return 1, else 0
/*! Note that the counts for the subfields are not incremented further.
 */
int StyleDef::push(const char *nname,const char *nName,const char *ndesc,
		ElementType fformat,const char *nrange, const char *newdefval,
		Laxkit::PtrStack<StyleDef> *nfields,unsigned int fflags,
		NewStyleFunc nnewfunc,
		StyleFunc nstylefunc)
{
	StyleDef *newdef=new StyleDef(NULL,nname,nName,ndesc,fformat,nrange,newdefval,nfields,fflags,nnewfunc);
	int c=push(newdef);
	if (c<0) delete newdef;
	return c;
}

//! Push newfield onto fields as not local. Its count is not incremented.
/*! Returns whatever PtrStack::push returns.
 * 
 * Only use pop() when removing elements, as that pops from the stack, then calls
 * its dec_count() function, rather than just deleting.
 */
int StyleDef::push(StyleDef *newfield)
{
	if (!newfield) return 1;
	if (!fields) fields=new PtrStack<StyleDef>;
	return fields->push(newfield,0);
}

//! The element is popped off the fields stack, then thatelement->dec_count() is called.
/*! Returns 1 if item is removed, else 0.
 */
int StyleDef::pop(int fieldindex)
{
	if (!fields || !fields->n || fieldindex<0 || fieldindex>=fields->n) return 0;
	StyleDef *d=fields->pop(fieldindex);
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
  * Consider a StyleDef vector(x,y,z), Now say you define a StyleDef called zenith
  * which is a vector with x,y,z. You want the normal x,y,z definitions, but the
  * head you want to now be "zenith" rather than "vector", So you extend "vector",
  * and do not define any new fields. Viola!
  *
  */
int StyleDef::getNumFields()
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
/*! Given the index of a desired field, this looks up which StyleDef actually has the text
 * and sets the pointers to point to there. Nothing is done if the particular pointer is NULL.
 *
 * def_ret is the StyleDef of index. This can be used to then read off enum names, for instance.
 *
 * If index==-1, then the info from *this is provided, rather than from a fields stack.
 *
 * Returns 0 success, 1 error.
 */
int StyleDef::getInfo(int index,
						const char **nm,
						const char **Nm,
						const char **desc,
						const char **rng,
						const char **defv,
						ElementType *fmt,
						int *objtype,
						StyleDef **def_ret)
{
	StyleDef *def=NULL;
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

//! Return the StyleDef corresponding to the field at index.
/*! If the element at index does not have subfields or is not an enum,
 * then return NULL.
 */
StyleDef *StyleDef::getField(int index)
{
	StyleDef *def=NULL;
	index=findActualDef(index, &def);
	if (index<0 || !def) return NULL;;
	return def->fields->e[index];
}

//! From a given total index, return the actual StyleDef the index lies in, and the index within the returned def.
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
int StyleDef::findActualDef(int index,StyleDef **def_ret)
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
 *  elements, where the number of elements is held in Style, not StyleDef.
 *
 *  If fname has something like "34ddfdsa.33" then -1 is returned and 
 *  next is set to fname changed. Field names must be all digits, or [a-zA-Z0-9-_] and
 *  start with a letter.
 */
int StyleDef::findfield(char *fname,char **next) // next=NULL
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

////**** return FieldMask instead?
////! returns 0 terminated list of indices: "1.4.23" -> { 1,4,23,-1 }
///*! in form of new int[n+1], n=number of subfields
// *  or NULL on error. Index numbers are transferred without modification,
// *  while field names must correspond to a name in the StyleDef.
// *  *** perhaps this should be in Value to allow Value to interperet
// *  the passed in field names. Having separate getfields here is unnecessary?
// *  .. thinking of bitfields where there is a whole slew of names, but they
// *  all control the same (internal) int, plus the mechanism to return a mask
// *  of what changes when you set a value..
// * 
// *  end_ptr is changed to point to NULL on success, or the first invalid
// *  or unparsable field section.
// */  
//int *StyleDef::getfields(char *extstr,char **end_ptr)
//{***
//	int n=1;
//	char *str=extstr;
//	while (*str) { 
//		if (*str=='.') n++; 
//		str++;
//	}
//	int *list=new int[n+1];
//
//	***
//}



//------------------------- Style ------------------------------------


//------------------------------- FieldNode -----------
	
/*! \class FieldNode 
 * \ingroup stylesandstyledefs
 * \brief An internal stack node for Style::fields.
 * 
 * \todo *** this class is a little clunky, need to think more about a good system
 * for interacting with the values of Style objects. such a system must make 
 * interpreter integration a snap, be it my own interpreter or potential python/perl/whatever
 * add ons...
 * 
 * This class is used to wrap which fields a given Style instance has in the
 * form of a union with the typical formats unsigned int, enum (stored as int), int, 
 * bit (stored as int),
 * double, string, fields, Style *, Value * ***include Value yes or no, or other (void *).
 * Any "other" formats must be deciphered some other way.
 *
 * Extraneous FieldNodes such as are returned by Style should be deleted with
 * deleteFieldNode(), which checks to make sure that the node is not held by
 * something else before deleting. If it is held elsewhere, the pointer is simply
 * set to NULL, and nothing is deleted. If it is not held elsewhere, the FieldNode is
 * deleted. In that case, if format is fields, the value.fields are recursively deleted 
 * with deleteFieldNode(). value.str, v, and other are assumed to be held by something
 * else, and are never deleted.*****??have a check for that?
 */
/*! \var int FieldNode::index
 * \brief The index number of the value.
 *
 * ***Please note that for variable sized sets, the index might suddenly become
 * inaccurate, and so must be maintained whenever an element is popped or pushed
 * onto such a set.
 */
/*! \var int FieldNode::format
 * \brief The format of the value.
 *
 * This is the same format number as is in StyleDef::format.
 */
/*! \var char islocal
 * \brief Whether this field node is held somewhere else or stands alone.
 *
 * When a Style returns a FieldNode in response to a query of a field, there is not
 * a guarantee that it is a pointer held by that Style. It might have been a FieldNode
 * created by Style on the fly to hold the field data. If islocal is nonzero, it implies
 * that it is not held by anything, and must be delete'd by the calling code. The contents
 * of value.str, value.otherfields, and value.v are assumed to be held elsewhere and are
 * not deleted. Also if islocal is nonzero, each field value.fields 
 * (if fields is the format) is called with
 * deleteFieldNode(), which deletes or not depending on each field's islocal.
 */
/*! \var unsigned int FieldNode::flags
 * \brief Extra flags for this field, such as read-only.
 *
 * This holds any extra information that might be useful to the Style.
 */
//class FieldNode
//{
// public:
//	int index; // have an int for index, implies cindex=NULL
//	char *cindex; // have a string for index, implies index=-1 ***
//	int format;
//	unsigned int flags; *** thisislocal flag
//	union { 
//		unsigned int f;    // format=0, for (non-exclusive) flags***** must settle on defines for standard formats
//		int i;             // format=1,2 for enums, bit, ints
//		double d;          // format=3
//		char *str;         // format=4
//		FieldNode *fields; // format=5
//		Style *style;      // format=6, this is useful for variable sized sets
//		Value *v;          // format=7
//		void *other;          // format=8 other
//	} value;
//	FieldNode() { value.d=0; } //*** does this really wipe out all?
//};
//--OR--
//class FieldNode
//{
// public:
//	unsigned int flags; *** thisislocal flag
//	int index; // have an int for index, this is consulted before cindex
//	char *cindex; // have a string for index, non-null cindex means can use this rather than index, which is consulted first
//	int format;
//	void *value;
//	PtrStack<FieldNode> *subfields;
//	FieldNode() { cindex=NULL; index=-1; flags=0; format=0; value=NULL; subfields=NULL; }
//};

//! If the FieldNode contains fields, then delete the fields.
/*! **** This sure as hell needs some further thinking... memory leak galore...
 */
void deleteFieldNode(FieldNode *fn)
{
	if (!fn) return;
	if (fn->flags&STYLEDEF_ORPHAN) { //***
		if (fn->format==Element_Fields) {
			delete[] fn->value.fields;
//			--------------assumes FieldNode **fields, and fields is NULL terminated:
//			int c=0;
//			FieldNode *f=fn.value.fields[c];
//			while (f) {
//				deleteFieldNode(f);
//				f=value.fields[c++];
		}
		delete fn;
	}
	fn=NULL;
}

//------------------------------------- Style -------------------------------------------


/*! \class Style
 * \ingroup stylesandstyledefs
 * \brief Abstract base class for styles.
 * 
 *  class Style : public Laxkit::anObject, public LaxFiles::DumpUtility, public Laxkit::RefCounted 
 * 
 *  Styles hold the actual values of a style definition found in a StyleDef.
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
 *  **** some default way to set/access fields that uses all the FieldNode/StyleDef mechanism. This would 
 *  		be beneficial for scripting interface
 * </pre>
 */
/*! \var char *Style::stylename
 * \brief The name of the style.
 * 
 * Note this is not necessarily a variable name as seen by an interpreter, 
 * but it is an instance name.
 * It would be included in a list of available styles, like "Bold Body", and the
 * StyleDef name/Name might be "charstyle"/"Character Style"
 */
/*! \var StyleDef *Style::styledef
 * \brief The StyleDef that gives information about what the Style's fields are.
 *
 *  This points to an original StyleDef that would be kept in a pool somewhere, and
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
/*! \fn StyleDef *Style::makeStyleDef()
 * \brief Construct and return an instance of the relevant StyleDef.
 *
 * Default just returns NULL. This function should otherwise return a unique instance
 * of the relevant StyleDef. Normally, this would then be added to the StyleManager,
 * and other Styles of the same kind would have a reference to that one, not a unique
 * def of their own.
 */
/*! \fn const char *Style::Stylename()
 * \brief Return const pointer to stylename.
 */
/*! \fn int Style::Stylename(const char *nname)
 * \brief Change stylename. Return 1 for changed, 0 for not.
 */


//! Start up with blank slate. All null variables.
Style::Style()
{ 
	basedon=NULL; 
	styledef=NULL; 
	stylename=NULL; 
}

//! Set up with the given styledef, basedon and stylename.
Style::Style(StyleDef *sdef,Style *bsdon,const char *nstn)
{ 
	styledef=sdef; 
	basedon=bsdon; 
	stylename=NULL; 
	makestr(stylename,nstn); 
}

//! Destructor, currently just deletes stylename. styledef and based on assumed non-local.
/*! \todo *** should implement some reference counting thing for Style and StyleDef..
 */
Style::~Style()
{
	////DBG cerr <<"Style \""<<stylename<<"\" destructor"<<endl;
	if (stylename) delete[] stylename;
	if (styledef) styledef->dec_count();
}

int Style::Stylename(const char *nname)
{ makestr(stylename,nname); return 1; }

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
//Style *Style::duplicate(Style *s)//s=NULL
//{
//	//***
//	return NULL;
//}

//! Return the number of top fields in this Style.
/*! This would be redefined for Styles that are variable length sets
 *  StyleDef itself has no way of knowing how many fields there are
 *  and even what kind of fields they are in that case.
 *
 *  Returns styledef->getNumFields(). If styledef==NULL, then return -1;
 *  
 *  \todo *** so what? how is this used in conjunction with StyleDef::getNumFields?
 */
int Style::getNumFields()
{ 
	if (styledef) return styledef->getNumFields(); 
	return -1;
}
	

//-------------------------------- EnumStyle -------------------------------------


/*! \class EnumStyle
 * \brief Convenience class to simplify creation of dynamically created lists in dialogs.
 */

EnumStyle::EnumStyle()
	: names(2)
{}

StyleDef *EnumStyle::makeStyleDef()
{
	cout <<" *** imp me! EnumStyle::makeStyleDef"<<endl;
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


