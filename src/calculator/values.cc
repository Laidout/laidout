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
// Copyright (C) 2009-2014 by Tom Lechner
//


#include "values.h"
#include "../language.h"

#include <lax/strmanip.h>
#include <lax/fileutils.h>

//template implementation
#include <lax/refptrstack.cc>


#include <cctype>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <iostream>

#define DBG
using namespace std;


using namespace Laxkit;
using namespace LaxFiles;



namespace Laidout {





//----------------------------- typedef docs -----------------------------


/*! \typedef Value *(*NewObjectFunc)()
 * \ingroup stylesandstyledefs
 * \brief These are in ObjectDef to aid in creation of new Value objects.
 */

/*! \ingroup stylesandstyledefs
 * Names for the ValueTypes enum.
 */
const char *element_TypeNames(int type)
{
	if (type==VALUE_Any)         return "any";
	if (type==VALUE_None)        return "none";
	if (type==VALUE_Set)         return "set";
	if (type==VALUE_Object)      return "object";
	if (type==VALUE_Int)         return "int";
	if (type==VALUE_Real)        return "real";
	if (type==VALUE_Number)      return "number";
	if (type==VALUE_String)      return "string";
	if (type==VALUE_Fields)      return "fields";
	if (type==VALUE_Flatvector)  return "flatvector";
	if (type==VALUE_Spacevector) return "spacevector";
	if (type==VALUE_File)        return "File"; //this one is capitalized, as it is a bit specialized
	if (type==VALUE_Flags)       return "flags";
	if (type==VALUE_Enum)        return "enum";
	if (type==VALUE_EnumVal)     return "enumval";
	if (type==VALUE_Color)       return "color";
	if (type==VALUE_Date)        return "date";
	if (type==VALUE_Time)        return "time";
	if (type==VALUE_Boolean)     return "boolean";
	if (type==VALUE_Complex)     return "complex";

	if (type==VALUE_Variable)    return "variable";
	if (type==VALUE_Operator)    return "operator";
	if (type==VALUE_Class)       return "class";
	if (type==VALUE_Function)    return "function";
	if (type==VALUE_Namespace)   return "namespace";
	if (type==VALUE_LValue)      return "lvalue";

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
	if (format==VALUE_Number)      return "VALUE_Number";
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

//! Return the ValueTypes of the string. Non recognized names return VALUE_Fields.
ValueTypes element_NameToType(const char *type)
{
	if (!type) return VALUE_Any;

	if (!strcmp(type,"any"))         return VALUE_Any;
	if (!strcmp(type,"none"))        return VALUE_None;
	if (!strcmp(type,"set"))         return VALUE_Set;
	if (!strcmp(type,"object"))      return VALUE_Object;
	if (!strcmp(type,"boolean"))     return VALUE_Boolean;
	if (!strcmp(type,"int"))         return VALUE_Int;
	if (!strcmp(type,"real"))        return VALUE_Real;
	if (!strcmp(type,"number"))      return VALUE_Number;
	if (!strcmp(type,"string"))      return VALUE_String;
	if (!strcmp(type,"flatvector"))  return VALUE_Flatvector;
	if (!strcmp(type,"spacevector")) return VALUE_Spacevector;
	if (!strcmp(type,"flags"))       return VALUE_Flags;
	if (!strcmp(type,"enum"))        return VALUE_Enum;
	if (!strcmp(type,"enumval"))     return VALUE_EnumVal;

	if (!strcmp(type,"color"))       return VALUE_Color;
	if (!strcmp(type,"File"))        return VALUE_File;
	if (!strcmp(type,"date"))        return VALUE_Date;
	if (!strcmp(type,"time"))        return VALUE_Time;
	if (!strcmp(type,"complex"))     return VALUE_Complex;

	if (!strcmp(type,"fields"))      return VALUE_Fields;

	if (!strcmp(type,"variable"))    return VALUE_Variable;
	if (!strcmp(type,"operator"))    return VALUE_Operator;
	if (!strcmp(type,"class"))       return VALUE_Class;
	if (!strcmp(type,"function"))    return VALUE_Function;
	if (!strcmp(type,"namespace"))   return VALUE_Namespace;
	if (!strcmp(type,"lvalue"))      return VALUE_LValue;

	return VALUE_Fields;
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
 *  0 for success, value optionally returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */


SimpleFunctionEvaluator::SimpleFunctionEvaluator(ObjectFunc func)
{
	newfunc=NULL;
	function=func;
}

/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int SimpleFunctionEvaluator::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (!function) return 1;

	//typedef int (*ObjectFunc)(ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog &log);
	return function(context, parameters, value_ret, *log);
}


//------------------------ ValueConstraint --------------------------------

/*! \class ValueConstraint
 * Define a constraint that can be used to clamp values, or otherwise help make Value
 * objects fit constraints. Base class is built for integers and doubles. More specialized
 * behavior needs to be subclassed.
 */

/*! A single range like:
 * <pre>
 *   "(0]"      unbounded minimum, hard max of 0
 *   "[0)"      hard min of 0, unbounded maximum
 *   "()"       no bounds, same as "" or NULL.
 *   "(1)"      same as (), but min and max set to 1 instead of 0
 *   "[0, 1]"   hard minimum 0, hard max of 1
 *   "[.1, .9)" hard minimum .1, soft max of .9
 *   "(1,9]"    soft minimum 1 (just an interface hint, will not accually clamp), hard max of 9.
 * </pre>
 *
 * Return 0 for successfull set, or nonzero for parsing error.
 */
int ValueConstraint::SetBounds(const char *bounds)
{
	while (isspace(*bounds)) bounds++;

	if (!isblank(bounds) || !strncmp(bounds, "()", 2)) {
		mintype = PARAM_No_Minimum;
		maxtype = PARAM_No_Maximum;
		return 0;
	}

	if (*bounds == '(') mintype = PARAM_No_Minimum;
	else if (*bounds == '[') mintype = PARAM_Min_Clamp;
	else return 1;
	bounds++;

	char *end=NULL;
	double d = strtod(bounds, &end), d2 = 0;
	int n = 0;
	if (end != bounds) {
		n++;
		bounds = end;
		while (isspace(*bounds)) bounds++;
		if (*bounds == ',') {
			bounds++;
			d2 = strtod(bounds, &end), d2 = 0;
			if (bounds == end) {
				return 2;
			}
			n++;
		}
	}

	while (isspace(*bounds)) bounds++;

	if (*bounds == ')') maxtype = PARAM_No_Maximum;
	else if (*bounds == ']') maxtype = PARAM_Max_Clamp;

	if (n==0) {
		if (mintype != PARAM_No_Minimum || maxtype != PARAM_No_Maximum) return 3;
		min = max = 0;

	} else if (n==1) {
		min = max = d;

	} else {
		min = d;
		max = d2;
		if (mintype == PARAM_No_Minimum && maxtype == PARAM_No_Maximum) {
			mintype = PARAM_Min_Loose_Clamp;
			maxtype = PARAM_Max_Loose_Clamp;
		} else if (mintype == PARAM_No_Minimum && maxtype == PARAM_Max_Clamp) {
			mintype = PARAM_Min_Loose_Clamp;
		} else if (mintype == PARAM_Min_Clamp && maxtype == PARAM_No_Maximum) {
			maxtype = PARAM_Max_Loose_Clamp;
		}
	}

	return 0;
}

int ValueConstraint::SlideInt(int oldvalue, double numsteps)
{
	int newvalue = oldvalue;

	if (numsteps) {
		if (steptype == PARAM_Step_Add) newvalue += numsteps * step;
		else if (steptype == PARAM_Step_Adaptive_Add) newvalue += numsteps * step * (fabs(newvalue)>1 ? int(log10(fabs(newvalue)))*10+1 : 1);
		else if (steptype == PARAM_Step_Mult) {
			int vv = newvalue;
			if (newvalue == 0) {
				if (numsteps>0) newvalue = step-1; else newvalue = 1-step; //assumes step is like 1.1
			} else {
				if (numsteps>0) newvalue *= numsteps * step;
				else newvalue /= -numsteps * step;
			}
			if (vv == newvalue) {
				 //make sure it always moves by at least one
				if (numsteps>0) newvalue = vv+1;
				else newvalue = vv-1;
			}
		}
	}

	if (mintype == PARAM_Min_Clamp && newvalue < min) newvalue = min;
	else if (maxtype == PARAM_Max_Clamp && newvalue > max) newvalue = max;

	return newvalue;
}

double ValueConstraint::SlideDouble(double oldvalue, double numsteps)
{
	double newvalue = oldvalue;

	if (numsteps) {
		if (steptype == PARAM_Step_Add) newvalue += numsteps * step;
		else if (steptype == PARAM_Step_Adaptive_Add) newvalue += numsteps * step * (fabs(newvalue)>1 ? int(log10(fabs(newvalue)))*10+1 : 1);
		else if (steptype == PARAM_Step_Mult) {
			double vv = newvalue;
			if (newvalue == 0) {
				if (numsteps>0) newvalue = step-1; else newvalue = 1-step; //assumes step is like 1.1
			} else {
				if (numsteps>0) newvalue *= numsteps * step;
				else newvalue /= -numsteps * step;
			}
			if (vv == newvalue) {
				 //make sure it always moves by at least one
				if (numsteps>0) newvalue = vv+step;
				else newvalue = vv-step;
			}
		}
	}

	if (mintype == PARAM_Min_Clamp && newvalue < min) newvalue = min;
	else if (maxtype == PARAM_Max_Clamp && newvalue > max) newvalue = max;

	return newvalue;
}


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
 *   compound format which by default is (VALUE_MaxBuiltIn+object_id).
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


//! Null creation assumes namespace.
ObjectDef::ObjectDef()
{
	name         = NULL;
	Name         = NULL;
	description  = NULL;

	parent_namespace = NULL;
	newfunc      = NULL;
	stylefunc    = NULL;
	opevaluator  = NULL;
	evaluator    = NULL;

	range = defaultvalue = NULL;
	defaultValue = NULL;
	flags        = 0;
	format       = VALUE_Namespace;
	fieldsformat = VALUE_MaxBuiltIn + object_id;
	fieldsdef    = NULL;
	fields       = NULL;
	format_str   = NULL;
	islist       = 0; //if this element should be considered a set of such elements

	uihint       = NULL;
	extrainfo    = NULL;
}

//! Constructor.
/*! Takes possession of fields, and will delete in destructor.
 */
ObjectDef::ObjectDef(ObjectDef *nextends, //!< Definition of a class to derive from
			const char *nname, //!< The name that would be used in the interpreter
			const char *nName, //!< A basic title, most likely an input label
			const char *ndesc,  //!< Description, newlines ok.
			const char *fmt,     //!< Format of this ObjectDef
			const char *nrange,    //!< String showing range of allowed values
			const char *newdefval,   //!< Default value for the style
			Laxkit::RefPtrStack<ObjectDef> *nfields, //!< ObjectDef for the subfields or enum values.
			unsigned int fflags,       //!< New flags
			NewObjectFunc nnewfunc,    //!< Default creation function
			ObjectFunc    nstylefunc)  //!< Full Function
  : suggestions(2)
{
	DBG cerr <<"..creating ObjectDef "<<nname<<endl;

	parent_namespace=NULL;

	newfunc=nnewfunc;
	stylefunc=nstylefunc;
	opevaluator=NULL;
	evaluator=NULL;
	defaultValue=NULL;
	range=defaultvalue=name=Name=description=NULL;

	makestr(name,nname);
	makestr(Name,nName);
	makestr(description,ndesc);
	makestr(range,nrange);
	makestr(defaultvalue,newdefval);


	fields=nfields;
	fieldsformat=VALUE_MaxBuiltIn + object_id;
	fieldsdef=NULL;
	format=element_NameToType(fmt);
	format_str=newstr(fmt);
	flags=fflags;
	islist=0; //if this element should be considered a set of such elements

	uihint=NULL;
	extrainfo=NULL;

	if (nextends) {
		Extend(nextends);

		//-----vv this depends on stylemanager, which is specific to Laidout.. trying to isolate that..
//		ObjectDef *extendsdef;
//		int n=0;
//		char **strs=split(nextends,',',&n);
//		char *extends;
//		for (int c=0; c<n; c++) {
//			extends=strs[c];
//			while (isspace(*extends)) extends++;
//			while (strlen(extends) && isspace(extends[strlen(extends)-1])) extends[strlen(extends)-1]='\0';
//			extendsdef=stylemanager.FindDef(extends); // must look up extends and inc_count()
//			if (extendsdef) extendsdefs.pushnodup(extendsdef);
//		}
	}

}

//! Create a new VALUE_Variable object def.
ObjectDef::ObjectDef(const char *nname,
					 const char *nName,
					 const char *ndesc,
					 Value *newval,
					 const char *type, unsigned int fflags)
{
	DBG cerr <<"..creating ObjectDef "<<nname<<endl;

	name=newstr(nname);
	Name=newstr(nName);
	description=newstr(ndesc);

	parent_namespace=NULL;
	newfunc=NULL;
	stylefunc=NULL;
	opevaluator=NULL;
	evaluator=NULL;
	range=defaultvalue=NULL;
	defaultValue=newval;
	if (defaultValue) defaultValue->inc_count();
	flags=fflags;
	format=(type ? element_NameToType(type) : VALUE_Variable);
	fieldsformat=(newval?newval->type():0);
	fieldsdef=NULL;
	format_str=newstr(type);
	fields=NULL;
	islist=0; //if this element should be considered a set of such elements

	uihint=NULL;
	extrainfo=NULL;
}

//! Delete the various strings, and styledef->dec_count().
ObjectDef::~ObjectDef()
{
	//parent_namespace->dec_count(); <- do NOT do this, we assume the module will outlive the objectdef

	DBG cerr <<"--<ObjectDef \""<<(name?name:"(no name)")<<"\" destructor"<<endl;

	if (name)         delete[] name;
	if (Name)         delete[] Name;
	if (description)  delete[] description;
	if (range)        delete[] range;
	if (defaultvalue) delete[] defaultvalue;
	if (defaultValue) defaultValue->dec_count();
	if (format_str)   delete[] format_str;
	//if (fieldsdef) fieldsdef->dec_count(); <- don't do this, assume type will outlive things based on type
	delete[] uihint;
	if (extrainfo) extrainfo->dec_count();

	if (fields) {
		DBG cerr <<"---Delete fields stack"<<endl;
		delete fields;
		fields=NULL;
	}
}

/*! If which&1, remove fields.
 *  If which&2, remove range, defaultvalues.
 *  if which&4, remove evaluators.
 *  If which&8, remove name,Name,description.
 */
void ObjectDef::Clear(int which)
{
	if (which&1) { if (fields) delete fields; fields=NULL; }
	if (which&2) {
		makestr(range,NULL);
		makestr(defaultvalue,NULL);
		if (defaultValue) defaultValue->dec_count();
	}
	if (which&4) {
		newfunc=NULL;
		stylefunc=NULL;
		opevaluator=NULL;
		evaluator=NULL;
	}
	if (which&8) {
		makestr(name,NULL);
		makestr(Name,NULL);
		makestr(description,NULL);
	}
}

//! Add def to extendsdefs.
int ObjectDef::Extend(ObjectDef *def)
{
	extendsdefs.push(def);
	return 0;
}

int ObjectDef::SetType(const char *type)
{
	makestr(format_str,type);
	format=element_NameToType(type);
	return 0;
}

/*! \ingroup misc
 * Append src to dest, but escape newlines, tabs, and quotes.
 */
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

LaxFiles::Attribute *ObjectDef::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	if (what==0 || what==-1) {
		 //Attribute format, ala indented data
		if (!att) att=new LaxFiles::Attribute();

		att->push("name",name);
		if (Name)        att->push("Name",Name);
		if (description) att->push("description",description);
		if (flags)       att->push("flags",(long)flags,-1);
		if (format)      att->push("format",element_TypeNames(format));
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
				const char *fname = fields->e[c]->name;
				const char *fformat = element_TypeNames(fields->e[c]->format);
				if (fformat) att->push(fformat, fname);
				else att->push("field");
				//-----------
				//att->push("field");

				fields->e[c]->dump_out_atts(att->Top(), what, savecontext);
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

	} else if (what==DEFOUT_JSON) {
		 //NOTE that dumping out here is object definition, NOT actual json objects
		 //names and any strings are all in quotes, follow a name with a colon
		 //arrays are in brackets
		 //subelements are contained in brackets
		char *str=NULL;
		if (name) {
			appendstr(str,"\""); appendstr(str,name); appendstr(str,"\": ");

		}
	}

	return NULL;
}


//! Write out the stuff inside.
/*! If this styledef extends another, this does not write out the whole
 * def of that, only the name element of it.
 */
void ObjectDef::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
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

