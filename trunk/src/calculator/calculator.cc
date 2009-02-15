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
 */


LaidoutCalculator::LaidoutCalculator()
{
	dir=NULL; //working directory...
}

LaidoutCalculator::~LaidoutCalculator()
{
	if (dir) delete[] dir;
}

//! Process a command or script.
/*! This will return a new char[].
 * 
 * \todo it almost goes without saying this needs automation
 */
char *LaidoutCalculator::In(const char *in)
{

	while (isspace(*in)) in++;
	if (!strncmp(in,"quit",4)) {
		laidout->quit();
		return newstr("");
	} else if (!strncmp(in,"newdoc",6)) {
		if (!isalnum(in[6])) {
			in+=6;
			while (isspace(*in)) in++;
			if (laidout->NewDocument(in)==0) return newstr(_("Document added."));
			else return newstr(_("Error adding document. Not added"));
		}
	} else if (!strncmp(in,"show",4)) {
		 //this is currently just "show all"
		 
		 //Show project and documents
		char *temp=newstr(_("Project: ")), temp2[10];
		if (laidout->project->name) appendstr(temp,laidout->project->name);
		else appendstr(temp,_("(untitled)"));
		appendstr(temp,"\n");

		if (laidout->project->filename) {
			appendstr(temp,laidout->project->filename);
			appendstr(temp,"\n");
		}

		if (laidout->project->docs.n) appendstr(temp," documents\n");
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
			appendstr(temp,"\nObject Definitions:\n");
			for (int c=0; c<stylemanager.styledefs.n; c++) {
				appendstr(temp,"  ");
				appendstr(temp,stylemanager.styledefs.e[c]->name);
				//appendstr(temp,", ");
				//appendstr(temp,stylemanager.styledefs.e[c]->Name);
				if (stylemanager.styledefs.e[c]->extends) {
					appendstr(temp,". Extends: ");
					appendstr(temp,stylemanager.styledefs.e[c]->extends);
				}
				appendstr(temp,"\n");
			}
		}

		return (temp?temp:newstr(_("Nothing to see here. Move along.")));
	} else if (!strncmp(in,"open",4) && (isspace(in[4]) || in[4]=='\0')) {
		if (isspace(in[4])) {
			 // get filename potentially
			in+=5;
			while (isspace(*in)) in++;
			if (*in!='\0') {
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
					appendstr(temp,in);
				}
				if (file_exists(temp,1,NULL)!=S_IFREG) in=NULL;
				delete[] temp;
				if (in==NULL) return newstr(_("Could not load that."));

				if (laidout->findDocument(in)) return newstr(_("That document is already loaded."));
				int n=laidout->numTopWindows();
				char *error=NULL;
				Document *doc=NULL;
				if (laidout->Load(in,&error)>=0) doc=laidout->curdoc;
				if (!doc) {
					prependstr(error,_("Errors loading.\n"));
					appendstr(error,_("Not loaded."));
					return error;
				}

				 // create new window only if LoadDocument() didn't create new windows
				 // ***this is a little icky since any previously saved windows might not
				 // actually refer to the document opened
				if (n!=laidout->numTopWindows()) {
					anXWindow *win=newHeadWindow(doc,"ViewWindow");
					if (win) laidout->addwindow(win);
				}
				if (!error) return newstr("Opened.");
				prependstr(error,_("Warnings encountered while loading:\n"));
				appendstr(error,_("Loaded."));
				return error;
			}
		}
		//*** call up an open dialog for laidout document files
		return newstr("****** open ALMOST works *****");
	} else if (!strncmp(in,"?",1) || !strncmp(in,"help",4)) {
		return newstr(_("The only recognized commands are:\n"
					  " show\n"
					  " newdoc [spec]\n"
					  " open [a laidout document]\n"
					  " help\n"
					  " quit\n"
					  " ?"));
	}

	return newstr(_("You are surrounded by twisty passages, all alike."));
}

