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
// Copyright (C) 2009-2013 by Tom Lechner
//


#include <lax/fileutils.h>
#include <lax/units.h>
#include "calculator.h"
#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../headwindow.h"
#include "../version.h"

#include <readline/readline.h>
#include <readline/history.h>


//template implementation
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxFiles;

#define DBG 


namespace Laidout {


//----------------------------- CalcSettings -------------------------------------
/*! \class CalcSettings
 * \brief Bundle of various settings of a LaidoutCalculator.
 */

CalcSettings::CalcSettings(long pos,long line,int level, int base,char dec,char comp,char nullset)
{
	calculator=NULL;

     //parse state
    from=pos;
	curline=line;
    EvalLevel=level;

     //computation settings
    CurBase=base;
    decimal=dec;
    allow_complex=comp;
    allow_null_set_answers=nullset;
    zero_threshhold=1e-15;
}

CalcSettings::~CalcSettings()
{
}




//---------------------------------- OperatorFunction ------------------------------------------
/*! \class OperatorFunction
 * \brief Class for easy access to one or two parameter operators. Stack of these in CalculatorModule.
 *
 * Operators are strings of non-whitespace, non-letters, non-numbers, that do not contain ';',
 * or container characters such as "'(){}[].
 *
 */



OperatorFunction::OperatorFunction(const char *newop, int dir, int mod_id, OpFuncEvaluator *func,ObjectDef *opdef)
{
	op=newstr(newop);
	direction=dir;
	module_id=mod_id;
	function=func;
	def=opdef; if (def) def->inc_count();
}

OperatorFunction::~OperatorFunction()
{
	if (def) def->dec_count();
	if (op) delete[] op;
}

//! Return 1 if the first len characters of opstr matches op.
int OperatorFunction::isop(const char *opstr,int len)
{
	return ((int)strlen(op)==len && !strncmp(opstr,op,len));
}



//----------------------------------- OperatorLevel -----------------------------------------
/*! \class OperatorLevel 
 * \brief Class that holds all operators at a given direction and level of precedence
 */

OperatorLevel::OperatorLevel(int dir,int rank)
{
	direction=dir;
	priority=rank;
}

/*! 
 * \todo maybe have progressively search for anop here?
 */
OperatorFunction *OperatorLevel::hasOp(const char *anop,int n, int dir, int *index, int afterthis)
{
	if (afterthis<0) afterthis=-1;
	for (int c=afterthis+1; c<ops.n; c++) {
		if (dir==ops.e[c]->direction && !strncmp(anop,ops.e[c]->op,n) && n==(int)strlen(ops.e[c]->op)) {
			if (index) *index=c;
			return ops.e[c];
		}
	}
	*index=-1;
	return NULL;
//	--------
//	while (n) {
//		for (int c=0; c<ops.n; c++) {
//			if (!strncmp(anop,ops.e[c]->op,n) && n==(int)strlen(ops.e[c]->op)) {
//				*index=c;
//				return ops.e[c];
//			}
//		}
//		n--;
//	}
//	*index=-1;
//	return NULL;
}

/*! Return 0 for added, or -1 for the op with specified direction and OpFuncEvaluator.
 */
int OperatorLevel::pushOp(const char *op,int dir, OpFuncEvaluator *opfunc,ObjectDef *def, int module_id)
{
	for (int c=0; c<ops.n; c++) {
		if (!strcmp(op,ops.e[c]->op) && dir==ops.e[c]->direction && opfunc==ops.e[c]->function) return -1;
	}

	OperatorFunction *func=new OperatorFunction(op,dir,module_id,opfunc,def);
	ops.push(func);
	return 0;
}

//--------------------------- Entry/Dictionary ------------------------------------------
/*! \class Entry
 * \brief Class to simplify name lookups for LaidoutCalculator.
 */

Entry::Entry(const char *newname, int modid)
{
	name=newstr(newname);
	module_id=modid;
}

Entry::~Entry()
{
	if (name) delete[] name;
}

//! Return an objectDef associated with this entry. Default is return NULL.
ObjectDef *Entry::GetDef()
{ return NULL; }

//-------------------------- Dictionary
//class Dictionary
//{
//  public:
//	PtrStack<Entry> entries; //one entry for each available name
//	Dictionary();
//	~Dictionary() {}
//};
//

//--------------------------- OverloadedEntry
/*! For entries that are overloaded, push them into a special stack
 */
class OverloadedEntry : public Entry
{
  public:
	Laxkit::RefPtrStack<Entry> entries; //stores overloaded names

	OverloadedEntry(const char *newname, int modid);
	virtual ~OverloadedEntry();
	int type() { return VALUE_Overloaded; }
};

OverloadedEntry::OverloadedEntry(const char *newname, int modid)
  : Entry(newname,modid)
{
}

OverloadedEntry::~OverloadedEntry()
{
}

//--------------------------- NamespaceEntry
/*! Namespaces are stored in LaidoutCalculator::modules and individual scopes.
 * These can double as ClassEntry.
 */
class NamespaceEntry : public Entry, public ValueHash
{
  public:
	CalculatorModule *module;
	int isobject;
	NamespaceEntry(CalculatorModule *mod, const char *newname, int modid, int is_object);
	virtual ~NamespaceEntry();
	int type() { return (isobject ? VALUE_Class : VALUE_Namespace); }

	virtual const char *GetName() { if (module) return module->name; return NULL; }
	virtual ObjectDef *GetDef();
};

NamespaceEntry::NamespaceEntry(CalculatorModule *mod, const char *newname, int modid, int is_object)
  : Entry(newname,modid)
{
	isobject=is_object;
	module=mod;
	if (mod) mod->inc_count();
	sorted=1;
}

NamespaceEntry::~NamespaceEntry()
{
	if (module) module->dec_count();
}

ObjectDef *NamespaceEntry::GetDef()
{
	return module;
}

typedef NamespaceEntry ObjectDefEntry;

//--------------------------- FunctionEntry
/*! \class FunctionEntry
 * \brief Used for functions that are not part of an object.
 *
 * Object classes and functions are stored in the BlockInfo::scope_namespace elements. (???)
 */
class FunctionEntry : public Entry
{
  public:
	ObjectDef *def; // *** //def->defval would be code, if no function evaluator?
	char *code; //usually there will be EITHER code OR function.
	FunctionEvaluator *function;
	FunctionEntry(const char *newname, int modid, const char *newcode,FunctionEvaluator *func, ObjectDef *newdef);
	virtual ~FunctionEntry();
	int type() { return VALUE_Function; }
	virtual ObjectDef *GetDef();

	//virtual int Evaluate(const char *name,ValueHash *context,ValueHash *parameters,Value **value_ret, CalcSettings *settings, ErrorLog *log);
};

FunctionEntry::FunctionEntry(const char *newname, int modid, const char *newcode,FunctionEvaluator *func, ObjectDef *newdef)
  : Entry(newname,modid)
{
	def=newdef; if (def) def->inc_count();
	code=newstr(newcode);
	function=func;
	// *** if (function) function->inc_count();
}

FunctionEntry::~FunctionEntry()
{
	if (def) def->dec_count();
	if (code) delete[] code;
	// *** if (function) function->dec_count();
}

ObjectDef *FunctionEntry::GetDef()
{ return def; }


//--------------------------- ValueEntry
/*! \class ValueEntry
 * \brief Used for variables.
 *
 * Usually, the containing_object will be the scope's namespace.
 */
class ValueEntry : public Entry
{
  public:
	Value *value;
	Value *containing_object; //non-null for item is an element of this object
	ObjectDef *containing_space;
	ObjectDef *item;
	ValueEntry(const char *newname, int modid, ObjectDef *ns_element, ObjectDef *container, Value *object, Value *v);
	virtual ~ValueEntry();
	int type() { return VALUE_Variable; }
	virtual ObjectDef *GetDef();
	virtual Value *GetValue();
	virtual int SetVariable(const char *name,Value *v,int absorb);
};

ValueEntry::ValueEntry(const char *newname, int modid, ObjectDef *ns_element, ObjectDef *container, Value *object, Value *v)
  : Entry(newname,modid)
{
	containing_object=object;
	if (object) object->inc_count();

	containing_space=container;
	if (container) container->inc_count();

	item=ns_element;
	if (item) item->inc_count();

	value=v;
	if (!value && ns_element) value=ns_element->defaultValue;
	if (value) value->inc_count();
}

ValueEntry::~ValueEntry()
{
	if (value) value->dec_count();
	if (containing_object) containing_object->dec_count();
	if (containing_space) containing_space->dec_count();
	if (item) item->dec_count();
}

ObjectDef *ValueEntry::GetDef()
{ return value->GetObjectDef(); }

Value *ValueEntry::GetValue()
{
	if (value) return value;
	if (item) return item->defaultValue;
	return NULL;
}

int ValueEntry::SetVariable(const char *nname,Value *v,int absorb)
{
	if (!nname) nname=name;

	if (value!=v) {
		if (value) value->dec_count();
		value=v;
		if (v) v->inc_count();
	}

	if (containing_object) {
		FieldExtPlace fext;
		fext.push(nname);
		containing_object->assign(&fext,v);
		if (v && absorb) v->dec_count();
		return 0;
	}

	if (containing_space) {
		containing_space->SetVariable(nname,v,absorb);
		return 0;
	}
	if (v && absorb) v->dec_count();
	return 0;
}


//--------------------------- OperatorEntry
/*! \class OperatorEntry
 * \brief Entry for potentially overloaded operators.
 */
class OperatorEntry : public Entry
{
  public:
	int optype; //ltor, rtol, l, r
	Laxkit::RefPtrStack<FunctionEntry> functions; //overloading for op
	int type() { return VALUE_Operator; }

	OperatorEntry(const char *newname, int modid);
};

OperatorEntry::OperatorEntry(const char *newname, int modid)
  : Entry(newname,modid)
{}


//--------------------------- AliasEntry
/*! \class AliasEntry
 * \brief Used to alias names to other names (not operators).
 */
class AliasEntry : public Entry
{
  public:
	ObjectDef *aliasto;
	AliasEntry(ObjectDef *thing_to_alias, const char *newname, int modid);
	int type() { return VALUE_Alias; }
	virtual ~AliasEntry();
};

AliasEntry::AliasEntry(ObjectDef *thing_to_alias, const char *newname, int modid)
  : Entry(newname,modid)
{
	aliasto=thing_to_alias;
	if (aliasto) aliasto->inc_count();
}

AliasEntry::~AliasEntry()
{
	if (aliasto) aliasto->dec_count();
}

//------------------------------- BlockInfo ---------------------------------------
/*! \class BlockInfo
 * \brief Scope information for LaidoutCalculator.
 */

BlockInfo::BlockInfo()
{
	scope_namespace=NULL;
	scope_object=NULL;
	type=BLOCK_none;
	start_of_condition=0;
	start_of_loop=0;
	start_of_advance=0;
	current=0;
	max=0;
	word=NULL;
	list=NULL;
	parentscope=NULL;
	containing_object=NULL;
}

/*! var is taken, and delete'd in the destructor.
 */
BlockInfo::BlockInfo(CalculatorModule *mod, BlockTypes scopetype, int loop_start, int condition_start, char *var, Value *v)
{
	current=0;

	containing_object=NULL;
	parentscope=NULL;
	scope_object=NULL;
	scope_namespace=mod;
	if (mod) mod->inc_count();
	else scope_namespace=new CalculatorModule;

	start_of_condition=condition_start;
	start_of_loop=loop_start;
	start_of_advance=0;
	type=scopetype;
	word=var;

	if (scopetype==BLOCK_foreach) {
		list=dynamic_cast<SetValue*>(v);
		if (!list && !dynamic_cast<ValueHash*>(v)) list->dec_count();
		if (!list && dynamic_cast<ValueHash*>(v)) {
			list=new SetValue;
			ValueHash *hash=dynamic_cast<ValueHash*>(v);
			for (int c=0; c<hash->n(); c++) if (hash->e(c)) list->Push(hash->e(c),0);
			v->dec_count();
		}
		scope_namespace->pushVariable(word,word,NULL, NULL,0, list->values.e[0],0);
		AddName(scope_namespace,scope_namespace->fields->e[scope_namespace->fields->n-1],NULL);
	} else list=NULL;

	 //add names to scope if necessary
	if (scopetype==BLOCK_object) {
		containing_object=v; //any variables need to be set in containing_object, not the entry->namespace
		v->inc_count();
		ObjectDef *def=v->GetObjectDef();
		ObjectDef *item;
		if (def) {
			for (int c=0; c<def->getNumFields(); c++) {
				item=def->getField(c);
				AddName(def,item, containing_object);
			}
		}
	}
}

BlockInfo::~BlockInfo()
{
	if (scope_namespace) scope_namespace->dec_count();
	if (containing_object) containing_object->dec_count();
	if (list) list->dec_count();
	if (word) delete[] word;
}

const char *BlockInfo::BlockType()
{
	if (type==BLOCK_if) return "if";
	if (type==BLOCK_for) return "for";
	if (type==BLOCK_foreach) return "foreach";
	if (type==BLOCK_while) return "while";
	if (type==BLOCK_namespace) return "namespace";
	if (type==BLOCK_function) return "function";
	if (type==BLOCK_class) return "class";
	return "(unnamed block)";
}

/*! Add to entry list a bare value, unassociated with a namespace.
 * Does not check for preexistence of the value, it will add overloaded.
 *
 * This is used for coded function value, passing in context and parameters.
 */
int BlockInfo::AddValue(const char *name, Value *v)
{
	 //find position in stack
	int pos=-1;
	int found=-1;
	int s=0,e=dict.n-1,m, cmp;

	if (s<=e) {
		cmp=strcmp(name,dict.e[s]->name);
		if (cmp==0) { found=1; pos=s; }
		else if (cmp<0) { found=0; pos=s; } //was before lowest
		
		if (found<0) {
			cmp=strcmp(name,dict.e[e]->name);
			if (cmp==0) { found=1; pos=e; }
			else if (cmp>0) { found=0; pos=e+1; }
		}
		
		if (found<0) {
			found=0;
			while (pos<0 && s<e) {
				m=(s+e)/2;
				cmp=strcmp(name,dict.e[m]->name);
				if (cmp==0) { found=1; pos=m; break; }
				if (cmp<0) {
					e=m-1;
					cmp=strcmp(name,dict.e[e]->name);
					if (cmp==0) { found=1; pos=e; break; }
					if (cmp>0) { pos=m; break; } //between m-1 and m
				} else {
					s=m+1;
					cmp=strcmp(name,dict.e[s]->name);
					if (cmp==0) { found=1; pos=s; break; }
					if (cmp<0) { pos=s; break; } //between m-1 and m
				}
			}
			if (!found && pos<0) pos=s;
		}
	}
	if (found<0) found=0;

	if (found) {
		OverloadedEntry *oo=dynamic_cast<OverloadedEntry*>(dict.e[pos]);
		if (oo) {
			//for (int c=0; c<oo->entries.n; c++) {
			//	if (***isSameEntry(item,oo->entries.e[c])) return -2;
			//}
			//item not found, so add to top of overloads

		} else {
			 //need to overload name maybe
			//if (***isSameEntry(item,dict.e[pos])) return -2; //already there!
			Entry *entry=dict.pop(pos);
			oo=new OverloadedEntry(name,0);
			oo->entries.push(entry);
			dict.push(oo,1,pos); //replace old entry with overloaded entry
		}
		Entry *newentry=new ValueEntry(name, 0,NULL,NULL, containing_object, v);

		oo->entries.push(newentry,1,-1);
		return -1;
	}

	 //entry not found, so add!
	Entry *entry=new ValueEntry(name, 0,NULL,NULL, containing_object, v);
	dict.push(entry,1,pos);

	return 0;
}

//! Add item, which is assumed to be in mod somewhere, to the scope's dictionary.
/*! Return 0 for added.
 * -1 for name there, but added overloaded.
 * -2 for item already there, so nothing done.
 * 1 for error and not added.
 */
int BlockInfo::AddName(CalculatorModule *mod, ObjectDef *item, Value *container_v)
{
	if (!item) return 1;
	const char *name=item->name;

	 //find position in stack
	int pos=-1;
	int found=-1;
	int s=0,e=dict.n-1,m, cmp;

	if (s<=e) {
		cmp=strcmp(name,dict.e[s]->name);
		if (cmp==0) { found=1; pos=s; }
		else if (cmp<0) { found=0; pos=s; } //was before lowest
		
		if (found<0) {
			cmp=strcmp(name,dict.e[e]->name);
			if (cmp==0) { found=1; pos=e; }
			else if (cmp>0) { found=0; pos=e+1; }
		}
		
		if (found<0) {
			found=0;
			while (pos<0 && s<e) {
				m=(s+e)/2;
				cmp=strcmp(name,dict.e[m]->name);
				if (cmp==0) { found=1; pos=m; break; }
				if (cmp<0) {
					e=m-1;
					cmp=strcmp(name,dict.e[e]->name);
					if (cmp==0) { found=1; pos=e; break; }
					if (cmp>0) { pos=m; break; } //between m-1 and m
				} else {
					s=m+1;
					cmp=strcmp(name,dict.e[s]->name);
					if (cmp==0) { found=1; pos=s; break; }
					if (cmp<0) { pos=s; break; } //between m-1 and m
				}
			}
			if (!found && pos<0) pos=s;
		}
	}
	if (found<0) found=0;

	if (found) {
		OverloadedEntry *oo=dynamic_cast<OverloadedEntry*>(dict.e[pos]);
		if (oo) {
			for (int c=0; c<oo->entries.n; c++) {
				if (isSameEntry(item,oo->entries.e[c])) return -2;
			}
			//item not found, so add to top of overloads

		} else {
			 //need to overload name maybe
			if (isSameEntry(item,dict.e[pos])) return -2; //already there!
			Entry *entry=dict.pop(pos);
			oo=new OverloadedEntry(item->name,0);
			oo->entries.push(entry);
			dict.push(oo,1,pos); //replace old entry with overloaded entry
		}
		Entry *newentry=createNewEntry(item,mod,container_v);
		oo->entries.push(newentry,1,-1);
		return -1;
	}

	 //entry not found, so add!
	Entry *entry=createNewEntry(item,mod,container_v);
	if (!entry) return 1; //cannot add this item!
	dict.push(entry,1,pos);

	return 0;
}

Entry *BlockInfo::createNewEntry(ObjectDef *item, CalculatorModule *module, Value *container_v)
{
	if (item->format==VALUE_Operator) {
		return NULL;

	} else if (item->format==VALUE_Variable) {
		return new ValueEntry(item->name,module->object_id, item, module, container_v,NULL);

	} else if (item->format==VALUE_Class) {
		return new NamespaceEntry(item, item->name, module->object_id, 1);

	} else if (item->format==VALUE_Function) {
		//FunctionEntry(const char *newname, int modid, const char *newcode,FunctionEvaluator *func, ObjectDef *newdef);
		return new FunctionEntry(item->name,module->object_id, NULL,NULL,item);

	} else if (item->format==VALUE_Namespace) {
		return new NamespaceEntry(item, item->name, module->object_id, 0);

	//} else if (item->format==VALUE_Alias) {   
	}

	return NULL;
}

int BlockInfo::isSameEntry(ObjectDef *item, Entry *entry)
{
	switch (entry->type()) {
		case VALUE_Variable : {
			ValueEntry *vv=dynamic_cast<ValueEntry*>(entry);
			return vv->item==item;
		  }

		case VALUE_Class    :
		case VALUE_Namespace: {
			NamespaceEntry *vv=dynamic_cast<NamespaceEntry*>(entry);
			return vv->module==item;
		  }

		case VALUE_Function : {
			FunctionEntry *vv=dynamic_cast<FunctionEntry*>(entry);
			return vv->def==item;
		  }

		//case VALUE_Operator :
		//case VALUE_Alias    :
	};
	return 0;
}

Entry *BlockInfo::FindName(const char *name,int len)
{
	int s=0,e=dict.n-1,m=0;
	int nlen;

	if (s<=e) {
		int cmp=strncmp(name,dict.e[s]->name,len);
		if (cmp==0) {
			if ((int)strlen(dict.e[s]->name)==len) return dict.e[s];
			cmp=1;
		}
		if (cmp<0) return NULL; //it is less than lowest element
		
		cmp=strncmp(name,dict.e[e]->name,len);
		if (cmp==0) {
			if ((int)strlen(dict.e[e]->name)==len) return dict.e[e];
			cmp=1;
		}
		if (cmp>0) return NULL; //it is greater than greatest element
		
		while (s<e) {
			m=(s+e)/2;
			if (m == s) break; //s was right next to e
			cmp=strncmp(name,dict.e[m]->name,len);
			nlen=strlen(dict.e[m]->name);
			if (cmp==0 && len<nlen) cmp=-1;
			if (cmp==0) return dict.e[m];

			if (cmp<0) {
				e=m-1;

				nlen=strlen(dict.e[e]->name);
				cmp=strncmp(name,dict.e[e]->name,len);
				if (cmp==0 && len<nlen) cmp=-1;
				if (cmp==0) return dict.e[e];

				if (cmp>0) return NULL; //between m-1 and m, not in list
			} else {
				s=m+1;

				nlen=strlen(dict.e[s]->name);
				cmp=strncmp(name,dict.e[s]->name,len);
				if (cmp==0 && len<nlen) cmp=-1;
				if (cmp==0) return dict.e[s];

				if (cmp<0) return NULL; //between m-1 and m, not in list
			}
		}
	}
	return NULL;
}


//--------------------------------------- NamespaceValue ------------------------------------
/*! \class NamespaceValue
 *  Hold a namespace, for the purpose of easing dereferencing.
 */
class NamespaceValue : public Value
{
  public:
	Value *usethis;
	NamespaceValue(ObjectDef *ns, Value *fromthis=NULL);
	virtual ~NamespaceValue();
	virtual int type();
 	virtual ObjectDef *makeObjectDef() { return NULL; }
	virtual Value *duplicate() { return NULL; }
	virtual Value *dereference(const char *extstring, int len);
	//virtual int getValueStr(char *buffer,int len);
};

//typedef NamespaceValue FunctionValue; <-- see values.h
//typedef NamespaceValue ClassValue;

NamespaceValue::NamespaceValue(ObjectDef *ns, Value *fromthis)
{
	usethis=fromthis;
	if (usethis) usethis->inc_count();

	objectdef=ns;
	if (objectdef) objectdef->inc_count();
}

NamespaceValue::~NamespaceValue()
{
	if (usethis) usethis->dec_count();
}

/*! NamespaceValue objects may be namespaces, object classes, or functions.
 * This just returns objectdef->format.
 */
int NamespaceValue::type()
{
	if (!objectdef) return 0; //shouldn't happen, but just in case
	//return objectdef->format==VALUE_Namespace ? VALUE_Namespace : VALUE_Class;
	return objectdef->format;
}

Value *NamespaceValue::dereference(const char *extstring, int len)
{
	ObjectDef *def=objectdef->FindDef(extstring,len);
	if (!def) return NULL;

	if (def->format==VALUE_Variable) {
		if (!def->defaultValue) return NULL; //what if there is a scripted or Evaluator value?
		def->defaultValue->inc_count();
		return def->defaultValue;
	}
	
	if (def->format==VALUE_Function) {
		Value *v=new NamespaceValue(def);
		return v;
	}

	if (def->format==VALUE_Class) {
		Value *v=new NamespaceValue(def);
		return v;
	}

	if (def->format==VALUE_Namespace) {
		Value *v=new NamespaceValue(def);
		return v;
	}

	if (def->format==VALUE_Operator) {
	}

	return NULL;
}


//-------------------------------------- LValue -------------------------------------------
/*! \class LValue
 * Hold something that can take an assignment if necessary. This is only used internally
 * during the parsing of expressions. They should not be passed around externally.
 */
class ValueEntry;

class LValue : public Value
{
  public:
	char *name;

