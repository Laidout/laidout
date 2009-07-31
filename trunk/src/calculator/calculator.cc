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
// Copyright (C) 2009 by Tom Lechner
//


#include <lax/fileutils.h>
#include "calculator.h"
#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../headwindow.h"

using namespace Laxkit;
using namespace LaxFiles;

#define DBG 



//------------------------------------ LaidoutCalculator -----------------------------
/*! \class LaidoutCalculator
 * \brief Command processing backbone.
 *
 * The base LaidoutCalculator is a bare bones interpreter, and is built like
 * a very primitive console command based shell. ***this may change!
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
 */


LaidoutCalculator::LaidoutCalculator()
{
	dir=NULL; //working directory...
	
	decimal=1;
	from=0;
	curexprs=NULL;
	curexprslen=0;
	calcerror=0;
	calcmes=NULL;
	messagebuffer=NULL;
	last_answer=NULL;


	DBG cerr <<" ~~~~~~~ New Calculator created."<<endl;

	//***set up stylemanager.functions and stylemanager.styledefs here?
}

LaidoutCalculator::~LaidoutCalculator()
{
	if (dir) delete[] dir;
	if (curexprs) delete[] curexprs;
	if (calcmes) delete[] calcmes;
	if (messagebuffer) delete[] messagebuffer;
	if (last_answer) last_answer->dec_count();
}



//*********begin transplant******************************

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
}

//------------- String parse helper functions

//! Get a word starting at from. from is not modified.
/*! A name starts with a letter or an underscore, and 
 * continues until the first character that is not a letter, number,
 * or an underscore.
 *
 * curexprs[from] must be a letter or a '_'. Otherwise NULL is returned.
 * The length of the word is put in *n if n!=NULL.
 * The from variable is not advanced.
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

//! Skip whitespace and "#...\n" based comments.
/*! This will not advance past curexprslen, the length of the current expression.
 *
 * \todo should have something that keeps track of passing newlines, to make it easier
 *   go to line number of offending code
 */
void LaidoutCalculator::skipwscomment()
{
	while (isspace(curexprs[from]) && from<curexprslen) from++;
	if (curexprs[from]=='#') {
		while (curexprs[from++]!='\n' && from<curexprslen); 
		skipwscomment();
	}
}

/*! Skip whitespace and skip final comments in the form "#....\n".
 * If curexprs[from]==ch then from is advanced and 1 is returned.
 * Else 0 is returned (from will still be past the whitespace and comments).
 *
 * NOTE: since this skips ANYTHING after a '#' until a newline, you should
 * not use this while parsing strings.
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


//! Process a command or script. Returns a new char[] with the result.
/*! Do not forget to delete the returned array!
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

	if (in==NULL) { calcerr(_("Blank expression")); return NULL; }
	clearerror();

	Value *answer=NULL;
	if (len<0) len=strlen(in);
 	newcurexprs(in,len);
	if (curexprslen>len) curexprslen=len;
	skipwscomment();

	int tfrom=-1;
	while(tfrom!=from && from<curexprslen) { 
		tfrom=from;
		if (answer) { answer->dec_count(); answer=NULL; }
		if (sessioncommand()) { //  checks for session commands 
			if (!messagebuffer) messageOut(_("Ok.")); //***
			skipwscomment();
			if (from>=curexprslen) break;
			if (nextchar(';')) ; //advance past a ;
			continue;
		}
		if (calcerror) break;
		answer=eval();
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

//! Replace curexprs with newex, and set from=0.
void LaidoutCalculator::newcurexprs(const char *newex,int len)
{
	makenstr(curexprs,newex,len);
	curexprslen=strlen(curexprs);
	from=0;
}

//! Parse any commands that change internal calculator settings like radians versus decimal.
/*! Set dec==1 if 'deg' or 'degrees', set to 1 if 'rad' or 'radians'
 */
