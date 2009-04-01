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

/*! \class LaidoutCalculator
 * \brief Command processing backbone.
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
 * \todo ***** this will eventually become the entry point for other language bindings
 *   to Laidout..
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

		while (isspace(*in)) in++;
		end=in;
		while (*end && *end!=';') end++;

		tmp=in;
		while (!isspace(*in)) in++;

		if (in==tmp) break; //no command word!!

		word=newnstr(tmp,in-tmp);

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
			if (*tmp) {
				if (stylemanager.styledefs.n || stylemanager.functions.n) {
					StyleDef *sd;
					int n=stylemanager.styledefs.n + stylemanager.functions.n;
					for (int c=0; c<n; c++) {
						 //search in styledefs and functions
						if (c<stylemanager.styledefs.n) sd=stylemanager.styledefs.e[c];
						else sd=stylemanager.functions.e[c-stylemanager.styledefs.n];

						if (!strcmp(sd->name,tmp)) {
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
			}
			if (temp) {
				appendline(str_ret,temp);
				delete[] temp;
			} else appendline(str_ret,_("Nothing to see here. Move along."));

		} else if (!strcmp(word,"open")) {
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
		}
	}

	if (str_ret) return str_ret;
	return newstr(_("You are surrounded by twisty passages, all alike."));
}

