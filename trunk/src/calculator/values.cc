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
// Copyright (C) 2009-2013 by Tom Lechner
//


#include "values.h"
#include "../styles.h"
#include "stylemanager.h"

#include <lax/strmanip.h>
#include <lax/fileutils.h>
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

/*! \typedef Style *(*NewObjectFunc)(ObjectDef *def)
 * \ingroup stylesandstyledefs
 * \brief These are in ObjectDef to aid in creation of new Style instances by StyleManager.
 */

/*! \ingroup stylesandstyledefs
 * Names for the ValueTypes enum.
 */
const char *element_TypeNames(int type)
{
	if (type==VALUE_Any) return "any";
	if (type==VALUE_None) return "none";
	if (type==VALUE_Set) return "set";
	if (type==VALUE_Object) return "object";
	if (type==VALUE_Int) return "int";
	if (type==VALUE_Real) return "real";
	if (type==VALUE_String) return "string";
	if (type==VALUE_Fields) return "fields";
	if (type==VALUE_Flatvector) return "flatvector";
	if (type==VALUE_Spacevector) return "spacevector";
	if (type==VALUE_File) return "file";
	if (type==VALUE_Flags) return "flags";
	if (type==VALUE_Enum) return "enum";
	if (type==VALUE_EnumVal) return "enumval";
	if (type==VALUE_Color) return "color";
	if (type==VALUE_Date) return "date";
	if (type==VALUE_Time) return "time";
	if (type==VALUE_Boolean) return "boolean";
	if (type==VALUE_Complex) return "complex";

	if (type==VALUE_Variable) return "variable";
	if (type==VALUE_Operator) return "operator";
	if (type==VALUE_Class)    return "class";
	if (type==VALUE_Function) return "function";
	if (type==VALUE_Namespace) return "namespace";
	if (type==VALUE_LValue) return "lvalue";

	return "";
}

//! Return the string name of a ValueTypes.
const char *valueEnumCodeName(int format)
{
	if (format==VALUE_Any)         return "VALUE_Any";
	if (format==VALUE_None)        return "VALUE_None";
	if (format==VALUE_Set)         return "VALUE_Set";
	if (format==VALUE_Object)      return "VALUE_Object";
	if (format==VALUE_Int)         return "VALUE_Int";
	if (format==VALUE_Real)        return "VALUE_Real";
	if (format==VALUE_String)      return "VALUE_String";
	if (format==VALUE_Fields)      return "VALUE_Fields";
	if (format==VALUE_Flatvector)  return "VALUE_Flatvector";
	if (format==VALUE_Spacevector) return "VALUE_Spacevector";
	if (format==VALUE_File)        return "VALUE_File";
	if (format==VALUE_Flags)       return "VALUE_Flags";
	if (format==VALUE_Enum)        return "VALUE_Enum";
	if (format==VALUE_EnumVal)     return "VALUE_EnumVal";
	if (format==VALUE_Color)       return "VALUE_Color";
	if (format==VALUE_Date)        return "VALUE_Date";
	if (format==VALUE_Time)        return "VALUE_Time";
	if (format==VALUE_Boolean)     return "VALUE_Boolean";
	if (format==VALUE_Complex)     return "VALUE_Complex";
	if (format==VALUE_Function)    return "VALUE_Function";
	if (format==VALUE_Array)       return "VALUE_Array";
	if (format==VALUE_Hash)        return "VALUE_Hash";
	if (format==VALUE_LValue)      return "VALUE_LValue";
	return "";
}


//---------------------------------- OpFuncEvaluator ------------------------------------------
/*! \class OpFuncEvaluator
 * \brief Evaluates operators for LaidoutCalculator
 *
 * The Op() function returns 0 for number returned. -1 for no number returned due to not being
 * able to handle the provided parameters. Return 1 for parameters right type, but there
 * is an error with them somehow, and no number returned.
 */


//---------------------------------- FunctionEvaluator ------------------------------------------
/*! \class FunctionEvaluator
 * \brief Class to aid evaluating functions in a LaidoutCalculator.
 */

/*! \function int FunctionEvaluator::Evaluate(const char *func,int len, ValueHash *context,
 * 						 ValueHash *parameters, CalcSettings *settings,
 *						 Value **value_ret,
 *						 ErrorLog *log)
 *	\brief Calculate a function.
 *
 * Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */



//------------------------------ ObjectDef --------------------------------------------

/*! \enum ValueTypes 
 * \ingroup stylesandstyledefs
 * \brief Says what a ObjectDef element is.
 */
	
/*! \class ObjectDef
 * \ingroup stylesandstyledefs
 * \brief The definition of the elements of a Value.
 *
 * These things are used to provide documentation and scripting facility for various kinds
 * of data. They can be used to automatically create edit dialogs, for instance.
 * Ideally there should be only one instance of an ObjectDef per type of data held in a namespace.
 * Individual data will have pointers back to these ObjectDef objects.
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
 *   The description would be used in some sort of help system.
 *  </pre>
 *
 */
/*! \var ValueTypes ObjectDef::format
 * \brief What is the nature of *this.
 * 
 *   The format of the value of this ObjectDef. This is for any built in objects, with VALUE_Fields
 *   meaning it is some compound object, with fieldsformat being a unique identifier for that
 *   compound format.
 */
/*! \var char *ObjectDef::name
 *  \brief Basically a class name, meant to be seen in the interpreter.
 */
/*! \var char *ObjectDef::Name
 *  \brief Basically a human readable version of the class name.
 *
 *  This is used as a label in automatically created edit windows.
 */