		//ObjectDef(ObjectDef *nextends,const char *nname,const char *nName, const char *ndesc,
		//		const char *fmt,const char *nrange, const char *newdefval,
		//		Laxkit::RefPtrStack<ObjectDef>  *nfields=NULL,unsigned int fflags=OBJECTDEF_CAPPED,
		//		NewObjectFunc nnewfunc=0,ObjectFunc nstylefunc=0);

		fprintf(f,"%sObjectDef *%sDef = new ObjectDef(",spc, name);
		if (extendsdefs.n) {
			 //output list of classes extended
			fprintf(f,"\"");
			for (int c=0; c<extendsdefs.n; c++) {
				fprintf(f,"%s",extendsdefs.e[c]->name);
				if (c!=extendsdefs.n-1) fprintf(f,", ");
			}
			fprintf(f,"\"");
		} else fprintf(f,"NULL, ");

		 //name
		if (name) fprintf(f, "\"%s\", ",name); else fprintf(f,"NULL, ");

		 //Name
		if (Name) {
			fwrite("\"",1,1,f);
			dump_out_escaped(f, Name, -1);
			fwrite("\", ",1,3,f);
		} else fprintf(f,"NULL, ");

		 //description
		if (description) {
			fwrite("\"",1,1,f);
			dump_out_escaped(f, description, -1);
			fwrite("\", ",1,3,f);
		} else fprintf(f, "NULL, ");

		 //format
		fprintf(f,"\"%s\",", element_TypeNames(format));

		 //range
		fwrite("NULL,",1,5,f); // ***

		 //default value
		fwrite("NULL",1,4,f); // ***

		fwrite(");\n\n",1,4,f);


		if (fields) for (int c=0; c<fields->n; c++) {
			ObjectDef *fdef = fields->e[c];

			if (     fdef->format == VALUE_Class
				  || fdef->format == VALUE_Variable
				  || fdef->format == VALUE_Operator
				  || fdef->format == VALUE_Class
				  || fdef->format == VALUE_Function
				  || fdef->format == VALUE_Namespace
				  || fdef->format == VALUE_Enum
				  ) {
				 //not a simple value, has sub fields
				fdef->dump_out(f, indent+2, what, context);
				fprintf(f, "%s%sDef->push(%sDef, 1)\n\n", spc, name, fdef->name);

			} else {
				 //use push, not a separate ObjectDef
				if (fdef->format == VALUE_EnumVal) {
					fprintf(f, "%s%sDef->pushEnumValue(\"%s\", %s%s%s, %s%s%s);\n", spc,
							name,
							fdef->name,
							fdef->Name ? "" : "",
							fdef->Name ? fdef->Name : "NULL",
							fdef->Name ? "" : "",
							fdef->description ? "" : "",
							fdef->description ? fdef->description : "NULL",
							fdef->description ? "" : ""
						   );
				} else {
					fprintf(f, "%s%sDef->push(\"%s\", %s%s%s, %s%s%s,\n"
							   "%s            \"%s\", NULL, NULL,\n"
							   "%s            0, NULL  );\n", spc,
							name,
							fdef->name,
							fdef->Name ? "" : "",
							fdef->Name ? fdef->Name : "NULL",
							fdef->Name ? "" : "",
							fdef->description ? "" : "",
							fdef->description ? fdef->description : "NULL",
							fdef->description ? "" : "",
							spc,
							element_TypeNames(fdef->format),
							spc
						   );
				}

	//virtual int push(const char *nname,const char *nName,const char *ndesc,
					 //const char *fformat,const char *nrange, const char *newdefval,
					 //unsigned int fflags,
					 //NewObjectFunc nnewfunc,
					 //ObjectFunc nstylefunc=NULL);
			}

			//cerr <<" *** finish implementing ObjectDef code out!"<<endl;
		}

		return;
	}
}