	ValueEntry *entry; //entry the base is found in, might just be an alias to base
	Value *basevalue; //assumed to NOT be an LValue
	ObjectDef *basedef; //if basevalue==NULL, then assume extension is from namespace basedef
	FieldExtPlace extension;

	LValue(const char *newname,int len, Value *v, ValueEntry *ve, ObjectDef *def, FieldExtPlace *p);
	virtual ~LValue();
	virtual int type() { return VALUE_LValue; }
 	virtual ObjectDef *makeObjectDef() { return NULL; }
	virtual Value *duplicate();
	virtual Value *Resolve(); //remove lvalue state, returns new instance (or inc counted)
	virtual Value *dereference(const char *extstring, int len);
	virtual Value *dereference(int index);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual int getValueStr(char *buffer,int len);
};

LValue::LValue(const char *newname,int len, Value *v, ValueEntry *ve, ObjectDef *def, FieldExtPlace *p)
{
	name=newnstr(newname,len);
	basevalue=v; if (v) v->inc_count();
	basedef=def; if (def) def->inc_count();
	entry=ve;

	if (p) extension=*p;
}

LValue::~LValue()
{
	if (basedef) basedef->dec_count();
	if (basevalue) basevalue->dec_count();
	if (name) delete[] name;
}

Value *LValue::duplicate()
{ return NULL; }

int LValue::getValueStr(char *buffer,int len)
{
	Value *v=Resolve();
	if (!v) return Value::getValueStr(buffer,len);
	int s=v->getValueStr(buffer,len);
	v->dec_count();
	return s;
}

int LValue::assign(FieldExtPlace *ext,Value *v)
{
	int oldn=extension.n();

	char *str;
	int i;

	if (ext && ext->n()) {
		for (int c=0; c<ext->n(); c++) {
			str=ext->e(c,&i);
			if (str) extension.push(str);
			else extension.push(i);
		}
	}

	str=NULL;
	i=-1;
	if (extension.n()) {
		str=extension.pop(&i);
	}

	Value *vv=Resolve();
	FieldExtPlace fext;
	if (str) fext.push(str);
	else if (i>=0) fext.push(i);

	if (fext.n()) {
		int status=vv->assign(&fext,v);
		vv->dec_count();
		while (extension.n()>oldn) extension.remove();
		return status;
	}

	if (entry) {
		 //replace old value totally with new value
		entry->SetVariable(entry->name, v, 0);
		while (extension.n()>oldn) extension.remove();
		v->inc_count();
		if (basevalue) basevalue->dec_count();
		basevalue=v;
		return 1;
	}
	
	//else assign value to existing value
	int status=vv->assign(NULL,v);
	vv->dec_count();
	while (extension.n()>oldn) extension.remove();
	v->inc_count();
	if (basevalue) basevalue->dec_count();
	basevalue=v;
	return status;
}

Value *LValue::dereference(const char *extstring, int len)
{
	if (!basevalue) return NULL;

	Value *v=Resolve();
	if (!v) return NULL;

	char ext[len+1];
	strncpy(ext,extstring,len);
	ext[len]='\0';

	ObjectDef *def=v->GetObjectDef();
	if (def) def=def->FindDef(extstring,len);
	if (def) {
		// *** if (def->format==VALUE_Variable)  ... just add the extension
		// ***TEMP:
		if (def->format==VALUE_Variable) {
			Value *vv=v->dereference(extstring,len);
			v->dec_count();
			return vv;
		}
		
		if (def->format==VALUE_Function) {
			Value *fv=new NamespaceValue(def,v);
			v->dec_count();
			return fv;
		}

		if (def->format==VALUE_Class) {
			Value *v=new NamespaceValue(def);
			return v;
		}

		if (def->format==VALUE_Namespace) {
			Value *v=new NamespaceValue(def);
			return v;
		}
	}

	extension.push(ext);
	inc_count();
	return this;
}

/*! Resolve old value, then push the index to extension.
 */
Value *LValue::dereference(int index)
{
	if (!basevalue) return NULL;

	Value *v=NULL;
	if (extension.n()) v=Resolve();
	if (v) {
		basevalue->dec_count();
		basevalue=v;
	}
	entry=NULL;
	extension.push(index);
	inc_count();
	return this;
}

//! Remove lvalue state, returns new instance (or inc counted)
/*! Returns NULL, if unable to dereference.
 */
Value *LValue::Resolve()
{
	Value *v=basevalue, *v2;
	if (v) v->inc_count();
	else {
		cerr << " Warning: missing LValue::basevalue"<<endl;
		return NULL;
	}
	char *str;
	int i;

	while (extension.n()) {
		str=extension.pop(&i, 0);
		if (str) {
			v2=v->dereference(str,strlen(str));
		} else {
			v2=v->dereference(i);
		}
		if (!v2) {
			 //unable to dereference!
			v->dec_count();
			return NULL; 
		}

		v->dec_count();
		v=v2;
	}
	return v;
}


//------------------------------------ LaidoutCalculator -----------------------------
/*! \class LaidoutCalculator
 * \brief Command processing backbone.
 *
 * This is an object oriented scripting language, featuring loadable modules, function and
 * operater overloading, with the ability to define your own new operators.
 *
 * The LaidoutApp class maintains a single calculator to aid
 * in default processing. When the '--command' command line option is 
 * used, that is the calculator used.
 *
 * Each command prompt pane will have a separate calculator shell,
 * thus you may define your own variables and functions in each pane.
 * However, each pane shares the same Laidout data (the laidout project,
 * styles, etc). This means if you add a new document to the project,
 * then the document is accessible to all the command panes, and the
 * LaidoutApp calculator, but if you define your own functions,
 * they remain local, unless you specify them as shared across
 * calculators.
 *
 * \todo ***** this class will eventually become the entry point for other language bindings
 *   to Laidout, standardizing how messages are passed, and how objects are accessed(?)
 *   ... need to research swig, and see about bindings through that
 */
/*! \var Laxkit::RefPtrStack<ObjectDef> LaidoutCalculator::modules
 * \brief List of modules globally available at the moment in the LaidoutCalculator.
 *
 * Define any number of functions, variables, object types, operators, and defined variables.
 * All of these things are accessed by "modulename.thing". In the calculator, you
 * can say "using modulename;" to import all items into a script's space.
 */


LaidoutCalculator::LaidoutCalculator()
  : leftops(OPS_Left,0),
	rightops(OPS_Right,0)
{
	dir=NULL; //working directory...
	calcsettings.decimal=0;

	from=0;
	curline=0;
	curexprs=NULL;
	curexprslen=0;
	calcerror=0;
	calcmes=NULL;
	messagebuffer=NULL;
	errorlog=&default_errorlog;
	last_answer=NULL;

	global_scope.scope_namespace=new ObjectDef(NULL, "Global", _("Global"), _("Global namespace"), "namespace",NULL,NULL);
	scopes.push(&global_scope,0); //push so as to not delete global scope

	DBG cerr <<" ~~~~~~~ New Calculator created."<<endl;


	 //initialize base modules
	InstallInnate();
	InstallBaseTypes();

	 //things specific to Laidout:
	InstallModule(&stylemanager,1); //autoimport name only

	//DBG cerr <<"Calculator Contents: "<<endl;
	//DBG
	//DBG	for (int c=0; c<modules.n; c++) {
	//DBG		cerr <<"Module:"<<endl;
	//DBG		modules.e[c]->dump_out(stderr, 2, 0, NULL);
	//DBG	}
}

LaidoutCalculator::~LaidoutCalculator()
{
	if (dir) delete[] dir;
	if (curexprs) delete[] curexprs;
	if (calcmes) delete[] calcmes;
	if (messagebuffer) delete[] messagebuffer;
	if (last_answer) last_answer->dec_count();
}

const char *LaidoutCalculator::Id()          
{ return "Laidout"; }

const char *LaidoutCalculator::Name()        
{ return _("Laidout Default"); }

const char *LaidoutCalculator::Description() 
{ return _("Default Laidout interpreter"); }

const char *LaidoutCalculator::Version()     
{ return "0.1"; }

int LaidoutCalculator::InitInterpreter()
{ return 0; }

int LaidoutCalculator::CloseInterpreter()
{ return 0; }

LaxFiles::Attribute *LaidoutCalculator::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	// ***
	return att;
}

void LaidoutCalculator::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	// ***
}


/*! Kind of a lightweight InstallModule(). This just adds plain names to the global namespace.
 * Any name that exists already has its value replaced.
 *
 * Returns 0 for success, nonzero for fail.
 */
int LaidoutCalculator::InstallVariables(ValueHash *values)
{
	Entry *entry;
	for (int c=0; c<values->n(); c++) {
		entry=global_scope.FindName(values->key(c),strlen(values->key(c)));
		if (entry) {
			ValueEntry *ve=dynamic_cast<ValueEntry*>(entry);
			ve->SetVariable(NULL,values->value(c),0);
		} else {
			global_scope.AddValue(values->key(c),values->value(c));
		}
	}
	return 0;
}

//! Add module to list of available modules.
/*! If autoimport==0, then only make available, do not actually make names available.
 * If autoimport==1, then make the module name accessible in the current namespace.
 * If autoimport==2, tehn automatically import all names in module to the current namespace.
 * Please note that operators are always made available in the global namespace.
 *
 * Return 0 for installed, or 1 for not able to install for some reason.
 */
int LaidoutCalculator::InstallModule(CalculatorModule *module, int autoimport)
{
	modules.push(module);
	//global_scope.AddName(module,module,NULL);
	if (autoimport) currentLevel()->AddName(module,module,NULL);
	if (autoimport==2) importAllNames(module);
	importOperators(module);
	return 0;
}

//! Make module accessible from current scope, and install its operators.
/*! module is in this->modules list. Return 1 for module not found, else 0.
 *
 * If module is already accessible from current scope, then it is not added
 * to the current scope, but names are.
 */
int LaidoutCalculator::ImportModule(const char *name, int allnames)
{
	CalculatorModule *module=NULL;
	for (int c=0; c<modules.n; c++) {
		if (!strcmp(modules.e[c]->name,name)) { module=modules.e[c]; break; }
	}
	if (!module) return 1;

	 //see if module already accessible
	Entry *entry=NULL;
	BlockInfo *scope=currentLevel();
	int n=strlen(name);
	while (scope) {
		entry=scope->FindName(name,n);
		if (entry) {
			if (entry->type()==VALUE_Overloaded) {
				OverloadedEntry *oo=dynamic_cast<OverloadedEntry*>(entry);

				if (oo->entries.n) {
					entry=oo->entries.e[oo->entries.n-1]; //note it only checks top of overloaded, this might be wrong way to do it
				}
			}
		}
		if (entry->type()==VALUE_Namespace && module==dynamic_cast<NamespaceEntry*>(entry)->module) 
			break;
		entry=NULL;
		scope=scope->parentscope;
	}
	if (!entry) {
		 //module is not accessible at any scope, so add it to current
		currentLevel()->AddName(module,module,NULL);
		importOperators(module);
	}

	if (allnames) importAllNames(module);
	return 0;
}

//! Return 0 for removed, or 1 for name not found.
int LaidoutCalculator::RemoveModule(const char *modulename)
{
	int c=0;
	for (c=0; c<modules.n; c++) {
		if (!strcmp(modulename, modules.e[c]->name)) {
			cerr << " *** WARNING! need to implement removing all name and dependencies to module from all namespaces when remove module"<<endl;
			modules.remove(c);
		}
	}
	if (c==modules.n) return 1;
	return 0;
}


int LaidoutCalculator::RunShell()
{
	cout << "Laidout "<<LAIDOUT_VERSION<<" shell. Type \"quit\" to quit."<<endl;

//----------non-readline variant--------------------------
//	int numl=0;
//	char *temp=NULL;
//	char prompt[50];
//	string input;
//
//	while (1) {
//		numl++;
//		
//		strcpy(prompt,"\n\nInput(");
//		char *temp2=itoa(numl,prompt+strlen(prompt));
//		if (temp2) temp2[0]='\0';
//		strcat(prompt,"): ");
//	
//		cout << prompt;
//		if (!getline(cin, input)) break;
//		if (input=="quit") return 0;
//
//		temp=In(input.c_str());
//		cout << temp;
//
//		delete[] temp;
//	}
//----------end non-readline variant--------------------------


//---------readline variant--------------(rough draft: needs debugging to work properly)
	int numl=0;
	char *temp=NULL;
	char *input=0;
	char prompt[50];
	int status;

	while (1) {
		numl++;
		
		strcpy(prompt,"\n\nInput(");
		char *temp2=itoa(numl,prompt+strlen(prompt));
		if (temp2) temp2[0]='\0';
		strcat(prompt,"): ");
	
		input=readline(prompt); //tempexprs is malloc based

		if (input==NULL) continue;
		//DBG cout<<"\nreadline:"<<input;

		if (strlen(input)>0) {
			if (!strcmp(input,"quit")) return 0;
			add_history(input); //readline takes it, don't free here (i think)
		} else numl--; //don't count blank line

		temp=In(input,&status);
		cout << temp;

		if (!isblank(temp) && status==1) {
			input=(char*)malloc(strlen(temp)+1);
			strcpy(input,temp);
			add_history(input);
		}

		delete[] temp;
	}
//---------end readline variant--------------

	return 0;
}


//! Process a command or script. Returns a new char[] with the result.
/*! Do not forget to delete[] the returned array!
 * 
 * If in results in a value, then 1 is returned.
 * If in results in something done, but no value, then 2 is returned.
 * If in results in an error, then 0 is returned.
 * If in results in twisty passages, then -1 is returned.
 *
 */
char *LaidoutCalculator::In(const char *in, int *return_type)
{
	makestr(messagebuffer,NULL);

	Value *v=NULL;
	default_errorlog.Clear();
	errorlog=&default_errorlog;
	int status=evaluate(in,-1, &v, errorlog);

	char *buffer=NULL;
	int len=0;
	if (v) {
		 //assume success
		v->getValueStr(&buffer,&len,1);
		appendstr(messagebuffer,buffer);
		v->dec_count();
		if (return_type) *return_type=1;

	} else if (status!=0) {
		 //there was an error
		if (return_type) *return_type=0;
		if (errorlog->Total()) {
			makestr(messagebuffer,NULL);
			for (int c=0; c<errorlog->Total(); c++) {
				appendline(messagebuffer,errorlog->Message(c,NULL,NULL,NULL,NULL));
			}
		} else makestr(messagebuffer,calcmes);
		return newstr(messagebuffer);

	}
	
	if (messagebuffer) return newstr(messagebuffer);

	if (from!=0) {
		if (return_type) *return_type=2;
		return newstr(_("Ok."));
	}

	if (return_type) *return_type=-1;
	return newstr(_("You are surrounded by twisty passages, all alike."));
}

//! Return object type info for an expression
/*! \todo *** WARNING, this is incomplete, it does not allow full parsing of a location,
 *   it should be a combination of "show" and eval().
 */
ObjectDef *LaidoutCalculator::GetInfo(const char *expr)
{
	if (isblank(expr)) return NULL;

	ObjectDef *def=NULL;
	int scope,module,index;
	int len=strlen(expr);
	Entry *entry=findNameEntry(expr,len, &scope, &module, &index);
	int pos=0;
	if (entry) {
		//Overloaded, function, value, namespace, alias, class, operator
		def=entry->GetDef();
		pos+=len;
	}

	 // *** WARNING! this does not do index parsing, so "blah[123].blah" will not work,
	 //     nor will blah.(34+2).blah
	int n;
	const char *showwhat=NULL;
	while (isspace(expr[pos])) pos++;
	while (def && expr[pos]=='.') { //for "Math.sin", for instance
		n=0;
		while (pos+n<len && (isalnum(expr[pos+n]) || expr[pos+n]=='_')) n++;
		showwhat=expr+pos;

		ObjectDef *ssd=def->FindDef(showwhat,n);
		if (!ssd) break;
		pos+=n;
		def=ssd;
	} //if foreach dereference

	return def;
}

//! Process a command or script. Returns a Value object with the result, if any in value_ret.
/*! The function's return value is 0 for success, -1 for success with warnings, 1 for error in
 *  input. Note that it is possible for 0 to be returned and also have value_ret return NULL.
 *
 *  This will parse multiple expressions, and is meant to return a value. Only characters up to but
 *  not including in+len are
 *  parsed. If there are multiple expressions, then the value from the final expression is returned.
 *
 *  This function is called as necessary by In().
 *  The other evaluate() is used to process functions during a script, not directly by the user.
 */
