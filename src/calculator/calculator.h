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

#include "../styles.h"
#include "values.h"

//----------------------------- LaidoutCalculator -------------------------------------
class LaidoutCalculator : public Laxkit::anObject
{
 private:
	 //context state
	char *dir;
	ValueHash *build_context();

	 //evaluation stuff
	Value *last_answer;
	char *messagebuffer;
	int from;
	int curline;
	char *curexprs;
	int curexprslen;
	char decimal;

	ValueHash uservars;
	Value *checkAssignments();
	Value *evalUservar(const char *word);

	ErrorLog *temp_errorlog;
	int calcerror;
	char *calcmes;

	void calcerr(const char *error,const char *where=NULL,int w=0, int surround=40);
	char *getnamestring(int *n);
	int getnamestringlen();
	void skipwscomment();
	int nextchar(char ch);
	int nextword(const char *word);
	void newcurexprs(const char *newex,int len);
	int sessioncommand();
	Value *eval(const char *exprs);
	Value *eval();
	Value *multterm();
	Value *powerterm();
	Value *number();
	long intnumber();
	double realnumber();
	Value *getstring();
	Value *getset();
	Value *evalname();
	Value *evalInnate(const char *word, int len);
	Value *ApplyDefaultSets(SetValue *set);
	void messageOut(const char *str);
	int add(Value *&num1,Value *&num2);
	int subtract(Value *&num1,Value *&num2);
	int multiply(Value *&num1,Value *&num2);
	int divide(Value *&num1,Value *&num2);
	int power(Value *&num1,Value *&num2);

	ValueHash *parseParameters(StyleDef *def);

 protected:
 public:
	LaidoutCalculator();
	virtual ~LaidoutCalculator();

	virtual char *In(const char *in);
	virtual int evaluate(const char *in, int len, Value **value_ret, int *error_pos, char **error_ret);

	virtual void clearerror();
	const char *Message();

	virtual int RunShell();
};

//------------------------------- parsing helpers ------------------------------------
ValueHash *MapParameters(StyleDef *def,ValueHash *rawparams);


#endif