void ObjectDef::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	int what=0;
	if (what==0 || what==-1) {
		//indented

		char *nname,*value;

		for (int c=0; c<att->attributes.n; c++) {
			nname= att->attributes.e[c]->name;
			value=att->attributes.e[c]->value;

			if (!strcmp(nname,"name")) {
				makestr(name, value);

			} else if (!strcmp(nname,"Name")) {
				makestr(Name, value);

			} else if (!strcmp(nname,"description")) {
				makestr(description, value);

			} else if (!strcmp(nname,"extends")) {
					// *** this only checks parent_namespace, not some broader scope
					// should probably recursively check upward

					int n=0;
					char **extends = split(value, ',', &n);
					ObjectDef *edef;
					for (int c=0; c<n; c++) {
						stripws(extends[c]);

						if (parent_namespace) {
							edef = parent_namespace->FindDef(extends[c]);
							if (edef) extendsdefs.push(edef);
							deletestrs(extends, n);
						}
					}

			} else if (!strcmp(nname,"format")) {
				format = element_NameToType(value);

			} else if (!strcmp(nname,"flags")) {
				UIntAttribute(value, &flags, NULL);

			} else if (!strcmp(nname,"uihint")) {
				makestr(uihint, value);

			} else if (!strcmp(nname,"defaultValue")) {
				// ***
			} else if (!strcmp(nname,"range")) {
				// ***
			} else if (!strcmp(nname,"suggestions")) {
				// ***

			} else {
				ValueTypes fformat = element_NameToType(nname);
				if (fformat != VALUE_None) {
					ObjectDef *ndef = new ObjectDef();
					ndef->parent_namespace = this;
					ndef->format = fformat;
					if (value) makestr(ndef->name, value);
					push(ndef, 1);

					ndef->dump_in_atts(att->attributes.e[c], flag, context);
				}
			}
		}


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
	if (newfunc) return newfunc();
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
	if (id==-10000000) id=(fields?fields->n:0); //use number of enum values
	char idstr[30];
	sprintf(idstr,"%d",id);
	return push(str,Str,dsc,
				"enumval", NULL, idstr,
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
							 "enum", NULL, newdefval,
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
int ObjectDef::pushOperator(const char *op,int dir,int priority, const char *desc, OpFuncEvaluator *evaluator, int nflags)
{
	char rr[2];
	if (dir==OPS_Left) *rr='l';
	else if (dir==OPS_Right) *rr='r';
	else if (dir==OPS_LtoR) *rr='>';
	else if (dir==OPS_RtoL) *rr='<';
	rr[1]='\0';

	char pp[20];
	sprintf(pp,"%d",priority);

	ObjectDef *def=new ObjectDef(NULL, op,op,desc, "operator", rr, pp,  NULL,nflags);
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
 * - format (string)
 * - range (string)
 * - default value (string)
 *
 * For instance, if you are adding 2 parameters, you must supply 12 const char * values, followed
 * by a single NULL, or all hell will break loose.
 */
int ObjectDef::pushFunction(const char *nname,const char *nName,const char *ndesc,
					 FunctionEvaluator *nfunc,
					 ...)
{
	push(nname,nName,ndesc,
		 "function", NULL, NULL, 0,
		 NULL, NULL);
	ObjectDef *f=fields->e[fields->n-1];
	f->evaluator=nfunc;

	va_list ap;
	va_start(ap, nfunc);
	const char *v1,*v2,*v3, *v4,*v5,*v6;
	while (1) {
		v1=va_arg(ap,const char *);
		if (v1==NULL) break;
		v2=va_arg(ap,const char *);
		v3=va_arg(ap,const char *);
		v4=va_arg(ap,const char *);
		v5=va_arg(ap,const char *);
		v6=va_arg(ap,const char *);

		f->pushParameter(v1,v2,v3, v4,v5,v6, NULL);
	}
	va_end(ap);

	return fields->n-1;
}

//! Push def without fields. If pushing this new field onto fields fails, return -1, else the new field's index.
int ObjectDef::push(const char *nname,const char *nName,const char *ndesc,
			const char *fformat,const char *nrange, const char *newdefval,unsigned int fflags,
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
		const char *fformat,const char *nrange, const char *newdefval,
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
 *
 * Usually, this is done only in namespaces and class definitions. If type==NULL, then add as "variable".
 */
int ObjectDef::pushVariable(const char *name,const char *nName, const char *ndesc, const char *type, unsigned int fflags, Value *v, int absorb)
{
	ObjectDef *def=FindDef(name);

	if (def) {
		 //name found, overwrite with new stuff
		makestr(def->Name,nName);
		makestr(def->description,ndesc);
		if (v) v->inc_count();
		if (def->defaultValue) def->defaultValue->dec_count();
		def->defaultValue=v;
		if (v && absorb) v->dec_count();
		return -1;
	}

	//else name didn't exist, so ok to push
	def=new ObjectDef(name,nName,ndesc, v, type,fflags);
	push(def,1);
	if (v && absorb) v->dec_count();
	return 0;
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
		def=new ObjectDef(name,name,NULL, v, NULL,0);
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
	if (i>=0) { if (absorb) def->dec_count(); return 1; }
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
			const char *fformat, const char *nrange, const char *newdefval, Value *defvalue)
{
	ObjectDef *newdef=new ObjectDef(NULL,nname,nName,
								  ndesc,fformat,nrange,newdefval,
								  NULL,0,NULL);
	if (defvalue) { newdef->defaultValue=defvalue; defvalue->inc_count(); }

	int c;
	if (!fields || !fields->n) c=push(newdef);//absorbs
	else {
		c=fields->push(newdef); //incs count
		newdef->dec_count();
	}
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

/*! Return whether this is plain data (1) or is something else like a class, function, etc.
 * Basically, if it is not a namespace, class, function, operator, or alias, then it is assumed to be data.
 */
int ObjectDef::isData()
{
	if (    format==VALUE_Namespace
		 || format==VALUE_Class
		 || format==VALUE_Operator
		 || format==VALUE_Function
		 || format==VALUE_Alias
	   ) return 0;
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
  * If enum, then use enum specific fuctions. getNumEnumValues(), getEnumInfo().
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

/*! Return the number of enum values defined by this and and any in extendsdef.
 * This is needed since enums are excepted by the regular getNumFields().
 * See also getEnumInfo().
 */
int ObjectDef::getNumEnumFields()
{
	if (format!=VALUE_Enum && format!=VALUE_EnumVal) return 0;

	int n=0;
	if (fields) n+=fields->n;

	for (int c=0; c<extendsdefs.n; c++)
		n+=extendsdefs.e[c]->getNumEnumFields();

	return n;
}

//! Return a new object of the specified type. Object def needs to have the newfunc defined.
Value *ObjectDef::newValue(const char *objectdef)
{
    ObjectDef *s=FindDef(objectdef,2);
    if (!s) return NULL;
    if (s->newfunc) return s->newfunc();
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
 * If len<0, then use strlen(objectdef).
 *
 * \todo Should probably optimize this for searching.. have list of sorted field names?
 */
ObjectDef *ObjectDef::FindDef(const char *objectdef, int len, int which)
{
	if (len<0) len=strlen(objectdef);
	if (fields) for (int c=0; c<fields->n; c++) {
		if (!strncmp(objectdef,fields->e[c]->name,len) && strlen(fields->e[c]->name)==(unsigned int)len) {
			if (which==0) return fields->e[c];
			if ((which&1) && fields->e[c]->format==VALUE_Function) return fields->e[c];
			else if ((which&2) && fields->e[c]->format==VALUE_Class) return fields->e[c];
			else if ((which&4) && fields->e[c]->format==VALUE_Variable) return fields->e[c];
		}
	}
	if (extendsdefs.n) {
		ObjectDef *def=NULL;
		for (int c=0; c<extendsdefs.n; c++) {
			def=extendsdefs.e[c]->FindDef(objectdef,len,which);
			if (def) return def;
		}
	}
	return NULL;
}

/*! Return various information about particular enum values.
 * Given the index of a desired field, this looks up which ObjectDef actually has the text
 * and sets the pointers to point to there. Nothing is done if the particular pointer is NULL.
 *
 * Returns 0 success, 1 error.
 */
int ObjectDef::getEnumInfo(int index,
						const char **nm,
						const char **Nm,
						const char **desc,
						int *id)
{
	if (!fields) return 1;

	ObjectDef *def=NULL;

	// *** this ignores extendsdef:
	//index=findActualDef(index,&def);
	if (index>=0 && index<fields->n) def = fields->e[index];

	if (!def) return 1; // otherwise index should be a valid value in fields, or refer to this

	if (nm)     *nm = def->name;
	if (Nm)     *Nm = def->Name;
	if (desc) *desc = def->description;
	if (id) *id = strtol(def->defaultvalue, NULL, 10);

	return 0;
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
						const char **objtype,
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
		if (objtype) *objtype=def->format_str;
		if (def_ret) *def_ret=def;
		return 0;
	}

	if (!fields) return 1;
	if (nm) *nm=def->fields->e[index]->name;
	if (Nm) *Nm=def->fields->e[index]->Name;
	if (desc) *desc=def->fields->e[index]->description;
	if (rng) *rng=def->fields->e[index]->range;
	if (defv) *defv=def->fields->e[index]->defaultvalue;
	if (fmt) *fmt=def->fields->e[index]->format;
	if (objtype) *objtype=def->fields->e[index]->format_str;
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
	if (!def->fields) return NULL; //for instance, many base types might not have fields
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

//! Find the index of the field named in fname.
/*! Assumes fname is composed of letters, numbers, or underscores.
 *  "a.b" will find a, and next will point to "b". If there is only "a", then next
 *  will be set to NULL.
 *
 *  On success, the field index is returned (field index starting at 0).
 *  If the field string is a number (ie "34.2...") the number is parsed
 *  and returned without bounds checking (though it must be non-negative). This
 *  number passing is to accommodate sets with variable numbers of
 *  elements, where the number of elements is held in Object, not ObjectDef.
 *
 *  This returned index takes in to account all inherited defs, which add
 *  before. So say class A inherits from class B, and B has 3 members, if
 *  we find the first member defined by A, index will be 3, the number of
 *  members of B, plus the immediate index in A.
 */
int ObjectDef::findfield(const char *fname,char **next) // next=NULL
{
	int index=0;
	int nn;
	if (extendsdefs.n) {
		ObjectDef *extendsdef;
		for (int c=0; c<extendsdefs.n; c++) {
			extendsdef=extendsdefs.e[c];
			nn=extendsdef->findfield(fname,next);
			if (nn>=0) return nn+index;
			index+=extendsdef->getNumFieldsOfThis();
		}
	}
	nn=findFieldOfThis(fname,next);
	if (nn>=0) return nn+index;
	if (next) *next=NULL;
	return -1;
}

//! Like findfield(), but only return index of this class, without adding or searching from inherited classes.
/*! So, returned index is in this->fields.
 */
int ObjectDef::findFieldOfThis(const char *fname,char **next) // next=NULL
{
	 // make n the index of the first '.' or end of string
	unsigned int n=0; //length of string to check in fname
	while (isalnum(fname[n]) || fname[n]=='_') n++;
	if (n==0) {
		if (next) *next=NULL;
		return -1;
	}

	 // check for number first: "34.blah.blah"
	if (isdigit(fname[0])) {
		char *nxt=NULL;
		int nn=strtol(fname,&nxt,10);
		if (nxt-fname==n) {
			 //found valid number
			if (nn>=0) {
				if (next) *next=const_cast<char*>(fname) + n + (fname[n]=='.'?1:0);
				return nn;
			}
		}
	}

	if (!fields) {
		if (next) *next=NULL;
		return -1;
	}

	for (int c=0; c<fields->n; c++) {
		if (!strncmp(fname,fields->e[c]->name,n) && strlen(fields->e[c]->name)==n) {
			if (next) *next=const_cast<char*>(fname) + n + (fname[n]=='.'?1:0);
			return c;
		}
	}

	return -1;
}




//---------------------------------------- ValueHash --------------------------------------
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

int ValueHash::type()
{ return VALUE_Hash; }

/*! Currently copies references, does not create new instances of value objects.
 */
Value *ValueHash::duplicate()
{
	ValueHash *v=new ValueHash;
	Value *vv;
	for (int c=0; c<keys.n; c++) {
		vv=values.e[c]->duplicate();
		vv->inc_count();
		v->push(keys.e[c],vv);
	}
	return v;
}

/*! Something like:
 * <pre>
 *  { name: 3,
 *    name2: "string",
 *    othername: (3,5)
 *  }
 * </pre>
 */
int ValueHash::getValueStr(char *buffer,int len)
{
	int needed=3;//"{:}"
	for (int c=0; c<values.n; c++) {
		needed+= 1 + values.e[c]->getValueStr(NULL,0) + (keys.e[c] ? strlen(keys.e[c]) : 0) + 4;
	}
	if (!buffer || len<needed) return needed;

	int pos=1;
	sprintf(buffer,"{");
	if (!values.n) { sprintf(buffer+1,":"); pos++; }
	for (int c=0; c<values.n; c++) {
		 //add key name
		sprintf(buffer+pos," %s: ",keys[c]);
		pos+=strlen(buffer+pos);

		 //add value
		values.e[c]->getValueStr(buffer+pos,len);
		if (c!=values.n-1) strcat(buffer+pos,",");
		else strcat(buffer+pos," ");
		pos+=strlen(buffer+pos);
	}
	strcat(buffer+pos,"}");
	modified=0;
	return 0;
}

//! Return a set with the index'th {name,value}. value is a ref to original value, not a duplicate.
Value *ValueHash::dereference(int index)
{
	if (index<0 || index>=values.n) return NULL;
	SetValue *set=new SetValue();
	set->Push(new StringValue(keys.e[index]),1);
	set->Push(values.e[index],0);
	return set;
}

ObjectDef default_ValueHash_ObjectDef(NULL,"Hash",_("Hash"),_("Set of name-value pairs"),
							 "class", NULL, "{ : }",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_ValueHash_ObjectDef()
{
	ObjectDef *def = &default_ValueHash_ObjectDef;
	if (def->fields) return def;

//	virtual int pushFunction(const char *nname,const char *nName,const char *ndesc,
//					 FunctionEvaluator *nfunc,
//					 ...);
// * - the parameter name,
// * - the parameter name translated, human readable name
// * - the description of the parameter
// * - format
// * - range (string)
// * - default value (string)

	def->pushFunction("n",_("Number of elements"),_("Number of elements"), NULL,
					  NULL);

	def->pushFunction("keys",_("Keys"),_("Return set of key names"), NULL,
					  NULL);

	def->pushFunction("values",_("Values"),_("Return set of values"), NULL,
					  NULL);

	def->pushFunction("push",_("Push"),_("Add a new name-value pair"),
					  NULL,
					  "key",_("Key"),_("Key"), "string",NULL,NULL,
					  "value",_("Value"),_("Value"), "any",NULL,NULL,
					  "pos",_("Position"),_("Where to push"), "int",NULL,"-1",
					  NULL);

	def->pushFunction("flush",_("Flush"),_("Remove all name-value pairs"), NULL,
					  NULL);

	def->pushFunction("swap",_("Swap"),_("Swap two positions"),
					  NULL,
					  "pos",_("Position 1"),_("Position to swap."), "int",NULL,NULL,
					  "pos2",_("Position 2"),_("Position to swap."), "int",NULL,NULL,
					  NULL);

	def->pushFunction("slide",_("Slide"),_("Same as push(pop(p1),p2)"),
					  NULL,
					  "pos",_("Position 1"),_("Position to take."), "int",NULL,NULL,
					  "pos2",_("Position 2"),_("Where to put."), "int",NULL,NULL,
					  NULL);

	def->pushFunction("key",_("Key"),_("Return string of the key"),
					  NULL,
					  "pos",_("Index"),_("Index"), "int",NULL,NULL,
					  NULL);

	def->pushFunction("value",_("Value"),_("Return value associated with key"),
					  NULL,
					  "key",_("Key"),_("Either an integer index, or string key"), "any",NULL,NULL,
					  NULL);

	return def;
}

ObjectDef *ValueHash::makeObjectDef()
{
	ObjectDef *def=Get_ValueHash_ObjectDef();
	def->inc_count();
	return def;
}


int ValueHash::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (len==1 && *func=='n') { *value_ret=new IntValue(keys.n); return 0; }

	 //no parameter ones:
	if (isName(func,len,"keys")) {
		SetValue *set=new SetValue;
		for (int c=0; c<keys.n; c++) set->Push(new StringValue(keys.e[c]),1);
		*value_ret=set;
		return 0;
	}

	if (isName(func,len,"values")) {
		SetValue *set=new SetValue;
		for (int c=0; c<values.n; c++) set->Push(values.e[c],0);
		*value_ret=set;
		return 0;
	}

	if (isName(func,len,"flush")) {
		flush();
		value_ret=NULL;
		return 0;
	}

	 //with parameters:
	if (!parameters) return -1;
	try {

		if (isName(func,len,"value")) {
			Value *v=parameters->find("key");
			if (!v) throw 1; //missing parameter!
			int pos=-1;
			if (v->type()==VALUE_Int) pos=dynamic_cast<IntValue*>(v)->i;
			else if (v->type()==VALUE_String) {
				pos=findIndex(dynamic_cast<StringValue*>(v)->str);
			}
			if (pos<0 || pos>=keys.n) throw 2; //index out of range!
			*value_ret=values.e[pos];
			values.e[pos]->inc_count();
			return 0;
		}

		if (isName(func,len,"key")) {
			int success;
			int pos=parameters->findInt("pos",-1,&success);
			if (success!=0) throw -1; //wrong parameters
			if (pos<0 || pos>=keys.n) throw 2; //index out of range!
			*value_ret=new StringValue(keys.e[pos]);
			return 0;
		}


		if (isName(func,len,"push")) {
			int success;
			int pos=parameters->findInt("pos",-1,&success);
			if (success!=0) pos=keys.n; //pos not found, use top
			if (pos<0) pos=keys.n;

			const char *key=parameters->findString("key",-1,&success);
			if (success!=0) throw -1;

			Value *v=parameters->find("value");
			if (!v) throw -1;

			push(key,v);
			*value_ret=NULL;
			return 0;
		}

		if (isName(func,len,"pop")) {
			Value *v=parameters->find("key");
			if (!v) throw 1; //missing parameter!
			int pos=-1;
			if (v->type()==VALUE_Int) pos=dynamic_cast<IntValue*>(v)->i;
			else if (v->type()==VALUE_String) {
				pos=findIndex(dynamic_cast<StringValue*>(v)->str);
			}
			if (pos<0 || pos>=keys.n) throw 2; //index out of range!

			SetValue *set=new SetValue();
			set->Push(new StringValue(keys.e[pos]),1);
			keys.remove(pos);
			set->Push(values.e[pos],0);
			values.remove(pos);
			*value_ret=set;
			return 0;
		}

		if (isName(func,len,"swap")) {
			int success;
			int pos=parameters->findInt("pos",-1,&success);
			if (success!=0) throw -1; //wrong parameters
			int pos2=parameters->findInt("pos2",-1,&success);
			if (pos<0  || pos>=keys.n)  throw 2; //index out of range!
			if (pos2<0 || pos2>=keys.n) throw 2; //index out of range!
			swap(pos,pos2);
			return 0;
		}

		if (isName(func,len,"slide")) {
			int success;
			int pos=parameters->findInt("pos",-1,&success);
			if (success!=0) throw -1; //wrong parameters
			int pos2=parameters->findInt("pos2",-1,&success);
			if (pos<0  || pos>=keys.n)  throw 2; //index out of range!
			if (pos2<0 || pos2>=keys.n) throw 2; //index out of range!

			 //push(pop(p1),p2)
			char *key=keys.pop(pos);
			Value *value=values.pop(pos);
			push(key,value);
			value->dec_count();
		}
	} catch (int e) {
		if (log) {
			if (e==-1) return -1; //can't use parameters!
			if (e==1) log->AddMessage(_("Missing parameter!"),ERROR_Fail);
			else if (e==2) log->AddMessage(_("Index out of range!"),ERROR_Fail);
		}
		return 1;
	}


	if (log) log->AddMessage(_("Unknown name!"),ERROR_Fail);
	return 1;
}

int ValueHash::getNumFields()
{
	return values.n;
}

ObjectDef *ValueHash::FieldInfo(int i)
{
	if (i<0 || i>=values.n) return NULL;
	return values.e[i]->GetObjectDef();
}

//! Returns object name, or NULL.
const char *ValueHash::FieldName(int i)
{
	if (i<0 || i>=values.n) return NULL;
	return values.e[i]->Id();
}

int ValueHash::flush()
{
	keys.flush();
	values.flush();
	return 0;
}

int ValueHash::push(const char *name,int i,int where)
{
	Value *v=new IntValue(i);
	int c=push(name,v,where);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,double d,int where)
{
	Value *v=new DoubleValue(d);
	int c=push(name,v,where);
	v->dec_count();
	return c;
}

int ValueHash::push(const char *name,const char *value,int where)
{
	Value *v=new StringValue(value);
	int c=push(name,v,where);
	v->dec_count();
	return c;
}

//! Create an ObjectValue with obj, and push.
/*! Increments obj count. */
int ValueHash::pushObject(const char *name,Laxkit::anObject *obj,int where)
{
	DBG cerr <<"pushObject: "<<(obj?obj->whattype():"null")<<endl;
	if (dynamic_cast<Value*>(obj)) return push(name,dynamic_cast<Value*>(obj));

	Value *v=new ObjectValue(obj);
	int c=push(name,v,where);
	v->dec_count();
	return c;
}

/*! Increments count on v. If name exists already, just replace value.
 *
 * If adding, not replacing value, then
 * if where<0 then push at end, else push at that position in stack.
 *
 * where is ignored if hash is sorted.
 *
 * Returns index of pushed value, or -1 if failed somehow.
 */
int ValueHash::push(const char *name,Value *v,int where)
{
	if (where<0 || where>=keys.n) where=keys.n;

	int place=findIndex(name);
	int cmp=-2;
	if (place>=0) cmp=0;

	if (sorted) {
		 //sorts largest first.. *** optimize this!!
		for (place=0; place<keys.n; place++) {
			cmp=strcmp(name,keys.e[place]);
			if (cmp>=0) {
				break;
			}
		}
		where=place;
	}

	if (cmp==0) { //exact match
		set(place,v);
		return place;
	}

	keys.push(newstr(name),-1,where);
	return values.push(v,-1,where);
}

/*! Increments count on v.
 * len is how much of name to use. -1 means use all of name. */
int ValueHash::push(const char *name,int len, Value *v, int where)
{
	char *nname=newnstr(name,len);
	int c=push(nname,v,where);
	delete[] nname;
	return c;
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

/*! Set value of an existing key. Return 0 for success, or nonzero for error such as key not found.
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
	if (newv) newv->inc_count();
	if (values.e[which]) values.e[which]->dec_count();
	values.e[which]=newv;
	return 0;
}

//! Return the index corresponding to name, or -1 if not found.
int ValueHash::findIndex(const char *name,int len)
{
	if (!name) return -1;
	if (len<0) len = strlen(name);
	for (int c=0; c<keys.n; c++) {
		if (len<0 && !strcmp(name,keys.e[c])) return c;
		if (len>0 && !strncmp(name,keys.e[c],len)) return c;
	}
	return -1;
}

/*! Return the Value object for key==name.
 */
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
 * If the value exists, but is not a boolean, int, or double, then sets *error_ret=2.
 * Otherwise set to 0.
 *
 * Nonzero int or double translates to true.
 */
int ValueHash::findBoolean(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return 0; }

	IntValue *i=dynamic_cast<IntValue*>(values.e[which]);
	if (i) { if (error_ret) *error_ret=0; return i->i; }

	BooleanValue *b=dynamic_cast<BooleanValue*>(values.e[which]);
	if (b) { if (error_ret) *error_ret=0; return b->i; }

	DoubleValue *d=dynamic_cast<DoubleValue*>(values.e[which]);
	if (d) { if (error_ret) *error_ret=0; return d->d!=0; }

	if (error_ret) *error_ret=2;
	return 0;
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
 * If name is not found, then set *error_ret=1 (if error_ret!=NULL).
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

/*! If which>=0 then interpret that Value and ignore name.
 * Otherwise find it with findIndex().
 *
 * If name is not found, then set *error_ret=1 if error_ret!=0.
 * If the value exists, but is not a FlatvectorValue, then sets *error_ret=2.
 * Otherwise set to 0.
 */
flatvector ValueHash::findFlatvector(const char *name, int which, int *error_ret)
{
	if (which<0) which=findIndex(name);
	if (which<0 || !values.e[which]) { if (error_ret) *error_ret=1; return flatvector(); }
	FlatvectorValue *v=dynamic_cast<FlatvectorValue*>(values.e[which]);
	if (!v) { if (error_ret) *error_ret=2; return flatvector(); }
	if (error_ret) *error_ret=0;
	return v->v;
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
	objectdef=NULL;
}

Value::~Value()
{
	if (objectdef) objectdef->dec_count();
}

//! Return a Value's string id, if any. NULL might be returned for unnamed values.
/*! Default is to return this->object_idstr.
 */
const char *Value::Id()
{ return object_idstr; }

/*! Default is to return GetObjectDef()->format, or ->fieldsformat if format==VALUE_Formats.
 * Built in types will redefine.
 */
int Value::type()
{
	ObjectDef *def=GetObjectDef();
	if (!def) return VALUE_Any;
	if (def->format==VALUE_Fields) return def->fieldsformat;
	return def->format;
}


//! Dereference once, retrieving values from members. Note this is for Values only, not functions, classes, namespaces, etc.
/*! len is the length of extstring to consider.
 * extstring could be something like "a.21.c", then len should be 1,
 * and we dereference "a", leaving to ".21.c" to be dealt with elsewhere. For the number, dereference(int) is used, which is
 * optional for subclasses to define, but the number is usually parsed by the calculator, not here, though you can if you want.
 *
 * Returns a reference if possible, or new. Calling code MUST decrement count.
 *
 * Default returns NULL.
 */
Value *Value::dereference(const char *extstring, int len)
{ return NULL; }


//! Returns a reference if possible, or new if necessary. Calling code MUST decrement count.
/*! Default is to return NULL. Subclasses may decide for themselves if they implement number
 * indexing.
 */
Value *Value::dereference(int index)
{ return NULL; }


/*! If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 *
 *  Default is return 0;
 */
int Value::assign(FieldExtPlace *ext,Value *v)
{ return 0; }



//! Output to buffer, do NOT reallocate buffer, assume it has enough space. If len is not enough, return how much is needed.
/*! Generally, subclasses should redefine this, and not the other getValueStr().
 * The string returned generally should be a string that can reasonably be
 * expected to convert back to a value in the interpreter.
 *
 * Return 0 for successfully written, or the length necessary if len is not enough.
 */
int Value::getValueStr(char *buffer,int len)
{
	ObjectDef *def = GetObjectDef();
	const char *defname=(def?def->name:whattype());

	if (!buffer || len<(int)strlen(whattype()+1)) return strlen(defname)+1;


	sprintf(buffer,"%s",defname);
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
	int needed = getValueStr(NULL,0);
	if (*len<needed) {
		if (!oktoreallocate) return needed;
		if (*buffer) delete[] *buffer;
		*buffer=newnstr(NULL,needed);
		*len=needed;
	}
	return getValueStr(*buffer,*len);
}

/*! Return objectdef, calling makeObjectDef() if necessary.
 * If other objects reference the returned def, they will need to call inc_count() on it.
 */
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

int Value::FieldIndex(const char *name)
{
    ObjectDef *def=GetObjectDef();
    if (!def) return -1;
	return def->findfield(name,NULL);
}

/*! Default dump out to simple name/value pairs.
 * NOTE that this does not automatically put in subattributes for values that have them.
 * It uses getValueStr() to attach a string to each FieldInfo().
 *
 * If this->getValueStr() has anything, then that value gets put in the first subatt with name "value".
 *
 * After that, one subatt gets pushed by anything that is not a function, class, or op, as per the value's ObjectDef.
 */
LaxFiles::Attribute *Value::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	if (!att) att=new Attribute;

	ObjectDef *def=GetObjectDef();

	if (what==-1) {
		 //dump out part of object def
		if (!def) {
			DBG cerr << "  Value::dump_out_atts(): missing ObjectDef for "<<whattype()<<endl;
			return NULL;
		}

		//dump out atts with:
		//   name:  name defaultvalue??
		//   value: description

		if (def->fields) {
			Attribute *subatt;
			for (int c=0; c<def->fields->n; c++) {
				if (def->fields->e[c]->format==VALUE_Fields) {
					subatt=att->pushSubAtt(def->fields->e[c]->name, def->fields->e[c]->description);
				} else {
					subatt=NULL;
					att->push(def->fields->e[c]->name, def->fields->e[c]->description);
				}

				if (!isblank(def->fields->e[c]->defaultvalue)) {
					appendstr(att->attributes.e[att->attributes.n-1]->name, " ");
					appendstr(att->attributes.e[att->attributes.n-1]->name, def->fields->e[c]->defaultvalue);
				}

				if (subatt) subatt->push("...");
			}
		}

		//def->dump_out_atts(att,-1,NULL);
		return att;
	}


	ObjectDef *fdef;
	const char *str;
	const char *name;
	char *buffer=NULL;
	int len=0;

	////for whole Value:
	//getValueStr(&buffer,&len, 1);
	//str=buffer;
	//if (str) {
	//	fprintf(f," %s\n",str);
	//}

	if (def->format == VALUE_Enum) {
		EnumValue *ev = dynamic_cast<EnumValue*>(this);
		const char *nm = NULL;
		def->getEnumInfo(ev->value, &nm);
		string s = (string)"" + def->name + '.' + nm;
		att->push(whattype(), s.c_str());

	} else if (!strcmp(def->name, "string")) {
		StringValue *v = dynamic_cast<StringValue*>(this);
		att->push(whattype(), v->str);

	} else {
		Value *v;
		int numout = 0;

		Attribute *vatt=NULL;

		for (int c=0; c<getNumFields(); c++) {
			fdef=FieldInfo(c); //this is the object fdef of a field. If it exists, then this element has subfields.
			if (!fdef) continue;

			 //output values only, not functions
			if (fdef->format==VALUE_Function) continue;
			if (fdef->format==VALUE_Class) continue;
			if (fdef->format==VALUE_Operator) continue;

			v=dereference(fdef->name,strlen(fdef->name));
			if (!v) continue;

			name=FieldName(c);
			v->getValueStr(&buffer,&len, 1);
			str=buffer;

			if (vatt == NULL) vatt = att->pushSubAtt(whattype());
			vatt->push(name,str);
			numout++;
		}

		if (numout == 0) {
			len = 0;
			getValueStr(&buffer,&len, 1);
			if (len>0) att->push(whattype(), buffer);
		}
	}

	if (buffer) delete[] buffer;

	return att;
}

/*! For dumping out Value descriptions.
 * Fit within columns number of characters per line.
 */
void dump_out_desc(Attribute *att, FILE *f, int indent, int columns)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	 //determine indent for description
	int nw=0, w;
	for (int c=0; c<att->attributes.n; c++) {
		w=strlen(att->attributes.e[c]->name) + 1;
		if (w>nw) nw=w;
	}

	char fmt[20];
	sprintf(fmt,"%s%%-%ds",spc, nw);
	//char spc2[indent+1+nw]; memset(spc,' ',indent+nw); spc[indent+nw]='\0';

	int first;
	char odescchar;
	char *ndesc, *desc, *oo;
	int descwidth=columns-indent-nw;
	if (descwidth<=10) descwidth=60;

	for (int c=0; c<att->attributes.n; c++) {
		fprintf(f,fmt,att->attributes.e[c]->name);

		if (att->attributes.e[c]->value) {
			 //output #description... but wrap to columns
			ndesc=newstr(att->attributes.e[c]->value);
			desc=ndesc;

			first=1;
	        do {
	          if ((int)strlen(desc)<descwidth) {
	               //the line fits
				  if (first) fprintf(f,"#%s\n",desc);
				  else       fprintf(f,"# %s\n",desc);
	              break;
	          }

	           //else line has to be broken
	          oo=desc+descwidth;
	          while (oo!=desc && !isspace(*oo)) oo--;
	          if (oo==desc) {
	               //no whitespace at all, so print that line anyway
	              oo=desc+descwidth;
	          }
	          odescchar=*oo;
	          *oo='\0';
			  if (first) { fprintf(f,"#%s\n",desc); first=0; }
			  else         fprintf(f,"# %s\n",desc);
	          fprintf(f,fmt,""); //<- prepare next line

			  *oo=odescchar;
			  desc=oo;
			  while (isspace(*desc)) desc++;
	        } while (desc && *desc);

			delete[] ndesc;

		} else {
			//no description
			fprintf(f,"\n");
		}

		if (att->attributes.e[c]->attributes.n) {
			dump_out_desc(att->attributes.e[c], f, indent+2, columns);
		}
	}
}

/*! Default will not output the Value's id string. The object calling this class should be doing that.
 */
void Value::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	if (what==-1) {
		Attribute att;
		dump_out_atts(&att, -1, context);
		dump_out_desc(&att, f,indent, 120);
		return;

		//-------------------
		// //dump out object def
    	//ObjectDef *def=GetObjectDef();
		//if (!def) {
		//	DBG cerr << "  missing ObjectDef for "<<whattype()<<endl;
		//	return;
		//}
		//def->dump_out(f,indent,-1,context);
		//return;
		//-------------------
	}

	if (what==DEFOUT_JSON) {
		DBG cerr <<" *** value out to json, todo!!"<<endl;

		Value *v;
		ObjectDef *def;
		for (int c=0; c<getNumFields(); c++) {
			def=FieldInfo(c); //this is the object def of a field. If it exists, then this element has subfields.
			if (!def) continue;

			 //output values only, not functions
			if (def->format==VALUE_Function || def->format==VALUE_Class || def->format==VALUE_Operator) continue;

			v=dereference(def->name,strlen(def->name));
			if (!v) continue;
			def=v->GetObjectDef();
			if (!def) continue;

			//fprintf(f,"%s\"%s\": ",spc,FieldName(c));

		}

		return;
	}

	Attribute att;
    dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

void Value::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ //  ***
}

/*! Try to convert att into a builtin Value object. This is designed to work well
 * with various dump_in_atts functions.
 *
 * For simple values, the value is in att->value.
 * Otherwise, assume subfields.
 */
Value *AttributeToValue(Attribute *att)
{
	if (!strcmp(att->name, "DoubleValue")) {
		double d=0;
		if (DoubleAttribute(att->value, &d)) {
			return new DoubleValue(d);
		} else {
			return NULL;
		}

	} else if (!strcmp(att->name, "IntValue")) {
		long v=0;
		if (LongAttribute(att->value, &v)) {
			return new IntValue(v);
		} else {
			return NULL;
		}

	} else if (!strcmp(att->name, "BooleanValue")) {
		return new BooleanValue(att->value);

    } else if (!strcmp(att->name, "StringValue")) {
		return new StringValue(att->value);

    } else if (!strcmp(att->name, "BytesValue")) {
		//encoded binary in value: "123ascii_escaped:\ff\fe\a8\03"
		//-or- subatt[0].name == "binary[byte length]", subatt[0].value == straight binary data
		//return new BytesValue(att->value);

    } else if (!strcmp(att->name, "ColorValue")) {
		return new ColorValue(att->value);

    } else if (!strcmp(att->name, "FlatvectorValue")) {
		flatvector v;
		char *endptr=NULL;
		if (FlatvectorAttribute(att->value, &v, &endptr)) {
			return new FlatvectorValue(v);
		}
		return NULL;

    } else if (!strcmp(att->name, "SpacevectorValue")) {
		spacevector v;
		char *endptr=NULL;
		if (SpacevectorAttribute(att->value, &v, &endptr)) {
			return new SpacevectorValue(v);
		}
		return NULL;

    } else if (!strcmp(att->name, "QuaternionValue")) {
		Quaternion v;
		char *endptr=NULL;
		if (QuaternionAttribute(att->value, &v, &endptr)) {
			return new QuaternionValue(v);
		}
		return NULL;

    } else if (!strcmp(att->name, "FileValue")) {
		return new FileValue(att->value);

    } else if (!strcmp(att->name, "EnumValue")) {
    } else if (!strcmp(att->name, "ValueHash")) {
    } else if (!strcmp(att->name, "GenericValue")) {
    } else if (!strcmp(att->name, "SetValue")) {
    } else if (!strcmp(att->name, "ArrayValue")) {
    } else if (!strcmp(att->name, "NullValue")) {
		return new NullValue();

    } else if (!strcmp(att->name, "FunctionValue")) {
    } else if (!strcmp(att->name, "ObjectValue")) {
	}

	return NULL;
}

////----------------------------- GenericValue ----------------------------------
/*! \class GenericValue
 * Value object for a purely coded class.
 */

Value *NewGenericValue(ObjectDef *def)
{
	if (!def) return NULL;
	return new GenericValue(def);
}

GenericValue::GenericValue(ObjectDef *def)
{
	objectdef=def;
	if (objectdef) objectdef->inc_count();

	const char *nm,*type;
	ValueTypes fmt;
	for (int c=0; c<def->getNumFields(); c++) {
		def->getInfo(c, &nm,NULL,NULL,NULL,NULL,&fmt,&type,NULL);
		if (fmt!=VALUE_Function
				&& fmt!=VALUE_Class
				&& fmt!=VALUE_Operator
				&& fmt!=VALUE_Namespace) {

			 //push only pure value elements
			elements.push(nm,(Value*)NULL);
		}
	}
}

GenericValue::~GenericValue()
{
}

int GenericValue::getValueStr(char *buffer,int len)
{//***put class name? this just leaves {a=1, b=2,...}
	return elements.getValueStr(buffer,len);
}

Value *GenericValue::duplicate()
{
	GenericValue *v=new GenericValue(objectdef);
	Value *vv;
	for (int c=0; c<elements.n(); c++) {
		vv=elements.e(c)->duplicate();
		if (v->elements.set(elements.key(c),vv)==0) vv->dec_count();
	}
	return v;
}


ObjectDef *GenericValue::makeObjectDef()
{
	 //assume that the objectdef was always assigned in constructor
	return NULL;
}

Value *GenericValue::dereference(const char *extstring, int len)
{
	char *str=newnstr(extstring,len);
	Value *v=elements.find(str);
	delete[] str;
	if (!v) return NULL;
	v->inc_count();
	return v;
}

int GenericValue::assign(FieldExtPlace *ext,Value *v)
{//***
//	if (!ext || !ext->n()) {
//		*** whole assign
//	}

	const char *str=ext->e(0);
	int index=elements.findIndex(str);
	if (index>=0) {
		//elements.assign(ext,v);
		DBG if (ext->n()>1) cerr <<"*** need to implement ValueHash->assign()!!"<<endl;
		elements.set(index,v);
	} else return 1;
	return 1;
}


//----------------------------- CodedValue ----------------------------------

/*! \class CodedValue
 */
class CodedValue : public Value, public FunctionEvaluator
{
  public:
	CodedValue();
	virtual ~CodedValue();
};

//----------------------------- SetValue ----------------------------------
/*! \class SetValue
 */

SetValue::SetValue(const char *restricted)
{
	restrictto=newstr(restricted);
}

SetValue::~SetValue()
{
	if (restrictto) delete[] restrictto;
}


//! Push v, which increments its count if !absorb.
/*! Return 0 for success or nonzero for error.
 */
int SetValue::Push(Value *v,int absorb, int where)
{
	if (!v) return 1;
	if (values.push(v,-1,where)>=0) {
		if (absorb) v->dec_count();
		return 0; //success
	}
	return 1; //fail
}

int SetValue::getValueStr(char *buffer,int len)
{
	int needed=3;//"{}\n"
	for (int c=0; c<values.n; c++) {
		needed+= 1 + values.e[c]->getValueStr(NULL,0);
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
		s->Push(v,1);
	}
	return s;
}

//! Return the index'th element.
Value *SetValue::dereference(int index)
{
	if (index<0 || index>=values.n) return NULL;
	if (values.e[index]) values.e[index]->inc_count();
	return values.e[index];
}


ObjectDef default_SetValue_ObjectDef(NULL,"set",_("Set"),_("Set of values"),
							 "class", NULL, "{}",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_SetValue_ObjectDef()
{
	ObjectDef *def=&default_SetValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("n",_("Number of elements"),_("Number of elements"), NULL,
					  NULL);

	def->pushFunction("push",_("Push"),_("Add a new value"),
					  NULL,
					  "value",_("Value"),_("Value"), "any",NULL,NULL,
					  "pos",_("Position"),_("Where to push"), "int",NULL,"-1",
					  NULL);

	def->pushFunction("pop",_("Pop"),_("Remove a value"),
					  NULL,
					  "pos",_("Position"),_("Which to pop."), "any",NULL,"-1",
					  NULL);

	def->pushFunction("swap",_("Swap"),_("Swap two positions"),
					  NULL,
					  "pos",_("Position 1"),_("Position to swap."), "int",NULL,NULL,
					  "pos2",_("Position 2"),_("Position to swap."), "int",NULL,NULL,
					  NULL);

	def->pushFunction("slide",_("Slide"),_("Same as push(pop(p1),p2)"),
					  NULL,
					  "pos",_("Position 1"),_("Position to take."), "int",NULL,NULL,
					  "pos2",_("Position 2"),_("Where to put."), "int",NULL,NULL,
					  NULL);

	def->pushFunction("removeValue",_("Remove value"),_("Remove this same value if it is in set"),
					  NULL,
					  "value",_("Value"),_("Value"), "any",NULL,NULL,
					  NULL);

	return def;
}

ObjectDef *SetValue::makeObjectDef()
{
	ObjectDef *def=Get_SetValue_ObjectDef();
	def->inc_count();
	return def;
}


int SetValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (len==1 && *func=='n') { *value_ret=new IntValue(values.n); return 0; }

	 //with parameters:
	try {
		if (isName(func,len,"removeValue")) {
			Value *v=parameters ? parameters->find("value") : NULL;
			if (!v) throw -1;

			int i=values.findindex(v);
			if (i>=0) values.remove(i);
			*value_ret=NULL;
			return 0;
		}

		if (isName(func,len,"push")) {
			int success=1;
			int pos=parameters ? parameters->findInt("pos",-1,&success) : -1;
			if (success!=0) pos=values.n; //pos not found, use top
			if (pos<0) pos=values.n;

			Value *v=parameters ? parameters->find("value") : NULL;
			if (!v) throw -1;

			Push(v,0, pos);
			*value_ret=NULL;
			return 0;
		}

		if (isName(func,len,"pop")) {
			int success=1;
			int pos=parameters ? parameters->findInt("pos",-1,&success) : -1;
			if (success!=0) pos=-1;
			if (pos<0) pos=values.n-1;

			if (pos<0 || pos>=values.n) throw 2; //index out of range!

			values.e[pos]->inc_count();
			*value_ret=values.e[pos];
			return 0;
		}

		if (isName(func,len,"swap")) {
			int success;
			int pos=parameters->findInt("pos",-1,&success);
			if (success!=0) throw -1; //wrong parameters
			int pos2=parameters->findInt("pos2",-1,&success);
			if (pos<0  || pos>=values.n)  throw 2; //index out of range!
			if (pos2<0 || pos2>=values.n) throw 2; //index out of range!
			values.swap(pos,pos2);
			return 0;
		}

		if (isName(func,len,"slide")) {
			int success;
			int pos=parameters->findInt("pos",-1,&success);
			if (success!=0) throw -1; //wrong parameters
			int pos2=parameters->findInt("pos2",-1,&success);
			if (pos<0  || pos >=values.n)  throw 2; //index out of range!
			if (pos2<0 || pos2>=values.n) throw 2; //index out of range!

			 //push(pop(p1),p2)
			Value *value=values.pop(pos);
			values.push(value,-1,pos);
			value->dec_count();
			return 0;
		}

	} catch (int e) {
		if (log) {
			if (e==-1) return -1; //can't use parameters!
			if (e==1) log->AddMessage(_("Missing parameter!"),ERROR_Fail);
			else if (e==2) log->AddMessage(_("Index out of range!"),ERROR_Fail);
		}
		return 1;
	}


	if (log) log->AddMessage(_("Unknown name!"),ERROR_Fail);
	return 1;
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


//----------------------------- ArrayValue ----------------------------------
/*! \class ArrayValue
 * Just a set with optional element type.
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
	return NULL; // ***
}

int ArrayValue::getValueStr(char *buffer,int len)
{
	int needed=3;//"[]\n"
	for (int c=0; c<values.n; c++) {
		needed+= 1 + values.e[c]->getValueStr(NULL,0);
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
		s->Push(v,1);
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


//--------------------------------- NullValue -----------------------------
int NullValue::getValueStr(char *buffer,int len)
{
	if (!buffer || len<4) return 4;
	sprintf(buffer,"null");
	return 0;
}

//! Return ref to this.. all null values are the same.
Value *NullValue::duplicate()
{
	inc_count();
	return this;
}



//--------------------------------- BooleanValue -----------------------------

/*! Empty string maps to 0.
 * "1","yes","true" (caseless) all map to false.
 * "0","no","false" (caseless) all map to false.
 * Anything else maps to 1.
 */
BooleanValue::BooleanValue(const char *val)
{
	i = 0;
	if (isblank(val)) i=0;
	else if (!strcmp(val,"0") || !strcasecmp(val,"no") || !strcasecmp(val,"false")) i=0;
	else if (!strcmp(val,"1") || !strcasecmp(val,"yes") || !strcasecmp(val,"true")) i=1;
	else i=1;
}


int BooleanValue::getValueStr(char *buffer,int len)
{
	if (!buffer || len<6) return 6;
	if (i) sprintf(buffer,"true");
	else sprintf(buffer,"false");
	return 0;
}

Value *BooleanValue::duplicate()
{ return new BooleanValue(i); }

int BooleanValue::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()) return 0;
	int isnum=0;
	int d=getNumberValue(v,&isnum);
	if (!isnum) return 0;

	if (d) i=1; else i=0;
	return 1;
}


ObjectDef default_BooleanValue_ObjectDef(NULL,"boolean",_("Boolean"),_("Boolean, true or false"),
							 "class", NULL, "true",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_BooleanValue_ObjectDef()
{ return &default_BooleanValue_ObjectDef; }

ObjectDef *BooleanValue::makeObjectDef()
{
	Get_BooleanValue_ObjectDef()->inc_count();
	return Get_BooleanValue_ObjectDef();
}



//--------------------------------- IntValue -----------------------------

IntValue::IntValue(const char *val, int base)
{
	i = (val ? strtol(val, NULL, base) : 0);
}

int IntValue::getValueStr(char *buffer,int len)
{
	if (!buffer || len<20) return 20;

	sprintf(buffer,"%ld",i);
	modified=0;
	return 0;
}

Value *IntValue::duplicate()
{ return new IntValue(i); }

int IntValue::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()) return 0;
	int isnum=0;
	int d=getNumberValue(v,&isnum);
	if (!isnum) return 0;

	i=d;
	return 1;
}


ObjectDef default_IntValue_ObjectDef(NULL,"int",_("Int"),_("Integers"),
							 "class", NULL, "0",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_IntValue_ObjectDef()
{ return &default_IntValue_ObjectDef; }

ObjectDef *IntValue::makeObjectDef()
{
	Get_IntValue_ObjectDef()->inc_count();
	return Get_IntValue_ObjectDef();
}


//--------------------------------- DoubleValue -----------------------------

/*! This would be a constructor, but there are too many compiler nags about
 * ambiguous parameters.
 */
void DoubleValue::Set(const char *val)
{
	d = (val ? strtod(val, NULL) : 0);
}

/*! \class DoubleValue
 * \brief Holds a real number.
 */
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

int DoubleValue::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()) return 0;
	int isnum=0;
	double dd=getNumberValue(v,&isnum);
	if (!isnum) return 0;

	d=dd;
	return 1;
}


ObjectDef default_DoubleValue_ObjectDef(NULL,"real",_("Real"),_("Real numbers"),
							 "class", NULL, "0.",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_DoubleValue_ObjectDef()
{
	ObjectDef *def=&default_DoubleValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("abs",_("Abs"),_("Absolute value"), NULL,
					  NULL);

	def->pushFunction("int",_("Int"),_("Integer portion"), NULL,
					  NULL);

	def->pushFunction("fraction",_("Fraction"),_("The non-integer part of a real number"), NULL,
					  NULL);

	return def;
}

ObjectDef *DoubleValue::makeObjectDef()
{
	Get_DoubleValue_ObjectDef()->inc_count();
	return Get_DoubleValue_ObjectDef();
}


/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int DoubleValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (isName(func,len, "abs")) {
		*value_ret=new DoubleValue(fabs(d));
		return 0;

	} else if (isName(func,len, "int")) {
		*value_ret=new IntValue(d);
		return 0;

	} else if (isName(func,len, "fraction")) {
		*value_ret=new DoubleValue(d-(int)d);
		return 0;
	}

	return -1;
}





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


Value *FlatvectorValue::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "x")) return new DoubleValue(v.x);
	if (extequal(extstring,len, "y")) return new DoubleValue(v.y);
	return NULL;
}