int LaidoutCalculator::evaluate(const char *in, int len, Value **value_ret, ErrorLog *log)
{
	int num_expr_parsed=0;
	ClearError();
	errorlog = log;
	if (errorlog == NULL) errorlog=&default_errorlog;

	if (in==NULL) { calcerr(_("Blank expression")); return 1; }

	Value *answer=NULL;
	if (len<0) len=strlen(in);

	newcurexprs(in,len);

	if (curexprslen>len) curexprslen=len;

	skipwscomment();
	int curscope=scopes.n;


	int tfrom=-1;
	int numerr=errorlog->Total();


	while(!calcerror && tfrom!=from && from<curexprslen) { 
		tfrom=from;
		if (answer) { answer->dec_count(); answer=NULL; }


		if (sessioncommand()) { //  checks for session commands 
			if (calcerror) break;
			//if (!messagebuffer) messageOut(_("Ok."));
			skipwscomment();
			if (from>=curexprslen) break;
			if (nextchar(';')) ; //advance past a ;
			continue;
		}

		if (calcerror) break;

		skipwscomment();
		if (checkBlock(&answer)) continue;
		//if (scopes.n>1 && (nextchar('}') || nextword("break"))) {
		//	popScope();
		//	continue;
		//}

		answer=evalLevel(0);
		if (calcerror) break;

		if (answer && answer->type()==VALUE_LValue) {
			Value *v=dynamic_cast<LValue*>(answer)->Resolve();
			answer->dec_count();
			answer=v;
		}

		//updatehistory(answer,outexprs);
		num_expr_parsed++;
		atNextCommandStep(); //for future debugging purposes?

		if (nextchar(';')) ; //advance past a ;
	} 

	if (!calcerror && scopes.n!=curscope) {
		calcerr(_("Unterminated scope!"));
		if (answer) { answer->dec_count(); answer=NULL; }
	}

	while (scopes.n!=curscope) scopes.remove(scopes.n-1);

	if (value_ret) { *value_ret=answer; if (answer) answer->inc_count(); }
	if (last_answer) last_answer->dec_count();
	last_answer=answer; //last_answer takes the reference

	return calcerror>0 ? 1 : (errorlog->Total()>numerr && errorlog->Warnings(numerr)>0 ? -1: 0);
}

/*! Called recursively for scripted functions, establish a scope containing "context", and parameters.
 * Store old expression, and call the other evaluate().
 */
int LaidoutCalculator::evaluate(const char *in, int len, ValueHash *context, ValueHash *parameters, Value **value_ret, ErrorLog *log)
{
	 //1. back up state:
	ErrorLog *old_errorlog=errorlog;
	errorlog=log;
	int oldfrom=from;
	int oldlen=curexprslen;
	int oldline=curline;
	char *texprs=newnstr(curexprs,curexprslen);


	 //2. establish parameters and process the call
	if (context || parameters) {
		pushScope(BLOCK_function, 0,0,NULL,NULL,NULL);
		BlockInfo *evalscope=currentLevel();
	
		if (context) {
			evalscope->AddValue("context",context);
		}

		if (parameters && parameters->n()) {
			for (int c=0; c<parameters->n(); c++) {
				if (!parameters->key(c)) continue; //skip unnamed parameters
				evalscope->AddValue(parameters->key(c),parameters->value(c));
			}
		}
	}

	int status=evaluate(in,len, value_ret,log);

	if (context || parameters) popScope();


	 //3. restore state
	makestr(curexprs,texprs);
	curexprslen=oldlen;
	from       =oldfrom;
	curline    =oldline;
	errorlog   =old_errorlog;

	return status;
}

//-------------Error parsing functions

void LaidoutCalculator::ClearError()
{
	calcerror=0;
	if (calcmes) { delete[] calcmes; calcmes=NULL; }
	default_errorlog.Clear();
}

//! If in error state, return the error message, otherwise the outexprs, otherwise "Ok.".
const char *LaidoutCalculator::Message()
{
	if (calcmes) return calcmes;
	//if (outexprs) return outexprs;
	return _("Ok.");
}



//------------------------------ the rest are protected functions. Above are public api ---------

//! Import all non-operators into the current namespace, which is the topmost scope.
/*! Return number of names imported.
 *
 * For operators, use importOperators().
 */
int LaidoutCalculator::importAllNames(CalculatorModule *module)
{
	if (!module) return 0;
	int n=0;

	ObjectDef *def;
	for (int c=0; c<module->getNumFields(); c++) {
		def=module->getField(c);
		if (def->format==VALUE_Operator) continue; //ops are already imported
		importName(module,def);
	}

	return n;
}

/*! Assume def is an ObjectDef in module. Insert name into current scope.
 */
int LaidoutCalculator::importName(CalculatorModule *module, ObjectDef *def)
{
	currentLevel()->AddName(module,def,NULL);
	return 0;
}

/*! Return number of operators added.
 */
int LaidoutCalculator::importOperators(CalculatorModule *module)
{
	int n=0;

	ObjectDef *def;
	int dir,priority;
	const char *rr;
	for (int c=0; c<module->getNumFields(); c++) {
		def=module->getField(c);
		if (def->format!=VALUE_Operator) continue;

		rr=def->range;
		if      (*rr=='l') dir=OPS_Left ;
		else if (*rr=='r') dir=OPS_Right;
		else if (*rr=='>') dir=OPS_LtoR ;
		else if (*rr=='<') dir=OPS_RtoL ;

		priority=strtol(def->defaultvalue,NULL,10);
		addOperator(def->name,dir,priority, module->object_id, def->opevaluator, def);
		n++;
	}

	return n;
}

//! Set calcmes to a 2 line error message, 1 line for error, 1 line to show position of error.
/*! Format will be "error: where"
 *
 * surround is how many characters before and after the error point to show is error message.
 */
void LaidoutCalculator::calcerr(const char *error,const char *where,int w, int surround)
{							  //  where=NULL w=0 
	delete[] calcmes; calcmes=NULL;
	calcerror=from;
	if (from==0) calcerror=1;

	appendstr(calcmes,error);
	if (w) appendstr(calcmes,": ");
	appendstr(calcmes,where);
	appendstr(calcmes,":\n");

	int before=0, pos=from, after=0;

	 //error occured somewhere inside of expression
	int bbb=1, aaa=1; //whether to include "..." before or after
	pos=from;
	if (pos>=curexprslen) pos=curexprslen;
	if (pos<0) pos=0;

	before=pos-surround;
	if (before<0) { before=0; bbb=0; }

	after=pos+surround;
	if (after>=curexprslen) { after=curexprslen; aaa=0; }

	if (before<pos) {
		if (bbb) appendstr(calcmes,"...");
		appendnstr(calcmes,curexprs+before,pos-before);
	}
	appendstr(calcmes,"<*>");
	if (after>pos) {
		appendnstr(calcmes,curexprs+pos,after-pos);
		if (aaa) appendstr(calcmes,"...");
	}

	appendstr(calcmes,"  pos:");
	appendintstr(calcmes,pos);

	if (curline>0) {
		appendstr(calcmes,"  line: ");
		char ll[20];
		sprintf(ll,"%d",curline);
		appendstr(calcmes,ll);
	}


	if (errorlog) errorlog->AddMessage(calcmes,ERROR_Fail, 0, from,curline);
}

//------------- String parse helper functions

//! Get a word starting at from. from is not modified.
/*! A name starts with a letter or an underscore, and 
 * continues until the first character that is not a letter, number,
 * or an underscore.
 *
 * curexprs[from] must be a letter (as returned by isalpha()), or a '_'. Otherwise NULL is returned.
 * The length of the word is put in *n if n!=NULL.
 * The from variable is not advanced.
 *
 * Returns a new char[].
 */
char *LaidoutCalculator::getnamestring(int *n)  //  alphanumeric or _ 
{
	if (!isalpha(curexprs[from]) && curexprs[from]!='_') { *n=0; return NULL; }

	char *tname=NULL;
	int c=0;
	while (from+c<curexprslen && (isalnum(curexprs[from+c]) || curexprs[from+c]=='_')) c++;
	if (c!=0) {
		tname=new char[c+1];
		strncpy(tname,curexprs+from,c);
		tname[c]='\0';
	}
	if (n) *n=c;
	return tname;
}

const char *LaidoutCalculator::getnamestringc(int *n)  //  alphanumeric or _ 
{
	*n=getnamestringlen();
	if (*n>0) return curexprs+from;
	return NULL;
}

//! Return length of a name string, starting at from.
/*! If there is no name beginning at from, then return 0.
 *
 * Does not advance from.
 */
int LaidoutCalculator::getnamestringlen()  //  alphanumeric or _ 
{
	if (!isalpha(curexprs[from]) && curexprs[from]!='_') return 0;

	int c=0;
	while (from+c<curexprslen && (isalnum(curexprs[from+c]) || curexprs[from+c]=='_')) c++;
	return c;
}

/*! Operators are strings of non-whitespace, non-letters, non-numbers, that does not contain ';',
 * or container characters such as "'(){}[] or a period, or '#'.
 *
 * \todo it would be better to allow operators composed of non-strange characters, like "mod" for instance.
 */
const char *LaidoutCalculator::getopstring(int *n)
{
	skipwscomment();
	char ch=curexprs[from];
	int pos=from;
	*n=0;

	 //note: this leaves:  :\|<>?+-/!@$%^&*~`=
	while (pos<curexprslen 
			&& !isspace(ch)
			&& !isalnum(ch)
			&& ch!='#'
			&& ch!=';'
			&& ch!='"'
			&& ch!='\''
			&& ch!='.'
			&& ch!='(' && ch!=')'
			&& ch!='[' && ch!=']'
			&& ch!='{' && ch!='}') {
		(*n)++;
		pos++;
		ch=curexprs[pos];
	}
	if (*n) return curexprs+from;
	return NULL;
}


//! Assuming we are either on whitespace, or the start of a comment, skip whitespace and "#...\n" based comments.
/*! This will not advance past curexprslen, the length of the current expression.
 */
void LaidoutCalculator::skipwscomment()
{
	//-------------- only "#....\n" comments
//	do {
//		while (isspace(curexprs[from]) && from<curexprslen) {
//			if (curexprs[from]=='\n') curline++;
//			from++;
//		}
//		if (curexprs[from]=='#') {
//			while (curexprs[from++]!='\n' && from<curexprslen); 
//		}
//	} while (isspace(curexprs[from]) || curexprs[from]=='#');

	//----------------for //comments  and /* comments */
	do {
		 //skip actual whitespace
		while (isspace(curexprs[from]) && from<curexprslen) {
			if (curexprs[from]=='\n') {
				curline++;
				DBG cerr <<"next line ("<<curline<<")..."<<endl;
			}
			from++;
		}

		if (curexprs[from]=='#') {
			while (curexprs[from++]!='\n' && from<curexprslen); 
			
		} else if (curexprs[from]=='/' && curexprs[from+1]=='/') {
			while (from<curexprslen && curexprs[from++]!='\n'); 

		} else if (curexprs[from]=='/' && curexprs[from+1]=='*') {
			 //skip until "*/" is encountered...
			while ((curexprs[from]!='*' || curexprs[from+1]!='/') && from<curexprslen) {
				if (curexprs[from]=='\n') {
					curline++;
					DBG cerr <<"next line ("<<curline<<")..."<<endl;
				}
				from++;
			}
			if (from<curexprslen && curexprs[from]=='*' && curexprs[from+1]=='/') from+=2;
		}
	} while (from<curexprslen && (
				  isspace(curexprs[from])
			   || curexprs[from]=='#'
			   || (curexprs[from]=='/' && curexprs[from+1]=='*')
			   || (curexprs[from]=='/' && curexprs[from+1]=='/')
			   ));
}

/*! Skip whitespace and skip final comments in the form "#....\n".
 * If curexprs[from]==ch then from is advanced and 1 is returned.
 * Else 0 is returned (from will still be past the whitespace and comments).
 *
 * NOTE: since this skips ANYTHING after a '#' until a newline, you should
 * not use this while parsing strings.
 *
 * \todo currently NOT utf8 ok. So far this is ok, as nextchar() is only used to 
 *   scan for ascii punctuation
 */
int LaidoutCalculator::nextchar(char ch)
{
	if (from>curexprslen) { calcerr(_("Abrupt ending")); return 0; }
	skipwscomment();
	if (curexprs[from]==ch)	{ from++; return 1; }
	return 0;
}

/*! Skip whitespace and comments. Then if the next characters is curexprs
 * are the same as word and the character following the word is not
 * alphanumeric or an underscore, then return 1. from is advanced to the character
 * right after the final character of the word.
 * Otherwise 0 is returned
 * and from is still past the whitespace and comments.
 *
 * word must NOT be NULL.
 */
int LaidoutCalculator::nextword(const char *word)
{
	skipwscomment();

	int len=strlen(word);
	for (int c=0; c<len; c++) if (curexprs[from+c]!=word[c]) return 0;
	if (isalnum(curexprs[from+len]) || curexprs[from+len]=='_') return 0;

	from+=len;
	return 1;
}


//--------------Evaluation Functions


void LaidoutCalculator::pushScope(BlockTypes scopetype, int loop_start, int condition_start, char *var, Value *v, CalculatorModule *module)
{
	BlockInfo *block=new BlockInfo(module, scopetype,loop_start,condition_start,var,v);
	block->parentscope=scopes.e[scopes.n-1];
	scopes.push(block);
}

/*! Returns the type of scope popped.
 */
BlockTypes LaidoutCalculator::popScope()
{
	DBG cerr <<"pop scope!"<<endl;

	if (scopes.n==1) { calcerr(_("Unexpected end!")); return BLOCK_error; }
	BlockInfo *scope=scopes.e[scopes.n-1];

	BlockTypes type=scope->type;

	if (scope->type==BLOCK_if) {
		 // if closing an if block, must skip over subsequent blocks...
		int cont=1;
		while(cont) {
			cont=0;
			skipwscomment();
			int n=0;
			const char *word=getnamestringc(&n);
			if (n!=4 || strncmp(word,"else",4)) break; //done if no else
			from+=4;
			skipwscomment();

			word=getnamestringc(&n);
			if (n==2 && !strncmp(word,"if",2)) {
				 //was else if (...) {...}
				from+=2;
				skipwscomment();
				if (!nextchar('(')) { calcerr(_("Expected '('!")); return BLOCK_error; }
				skipBlock(')');
				cont=1; //still have to skip rest of if chain
			}
			if (!nextchar('{')) { calcerr(_("Expected '{'!")); return BLOCK_error; }
			skipBlock('}');
		}

	} else if (scope->type==BLOCK_namespace || scope->type==BLOCK_function || scope->type==BLOCK_class) {
		 //close out namespace addition
		 //nothing special to do

	} else if (scope->type==BLOCK_for) {
		 //for: evaluate final functions, evaluate condition, jump back to start of block

		int tfrom=from;
		from=scope->start_of_advance;
		while (!calcerror) {
			Value *v=evalLevel(0);
			if (v) v->dec_count();
			if (!nextchar(',')) break;
		}
		if (calcerror) return BLOCK_error;

		int ifso=evalcondition();
		if (calcerror) return BLOCK_error;
		if (ifso) {
			from=scope->start_of_loop;
			scope->current++;
			return BLOCK_none; //not done with scope. looped once!
		} else from=tfrom; //done with scope, ok to remove

	} else if (scope->type==BLOCK_foreach) {
		 //foreach: update name variable, jump back to start
		scope->current++;
		if (scope->current<scope->list->values.n) {
			from=scope->start_of_loop;

			ValueEntry *entry=dynamic_cast<ValueEntry*>(scope->FindName(scope->word,strlen(scope->word)));
			if (entry) entry->SetVariable(scope->word,scope->list->values.e[scope->current],0); //if no entry is overloaded? what to do?

			return BLOCK_none;
		}
		//else all done with loop!

	} else if (scope->type==BLOCK_while) {
		 //while: evaluate condition, jump back to start
		int tfrom=from;
		from=scope->start_of_condition;

		int ifso=evalcondition();
		if (calcerror) return BLOCK_error;
		if (ifso) {
			from=scope->start_of_loop;
			scope->current++;
			return BLOCK_none; //not done with scope. looped once!
		} else from=tfrom; //done with scope, ok to remove

	}

	scopes.remove(scopes.n-1);
	return type;
}


int LaidoutCalculator::evalcondition()
{
	Value *v=evalLevel(0);
	if (v->type()!=VALUE_Int && v->type()!=VALUE_Real && v->type()!=VALUE_Boolean) {
		calcerr(_("Bad condition"));
		return 0;
	}
	int ifso=0;
	if (v->type()==VALUE_Boolean) ifso=dynamic_cast<BooleanValue*>(v)->i;
	else if (v->type()==VALUE_Int) ifso=(dynamic_cast<IntValue*>(v)->i==0?0:1);
	else ifso=(dynamic_cast<DoubleValue*>(v)->d==0?0:1);
	v->dec_count();
	return ifso;
}

/*! \todo when single expression if, this is broken...
 */
void LaidoutCalculator::skipRemainingBlock(char ch)
{
	skipBlock(ch);
}

//! Assuming just after an opening of ch, skip to after a ch.
/*! ch should be one of: )}].
 */
void LaidoutCalculator::skipBlock(char ch)
{
	char *str;
	int tfrom=-1;
	while (from!=tfrom && from<curexprslen) {
		if (tfrom<0) tfrom=from;

		str=strpbrk(curexprs+from, "\"'({[]})#\n"); //first occurence of any of these
		if (!str) calcerr(_("Missing end!"));
		if (*str=='\n') {
			curline++;
			DBG cerr <<"next line ("<<curline<<")..."<<endl;
			from++;
			continue;
		}

		if (*str==ch) { from=str-curexprs+1; return; } //found the end!!
		if (*str==')' || *str=='}' || *str==']') {
			 //if it was a closing like ch, it was dealt with above
			calcerr(_("Unexpected end!"));
			return;
		}

		if (*str=='#') { from=str-curexprs; skipwscomment(); continue; }
		if (*str=='/' && str[1]=='/') { from=str-curexprs; skipwscomment(); continue; }
		if (*str=='/' && str[1]=='*') { from=str-curexprs; skipwscomment(); continue; }
		if (*str=='(') { from=str-curexprs+1; skipBlock(')'); continue; }
		if (*str=='[') { from=str-curexprs+1; skipBlock(']'); continue; }
		if (*str=='{') { from=str-curexprs+1; skipBlock('}'); continue; }

		if (*str=='"' || *str=='\'') {
			from=str-curexprs;
			skipstring();
			continue;
		}
	}
}

//! Skip until a ';' or eof.
void LaidoutCalculator::skipExpression()
{
	char *str;
	int tfrom=-1;
	while (from!=tfrom && from<curexprslen && !calcerror) {
		if (tfrom<0) tfrom=from;

		str=strpbrk(curexprs+from, ";\"'({[]})#"); //first occurence of any of these
		if (!str) { str=curexprs+curexprslen; from=curexprslen; }

		if (*str==';') { from=str-curexprs+1; return; } //found the end!!

		if (*str=='#') { from=str-curexprs; skipwscomment(); continue; }
		if (*str=='/' && str[1]=='/') { from=str-curexprs; skipwscomment(); continue; }
		if (*str=='/' && str[1]=='*') { from=str-curexprs; skipwscomment(); continue; }
		if (*str=='(') { skipBlock(')'); continue; }
		if (*str=='[') { skipBlock(']'); continue; }
		if (*str=='{') { skipBlock('}'); continue; }

		if (*str=='"' || *str=='\'') {
			from=str-curexprs;
			skipstring();
			continue;
		}
	}
}


void LaidoutCalculator::skipstring()
{
	char quote;
	if (nextchar('\'')) quote='\'';
	else if (nextchar('"')) quote='"';
	else return;
	int tfrom=from;

	int spos=0;
	char ch;

	 // read in the string, translating escaped things as we go
	while (from<curexprslen && curexprs[from]!=quote) {
		ch=curexprs[from];
		 // escape sequences
		if (ch=='\\') {
			ch=0;
			if (curexprs[from+1]=='\n') { from+=2; continue; } // escape the newline
			else if (curexprs[from+1]=='\'') ch='\'';
			else if (curexprs[from+1]=='"') ch='"';
			else if (curexprs[from+1]=='n') ch='\n';
			else if (curexprs[from+1]=='t') ch='\t';
			else if (curexprs[from+1]=='\\') ch='\\';
			if (ch) from++; else ch='\\';
		}
		spos++;
		from++;
	}
	if (curexprs[from]!=quote) {
		from=tfrom;
		calcerr(_("String not closed."));
		return;
	}
	from++;
	return;
}

/*! If we were in a function, and "return" is encountered, then we might be returning a value.
 */
