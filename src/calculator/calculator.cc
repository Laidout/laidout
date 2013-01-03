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


#include <lax/fileutils.h>
#include "calculator.h"
#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../headwindow.h"
#include "../version.h"

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
    zero_threshhold=1e-30;
}

CalcSettings::~CalcSettings()
{
}



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



//---------------------------------- OperatorFunction ------------------------------------------
/*! \class OperatorFunction
 * \brief Class for easy access to one or two parameter operators. Stack of these in CalculatorModule.
 *
 * Operators are strings of non-whitespace, non-letters, non-numbers, that do not contain ';',
 * or container characters such as "'(){}[].
 *
 */

/*! \class OpFuncEvaluator
 * \brief Evaluates operators for LaidoutCalculator
 *
 * The Op() function returns 0 for number returned. -1 for no number returned due to not being
 * able to handle the provided parameters. Return 1 for parameters right type, but there
 * is an error with them somehow, and no number returned.
 */



OperatorFunction::OperatorFunction(const char *newop, int dir, int mod_id, OpFuncEvaluator *func)
{
	op=newstr(newop);
	direction=dir;
	module_id=mod_id;
	function=func;
}

OperatorFunction::~OperatorFunction()
{
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

/*! Progressively search for anop. If none found, check progressively shorter for ops.
 */
OperatorFunction *OperatorLevel::hasOp(const char *anop,int n, int dir)
{
	while (n) {
		for (int c=0; c<ops.n; c++) {
			if (!strncmp(anop,ops.e[c]->op,n) && n==(int)strlen(ops.e[c]->op)) return ops.e[c];
		}
		n--;
	}
	return NULL;
}

/*! *** currently ignores op... not quite sure why it's there! just adds opfunc.
 */
int OperatorLevel::pushOp(const char *op, OperatorFunction *opfunc)
{
	return ops.push(opfunc);
}

//--------------------------- Entry/Dictionary ------------------------------------------
/*! \class Entry
 * \brief Class to simplify name lookups for LaidoutCalculator.
 */

Entry::Entry(const char *newname, int modid)
{
	makestr(name,newname);
	module_id=modid;
}

Entry::~Entry()
{
	if (name) delete[] name;
}

//-------------------------- Dictionary
class Dictionary
{
  public:
	PtrStack<Entry> entries; //one entry for each available name
	Dictionary();
	~Dictionary() {}
};


//--------------------------- NamespaceEntry
/*! Namespaces are stored in LaidoutCalculator::modules and individual scopes.
 */
class NamespaceEntry : public Entry, public ValueHash
{
  public:
	CalculatorModule *module;
	NamespaceEntry(const char *newname, int modid, CalculatorModule *mod);
	virtual ~NamespaceEntry();

	virtual const char *GetName() { if (module) return module->name; return NULL; }
};

NamespaceEntry::NamespaceEntry(const char *newname, int modid, CalculatorModule *mod)
  : Entry(newname,modid)
{
	module=mod;
	if (mod) mod->inc_count();
	sorted=1;
}

NamespaceEntry::~NamespaceEntry()
{
	if (module) module->dec_count();
}

//--------------------------- FunctionEntry
/*! \class FunctionEntry
 * \brief Used for functions that are not part of an object.
 *
 * Object classes and functions are stored in the BlockInfo::scope_namespace elements.
 */
class FunctionEntry : public Entry
{
  public:
	ObjectDef *def; // *** //def->defval would be code, if no function evaluator?
	char *code; //usually there will be EITHER code OR function.
	FunctionEvaluator *function;
	FunctionEntry(const char *newname, int modid, const char *newcode,FunctionEvaluator *func, ObjectDef *newdef);
	virtual ~FunctionEntry();

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


//--------------------------- ValueEntry
/*! \class ValueEntry
 * \brief Used for variables.
 */
class ValueEntry : public Entry
{
  public:
	ValueEntry(const char *newname, int modid);
};

ValueEntry::ValueEntry(const char *newname, int modid)
  : Entry(newname,modid)
{}


//--------------------------- OperatorEntry
/*! \class OperatorEntry
 * \brief Entry for potentially overloaded operators.
 */
class OperatorEntry : public Entry
{
  public:
	int optype; //ltor, rtol, l, r
	Laxkit::RefPtrStack<FunctionEntry> functions; //overloading for op

	OperatorEntry(const char *newname, int modid);
	//virtual int AddFunction(FunctionEntry *func);
};

OperatorEntry::OperatorEntry(const char *newname, int modid)
  : Entry(newname,modid)
{}


//------------------------------- BlockInfo ---------------------------------------
/*! \class BlockInfo
 * \brief Scope information for LaidoutCalculator.
 */

BlockInfo::BlockInfo()
{
	scope_namespace=NULL;
	type=0;
	start_of_condition=0;
	start_of_loop=0;
	start_of_advance=0;
	current=0;
	max=0;
	word=NULL;
	list=NULL;
}

BlockInfo::BlockInfo(CalculatorModule *mod, int scopetype, int loop_start, int condition_start, char *var, Value *v)
{
	current=0;

	scope_namespace=mod;
	if (mod) mod->inc_count();
	else scope_namespace=new CalculatorModule;

	start_of_condition=condition_start;
	start_of_loop=loop_start;
	start_of_advance=0;
	type=scopetype;
	word=var;
	list=dynamic_cast<SetValue*>(v);
}

BlockInfo::~BlockInfo()
{
	if (scope_namespace) scope_namespace->inc_count();
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
	return "(unnamed block)";
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
	temp_errorlog=NULL;
	last_answer=NULL;

	global_scope.scope_namespace=new ObjectDef(NULL, "Global", _("Global"), _("Global namespace"),VALUE_Namespace,NULL,NULL);
	scopes.push(&global_scope,0); //push so as to not delete global scope

	DBG cerr <<" ~~~~~~~ New Calculator created."<<endl;


	 //things specific to Laidout:
	InstallModule(&stylemanager,1); //autoimport
}

LaidoutCalculator::~LaidoutCalculator()
{
	if (dir) delete[] dir;
	if (curexprs) delete[] curexprs;
	if (calcmes) delete[] calcmes;
	if (messagebuffer) delete[] messagebuffer;
	if (last_answer) last_answer->dec_count();
}

//! Add module to list of available modules.
/*! If autoimport, then automatically import all names in module to the global namespace.
 * Please note that operators are always made available in the global namespace.
 *
 * Return 0 for installed, or 1 for not able to install for some reason.
 */
int LaidoutCalculator::InstallModule(CalculatorModule *module, int autoimport)
{
	modules.push(module);
	if (autoimport) importAllNames(module);
	importOperators(module);
	return 0;
}

//! Import names from an existing available module. It is assumed operaters are already "imported".
/*! Return 1 for module not found, else 0.
 */
int LaidoutCalculator::ImportModule(const char *name)
{
	CalculatorModule *module=NULL;
	for (int c=0; c<modules.n; c++) {
		if (!strcmp(modules.e[c]->name,name)) { module=modules.e[c]; break; }
	}
	if (!module) return 1;
	importAllNames(module);
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
	int numl=0;
	char *temp=NULL;
	char prompt[50];
	string input;

	cout << "Laidout "<<LAIDOUT_VERSION<<" shell. Type \"quit\" to quit."<<endl;

	while (1) {
		numl++;
		
		strcpy(prompt,"\n\nInput(");
		char *temp2=itoa(numl,prompt+strlen(prompt));
		if (temp2) temp2[0]='\0';
		strcat(prompt,"): ");
	
		cout << prompt;
		if (!getline(cin, input)) break;
		if (input=="quit") return 0;

//---------readline variant--------------
//		tempexprs=readline(temp);
//
//		if (tempexprs==NULL) continue;
//		//DBG cout<<"\nreadline:"<<tempexprs;
//		delete[] temp; temp=NULL;
//		if (strlen(tempexprs)==0) { free(tempexprs); continue; }
//		add_history(tempexprs);
//
//		strncpy(exprs,tempexprs,BUFLEN-1);
//		if (strlen(tempexprs)>BUFLEN-1) exprs[BUFLEN-1]='\0';
//---------end readline variant--------------

		temp=In(input.c_str());

		cout << temp;
		delete[] temp;
	}

	return 0;
}


//! Process a command or script. Returns a new char[] with the result.
/*! Do not forget to delete[] the returned array!
 * 
 * \todo it almost goes without saying this needs automation
 * \todo it also almost goes without saying this is in major hack stage of development
 */
char *LaidoutCalculator::In(const char *in)
{
	makestr(messagebuffer,NULL);

	Value *v=NULL;
	int errorpos=0;
	evaluate(in,-1, &v, &errorpos, NULL);

	if (v) {
		appendstr(messagebuffer,v->toCChar());
		v->dec_count();
	} else if (!messagebuffer && calcerror) makestr(messagebuffer,calcmes);
	if (messagebuffer) return newstr(messagebuffer);

	if (from!=0) return newstr(_("Ok."));
	return newstr(_("You are surrounded by twisty passages, all alike."));
}

//! Process a command or script. Returns a Value object with the result, if any in value_ret.
/*! The function's return value is 0 for success, -1 for success with warnings, 1 for error in
 *  input. Note that it is possible for 0 to be returned and also have value_ret return NULL.
 *
 *  This will parse multiple expressions, and is meant to return a value. Only characters up to but
 *  not including in+len are
 *  parsed. If there are multiple expressions, then the value from the final expression is returned.
 *
 *  This function is called as necessary called by In().
 */
int LaidoutCalculator::evaluate(const char *in, int len, Value **value_ret, int *error_pos, char **error_ret)
{
	int num_expr_parsed=0;

	if (in==NULL) { calcerr(_("Blank expression")); return 1; }
	clearerror();

	Value *answer=NULL;
	if (len<0) len=strlen(in);
	newcurexprs(in,len);
	if (curexprslen>len) curexprslen=len;
	skipwscomment();

	int tfrom=-1;

	while(!calcerror && tfrom!=from && from<curexprslen) { 
		tfrom=from;
		if (answer) { answer->dec_count(); answer=NULL; }

		if (sessioncommand()) { //  checks for session commands 
			if (!messagebuffer) messageOut(_("Ok."));
			skipwscomment();
			if (from>=curexprslen) break;
			if (nextchar(';')) ; //advance past a ;
			continue;
		}

		if (calcerror) break;

		skipwscomment();
		if (checkBlock()) continue;
		if (scopes.n>1 && (nextchar('}') || nextword("break"))) {
			popScope();
			continue;
		}

		 ////check for variable assignments, simple a=b usage, no (a,b)=(1,2) yet....
		//answer=checkAssignments(); not necessary now that '=' is an operator
		if (calcerror) break;
		else if (!answer) answer=evalLevel(0);

		if (calcerror) break;

		//updatehistory(answer,outexprs);
		num_expr_parsed++;

		if (nextchar(';')) ; //advance past a ;
	} 

	if (calcerror) { 
		if (error_pos) *error_pos=calcerror;
	} 
	if (error_ret && calcmes) appendline(*error_ret,calcmes); 
	if (value_ret) { *value_ret=answer; if (answer) answer->inc_count(); }
	if (last_answer) last_answer->dec_count();
	last_answer=answer; //last_answer takes the reference

	return calcerror>0 ? 1 : 0;
}


//-------------Error parsing functions

void LaidoutCalculator::clearerror()
{
	calcerror=0;
	if (calcmes) { delete[] calcmes; calcmes=NULL; }
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

/*! def is an ObjectDef in module.
 */
int LaidoutCalculator::importName(CalculatorModule *module, ObjectDef *def)
{ // ***
	cerr << " *** need to implement importName!"<<endl;
	return 0;
}

int LaidoutCalculator::importOperators(CalculatorModule *module)
{ // ***
	cerr << " *** need to implement importOperators!"<<endl;
	return 0;
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

	if (curline>0) {
		appendstr(calcmes,"  line: ");
		char ll[20];
		sprintf(ll,"%d",curline);
		appendstr(calcmes,ll);
	}

	if (temp_errorlog) temp_errorlog->AddMessage(calcmes,ERROR_Fail, 0, from,curline);
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
	if (!isalpha(curexprs[from]) && curexprs[from]!='_') return NULL;

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
	return curexprs+from;
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
	*n=0;
	while (!isalnum(ch)
			&& ch!='#'
			&& ch!=';'
			&& ch!='"'
			&& ch!='\''
			&& ch!='.'
			&& ch!='(' && ch!=')'
			&& ch!='[' && ch!=']'
			&& ch!='{' && ch!='}') {
		(*n)++;
	}
	if (*n) return curexprs+from;
	return NULL;
}


//! Skip whitespace and "#...\n" based comments.
/*! This will not advance past curexprslen, the length of the current expression.
 */
void LaidoutCalculator::skipwscomment()
{
	do {
		while (isspace(curexprs[from]) && from<curexprslen) {
			if (curexprs[from]=='\n') curline++;
			from++;
		}
		if (curexprs[from]=='#') {
			while (curexprs[from++]!='\n' && from<curexprslen); 
		}
	} while (isspace(curexprs[from]) || curexprs[from]=='#');
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


void LaidoutCalculator::pushScope(int scopetype, int loop_start, int condition_start, char *var, Value *v, CalculatorModule *module)
{
	BlockInfo *block=new BlockInfo(module, scopetype,loop_start,condition_start,var,v);
	scopes.push(block);
}

void LaidoutCalculator::popScope()
{
	if (scopes.n==1) { calcerr(_("Unexpected end!")); return; }
	BlockInfo *scope=scopes.e[scopes.n-1];

	if (scope->type==BLOCK_if) {
		 // if closing an if block, must skip over subsequent blocks...
		while(1) {
			skipwscomment();
			int n=0;
			const char *word=getnamestringc(&n);
			if (n==4 && strncmp(word,"else",4)) break; //done if no else
			from+=4;
			skipwscomment();

			word=getnamestringc(&n);
			if (n==2 && !strncmp(word,"if",2)) {
				from+=2;
				skipwscomment();
			}
			if (!nextchar('{')) { calcerr(_("Expected '{'!")); return; }
			skipBlock('}');
		}
		
	} else if (scope->type==BLOCK_namespace) {
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
		if (calcerror) return;

		int ifso=evalcondition();
		if (calcerror) return;
		if (ifso) {
			from=scope->start_of_loop;
			scope->current++;
			return; //not done with scope. looped once!
		} else from=tfrom; //done with scope, ok to remove

	} else if (scope->type==BLOCK_foreach) {
		 //foreach: update name variable, jump back to start
		scope->current++;
		if (scope->current<scope->list->values.n) {
			from=scope->start_of_loop;
			scope->scope_namespace->SetVariable(scope->word, scope->list->values.e[scope->current],0);
			return;
		}
		//else all done with loop!

	} else if (scope->type==BLOCK_while) {
		 //while: evaluate condition, jump back to start
		int tfrom=from;
		from=scope->start_of_condition;

		int ifso=evalcondition();
		if (calcerror) return;
		if (ifso) {
			from=scope->start_of_loop;
			scope->current++;
			return; //not done with scope. looped once!
		} else from=tfrom; //done with scope, ok to remove
	}

	scopes.remove(scopes.n-1);
}


int LaidoutCalculator::evalcondition()
{
	Value *v=evalLevel(0);
	if (v->type()!=VALUE_Int && v->type()!=VALUE_Real) {
		calcerr(_("Bad condition"));
		return 0;
	}
	int ifso=0;
	if (v->type()==VALUE_Int) ifso=(dynamic_cast<IntValue*>(v)->i==0?0:1);
	else ifso=(dynamic_cast<DoubleValue*>(v)->d==0?0:1);
	v->dec_count();
	return ifso;
}

//! Assuming just after an opening of ch, skip to after a ch.
/*! ch should be one of: )}].
 */
void LaidoutCalculator::skipBlock(char ch)
{
	char *str;
	int tfrom=-1;
	while (from!=tfrom && from<curexprslen) {
		str=strpbrk(curexprs+from, "\"'({[]})#"); //first occurence of any of these
		if (!str) calcerr(_("Missing end!"));

		if (*str==ch) { from=str-curexprs+1; return; } //found the end!!
		if (*str==')' || *str=='}' || *str==']') {
			 //if it was a closing like ch, it was dealt with above
			calcerr(_("Unexpected end!"));
			return;
		}

		if (*str=='#') { from=str-curexprs; skipwscomment(); continue; }
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

int LaidoutCalculator::checkBlock()
{
	if (!isalpha(curexprs[from])) return 0;

	int n=0;
	const char *word=getnamestringc(&n);
	if (!word) return 0;

	if (!strncmp(word,"if",2)) {
		 //if (condition) { ... } else if { ... }
		from+=5;
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
				if (!strncmp(word,"if",2)) return checkBlock();
				if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }
				pushScope(BLOCK_if);
				return 1; //start over from the else
			}
		} 
		return 1;

	} else if (!strncmp(word,"foreach",7)) {
		 //foreach name in set|array|hash { ... }
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
		if (v->type()!=VALUE_Set && v->type()!=VALUE_Array) {
			calcerr(_("Expected set, array, or hash!"));
			return 1;
		}
		if (!nextchar('{')) { calcerr(_("Expected '{'!")); return 1; }
		int start_of_block=from;

		if (dynamic_cast<SetValue*>(v) && dynamic_cast<SetValue*>(v)->values.n==0) {
			 //no actual values, so skip!
			skipBlock('}');
			return 1;
		}

		pushScope(BLOCK_foreach, start_of_block, 0, var, v);
		return 1;


	} else if (!strncmp(word,"for",3)) {
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

	} else if (!strncmp(word,"while",5)) {
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

	} else if (!strncmp(word,"namespace",9)) {
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
			def=new ObjectDef(NULL, str,str,NULL,VALUE_Namespace,NULL,NULL);
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

//--------------------------- LValue
/*! \class LValue
 * Hold something that can take an assignment if necessary.
 */
class LValue : public Value
{
  public:
	char *name;
	Value *basevalue;
	FieldPlace extension;

	LValue(const char *newname,Value *v, FieldPlace &p);
	virtual ~LValue();
	virtual int type() { return VALUE_LValue; }
};

LValue::LValue(const char *newname,Value *v, FieldPlace &p)
{
	basevalue=v;
	v->inc_count();
	extension=p;
	name=newstr(newname);
}

LValue::~LValue()
{
	if (basevalue) basevalue->dec_count();
	if (name) delete[] name;
}

//--------------------------- end LValue

/*! \todo need much more sophisticated assignments like flatvector v; v.x=3;
 *     and (a,b)=(1,2)
 *
 *  Return 0 for no assignments.
 */
Value *LaidoutCalculator::checkAssignments()
{ // ***
	return NULL;

//	int namelen=getnamestringlen();
//	if (!namelen) return NULL;
//
//	int ttfrom=from;
//	from+=namelen;
//	skipwscomment();
//
//	if (!nextchar('=')) {
//		from=ttfrom;
//		return NULL;
//	}
//
//	 //we will be assigning something to a variable
//	int assign=uservars.findIndex(curexprs+ttfrom, namelen);
//
//	Value *v=evalLevel(0);
//	if (calcerror) return NULL;
//
//	if (!v) { calcerr(_("Expected a value!")); return NULL; }
//
//	if (assign<0) {
//		char *assignname=NULL;
//		assignname=newnstr(curexprs+ttfrom, namelen);
//		uservars.push(assignname, v);
//		delete[] assignname;
//	} else {
//		uservars.set(assign, v);
//	}
//
//	return v;
}

Value *LaidoutCalculator::evalUservar(const char *word)
{ // ***
	return NULL;

	//Value *v=uservars.find(word);
	//if (v) v=v->duplicate();
	//return v;
}

//! Replace curexprs with newex, and set from=0.
void LaidoutCalculator::newcurexprs(const char *newex,int len)
{
	makenstr(curexprs,newex,len);
	curexprslen=strlen(curexprs);
	from=0;
	curline=0;
}

ObjectDef *LaidoutCalculator::CreateSessionCommandObjectDef()
{
	ObjectDef *def=new ObjectDef(NULL,"sessioncommand", _("Session Commands"), _("Built in session commands"),
								 VALUE_Namespace, NULL,NULL);

	def->pushFunction("show", _("Show"), _("Give information about something"), NULL, NULL);
	def->pushFunction("about",_("About"),_("Show version information"), NULL, NULL);
	def->pushFunction("unset",_("Unset"),_("Remove a name from the current namespace"), NULL, NULL);
	def->pushFunction("help", _("Help"), _("Show a quick help"), NULL, NULL);
	def->pushFunction("?",    _("Help"), _("Show a quick help"), NULL, NULL);
	def->pushFunction("quit", _("Quit"), _("Quit"), NULL, NULL);

	return def;
}

//! Parse any commands that change internal calculator settings like radians versus decimal.
/*! Set decimal==1 if 'degrees', set to 0 if 'radians'
 */
int LaidoutCalculator::sessioncommand() //  done before eval
{
	if (nextword("?") || nextword("help")) {
		messageOut(_("The very basic commands are:"));

		 //***todo: this should probably be a dump of session commands based
		 //      on an ObjectDef.. won't get out of sync that way... well, maybe...?
		 //show very base built in
		messageOut(	  " show [object type or function name]\n"
					  " about\n"
					  " import\n"
					  //" radians\n"
					  //" degrees\n"
					  " unset\n"
					  " help\n"
					  " quit\n"
					  " ?");
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

	 //these things define ObjectDef objects, and have special meta read in modes.
	 //It might be nice to be able to add meta to any object
	//if (nextword("function")) ***
	//if (nextword("var")) ***
	//if (nextword("operator")) ***
	//if (nextword("var")) ***
	//if (nextword("namespace")) ***

	if (nextword("radians"))
	    { calcsettings.decimal=0; from+=7; return 1; }
	
	if (nextword("degrees"))
	    { calcsettings.decimal=1; from+=7; return 1; }

	if (nextword("import")) {
		 //take names from another namespace and make them immediately accessible in the current scope
		 //use unset to remove imported names from current scope
		from+=6;
		int n=0;
		const char *name=getnamestringc(&n);
		if (isblank(name)) { calcerr(_("Expecting name!")); return 1; }
		from+=n;
		skipwscomment();
		ImportModule(name);
		return 1;
	}

	if (nextword("unset")) {
//		***
//		from+=5;
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
		skipwscomment();
		int ch=0;
		while (!isspace(curexprs[ch]) && curexprs[ch]!=';' && curexprs[ch]!='#') ch++;
		char *showwhat=newnstr(curexprs+from,ch);
		char *temp=NULL;
		
		ObjectDef *sd=NULL;
		if (showwhat) { //show a particular item
			int numfound=0;
			for (int c=scopes.n-1; c>=0; c++) {
				if (scopes.e[c]->scope_namespace) sd=scopes.e[c]->scope_namespace->FindDef(showwhat);
				if (!sd) continue;
				numfound++;

				//*** maybe do this: it's easier
				//Attribute att
				//sd->dump_out_atts(&att, DEFOUT_HumanSummary,NULL);
				//appendstr(temp,att.value);
	
				appendstr(temp,element_TypeNames(sd->format));
				appendstr(temp," ");
				appendstr(temp,sd->name);
				//appendstr(temp,": ");
				//appendstr(temp,sd->Name);
				appendstr(temp,", ");
				appendstr(temp,sd->description);
				if ((sd->format==VALUE_Class || sd->format==VALUE_Fields) && sd->extendsdefs.n) {
					appendstr(temp,", extends: ");
					for (int c2=0; c2<sd->extendsdefs.n; c2++) {
						appendstr(temp,sd->extendsdefs.e[c2]->name);
					}
				}

				if ((sd->format==VALUE_Fields || sd->format==VALUE_Function) && sd->getNumFields()) {
					const char *nm,*Nm,*desc,*rng;
					ValueTypes fmt;
					ObjectDef *subdef=NULL;
					appendstr(temp,"\n");
					for (int c2=0; c2<sd->getNumFields(); c2++) {
						sd->getInfo(c2,&nm,&Nm,&desc,&rng,NULL,&fmt,NULL,&subdef);
						appendstr(temp,"  ");
						appendstr(temp,nm);
						appendstr(temp,": (");
						appendstr(temp,element_TypeNames(fmt));
						if (fmt==VALUE_Set) {
							if (!isblank(rng)) {
								appendstr(temp," of ");
								appendstr(temp,rng);
							}
						}
						appendstr(temp,") ");
						appendstr(temp,Nm);
						appendstr(temp,", ");
						appendstr(temp,desc);
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
								appendstr(temp,", ");
								appendstr(temp,desc);
								appendstr(temp,"\n");
							}
						}
					}
				} //if fields or function
			} //if foreach scope

			if (!numfound) {
				 //check for match of module names, just in case
				for (int c=0; c<modules.n; c++) {
					if (!strcmp(modules.e[c]->name, showwhat)) {
						appendstr(temp,"module ");
						appendstr(temp,showwhat);
						numfound++;
					}
				}
			}

			if (!numfound) { appendline(temp,_("Unknown name!")); }

			from+=strlen(showwhat);
			delete[] showwhat; showwhat=NULL;

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


			 //operator dump
			// ***
			appendstr(temp,"Operators: ...need to implement operator dump\n");


			 //scope dump
			if (scopes.n>1) {
				appendstr(temp, "Scopes:\n");
				for (int c=1; c<scopes.n; c++) {
					for (int c2=0; c2<c; c2++) appendstr(temp,"  ");
					appendstr(temp, scopes.e[c]->BlockType());
					appendstr(temp,"\n");
				}
			}

			for (int c=0; c<scopes.n; c++) {
				if (c==0) appendstr(temp,"Global\n");
				else {
					appendstr(temp,"scope ");
					appendstr(temp, scopes.e[c]->BlockType());
					appendstr(temp,"\n");
				}

				ObjectDef *def=scopes.e[c]->scope_namespace;

				 //Show object definitions in scope
				if (def->getNumFields()) {
					appendstr(temp,_("\nObject Definitions:\n"));

					ObjectDef *def=NULL;
					for (int c=0; c<def->getNumFields(); c++) {
						def->findActualDef(c,&def);
						if (!def || (def->format!=VALUE_Class && def->format!=VALUE_Fields)) continue;

						appendstr(temp,"  ");
						appendstr(temp,def->name);
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
					for (int c=0; c<def->getNumFields(); c++) {
						def->findActualDef(c,&deff);
						if (!deff || (deff->format!=VALUE_Variable)) continue;

						appendstr(temp,"  ");
						appendstr(temp,deff->name);
						appendstr(temp," = ");
						if (deff->defaultvalue) appendstr(temp,deff->defaultvalue);
						else if (deff->defaultValue) appendstr(temp,deff->defaultValue->CChar());
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
	Value *num=NULL, *num2=NULL, *num_ret;
	if (level>=oplevels.n) {
		 //all done with 2 number ops, now check left ops, read number, right ops
		op=getopstring(&n);
		if (n) { //found an op string
			opfunc=leftops.hasOp(op,n,OPS_Left);
			if (!opfunc) {
				 //if left hand op.. if not, is error
				calcerr(_("Unexpected characters!"));
				return NULL;
			}

			from+=n;
			num=evalLevel(level); //will plow through any other left hand ops
			// *** still need to apply the op
			if (calcerror) { if (num) num->dec_count(); return NULL; }

		} else {
			num=number(); //get a plain number
		}
		if (!num) return NULL; //did not find a number
		 
		 //check for right hand ops
		op=getopstring(&n);
		while (n) {
			 //found an op string
			opfunc=rightops.hasOp(op,n,OPS_Right);
			if (!opfunc) return num; //no right ops, so done!

			Value *num_ret=NULL;
			CalcSettings settings=calcsettings;
			settings.calculator=this;
			int status=opfunc->function->Op(op,n,OPS_Right, num,NULL, &settings, &num_ret);
			if (status==-1) {
				// *** need to do operator overloading
			}
			if (calcerror) { if (num) num->dec_count(); return NULL; }

			from+=n;
			op=getopstring(&n);
		}
		return num;
	}

	//left and right ops out of the way, now for 2 number ops
	int dir=oplevels.e[level]->direction;
	num=evalLevel(level+1);
	if (calcerror) return NULL;

	op=getopstring(&n);
	while (n) {
		opfunc=oplevels.e[level]->hasOp(op,n,dir);
		if (!opfunc) break;

		if (dir==OPS_LtoR) num2=evalLevel(level+1);
		else num2=evalLevel(level);
		if (calcerror) { num->dec_count(); return NULL; }

		CalcSettings settings=calcsettings;
		settings.calculator=this;
		int status=opfunc->function->Op(op,n,dir, num,num2, &settings, &num_ret);
		if (status==-1) {
			 // *** need to implement overloading!!!
		}
		num->dec_count();  num=NULL;
		num2->dec_count(); num2=NULL;
		if (calcerror) {
			return NULL;
		}

		num=num_ret;
		op=getopstring(&n);
	}

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

	} else if (isalpha(curexprs[from]) || curexprs[from]=='_') {
		 //  is not a simple number, maybe function, objectdef, cast, variable, namespace
		snum=evalname();

		if (calcerror) { if (snum) delete snum; return NULL; }
		if (!snum) return NULL; 
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

//! Read in values for a set.
/*! Next char must be '(' or '{'. Otherwise return NULL.
 * If '(', then ApplyDefaultSets() is used to convert something like (2,3) to a flatvector.
 */
Value *LaidoutCalculator::getset()
{
	char setchar=0;
	if (nextchar('{')) setchar='}';
	else if (nextchar('(')) setchar=')';
	else return NULL;

	SetValue *set=new SetValue;
	do {
		Value *num=evalLevel(0);
		if (calcerror) {
			return NULL;
		}
		set->Push(num);
		num->dec_count();
		
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

Value *LaidoutCalculator::getarray()
{
	if (!nextchar('[')) return NULL;

	ArrayValue *array=new ArrayValue;
	do {
		Value *num=evalLevel(0);
		if (calcerror) {
			return NULL;
		}
		array->Push(num);
		num->dec_count();
		
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
		Value *v=set->values.e[0]->duplicate();
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

Entry *LaidoutCalculator::findNameEntry(const char *word,int len, int *scope, int *module)
{
	 // ***
	return NULL;
}

/*! from points to beginning of word.
 * If name not found, then from will still point to beginning of word.
 */
Value *LaidoutCalculator::evalname()
{ // ***
	 //search for function name
	int n=0;
	const char *word=getnamestringc(&n);
	if (!word) return NULL;

	// ***todo? if a function takes one string input, then send everything from [in..end) as the string
	//    this lets you have "command aeuaoe aoeu aou;" where the function does its own parsing.
	//    In this case, the function parameters will have 1 parameter: "commanddata",value=[in..end).
	//    otherwise, assume nested parsing like "function (x,y)", "function x,y"

	 //search for function in stylemanager
	int scope=-1;
	int module=-1;
	Entry *entry=findNameEntry(word,n, &scope, &module);
	if (!entry) return NULL;


	if (dynamic_cast<FunctionEntry*>(entry)) {
		ObjectDef *function=dynamic_cast<FunctionEntry*>(entry)->def;

		 //we found a function!
		from+=n;
		skipwscomment();

		 //build context and find parameters
		//int error_pos=0;
		ValueHash *context=build_context(); //build calculator context
		ValueHash *pp=parseParameters(function); //build parameter hash in order of styledef
		if (calcerror) {
			delete context;
			return NULL;
		}

		 //call the actual function
		ErrorLog log;
		Value *value=NULL;
		int status=0;
		if (!calcerror) {
			if (function->stylefunc) status=function->stylefunc(context,pp, &value,log);
			else if (function->newfunc) {
				Value *s=function->newfunc(function);
				if (s) {
					status=0;
					value=new ObjectValue(s);
				} else status=1;
			}

			 //report default success or failure
			if (log.Total()) {
				char *error=NULL;
				char scratch[100];
				ErrorLogNode *e;
				for (int c=0; c<log.Total(); c++) {                 
					e=log.Message(c);
					if (e->severity==ERROR_Ok) ;
					else if (e->severity==ERROR_Warning) appendstr(error,"Warning: ");
					else if (e->severity==ERROR_Fail) appendstr(error,"Error! ");
				
					if (e->objectstr_id) {
						sprintf(scratch,"id:%s, ",e->objectstr_id);
						appendstr(error,scratch);
					}
					appendstr(error,e->description);
					appendstr(error,"\n");
				}
				messageOut(error);
				delete[] error;
			} else {
				if (status>0) calcerr(_("Command failed, tell the devs to properly document failure!!"));
				else if (status<0) messageOut(_("Command succeeded with warnings, tell the devs to properly document warnings!!"));
			}
		}
		if (pp) delete pp;
		if (context) delete context;
		if (calcerror) { if (value) value->dec_count(); value=NULL; }
		return value;
	}

	 //search for innate functions
	Value *num=evalInnate(word,n);
	if (calcerror || num) {
		return num;
	}
	
	 //search for user variables
	num=evalUservar(word);
	if (calcerror || num) {
		return num;
	}

	calcerr(_("Unknown name!"));
	return NULL;
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

int LaidoutCalculator::addOperator(const char *op,int dir,int priority, int module_id, OpFuncEvaluator *opfunc)
{
	OperatorFunction *opf=new OperatorFunction(op,dir, module_id, opfunc);

	if (dir==OPS_Left) leftops.pushOp(op, opf);
	else if (dir==OPS_Right) rightops.pushOp(op, opf);
	else {
		int c;
		for (c=0; c<oplevels.n; c++) {
			if (priority<oplevels.e[c]->priority) {
				OperatorLevel *level=new OperatorLevel(dir,priority);
				oplevels.push(level,1,c); //insert new level before this level
				level->pushOp(op,opf);
				break;
			}
			if (priority==oplevels.e[c]->priority && oplevels.e[c]->direction==dir) {
				oplevels.e[c]->ops.push(opf);
				break; //found an existing level
			}
		}
		if (c==oplevels.n) {
			 //adding level at end
			OperatorLevel *level=new OperatorLevel(dir,priority);
			oplevels.push(level,1,c); //insert new level before this level
		}
	}
	return 0;
}

//--------------------- ***
class InnateFunctions : public ObjectDef, public OpFuncEvaluator, public FunctionEvaluator
{
  public:
	InnateFunctions() {}
	virtual ~InnateFunctions() {}
	virtual int Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings, Value **value_ret);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log);
};

int InnateFunctions::Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings, Value **value_ret)
{ //***
	return 1;
}

int InnateFunctions::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret,
						 ErrorLog *log)
{ // ***
	return 1;
}


void LaidoutCalculator::installInnate()
{
	InnateFunctions *innates=new InnateFunctions;

	 //operators
	addOperator("||",OPS_LtoR,100, 0,innates);

	addOperator("&&",OPS_LtoR,200, 0,innates);

	addOperator("<=",OPS_LtoR,300, 0,innates);
	addOperator("<", OPS_LtoR,300, 0,innates);
	addOperator(">=",OPS_LtoR,300, 0,innates);
	addOperator(">", OPS_LtoR,300, 0,innates);
	addOperator("==",OPS_LtoR,300, 0,innates);
	addOperator("!=",OPS_LtoR,300, 0,innates);

	addOperator("+", OPS_LtoR,400, 0,innates);
	addOperator("-", OPS_LtoR,400, 0,innates);

	addOperator("*", OPS_LtoR,500, 0,innates);
	addOperator("/", OPS_LtoR,500, 0,innates);

	addOperator("^", OPS_RtoL,600, 0,innates);

	addOperator("+", OPS_Left,0,   0,innates);
	addOperator("-", OPS_Left,0,   0,innates);


	 //variables
    innates->pushVariable("pi", "pi",      _("pi"),                        new DoubleValue(M_PI),1);
    innates->pushVariable("tau","tau",     _("golden section"),            new DoubleValue((1+sqrt(5))/2),1);
    innates->pushVariable("e",  "e",       _("base of natural logarithm"), new DoubleValue(exp(1)),1);
    //innates->define("i",  "sqrt(-1)",_("imaginary number"),          0);


	 //functions
    innates->pushFunction("atan2",    NULL,_("arctangent2"),                            NULL,NULL); //innates, NULL); // "x:int|real,y:int|real");
    innates->pushFunction("sin",      NULL,_("sine"),                                   NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("asin",     NULL,_("arcsine"),                                NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("cos",      NULL,_("cosine"),                                 NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("acos",     NULL,_("arccosine"),                              NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("tan",      NULL,_("tangent"),                                NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("atan",     NULL,_("arctangent"),                             NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("abs",      NULL,_("absolute value"),                         NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("sqrt",     NULL,_("square root"),                            NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("log",      NULL,_("base 10 logarithm"),                      NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("ln",       NULL,_("natural logarithm"),                      NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("exp",      NULL,_("exponential"),                            NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("cosh",     NULL,_("hyperbolic cosine"),                      NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("sinh",     NULL,_("hyperbolic sine"),                        NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("tanh",     NULL,_("hyperbolic tangent"),                     NULL,NULL); //innates, NULL); // "int|real|complex");
    innates->pushFunction("sgn",      NULL,_("pos,neg,or 0"),                           NULL,NULL); //innates, NULL); // "int|real");
    innates->pushFunction("int",      NULL,_("integer of"),                             NULL,NULL); //innates, NULL); // "int|real");
    innates->pushFunction("gint",     NULL,_("greatest integer less than"),             NULL,NULL); //innates, NULL); // "int|real");
    innates->pushFunction("lint",     NULL,_("least integer greater than"),             NULL,NULL); //innates, NULL); // "int|real");
    innates->pushFunction("factor",   NULL,_("set of factors of an integer"),           NULL,NULL); //innates, NULL); // "int");
    innates->pushFunction("det",      NULL,_("determinant of a square array"),          NULL,NULL); //innates, NULL); // "array");
    innates->pushFunction("transpose",NULL,_("transpose of an array"),                  NULL,NULL); //innates, NULL); // "array");
    innates->pushFunction("inverse",  NULL,_("inverse of square array"),                NULL,NULL); //innates, NULL); // "array");
    innates->pushFunction("random",   NULL,_("Return a random number from 0 to 1"),     NULL,NULL); //innates, NULL);
    innates->pushFunction("randomint",NULL,_("Return a random integer from min to max"),NULL,NULL); //innates, NULL); // "min:int, max:int");

}

//! parse and compute standard math functions like sqrt(x), sin(x), etc.
/*! Also a couple math numbers like e, pi, and tau (golden ratio).
 *
 * Another function is factor(i) which returns a string with the factors of i.
 */
Value *LaidoutCalculator::evalInnate(const char *word, int len)
{
	 //math constants
	if (!strcmp(word,"pi")) { from+=len; return new DoubleValue(M_PI); }
	if (!strcmp(word,"e")) { from+=len; return new DoubleValue(exp(1)); }
	if (!strcmp(word,"tau")) { from+=len; return new DoubleValue((1+sqrt(5))/2); }

	if (!strcmp(word,"random")) { from+=len; return new DoubleValue(random()/(double)RAND_MAX); }

	int tfrom=from;
	from+=len;
	ValueHash *pp=parseParameters(NULL);
	if (!pp || calcerror) return NULL;
	Value *v=NULL;

	try {
		v=pp->value(0);
		double d;
		if (v->type()==VALUE_Int) d=((IntValue*)v)->i;
		else if (v->type()==VALUE_Real) d=((DoubleValue*)v)->d;
		else throw 1;

		if (pp->n()==2) {
			double d2;
			v=pp->value(1);
			if (v->type()==VALUE_Int) d2=((IntValue*)v)->i;
			else if (v->type()==VALUE_Real) d2=((DoubleValue*)v)->d;
			else { v=NULL; throw 1; }

			if (!strcmp(word,"atan2")) {
				d=atan2(d,d2);
				if (calcsettings.decimal) d*=180/M_PI;
				v=new DoubleValue(d);

			} else if (!strcmp(word,"randomint")) {
				int min=int(d);
				int max=int(d2);
				return new IntValue(min+(max-min)*random()/(double)RAND_MAX);
			}
			else { v=NULL; throw -1; }

		} else if (pp->n()==1) {
			if (!strcmp(word,"sin"))  { if (calcsettings.decimal) d*=M_PI/180; v=new DoubleValue(sin(d)); }
			else if (!strcmp(word,"asin")) { if (d<-1. || d>1.) throw 3; d=asin(d); if (calcsettings.decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else if (!strcmp(word,"cos"))  { if (calcsettings.decimal) d*=M_PI/180; v=new DoubleValue(cos(d)); }
			else if (!strcmp(word,"acos")) { if (d<-1. || d>1.) throw 3; d=acos(d); if (calcsettings.decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else if (!strcmp(word,"tan"))  { if (calcsettings.decimal) d*=M_PI/180; v=new DoubleValue(tan(d)); }
			else if (!strcmp(word,"atan")) { v=new DoubleValue(atan(d)); }

			else if (!strcmp(word,"abs"))  { 
				if (v->type()==VALUE_Int) v=new IntValue(abs(((IntValue*)v)->i));
				else v=new DoubleValue(fabs(d)); 
			}
			else if (!strcmp(word,"sgn"))  { v=new IntValue(d>0?1:(d<0?-1:0)); }
			else if (!strcmp(word,"int"))  { v=new IntValue(int(d)); }
			else if (!strcmp(word,"gint")) { v=new IntValue(floor(d)); }
			else if (!strcmp(word,"floor")) { v=new IntValue(floor(d)); }
			else if (!strcmp(word,"lint")) { v=new IntValue(ceil(d)); }
			else if (!strcmp(word,"ceiling")) { v=new IntValue(ceil(d)); }
			else if (!strcmp(word,"exp"))  { v=new DoubleValue(exp(d)); }
			else if (!strcmp(word,"cosh")) { v=new DoubleValue(cosh(d)); }
			else if (!strcmp(word,"sinh")) { v=new DoubleValue(sinh(d)); }
			else if (!strcmp(word,"tanh")) { v=new DoubleValue(tanh(d)); }

			else if (!strcmp(word,"sqrt")) { if (d<0) throw 2; v=new DoubleValue(sqrt(d)); }

			else if (!strcmp(word,"log"))  { if (d<=0) throw 4; v=new DoubleValue(log(d)/log(10)); }
			else if (!strcmp(word,"ln"))   { if (d<=0) throw 4; v=new DoubleValue(log(d)); } // d must be >0

			else if (!strcmp(word,"factor")) {
				if (v->type()!=VALUE_Int) throw 1;
				long n=((IntValue*)v)->i;
				char *str=NULL, temp[20];
				
				double c=2;
				if (n==0) appendstr(str,"0");
				else if (n<0) {
					n=-n;
					appendstr(str,"-1");
				} 
				while (n>1) {
					if (n-c*(int(n/c))==0) {
						if (str) appendstr(str,",");
						sprintf(temp,"%d",int(c));
						appendstr(str,temp);
						n/=c;
						c=2;
					} else c++;
				}
				v=new StringValue(str);
				delete[] str;
			} else throw -1;

		} else { v=NULL; throw -1; }
	} catch (int e) {
		if (e==-1) from=tfrom;
		else if (e==1) calcerr(_("Cannot compute with that type"));
		else if (e==2) calcerr(_("Parameter must be greater than or equal to 0"));
		else if (e==3) calcerr(_("Parameter must be in range [-1,1]"));
		else if (e==4) calcerr(_("Parameter must be greater than 0"));
		else calcerr(_("Can't do that math"));
		v=NULL;
	}

	if (pp) delete pp;
	return v;
}

//! This will be for status messages during the execution of a script.
/*! \todo *** implement this!!
 *
 * This function exists to aid in providing incremental messages to another thread. This is
 * so you can run a script basically in the backgroud, and give status reports to the user,
 * who might want to halt the script based on what's happening.
 */
void LaidoutCalculator::messageOut(const char *str)
{//***
	DBG cerr <<"*** must implement LaidoutCalculator::messageOut() properly! "<<str<<endl;

	appendline(messagebuffer,str);
}


//---------------------built in ops:

double getNumberValue(Value *v, int *isnum)
{
	if (v->type()==VALUE_Real) {
		*isnum=1;
		return dynamic_cast<DoubleValue*>(v)->d;
	} else if (v->type()==VALUE_Int) {
		*isnum=1;
		return dynamic_cast<IntValue*>(v)->i;
	} else if (v->type()==VALUE_Boolean) {
		*isnum=1;
		return dynamic_cast<BooleanValue*>(v)->i;
	}
	*isnum=0;
	return 0;
}

//! Figure out and evaluate built in operators
int LaidoutCalculator::Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings, Value **value_ret)
{
	if (len==1 && !strncmp(the_op,"+",1)) {
		return add(num1,num2, value_ret);
	}

	int isnum=0;
	double v1,v2;
	//num*num, fv*fv, v*v, matrix*matrix


	if (num2==NULL) { //is left hand op
		if (num2==NULL && len==1 && *the_op=='+') ; //***return num1;
		if (num2==NULL && len==1 && *the_op=='-') ; //***return -num1;
		calcerr("INCOMPLETE CODING!!");
		return 0;
	}

	if (num1==NULL) { //is right hand op
		if (num2==NULL && len==2 && !strncmp(the_op,"++",2)) ; //***return num1++;
		if (num2==NULL && len==2 && !strncmp(the_op,"--",2)) ; //***return num1--;
		calcerr("INCOMPLETE CODING!!");
		return 0;
	}

	if (len==1 && *the_op=='*')      return multiply(num1,num2, value_ret);
	else if (len==1 && *the_op=='/') return divide  (num1,num2, value_ret);
	else if (len==1 && *the_op=='+') return add     (num1,num2, value_ret);
	else if (len==1 && *the_op=='-') return subtract(num1,num2, value_ret);
	else if (len==1 && *the_op=='^') return power   (num1,num2, value_ret); 
	

	 //check for relatively simple comparisons
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

	if (len==1 && *the_op=='=') {
		//*** is an assignment, needs num1 to be an LValue, num2 cannot be an LValue. Returns an LValue
	}

	return 1;
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
		*ret=new DoubleValue(((DoubleValue *) num2)->d-(double)((IntValue*)num1)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Real) { //d+d
		*ret=new DoubleValue(((DoubleValue *) num1)->d-((DoubleValue*)num2)->d);

	} else if (num1->type()==VALUE_Flatvector && num2->type()==VALUE_Flatvector) { //fv+fv
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v-((FlatvectorValue*)num2)->v);

	} else if (num1->type()==VALUE_Spacevector && num2->type()==VALUE_Spacevector) { //sv+sv
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v-((SpacevectorValue*)num2)->v);
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

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) { //i+i
		*ret=new IntValue(((IntValue *) num1)->i * ((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Int) { //d+i
		*ret=new DoubleValue(((DoubleValue *) num1)->d * (double)((IntValue*)num2)->i);

	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Real) { //i+d
		*ret=new DoubleValue(((DoubleValue *) num2)->d * (double)((IntValue*)num1)->i);

	} else if (num1->type()==VALUE_Real && num2->type()==VALUE_Real) { //d+d
		*ret=new DoubleValue(((DoubleValue *) num1)->d * ((DoubleValue*)num2)->d);


	} else if ((num1->type()==VALUE_Real || num1->type()==VALUE_Int) && num2->type()==VALUE_Flatvector) { //i*v
		*ret=new FlatvectorValue(((FlatvectorValue *) num2)->v * ((IntValue*)num1)->i);

	} else if ((num1->type()==VALUE_Real || num1->type()==VALUE_Int) && num2->type()==VALUE_Flatvector) { //d*v
		*ret=new FlatvectorValue(((FlatvectorValue *) num2)->v * ((DoubleValue*)num1)->d);

	} else if ((num2->type()==VALUE_Real || num2->type()==VALUE_Int) && num1->type()==VALUE_Flatvector) { //v*i
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v * ((IntValue*)num2)->i);

	} else if ((num2->type()==VALUE_Real || num2->type()==VALUE_Int) && num1->type()==VALUE_Flatvector) { //v*d
		*ret=new FlatvectorValue(((FlatvectorValue *) num1)->v * ((DoubleValue*)num2)->d);

	} else if (num1->type()==VALUE_Flatvector && num2->type()==VALUE_Flatvector) { //v*v 2-d
		 //dot product
		*ret=new DoubleValue(((FlatvectorValue *) num1)->v * ((FlatvectorValue*)num2)->v);


	} else if ((num1->type()==VALUE_Real || num1->type()==VALUE_Int) && num2->type()==VALUE_Spacevector) { //i*v
		*ret=new SpacevectorValue(((SpacevectorValue *) num2)->v * ((IntValue*)num1)->i);

	} else if ((num1->type()==VALUE_Real || num1->type()==VALUE_Int) && num2->type()==VALUE_Spacevector) { //d*v
		*ret=new SpacevectorValue(((SpacevectorValue *) num2)->v * ((DoubleValue*)num1)->d);

	} else if ((num2->type()==VALUE_Real || num2->type()==VALUE_Int) && num1->type()==VALUE_Spacevector) { //v*i
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v * ((IntValue*)num2)->i);

	} else if ((num2->type()==VALUE_Real || num2->type()==VALUE_Int) && num1->type()==VALUE_Spacevector) { //v*d
		*ret=new SpacevectorValue(((SpacevectorValue *) num1)->v * ((DoubleValue*)num2)->d);

	} else if (num1->type()==VALUE_Spacevector && num2->type()==VALUE_Spacevector) { //v*v 3-d
		 //dot product
		*ret=new DoubleValue(((SpacevectorValue *) num1)->v * ((SpacevectorValue*)num2)->v);
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
	if (num2->type()==VALUE_Int) divisor=((IntValue*)num2)->i;
	else if (num2->type()==VALUE_Real) divisor=((DoubleValue*)num2)->d;
	else return -1; //throw _("Cannot divide with that type");

	if (divisor==0) throw _("Division by zero");

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


} // namespace Laidout