int LaidoutCalculator::sessioncommand() //  done before eval
{
	if (nextword("radians"))
	    { decimal=0; return 1; }
	
	if (nextword("rad"))
	    { decimal=0; return 1; }
	
	if (nextword("degrees"))
	    { decimal=1; return 1; }
	
	if (nextword("deg"))
	    { decimal=1; return 1; }


	if (nextword("quit")) {
		laidout->quit();
		return 1;
	}

	if (nextword("about")) {
		messageOut(LaidoutVersion());
		return 1;
	}

	if (nextword("?") || nextword("help")) {
		messageOut(_("The only recognized commands are:"));
		 // show stylemanager.functions calculator::in()"<<endl;
		for (int c=0; c<stylemanager.functions.n; c++) {
			messageOut(stylemanager.functions.e[c]->name);
		}
		 //show otherwise built in
		messageOut(	  " show [object type or function name]\n"
					  " about\n"
					  " help\n"
					  " quit\n"
					  " ?");
		return 1;
	}

	if (nextword("show")) {
		skipwscomment();
		
		char *showwhat=getnamestring(NULL);
		char *temp=NULL;
		if (showwhat) { //show a particular item
			if (stylemanager.styledefs.n || stylemanager.functions.n) {
				StyleDef *sd;
				int n=stylemanager.styledefs.n + stylemanager.functions.n;
				for (int c=0; c<n; c++) {
					 //search in styledefs and functions
					if (c<stylemanager.styledefs.n) {
						sd=stylemanager.styledefs.e[c];
					} else {
						sd=stylemanager.functions.e[c-stylemanager.styledefs.n];
					}

					if (!strcmp(sd->name,showwhat)) {
						if (sd->format==Element_Function) {
							appendstr(temp,"function ");
						} else {
							appendstr(temp,"object ");
						}
						appendstr(temp,sd->name);
						//appendstr(temp,": ");
						//appendstr(temp,sd->Name);
						appendstr(temp,", ");
						appendstr(temp,sd->description);
						if (sd->format!=Element_Fields) {
							appendstr(temp," (");
							appendstr(temp,element_TypeNames[sd->format]);
							appendstr(temp,")");
						}
						if (sd->format!=Element_Function && sd->extends) {
							appendstr(temp,"\n extends ");
							appendstr(temp,sd->extends);
						}
						if ((sd->format==Element_Fields || sd->format==Element_Function) && sd->getNumFields()) {
							const char *nm,*Nm,*desc;
							appendstr(temp,"\n");
							for (int c2=0; c2<sd->getNumFields(); c2++) {
								sd->getInfo(c2,&nm,&Nm,&desc);
								appendstr(temp,"  ");
								appendstr(temp,nm);
								appendstr(temp,": ");
								appendstr(temp,Nm);
								appendstr(temp,", ");
								appendstr(temp,desc);
								appendstr(temp,"\n");

							}
						}

						break;
					}
				}
			} else {
				appendline(temp,_("Unknown name!"));
			}
			from+=strlen(showwhat);
			delete[] showwhat; showwhat=NULL;

		} else { //show all
			 
			 //Show project and documents
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
		
			 //Show object definitions in stylemanager
			if (stylemanager.styledefs.n) {
				appendstr(temp,_("\nObject Definitions:\n"));
				for (int c=0; c<stylemanager.styledefs.n; c++) {
					appendstr(temp,"  ");
					appendstr(temp,stylemanager.styledefs.e[c]->name);
					//appendstr(temp,", ");
					//appendstr(temp,stylemanager.styledefs.e[c]->Name);
					if (stylemanager.styledefs.e[c]->extends) {
						appendstr(temp,", extends: ");
						appendstr(temp,stylemanager.styledefs.e[c]->extends);
					}
					appendstr(temp,"\n");
				}
			}

			 //Show builtin math functions
			appendstr(temp,_("\nMath functions: "));
			appendstr(temp,"\npi, e, tau, sin, asin, cos, acos, tan, atan, atan2, abs, sgn, sqrt, int, "
						   "gint, floor, lint, ceiling, log, ln, exp, cosh, sinh, tanh, factor\n");


			 //Show function definitions in stylemanager
			if (stylemanager.functions.n) {
				appendstr(temp,_("\nFunction Definitions:\n"));
				const char *nm=NULL;
				for (int c=0; c<stylemanager.functions.n; c++) {
					appendstr(temp,"  ");
					appendstr(temp,stylemanager.functions.e[c]->name);
					appendstr(temp,"(");
					for (int c2=0; c2<stylemanager.functions.e[c]->getNumFields(); c2++) {
						stylemanager.functions.e[c]->getInfo(c2,&nm,NULL,NULL);
						appendstr(temp,nm);
						if (c2!=stylemanager.functions.e[c]->getNumFields()-1) appendstr(temp,",");
					}
					appendstr(temp,")\n");
				}
			}
		}
		if (temp) {
			messageOut(temp);
			delete[] temp;
		} else messageOut(_("Nothing to see here. Move along."));

		return 1; //from "show"
	}


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
	Value *num;
	texprs=new char[strlen(curexprs)+1];
	strcpy(texprs,curexprs);

	newcurexprs(exprs,-1);
	num=eval();

	newcurexprs(texprs,tfrom);
	delete[] texprs;
	return num;
}