int LaidoutCalculator::checkBlock(Value **value_ret)
{
	*value_ret=NULL;
	if (scopes.n>1) {
		if (nextchar('}')) {
			popScope();
			return 1;
		}
	}

	if (!isalpha(curexprs[from])) return 0;

	int n=0;
	const char *word=getnamestringc(&n);
	if (!word) return 0;


	if (n==5 && !strncmp(word,"break",5)) {
		 // must pop all through nearest loop block
		from+=5;
		while (currentLevel()->type==BLOCK_if) {
			popScope();
			skipRemainingBlock('}');
		}
		if (currentLevel()->type!=BLOCK_for && currentLevel()->type!=BLOCK_foreach && currentLevel()->type!=BLOCK_while) {
			calcerr(_("Cannot break from there!"));
			return 1;
		}
		scopes.remove(scopes.n-1);
		skipRemainingBlock('}');
		return 1;

	} else if (n==6 && !strncmp(word,"return",6)) {
		 //must pop all through nearest function block
		from+=6;
		skipwscomment();
		*value_ret=evalLevel(0);
		if (calcerror) return 1;
		int c;
		for (c=scopes.n-1; c>0; c--) if (scopes.e[c]->type==BLOCK_function) break;
		if (c==0) {
			calcerr(_("Cannot return!"));
			return 1;
		}
		while (scopes.n>c+1) popScope();
		from=curexprslen; //we were in a function expression, this jumps us out
		return 1;

	} else if (n==2 && !strncmp(word,"if",2)) {
		 //if (condition) { ... } else if { ... }
		from+=2;
		skipwscomment();
		if (!nextchar('(')) { calcerr(_("Expected '('!")); return 1; }
		int ifso=evalcondition();
		if (calcerror) return 1;
		if (!nextchar(')')) { calcerr(_("Expected ')'!")); return 1; }
		skipwscomment();
		if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }

		if (ifso) { 
			pushScope(BLOCK_if);
			return 1; //when popping, skip any else blocks
		} else {
			 //condition false, so go to else, if any
			skipBlock('}'); //advance to just after a closing '}'
			skipwscomment();
			n=getnamestringlen();
			word=curexprs+from;
			if (n==4 && !strncmp(word,"else",4)) {
				from+=4;
				n=getnamestringlen();
				word=curexprs+from;
				if (!strncmp(word,"if",2)) return checkBlock(value_ret);
				if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }
				pushScope(BLOCK_if);
				return 1; //start over from the else
			}
		} 
		return 1;

	} else if (n==7 && !strncmp(word,"foreach",7)) {
		 //foreach name in set|array|hash { ... }
		from+=7;
		skipwscomment();
		n=getnamestringlen();
		word=curexprs+from;
		if (!n) { calcerr(_("Expected name!")); return 1; }

		char *var=newnstr(word,n);
		from+=n;
		skipwscomment();
		word=getnamestringc(&n);
		if (n==2 && !strncmp(word,"in",2)) from+=2;
		Value *v=evalLevel(0);
		if (calcerror) return 1;
		if (v->type()!=VALUE_Set && v->type()!=VALUE_Array && v->type()!=VALUE_Hash) {
			calcerr(_("Expected set, array, or hash!"));
			return 1;
		}
		if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }
		int start_of_block=from;

		if ((v->type()==VALUE_Set && !dynamic_cast<SetValue*>(v)->values.n)
			|| (v->type()==VALUE_Array && !dynamic_cast<SetValue*>(v)->values.n)
			|| (v->type()==VALUE_Hash && !dynamic_cast<ValueHash*>(v)->n())) {
			 //no actual values, so skip!
			skipBlock('}');
			return 1;
		}

		pushScope(BLOCK_foreach, start_of_block, 0, var, v);
		return 1;


	} else if (n==3 && !strncmp(word,"for",3)) {
		//for (initializations; condition; loop commands) { ... }
		from+=3;
		skipwscomment();
		if (!nextchar('(')) { calcerr(_("Expected '('!")); return 1; }
		pushScope(BLOCK_for, from, 0);
		BlockInfo *scope=scopes.e[scopes.n-1];

		 //do initializations
		while (!calcerror) {
			Value *v=evalLevel(0);
			if (v) v->dec_count();
			if (!nextchar(',')) break;
		}
		if (!nextchar(';')) { calcerr(_("Expected ';'!")); return 1; }
		scope->start_of_condition=from;

		 //initial condition
		int ifso=evalcondition();
		if (calcerror) return 1;
		if (!nextchar(';')) { calcerr(_("Expected ';'!")); return 1; }
		skipwscomment();
		scope->start_of_advance=from;
		skipBlock(')');
		skipwscomment();
		if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }
		if (!ifso) {
			skipBlock('}');
			scopes.remove(scopes.n-1); //force removal of new one
			return 1;
		}
		scope->start_of_loop=from;
		return 1;

	} else if (n==5 && !strncmp(word,"while",5)) {
		 //while (condition) { ... }
		from+=5;
		skipwscomment();
		if (!nextchar('(')) { calcerr(_("Expected '('!")); return 1; }
		int start_of_condition=from;

		int ifso=evalcondition();
		if (calcerror) return 1;
		if (!nextchar(')')) { calcerr(_("Expected ')'!")); return 1; }
		skipwscomment();
		if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }
		if (ifso) pushScope(BLOCK_while, from, start_of_condition);
		else skipBlock('}');
		return 1;

	} else if (n==9 && !strncmp(word,"namespace",9)) {
		 //namespace name { ... } <-- create and install a new submodule in current module
		 //namespace { ... }  <-- create a temporary scope, identical to namespace, but delete when out of scope
		 //  if name already exists in current scope, then add to it
		from+=9;
		skipwscomment();
		word=NULL;
		if (isalpha(curexprs[from])) {
			word=getnamestringc(&n);
			from+=n;
			skipwscomment();
		}
		if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }

		ObjectDef *def=scopes.e[scopes.n-1]->scope_namespace->FindDef(word,n);
		if (def && def->format==VALUE_Namespace) {
			 //namespace already exists
			 def->inc_count();
		} else if (def) {
			 //word exists, but is not namespace!
			calcerr(_("Not a namespace"));
			return 1;
		} else {
			 //create new namespace
			char *str=newnstr(word,n);
			def=new ObjectDef(NULL, str,str,NULL,"namespace",NULL,NULL);
			// *** new namespaces have same mod_id of parent namespace
			//note that namespace is installed in current scope which may be a temporary one!
		}

		skipwscomment();
		pushScope(BLOCK_namespace, 0,0,NULL,NULL,def);
		def->dec_count();
		return 1;
	}

	return 0;
}


//! Replace curexprs with newex, and set from=0.
void LaidoutCalculator::newcurexprs(const char *newex,int len)
{
	DBG cerr <<"new curexprs: "<<newex<<endl;

	makenstr(curexprs,newex,len);
	curexprslen=strlen(curexprs);
	from=0;
	curline=0;
}

void LaidoutCalculator::showDef(char *&temp, ObjectDef *sd)
{
	//*** maybe do this: it's easier
	//Attribute att
	//sd->dump_out_atts(&att, DEFOUT_HumanSummary,NULL);
	//appendstr(temp,att.value);

	appendstr(temp,element_TypeNames(sd->format));
	appendstr(temp," ");
	appendstr(temp,sd->name);
	//appendstr(temp,": ");
	//appendstr(temp,sd->Name);
	if (sd->description) {
		appendstr(temp,",   ");
		appendstr(temp,sd->description);
	}

	if ((sd->format==VALUE_Class || sd->format==VALUE_Fields) && sd->extendsdefs.n) {
		appendstr(temp,", extends: ");
		for (int c2=0; c2<sd->extendsdefs.n; c2++) {
			appendstr(temp,sd->extendsdefs.e[c2]->name);
		}
	}

	if ((sd->format==VALUE_Namespace || sd->format==VALUE_Class || sd->format==VALUE_Fields || sd->format==VALUE_Function)
			&& sd->fields && sd->getNumFields()) {
		const char *nm,*Nm,*desc,*rng,*fmts;
		ValueTypes fmt;
		ObjectDef *subdef=NULL;
		appendstr(temp,"\n");
		for (int c2=0; c2<sd->getNumFields(); c2++) {
			sd->getInfo(c2,&nm,&Nm,&desc,&rng,NULL,&fmt,NULL,&subdef);
			appendstr(temp,"  ");

			if (fmt==VALUE_Function) {
				//appendstr(temp,"function ");
				appendstr(temp,nm);
				appendstr(temp," (");
				if (subdef && subdef->fields) for (int c3=0; c3<subdef->fields->n; c3++) {
					subdef->getInfo(c3,&nm,NULL,NULL,NULL,NULL,&fmt);
					if (fmt!=VALUE_Any) { appendstr(temp,element_TypeNames(fmt)); appendstr(temp," "); }
					appendstr(temp,nm);
					if (c3!=subdef->fields->n-1) appendstr(temp,", ");
				}
				appendstr(temp,")");
				if (desc) {
					appendstr(temp,", ");
					appendstr(temp,desc);
				}
				appendstr(temp,"\n");

			} else if (fmt==VALUE_Operator) {
				appendstr(temp,nm);
				appendstr(temp," (operator) (");
				if (subdef && subdef->fields) for (int c3=0; c3<subdef->fields->n; c3++) {
					subdef->getInfo(c3,&nm,NULL,NULL,NULL,NULL,&fmt,&fmts);
					if (fmt!=VALUE_Any) {
						//appendstr(temp,element_TypeNames(fmt));
						appendstr(temp,fmts);
						appendstr(temp," ");
					}
					appendstr(temp,nm);
					if (c3!=subdef->fields->n-1) appendstr(temp,", ");
				}
				appendstr(temp,")");
				if (desc) {
					appendstr(temp,", ");
					appendstr(temp,desc);
				}
				appendstr(temp,"\n");


			} else {
				 //namespace or class
				appendstr(temp,nm);
				appendstr(temp,": (");
				appendstr(temp,element_TypeNames(fmt));
				if (fmt==VALUE_Set) {
					if (!isblank(rng)) {
						appendstr(temp," of ");
						appendstr(temp,rng);
					}
				}
				appendstr(temp,")   ");
				appendstr(temp,Nm);
				if (desc) {
					appendstr(temp,", ");
					appendstr(temp,desc);
				}
				appendstr(temp,"\n");
				if (fmt==VALUE_Enum && subdef && subdef->fields) {
					appendstr(temp,"    ");
					appendstr(temp,_("Possible values:\n"));
					for (int c3=0; c3<subdef->fields->n; c3++) {
						subdef->getInfo(c3,&nm,&Nm,&desc);
						appendstr(temp,"      ");
						appendstr(temp,nm);
						appendstr(temp,": ");
						appendstr(temp,Nm);
						appendstr(temp,",   ");
						appendstr(temp,desc);
						appendstr(temp,"\n");
					}
				}
			}
		}
	} //if fields or function
}

//int LaidoutCalculator::scanInFunction()
//{
//}

ObjectDef *LaidoutCalculator::GetSessionCommandObjectDef()
{
	if (sessiondef.fields) return &sessiondef;

	sessiondef.SetType("namespace");
	makestr(sessiondef.name,"sessioncommands");
	makestr(sessiondef.Name,_("Session Commands"));

	sessiondef.pushFunction("show", _("Show"), _("Give information about something"), NULL, NULL);
	sessiondef.pushFunction("about",_("About"),_("Show version information"), NULL, NULL);
	sessiondef.pushFunction("unset",_("Unset"),_("Remove a name from the current namespace"), NULL, NULL);
	sessiondef.pushFunction("print",_("Print"),_("Print out something to the console"), NULL, NULL);

	sessiondef.pushFunction("typeof", _("typeof"), _("Return the final type of an object"), NULL, NULL);
	sessiondef.pushFunction("typesof",_("typesof"),_("Return a set of all the types of an object"), NULL, NULL);

	sessiondef.pushFunction("degrees",_("Degrees"),_("Numbers for angle inputs are assumed to be degrees"), NULL, NULL);
	sessiondef.pushFunction("radians",_("Radians"),_("Numbers for angle inputs are assumed to be radians"), NULL, NULL);
	sessiondef.pushFunction("help", _("Help"), _("Show a quick help"), NULL, NULL);
	sessiondef.pushFunction("?",    _("Help"), _("Show a quick help"), NULL, NULL);
	sessiondef.pushFunction("quit", _("Quit"), _("Quit"), NULL, NULL);

	return &sessiondef;
}

//! Parse any commands that change internal calculator settings like radians versus decimal.
/*! Set decimal==1 if 'degrees', set to 0 if 'radians'
 *
 * Return 
 */