/*! \var Laxkit::RefPtrStack<ObjectDef> ObjectDef::extendsdefs
 *  \brief Which ObjectDef(s) this one extends.
 *  
 *	These are pointers to defs. ObjectDef looks up extends in
 *	the global or scope namespace to get the appropriate reference during the constructor.
 *	In c++ terms, all members inherited are virtual public. There is no facility to make them private or protected.
 *  Fields can be overloaded by just having the same name as an element of a parent class.
 */
/*! \var NewObjectFunc ObjectDef::newfunc
 * \brief Default constructor when the ObjectDef represents an object.
 *
 * For full constructor for objects, use stylefunc.
 */
/*! \var ObjectFunc ObjectDef::stylefunc
 * \brief Callable function for the ObjectDef.
 *
 * If the styledef represents an object, then stylefunc is a full constructor,
 * to which you can supply parameters.
 *
 * If the styledef is a function, then this is what is called as the function.
 */


//! Constructor.
/*! Takes possession of fields, and will delete in destructor.
 */
ObjectDef::ObjectDef(const char *nextends, //!< Comma separated list of what this class extends.
			const char *nname, //!< The name that would be used in the interpreter
			const char *nName, //!< A basic title, most likely an input label
			const char *ndesc,  //!< Description, newlines ok.
			ValueTypes fmt,     //!< Format of this ObjectDef
			const char *nrange,    //!< String showing range of allowed values
			const char *newdefval,   //!< Default value for the style
			Laxkit::RefPtrStack<ObjectDef> *nfields, //!< ObjectDef for the subfields or enum values.
			unsigned int fflags,       //!< New flags
			NewObjectFunc nnewfunc,    //!< Default creation function
			ObjectFunc    nstylefunc)  //!< Full Function 
  : suggestions(2)
{
	source_module=NULL;

	newfunc=nnewfunc;
	stylefunc=nstylefunc;
	opevaluator=NULL;
	evaluator=NULL;
	range=defaultvalue=name=Name=description=NULL;
	defaultValue=NULL;

	if (nextends) {
		ObjectDef *extendsdef;
		int n=0;
		char **strs=split(nextends,',',&n);
		char *extends;
		for (int c=0; c<n; c++) {
			extends=strs[c];
			while (isspace(*extends)) extends++;
			while (strlen(extends) && isspace(extends[strlen(extends)-1])) extends[strlen(extends)-1]='\0';
			extendsdef=stylemanager.FindDef(extends); // must look up extends and inc_count()
			if (extendsdef) extendsdefs.pushnodup(extendsdef);
		}
	}
	
	makestr(name,nname);
	makestr(Name,nName);
	makestr(description,ndesc);
	makestr(range,nrange);
	makestr(defaultvalue,newdefval);
	
	fields=nfields;
	format=fmt;
	flags=fflags;
}

//! Create a new VALUE_Variable object def.
ObjectDef::ObjectDef(const char *nname,const char *nName, const char *ndesc, Value *newval)
{
	source_module=NULL;
	newfunc=NULL;
	stylefunc=NULL;
	opevaluator=NULL;
	evaluator=NULL;
	range=defaultvalue=NULL;
	defaultValue=newval;
	if (defaultValue) defaultValue->inc_count();
	flags=0;
	format=VALUE_Variable;
	fieldsformat=(newval?newval->type():0);
	fields=NULL;
}

//! Null creation assumes namespace.
ObjectDef::ObjectDef()
{
	source_module=NULL;
	newfunc=NULL;
	stylefunc=NULL;
	opevaluator=NULL;
	evaluator=NULL;

	range=defaultvalue=NULL;
	defaultValue=NULL;
	flags=0;
	format=VALUE_Namespace;
	fieldsformat=0;
	fields=NULL;
}

//! Delete the various strings, and styledef->dec_count().
ObjectDef::~ObjectDef()
{
	//source_module->dec_count(); <- do NOT do this, we assume the module will outlive the objectdef

	DBG cerr <<"ObjectDef \""<<name<<"\" destructor"<<endl;
	
	//if (extends)      delete[] extends;
	if (name)         delete[] name;
	if (Name)         delete[] Name;
	if (description)  delete[] description;
	if (range)        delete[] range;
	if (defaultvalue) delete[] defaultvalue;
	if (defaultValue) defaultValue->dec_count();
	
	if (fields) {
		DBG cerr <<"---Delete fields stack"<<endl;
		delete fields;
		fields=NULL;
	}
}

char *appendescaped(char *&dest, const char *src, char quote)
{
	if (!src) return dest;

	int n=0;
	for (unsigned int c=0; c<strlen(src); c++) if (src[c]==quote || src[c]=='\n' || src[c]=='\t') n++;
	if (n==0) return appendstr(dest,src);

	char newsrc[strlen(src)+n+1];
	n=strlen(src);
	int i=0;
	int is=0;
	while (is<n) {
		if (src[is]==quote) newsrc[i++]='\\';
		newsrc[i++]=src[is++];
	}
	newsrc[i]='\0';
	return appendstr(dest,newsrc);
}

#define DEFOUT_Indented      0
#define DEFOUT_Script        (-2)
#define DEFOUT_HumanSummary  (-3)
#define DEFOUT_CPP           (-4)
#define DEFOUT_JSON          (-5)