int FlatvectorValue::assign(FieldExtPlace *ext,Value *vv)
{
	if (!ext || !ext->n()) {
		if (vv->type()!=VALUE_Flatvector) return 0;
		v=dynamic_cast<FlatvectorValue*>(vv)->v;
		return 1;
	}

	if (ext->n()!=1) return -1;

	int isnum=0;
	double d=getNumberValue(vv,&isnum);
	if (!isnum) return 0;

	const char *str=ext->e(0);
	if (!strcmp(str,"x")) v.x=d;
	else if (!strcmp(str,"y")) v.y=d;
	else return -1;

	return 1;
}


ObjectDef default_FlatvectorValue_ObjectDef(NULL,"flatvector",_("Flatvector"),_("A two dimensional vector"),
							 "class", NULL, "(0,0)",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_FlatvectorValue_ObjectDef()
{
	ObjectDef *def=&default_FlatvectorValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("length",_("Length"),_("Length"), NULL,
					  NULL);

	def->pushFunction("norm2",_("Norm2"),_("Square of the length"), NULL,
					  NULL);

	def->pushFunction("angle",_("Angle"),_("Basically atan2(v.y,v.x)"), NULL,
					  NULL);

	def->pushFunction("normalize",_("Normalize"),_("Make length 1, but keep current angle."), NULL,
					  NULL);

	def->pushFunction("isnull",_("Is Null"),_("True if x and y are both 0."), NULL,
					  NULL);

	return def;
}

ObjectDef *FlatvectorValue::makeObjectDef()
{
	Get_FlatvectorValue_ObjectDef()->inc_count();
	return Get_FlatvectorValue_ObjectDef();
}


/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int FlatvectorValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (isName(func,len, "length")) {
		*value_ret=new DoubleValue(norm(v));
		return 0;

	} else if (isName(func,len, "norm2")) {
		*value_ret=new DoubleValue(norm2(v));
		return 0;

	} else if (isName(func,len, "angle")) {
		*value_ret=new DoubleValue(atan2(v.y,v.x));
		return 0;

	} else if (isName(func,len, "normalize")) {
		v.normalize();
		*value_ret=NULL;
		return 0;

	} else if (isName(func,len, "isnull")) {
		*value_ret=new BooleanValue(v.x==0 && v.y==0);
		return 0;
	}

	return -1;
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

Value *SpacevectorValue::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "x")) return new DoubleValue(v.x);
	if (extequal(extstring,len, "y")) return new DoubleValue(v.y);
	if (extequal(extstring,len, "z")) return new DoubleValue(v.z);
	return NULL;
}