int LaidoutCalculator::sessioncommand() //  done before eval
{
	if (nextword("?") || nextword("help")) {
		messageOut(_("The very basic commands are:"));

		ObjectDef *def=GetSessionCommandObjectDef();
		const char *nm,*desc;
		char buffer[200];
		for (int c=0; c<def->getNumFields(); c++) {
			def->getInfo(c,&nm,NULL,&desc,NULL,NULL,NULL,NULL,NULL);
			sprintf(buffer,"%10s  %s",nm,desc);
			messageOut(buffer);
		}

		return 1;
	}

	if (nextword("about")) {
		messageOut(LaidoutVersion());
		return 1;
	}

	if (nextword("quit")) {
		laidout->quit();
		return 1;
	}

	if (nextword("print")) {
		Value *value=evalLevel(0);
		if (calcerror || !value) return 1;
		char *buf=NULL;
		int n=0;

		if (value->type()==VALUE_String) {
			makestr(buf,dynamic_cast<StringValue*>(value)->str);
		} else {
			value->getValueStr(&buf,&n,1);
			value->dec_count();

			 //need to unescape the string
			int p=0;
			char ch;
			for (int c=0; c<n; c++) {
				ch=0;
				if (buf[c]=='\'') {
					if (buf[c+1]=='\'') ch='\'';
					else if (buf[c+1]=='"') ch='"';
					else if (buf[c+1]=='n') ch='\n';
					else if (buf[c+1]=='t') ch='\t';
					else if (buf[c+1]=='\\') ch='\\';
					if (ch) c++; else ch='\\';
				}
				if (ch==0) ch=buf[c];
				buf[p]=ch;
				p++;
			}
			buf[p]='\0';
		}

		messageOut(buf);
		delete[] buf;
		return 1;
	}

	
	if (nextword("radians"))
	    { calcsettings.decimal=0; from+=7; return 1; }
	
	if (nextword("degrees"))
	    { calcsettings.decimal=1; from+=7; return 1; }

	if (nextword("import")) {
		 //import an available module into the current scope
		skipwscomment();
		int n=0;
		char *name=getnamestring(&n);
		if (n==0) { calcerr(_("Expecting name!")); return 1; }
		from+=n;
		skipwscomment();
		ImportModule(name,0);
		delete[] name;
		return 1;
	}

	if (nextword("using")) {
		 //take names from another namespace and make them immediately accessible in the current scope
		 //use unset to remove imported names from current scope
		skipwscomment();
		int n=0;
		char *name=getnamestring(&n);
		if (n==0) { calcerr(_("Expecting name!")); return 1; }
		from+=n;
		skipwscomment();
		ImportModule(name,1);
		delete[] name;
		return 1;
	}

	 //these things define ObjectDef objects, and have special meta read in modes.
	 //It might be nice to be able to add meta to any object

	//if (nextword("alias")) {
	//	 ***

	if (nextword("var")) {
		 //add a variable to the current scope's namespace
		 // *** todo: Name and description
		ObjectDef *def=currentLevel()->scope_namespace;
		if (def->format!=VALUE_Namespace && def->format!=VALUE_Class) { calcerr(_("Cannot add variables to current scope!")); return 1; }

		skipwscomment();
		int n=0;
		char *type=getnamestring(&n);
		if (!type) { calcerr(_("Expected type!")); return 1; }
		from+=n;

		skipwscomment();
		char *name=getnamestring(&n);
		if (!name) { name=type; type=NULL; }
		from+=n;

		Value *value=NULL;
		if (nextchar('=')) {
			value=evalLevel(0);
			if (calcerror) {
				delete[] name;
				if (type) delete[] type;
				return 1;
			}
		}

		def->pushVariable(name,name,NULL, type,0, value,1);
		currentLevel()->AddName(def,def->fields->e[def->fields->n-1],NULL);

		delete[] name;
		if (type) delete[] type;

		return 1;
	}

	if (nextword("function")) {
		 //function funcname : "description" (type parameter_name=default_value : "description", ...) { ...code... }

		skipwscomment();
		ObjectDef *def;
		Value *v;
		int nn;
		int n=0;
		char *name=getnamestring(&n);
		char *type;
		if (!n) { calcerr(_("Expected name!")); return 1; }

		def=new ObjectDef(NULL, name,name,NULL, "function",NULL,NULL);
		delete[] name; name=NULL;

		from+=n;

		 //scan in parameters
		if (nextchar('(')) {
			do {
				 //option type
				skipwscomment();
				type=getnamestring(&nn);
				if (!type) break;
				from+=nn;

				 //get name, if none, then type was the name
				skipwscomment();
				name=getnamestring(&nn);
				if (!name) { name=type; type=NULL; }
				from+=nn;
				
				if (nextchar('=')) {
					v=evalLevel(0);
					if (calcerror) {
						if (type) delete[] type;
						delete[] name;
						delete def;
						return 1;
					}
				} else v=NULL;

				def->pushParameter(name,name,NULL, type,NULL,NULL, v);
				if (type) { delete[] type; type=NULL; }
				delete[] name; name=NULL;
				if (v) v->dec_count();				
			} while (nextchar(','));

			if (!nextchar(')')) {
				calcerr(_("Expected ')'!"));
				delete def;
				return 1;
			}
		} //if nextchar('(')

		 //scan in code
		if (nextchar('{')) {
			int tfrom=from;
			skipBlock('}');
			if (calcerror) {
				delete def;
				return 1;
			}
			char *code=newnstr(curexprs+tfrom, from-tfrom-1);
			def->defaultvalue=code;

		} else if (nextchar('=')) {
			int tfrom=from;
			skipExpression();
			char *code=newnstr(curexprs+tfrom, from-tfrom);
			def->defaultvalue=code;

		} else {
			skipwscomment();
			if (curexprs[from]!='\0' && curexprs[from]!=';') {
				calcerr(_("Badly formed function"));
				delete def;
				return 1;
			}
			//we encountered no defined code
		}

		 //add to current scope
		currentLevel()->scope_namespace->push(def,1);
		currentLevel()->AddName(currentLevel()->scope_namespace, def, NULL);

		return 1;
	} //"function"

	if (nextword("operator")) {
		 //operator * (x,y) { x*y }
		 //operator right ++ (o) { o=o+1; }
		 //operator x * y { x^2+y }
		 //operator o++ { o=o+1; }

		skipwscomment();
		ObjectDef *def;
		int nn;
		int n=0;
		int optype=OPS_LtoR;
		int priority=400;

		if (nextword("ltor")) optype=OPS_LtoR;
		else if (nextword("rtol")) optype=OPS_RtoL;
		else if (nextword("left")) optype=OPS_Left;
		else if (nextword("right")) optype=OPS_Right;

		skipwscomment();
		if (isdigit(curexprs[from])) priority=intnumber();

		const char *op=getopstring(&n);
		if (!n) { calcerr(_("Expected operator!")); return 1; }

		char *oop=newnstr(op,n);
		def=new ObjectDef(NULL, oop,oop,NULL, "operator",NULL,NULL);
		delete[] oop;
		from+=n;

		 //scan in parameters
		int nump=0;
		if (nextchar('(')) {
			 //read in 2 parameter names
			char *type;
			char *name;
			int n2;
			do {
				skipwscomment();
				type=getnamestring(&nn);
				if (!type) break;
				from+=nn;

				skipwscomment();
				name=getnamestring(&n2);
				if (!n2) { name=type; type=NULL; }
				else from+=n2;

				if (type) def->pushParameter(name,name,NULL, type, NULL,NULL, NULL);
				else def->pushParameter(name,name,NULL, "any", NULL,NULL, NULL);
				delete[] name; name=NULL;
				if (type) { delete[] type; type=NULL; }
				nump++;
			} while (nextchar(',') && nump<3);

			if (((optype==OPS_LtoR || optype==OPS_RtoL) && nump>2) 
				 || ((optype==OPS_Right || optype==OPS_Left) && nump>1)) {
				calcerr(_("Too many parameters for operator!"));
				delete def;
				return 1;
			}

			if (!nextchar(')')) {
				calcerr(_("Expected ')'!"));
				delete def;
				return 1;
			}
		} 
		
		if (nump==0) {
			 //there was no '(' or no parameters
			calcerr(_("Missing parameters!"));
			delete def;
			return 1;
		}

		 //scan in code
		if (nextchar('{')) {
			int tfrom=from;
			skipBlock('}');
			if (calcerror) {
				delete def;
				return 1;
			}
			char *code=newnstr(curexprs+tfrom, from-tfrom-1);
			def->defaultvalue=code;

		} else {
			 //missing {...}
			skipwscomment();
			if (curexprs[from]!='\0' && curexprs[from]!=';') {
				calcerr(_("Badly formed operator code"));
				delete def;
				return 1;
			}
			//we encountered no defined code
		}

		 //add to current scope
		currentLevel()->scope_namespace->push(def,1);
		addOperator(def->name,optype,priority,def->object_id,NULL,def);
		//currentLevel()->AddName(currentLevel()->scope_namespace, def);

		return 1;
	} //"operator"

	if (nextword("class")) {
		 //class name extends Class1,Class2 : "doc string"
		 //{ var one=1;
		 //  var two="two";
		 //  var three; //initialized as null object
		 //  random_code(12); //run once only when doing initial parse of class def
		 //  function f(x,y)=x+y;
		 //  operator * (num1,num2) { num3.x=num1.x+num2.x; num3.y=num1.y+num2.y; return num3; }
		 //}
		skipwscomment();
		int n=0;
		char *classname=getnamestring(&n);
		if (!n) {
			calcerr(_("Expected class name!"));
			return 1;
		}
		from+=n;
		ObjectDef *def=new ObjectDef(NULL, classname,classname,NULL,"class",NULL,NULL);
		delete[] classname;
		ObjectDef *edef;

		if (nextword("extends")) {
			skipwscomment();
			do {
				edef=getClass();
				if (!edef) {
					calcerr(_("Could not find class"));
					def->dec_count();
					return 1;
				}
				def->Extend(edef);
			} while (nextchar(','));
		}

		if (!nextchar('{')) {
			calcerr(_("Expected class definition!"));
			def->dec_count();
			return 1;
		}
		currentLevel()->scope_namespace->push(def,1);
		currentLevel()->AddName(currentLevel()->scope_namespace, def, NULL);
		pushScope(BLOCK_class, 0, 0, NULL, NULL, def);

		DBG cerr<<"start class definition:"<<endl;
		DBG def->dump_out(stdout,2,0,NULL);
		return 1;
	}

//	if (nextword("namespace")) {
//		//this is checked in checkBlock()
//		return 1;
//	}

	if (nextword("unset")) {
//		***
//		skipwscomment();
//		int namelen=getnamestringlen();
//		if (!namelen) {
//    		calcerr(_("Expecting name!"));
//			return 1;
//		}
//		int assign=uservars.findIndex(curexprs+from, namelen);
//		if (assign<0) {
//    		calcerr(_("Unknown name!"));
//			return 1;
//		}
//		uservars.remove(assign);
//		from+=namelen;
		return 1;
	}

	if (nextword("show")) {
		 //need scope dump
		 //need operator dump
		 //need modules dump


		 //find specific name to show if any
		 //***** this must adapt to
		 //  show Laidout.project
		 //  show Laidout.docs.3
		skipwscomment();
		//while (ch<curexprslen && !isspace(curexprs[ch]) && curexprs[ch]!=';' && curexprs[ch]!='#') ch++;
		int n=0;
		const char *showwhat=getnamestringc(&n);
		char *temp=NULL;

		ObjectDef *sd=NULL;
		if (showwhat) { //show a particular item
			int scope,module,index;
			Entry *entry=findNameEntry(showwhat,n, &scope, &module, &index);
			if (entry) {
				//Overloaded, function, value, namespace, alias, class, operator
				sd=entry->GetDef();
				from+=n;
				skipwscomment();
			}

			while (sd && nextchar('.')) { //for "show Math.sin", for instance
				showwhat=getnamestringc(&n);
				ObjectDef *ssd=sd->FindDef(showwhat,n);
				if (!ssd) break;
				from+=n;
				sd=ssd;
			} //if foreach dereference

			if (sd) showDef(temp, sd);
			else {
				 //check for match of module names, just in case
				for (int c=0; c<modules.n; c++) {
					sd=modules.e[c];
					if (!strcmp(modules.e[c]->name, showwhat)) {
						appendstr(temp,"module ");
						appendstr(temp,showwhat);
						appendstr(temp,"\n");
						showDef(temp,modules.e[c]);
					}
				}
			}

			if (!sd) { appendline(temp,_("Unknown name!")); }

			showwhat=NULL;

		} else { //show all
			 
			// *** all is:
			//   laidout project and laidout preferences
			//   operators
			//   modules
			//   scopes
			//   scope namespaces
			//     namespaces
			//     operators
			//     classes
			//     variables
			//     functions

			//Attribute att;
			//global_scope->scope_namespace->dump_out_atts(&att,2,NULL);
			//if (!isblank(att->value)) {
			//	messageOut(att->value);
			//} else messageOut(_("Nothing to see here. Move along."));
			//--------------

			 //vvvvvv *** this will be removed once module installation working....
			 // Show laidout project and documents
			temp=newstr(_("Project: "));
			if (laidout->project->name) appendstr(temp,laidout->project->name);
			else appendstr(temp,_("(untitled)"));
			appendstr(temp,"\n");

			if (laidout->project->filename) {
				appendstr(temp,laidout->project->filename);
				appendstr(temp,"\n");
			}

			if (laidout->project->docs.n) appendstr(temp," documents\n");
			char temp2[15];
			for (int c=0; c<laidout->project->docs.n; c++) {
				sprintf(temp2,"  %d. ",c+1);
				appendstr(temp,temp2);
				if (laidout->project->docs.e[c]->doc) //***maybe need project->DocName(int i)
					appendstr(temp,laidout->project->docs.e[c]->doc->Name(1));
				else appendstr(temp,_("unknown"));
				appendstr(temp,"\n");
			}
			appendstr(temp,"\n");
			 //^^^^^^ *** this will be removed once module installation working....
		

			 //module dump
			if (modules.n) {
				appendstr(temp,"Available modules:\n");
				for (int c=0; c<modules.n; c++) {
					appendstr(temp,"  ");
					appendstr(temp,modules.e[c]->name);
					appendstr(temp,"\n");
				}
			}
			appendstr(temp,"\n");


			 //operator dump
			// ***
			appendstr(temp,"Operators:\n");
			appendstr(temp," Levels:\n");
			char buffer[20];
			for (int c=0; c<oplevels.n; c++) {
				OperatorLevel *l=oplevels.e[c];
				appendstr(temp,"  priority:");
				sprintf(buffer,"%d",l->priority);
				appendstr(temp,buffer);
				appendstr(temp,", ");
				if (l->direction==OPS_LtoR) appendstr(temp,"left to right:  ");
				else if (l->direction==OPS_RtoL) appendstr(temp,"right to left:  ");
				for (int c2=0; c2<l->ops.n; c2++) {
					appendstr(temp,l->ops.e[c2]->op);
					appendstr(temp,"  ");
				}
				appendstr(temp,"\n");
			}
			appendstr(temp," Left:\n");
			for (int c=0; c<leftops.ops.n; c++) {
				appendstr(temp,"  ");
				appendstr(temp,leftops.ops.e[c]->op);
				if (leftops.ops.e[c]->def) {
					appendstr(temp,"(val)");
					//appendstr(temp,element_TypeNames(leftops.ops.e[c]->def->format));
					//appendstr(temp,")");
				} else appendstr(temp,"(any)");
				appendstr(temp,"\n");
			}
			appendstr(temp," Right:\n");
			for (int c=0; c<rightops.ops.n; c++) {
				appendstr(temp,"  ");
				appendstr(temp,rightops.ops.e[c]->op);
				if (rightops.ops.e[c]->def) {
					appendstr(temp,"(val)");
					//***need to have op parameter hints for: appendstr(temp,element_TypeNames(rightops.ops.e[c]->def->format));
					//appendstr(temp,")");
				} else appendstr(temp,"(any)");
				appendstr(temp,"\n");
			}
			appendstr(temp,"\n");


			 //scope dump
			appendstr(temp, "Scopes:\n");
			for (int c=0; c<scopes.n; c++) {
				sprintf(temp2,"%d. ",c);
				appendstr(temp,temp2);
				for (int c2=0; c2<c; c2++) appendstr(temp,"  ");
				if (c==0) appendstr(temp,"Global");
				else appendstr(temp, scopes.e[c]->BlockType());
				appendstr(temp,"\n");
			}

			for (int c=0; c<scopes.n; c++) {
				appendstr(temp,"\n");
				sprintf(temp2,"%d",c);
				if (c==0) appendstr(temp,"Global scope detail:\n");
				else {
					appendstr(temp,temp2);
					appendstr(temp,". scope detail in ");
					appendstr(temp, scopes.e[c]->BlockType());
					appendstr(temp,":\n");
				}

				 //entry dump
				for (int c2=0; c2<scopes.e[c]->dict.n; c2++) {
					appendstr(temp,"  ");
					appendstr(temp,element_TypeNames(scopes.e[c]->dict.e[c2]->type()));
					appendstr(temp," ");
					appendstr(temp,scopes.e[c]->dict.e[c2]->name);
					appendstr(temp,"\n");
				}


				ObjectDef *def=scopes.e[c]->scope_namespace;

				 //Show object definitions in scope
				if (def->getNumFields()) {
					appendstr(temp,_("\nObject Definitions:\n"));

					ObjectDef *ddef=NULL;
					for (int c=0; c<def->getNumFields(); c++) {
						def->findActualDef(c,&ddef);
						if (!ddef || (ddef->format!=VALUE_Class && ddef->format!=VALUE_Fields)) continue;

						appendstr(temp,"  ");
						appendstr(temp,ddef->name);
						appendstr(temp,"\n");
					}
				}


				 //Show function definitions in scope
				if (def->getNumFields()) {
					appendstr(temp,_("\nFunction Definitions:\n"));

					ObjectDef *deff=NULL;
					const char *nm=NULL;
					for (int c=0; c<def->getNumFields(); c++) {
						def->findActualDef(c,&deff);
						if (!deff || (deff->format!=VALUE_Function)) continue;

						appendstr(temp,"  ");
						appendstr(temp,deff->name);
						appendstr(temp,"(");
						for (int c2=0; c2<deff->getNumFields(); c2++) {
							deff->getInfo(c2,&nm,NULL,NULL);
							appendstr(temp,nm);
							if (c2!=deff->getNumFields()-1) appendstr(temp,",");
						}
						appendstr(temp,")\n");
					}
				}


				 //Show variables in scope
				if (def->getNumFields()) {
					appendstr(temp,_("\nVariables:\n"));

					ObjectDef *deff=NULL;
					char *buffer=NULL;
					int blen=0;
					for (int c=0; c<def->getNumFields(); c++) {
						def->findActualDef(c,&deff);
						if (!deff || (deff->format!=VALUE_Variable)) continue;

						appendstr(temp,"  ");
						appendstr(temp,deff->name);
						appendstr(temp," = ");
						if (deff->defaultvalue) appendstr(temp,deff->defaultvalue);
						else if (deff->defaultValue) {
							deff->defaultValue->getValueStr(&buffer,&blen,1);
							appendstr(temp,buffer);
						}
						appendstr(temp,"\n");
					}
				}
			} //foreach scope
		} //show all

		if (temp) {
			messageOut(temp);
			delete[] temp;

		} else messageOut(_("Nothing to see here. Move along."));

		return 1; //from "show"
	} //if show


	return 0;
}

//! Establish the new exprs, and valuate a single expression.
/*! This will typically be called by a function in the middle of parsing another expression.
 */
Value *LaidoutCalculator::eval(const char *exprs)
{
	if (!exprs) return(NULL);			 
	char *texprs;
	int tfrom=from;
	Value *num=NULL;
	texprs=new char[strlen(curexprs)+1];
	strcpy(texprs,curexprs);

	newcurexprs(exprs,-1);
	//num=eval();
	num=evalLevel(0);

	newcurexprs(texprs,tfrom);
	delete[] texprs;
	return num;
}

// *** Value *LaidoutCalculator::eval()
//{ return evalLevel(0); }

Value *LaidoutCalculator::evalLevel(int level)
{
	int n=0;
	const char *op;
	OperatorFunction *opfunc=NULL;
	Value *num=NULL, *num2=NULL, *num_ret=NULL;
	int index=-1;
	if (level>=oplevels.n) {
		 //all done with 2 number ops, now check left ops, read number, right ops
		op=getopstring(&n);
		if (n) { //found an op string
			opfunc=leftops.hasOp(op,n,OPS_Left, &index,-1);
			if (!opfunc) {
				 //if left hand op.. if not, is error
				calcerr(_("Unexpected characters!"));
				return NULL;
			}

			from+=n;
			num=evalLevel(level); //will plow through any other left hand ops
			if (num->type()==VALUE_LValue) {
				if (opfunc->def && (opfunc->def->flags&OPS_Assignment)) {
					//keep as lvalue
				} else {
					 //op is not assignment related, so remove any lvalue status
					num2=dynamic_cast<LValue*>(num)->Resolve();
				}
			}

			 //still need to apply the op
			int status=opfunc->function->Op(op,n,OPS_Left, (num2?num2:num),NULL, &calcsettings, &num_ret, NULL);
			if (status==-1) num_ret=opCall(op,n,OPS_Left, (num2?num2:num),NULL, &leftops, index); //need to try overloaded
			if (calcerror) { if (num) num->dec_count(); if (num2) num2->dec_count(); return NULL; }

			if (num2) num2->dec_count();
			if (num_ret) {
				num->dec_count();
				num=num_ret;
			}

		} else {
			 //did not find an op string, so we retrieve a plain number
			num=number();
		}
		if (!num) return NULL; //did not find a number
		 
		 //check for right hand ops
		op=getopstring(&n);
		while (n) {
			 //found an op string
			opfunc=rightops.hasOp(op,n,OPS_Right, &index,-1);
			if (opfunc) {

				if (num->type()==VALUE_LValue) {
					if (opfunc->def && (opfunc->def->flags&OPS_Assignment)) {
						//keep as lvalue
					} else {
						 //op is not assignment related, so remove any lvalue status
						num2=dynamic_cast<LValue*>(num)->Resolve();
					}
				}

				Value *num_ret=NULL;
				int status=opfunc->function->Op(op,n,OPS_Right, (num2?num2:num),NULL, &calcsettings, &num_ret, NULL);
				if (status==-1) num_ret=opCall(op,n,OPS_Right, (num2?num2:num),NULL, &rightops, index); //need to try overloaded
				if (calcerror) { if (num) num->dec_count(); if (num2) num2->dec_count(); return NULL; }
				if (num2) num2->dec_count();

				if (num_ret) {
					num->dec_count();
					num=num_ret;
					break;
				}

				from+=n;
				op=getopstring(&n);
			} else {
				return num; //no right ops, so done!
			}
		}
		return num;
	}

	//left and right ops out of the way, now for 2 number ops
	int dir=oplevels.e[level]->direction;
	int uselvalue;
	int status;
	Value *num1v=NULL, *num2v=NULL;

	num=evalLevel(level+1);
	if (calcerror) return NULL;

	op=getopstring(&n);
	while (n) { 
		opfunc=oplevels.e[level]->hasOp(op,n,dir, &index,-1);
		if (!opfunc) {
			n--; //search for smaller ops
			break;
		}

		from+=n;
		if (dir==OPS_LtoR) num2=evalLevel(level+1);
		else num2=evalLevel(level);
		if (calcerror) { num->dec_count(); return NULL; }

		if (!num2) {
			calcerr(_("Expected number!"));
			num->dec_count();
			return NULL;
		}

		uselvalue=(opfunc->def && (opfunc->def->flags&OPS_Assignment));
		if (uselvalue) {
			 //operator is potentially an assignment
			status=opfunc->function->Op(op,n,dir, num,  num2,  &calcsettings, &num_ret, NULL);
		} else {
			 //need to resolve any LValue states, operator is NOT as assignment
			if (!num1v && num->type()==VALUE_LValue)  num1v=dynamic_cast<LValue*>(num )->Resolve(); else { num1v=num;  num1v->inc_count(); }
			if (!num2v && num2->type()==VALUE_LValue) num2v=dynamic_cast<LValue*>(num2)->Resolve(); else { num2v=num2; num2v->inc_count(); }
			status=opfunc->function->Op(op,n,dir, num1v,num2v, &calcsettings, &num_ret, NULL);
		}

		if (status==-1) {
			 //overloading...
			num_ret=opCall(op,n,dir, num,num2, oplevels.e[level],index);
		}
		if (!calcerror && !num_ret) calcerr(_("Cannot compute with given values."));
		num->dec_count();  num=NULL;
		num2->dec_count(); num2=NULL;
		if (calcerror) {
			if (num1v) num1v->dec_count();
			if (num2v) num2v->dec_count();
			return NULL;
		}
		num=num_ret;
		
		op=getopstring(&n);
	}

	if (num1v) num1v->dec_count();
	if (num2v) num2v->dec_count();
	return num;
}

//! Read in a simple number, no processing of operators.
/*! If the number is an integer or real, then read in units also.
 */
Value *LaidoutCalculator::number()
{
	Value *snum=NULL;

	if (nextchar('(') || nextchar('{')) {
		 // handle stuff in parenthesis: (2-3-5)-2342
		from--;
		snum=getset();
		if (calcerror) return NULL;

	} else if (nextchar('[')) {
		from--;
		snum=getarray();
		if (calcerror) return NULL;

	} else if (curexprs[from]=='\'' || curexprs[from]=='"') {
		 //read in strings
		snum=getstring();
		if (!snum) return NULL;

	} else if (isdigit(curexprs[from]) || curexprs[from]=='.') {
		 //read in an integer or a real
		int tfrom=from;
		if (isdigit(curexprs[from])) {
			snum=new IntValue(intnumber());
		}
		if (curexprs[from]=='.' || curexprs[from]=='e') {
			 // was real number
			from=tfrom; 
			if (snum) delete snum;
			snum=new DoubleValue(realnumber());
		}
		int units=getunits();
		if (units) {
			if (dynamic_cast<IntValue*>(snum)) dynamic_cast<IntValue*>(snum)->units=units;
			else if (dynamic_cast<DoubleValue*>(snum)) dynamic_cast<DoubleValue*>(snum)->units=units;
		}

	} else if (isalpha(curexprs[from]) || curexprs[from]=='_') {
		 //  is not a simple number, maybe function, objectdef, cast, variable, namespace
		snum=evalname();

		if (calcerror) { if (snum) delete snum; return NULL; }
		if (!snum) return NULL; 
	}

	while (nextchar('.') || curexprs[from]=='[') {
		 //there is dereferencing to be done
		Value *snum2=dereference(snum);
		if (calcerror) {
			if (snum) snum->dec_count();
			return NULL;
		}

		snum->dec_count();
		snum=snum2;
	}

//	 // get units;
//	Unit *units=getUnits();
//	if (unitstr) {
//		Unit *units=unitmanager->MakeUnit(unitstr);
//		
//		if (snum->ApplyUnits(units)!=0) *** cannot apply units! already exist maybe;
//	}

	return snum;
}

//! Perform one round of dereferencing. So "Foo.Bar.La" will parse "Foo", leaving from pointing to ".Bar.La".
/*! Assume that we are just past initial '.' for dereferencing. We either have a Value
 * that we need to zero in on, or we have a namespace that we need to access part of.
 *
 * The result can be a namespace, class name, function, variable, or component of an object.
 *
 * blah[5] is the same as blah.5 or blah.(3+2)
 *
 * Returns a new Value.
 */