LaxFiles::Attribute *ObjectDef::dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext)
{
	if (what==0 || what==-1) {
		 //Attribute format, ala indented data
		if (!att) att=new LaxFiles::Attribute();

		att->push("name",name);
		att->push("Name",Name);
		att->push("description",description);
		att->push("flags",(int)flags,-1);
		att->push("format",element_TypeNames(format));
		//att->push("fieldsformat",???);

		if (extendsdefs.n) {
			 //output list of classes extended
			char *str=NULL;
			for (int c=0; c<extendsdefs.n; c++) {
				appendstr(str,extendsdefs.e[c]->name);
				if (c!=extendsdefs.n-1) appendstr(str,",");
			}
			att->push("extends",str);
			delete[] str;
		}

		if (fields) {
			for (int c=0; c<fields->n; c++) {
				att->push("field",NULL);
				fields->e[c]->dump_out_atts(att->attributes.e[att->attributes.n-1],what,savecontext);
			}
		}
		return att;

	} else if (what==DEFOUT_Script) {
		 //append a script ready definition to att->value
		char *str=NULL;
		int sub;

		if (format==VALUE_Namespace) {
			appendstr(str,"namespace ");
			sub=0;
			if (name) appendstr(str,name);
			if (Name) { sub++; appendstr(str, "Name:\""); appendstr(str,Name); appendstr(str,"\""); }
			if (description) {
				if (sub) appendstr(str,", ");
				sub++;
				appendstr(str,"doc:\""); appendescaped(str,description,'"'); appendstr(str,"\"");
			}
			appendstr(str," {\n");

		} else if (format==VALUE_Variable) {
			//if (defaultValue) appendstr(str, defaultValue->CChar());
			//***
		} else if (format==VALUE_Operator) {
			//***
		} else if (format==VALUE_Class) {
			//***
		} else if (format==VALUE_Function) {
			//***
		}

	} else if (what==DEFOUT_HumanSummary) {
		//append human readable summary pseudocode to att->value
		char *str=NULL;

		appendstr(str,element_TypeNames(format));
		appendstr(str,name);

		ObjectDef *ff;
		if (fields) {
		  appendstr(str,"(");
		  for (int c=0; c<fields->n; c++) {
			ff=fields->e[c];
			appendstr(str,valueEnumCodeName(ff->format)); appendstr(str," "); appendstr(str,ff->name);
			if (c<fields->n-1) appendstr(str,", ");
		  }
		  appendstr(str,")");
		}

		appendstr(att->value,str);
		delete[] str;

		return att;

	} else if (what==DEFOUT_CPP) {
		//append c++ code snippet to att->value

		char *str=NULL;
		appendstr(str, "ObjectDef *def=new ObjectDef(");
		if (extendsdefs.n) {
			 //output list of classes extended
			appendstr(str,"\"");
			for (int c=0; c<extendsdefs.n; c++) {
				appendstr(str,extendsdefs.e[c]->name);
				if (c!=extendsdefs.n-1) appendstr(str,", ");
			}
			appendstr(str,"\"");
		} else appendstr(str,"NULL, ");
		if (name) { appendstr(str,"\""); appendstr(str,name); appendstr(str,"\", "); } else appendstr(str,"NULL, ");
		if (Name) { appendstr(str,"\""); appendstr(str,Name); appendstr(str,"\", "); } else appendstr(str,"NULL, ");
		if (description) { appendstr(str,"\""); appendstr(str,description); appendstr(str,"\", "); } else appendstr(str,"NULL, ");
		appendstr(str,"\""); appendstr(str,valueEnumCodeName(format)); appendstr(str,"\""); 
		// *** defaultval
		// *** defaultValue
		// *** range
		// *** flags
		appendstr(str,");\n");

		ObjectDef *ff;
		if (fields) for (int c=0; c<fields->n; c++) {
			ff=fields->e[c];
			appendstr(str, "def->push(");
			if (ff->name) { appendstr(str,"\""); appendstr(str,ff->name); appendstr(str,"\", "); } else appendstr(str,"NULL, ");
			if (ff->Name) { appendstr(str,"\""); appendstr(str,ff->Name); appendstr(str,"\", "); } else appendstr(str,"NULL, ");
			if (ff->description) { appendstr(str,"\""); appendstr(str,ff->description); appendstr(str,"\", "); } else appendstr(str,"NULL, ");
			appendstr(str,"\""); appendstr(str,valueEnumCodeName(ff->format)); appendstr(str,"\""); 
			// *** flags
			appendstr(str,");\n");
		}
		cerr <<" *** finish implementing ObjectDef code out!"<<endl;
		return att;
	}

	return NULL;
}


//! Write out the stuff inside. 
/*! If this styledef extends another, this does not write out the whole
 * def of that, only the name element of it.
 */
void ObjectDef::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1 || what==0) {
		//output something like:
		// fieldname example #description of field
		Attribute att;
		dump_out_atts(&att,what,context);
		att.dump_out(f,indent);
		return;

	} else if (what == DEFOUT_Script) {
		 //scripting class definition
		Attribute att;
		dump_out_atts(&att,what,context);
		fprintf(f,"%s%s",spc,att.value);
		return;

	} else if (what == DEFOUT_CPP) {
		 //c++ ObjectDef create code
		fprintf(f,"%sObjectDef *def=new ObjectDef(",spc);
		if (extendsdefs.n) {
			 //output list of classes extended
			fprintf(f,"\"");
			for (int c=0; c<extendsdefs.n; c++) {
				fprintf(f,"%s",extendsdefs.e[c]->name);
				if (c!=extendsdefs.n-1) fprintf(f,", ");
			}
			fprintf(f,"\"");
		} else fprintf(f,"NULL, ");
		if (name) fprintf(f,   "\"%s\", ",name); else fprintf(f,"NULL, ");
		if (name) fprintf(f,   "\"%s\", ",name); else fprintf(f,"NULL, ");
		if (description) fprintf(f,   "\"%s\", ",description); else fprintf(f,"NULL, ");
		fprintf(f,"\"%s\");\n", valueEnumCodeName(format));

		if (fields) for (int c=0; c<fields->n; c++) {
			cerr <<" *** finish implementing ObjectDef code out!"<<endl;
		}
		return;
	}
}

