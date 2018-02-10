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

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <lax/refptrstack.h>
#include "interpreter.h"
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
	int module_id; //id of source ObjectDef (parent of def)
	ObjectDef *def; //def of this op
	OpFuncEvaluator *function;

	OperatorFunction(const char *newop, int dir, int mod_id, OpFuncEvaluator *func,ObjectDef *opdef);
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
	int pushOp(const char *op,int dir, OpFuncEvaluator *opfunc,ObjectDef *def, int module_id);
	virtual OperatorFunction *hasOp(const char *anop,int n, int dir, int *index, int afterthis);
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
	virtual ObjectDef *GetDef();
};

//---------------------------BlockInfo
enum BlockTypes {
	BLOCK_error,
	BLOCK_none,
	BLOCK_if,
	BLOCK_for,
	BLOCK_foreach,
	BLOCK_while,
	BLOCK_namespace,
	BLOCK_class,
	BLOCK_function,
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

	BlockTypes type; //one of BlockTypes
	int start_of_condition; //while
	int start_of_loop;     //while, foreach, for
	int start_of_advance; //for

	 //foreach:
	char *word;
	int current,max;
	SetValue *list;
	Value *containing_object;

	BlockInfo();
	BlockInfo(CalculatorModule *mod, BlockTypes scopetype, int loop_start, int condition_start, char *var, Value *v);
	virtual ~BlockInfo();
	virtual const char *BlockType();
	virtual int AddValue(const char *name, Value *v);
	virtual int AddName(CalculatorModule *mod, ObjectDef *item, Value *container_v);
	virtual Entry *FindName(const char *name,int len);
	virtual Entry *createNewEntry(ObjectDef *item, CalculatorModule *module, Value *container_v);
	virtual int isSameEntry(ObjectDef *item, Entry *entry);
};

//---------------------------LaidoutCalculator
class LaidoutCalculator : public Interpreter,
						  public OpFuncEvaluator,
						  public FunctionEvaluator
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
	ObjectDef sessiondef;
	
	Laxkit::RefPtrStack<CalculatorModule> modules;

	BlockInfo global_scope;
	Laxkit::PtrStack<BlockInfo> scopes;
	Laxkit::PtrStack<OperatorLevel> oplevels;
	OperatorLevel leftops, rightops;
	BlockInfo *currentLevel() { return scopes.e[scopes.n-1]; }

	Laxkit::ErrorLog default_errorlog;
	Laxkit::ErrorLog *errorlog;
	int calcerror;
	char *calcmes;
	Value *last_answer;
	char *messagebuffer;

	int importAllNames(CalculatorModule *module);
	int importName(CalculatorModule *module, ObjectDef *def);
	int importOperators(CalculatorModule *module);
	int removeOperators(int module_id);
	int addOperator(const char *op,int dir,int priority, int module_id, OpFuncEvaluator *opfunc,ObjectDef *def);
	void InstallInnate();
	void InstallBaseTypes();

	void calcerr(const char *error,const char *where=NULL,int w=0, int surround=40);
	char *getnamestring(int *n);
	const char *getopstring(int *n);
	const char *getnamestringc(int *n);
	int getnamestringlen();
	void skipwscomment();
	void skipstring();
	void skipBlock(char ch);
	void skipRemainingBlock(char ch);
	void skipExpression();
	int nextchar(char ch);
	int nextword(const char *word);
	void newcurexprs(const char *newex,int len);
	void atNextCommandStep() {}
	int sessioncommand();
	void showDef(char *&temp, ObjectDef *sd);
	ObjectDef *GetSessionCommandObjectDef();
	void pushScope(BlockTypes scopetype, int loop_start=0, int condition_start=0, char *var=NULL, Value *v=NULL, ObjectDef *module=NULL);
	BlockTypes popScope();

	Value *eval(const char *exprs);
	Value *evalLevel(int level);
	int evalcondition();
	Value *checkAssignments();
	int Assignment(Value *num1, Value *num2, Value **value_ret, Laxkit::ErrorLog *log);
	int checkBlock(Value **value_ret);
	Value *number();
	long intnumber();
	double realnumber();
	int getunits();
	Value *getstring();
	Value *getset();
	Value *getarray();
	ValueHash *getValueHash();
	Value *evalname();
	Value *dereference(Value *val);
	ObjectDef *getClass();
	Entry *findNameEntry(const char *word,int len, int *scope, int *module, int *index);
	Value *opCall(const char *op,int n,int dir, Value *num, Value *num2, OperatorLevel *level, int lastindex);
	int functionCall(const char *word,int n, Value **v_ret, Value *containingvalue, ObjectDef *function,ValueHash *context,ValueHash *pp);
	int evaluate(const char *in, int len, ValueHash *context, ValueHash *parameters, Value **value_ret, Laxkit::ErrorLog *log);

	int add(Value *num1,Value *num2, Value **ret);
	int subtract(Value *num1,Value *num2, Value **ret);
	int multiply(Value *num1,Value *num2, Value **ret);
	int divide(Value *num1,Value *num2, Value **ret);
	int power(Value *num1,Value *num2, Value **ret);

	virtual int Op(const char *the_op,int len, int dir, Value *num1, Value *num2, CalcSettings *settings, Value **value_ret, Laxkit::ErrorLog *log);
	virtual int Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
						 Value **value_ret, Laxkit::ErrorLog *log);

	Value *ApplyDefaultSets(SetValue *set);
	void messageOut(const char *str,int output_lines=1);

	ValueHash *parseParameters(StyleDef *def);

 protected:
 public:
	LaidoutCalculator();
	virtual ~LaidoutCalculator();

	virtual int InstallVariables(ValueHash *values);
	virtual int InstallModule(CalculatorModule *module, int autoimport);
	virtual int RemoveModule(const char *modulename);
	virtual int ImportModule(const char *name, int allnames);
	virtual ObjectDef *GetInfo(const char *expr);

	virtual char *In(const char *in, int *return_type=NULL);
	virtual int evaluate(const char *in, int len, Value **value_ret, Laxkit::ErrorLog *log);

	virtual void ClearError();
	virtual const char *Message();

	virtual int RunShell();


	 //interpreter functions
	virtual const char *Id();
    virtual const char *Name();
    virtual const char *Description();
    virtual const char *Version();
	virtual int InitInterpreter();
    virtual int CloseInterpreter();
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

};


} // namespace Laidout

#endif