Value *LaidoutCalculator::dereference(Value *val)
{
	DBG if (val->GetObjectDef()) cerr <<" --- trying to deref "<<val->GetObjectDef()->name<<endl;
	const char *name=NULL;
	int index=-1;
	int n=0;

	skipwscomment();
	name=getnamestringc(&n);
	Value *value=NULL;
	Value *usethis=NULL;
	ObjectDef *def=NULL;

	if (n) {
		 //we have a name
		from+=n;
		value=val->dereference(name,n); //if a simple value can be returned, we are in luck!
		if (!value) {
			 //if val is a normal Value, if deref comes back NULL, then we can look in
			 //the ObjectDef. If name is defined, and object can be cast to a FunctionEvaluator,
			 //then call that object's Evaluate() with parameters.

			def=val->GetObjectDef();
			if (def) def=def->FindDef(name,n);
			//if (def) then so def is the definition of the member function
			usethis=val;
		}

	} else if (curexprs[from]=='(' || curexprs[from]=='[') {
		char ch=(curexprs[from]=='(' ? ')' : ']');
		from++;

		 //something like set.(2+6-3), or set[3*2] so we need to evaluate
		Value *v=evalLevel(0);
		if (calcerror) return NULL;
		if (v->type()!=VALUE_Int) {
			calcerr(_("You may only dereference with integers"));
			v->dec_count();
			return NULL;
		}
		if (!nextchar(ch)) {
			if (ch==')') calcerr(_("Expected closing ')'"));
			else calcerr(_("Expected closing ']'"));
			v->dec_count();
			return NULL;
		}
		index=dynamic_cast<IntValue*>(v)->i;
		v->dec_count();

		value=val->dereference(index);
		if (!value) {
			calcerr(_("Bad index!"));
			return NULL;
		}

	} else if (isdigit(curexprs[from])) {
		long index=intnumber();
		if (calcerror) return NULL;

		value=val->dereference(index);
		if (!value) {
			calcerr(_("Bad index!"));
			return NULL;
		}


	} else {
		calcerr(_("Expected dereferencing!"));
		return NULL;
	}

	if (value && value->type()==VALUE_Function) {
		NamespaceValue *v=dynamic_cast<NamespaceValue*>(value);
		if (v && v->usethis) usethis=v->usethis;
	}

	 //if value is a function object or namespace like object, do something appropriate
	if (def!=NULL || (value && (value->type()==VALUE_Function || value->type()==VALUE_Class))) {
		 //if def!=NULL, then we are calling an object method on val
		if (!def) def=dynamic_cast<NamespaceValue*>(value)->GetObjectDef();
		skipwscomment();

		 //build context and find parameters
		//int error_pos=0;
		ValueHash *context=build_context(); //build calculator context
		ValueHash *pp=parseParameters(def); //build parameter hash in order of styledef
		if (calcerror) {
			context->dec_count();
			if (pp) delete pp;
			if (value) value->dec_count();
			return NULL;
		}

		if (usethis) {
			 //add as "this" to pp
			int i=(pp?pp->findIndex("this",4):-1);
			if (i>=0) pp->set(i,usethis);
			else {
				if (!pp) pp=new ValueHash;
				pp->push("this",usethis);
			}

		} else usethis=val;

		 //call the function
		int status=-1;
		Value *fvalue=NULL;
		status=functionCall(name,n, &fvalue, usethis,def,context,pp);
		context->dec_count();
		if (pp) delete pp;

		if (status==1 || calcerror) {
			if (fvalue) fvalue->dec_count();
			if (value) value->dec_count();
			return NULL;

		} else if (status==-1) {
			if (value) value->dec_count();
			calcerr(_("Cannot compute with given values."));
			return NULL;
		}

		if (value) value->dec_count();
		return fvalue;


	} else if (value && value->type()==VALUE_Operator) {
		// *** can't actually do anything with this, but may be called for informational purposes

	} else if (value && value->type()==VALUE_Namespace) {
		//return as is for further dereferencing!
	}

	return value;
}


ValueHash *LaidoutCalculator::getValueHash()
{
	int tfrom=from;
	if (!nextchar('{')) return NULL;

	int n;
	const char *key;
	ValueHash *hash=NULL;
	Value *s=NULL;
	Value *num=NULL;

	do {
		key=NULL;
		n=0;

		if (nextchar('"')) {
			 //read in quoted thing, can be a hash key
			from--;
			Value *s=getstring();

			if (!s || s->type()!=VALUE_String) {
				if (s) s->dec_count();
				from=tfrom;
				return NULL;
			}

			key=dynamic_cast<StringValue*>(s)->str;
			n=strlen(key);
		} else {
			 //read in name string
			key=getnamestringc(&n);
			from+=n;
		}

		//if (nextchar(':')) {
		if (nextchar(':') || nextchar('=')) {
			if (!hash) hash=new ValueHash;
		} else {
			 //is not hash entry, so force reading as set
			if (hash) hash->dec_count();
			if (s) s->dec_count();
			from=tfrom;
			return NULL;
		}

		num=evalLevel(0);
		if (calcerror) {
			if (hash) hash->dec_count();
			if (s) s->dec_count();
			return NULL;
		}

		if (key && num) hash->push(key,n, num);
		// *** note that if either but not both is NULL, then it is skipped... what to do!!
		if (num) num->dec_count();

		if (s) { s->dec_count(); s=NULL; }
	} while (nextchar(',') || nextchar(';')); //hash entries are separated by ',' or ';', to be nice to css

	if (!nextchar('}')) {
		calcerr("Expecting '}'.");
		hash->dec_count();
		return NULL;
	}
	return hash;
}

//! Read in values for a set.
/*! Next char must be '(' or '{'. Otherwise return NULL.
 * If '(', then ApplyDefaultSets() is used to convert something like (2,3) to a flatvector.
 *
 * Will automatically convert LValue objects to normal values.
 */
Value *LaidoutCalculator::getset()
{
	char setchar=0;
	if (nextchar('{')) {
		from--;
		Value *v=getValueHash();
		if (v) return v;
		from++;
		setchar='}';
	} else if (nextchar('(')) setchar=')';
	else return NULL;

	SetValue *set=new SetValue;
	do {
		Value *num=evalLevel(0);
		if (calcerror) {
			delete set;
			return NULL;
		}
		if (num) {
			if (num->type()==VALUE_LValue) {
				set->Push(dynamic_cast<LValue*>(num)->Resolve(),1);
				num->dec_count();
			} else set->Push(num,1);
		}
		
	} while (nextchar(','));

	if (!nextchar(setchar)) {
		if (setchar==')') calcerr("Expecting ')'.");
		else calcerr("Expecting '}'.");
		delete set;
		return NULL;
	}

	Value *defaulted = (setchar==')'?ApplyDefaultSets(set):NULL); //always a set when '{'
	if (defaulted) { set->dec_count(); return defaulted; }
	else if (setchar==')' && set->values.n==0) {
		calcerr("Expecting a number!");
		delete set;
		return NULL;
	}

	return set;
}

/*! Will automatically convert LValue objects to normal values.
 */
Value *LaidoutCalculator::getarray()
{
	if (!nextchar('[')) return NULL;

	ArrayValue *array=new ArrayValue;
	do {
		Value *num=evalLevel(0);
		if (calcerror) {
			return NULL;
		}
		if (num->type()==VALUE_LValue) {
			array->Push(dynamic_cast<LValue*>(num)->Resolve(),1);
			num->dec_count();
		} else array->Push(num,1);
		
	} while (nextchar(','));

	if (!nextchar(']')) {
		calcerr("Expecting ']'.");
		delete array;
		return NULL;
	}

	if (array->values.n==0) {
		calcerr("Expecting a number!");
		delete array;
		return NULL;
	}

	return array;
}

/*! Currently, create flatvector or spacevector if has 2 or 3 real or int elements.
 * Sets of single elements are taken out of the set.
 *
 * Returns a fresh object upon conversion, set is left unmodified.
 */
Value *LaidoutCalculator::ApplyDefaultSets(SetValue *set)
{
	if (set->values.n==1) {
		Value *v=set->values.e[0];
		v->inc_count();
		return v;
	}

	if (set->values.n!=2 && set->values.n!=3) return NULL;
	double v[3];
	for (int c=0; c<set->values.n; c++) {
		if (set->values.e[c]->type()==VALUE_Int) v[c]=((IntValue*)set->values.e[c])->i;
		else if (set->values.e[c]->type()==VALUE_Int) v[c]=((DoubleValue*)set->values.e[c])->d;
		else return NULL;
	}
	if (set->values.n==2) return new FlatvectorValue(flatvector(v[0],v[1]));
	return new SpacevectorValue(spacevector(v[0],v[1],v[2]));
}

//! Read in a double.
double LaidoutCalculator::realnumber()
{
	char *endptr,*startptr=curexprs+from;
	double r=strtod(startptr,&endptr);
	if (endptr==startptr) {
		calcerr("Expected a number.");
		return 0;
	}
	from+=endptr-startptr;
	return r;
}

//! Read in an integer.
long LaidoutCalculator::intnumber()
{
	char *endptr,*startptr=curexprs+from;
	//long c=strtol(startptr,&endptr,base);
	long c=strtol(startptr,&endptr,10);
	if (endptr==startptr) {
		calcerr("Expected an integer.");
		return 0;
	}
	from+=endptr-startptr;
	return c;
}

/*! \todo *** this needs to be more robust.. right now just reads in a single unit.
 * 
 * Uses Laxkit::GetUnitManager().
 */
int LaidoutCalculator::getunits()
{
	skipwscomment();
	int n=0;
	const char *str=getnamestringc(&n);
	if (!n) return 0;

	UnitManager *u=GetUnitManager();
	return u->UnitId(str,n);
}

//! Read in a string enclosed in either single or double quote characters. Quotes are mandatory.
/*! If a string is not there, then NULL is returned.
 *
 * If we start on a single quote, then the string is closed with a single quote, and similarly
 * for a double quote.
 *
 * A couple of standard escape sequences apply:\n
 * \\n newline \n
 * \\t tab\n
 * \\\\ backslash\n
 * \\' single quote\n
 * \\" double quote\n
 * \# number sign, normally used to mark comments.
 */
Value *LaidoutCalculator::getstring()
{
	char quote;
	if (nextchar('\'')) quote='\'';
	else if (nextchar('"')) quote='"';
	else return NULL;
	int tfrom=from;

	int maxstr=20,spos=0;
	char *newstr=new char[maxstr];
	char ch;
	newstr[0]='\0';

	 // read in the string, translating escaped things as we go
	while (from<curexprslen && curexprs[from]!=quote) {
		ch=curexprs[from];
		 // escape sequences
		if (ch=='\\') {
			ch=0;
			if (curexprs[from+1]=='\n') { from+=2; continue; } // escape the newline
			else if (curexprs[from+1]=='\'') ch='\'';
			else if (curexprs[from+1]=='"') ch='"';
			else if (curexprs[from+1]=='n') ch='\n';
			else if (curexprs[from+1]=='t') ch='\t';
			else if (curexprs[from+1]=='\\') ch='\\';
			if (ch) from++; else ch='\\';
		}
		if (spos==maxstr) extendstr(newstr,maxstr,20);
		newstr[spos]=ch;
		spos++;
		from++;
	}
	if (curexprs[from]!=quote) {
		delete[] newstr;
		from=tfrom;
		calcerr(_("String not closed."));
		return NULL;
	}
	from++;
	newstr[spos]='\0';
	Value *str=new StringValue(newstr,spos);
	delete[] newstr;
	return str;
}

//! Return the nearest Entry object corresponding to word.
/*! scope is index in scopes.
 *  module is module id of that scope's namespace.
 *  index is index in name dict of that scope.
 */
Entry *LaidoutCalculator::findNameEntry(const char *word,int len, int *scope, int *module, int *index)
{
	Entry *entry=NULL;
	int i=-1; //index in scope->entries.. for overloading?
	for (int c=scopes.n-1; c>=0; c--) {
		//entry=scopes.e[c]->FindName(word,len, &i);
		entry=scopes.e[c]->FindName(word,len);
		if (entry) {
			*scope=c;
			*module=scopes.e[c]->scope_namespace->object_id;
			if (index) *index=i;
			return entry;
		}
	}
	return NULL;
}

/*! from points to beginning of word.
 * If name not found, then from will still point to beginning of word.
 * No dereferencing is done here. That is done in number().
 */
Value *LaidoutCalculator::evalname()
{
	int n=0;
	const char *word=getnamestringc(&n);
	if (!word) return NULL;

	 //--------check for reserved words
	if (n==4 && !strncmp(word,"true",4)) return new BooleanValue(1);
	if (n==5 && !strncmp(word,"false",5)) return new BooleanValue(0);
	//if (n==4 && !strncmp(word,"null",4)) return new NullValue(1);


	if (n==6 && !strncmp(word,"typeof",6)) {
		from+=6;
		Value *value=evalLevel(0);
		if (calcerror || !value) {
			if (!calcerror) calcerr(_("Expected value!"));
			return NULL;
		}
		ObjectDef *def=value->GetObjectDef();
		StringValue *s=NULL;
		if (def) {
			s=new StringValue(def->name);
		} else {
			s=new StringValue("unimplemented def! rats!"); // *** this shouldn't happen
		}
		value->dec_count();
		return s;
	}

	if (n==7 && !strncmp(word,"typesof",7)) {
		from+=7;
		Value *value=evalLevel(0);
		if (calcerror || !value) return NULL;
		ObjectDef *def=value->GetObjectDef();
		SetValue *set=new SetValue();
		if (def) {
			set->Push(new StringValue(def->name),1);
			for (int c=0; c<def->extendsdefs.n; c++) {
				set->Push(new StringValue(def->extendsdefs.e[c]->name),1);
			}
		} else {
			set->Push(new StringValue("unimplemented def! rats!"),1); // *** this shouldn't happen
		}
		value->dec_count();
		return set;
	}

	 //type conversion of anything to string
	if (n==6 && !strncmp(word,"string",6)) {
		from+=6;
		StringValue *s=NULL;
		if (nextchar('(')) {
			Value *v=evalLevel(0);
			if (calcerror) return NULL;
			s=new StringValue;
			int n=0;
			v->getValueStr(&s->str,&n,1);
			v->dec_count();

			if (!nextchar(')')) {
				calcerr(_("Expected closing ')'"));
				if (s) s->dec_count();
				return NULL;
			}
		} else s=new StringValue;
		return s;

	} else if (n==3 && !strncmp(word,"int",3)) {
		from+=3;
		IntValue *i=NULL;
		if (nextchar('(')) {
			Value *v=evalLevel(0);
			if (calcerror) return NULL;
			i=new IntValue;

			if (v->type()==VALUE_LValue) {
				Value *vv=dynamic_cast<LValue*>(v)->Resolve();
				v->dec_count();
				v=vv;
			}
			if (v->type()==VALUE_Int) {
				i->i=dynamic_cast<IntValue*>(v)->i;
			} else if (v->type()==VALUE_Real) {
				i->i=int(dynamic_cast<DoubleValue*>(v)->d);
			} else {
				calcerr(_("Could not convert to int"));
				i->dec_count();
				v->dec_count();
				return NULL;
			}
			v->dec_count();

			if (!nextchar(')')) {
				calcerr(_("Expected closing ')'"));
				if (i) i->dec_count();
				return NULL;
			}
		} else i=new IntValue;
		return i;
	}


	 //---------search for name in scopes
	int scope=-1;
	int module=-1;
	Entry *entry=findNameEntry(word,n, &scope, &module, NULL);
	OverloadedEntry *oo=dynamic_cast<OverloadedEntry*>(entry);
	int ooi=-1; //index in list of overloaded entries
	if (!entry) {
		calcerr(_("Unknown name!"));
		return NULL;
	}

	if (oo) {
		if (!oo->entries.n) return NULL; //should remove the overloaded thing, since none use it
		ooi=oo->entries.n-1;
		entry=oo->entries.e[oo->entries.n-1];
		//now any function calls that return -1 can step through available name resolution in oo
	}

	if (entry->type()==VALUE_Function) {
		ObjectDef *function=dynamic_cast<FunctionEntry*>(entry)->def;
		Value *v=NULL;

		 //we found a function!
		from+=n;
		skipwscomment();

		 //build context and find parameters
		//int error_pos=0;
		ValueHash *context=build_context(); //build calculator context
		ValueHash *pp=parseParameters(function); //build parameter hash in order of styledef
		if (calcerror) {
			context->dec_count();
			if (pp) delete pp;
			return NULL;
		}

		int status=-1;
		do {
			if (function) {
				status=functionCall(word,n, &v, NULL,function,context,pp);
				if (calcerror) {
					context->dec_count();
					if (pp) delete pp;
					return NULL;
				}
			}
			if (status==-1) ooi--;
			if (ooi>=0) {
				entry=oo->entries.e[ooi];
				if (entry->type()==VALUE_Function) function=dynamic_cast<FunctionEntry*>(entry)->def;
				else if (entry->type()==VALUE_Class) function=dynamic_cast<ObjectDefEntry*>(entry)->module;
				else function=NULL;

			} else entry=NULL;
		} while (status==-1 && ooi>=0);

		if (status==-1) {
			char *error=newstr(_("Cannot compute with given values."));
			//appendstr(error,_("Choices are:\n"));
			//if (oo) {
			//	for (int c=oo->entries.n-1; c>=0; c--) {
			//		***append description
			//	}
			//} else 
			calcerr(error);
			delete[] error;
			context->dec_count();
			if (pp) delete pp;
			return NULL;
		}

		context->dec_count();
		if (pp) delete pp;
		return v;

	} else if (entry->type()==VALUE_Namespace) {
		 //use namespace as a scope ONLY while parsing extensions
		ObjectDef *ns=dynamic_cast<NamespaceEntry*>(entry)->module;
		from+=n;
		Value *v=new NamespaceValue(ns);
		return v;

	} else if (entry->type()==VALUE_Class) {
		//just like NamespaceEntry, but you cannot make new objects from a namespace
		//object [+ other]-> create new instances of object
		//object(blah,blah) -> create new instances of object
		//object.blah() -> 1. run a "static" function, or get something else from within object
		//				   2. if in subclass space of object, run object's function with appropriate this
		from+=n;
		Value *v=NULL;
		if (nextchar('.')) {
			 //trying to access object member. If this is defined, and it inherites from this object class,
			 //then call that class's member..? todo
			cerr <<" *** cannot access class member, INCOMPLETE CODING!!"<<endl;
			calcerr("INCOMPLETE CODING");
			return NULL;
		}

		 //is perhaps constructor call
		ObjectDef *classdef=dynamic_cast<NamespaceEntry*>(entry)->module;
		ObjectDef *constructordef=classdef->FindDef(classdef->name);
		ValueHash *context=build_context(); //build calculator context
		ValueHash *pp=parseParameters(constructordef); //build parameter hash, but NULL means no parameter mapping as yet

		if (calcerror) {
			context->dec_count();
			if (pp) delete pp;
			return NULL;
		}

		//int status=-1;
		functionCall(word,n, &v, NULL, classdef, context,pp);

		context->dec_count();
		if (pp) delete pp;
		return v;

	} else if (dynamic_cast<ValueEntry*>(entry)) {
		//Value *v=dynamic_cast<ValueEntry*>(entry)->GetValue();
		//if (v) v->inc_count();
		//from+=n;
		//return v;

		 //prep for possible lvalue based assignment operator
		ValueEntry *ve=dynamic_cast<ValueEntry*>(entry);
		LValue *val=new LValue(word,n, ve->GetValue(),ve,NULL,NULL);
		from+=n;
		return val; 

//	} else if (dynamic_cast<AliasEntry*>(entry)) {
//		// *** should be able to do something like this: alias blah.(5+3).func() aliasname
//		//replace entry with new entry found via name in the alias, then repeat the found entry (if any)
//		AliasEntry *alias=dynamic_cast<AliasEntry*>(entry);
//		from+=n-strlen(alias->aliasto); //hack to trick evalname to using the alias
//		Value *v=evalname(alias->aliasto,strlen(alias->aliasto));
//		return v;

	//} else if (dynamic_cast<OperatorEntry*>(entry)) {
		//currently, operators are searched for differently and must not have letters, see eval()
	}


	return NULL;
}

/*! Parse out a name of an existing class, for use in the "extends" part of a class definition.
 */
ObjectDef *LaidoutCalculator::getClass()
{
	int tfrom=from;
	ObjectDef *def=NULL;

	Value *v=evalname();

	if (calcerror) { if (v) v->dec_count(); return NULL; }
	if (!v) { from=tfrom; return NULL; }

	while (nextchar('.') || curexprs[from]=='[') {
		 //there is dereferencing to be done
		Value *v2=dereference(v);
		if (calcerror) {
			if (v) v->dec_count();
			return NULL;
		}

		v->dec_count();
		v=v2;
	}

	if (v->type()!=VALUE_Class) { v->dec_count(); from=tfrom; return NULL; }

	def=v->GetObjectDef();
	def->inc_count();
	v->dec_count();
	return def;
}