/*! \todo *** imp me!!
 */
void ObjectDef::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	cout<<" *** imp me! ObjectDef::dump_in_atts(Attribute *att,int flag,context)"<<endl;
	
	int what=0;
	if (what==0 || what==-1) {
		//indented
	} else if (what == DEFOUT_Script) {
		//calc
	} else if (what == DEFOUT_CPP) {
		//c++
	} else if (what == DEFOUT_JSON) {
		//json schema
	}
}

Value *ObjectDef::newObject(ObjectDef *def)
{
	if (newfunc) return newfunc(this);
	if (evaluator) {
		Value *v=NULL;

		//	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
		//						 Value **value_ret,ErrorLog *log) = 0;
		evaluator->Evaluate(name,strlen(name), NULL,NULL,NULL, &v, NULL);
		return v;
	}
	return NULL;
}


//! Return the object def of the last pushed, meaning the one currently on the top of the stack.
ObjectDef *ObjectDef::last()
{
	if (!fields || !fields->n) return NULL;
	return fields->e[fields->n-1];
}

//! Push an VALUE_EnumVal on an VALUE_enum.
/*! Return 0 for value pushed or nonzero for error. It is an error to push an
 * enum value on anything but an enum, and that *this must be the enum, not the last pushed.
 */
int ObjectDef::pushEnumValue(const char *str, const char *Str, const char *dsc, int id)
{
	if (format!=VALUE_Enum) return 1;
	if (id==-10000000) id=getNumFields();
	char idstr[30];
	sprintf(idstr,"%d",id);
	return push(str,Str,dsc, 
				VALUE_EnumVal, idstr,NULL,
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
					 NewObjectFunc nnewfunc,
					 ObjectFunc nstylefunc,
					 ...)
{
	ObjectDef *e=new ObjectDef(NULL,nname,nName,ndesc,
							 VALUE_Enum, NULL, newdefval, 
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

//! Add an operator with description and op function.
/*! dir gets put in range such that range has one character corresponding to:
 *  'l' -> OPS_Left,
 *  'r' -> OPS_Right,
 *  '>' -> OPS_LtoR,
 *  '<' -> OPS_RtoL,
 * and priority gets written to defaultvalue.
 */
int ObjectDef::pushOperator(const char *op,int dir,int priority, const char *desc, OpFuncEvaluator *evaluator)
{
	char rr[2];
	if (dir==OPS_Left) *rr='l';
	else if (dir==OPS_Right) *rr='r';
	else if (dir==OPS_LtoR) *rr='>';
	else if (dir==OPS_RtoL) *rr='<';
	rr[1]='\0';

	char pp[20];
	sprintf(pp,"%d",priority);

	ObjectDef *def=new ObjectDef(NULL, op,op,desc, VALUE_Operator, rr, pp,  NULL,0);
	def->opevaluator=evaluator;
	return push(def,1);
}

//! Create a function and pass in all parameter values here.
/*! The '...' are all const char *, in groups of 6, with a single NULL after the last
 * description.
 *
 * - the parameter name,
 * - the parameter name translated, human readable name
 * - the description of the parameter
 * - format (integer)
 * - range (string)
 * - default value (string) 
 *
 * For instance, if you are adding 2 parameters, you must supply 6 const char * values, followed
 * by a single NULL, or all hell will break loose.
 */
int ObjectDef::pushFunction(const char *nname,const char *nName,const char *ndesc,
					 FunctionEvaluator *nfunc,
					 ...)
{
	push(nname,nName,ndesc,
		 VALUE_Function, NULL, NULL, 0,
		 NULL, NULL);
	ObjectDef *f=fields->e[fields->n-1];
	f->evaluator=nfunc;

	va_list ap;
	va_start(ap, nfunc);
	const char *v1,*v2,*v3, *v5,*v6;
	int f4;
	while (1) {
		v1=va_arg(ap,const char *);
		if (v1==NULL) break;
		v2=va_arg(ap,const char *);
		v3=va_arg(ap,const char *);
		f4=va_arg(ap,int);
		v5=va_arg(ap,const char *);
		v6=va_arg(ap,const char *);

		f->pushParameter(v1,v2,v3, (ValueTypes) f4,v5,v6);
	}
	va_end(ap);

	return fields->n-1;
}

//! Push def without fields. If pushing this new field onto fields fails, return -1, else the new field's index.
int ObjectDef::push(const char *nname,const char *nName,const char *ndesc,
			ValueTypes fformat,const char *nrange, const char *newdefval,unsigned int fflags,
			NewObjectFunc nnewfunc,
		 	ObjectFunc nstylefunc)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,
								  ndesc,fformat,nrange,newdefval,
								  NULL,fflags,nnewfunc);
	int c=push(newdef);//absorbs
	return c;
}

//! Push def with fields. If pushing this new field onto fields fails, return 1, else 0
/*! Note that the counts for the subfields are not incremented further.
 */
int ObjectDef::push(const char *nname,const char *nName,const char *ndesc,
		ValueTypes fformat,const char *nrange, const char *newdefval,
		Laxkit::RefPtrStack<ObjectDef> *nfields,unsigned int fflags,
		NewObjectFunc nnewfunc,
		ObjectFunc nstylefunc)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,ndesc,fformat,nrange,newdefval,nfields,fflags,nnewfunc);
	int c=push(newdef);//absorbs
	return c;
}