//! Evaluate a single expression.
Value *LaidoutCalculator::eval()
{
	Value *num=NULL,*num2=NULL;
	int plus;
	num=multterm();
	while (plus=nextchar('+'), num && (plus || nextchar('-'))) {
		num2=multterm();
		if (!num2){ calcerr(_("+ needs number")); num->dec_count(); return NULL; }
		try {
			if (plus) add(num,num2);
			else subtract(num,num2);
		} catch (const char *err) {
			calcerr(err);
			num->dec_count(); num=NULL;
			num2->dec_count(); num2=NULL;
		}
	}
	return num;
}

Value *LaidoutCalculator::multterm()
{
	Value *num=NULL,*num2=NULL;
	int mult;
	num=powerterm();
	while (mult=nextchar('*'), num && (mult || nextchar('/')))
	{
		num2=powerterm();
		try {
			if (mult) multiply(num,num2); 
			else divide(num,num2);
		} catch (const char *err) {
			calcerr(err);
			num->dec_count(); num=NULL;
			num2->dec_count(); num2=NULL;
		}
	}
	return num;
}

Value *LaidoutCalculator::powerterm()
{
	Value *num=NULL,*num2=NULL;
	num=number();
	if (num && nextchar('^'))
	{
		num2=powerterm();
		try {
			power(num,num2);
		} catch (const char *err) {
			calcerr(err);
			num->dec_count(); num=NULL;
			num2->dec_count(); num2=NULL;
		}
	}
	return num;
}

//! Read in a simple number, no processing of operators except initial '+' or '-'.
/*! If the number is an integer or real, then read in units also.
 */
Value *LaidoutCalculator::number()
{
	Value *snum=NULL;
	int sgn=1;
	while (1) {
		if (nextchar('-')) sgn*=-1;
		else if (!nextchar('+')) break;
	};
	 
	if (nextchar('(')) {
		 // handle stuff in parenthesis: (2-3-5)-2342
		snum=eval();
		if (calcerror) return NULL;
		if (!snum) { 
			calcerr(_("Number exepected."));
			return NULL;
		}
		//*** if nextchar(",") then we are reading in some kind of set
		if (!nextchar(')')) { 
			calcerr(_("')' expected.")); 
			delete snum;
			return NULL;
		}

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
		 //  is not a simple number, maybe styledef.function, styledef.objectdef, cast, or variable
		snum=evalname();

		if (calcerror) { if (snum) delete snum; return NULL; }
		if (!snum) return NULL; 
	}

	 //apply the sign as first found above
	if (sgn==-1 && snum) {
		Value *temp=new IntValue(-1);
		try {
			multiply(temp,snum);
		} catch (const char *e) {
			calcerr(e);
			if (snum) { snum->dec_count(); snum=NULL; }
			temp->dec_count(); temp=NULL;
		}
		snum=temp;
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
		calcerr(_("String not closed."));
		return NULL;
	}
	from++;
	newstr[spos]='\0';
	Value *str=new StringValue(newstr,spos);
	delete[] newstr;
	return str;
}