//! Perform operator overloading, using a particular operator level.
/*! Assume level and index were the last one checked, and it couldn't be used.
 */
Value *LaidoutCalculator::opCall(const char *op,int n,int dir, Value *num, Value *num2, OperatorLevel *level, int lastindex)
{
	if (lastindex<0) lastindex=-1;
	OperatorFunction *opfunc=NULL;
	int index=-1;
	int status=0;
	Value *num_ret=NULL;
	Value *num1v=NULL, *num2v=NULL;
	int uselvalue=0;

	for (int c=lastindex+1; c<level->ops.n; c++) {
		opfunc=level->hasOp(op,n,dir, &index,lastindex);
		if (!opfunc) {
			if (num1v) num1v->dec_count();
			if (num2v) num2v->dec_count();
			return NULL;
		}

		uselvalue=(opfunc->def && (opfunc->def->flags&OPS_Assignment));
		if (uselvalue) {
			 //operator is potentially an assignment
			status=opfunc->function->Op(op,n,dir, num,  num2,  &calcsettings, &num_ret, NULL);
		} else {
			 //need to resolve any LValue states, operator is NOT as assignment
			if (!num1v && num  && num->type() ==VALUE_LValue) num1v=dynamic_cast<LValue*>(num) ->Resolve();
			if (!num2v && num2 && num2->type()==VALUE_LValue) num2v=dynamic_cast<LValue*>(num2)->Resolve();
			status=opfunc->function->Op(op,n,dir, num1v,num2v, &calcsettings, &num_ret, NULL);
		}

		if (status==0) {
			if (num1v) num1v->dec_count();
			if (num2v) num2v->dec_count();
			return num_ret;
		}
		if (status==-1) { lastindex=c; continue; }
		if (calcerror) break;
	}

	if (num1v) num1v->dec_count();
	if (num2v) num2v->dec_count();
	return NULL;
}

//! Convenience function to call functions, and do error handling
int LaidoutCalculator::functionCall(const char *word,int n, Value **v_ret, 
									Value *containingvalue, ObjectDef *function,ValueHash *context,ValueHash *pp)
{
	if (calcerror) return 1;

	 //call the actual function
	ErrorLog log;
	Value *value=NULL;
	int status=-2;

	 //first check standard evaluation methods
	if (function->evaluator) status=function->evaluator->Evaluate(word,n, context,pp,&calcsettings, &value,&log);
	else if (function->stylefunc) status=function->stylefunc(context,pp, &value,log);
	else if (function->newfunc) {
		 //a plain object creation function with no parameters
		Value *s=function->newfunc();
		if (s) {
			status=0;
			value=s;
		} else status=1;
	} else if (function->defaultvalue) {
		 //execute code
		 // *** set up a scope with containingvalue elements, and call function

		//if (!pp) pp=new ValueHash;
		//int i=pp->findIndex("this");
		//if (i>=0) pp->set(i,containingvalue);
		//else pp->push("this",containingvalue);

		int dec=0;
		if (function->format==VALUE_Class && !containingvalue) {
			containingvalue=new GenericValue(function);
			dec=1;
			//***
		}
		//f(x,y)   <- x and y are parameters, listed in pp
		//o.f(x,y) <- function must run in object o space, o==containingvalue
		if (containingvalue) pushScope(BLOCK_object, 0,0,NULL,containingvalue,NULL);

		status=evaluate(function->defaultvalue,-1, context,pp, &value,&log); 
		if (status==-1) status=0;//warnings, but success
		if (containingvalue) popScope(); // ...pop from scope we just added
		if (dec) {
			if (status==0) value=containingvalue;
			else containingvalue->dec_count();
			containingvalue=NULL;
		}
	}


	 //if class name, then search for scripted constructor functions.
	if (!value && status==-2 && function->format==VALUE_Class) {
		ObjectDef *def=function->FindDef(function->name);
		if (def) {
			 //constructor found
			GenericValue *v=new GenericValue(function);
			status=functionCall(word,n,v_ret, v, def, context,pp);
			if (!*v_ret) *v_ret=v; //if constructor returns a value, use that
			else { v->dec_count(); v=NULL; }
			if (status!=-1) return status;
			if (v) v->dec_count();
		} else {
			 //no constructor, and no other evaluator, so make a new GenericValue
			GenericValue *v=new GenericValue(function);
			*v_ret=v;
			return 0;
		}
	}


	 //function not found yet, so try containingvalue as a FunctionEvaluator
	if (!value && status==-2 && containingvalue) {
		FunctionEvaluator *f=dynamic_cast<FunctionEvaluator*>(containingvalue);
		if (f) {
			status=f->Evaluate(word,n, context,pp,&calcsettings, &value,&log);
		}
	}

	 //report default success or failure
	if (log.Total()) {
		char *error=NULL;
		char scratch[100];
		ErrorLogNode *e;
		int fail=0;
		for (int c=0; c<log.Total(); c++) {                 
			e=log.Message(c);
			if (e->severity==ERROR_Ok) ;
			else if (e->severity==ERROR_Warning) appendstr(error,"Warning: ");
			else if (e->severity==ERROR_Fail) { fail++; appendstr(error,"Error! "); }

			if (e->line>=0) {
				sprintf(scratch," line %d, ",e->line);
				appendstr(error,scratch);
			}
		
			if (e->objectstr_id) {
				sprintf(scratch,"id:%s, ",e->objectstr_id);
				appendstr(error,scratch);
			}
			appendstr(error,e->description);
			appendstr(error,"\n");
		}
		
		if (fail) calcerr(error);
		messageOut(error);
		delete[] error;

	} else if (status==-2) {
		calcerr(_("Could not call function! Poor programming!"));
	} else {
		if (status>0) calcerr(_("Command failed, tell the devs to properly document failure!!"));
	}

	if (calcerror) { if (value) value->dec_count(); value=NULL; }
	*v_ret=value;
	return status;
}

/*! Return number of OperatorFunction instances removed.
 */
int LaidoutCalculator::removeOperators(int module_id)
{
	int n=0;
	for (int c=leftops.ops.n-1; c>=0; c--) {
		if (leftops.ops.e[c]->module_id==module_id) { leftops.ops.remove(c); n++; }
	}

	for (int c=rightops.ops.n-1; c>=0; c--) {
		if (rightops.ops.e[c]->module_id==module_id) { rightops.ops.remove(c); n++; }
	}

	for (int c=oplevels.n; c>=0; c--) {
		for (int c2=oplevels.e[c]->ops.n-1; c2>=0; c2--) {
			if (oplevels.e[c]->ops.e[c2]->module_id==module_id) oplevels.e[c]->ops.remove(c2);
			if (oplevels.e[c]->ops.n==0) oplevels.remove(c);
			n++;
		}
	}

	return n;
}

int LaidoutCalculator::addOperator(const char *op,int dir,int priority, int module_id, OpFuncEvaluator *opfunc,ObjectDef *def)
{
	if (dir==OPS_Left) leftops.pushOp(op, dir, opfunc, def, module_id);
	else if (dir==OPS_Right) rightops.pushOp(op, dir, opfunc, def, module_id);
	else {
		int c;
		for (c=0; c<oplevels.n; c++) {
			if (priority<oplevels.e[c]->priority) {
				OperatorLevel *level=new OperatorLevel(dir,priority);
				oplevels.push(level,1,c); //insert new level before this level
				level->pushOp(op, dir, opfunc, def, module_id);
				break;
			}
			if (priority==oplevels.e[c]->priority && oplevels.e[c]->direction==dir) {
				oplevels.e[c]->pushOp(op, dir, opfunc, def, module_id);
				break; //found an existing level
			}
		}
		if (c==oplevels.n) {
			 //adding level at end
			OperatorLevel *level=new OperatorLevel(dir,priority);
			oplevels.push(level,1,c); //insert new level before this level
			level->pushOp(op, dir, opfunc, def, module_id);
		}
	}
	return 0;
}

//! This will be for status messages during the execution of a script.
/*! \todo *** implement this!!
 *
 * This function exists to aid in providing incremental messages to another thread. This is
 * so you can run a script basically in the backgroud, and give status reports to the user,
 * who might want to halt the script based on what's happening.
 */
void LaidoutCalculator::messageOut(const char *str,int output_lines)
{//***
	//DBG cerr <<"*** must implement LaidoutCalculator::messageOut() properly! "<<endl;
	//DBG if (laidout->runmode!=RUNMODE_Shell) cerr <<str<<endl;

	if (output_lines) appendline(messagebuffer,str);
	else appendstr(messagebuffer,str);
}


//! Create an object with the context of this calculator.
/*! This includes a default document, default window, current directory.
 *
 * \todo shouldn't have to build this all the time
 */
ValueHash *LaidoutCalculator::build_context()
{
	ValueHash *pp=new ValueHash();
	pp->push("current_directory", dir);
	//if (laidout->project->docs.n) pp->push("document", laidout->project->docs.e[0]->doc->Saveas());//***
	if (laidout->project->docs.n) pp->pushObject("document", laidout->project->docs.e[0]->doc);
	//pp.push("window", NULL); ***default to first view window pane in same headwindow?
	return pp;
}

//! Parse into function parameters.
/*! If def!=NULL, then call MapParameters() to get the parameters to match the fields of the ObjectDef.
 */
ValueHash *LaidoutCalculator::parseParameters(ObjectDef *def)
{
	ValueHash *pp=NULL;

// ***	 //check for when the styledef has 1 string parameter, and you have a large in string.
//	 //all of in maps to that one string. This allows functions to act like command line strings.
//	 //They would then call some other function to parse the string or parse it internally.
//	if (def->getNumFields()==1 ) {
//		ValueTypes fmt=VALUE_None;
//		const char *nm=NULL;
//		def->getInfo(0,&nm,NULL,NULL,NULL,NULL,&fmt);
//		if (fmt==VALUE_String) {
//			pp->push(nm,str);
//			delete[] str;
//			return pp;
//		}
//	}

	if (nextchar('(')) {
		pp=new ValueHash;
		int tfrom;
		char *pname=NULL, *ename=NULL;
		int namel;
		Value *v=NULL;
		int pnum=0;
		int enumcheck;
		if (nextchar(')')) { from--; }
		else do {
			enumcheck=-1;//default field number to check for enum values in
			pnum++; //starts at 1

			 //check for parameter name given
			skipwscomment();
			tfrom=from;
			pname=getnamestring(&namel);
			if (pname) {
				from+=namel;
				if (nextchar('=')) {
					//it is a parameter name (hopefully) so nothing special to do here
					if (def) enumcheck=def->findfield(pname,NULL);
					tfrom=from; //update from to just after '=', might have to reset after enum check
				} else {
					 //name was not in the form of parameter assignment
					ename=pname;
					pname=NULL; //ename will be deleted below
					if (def) enumcheck=pnum; //pname might be an enum value for styledef.field[pnum]
				}
			}

			 //do special check for enum values, and convert to an IntValue, holding
			 //the index of the corresponding enum value
			if (def && enumcheck>=0) {
				 //If the field is known to not be an enum, then do not check for enum values
				ValueTypes t;
				if (def->getInfo(enumcheck, NULL,NULL,NULL,NULL,NULL,&t,NULL)==0 && t==VALUE_Enum) {
					if (!ename) {
						skipwscomment();
						int len;
						ename=getnamestring(&len);
						if (ename) from+=len;
					}
					ObjectDef *ev=NULL;
					if (ename) ev=def->getField(enumcheck);
					DBG if (!ev) { cerr <<" ***** Missing fields in expected enum!!!"<<endl;  }
					
					 //*** warning, assumes no extended enum, or dynamic enum!!! maybe bad....
					DBG cerr <<"*** enum value scan must search for dynamic enums eventually..."<<endl;
					if (ev && ev->fields) for (int c=0; c<ev->fields->n; c++) {
						if (strcmp(ev->fields->e[c]->name,ename)==0) {
							skipwscomment();
							if (curexprs[from]!=',' && curexprs[from]!=')') {
								calcerr(_("Problem parsing enumeration value"));
							} else v=new IntValue(c);
							break;
						}
					}
					if (!v) enumcheck=-1; //force scan for value 
				} else {
					enumcheck=-1; //either info check failed, or field is not an enum, so still must parse value
					from=tfrom;
				}
				if (ename) { delete[] ename; ename=NULL; }
			}
			if (ename) {
				delete[] ename;
				from=tfrom;
			}
			if (enumcheck<0) v=evalLevel(0); //either enum check failed, or was not an enum

			if (v && v->type()==VALUE_LValue) {
				 //functions can't use LValue objects, only operators can do that, so remove any LValue status
				Value *v2=dynamic_cast<LValue*>(v)->Resolve();
				v->dec_count();
				v=v2;
			}

			if (v && !calcerror) {
				pp->push(pname,v);
				v->dec_count();
				v=NULL;
			} else if (calcerror) {
				if (pname) { delete[] pname; pname=NULL; }
				break;
			}
			if (pname) { delete[] pname; pname=NULL; }

		} while (!calcerror && from!=tfrom && nextchar(',')); //do foreach parameter
		if (!calcerror) {
			if (!nextchar(')')) {
				calcerr(_("Expected closing ')'"));
				delete pp;
				pp=NULL;
			}
		} else {//calcerror
			if (pp) delete pp;
			return NULL;
		}
	} 

	if (pp && def) MapParameters(def,pp);
	return pp;
}


//---------------------built in ops:


void LaidoutCalculator::InstallBaseTypes()
{
	global_scope.scope_namespace->push(Get_ValueHash_ObjectDef(),0);
	global_scope.scope_namespace->push(Get_SetValue_ObjectDef(),0);
	global_scope.scope_namespace->push(Get_StringValue_ObjectDef(),0);

	//global_scope.scope_namespace->push(Get_BooleanValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_IntValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_RealValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_FlatvectorValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_SpacevectorValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_DateValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_ColorValue_ObjectDef(),0);
	//global_scope.scope_namespace->push(Get_FileValue_ObjectDef(),0);
}

