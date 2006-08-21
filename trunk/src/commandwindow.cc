//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//


#include "commandwindow.h"
#include "headwindow.h"
#include "laidout.h"

#include <lax/fileutils.h>
#include <sys/stat.h>

using namespace LaxFiles;


/*! \class CommandWindow
 * \brief Command line input. Future home of interactive interpreter.
 */
//class CommandWindow : public Laxkit::PromptEdit
//{
// protected:
//	virtual char *process(const char *in);
// public:
// 	CommandWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
// 		int xx,int yy,int ww,int hh,int brder);
// 	virtual const char *whattype() { return "CommandWindow"; }
//	virtual ~CommandWindow();
//};

//! Set pads to 6.
CommandWindow::CommandWindow(Laxkit::anXWindow *parnt,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder)
	: PromptEdit(parnt,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,None,NULL)
{
	padx=pady=6;
	dir=NULL;
}

//! Empty destructor.
CommandWindow::~CommandWindow()
{
	if (dir) delete[] dir;
}

/*! Take a whole command line, and process it, returning the command's text output.
 *
 * Must return a new'd char[].
 */
char *CommandWindow::process(const char *in)
{
	if (!in) return NULL;
	while (isspace(*in)) in++;
	if (!strncmp(in,"quit",4)) {
		app->quit();
		return newstr("");
	} else if (!strncmp(in,"newdoc",6)) {
		if (!isalnum(in[6])) {
			in+=6;
			while (isspace(*in)) in++;
			if (laidout->NewDocument(in)==0) return newstr("Document added.");
			else return newstr("Error adding document. Not added");
		}
	} else if (!strncmp(in,"show",4)) {
		char *temp=newstr("Project: "), temp2[10];
		if (laidout->project->name) appendstr(temp,laidout->project->name);
		else appendstr(temp,"(untitled)");
		appendstr(temp,"\n");
		for (int c=0; c<laidout->project->docs.n; c++) {
			sprintf(temp2," %d. ",c+1);
			appendstr(temp,temp2);
			appendstr(temp,laidout->project->docs.e[c]->Name());
			appendstr(temp,"\n");
		}
		return (temp?temp:newstr("No documents in project."));
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
				if (in==NULL) return newstr("Could not load that.");

				if (laidout->findDocument(in)) return newstr("That document is already loaded.");
				Document *doc=laidout->LoadDocument(in);
				if (!doc) return newstr("Could not load that.");

				anXWindow *win=newHeadWindow(doc,"ViewWindow");
				if (win) app->addwindow(win);
				return newstr("Opened.");
			}
		}
		//*** call up an open dialog for laidout document files
		return newstr("****** open ALMOST works *****");
	} else if (!strncmp(in,"?",1) || !strncmp(in,"help",4)) {
		return newstr("The only recognized commands are:\n"
					  "newdoc [spec]\n"
					  "open [a laidout document]\n"
					  "help\n"
					  "quit\n"
					  "?");
	}

	return newstr("You are surrounded by twisty passages, all alike.");
}