int SpacevectorValue::assign(FieldExtPlace *ext,Value *vv)
{
	if (!ext || !ext->n()) {
		if (vv->type()!=VALUE_Spacevector) return 0;
		v=dynamic_cast<SpacevectorValue*>(vv)->v;
		return 1;
	}

	if (ext->n()!=1) return -1;

	int isnum=0;
	double d=getNumberValue(vv,&isnum);
	if (!isnum) return 0;

	const char *str=ext->e(0);
	if (!strcmp(str,"x")) v.x=d;
	else if (!strcmp(str,"y")) v.y=d;
	else if (!strcmp(str,"z")) v.z=d;
	else return -1;

	return 1;
}


ObjectDef default_SpacevectorValue_ObjectDef(NULL,"spacevector",_("Spacevector"),_("A three dimensional vector"),
							 "class", NULL, "(0,0)",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_SpacevectorValue_ObjectDef()
{
	ObjectDef *def=&default_SpacevectorValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("length",_("Length"),_("Length"), NULL,
					  NULL);

	def->pushFunction("norm2",_("Norm2"),_("Square of the length"), NULL,
					  NULL);

	def->pushFunction("angle",_("Angle"),_("Basically atan2(v.y,v.x)"), NULL,
					  NULL);

	def->pushFunction("normalize",_("Normalize"),_("Make length 1, but keep current angle."), NULL,
					  NULL);

	def->pushFunction("isnull",_("Is Null"),_("True if x and y are both 0."), NULL,
					  NULL);

	return def;
}

ObjectDef *SpacevectorValue::makeObjectDef()
{
	Get_SpacevectorValue_ObjectDef()->inc_count();
	return Get_SpacevectorValue_ObjectDef();
}


/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int SpacevectorValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (isName(func,len, "length")) {
		*value_ret=new DoubleValue(norm(v));
		return 0;

	} else if (isName(func,len, "norm2")) {
		*value_ret=new DoubleValue(norm2(v));
		return 0;

	} else if (isName(func,len, "angle")) {
		*value_ret=new DoubleValue(atan2(v.y,v.x));
		return 0;

	} else if (isName(func,len, "normalize")) {
		v.normalize();
		*value_ret=NULL;
		return 0;

	} else if (isName(func,len, "isnull")) {
		*value_ret=new BooleanValue(v.x==0 && v.y==0 && v.z==0);
		return 0;
	}

	return -1;
}