/*! from points to beginning of word.
 * If name not found, then from will still point to beginning of word.
 */
Value *LaidoutCalculator::evalname()
{
	 //search for function name
	int n=0;
	char *word=getnamestring(&n);
	if (!word) return NULL;

	// ***todo? if a function takes one string input, then send everything from [in..end) as the string
	//    this lets you have "command aeuaoe aoeu aou;" where the function does its own parsing.
	//    In this case, the function parameters will have 1 parameter: "commanddata",value=[in..end).
	//    otherwise, assume nested parsing like "function (x,y)", "function x,y"
	int c;

	 //search for function in stylemanager
	for (c=0; c<stylemanager.functions.n; c++) {
		if (!strcmp(word,stylemanager.functions.e[c]->name)) break;
	}
	if (c!=stylemanager.functions.n && stylemanager.functions.e[c]->stylefunc) {
		 //we found a function!
		StyleDef *function=stylemanager.functions.e[c];
		from+=n;
		skipwscomment();

		 //build context and find parameters
		//int error_pos=0;
		ValueHash *context=build_context(); //build calculator context
		ValueHash *pp=parseParameters(function); //build parameter hash in order of styledef

		 //call the actual function
		char *message=NULL;
		Value *value=NULL;
		int status=function->stylefunc(context,pp, &value,&message);
		delete pp;
		delete context;

		 //cleanup and report
		if (message) messageOut(message);
		else {
			if (status>0) calcerr(_("Command failed, tell the devs to properly document failure!!"));
			else if (status<0) messageOut(_("Command succeeded with warnings, tell the devs to properly document warnings!!"));
		}
		if (calcerror) { if (value) value->dec_count(); value=NULL; }
		return value;
	}

	 //search for user variables
	//***

	 //search for innate functions
	Value *num=evalInnate(word,n);
	if (num) return num;
	
	return NULL;
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

	int tfrom=from;
	from+=len;
	ValueHash *pp=parseParameters(NULL);
	if (!pp) return NULL;
	Value *v=NULL;

	try {
		v=pp->value(0);
		double d;
		if (v->type()==VALUE_Int) d=((IntValue*)v)->i;
		else if (v->type()==VALUE_Double) d=((DoubleValue*)v)->d;
		else throw 1;

		if (pp->n()==2) {
			double d2;
			v=pp->value(1);
			if (v->type()==VALUE_Int) d2=((IntValue*)v)->i;
			else if (v->type()==VALUE_Double) d2=((DoubleValue*)v)->d;
			else { v=NULL; throw 1; }

			if (!strcmp(word,"atan2")) { d=atan2(d,d2); if (decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else { v=NULL; throw -1; }

		} else if (pp->n()==1) {
			if (!strcmp(word,"sin"))  { if (decimal) d*=M_PI/180; v=new DoubleValue(sin(d)); }
			else if (!strcmp(word,"asin")) { if (d<-1. || d>1.) throw 3; d=asin(d); if (decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else if (!strcmp(word,"cos"))  { if (decimal) d*=M_PI/180; v=new DoubleValue(cos(d)); }
			else if (!strcmp(word,"acos")) { if (d<-1. || d>1.) throw 3; d=acos(d); if (decimal) d*=180/M_PI; v=new DoubleValue(d); }
			else if (!strcmp(word,"tan"))  { if (decimal) d*=M_PI/180; v=new DoubleValue(tan(d)); }
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

//! Add integers and reals, and concat strings.
/*! num1 and num2 must both exist. num2 is dec_count()'d, and num1 is made
 * to point to the answer.
 *
 * Return 1 on success or 0 for no-can-do.
 */
int LaidoutCalculator::add(Value *&num1,Value *&num2)
{
	if (num1==NULL) { if (num2) { num2->dec_count(); num2=NULL; } return 0; }
	if (num2==NULL) { num1->dec_count(); num1=NULL; return 0; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) {
		((IntValue *) num1)->i+=((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Int) {
		((DoubleValue *) num1)->d+=(double)((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Double) {
		((DoubleValue *) num2)->d+=(double)((IntValue*)num1)->i;
		Value *t=num2;
		num2=num1;
		num1=t;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Double) {
		((DoubleValue *) num1)->d+=((DoubleValue*)num2)->d;
	} else if (num1->type()==VALUE_String && num2->type()==VALUE_String) {
		appendstr(((StringValue *)num1)->str,((StringValue *)num2)->str);
	} else throw _("Cannot add those types");

	num2->dec_count(); num2=NULL;
	return 1;
}

//! Subtract integers and reals.
int LaidoutCalculator::subtract(Value *&num1,Value *&num2)
{
	if (num1==NULL) { if (num2) { num2->dec_count(); num2=NULL; } return 0; }
	if (num2==NULL) { num1->dec_count(); num1=NULL; return 0; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) {
		((IntValue *) num1)->i-=((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Int) {
		((DoubleValue *) num1)->d-=(double)((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Double) {
		((DoubleValue *) num2)->d=(double)((IntValue*)num1)->i - ((DoubleValue *) num2)->d;
		Value *t=num2;
		num2=num1;
		num1=t;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Double) {
		((DoubleValue *) num1)->d-=((DoubleValue*)num2)->d;
	} else throw _("Cannot subtract those types");

	num2->dec_count(); num2=NULL;
	return 1;
}

//! Multiply integers and reals.
int LaidoutCalculator::multiply(Value *&num1,Value *&num2)
{
	if (num1==NULL) { if (num2) { num2->dec_count(); num2=NULL; } return 0; }
	if (num2==NULL) { num1->dec_count(); num1=NULL; return 0; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) {
		((IntValue *) num1)->i*=((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Int) {
		((DoubleValue *) num1)->d*=(double)((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Double) {
		((DoubleValue *) num2)->d*=(double)((IntValue*)num1)->i;
		Value *t=num2;
		num2=num1;
		num1=t;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Double) {
		((DoubleValue *) num1)->d*=((DoubleValue*)num2)->d;
	} else throw _("Cannot multiply those types");

	num2->dec_count(); num2=NULL;
	return 1;
}

int LaidoutCalculator::divide(Value *&num1,Value *&num2)
{
	if (num1==NULL) { if (num2) { num2->dec_count(); num2=NULL; } return 0; }
	if (num2==NULL) { num1->dec_count(); num1=NULL; return 0; }
//	if (strcmp(num1->units,num2->units)) {
//		 //must correct units
//		***
//		if one has units, but the other doesn't, then assume same units
//	}


	double divisor;
	if (num2->type()==VALUE_Int) divisor=((IntValue*)num2)->i;
	else if (num2->type()==VALUE_Double) divisor=((DoubleValue*)num2)->d;
	else throw _("Cannot divide with that type");

	if (divisor==0) throw _("Division by zero");

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int) {
		if (((IntValue *) num1)->i % ((IntValue*)num2)->i == 0) 
			((IntValue *) num1)->i/=((IntValue*)num2)->i;
		else {
			DoubleValue *v=new DoubleValue(((double)((IntValue *) num1)->i)/((IntValue*)num2)->i);
			num1->dec_count();
			num1=v;
		}
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Int) {
		((DoubleValue *) num1)->d/=(double)((IntValue*)num2)->i;
	} else if (num1->type()==VALUE_Int && num2->type()==VALUE_Double) {
		((DoubleValue *) num2)->d=(double)((IntValue*)num1)->i / ((DoubleValue *) num2)->d;
		Value *t=num2;
		num2=num1;
		num1=t;
	} else if (num1->type()==VALUE_Double && num2->type()==VALUE_Double) {
		((DoubleValue *) num1)->d/=((DoubleValue*)num2)->d;
	} else throw _("Cannot divide those types");

	num2->dec_count(); num2=NULL;
	return 1;
}

/*! \todo for roots that are actually integers, should maybe check for that.
 */
int LaidoutCalculator::power(Value *&num1,Value *&num2)
{
	if (num1==NULL) { if (num2) { num2->dec_count(); num2=NULL; } return 0; }
	if (num2==NULL) { num1->dec_count(); num1=NULL; return 0; }

	double base, expon;

	if (num1->type()==VALUE_Int) base=(double)((IntValue*)num1)->i;
	else if (num1->type()==VALUE_Double) base=((DoubleValue*)num1)->d;
	else throw _("Cannot raise powers with those types");

	if (num2->type()==VALUE_Int) expon=((IntValue*)num2)->i;
	else if (num2->type()==VALUE_Double) expon=((DoubleValue*)num2)->d;
	else throw _("Cannot raise powers with those types");

	if (base==0) throw _("Cannot compute 0^x");
	//***this could check for double close enough to int:
	if (base<0 && num2->type()!=VALUE_Int) throw _("(-x)^(non int): Complex not allowed"); 

	if (num1->type()==VALUE_Int && num2->type()==VALUE_Int && expon>=0) {
		((IntValue*)num1)->i=(long)(pow(base,expon)+.5);
	} else {
		num1->dec_count();
		num1=new DoubleValue(pow(base,expon));
	}

	num2->dec_count(); num2=NULL;
	return 1;
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
	if (laidout->project->docs.n) pp->push("document", laidout->project->docs.e[0]->doc->Saveas());//***
	//pp.push("window", NULL); ***default to first view window pane in same headwindow?
	return pp;
}

//! Parse into function parameters.
/*! If def!=NULL, then call MapParameters() to get the parameters to match the fields of the StyleDef.
 */
ValueHash *LaidoutCalculator::parseParameters(StyleDef *def)
{
	ValueHash *pp=NULL;

// ***	 //check for when the styledef has 1 string parameter, and you have a large in string.
//	 //all of in maps to that one string. This allows functions to act like command line strings.
//	 //They would then call some other function to parse the string or parse it internally.
//	if (def->getNumFields()==1 ) {
//		ElementType fmt=Element_None;
//		const char *nm=NULL;
//		def->getInfo(0,&nm,NULL,NULL,NULL,NULL,&fmt);
//		if (fmt==Element_String) {
//			pp->push(nm,str);
//			delete[] str;
//			return pp;
//		}
//	}

	if (nextchar('(')) {
		pp=new ValueHash;
		int tfrom;
		char *pname=NULL;
		int namel;
		Value *v=NULL;
		if (!nextchar(')'))
		  do {
			skipwscomment();
			tfrom=from;
			pname=getnamestring(&namel);
			if (pname) {
				from+=namel;
				if (nextchar('=')) {
					//it is a parameter name (hopefully) so nothing special to do here
				} else {
					from=tfrom; //is not a parameter name, so reset from
				}
			}
			v=eval();
			if (v && !calcerror) {
				pp->push(pname,v);
				v->dec_count();
			}
			if (pname) { delete[] pname; pname=NULL; }

		} while (!calcerror && from!=tfrom && nextchar(','));
		if (!nextchar(')')) {
			delete pp;
			pp=NULL;
			calcerr(_("Expected closing ')'")); 
		}
	} 

	if (pp && def) MapParameters(def,pp);
	return pp;
}





//----------------------------- StyleDef utils ------------------------------------------

//! Map a ValueHash to a function StyleDef.
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
 * Now say we have a StyleDef something like:
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
ValueHash *MapParameters(StyleDef *def,ValueHash *rawparams)
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

		//if (format==Element_DynamicEnum || format==Element_Enum) {
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