/*! Return 0 if new variable added.
 * Return -1 if variable existed, and Name, description, and value overwrite the old values.
 * Return 1 if unable add for some reason (like being read only).
 */
int ObjectDef::pushVariable(const char *name,const char *nName, const char *ndesc, Value *v, int absorb)
{
	ObjectDef *def=FindDef(name);
	if (!def) {
		def=new ObjectDef(name,nName,ndesc, v);
		if (absorb) v->dec_count();
		return 0;
	} else {
		makestr(def->Name,nName);
		makestr(def->description,ndesc);
		if (v) v->inc_count();
		if (def->defaultValue) def->defaultValue->dec_count();
		def->defaultValue=v;
		if (absorb) v->dec_count();
		return -1;
	}
	if (absorb) v->dec_count();
	return 1;
}

//! Set the variable, adding a bare one if it does not exist. If name==NULL, then set in *this.
/*! 0 for set and added. -1 for set and was already there. 1 for unable to set.
 *
 * v's count is incremented.
 */
int ObjectDef::SetVariable(const char *name,Value *v, int absorb)
{
	ObjectDef *def=(name ? FindDef(name): this);
	if (!def) {
		def=new ObjectDef(name,name,NULL, v);
		if (absorb) v->dec_count();
		return 0;
	} else {
		if (v) v->inc_count();
		if (def->defaultValue) def->defaultValue->dec_count();
		def->defaultValue=v;
		if (absorb) v->dec_count();
		return -1;
	}
	if (absorb) v->dec_count();
	return 1;
}

//! Add def, and sort into fields stack.
/*! This is just like push(ObjectDef*,int), but with the extra sorting.
 * You might want this if you are creating a namespace, rather than an object, for instance.
 *
 * Return 0 for added. 1 for already there, 2 for not added.
 */
int ObjectDef::AddObjectDef(ObjectDef *def, int absorb)
{
	if (!def) return 1;
	if (!fields) fields=new Laxkit::RefPtrStack<ObjectDef>;
	int i=fields->findindex(def);
	if (i>=0) return 1;
	int c=0;
	if (fields->n) {
		for (c=0; c<fields->n; c++) {
			 //push sorted
			if (strcmp(def->name,fields->e[c]->name)>0) {
				fields->push(def,-1,c);
				break;
			}
		}
		if (c==fields->n) fields->push(def,-1,c);
	} else {
		fields->push(def,-1,c);
	}
	if (absorb) def->dec_count();
	return c;
}

//! Push newfield onto fields as not local. Its count is incremented.
/*! Returns whatever RefPtrStack::push returns.
 * Warning: does not check for duplicate names, only duplicate pointer references.
 *
 * \todo add sorted pushing for faster access
 */
int ObjectDef::push(ObjectDef *newfield, int absorb)
{
	if (!newfield) return 1;
	if (!fields) fields=new Laxkit::RefPtrStack<ObjectDef>;
	int c=fields->pushnodup(newfield);
	if (absorb) newfield->dec_count();
	return c;
}

//! Convenience function to push a parameter field to previously added def.
/*! So, this will push a field onto fields->e[fields->n-1].
 * Aids in easier creation of function definitions.
 *
 * If there is no last item in fields, then create a new fields object. Note that
 * future pushParameter() calls in this case will add to that one, not to *this.
 */
int ObjectDef::pushParameter(const char *nname,const char *nName,const char *ndesc,
			ValueTypes fformat,const char *nrange, const char *newdefval)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,
								  ndesc,fformat,nrange,newdefval,
								  NULL,0,NULL);
	int c;
	if (!fields || !fields->n) c=push(newdef);//absorbs
	else c=fields->e[fields->n-1]->push(newdef);
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

//! Return the number of fields ONLY of this def, not in extendsdefs.
int ObjectDef::getNumFieldsOfThis()
{
	if (!fields) return 0;
	return fields->n;
}

//! Return the fields at index of *this, NOT one that's in extendsdefs. NULL if not found.
ObjectDef *ObjectDef::getFieldOfThis(int index)
{
	if (!fields) return NULL;
	if (index<0 || index>=fields->n) return NULL;
	return fields->e[index];
}

 //! Returns the number of upper most fields that this styledef contains.
 /*! If this styledef is an extension of another, then the number returned is
  * the total number of fields defined in *this plus all the upper fields in
  * the extended styledef(s). Each styledef simply adds fields->n if fields
  * exists, or 0 if fields does not exists and extendsdef exists, or 1 if neither fields 
  * nor extendsdef exist. 
  *
  * A special exception is when format==VALUE_Enum. In that case, fields would
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

	if (format==VALUE_Enum) {
		if (!extendsdefs.n) n++;
	} else {
		if (fields && fields->n) n+=fields->n; 
			else if (!extendsdefs.n) n++;
		if (extendsdefs.n) {
			for (int c=0; c<extendsdefs.n; c++) 
				n+=extendsdefs.e[c]->getNumFields();
		}
	}
	return n; 
}

//! Return a new object of the specified type. Object def needs to have the newfunc defined.
Value *ObjectDef::newValue(const char *objectdef)
{
    ObjectDef *s=FindDef(objectdef,2);
    if (!s) return NULL;
    if (s->newfunc) return s->newfunc(s);
	return NULL;
}

//! Return the def with the given name.
/*! This returns the pointer, but does NOT increase the count on it.
 *  
 * If which&1, then only look in functions.
 * If which&2, only look in objects.
 * If which&4 then only look in variables.
 * Default is to look in all. The returned def MUST be a function, class, or variable.
 *
 * \todo Should probably optimize this for searching.. have list of sorted field names?
 */
