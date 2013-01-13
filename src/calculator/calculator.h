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

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <lax/refptrstack.h>
#include "../styles.h"
#include "values.h"



namespace Laidout {


class LaidoutCalculator;

//----------------------------- CalcSettings -------------------------------------
class CalcSettings
{
 public:
	LaidoutCalculator *calculator; //assuming calculator will outlive this CalcSettings

     //parse state
    long from;
	long curline;
    int EvalLevel;

     //computation settings
    int CurBase;
    char decimal;
    char allow_complex;
    char allow_null_set_answers;
    double zero_threshhold; //!< Any fabs(value) less than this should be considered 0.

	CalcSettings(long pos=0,long line=0,int level=0, int base=10,char dec=1,char comp=0,char nullset=1);
	~CalcSettings();
};


//---------------------------------- OperatorLevel/OperatorFunction ------------------------------------------

class OperatorFunction
{
  public:
	char *op;
	int direction;
	int module_id;
	OpFuncEvaluator *function;

	OperatorFunction(const char *newop, int dir, int mod_id, OpFuncEvaluator *func);
	virtual ~OperatorFunction();
	virtual int isop(const char *opstr,int len);
};

class OperatorLevel 
{
  public:
	int direction; //rtol or ltor
	int priority;
	Laxkit::PtrStack<OperatorFunction> ops;

	OperatorLevel(int dir,int rank);
	int pushOp(const char *op, OperatorFunction *opfunc);
	virtual OperatorFunction *hasOp(const char *anop,int n, int dir);
	virtual ~OperatorLevel() {}
};


//----------------------------- LaidoutCalculator -------------------------------------
typedef ObjectDef CalculatorModule;

//---------------------------Entry
class Entry
{
  public:
	char *name;
	unsigned int module_id;
	Entry(const char *newname, int modid);
	virtual int type() = 0;
	virtual ~Entry();
};

//---------------------------BlockInfo
enum BlockTypes {
	BLOCK_if,
	BLOCK_for,
	BLOCK_foreach,
	BLOCK_while,
	BLOCK_namespace,
	BLOCK_object,   //!< When calling an object method, the object class instance becomes a namespace 
	BLOCK_MAX
};

class BlockInfo
{
  public:
	BlockInfo *parentscope;
	CalculatorModule *scope_namespace;
	Value *scope_object;
	Laxkit::RefPtrStack<Entry> dict; //stores potentially overloaded names, and imported names

	int type; //one of BlockTypes
	int start_of_condition; //while
	int start_of_loop;     //while, foreach, for
	int start_of_advance; //for

	 //foreach:
	char *word;
	int current,max;
	SetValue *list;

	BlockInfo();
	BlockInfo(CalculatorModule *mod, int scopetype, int loop_start, int condition_start, char *var, Value *v);
	virtual ~BlockInfo();
	virtual const char *BlockType();
	virtual int AddName(CalculatorModule *mod, ObjectDef *item);
	virtual Entry *FindName(const char *name,int len);
	virtual Entry *createNewEntry(ObjectDef *item, CalculatorModule *module);
	virtual int isSameEntry(ObjectDef *item, Entry *entry);
};

//---------------------------LaidoutCalculator
class LaidoutCalculator : public Laxkit::anObject, public OpFuncEvaluator, public FunctionEvaluator
{
 private:
	 //context state
	char *dir;
	ValueHash *build_context();

	 //evaluation stuff
	int from;
	int curline;
	char *curexprs;
	int curexprslen;

	 //settings
	CalcSettings calcsettings;

	
	Laxkit::RefPtrStack<CalculatorModule> modules;

	BlockInfo global_scope;
	Laxkit::PtrStack<BlockInfo> scopes;
	Laxkit::PtrStack<OperatorLevel> oplevels;
	OperatorLevel leftops, rightops;
	BlockInfo *currentLevel() { return scopes.e[scopes.n-1]; }

	ErrorLog *temp_errorlog;
	int calcerror;
	char *calcmes;
	Value *last_answer;
	char *messagebuffer;

	int importAllNames(CalculatorModule *module);
	int importName(CalculatorModule *module, ObjectDef *def);
	int importOperators(CalculatorModule *module);
	int removeOperators(int module_id);
	int addOperator(const char *op,int dir,int priority, int module_id, OpFuncEvaluator *opfunc);
	void InstallInnate();

	void calcerr(const char *error,const char *where=NULL,int w=0, int surround=40);
	char *getnamestring(int *n);
	const char *getopstring(int *n);
	const char *getnamestringc(int *n);
	int getnamestringlen();
	void skipwscomment();
	void skipstring();
	void skipBlock(char ch);
	int nextchar(char ch);
	int nextword(const char *word);
	void newcurexprs(const char *newex,int len);
	void atNextCommandStep() {}
	int sessioncommand();
	ObjectDef *CreateSessionCommandObjectDef();
	void pushScope(int scopetype, int loop_start=0, int condition_start=0, char *var=NULL, Value *v=NULL, ObjectDef *module=NULL);
	void popScope();

	Value *eval(const char *exprs);
	//Value *eval();
	Value *evalLevel(int level);
	int evalcondition();
	Value *checkAssignments();
	int checkBlock(Value **value_ret);
	Value *number();
	long intnumber();
	double realnumber();
	Value *getstring();
	Value *getset();
	Value *getarray();
	Value *evalname();
	Value *dereference(Value *val,ObjectDef *def);
	Entry *findNameEntry(const char *word,int len, int *scope, int *module, int *index);
	Value *evalInnate(const char *word, int len);
	int functionCall(const char *word,int n, Value **v, ObjectDef *function,ValueHash *context,ValueHash *pp);
	int add(Value *num1,Value *num2, Value **ret);
	int subtract(Value *num1,Value *num2, Value **ret);
	int multiply(Value *num1,Value *num2, Value **ret);
	int divide(Value *num1,Value *num2, Value **ret);
	int power(Value *num1,Value *num2, Value **ret);

	virtual int Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings, Value **value_ret);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret, ErrorLog *log);

	Value *evalUservar(const char *word);
	Value *ApplyDefaultSets(SetValue *set);
	void messageOut(const char *str);

	ValueHash *parseParameters(StyleDef *def);

 protected:
 public:
	LaidoutCalculator();
	virtual ~LaidoutCalculator();

	virtual int InstallModule(CalculatorModule *module, int autoimport);
	virtual int RemoveModule(const char *modulename);
	virtual int ImportModule(const char *name, int allnames);

	virtual char *In(const char *in);
	virtual int evaluate(const char *in, int len, Value **value_ret, int *error_pos, char **error_ret);

	virtual void clearerror();
	const char *Message();

	virtual int RunShell();
};


} // namespace Laidout

#endif