//--------------------------------- QuaternionValue -----------------------------
int QuaternionValue::getValueStr(char *buffer,int len)
{
	int needed=120;
	if (!buffer || len<needed) return needed;

	sprintf(buffer,"(%g,%g,%g,%g)", v.x, v.y, v.z, v.w);
	modified=0;
	return 0;
}

Value *QuaternionValue::duplicate()
{ return new QuaternionValue(v); }

Value *QuaternionValue::dereference(const char *extstring, int len)
{
	if (extequal(extstring,len, "x")) return new DoubleValue(v.x);
	if (extequal(extstring,len, "y")) return new DoubleValue(v.y);
	if (extequal(extstring,len, "z")) return new DoubleValue(v.z);
	if (extequal(extstring,len, "w")) return new DoubleValue(v.w);
	return NULL;
}

int QuaternionValue::assign(FieldExtPlace *ext,Value *vv)
{
	if (!ext || !ext->n()) {
		if (vv->type()!=VALUE_Quaternion) return 0;
		v=dynamic_cast<QuaternionValue*>(vv)->v;
		return 1;
	}

	if (ext->n()!=1) return -1;

	int isnum=0;
	double d=getNumberValue(vv,&isnum);
	if (!isnum) return 0;

	const char *str=ext->e(0);
	if (!strcmp(str,"x"))      v.x=d;
	else if (!strcmp(str,"y")) v.y=d;
	else if (!strcmp(str,"z")) v.z=d;
	else if (!strcmp(str,"w")) v.w=d;
	else return -1;

	return 1;
}