//! Create and install various built in operators and math functions.
void LaidoutCalculator::InstallInnate()
{
//	ObjectDef *innates=new ObjectDef(NULL,"Builtin",_("Builtin"),_("Things that are built in to the calculator."),
//										   VALUE_Namespace,NULL,NULL);

	ObjectDef *innates=new ObjectDef(NULL,"Math",_("Math"),_("Mathematical functions, plus pi, tau, and e."),
										   "namespace",NULL,NULL);

	 //operators
	innates->pushOperator("=",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);
	//innates->pushOperator("+=",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);
	//innates->pushOperator("-=",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);
	//innates->pushOperator("*=",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);
	//innates->pushOperator("/=",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);
	//innates->pushOperator("++",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);
	//innates->pushOperator("--",OPS_RtoL,50, _("Assignment"), this, OPS_Assignment);

	innates->pushOperator("||",OPS_LtoR,100, _("Condition or"), this);

	innates->pushOperator("&&",OPS_LtoR,200, _("Condition and"), this);

	innates->pushOperator("<=",OPS_LtoR,300, _("Less than or equal"), this);
	innates->pushOperator("<", OPS_LtoR,300, _("Less than"), this);
	innates->pushOperator(">=",OPS_LtoR,300, _("Greater than or equal"), this);
	innates->pushOperator(">", OPS_LtoR,300, _("Greater than"), this);
	innates->pushOperator("==",OPS_LtoR,300, _("Is equal to"), this);
	innates->pushOperator("!=",OPS_LtoR,300, _("Is not equal to"), this);

	innates->pushOperator("+", OPS_LtoR,400, _("Add"), this);
	innates->pushOperator("-", OPS_LtoR,400, _("Subtract"), this);

	innates->pushOperator("*", OPS_LtoR,500, _("Multiply"), this);
	innates->pushOperator("/", OPS_LtoR,500, _("Divide"), this);

	innates->pushOperator("^", OPS_RtoL,600, _("Raise to the power of"), this);

	innates->pushOperator("+", OPS_Left,0,   _("Unary positive"), this);
	innates->pushOperator("-", OPS_Left,0,   _("Unary negative"), this);


//	ObjectDef *innates=new ObjectDef(NULL,"Math",_("Math"),_("Basic math functions and numbers."),
//										   VALUE_Namespace,NULL,NULL);
	 //variables
    innates->pushVariable("pi", "pi",      _("pi"),                        NULL,0, new DoubleValue(M_PI),1);
    innates->pushVariable("tau","tau",     _("golden section"),            NULL,0, new DoubleValue((1+sqrt(5))/2),1);
    innates->pushVariable("E",  "E",       _("base of natural logarithm"), NULL,0, new DoubleValue(exp(1)),1);
    //innates->define("i",  "sqrt(-1)",_("imaginary number"),          0);


	 //functions
    innates->pushFunction("atan2",    NULL,_("Arctangent, returning values in range [-pi,pi]."),this, 
									   "y",NULL,NULL,"number",NULL,NULL,
									   "x",NULL,NULL,"number",NULL,NULL,
									   NULL); // "x:int|real,y:int|real");
    innates->pushFunction("sin",      NULL,_("sine"),                                   this, "r",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("asin",     NULL,_("arcsine"),                                this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("cos",      NULL,_("cosine"),                                 this, "r",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("acos",     NULL,_("arccosine"),                              this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("tan",      NULL,_("tangent"),                                this, "r",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("atan",     NULL,_("arctangent"),                             this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("abs",      NULL,_("absolute value"),                         this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("sqrt",     NULL,_("square root"),                            this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("log",      NULL,_("base 10 logarithm"),                      this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("ln",       NULL,_("natural logarithm"),                      this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("exp",      NULL,_("exponential"),                            this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("cosh",     NULL,_("hyperbolic cosine"),                      this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("sinh",     NULL,_("hyperbolic sine"),                        this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("tanh",     NULL,_("hyperbolic tangent"),                     this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real|complex");
    innates->pushFunction("sgn",      NULL,_("pos,neg,or 0"),                           this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real");
    innates->pushFunction("int",      NULL,_("integer of"),                             this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real");
    innates->pushFunction("gint",     NULL,_("greatest integer less than"),             this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real");
    innates->pushFunction("lint",     NULL,_("least integer greater than"),             this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "int|real");
    innates->pushFunction("factor",   NULL,_("set of factors of an integer"),           this, "i",NULL,NULL,"int",NULL,NULL, NULL); // "int");
    innates->pushFunction("det",      NULL,_("determinant of a square array"),          this, "a",NULL,NULL,NULL,NULL,NULL, NULL); // "array");
    innates->pushFunction("transpose",NULL,_("transpose of an array"),                  this, "a",NULL,NULL,NULL,NULL,NULL, NULL); // "array");
    innates->pushFunction("inverse",  NULL,_("inverse of square array"),                this, "x",NULL,NULL,NULL,NULL,NULL, NULL); // "array");
    innates->pushFunction("random",   NULL,_("Return a random number from 0 to 1"),     this, NULL);
    innates->pushFunction("randomint",NULL,_("Return a random integer from min to max"),this, // "min:int, max:int");
									   "min",NULL,NULL,"int",NULL,NULL,
									   "max",NULL,NULL,"int",NULL,NULL,
									   NULL); // "x:int|real,y:int|real");

	InstallModule(innates,1);
	innates->dec_count();
}

//! Compute various built in functions, such as from the built in Math module.
/*! parse and compute standard math functions like sqrt(x), sin(x), etc.
 * Also a couple math numbers like e, pi, and tau (golden ratio).
 *
 * Another function is factor(i) which returns a string with the factors of i.
 * 
 * Return
 *  0 for success, value returned.
 * -1 for no value returned due to incompatible parameters, which aids in function overloading.
 *  1 for parameters ok, but there was somehow an error, so no value returned.
 */
int LaidoutCalculator::Evaluate(const char *word,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *Log)
{
	ValueHash *pp=parameters;
	if (calcerror) return 1;

	 //math constants
	if (len==2 && !strncmp(word,"pi",2))  { *value_ret=new DoubleValue(M_PI);          return 0; }
	if (len==1 && !strncmp(word,"e",1))   { *value_ret=new DoubleValue(exp(1));        return 0; }
	if (len==3 && !strncmp(word,"tau",3)) { *value_ret=new DoubleValue((1+sqrt(5))/2); return 0; }

	if (pp==NULL || pp->n()==0) {
		if (len==6 && !strncmp(word,"random",6)) { *value_ret=new DoubleValue(random()/(double)RAND_MAX); return 0; }
		return -1;
	}

	Value *v=NULL, *vv=NULL;
	int status=0;
	int isnum;

	try {
		 //need at least one parameter
		vv=pp->value(0);
		isnum=0;
		double d=getNumberValue(vv,&isnum);
		if (!isnum) throw 1;

		 //two parameter functions
		if (pp->n()==2) {
			vv=pp->value(1);
			double d2=getNumberValue(vv,&isnum);
			if (!isnum) throw 1;

			if (len==5 && !strncmp(word,"atan2",5)) {
				d=atan2(d,d2);
				if (calcsettings.decimal) d*=180/M_PI;
				v=new DoubleValue(d);

			} else if (len==9 && !strncmp(word,"randomint",9)) {
				int min=int(d);
				int max=int(d2);
				v=new IntValue(min+(max-min+1)*random()/(double)RAND_MAX);

			} else { v=NULL; throw -1; }

		} else if (pp->n()==1) {
			if (len==3 && !strncmp(word,"sin",3))  { if (calcsettings.decimal) d*=M_PI/180; v=new DoubleValue(sin(d)); }
			else if (len==4 && !strncmp(word,"asin",4)) { if (d<-1. || d>1.) throw 3; d=asin(d); if (calcsettings.decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else if (len==3 && !strncmp(word,"cos",3))  { if (calcsettings.decimal) d*=M_PI/180; v=new DoubleValue(cos(d)); }
			else if (len==4 && !strncmp(word,"acos",4)) { if (d<-1. || d>1.) throw 3; d=acos(d); if (calcsettings.decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else if (len==3 && !strncmp(word,"tan",3))  { if (calcsettings.decimal) d*=M_PI/180; v=new DoubleValue(tan(d)); }
			else if (len==4 && !strncmp(word,"atan",4)) { v=new DoubleValue(atan(d)); }

			else if (len==3 && !strncmp(word,"abs",3))  { 
				if (vv->type()==VALUE_Int) v=new IntValue(abs(((IntValue*)vv)->i));
				else v=new DoubleValue(fabs(d)); 
			}
			else if (len==3 && !strncmp(word,"sgn",3))     { v=new IntValue(d>0?1:(d<0?-1:0)); }
			else if (len==3 && !strncmp(word,"int",3))     { v=new IntValue(int(d)); }
			else if (len==4 && !strncmp(word,"gint",4))    { v=new IntValue(floor(d)); }
			else if (len==5 && !strncmp(word,"floor",5))   { v=new IntValue(floor(d)); }
			else if (len==4 && !strncmp(word,"lint",4))    { v=new IntValue(ceil(d)); }
			else if (len==7 && !strncmp(word,"ceiling",7)) { v=new IntValue(ceil(d)); }
			else if (len==3 && !strncmp(word,"exp",3))     { v=new DoubleValue(exp(d)); }
			else if (len==4 && !strncmp(word,"cosh",4))    { v=new DoubleValue(cosh(d)); }
			else if (len==4 && !strncmp(word,"sinh",4))    { v=new DoubleValue(sinh(d)); }
			else if (len==4 && !strncmp(word,"tanh",4))    { v=new DoubleValue(tanh(d)); }

			else if (len==4 && !strncmp(word,"sqrt",4)) { if (d<0) throw 2; v=new DoubleValue(sqrt(d)); }

			else if (len==3 && !strncmp(word,"log",3))  { if (d<=0) throw 4; v=new DoubleValue(log(d)/log(10)); }
			else if (len==4 && !strncmp(word,"ln",2))   { if (d<=0) throw 4; v=new DoubleValue(log(d)); } // d must be >0

			else if (len==6 && !strncmp(word,"factor",6)) {
				if (vv->type()!=VALUE_Int) throw 1;
				long n=((IntValue*)vv)->i;
				
				double c=2;
				SetValue *set=new SetValue;
				if (n==0) set->Push(new IntValue(0),1);
				else if (n<0) {
					n=-n;
					set->Push(new IntValue(-1),1);
				} 
				while (n>1) {
					if (n-c*(int(n/c))==0) {
						set->Push(new IntValue(c),1);
						n/=c;
						c=2;
					} else c++;
				}
				v=set;
			} else throw -1;

		} else { v=NULL; throw -1; }

	} catch (int e) {
		if (e==-1) status=-1; //wrong number of parameters or name not found
		else if (e==1) { status=-1; } //not actually auto fail anymore: calcerr(_("Cannot compute with that type")); }
		else if (e==2) { status=1;  calcerr(_("Parameter must be greater than or equal to 0")); }
		else if (e==3) { status=1;  calcerr(_("Parameter must be in range [-1,1]")); }
		else if (e==4) { status=1;  calcerr(_("Parameter must be greater than 0")); }
		else { status=-1; calcerr(_("Can't do that math")); } //note that supposed this line should never be reached
		//v=NULL; //v should already be null here
	}

	*value_ret=v;
	return status;
}

//! Tries to assign num2 to num1, and return an LValue.
/*! num1 has to be assignable. Currently, this means num1 must be an LValue.
 *
 * \todo A future goal is maybe to have python like assignments (a,b)={1,2}.
 */
int LaidoutCalculator::Assignment(Value *num1, Value *num2, Value **value_ret, ErrorLog *log)
{
	DBG cerr <<" *** assignment"<<endl;

	if (num1->type()!=VALUE_LValue) {
		calcerr(_("Cannot assign to that!"));
		//if (log) log->AddMessage(..., ERROR_Fail, 0, from,curline);
		return 1;
	}

	int status=num1->assign(NULL,num2);
	if (status<=0) {
		calcerr(_("Cannot assign to that!"));
		return 1;
	}

	num1->inc_count();
	*value_ret=num1;
	return 0;
}

//! Figure out and evaluate built in operators
int LaidoutCalculator::Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings,
						  Value **value_ret, ErrorLog *log)
{
	if (len==1 && *the_op=='=') {
		return Assignment(num1,num2, value_ret,log);
	}

	if (dir==OPS_LtoR && len==1 && !strncmp(the_op,"+",1)) { return add(num1,num2, value_ret);
	} else if (dir==OPS_LtoR && len==1 && !strncmp(the_op,"-",1)) { return subtract(num1,num2, value_ret);
	} else if (dir==OPS_LtoR && len==1 && !strncmp(the_op,"*",1)) { return multiply(num1,num2, value_ret);
	} else if (dir==OPS_LtoR && len==1 && !strncmp(the_op,"/",1)) { return divide(num1,num2, value_ret);
	} else if (dir==OPS_RtoL && len==1 && !strncmp(the_op,"^",1)) { return power(num1,num2, value_ret);
	}

	int isnum=0;
	double v1,v2;
	//num*num, fv*fv, v*v, matrix*matrix


	if (dir==OPS_Left) { //is left hand op
		if (len==1 && *the_op=='+') {
			 //return unary positive
			*value_ret=NULL;
			if (     num1->type()==VALUE_Int
				  || num1->type()==VALUE_Real
				  || num1->type()==VALUE_Flatvector
				  || num1->type()==VALUE_Spacevector
				  || num1->type()==VALUE_Quaternion) {
				*value_ret = num1->duplicate();
				return 0;
			}
			return -1; //can't positive that type
		}
		if (len==1 && *the_op=='-') {
			 //return -num1;
			*value_ret=NULL;
			if (num1->type()==VALUE_Int) *value_ret= new IntValue(-dynamic_cast<IntValue*>(num1)->i);
			else if (num1->type()==VALUE_Real) *value_ret= new DoubleValue(-dynamic_cast<DoubleValue*>(num1)->d);
			else if (num1->type()==VALUE_Flatvector) *value_ret= new FlatvectorValue(-dynamic_cast<FlatvectorValue*>(num1)->v);
			else if (num1->type()==VALUE_Spacevector) *value_ret= new SpacevectorValue(-dynamic_cast<SpacevectorValue*>(num1)->v);
			else if (num1->type()==VALUE_Quaternion)  *value_ret= new QuaternionValue(-dynamic_cast<QuaternionValue*>(num1)->v);
			if (*value_ret) return 0;
			return -1; //can't negative that type
		}

		return 0;
	}

	if (dir==OPS_Right) { //is right hand op
		if (len==2 && !strncmp(the_op,"++",2)) ; //***return num1++;
		if (len==2 && !strncmp(the_op,"--",2)) ; //***return num1--;
		calcerr("INCOMPLETE CODING for right op!!");
		return 0;
	}

	if (len==1 && *the_op=='*')      return multiply(num1,num2, value_ret);
	else if (len==1 && *the_op=='/') return divide  (num1,num2, value_ret);
	else if (len==1 && *the_op=='+') return add     (num1,num2, value_ret);
	else if (len==1 && *the_op=='-') return subtract(num1,num2, value_ret);
	else if (len==1 && *the_op=='^') return power   (num1,num2, value_ret); 
	

	 //check for relatively simple comparisons
	if (num1->type()==VALUE_String && num2->type()==VALUE_String) {
		const char *s1=dynamic_cast<StringValue*>(num1)->str;
		const char *s2=dynamic_cast<StringValue*>(num2)->str;
		if (len==2 && !strncmp(the_op,"<=",2)) { *value_ret=new BooleanValue(strcmp(s1,s2) <= 0); return 0; }
		if (len==2 && !strncmp(the_op,">=",2)) { *value_ret=new BooleanValue(strcmp(s1,s2) >= 0); return 0; }
		if (len==2 && !strncmp(the_op,"==",2)) { *value_ret=new BooleanValue(strcmp(s1,s2) == 0); return 0; }
		if (len==2 && !strncmp(the_op,"!=",2)) { *value_ret=new BooleanValue(strcmp(s1,s2) != 0); return 0; }
		if (len==1 && *the_op=='<')            { *value_ret=new BooleanValue(strcmp(s1,s2) < 0 ); return 0; }
		if (len==1 && *the_op=='>')            { *value_ret=new BooleanValue(strcmp(s1,s2) > 0 ); return 0; }

		return -1;
	}

	v1=getNumberValue(num1, &isnum);
	if (isnum) v2=getNumberValue(num2, &isnum);
	if (!isnum) { *value_ret=NULL; return -1; } //unable to use the given arguments

	//so now num1 and num2 were resolvable to numbers, now just check for ops

	if (len==2 && !strncmp(the_op,"<=",2)) { *value_ret=new BooleanValue(v1 <= v2); return 0; }
	if (len==2 && !strncmp(the_op,">=",2)) { *value_ret=new BooleanValue(v1 >= v2); return 0; }
	if (len==2 && !strncmp(the_op,"==",2)) { *value_ret=new BooleanValue(v1 == v2); return 0; }
	if (len==2 && !strncmp(the_op,"!=",2)) { *value_ret=new BooleanValue(v1 != v2); return 0; }
	if (len==1 && *the_op=='<')            { *value_ret=new BooleanValue(v1 < v2);  return 0; }
	if (len==1 && *the_op=='>')            { *value_ret=new BooleanValue(v1 > v2);  return 0; }

	if (len==2 && !strncmp(the_op,"&&",2)) { *value_ret=new BooleanValue(v1 && v2); return 0; }
	if (len==2 && !strncmp(the_op,"||",2)) { *value_ret=new BooleanValue(v1 || v2); return 0; }

	return -1;
}

//! Add integers and reals, and concat strings.
/*! num1 and num2 must both exist. num2 is dec_count()'d, and num1 is made
 * to point to the answer.
 *
 * Return 0 on success, -1 for incompat, or 1 for error.
 */
int LaidoutCalculator::add(Value *num1,Value *num2, Value **ret)
{
	*ret=NULL;

	if (num1==NULL) { return -1; }
	if (num2==NULL) { return -1; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) { //i+i
		*ret=new IntValue(((IntValue *) num1)->i+((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Int) { //d+i
		*ret=new DoubleValue(((DoubleValue *) num1)->d+(double)((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Real) { //i+d
		*ret=new DoubleValue(((DoubleValue *) num2)->d+(double)((IntValue*)num1)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Real) { //d+d
		*ret=new DoubleValue(((DoubleValue *) num1)->d+((DoubleValue*)num2)->d);

	} else if (num1->type()==VALUE_Flatvector && num2->type()==VALUE_Flatvector) { //fv+fv
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v+((FlatvectorValue*)num2)->v);

	} else if (num1->type()==VALUE_Spacevector && num2->type()==VALUE_Spacevector) { //sv+sv
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v+((SpacevectorValue*)num2)->v);

	} else if (num1->type()==VALUE_Quaternion && num2->type()==VALUE_Quaternion) { //sv+sv
		*ret=new QuaternionValue(((QuaternionValue *) num1)->v+((QuaternionValue*)num2)->v);

	} else if (num1->type()==VALUE_String && num2->type()==VALUE_String) {
		char str[strlen(((StringValue *)num1)->str) + strlen(((StringValue *)num1)->str) + 1];
		strcpy(str,((StringValue *)num1)->str);
		strcat(str,((StringValue *)num2)->str);
		*ret=new StringValue(str);
	}

	if (*ret) return 0;
	return -1; //throw _("Cannot add those types");
}

//! Subtract integers and reals.
int LaidoutCalculator::subtract(Value *num1,Value *num2, Value **ret)
{
	*ret=NULL;

	if (num1==NULL) { return -1; }
	if (num2==NULL) { return -1; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) { //i+i
		*ret=new IntValue(((IntValue *) num1)->i-((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Int) { //d+i
		*ret=new DoubleValue(((DoubleValue *) num1)->d-(double)((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Real) { //i+d
		*ret=new DoubleValue((double)(((IntValue *) num1)->i)-((DoubleValue*)num2)->d);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Real) { //d+d
		*ret=new DoubleValue(((DoubleValue *) num1)->d-((DoubleValue*)num2)->d);

	} else if (num1->type()==VALUE_Flatvector && num2->type()==VALUE_Flatvector) { //fv+fv
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v-((FlatvectorValue*)num2)->v);

	} else if (num1->type()==VALUE_Spacevector && num2->type()==VALUE_Spacevector) { //sv+sv
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v-((SpacevectorValue*)num2)->v);

	} else if (num1->type()==VALUE_Quaternion && num2->type()==VALUE_Quaternion) { //sv+sv
		*ret=new QuaternionValue(((QuaternionValue *) num1)->v-((QuaternionValue*)num2)->v);
	}

	if (*ret) return 0;
	return -1; //throw _("Cannot subtract those types");
}

//! Multiply integers and reals.
int LaidoutCalculator::multiply(Value *num1,Value *num2, Value **ret)
{
	*ret=NULL;

	if (num1==NULL) { return -1; }
	if (num2==NULL) { return -1; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	double v1=0, v2=0;

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) { //i*i
		*ret=new IntValue(((IntValue *) num1)->i * ((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Int) { //d*i
		*ret=new DoubleValue(((DoubleValue *) num1)->d * (double)((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Real) { //i*d
		*ret=new DoubleValue(((DoubleValue *) num2)->d * (double)((IntValue*)num1)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Real) { //d*d
		*ret=new DoubleValue(((DoubleValue *) num1)->d * ((DoubleValue*)num2)->d);


	} else if (isNumberType(num1, &v1) && num2->type()==VALUE_Flatvector) { //i*v, d*v
		*ret=new FlatvectorValue(((FlatvectorValue *) num2)->v * v1);

	} else if (isNumberType(num2, &v2) && num1->type()==VALUE_Flatvector) { //v*i, v*d
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v * v2);

	} else if (num1->type()==VALUE_Flatvector && num2->type()==VALUE_Flatvector) { //v*v 2-d
		 //dot product
		*ret=new DoubleValue(((FlatvectorValue *) num1)->v * ((FlatvectorValue*)num2)->v);


	} else if (isNumberType(num1, &v1) && num2->type()==VALUE_Spacevector) { //i*v, d*v
		*ret=new SpacevectorValue(((SpacevectorValue *) num2)->v * v1);

	} else if (isNumberType(num2, &v2) && num1->type()==VALUE_Spacevector) { //v*i, v*d
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v * v2);

	} else if (num1->type()==VALUE_Spacevector && num2->type()==VALUE_Spacevector) { //v*v 3-d
		 //dot product
		*ret=new DoubleValue(((SpacevectorValue *) num1)->v * ((SpacevectorValue*)num2)->v);


	} else if (isNumberType(num1, &v1) && num2->type()==VALUE_Quaternion) { //i*v, d*v
		*ret=new QuaternionValue(((QuaternionValue *) num2)->v * v1);

	} else if (isNumberType(num2, &v2) && num1->type()==VALUE_Quaternion) { //v*i, v*d
		*ret=new QuaternionValue(((QuaternionValue *) num1)->v * v2);

	} else if (num1->type()==VALUE_Quaternion && num2->type()==VALUE_Quaternion) { //v*v (is a non-commutative quaternion, not dot)
		*ret=new QuaternionValue(((QuaternionValue *) num1)->v * ((QuaternionValue*)num2)->v);
	}

	if (*ret) return 0;
	return -1; //throw _("Cannot multiply those types");
}

int LaidoutCalculator::divide(Value *num1,Value *num2, Value **ret)
{
	*ret=NULL;

	if (num1==NULL) { return -1; }
	if (num2==NULL) { return -1; }

//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	double divisor;
	if (num2->type()==VALUE_Int)       divisor = ((IntValue*)   num2)->i;
	else if (num2->type()==VALUE_Real) divisor = ((DoubleValue*)num2)->d;
	else return -1; //throw _("Cannot divide with that type");

	if (divisor==0) {
		calcerr(_("Division by zero!"));
		return 1;
	}

	double v2;

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) { // i/i
		if (((IntValue *) num1)->i % ((IntValue*)num2)->i == 0) {
			*ret=new IntValue(((IntValue *) num1)->i / ((IntValue*)num2)->i);
		} else {
			*ret=new DoubleValue(((double)((IntValue *) num1)->i)/((IntValue*)num2)->i);
		}

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Int) { // d/i
		*ret=new DoubleValue(((DoubleValue *) num1)->d / (double)((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Real) { // i/d
		*ret=new DoubleValue((double)((IntValue*)num1)->i / ((DoubleValue *) num2)->d);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Real) { // d/d
		*ret=new DoubleValue(((DoubleValue *) num1)->d / ((DoubleValue*)num2)->d);

	} else if (isNumberType(num2, &v2) && num1->type()==VALUE_Flatvector) { // v/i, v/d
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v / v2);

	} else if (isNumberType(num2, &v2) && num1->type()==VALUE_Spacevector) { // v/i, v/d
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v / v2);

	} else if (isNumberType(num2, &v2) && num1->type()==VALUE_Quaternion) { // v/i, v/d
		*ret=new QuaternionValue(((QuaternionValue *) num1)->v / v2);
	} 

	if (*ret) return 0;
	return -1; //throw _("Cannot divide those types");
}

/*! \todo for roots that are actually integers, should maybe check for that.
 */
int LaidoutCalculator::power(Value *num1,Value *num2, Value **ret)
{
	*ret=NULL;

	if (num1==NULL) { return -1; }
	if (num2==NULL) { return -1; }


	double base, expon;

	if (num1->type()==VALUE_Int) base=(double)((IntValue*)num1)->i;
	else if (num1->type()==VALUE_Real) base=((DoubleValue*)num1)->d;
	else return -1; //throw _("Cannot raise powers with those types");

	if (num2->type()==VALUE_Int) expon=((IntValue*)num2)->i;
	else if (num2->type()==VALUE_Real) expon=((DoubleValue*)num2)->d;
	else return -1; //throw _("Cannot raise powers with those types");

	if (base==0) { calcerr(_("Cannot compute 0^x")); return 1; }

	//***this could check for double close enough to int:
	if (base<0 && num2->type()!=VALUE_Int) { calcerr(_("(-x)^(non int): Complex not allowed")); return 1; }

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int && expon>=0) {
		*ret=new IntValue((long)(pow(base,expon)+.5));

	} else {
		*ret=new DoubleValue(pow(base,expon));
	}

	if (*ret) return 0;
	return -1; //throw _("Cannot divide those types");
}



} // namespace Laidout

