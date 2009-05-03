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
	DBG cerr <<" ~~~~~~~ New Calculator created."<<endl;

	//***set up stylemanager.functions and stylemanager.styledefs here?
}

LaidoutCalculator::~LaidoutCalculator()
{
	if (dir) delete[] dir;
}

//! Process a command or script. Returns a new char[] with the result.
/*! Do not forget to delete the returned array!
 * 
 * \todo it almost goes without saying this needs automation
 * \todo it also almost goes without saying this is in major hack stage of development
 * \todo scan to end of expression does simple check for ';'. needs to parse as it goes...
 */
char *LaidoutCalculator::In(const char *in)
{
	const char *end=NULL,*tmp=NULL;
	char *str_ret=NULL;
	int err=0;
	char *word=NULL;
	while (!err && in && *in) {
		if (end) {
			if (*end) in=end+1;
			else break;
		}

		 //Each line of input is structured thusly:
		 //  > word ...other stuff....;  word ...otherstuff...;  etc
		 //
		 //After the next few lines:
		 // in    points to the next non-whitespace character after word
		 // end   points to the final character of a line, usually ';' or '\0'
		 // word  holds a string of the word start off the line
		while (isspace(*in)) in++;
		end=in;
		while (*end && *end!=';') end++; //*****this chokes on nested ';'

		tmp=in;
		while (*in && !isspace(*in) && *in!='(' && *in!='.') in++;

		if (in==tmp) break; //no command word!!

		word=newnstr(tmp,in-tmp); //word is any string of non-whitespace
		while (isspace(*in)) in++;

		if (!strcmp(word,"quit")) {
			laidout->quit();
			delete[] word;
			if (str_ret) return str_ret;
			return newstr("");

		} else if (!strcmp(word,"about")) {
			delete[] word;
			return newstr(LaidoutVersion());

		} else if (!strcmp(word,"newdoc")) {
			while (isspace(*in)) in++;
			tmp=newnstr(in,end-in);
			if (laidout->NewDocument(tmp)==0) appendline(str_ret,_("Document added."));
			else appendline(str_ret,_("Error adding document. Not added"));
			delete[] tmp;
			delete[] word;

		} else if (!strcmp(word,"show")) {
			delete[] word; word=NULL;
			while (isspace(*in) && in!=end) in++;
			tmp=newnstr(in,end-in);
			
			char *temp=NULL;
			if (*tmp) { //show a particular item
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

						if (!strcmp(sd->name,tmp)) {
							if (sd->format==Element_Function) {
								appendstr(temp,"function ");
							} else {
								appendstr(temp,"object ");
							}
							appendstr(temp,sd->name);
							appendstr(temp,": ");
							appendstr(temp,sd->Name);
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
				delete[] tmp; tmp=NULL;
			} else { //continue to show all
				if (tmp) { delete[] tmp; tmp=NULL; }
				 
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
				appendline(str_ret,temp);
				delete[] temp;
			} else appendline(str_ret,_("Nothing to see here. Move along."));

		} else if (!strcmp(word,"open")) {
			delete[] word; word=NULL;
			try {
				 // get filename potentially
				while (isspace(*in) && in!=end) in++;
				if (in==end) {
					 //*** ?? call up an open dialog for laidout document files
					appendline(str_ret, _("Open what? Missing filename."));
					throw -1;
				}

				 // try to open up whatever filename is in in
				char *temp;
				if (dir==NULL) {
					temp=get_current_dir_name();
					dir=newstr(temp);
					free (temp);
				}
				if (*in!='/') {
					temp=newstr(dir);
					appendstr(temp,"/");
					appendnstr(temp,in,end-in);
				}
				if (file_exists(temp,1,NULL)!=S_IFREG) err=1;
				delete[] temp;
				if (err) {
					appendline(str_ret,_("Could not load that."));
					throw 2;
				} 

				 //else not thrown...
				if (laidout->findDocument(in)) {
					appendline(str_ret,_("That document is already loaded."));
					throw 3;
				} 
				int n=laidout->numTopWindows();
				char *error=NULL;
				Document *doc=NULL;
				if (laidout->Load(in,&error)>=0) {
					 //on a successful load, laidout->curdoc is the document just loaded.
					doc=laidout->curdoc;
				}
				if (!doc) {
					prependstr(error,_("Errors loading.\n"));
					appendstr(error,_("Not loaded."));
					appendline(str_ret,error);
					delete[] error; error=NULL;
					throw 4;
				}

				 // create new window only if LoadDocument() didn't create new windows
				 // ***this is a little icky since any previously saved windows might not
				 // actually refer to the document opened
				if (n!=laidout->numTopWindows()) {
					anXWindow *win=newHeadWindow(doc,"ViewWindow");
					if (win) laidout->addwindow(win);
				}
				if (!error) {
					appendline(str_ret,_("Opened."));
				} else {
					prependstr(error,_("Warnings encountered while loading:\n"));
					appendstr(error,_("Loaded."));
					appendline(str_ret,error);
					delete[] error;
				}

			} catch (int thrown) {
				//hurumph.
				if (thrown>=0) err=1;
			}


		} else if (!strcmp(word,"?") || !strcmp(word,"help")) {
			appendline(str_ret,_("The only recognized commands are:"));
			 // show stylemanager.functions calculator::in()"<<endl;
			for (int c=0; c<stylemanager.functions.n; c++) {
				appendline(str_ret,stylemanager.functions.e[c]->name);
			}
			 //show otherwise built in
			appendline(str_ret,
						  " show [object type or function name]\n"
						  " newdoc [spec]\n"
						  " open [a laidout document]\n"
						  " about\n"
						  " help\n"
						  " quit\n"
						  " ?");

		} else { //search for function name
			const char *s=word;
			while (*s && *s!='(' && *s!='.') s++;

			//*** if a function takes one string input, then send everything from [in..end) as the string
			//    this lets you have "command aeuaoe aoeu aou;" where the function does its own parsing.
			//    In this case, the function parameters will have 1 parameter: "commanddata",value=[in..end).
			//    otherwise, assume nested parsing like "function (x,y)", "function x,y"
			int c;
			for (c=0; c<stylemanager.functions.n; c++) {
				if (!strcmp(word,stylemanager.functions.e[c]->name)) break;
			}
			if (c!=stylemanager.functions.n && stylemanager.functions.e[c]->stylefunc) {
				 //parse parameters
				StyleDef *function=stylemanager.functions.e[c];
				//int error_pos=0;
				char *error=NULL;
				ValueHash *pp=parse_parameters(function, in,end-in, &error);
				ValueHash *context=build_context();
				char *message=NULL;
				Value *value=NULL;
				int status=function->stylefunc(context,pp, &value,&message);
				delete pp;
				delete context;
				if (value) appendline(str_ret,value->toCChar());
				if (message) appendline(str_ret,message);
				else {
					if (status==0) { if (!value) appendline(str_ret,_("Ok.")); }
					else if (status>0) appendline(str_ret,_("Command failed, tell the devs to properly document failure!!"));
					else appendline(str_ret,_("Command succeeded with warnings, tell the devs to properly document warnings!!"));
				}
				if (value) value->dec_count();
			}
		}
	}

	if (str_ret) return str_ret;
	return newstr(_("You are surrounded by twisty passages, all alike."));
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
	if (laidout->project->docs.n) pp->push("document", laidout->project->docs.e[0]->doc->Saveas());
	//pp.push("window", NULL); ***default to first view window pane in same headwindow?
	return pp;
}

//! Take a string and parse into function parameters.
ValueHash *LaidoutCalculator::parse_parameters(StyleDef *def, const char *in, int len, char **error_ret)
{
	if (!def || !in || len<=0) return NULL;

	ValueHash *pp;

	char *str=newnstr(in,len);
	char *ee=NULL;
	Attribute *att=parse_fields(NULL, str, &ee);
	MapAttParameters(att, def);


	if (def->getNumFields()==1 ) {
	}
	if (*in=='(') ;

	return pp;
}

//----------------------------- StyleDef utils ------------------------------------------

//! Take the output of parse_fields(), and map to a StyleDef.
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
 *     . net
 *       . box
 *         width 5
 *         - 3
 *         - 6
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
 *     . net
 *       . box
 *         width 5
 *         - 3
 *         - 6
 * </pre>
 *
 * \todo enum searching is not currently implemented
 */
LaxFiles::Attribute *MapAttParameters(LaxFiles::Attribute *rawparams, StyleDef *def)
{
	if (!rawparams || !def) return NULL;
	int n=def->getNumFields();
	int c2;
	const char *name;
	Attribute *p;
	while (rawparams->attributes.n!=n) rawparams->attributes.push(NULL);
	 //now there are the same number of elements in rawparams and the styledef
	 //we go through from 0, and swap as needed
	for (int c=0; c<n; c++) { //for each styledef field
		def->getInfo(c,&name,NULL,NULL);

		//if (format==Element_DynamicEnum || format==Element_Enum) {
		//	 //rawparam att name could be any one of the enum names
		//}

		for (c2=c; c2<rawparams->attributes.n; c2++) { //find the right parameter
			p=rawparams->attributes.e[c2];
			if (!strcmp(p->name,"-")) continue;
			if (!strcmp(p->name,name)) {
				 //found field name match between a parameter and the styledef
				if (c2!=c) {
					 //parameter in the wrong place, so swap with the right place
					rawparams->attributes.swap(c2,c);
				} // else param was in right place
				break;
			}
		}
		if (c2!=rawparams->attributes.n) continue;
		if (c2==rawparams->attributes.n) {
			 // did not find a matching name, so grab the 1st "-" parameter
			for (c2=c; c2<rawparams->attributes.n; c2++) {
				p=rawparams->attributes.e[c2];
				if (strcmp(p->name,"-")) continue;
				if (c2!=c) {
					 //parameter in the wrong place, so swap with the right place
					rawparams->attributes.swap(c2,c);
				} // else param was in right place
				makestr(rawparams->attributes.e[c]->name,name); //name the parameter correctly
			}
		}
		if (c2==rawparams->attributes.n) {
			 //there were extra parameters that were not know to the styledef
			 //this breaks the transfer, return NULL for error
			return NULL;
		}
	}
	return rawparams;
}


//-------------------------- various parsing functions ------------------------------


//! Parse str into a series of parameter, as for a function call.
/*! \ingroup misc
 * Parsing will terminate if it encounters an unmatched ')'
 *
 * Warning: this is really just a hack placeholder, until scripting and data
 * handling gets a proper implementation.
 *
 * This assumes there is a comma separated list of paremeters. You can give
 * the name of the paremeter, a la python: "width=5". The string will be turned
 * into a list with each parameter as a seperate subattribute. If you pass in
 * "width=5", then there will be an attribute with name="width" and value="5".
 *
 * Any parameter that has no explicit name will get name="-". For example,
 * if you simply pass in "width", then you will get name="-" and value="width".
 *
 * Note that this will only expand values, not names. Names must be a string of
 * non-whitespace characters before an '='. If the value is something 
 * like "imposition=net.box(3,5)" then
 * the attribute will be name="imposition" and value="net", and there
 * will be a subattribute with name="." and value="box", which has further subattributes with
 * values "3" and "5", and whose names are both '-'.
 *
 * Something like "imposition.net.box(width=5,3,6)" will be an attribute like this:
 * <pre>
 *  - imposition
 *     . net
 *       . box
 *         width 5
 *         - 3
 *         - 6
 * </pre>
 *
 * <pre>
 *    letter, 3 pages, net.box(3,5,6)
 *    letter, 3 pages, net.box(width=3,height=5,depth=6)
 *    blah=blabber, imposition.file(/some/file)
 *    net.file(/some/file.off)
 * </pre>
 *
 * Used, for instance, in LaidoutApp::parseargs(), or in command line pane.
 *
 * \todo ***this is a means to an end at the moment. Full scripting will likely obsolete this.
 */
Attribute *parse_fields(Attribute *Att, const char *str,char **end_ptr)
{
	Attribute *tatt,*att=Att;
	if (!att) att=new Attribute;
	const char *s=str;
	char *e;

	while (isspace(*s)) s++;
	while (s && *s) {
		tatt=parse_a_field(att,s,&e);
		if (e==s || !tatt) break;
		if (*e==',') e++;
		s=e;
	}
	if (end_ptr) *end_ptr=e;

	//DBG cerr <<"parsed str into attribute:"<<endl;
	//DBG att->dump_out(stderr,0);

	return att;
}

/*! \ingroup misc
 * If nothing parsed, end_ptr is set to str.
 * See parse_fields() for full explanation
 */
Attribute *parse_a_field(Attribute *Att, const char *str, char **end_ptr)
{
	Attribute *att=Att;
	if (!att) att=new Attribute;

	Attribute *subatt=NULL;
	const char *s,*e;
	char *name=NULL, *value=NULL;
	char error=0;
	while (isspace(*str) && *str!=',' && *str!=')') str++;

	 //now str points to the start of a name..
	s=str;
	while (*s && !isspace(*s) && *s!='=' && *s!=',' && *s!=')' && *s!='.' && *s!='(') s++;
	if (s==str) {
		 //empty string, or premature ending!
		if (end_ptr) *end_ptr=const_cast<char*>(s);
		return NULL; //error! no name found;
	}
	e=s;
	while (isspace(*e)) e++;

	if (*e=='=') {
		name=newnstr(str,s-str);
		str=e+1;
		while (isspace(*str)) str++;
	} else {
		//leave str at beginning of what might have been a name
		name=newstr("-");
	}

	 //now str points at the start of a value, we must parse the value now
	subatt=att;
	do {
		s=str; //*str!=whitespace

		 //scan for a value string
		if (isdigit(*s)) while (*s && !isspace(*s) && *s!=',' && *s!=')') s++;
		else while (*s && !isspace(*s) && *s!=',' && *s!=')' && *s!='.' && *s!='(') s++;
		if (s==str) {
			 //reached end of field
			if (*str==',') str++; //make str point to start of next field
			if (*s=='(' || *s=='.') error=1;
			break;
		}

		 //push new attribute with name and value. we might add subattributes
		value=newnstr(str,s-str);
		subatt->push(name,value); //1st is something like "-" -> "net", then "."->"net"
		delete[] name;  name=NULL;
		delete[] value; value=NULL;

		while (isspace(*s)) s++;
		if (*s=='.') {
			 //found something like "net.box(1,2,3)", and value would be "net"
			if (name==NULL) name=newstr(".");
			s++;
			while (isspace(*s)) s++;
			str=s;
			
			subatt=subatt->attributes.e[subatt->attributes.n-1];
			continue;
		}
		if (*s=='(') {
			s++;
			str=s;
			subatt=subatt->attributes.e[subatt->attributes.n-1];
			char *ee;
			if (parse_fields(subatt,str,&ee)==NULL) {
				error=1;
				break;
			}
			s=ee;
			if (*s!=')') {
				error=1; //error! unmatched ')'
				break;
			}
			str=s+1;
			while (isspace(*str)) str++;
			if (*str==',') str++;
			//at this point, there should be ONLY a ')', whitespace, or eol left in field
			break;
		}
		if (*s==')' || *s==',') {
			 //we have a simple attribute, then we are at end of the field
			str=s;
			break;
		}
		str=s;
	} while (str && *str && !error);

	if (name) delete[] name;
	if (value) delete[] value;

	if (end_ptr) *end_ptr=const_cast<char*>(str);
	if (error && att!=Att) { delete att; att=NULL; } //att was created locally

	return att;
}