ObjectDef default_QuaternionValue_ObjectDef(NULL,"Quaternion",_("Quaternion"),_("Quaternion"),
							 "class", NULL, "(0,0,0,0)",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_QuaternionValue_ObjectDef()
{
	ObjectDef *def=&default_QuaternionValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("length",_("Length"),_("Length"), NULL,
					  NULL);

	def->pushFunction("norm2",_("Norm2"),_("Square of the length"), NULL,
					  NULL);

	def->pushFunction("normalize",_("Normalize"),_("Make length 1, but keep current angle."), NULL,
					  NULL);

	def->pushFunction("isnull",_("Is Null"),_("True if x and y are both 0."), NULL,
					  NULL);

	return def;
}

ObjectDef *QuaternionValue::makeObjectDef()
{
	Get_QuaternionValue_ObjectDef()->inc_count();
	return Get_QuaternionValue_ObjectDef();
}


/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int QuaternionValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (isName(func,len, "length")) {
		*value_ret=new DoubleValue(v.norm());
		return 0;

	} else if (isName(func,len, "norm2")) {
		*value_ret=new DoubleValue(v.norm2());
		return 0;

	} else if (isName(func,len, "normalize")) {
		v.normalize();
		*value_ret=NULL;
		return 0;

	} else if (isName(func,len, "isnull")) {
		*value_ret=new BooleanValue(v.x==0 && v.y==0 && v.z==0 && v.w==0);
		return 0;
	}

	return -1;
}




//--------------------------------- StringValue -----------------------------
//! Create a string value with the first len characters of s.
/*! If len<=0, then use strlen(s).
 */

StringValue::StringValue(const char *s, int len)
{ str=newnstr(s,len); }

/*! Warning! This quotes the string.
 */
int StringValue::getValueStr(char *buffer,int len)
{
	 //need to escape and put quotes around the string..

	int needed = (str ? strlen(str) : 0) + 3;
	for (int c=0; c<needed-3; c++) { if (str[c]=='\n' || str[c]=='\t') needed++; }
	if (!buffer || len<needed) return needed;

	buffer[0]='"';
	int pos=1, l = (str ? strlen(str) : 0);
	for (int c=0; c<l; c++) {
		if (str[c]=='\t') { buffer[pos++]='\\'; buffer[pos++]='t'; }
		else if (str[c]=='\n') { buffer[pos++]='\\'; buffer[pos++]='n'; }
		else buffer[pos++]=str[c];
	}
	buffer[pos++]='"';
	buffer[pos]='\0';
	modified=0;
	return 0;
}

Value *StringValue::duplicate()
{ return new StringValue(str); }

/*! Replace current str with nstr.
 */
void StringValue::Set(const char *nstr, int n)
{
	if (!nstr) {
		makestr(str, NULL);

	} else {
		int slen = strlen(nstr);
		if (n<0) n = slen;
		else if (n>slen) n = slen;
		makenstr(str,nstr, n);
	}
}


ObjectDef default_StringValue_ObjectDef(NULL,"string",_("String"),_("String"),
							 "class", NULL, "\"\"",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_StringValue_ObjectDef()
{
	ObjectDef *def=&default_StringValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("len",_("Length of string"),_("Length of string"), NULL,
					  NULL);

	def->pushFunction("sub",_("Substring"),_("Retrieve a substring"),
					  NULL,
					  "start",_("Start"),_("Counting from 0"), "int",NULL,NULL,
					  "end",  _("End"),  _("Counting from 0"), "int",NULL,"-1",
					  NULL);

	def->pushFunction("find",_("Find"),_("Return position of substring, or -1"),
					  NULL,
					  "str",_("String"),_("String to search for."), "string",NULL,"",
					  "from",  _("From"),  _("Start search from here."), "int",NULL,NULL,
					  NULL);

	def->pushFunction("replace",_("Replace"),_("Replace substsring with new string"),
					  NULL,
					  "str",_("String"),_("String to insert"), "string",NULL,NULL,
					  "start",_("Start"),_("Counting from 0"), "int",NULL,NULL,
					  "end",  _("End"),  _("Counting from 0"), "int",NULL,"-1",
					  NULL);

	return def;
}

ObjectDef *StringValue::makeObjectDef()
{
	Get_StringValue_ObjectDef()->inc_count();
	return Get_StringValue_ObjectDef();
}


/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int StringValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (isName(func,len, "len")) {
		*value_ret=new IntValue(str?strlen(str):0);
		return 0;

	} else if (isName(func,len, "sub")) {
		int err=0;
		int start=parameters->findIntOrDouble("start",-1,&err);
		if (err!=0) start=0;
		int end=parameters->findIntOrDouble("end",-1,&err);
		if (err!=0) end=str?strlen(str)-1:0;
		if (start<0) start=0;
		if (end>=(int)strlen(str)) end=strlen(str)-1;
		StringValue *s=new StringValue();
		if (end-start>=0) makenstr(s->str, str,end-start+1);
		*value_ret=s;
		return 0;

	} else if (isName(func,len, "find")) {
		int err=0;
		const char *search=parameters->findString("string",-1,&err);
		if (!str || !search) {
			*value_ret=new IntValue(-1);
			return 0;
		}
		int from=parameters->findIntOrDouble("from",-1,&err);
		if (from<0) from=0; else if (from>=(int)strlen(str)) from=strlen(str)-1;
		char *pos=strstr(str+from,search);
		*value_ret=new IntValue(pos ? pos-str : -1);
		return 0;

	} else if (isName(func,len, "replace")) {
		int err=0;
		int start=parameters->findIntOrDouble("start",-1,&err);
		if (err!=0) start=0;
		int end=parameters->findIntOrDouble("end",-1,&err);
		if (err!=0) end=str?strlen(str):0;
		if (start<0) start=0;
		if (end>(int)strlen(str)) end=strlen(str);

		const char *replace=parameters->findString("str",-1,&err);
		if (err!=0 || !replace) replace="";

		StringValue *s=new StringValue();
		if (end-start>=0) makenstr(s->str, str,start);
		appendstr(s->str, replace);
		if (end<(int)strlen(str)) appendstr(s->str,str+end);
		*value_ret=s;
		return 0;

	}

	return -1;
}

//----------------------------- BytesValue ----------------------------------
/*! \class BytesValue
 * Class to hold raw binary data.
 */

/*! len must be >=0. Assume 0 otherwise.
 */
BytesValue::BytesValue(const char *s, int length)
{
	if (length<0) length=0;
	str=NULL;
	len=length;
	if (len) {
		str=new char[len];
		memcpy(str,s,len);
	}
}

BytesValue::~BytesValue()
{
	if (str) delete[] str;
}

int BytesValue::getValueStr(char *buffer,int len)
{
	int needed=strlen("(binary data)")+1;
	if (!buffer || len<needed) return needed;
	sprintf(buffer,"(binary data)");
	return 0;
}

Value *BytesValue::duplicate()
{
	return new BytesValue(str,len);
}

int BytesValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
	if (isName(func,len, "len")) {
		*value_ret=new IntValue(len);
		return 0;

	} else if (isName(func,len, "sub")) {
		int err=0;
		int start=parameters->findIntOrDouble("start",-1,&err);
		if (err!=0) start=0;
		int end=parameters->findIntOrDouble("end",-1,&err);
		if (err!=0) end=str?strlen(str)-1:0;
		if (start<0) start=0;
		if (end>=(int)strlen(str)) end=strlen(str)-1;
		BytesValue *s=NULL;
		if (end-start>=0) s=new BytesValue(str+start,end-start+1);
		else s=new BytesValue(NULL,0);
		*value_ret=s;
		return 0;
	}

	return -1;
}