ObjectDef *ObjectDef::FindDef(const char *objectdef, int len, int which)
{   
	if (!fields) return NULL;
	for (int c=0; c<fields->n; c++) {
		if (!strncmp(objectdef,fields->e[c]->name,len) && strlen(fields->e[c]->name)==(unsigned int)len) {
			if ((which&1) && fields->e[c]->format==VALUE_Function) return fields->e[c];
			else if ((which&2) && fields->e[c]->format==VALUE_Class) return fields->e[c];
			else if ((which&4) && fields->e[c]->format==VALUE_Variable) return fields->e[c];
		}
	}
	return NULL;
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
						ValueTypes *fmt,
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
	if (index==0 && (format==VALUE_Enum || (!extendsdefs.n && (!fields || !fields->n)))) {
		*def_ret=this;
		return 0;
	}
	 // else there should be fields somewhere
	int n=0;
	if (extendsdefs.n) {
		ObjectDef *extendsdef;
		for (int c=0; c<extendsdefs.n; c++) {
			extendsdef=extendsdefs.e[c];
			n+=extendsdef->getNumFields(); //counts all fields in extensions
			if (index<n) { // index lies in extendsdef somewhere
				return extendsdef->findActualDef(index,def_ret);
			} 	
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
	if (extendsdefs.n) {
		ObjectDef *extendsdef;
		for (int c=0; c<extendsdefs.n; c++) {
			extendsdef=extendsdefs.e[c];
			n=extendsdef->findfield(fname,next);
			if (n>=0) return n;
		}
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
			if (!strncmp(fname,fields->e[c]->name,n) && strlen(fields->e[c]->name)==strlen(fname)) {
				if (next) *next=fname + n + (fname[n]=='.'?1:0);
				int cc=c;
				if (extendsdefs.n) {
					cc+=extendsdefs.e[c]->getNumFields();
				}
				return cc;
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
{
	sorted=0;
}

ValueHash::~ValueHash()
{
	DBG values.flush(); //this should happen automatically anyway
}

int ValueHash::push(const char *name,int i)
{
	Value *v=new IntValue(i);
	int c=push(name,v);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,double d)
{
	Value *v=new DoubleValue(d);
	int c=push(name,v);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,const char *value)
{
	Value *v=new StringValue(value);
	int c=push(name,v);
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
	int place=keys.n;
	if (sorted) {
		for (place=0; place<keys.n; place++) {
			if (strcmp(name,keys.e[place])>=0) {
				break;
			}
		}
	}
	keys.push(newstr(name),-1,place);
	return values.push(v,-1,place);
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

Value::Value()
{
	modified=1;
}

Value::~Value()
{ }

//! Return a Value's string id, if any. NULL might be returned for unnamed values.
/*! Default is to return this->object_idstr.
 */
const char *Value::Id()
{ return object_idstr; }

//! Output to buffer, do NOT reallocate buffer, assume it has enough space. If len is not enough, return how much is needed.
/*! Generally, subclasses should redefine this, and not the other getValueStr().
 */
int Value::getValueStr(char *buffer,int len)
{
	if (!buffer || len<(int)strlen(whattype()+1)) return strlen(whattype())+1;

	sprintf(buffer,"%s",whattype());
	return -1;
}

/*! Generally, subclasses should NOT redefine this. Redefine the other one.
 * This one handles reallocation, then calls the other one.
 *
 *  If oktoreallocate, then change buffer and len to be big enough to contain a string
 * representation, and return 0. If !oktoreallocate, return the necessary size of buffer, and render nothing.
 *
 * Return -1 if unable to render (the default, unless subclasses redefine), and it returns whattype().
 */
int Value::getValueStr(char **buffer,int *len, int oktoreallocate)
{
	int needed=getValueStr(NULL,0);
	if (*len<needed) {
		if (!oktoreallocate) return needed;
		if (*buffer) delete[] *buffer;
		*buffer=newnstr(NULL,needed);
		*len=needed;
	}
	return getValueStr(*buffer,*len);
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

/*! Default will not output the Value's id string. The object calling this class should be doing that.
 */
void Value::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	if (what==-1) {
		 //dump out object def
    	ObjectDef *def=GetObjectDef();
		if (!def) {
			DBG cerr << "  missing ObjectDef for "<<whattype()<<endl;
		} else def->dump_out(f,indent,-1,context);
		return;
	}

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	ObjectDef *def;
	const char *str;
	char *buffer=NULL;
	int len=0;
	for (int c=0; c<getNumFields(); c++) {
		fprintf(f,"%s%s",spc,FieldName(c));
		def=FieldInfo(c); //this is the object def of a field. If it exists, then this element has subfields.
		if (!def) {
			getValueStr(&buffer,&len, 1);
			str=buffer;
			if (str) {
				fprintf(f," %s\n",str);
			}
		} else {
			if (def->format==VALUE_Function) continue; //output values only, not functions
		}
	}
}

void Value::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{ //  ***
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

int SetValue::getValueStr(char *buffer,int len)
{
	int needed=3;//"{}\n"
	for (int c=0; c<values.n; c++) {
		needed+= 1 + values.e[c]->getValueStr(NULL,0,0);
	}
	if (!buffer || len<needed) return needed;

	int pos=1;
	sprintf(buffer,"{");
	for (int c=0; c<values.n; c++) {
		values.e[c]->getValueStr(buffer+pos,len);
		if (c!=values.n-1) strcat(buffer+pos,",");
		pos+=strlen(buffer+pos);
	}
	strcat(buffer+pos,"}");
	modified=0;
	return 0;
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

//----------------------------- ArrayValue ----------------------------------
/*! \class ArrayValue
 */

ArrayValue::ArrayValue(const char *elementtype, int size)
{
	element_type=newstr(elementtype);
	fixed_size=size;
}

ArrayValue::~ArrayValue()
{
	if (element_type) delete[] element_type;
}

ObjectDef *ArrayValue::makeObjectDef()
{
	return NULL;
}

int ArrayValue::getValueStr(char *buffer,int len)
{
	int needed=3;//"[]\n"
	for (int c=0; c<values.n; c++) {
		needed+= 1 + values.e[c]->getValueStr(NULL,0,0);
	}
	if (!buffer || len<needed) return needed;

	int pos=1;
	sprintf(buffer,"[");
	for (int c=0; c<values.n; c++) {
		values.e[c]->getValueStr(buffer+pos,len);
		if (c!=values.n-1) strcat(buffer+pos,",");
		pos+=strlen(buffer+pos);
	}
	strcat(buffer+pos,"]");
	modified=0;
	return 0;
}

/*! Returns set with each element duplicate()'d.
 */
Value *ArrayValue::duplicate()
{
	ArrayValue *s=new ArrayValue;
	Value *v;
	for (int c=0; c<values.n; c++) {
		v=values.e[c]->duplicate();
		s->Push(v);
		v->dec_count();
	}
	return s;
}

//! Return number of common dimensions.
/*! If subfields are also arrays of the same dimension as each other, then that row counts as 1.
 */
int ArrayValue::Dimensions()
{
	//***;
	return 1;
}


//--------------------------------- BooleanValue -----------------------------
int BooleanValue::getValueStr(char *buffer,int len)
{
	if (!buffer || len<6) return 6;
	if (i) sprintf(buffer,"true");
	else sprintf(buffer,"false");
	return 0;
}

Value *BooleanValue::duplicate()
{ return new BooleanValue(i); }

//--------------------------------- IntValue -----------------------------
int IntValue::getValueStr(char *buffer,int len)
{
	if (!buffer || len<20) return 20;

	sprintf(buffer,"%ld",i);
	modified=0;
	return 0;
}

Value *IntValue::duplicate()
{ return new IntValue(i); }

//--------------------------------- DoubleValue -----------------------------
int DoubleValue::getValueStr(char *buffer,int len)
{
	int needed=30;
	if (!buffer || len<needed) return needed;

	sprintf(buffer,"%g",d);
	modified=0;
	return 0;
}

Value *DoubleValue::duplicate()
{ return new DoubleValue(d); }

//--------------------------------- FlatvectorValue -----------------------------
int FlatvectorValue::getValueStr(char *buffer,int len)
{
	int needed=60;
	if (!buffer || len<needed) return needed;

	sprintf(buffer,"(%g,%g)",v.x,v.y);
	modified=0;
	return 0;
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
int SpacevectorValue::getValueStr(char *buffer,int len)
{
	int needed=90;
	if (!buffer || len<needed) return needed;

	sprintf(buffer,"(%g,%g,%g)", v.x, v.y, v.z);
	modified=0;
	return 0;
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

int StringValue::getValueStr(char *buffer,int len)
{
	int needed=strlen(str)+1;
	if (!buffer || len<needed) return needed;

	strcpy(buffer,str);
	modified=0;
	return 0;
}

Value *StringValue::duplicate()
{ return new StringValue(str); }

//--------------------------------- FileValue -----------------------------
//! Create a value of a file location.
FileValue::FileValue(const char *f, int len)
{ filename=newnstr(f,len>0?len:(f?strlen(f):0)); }

FileValue::~FileValue()
{
	if (filename) delete[] filename;
}

int FileValue::getValueStr(char *buffer,int len)
{
	int needed=strlen(filename)+1;
	if (!buffer || len<needed) return needed;

	strcpy(buffer,filename);
	modified=0;
	return 0;
}


Value *FileValue::duplicate()
{ return new FileValue(filename); }

int FileValue::fileType()
{
	//file link, dir link, file, dir, block
	return file_exists(filename,1,NULL);
}

int FileValue::isLink()
{
	if (!filename) return 0;
	struct stat statbuf;
	stat(filename, &statbuf);
	return S_ISLNK(statbuf.st_mode&S_IFMT);
}

int FileValue::Exists()
{
	return file_exists(filename,1,NULL);
}

//--------------------------------- EnumValue -----------------------------
/*! \class EnumValue
 * \brief Value for a particular one of an enum.
 *
 * The ObjectDef for the enum is always stored with the value.
 */

//! Create a value corresponding to a particular enum field.
/*! baseenum is incremented.
 */
EnumValue::EnumValue(ObjectDef *baseenum, int which)
{
	if (objectdef) objectdef->dec_count();
	objectdef=baseenum;
	if (objectdef) objectdef->inc_count();
	value=which;
}

EnumValue::~EnumValue()
{
	if (objectdef) objectdef->dec_count();
}

int EnumValue::getValueStr(char *buffer,int len)
{
	if (!objectdef) return Value::getValueStr(buffer,len);
	const char *str=NULL;
	objectdef->getInfo(value,&str);

	int needed=strlen(str)+1;
	if (!buffer || len<needed) return needed;

	strcpy(buffer,str);

	modified=0;
	return 0;
}

Value *EnumValue::duplicate()
{ return new EnumValue(objectdef,value); }

//! Returns enumdef.
ObjectDef *EnumValue::makeObjectDef()
{
	return enumdef;
}



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

int ObjectValue::getValueStr(char *buffer,int len)
{
	if (!object) return Value::getValueStr(buffer,len);
	const char *str=NULL;
	if (dynamic_cast<Style*>(object)) str=dynamic_cast<Style*>(object)->Stylename();
	if (!str) str=object->whattype();
	if (!str) str="object(TODO!!)";

	int needed=strlen(str)+1;
	if (!buffer || len<needed) return needed;

	strcpy(buffer,str);
	modified=0;
	return 0;
}

Value *ObjectValue::duplicate()
{ return new ObjectValue(object); }


//--------------------------------- FunctionValue -----------------------------

//----------------------------- ObjectDef utils ------------------------------------------

//! Map a ValueHash to a function ObjectDef.
/*! \ingroup stylesandstyledefs
 *
 * This is useful for function calls or object creating from a basic script,
 * and makes it easier to map parameters to actual object methods.
 *
 * Note that this modifies the contents of rawparams. It does not create a duplicate.
 *
 * On success, it will return the value of rawparams, or NULL if there is any error.
 *
 * Something like "paper=letter,imposition.net.box(width=5,3,6), 3 pages" will be parsed 
 * by parse_fields into an attribute like this:
 * <pre>
 *  paper letter
 *  - imposition
 *  - 3 pages
 * </pre>
 *
 * Now say we have a ObjectDef something like:
 * <pre>
 *  field 
 *    name numpages
 *    format int
 *  field
 *    name paper
 *    format PaperType
 *  field
 *    name imposition
 *    format Imposition
 * </pre>
 *
 * Now paper is the only named parameter. It will be moved to position 1 to match
 * the position in the styledef. The other 2 are not named, so we try to guess. If
 * there is an enum field in the styledef, then the value of the parameter, "imposition" 
 * for instance, is searched for in the enum values. If found, it is moved to the proper
 * position, and labeled accordingly. Any remaining unclaimed parameters are mapped
 * in order to the unused styledef fields.
 *
 * So the mapping of the above will result in rawparams having:
 * <pre>
 *  numpages 3 pages
 *  paper letter
 *  imposition imposition
 * </pre>
 *
 * \todo enum searching is not currently implemented
 */
ValueHash *MapParameters(ObjectDef *def,ValueHash *rawparams)
{
	if (!rawparams || !def) return NULL;
	int n=def->getNumFields();
	int c2;
	const char *name;
	const char *k;

	 //now there are the same number of elements in rawparams and the styledef
	 //we go through from 0, and swap as needed
	for (int c=0; c<n; c++) { //for each styledef field
		def->getInfo(c,&name,NULL,NULL);
		if (c>=rawparams->n()) rawparams->push(name,(Value*)NULL);

		//if (format==VALUE_DynamicEnum || format==VALUE_Enum) {
		//	 //rawparam att name could be any one of the enum names
		//}

		for (c2=c; c2<rawparams->n(); c2++) { //find the parameter named already
			k=rawparams->key(c2);
			if (!k) continue; //skip unnamed
			if (!strcmp(k,name)) {
				 //found field name match between a parameter and the styledef
				if (c2!=c) {
					 //parameter in the wrong place, so swap with the right place
					rawparams->swap(c2,c);
				} // else param was in right place
				break;
			}
		}
		if (c2==rawparams->n()) {
			 // did not find a matching name, so grab the 1st NULL named parameter
			for (c2=c; c2<rawparams->n(); c2++) {
				k=rawparams->key(c2);
				if (k) continue;
				if (c2!=c) {
					 //parameter in the wrong place, so swap with the right place
					rawparams->swap(c2,c);
				} // else param was in right place
				rawparams->renameKey(c,name); //name the parameter correctly
			}
			if (c2==rawparams->n()) {
				 //there were extra parameters that were not known to the styledef
				 //this breaks the transfer, return NULL for error
				return NULL;
			}
		}
	}
	return rawparams;
}

//! Return a number from a real, integer, or boolean.
/*! Booleans are always 1 or 0.
 * Reals set isnum=1.
 * Ints set isnum=2.
 * Booleans set isnum=3.
 * Otherwise isnum=0.
 */
double getNumberValue(Value *v, int *isnum)
{
	if (v->type()==VALUE_Real) {
		*isnum=1;
		return dynamic_cast<DoubleValue*>(v)->d;
	} else if (v->type()==VALUE_Int) {
		*isnum=2;
		return dynamic_cast<IntValue*>(v)->i;
	} else if (v->type()==VALUE_Boolean) {
		*isnum=3;
		return dynamic_cast<BooleanValue*>(v)->i;
	}
	*isnum=0;
	return 0;
}

} // namespace Laidout