ObjectDef default_BytesValue_ObjectDef(NULL,"Bytes",_("Bytes"),_("Bytes"),
							 "class", NULL, NULL,
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_BytesValue_ObjectDef()
{
	ObjectDef *def=&default_BytesValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("len",_("Length of data"),_("Length of data"), NULL,
					  NULL);

	def->pushFunction("sub",_("Substring"),_("Retrieve a subsection of data"),
					  NULL,
					  "start",_("Start"),_("Counting from 0"), "int",NULL,NULL,
					  "end",  _("End"),  _("Counting from 0"), "int",NULL,"-1",
					  NULL);

	return def;
}

ObjectDef *BytesValue::makeObjectDef()
{
	Get_BytesValue_ObjectDef()->inc_count();
	return Get_BytesValue_ObjectDef();
}



//--------------------------------- FileValue -----------------------------

/*! \class FileValue
 * Holds a path to a file, with convenience functions for file access.
 */

//! Create a value of a file location.
FileValue::FileValue(const char *f, int len)
  : parts(2)
{
	filename=newnstr(f,len>0?len:(f?strlen(f):0));
	seperator='/';

}

FileValue::~FileValue()
{
	if (filename) delete[] filename;
}

int FileValue::Depth()
{
	return parts.n;
}

const char *FileValue::Part(int i)
{
	if (i<0 || i>=parts.n) return NULL;
	return parts.e[i];
}

void FileValue::Set(const char *nstr)
{
	makestr(filename, nstr);
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

ObjectDef default_FileValue_ObjectDef(NULL,"file",_("File"),_("File"),
							 "class", NULL, "/",
							 NULL, 0,
							 NULL, NULL);

ObjectDef *Get_FileValue_ObjectDef()
{
	ObjectDef *def=&default_FileValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("depth",_("Depth"),_("How many components path has"), NULL,
					  NULL);

	def->pushFunction("isLink",_("Is Link"),_("If path is a link"), NULL,
					  NULL);

	def->pushFunction("dirname",_("Directory"),_("Directory part of the path, with no final '/'."), NULL,
					  NULL);

	def->pushFunction("filename",_("File name"),_("Only the file name, no directories. Returns \"\" if not actual file."), NULL,
					  NULL);

	def->pushFunction("expandLink",_("Expand Link"),_("Return path to where link points to"), NULL,
					  NULL);

	def->pushFunction("stat",_("Stat"),_("Return a hash with various file properties"), NULL,
					  NULL);

	def->pushFunction("size",_("Size"),_("File size"), NULL,
					  NULL);

	def->pushFunction("expand",_("Expand"),_("Expand any '~' or relativeness, returning new path"), NULL,
					  NULL);

	def->pushFunction("contents",_("Contents"),_("Read in contents, if possible, returning new string"), NULL,
					  NULL);



	return def;
}

ObjectDef *FileValue::makeObjectDef()
{
	Get_FileValue_ObjectDef()->inc_count();
	return Get_FileValue_ObjectDef();
}


/*! Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int FileValue::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{
//	if (isName(func,len, "len")) {
//		*value_ret=new DoubleValue(str?strlen(str):0);
//		return 0;
//
//	} else if (isName(func,len, "sub")) {
//		int err=0;
//		int start=parameters->findIntOrDouble("start",-1,&err);
//		if (err!=0) start=0;
//		int end=parameters->findIntOrDouble("end",-1,&err);
//		if (err!=0) end=str?strlen(str)-1:0;
//		if (start<0) start=0;
//		if (end>=(int)strlen(str)) end=strlen(str)-1;
//		StringValue *s=new StringValue();
//		if (end-start>=0) makenstr(s->str, str,end-start+1);
//		*value_ret=s;
//		return 0;
//
//	}

	return -1;
}

//--------------------------------- EnumValue -----------------------------
/*! \class EnumValue
 * \brief Value for a particular one of an enum.
 *
 * The ObjectDef for the enum is always stored with the value.
 */

//! Create a value corresponding to a particular enum field.
/*! baseenum is incremented.
 *
 * which is an index into the fields array of baseenum.
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

int EnumValue::EnumId()
{
	int id=-1;
	objectdef->getEnumInfo(value, NULL,NULL,NULL, &id);
	return id;
}

const char *EnumValue::EnumLabel()
{
	const char *Nm = NULL;
	objectdef->getEnumInfo(value, NULL, &Nm);
	return Nm;
}

/*! Set value to the corresponding index. Also return it.
 */
int EnumValue::SetFromId(int id)
{
	if (!objectdef->fields) return -1;
	for (int c=0; c<objectdef->fields->n; c++) {
		if (objectdef->fields->e[c]->defaultvalue
				&& id == strtol(objectdef->fields->e[c]->defaultvalue, NULL, 10)) {
			value = c;
			return c;
		}
	}
	return -1;
}

Value *EnumValue::duplicate()
{ return new EnumValue(objectdef,value); }

//! Returns enumdef.
ObjectDef *EnumValue::makeObjectDef()
{
	return objectdef;
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

ObjectDef default_ObjectValue_ObjectDef(NULL,"ObjectWrapper",_("Object Wrapper"),_("Object Wrapper"),
							 "class",
							 NULL, //range
							 NULL, //default
							 NULL, 0, //fields
							 NULL, NULL); //funcs

ObjectDef *Get_ObjectValue_ObjectDef()
{
	ObjectDef *def=&default_ObjectValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("depth",_("Depth"),_("Bit depth of channels. Should return 8, 16, 24, 32, 32f, or 64f"), NULL,
					  NULL);


	return def;
}

ObjectDef *ObjectValue::makeObjectDef()
{
	Get_ObjectValue_ObjectDef()->inc_count();
	return Get_ObjectValue_ObjectDef();
}

void ObjectValue::SetObject(anObject *nobj, bool absorb_count)
{
	if (!absorb_count && nobj) nobj->inc_count();
	if (nobj != object) {
		if (object) object->dec_count();
		object = nobj;
		if (object) object->inc_count();
	}
}


//--------------------------------- ColorValue -----------------------------

ColorValue::ColorValue()
{
}

/*! Set from a hex string.
 */
ColorValue::ColorValue(const char *str)
  : color(LAX_COLOR_RGB,65535,0,0,0,65535)
{
	color.SetHexValue(str);
}

ColorValue::ColorValue(double r, double g, double b, double a)
  : color(LAX_COLOR_RGB, r,g,b,a,0, 1.0)
{}

/*! Objects gets count decremented.
 */
ColorValue::~ColorValue()
{
	DBG cerr <<"ColorValue destructor.."<<endl;
}

int ColorValue::getValueStr(char *buffer,int len)
{
	int needed=11;
	if (!buffer || len<needed) return needed;

	color.HexValue(buffer);
	modified=0;
	return 0;
}

Value *ColorValue::duplicate()
{
	char buffer[12];
	color.HexValue(buffer);
	return new ColorValue(buffer);
}

ObjectDef default_ColorValue_ObjectDef(NULL,"Color",_("Color"),_("Color"),
							 "class",
							 NULL, //range
							 NULL, //default
							 NULL, 0, //fields
							 NULL, NULL); //funcs

ObjectDef *Get_ColorValue_ObjectDef()
{
	ObjectDef *def=&default_ColorValue_ObjectDef;
	if (def->fields) return def;

	def->pushFunction("depth",_("Depth"),_("Bit depth of channels. Should return 8, 16, 24, 32, 32f, or 64f"), NULL,
					  NULL);


	return def;
}

ObjectDef *ColorValue::makeObjectDef()
{
	Get_ColorValue_ObjectDef()->inc_count();
	return Get_ColorValue_ObjectDef();
}


//--------------------------------- FunctionValue -----------------------------
/*! \class FunctionValue
 * Holds a coded function. These can be assign to variables, or called.
 */

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
 * Something like "paper=letter,imposition, 3" will be parsed
 * by parse_fields into an attribute like this:
 * <pre>
 *  paper letter
 *  - imposition
 *  - 3
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
 * the position in the styledef. The other 2 are not named, so we try to guess. (todo:) If
 * there is an enum field in the styledef, then the value of the parameter, "imposition"
 * for instance, is searched for in the enum values. If found, it is moved to the proper
 * position, and labeled accordingly. Any remaining unclaimed parameters are mapped
 * in order to the unused styledef fields. Extra or unknown values are placed at the end.
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
		if (def->getInfo(c,&name,NULL,NULL)==1) continue;
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
				break;
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
	if (!v) {
		*isnum=0; //it is a null!
		return 0;
	}
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

/*! Get integer value from bool, int, or real.
 * Return actual type in isnum, 0 for unknown, 1 for real, 2 for int, 3 for bool.
 */
int getIntValue(Value *v, int *isnum)
{
	if (!v) {
		*isnum=0; //it is a null!
		return 0;
	}
	if (v->type() == VALUE_Real) {
		*isnum=1;
		return dynamic_cast<DoubleValue*>(v)->d + .5;
	} else if (v->type() == VALUE_Int) {
		*isnum=2;
		return dynamic_cast<IntValue*>(v)->i;
	} else if (v->type() == VALUE_Boolean) {
		*isnum=3;
		return dynamic_cast<BooleanValue*>(v)->i;
	}
	*isnum=0;
	return 0;
}

/*! Like getNumberValue(), but swap order of returns.
 */
int isNumberType(Value *v, double *number_ret)
{
	int isnum = 0;
	double n = getNumberValue(v, &isnum);
	if (number_ret) *number_ret = n;
	return isnum;
}

/*! Return number of values in vector, or 0 if not a vector.
 * Real or int return 1. Flatvector 2, Spacevector 3, Quaternion 4.
 * values should be a double[4].
 */
int isVectorType(Value *v, double *values)
{
	int num = 0;
	if (values) {
		values[0] = values[1] = values[2] = values[3] = 0;
	}

	int vtype = v->type();

	if (vtype==VALUE_Real) {
		if (values) values[0] = dynamic_cast<DoubleValue*>(v)->d;
		return 1;

	} else if (vtype==VALUE_Int) {
		if (values) values[0] = dynamic_cast<IntValue*>(v)->i;
		return 1;

	} else if (vtype==VALUE_Boolean) {
		if (values) values[0] = dynamic_cast<BooleanValue*>(v)->i;
		return 1;

	} else if (vtype==VALUE_Flatvector) {
		if (values) {
			values[0] = dynamic_cast<FlatvectorValue*>(v)->v.x;
			values[1] = dynamic_cast<FlatvectorValue*>(v)->v.y;
		}
		return 2;
		
	} else if (vtype==VALUE_Spacevector) {
		if (values) {
			values[0] = dynamic_cast<SpacevectorValue*>(v)->v.x;
			values[1] = dynamic_cast<SpacevectorValue*>(v)->v.y;
			values[2] = dynamic_cast<SpacevectorValue*>(v)->v.z;
		}
		return 3;
		
	} else if (vtype==VALUE_Quaternion) {
		if (values) {
			values[0] = dynamic_cast<QuaternionValue*>(v)->v.x;
			values[1] = dynamic_cast<QuaternionValue*>(v)->v.y;
			values[2] = dynamic_cast<QuaternionValue*>(v)->v.z;
			values[3] = dynamic_cast<QuaternionValue*>(v)->v.w;
		}
		return 4;
	}

	return num;
}

//! Compare nonwhitespace until period with field, return 1 for yes, 0 for no.
/*! Return pointer to just after extension. If no match, next_ret gets NULL.
 * str can be "a.b.c.", and only "a" is checked, but field string must be "a",
 * it cannot have extra characters.
 * */
int extequal(const char *str, int len, const char *field, char **next_ret)
{
	unsigned int n=len;
	if (len<=0) while (isalnum(str[n]) || str[n]=='_') n++;

	if (n!=strlen(field) || strncmp(str,field,n)!=0) {
		if (next_ret) *next_ret=NULL;
		return 0;
	}

	str+=n;
	if (next_ret) *next_ret=const_cast<char*>(str);
	return 1;
}

/*! \ingroup misc
 * Return len==strlen(str) && !strncmp(longstr,str,len).
 * See also extequal(). The check here is simpler.
 */
int isName(const char *longstr,int len, const char *str)
{ return len==(int)strlen(str) && !strncmp(longstr,str,len); }



} // namespace Laidout

