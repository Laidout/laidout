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
// Copyright (C) 2004-2011 by Tom Lechner
//


// This file is for LaidoutApp functions that are not really involved in basic application functioning,
// or initial window creation.

#include "laidout.h"
#include "viewwindow.h"
#include "spreadeditor.h"
#include "headwindow.h"
#include "version.h"
#include "importimage.h"

#ifndef LAIDOUT_NOGL
#include "impositions/polyptychwindow.h"
#endif
#include "impositions/signatureinterface.h"

#include <lax/fileutils.h>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {


/*! \enum TreeChangeType
 * \ingroup misc
 * \brief Type for what in the doc tree has changed. See TreeChangeEvent.
 */

/*! \class TreeChangeEvent
 * \brief Event class to tell various windows what has been altered.
 *
 * Windows will receive this event after the changes occur, but potentially also after
 * the window wants to access a possibly altered tree. This should not 
 * crash the program because of reference counting, but the windows must verify all the
 * relevant references when they receive such an event.
 *
 * \todo why is enum TreeChangeType not turning into a doxygen doc??
 * TreeChangeType:
 * <pre>
 *	TreeDocGone,
 *	TreePagesAdded,
 *	TreePagesDeleted,
 *	TreePagesMoved,
 *	TreeObjectRepositioned,
 *	TreeObjectReorder,
 *	TreeObjectDiffPage,
 *	TreeObjectDeleted,
 *	TreeObjectAdded
 * </pre>
 */

TreeChangeEvent::TreeChangeEvent(const TreeChangeEvent &te)
{
	start=te.start;
	end=te.end;
	changer=te.changer;
	changetype=te.changetype;
	obj.doc=te.obj.doc;
	type=te.type;
	subtype=te.subtype;
	send_time=te.send_time;
	makestr(send_message,te.send_message);
	propagate=0;
}


//! Tell all ViewWindow, SpreadEditor, and other main windows that the doc has changed.
/*! Sends a TreeChangeEvent to all SpreadEditor and ViewWindow panes in each top level HeadWindow.
 *
 * \todo *** should probably replace s and e with a FieldPlace: doc,page,obj,obj,...
 */
void LaidoutApp::notifyDocTreeChanged(Laxkit::anXWindow *callfrom,TreeChangeType change,int s,int e)
{
	DBG cerr<<"notifyDocTreeChanged sending.."<<endl;
	HeadWindow *h;
	PlainWinBox *pwb;
	ViewWindow *view;
	SpreadEditor *se;
	anXWindow *w;

	TreeChangeEvent *edata;
	TreeChangeEvent *te=new TreeChangeEvent;

	te->send_message=newstr("docTreeChange");
	te->changer=callfrom;
	te->changetype=change;
	te->start=s;
	te->end=e;
	 
	int c2,yes=0;
	for (int c=0; c<topwindows.n; c++) {
		if (callfrom==topwindows.e[c]) continue; // this hardly has effect as most are children of top
		h=dynamic_cast<HeadWindow *>(topwindows.e[c]);
		if (!h) continue;
		
		for (c2=0; c2<h->numwindows(); c2++) {
			pwb=h->windowe(c2);
			w=pwb->win();
			if (!w || callfrom==w) continue;
			view=dynamic_cast<ViewWindow *>(w);
			if (view) yes=1;
			else if (se=dynamic_cast<SpreadEditor *>(w), se) yes=1;

			 //construct events for the panes
			if (yes){
				edata=new TreeChangeEvent(*te);
				edata->to=w->object_id;

				app->SendMessage(edata,w->object_id,"docTreeChange");

				DBG cerr <<"---(notifyDocTreeChanged) sending docTreeChange to "<<
				DBG 	(w->WindowTitle())<< "("<<w->whattype()<<")"<<endl;
				yes=0;
			}
		}
	}
	delete te; //delete template object
	DBG cerr <<"eo notifyDocTreeChanged"<<endl;
	return;
}

//! Make sure various windows respond to a change in global preferences, like default units change.
void LaidoutApp::notifyPrefsChanged(Laxkit::anXWindow *callfrom,int what)
{
	DBG cerr<<"notifyPrefsChanged sending.."<<endl;
	HeadWindow *h;
	PlainWinBox *pwb;
	anXWindow *w;

	SimpleMessage *edata;

	int c2,yes=1;
	for (int c=0; c<topwindows.n; c++) {
		if (callfrom==topwindows.e[c]) continue;
		h=dynamic_cast<HeadWindow *>(topwindows.e[c]);
		if (!h) continue;
		
		for (c2=0; c2<h->numwindows(); c2++) {
			pwb=h->windowe(c2);
			w=pwb->win();
			if (!w || callfrom==w) continue;

			 //construct events for the panes
			if (yes){
				edata=new SimpleMessage();
				edata->send_message=newstr("prefsChange");
				edata->info1=what;
				edata->to=w->object_id;

				app->SendMessage(edata,w->object_id,"prefsChange");

				DBG cerr <<"---(notifyPrefsChanged) sending prefsChange to "<<
				DBG 	(w->WindowTitle())<< "("<<w->whattype()<<")"<<endl;
			}
		}
	}
	DBG cerr <<"eo notifyPrefsChanged"<<endl;
	return;
}


//! Dump out a pseudocode mockup of everything that can appear in a Laidout document.
/*! 
 * The basic file of a Laidout document is a Project file, which references several
 * Document files, or just a Document file itself. 
 *
 * All the info is basically generated by passing what==-1 to all the dump_out() funtions that
 * are set up.
 *
 * If file existed already, then the format is written to the next available file by incerementing
 * a number before the final extension of file. 
 * 
 * See also createlaidoutrc(). Returns 0 if format dumped, or 1 if nothing dumped.
 */
int LaidoutApp::dump_out_file_format(const char *file, int nooverwrite)
{
	if (isblank(file)) return 1;
	FILE *f;
	if (!strcmp(file,"-")) {
		f=stdout;
	} else {
		if (file_exists(file,1,NULL)) {
			 //move existing file to somewhere else
			char *incfile,*tmp=newstr(file);
			while (1) {
				incfile=increment_file(tmp);
				delete[] tmp;
				tmp=incfile;
				
				if (!file_exists(incfile,1,NULL)) {
					f=fopen(incfile,"w");
					delete[] incfile;
					break;
				}
			}
		} else f=fopen(file,"w");
	}
	if (!f) return 1;

	setlocale(LC_ALL,"C");
	fprintf(f,"#Laidout %s File Formats\n\n",LAIDOUT_VERSION);

	fprintf(f," # This file describes:\n"
			  " #  1. Project files, which contain almost all of the possible Laidout elements\n"
			  " #  2. a Laidout Image List file.\n"
			  " # The laidoutrc file in ~/.laidout/(version)/laidoutrc documents itself.\n"
			  "\n"
			  " # Throughout this file format, data is grouped according to how much it is indented,\n"
			  " # reminiscent of python, and the yaml file format. The format here is kind of a simplified\n"
			  " # yaml. Here, there are basically a name and value (together called an attribute), plus any number\n"
			  " # of subattributes. A value can be on the same line as the name, or it can span several lines.\n"
			  " # If you write \"thename \\\" on one line, then the next several lines that are more indented\n"
			  " # than the name line contain the value. Always remember to use spaces for\n"
			  " # indents. NEVER tabs, because they cause too much confusion. Comments begin with a '#'\n"
			  " # character, and go until the end of the line. If you have a value that has such a character\n"
			  " # in it, then simply put double quotes around the value. Finally, everything is case sensitive.\n"
			  "\n\n"
			  "#----------------- Laidout Project files ------------------\n\n"
			  " # The broadest file of a Laidout document is a Project file,\n"
			  " # which contains 0 or more Document files, plus other resources.\n"
			  " # For a project, the first line of the file will be (including the '#'):\n\n");
	fprintf(f,"#Laidout %s Project\n\n",LAIDOUT_VERSION);
	fprintf(f," # followed by all the project attributes. A stand alone Document file will start:\n\n");
	fprintf(f,"#Laidout %s Document\n\n",LAIDOUT_VERSION);
	fprintf(f," # followed by Document attributes. This pattern follows for most fragments of\n"
			  " # Laidout elements, so for instance, a stand alone Laidout palette file will start:\n");
	fprintf(f," # \"#Laidout %s Palette\"",LAIDOUT_VERSION);
	fprintf(f," and will then continue with only palette attributes. Many resources, including\n"
			  " # palettes, and window arrangements, can appear at the Project level or the Document level.\n"
			  "\n"
			  " # Without further ado, here are the actual elements, starting at the Project level:\n\n"
			  "#----------------- (starts on next line) ------------\n");
	fprintf(f,"#Laidout %s Project\n",LAIDOUT_VERSION);
	
	if (project) project->dump_out(f,0,-1,NULL);
	else {
		Project p;
		p.dump_out(f,0,-1,NULL);
	}
	fprintf(f,"\n");

	HeadWindow h(NULL,"","",0, 0,0,0,0,0);
	fprintf(f,"#Window arrangements can be dumped out. These can be project attributes or Document\n"
			  "#attributes. If you are working on a project, not just a single Document,\n"
			  "#then the window attributes in a Document file are ignored when the project is loaded,\n"
			  "#and forgotten when the document is next saved..\n"
			  "#If there are no window blocks, then a default window with a view is created.\n"
			  "\n"
			  "window\n");
	h.dump_out(f,2,-1,NULL);
	
	fprintf(f,"\n\n\n#----------------- a Laidout Image List file ------------------\n");
	dumpOutImageListFormat(f);
	
	if (strcmp(file,"-")) fclose(f);
	setlocale(LC_ALL,"");

	return 0;
}

/*! By default, shortcuts per area only get defined when the are actually needed. This function
 * causes all the areas to initialize shortcuts.
 */
void LaidoutApp::InitializeShortcuts()
{
	static int initialized=0;
	if (initialized) return;
	initialized=1;

	 //for each head window pane
	HeadWindow h(NULL,"","",0, 0,0,0,0,0);
	h.InitializeShortcuts();


	 //for each resource helper dialogs
	SignatureInterface si;
	si.GetShortcuts();

#ifndef LAIDOUT_NOGL
	PolyptychWindow w(NULL,NULL,0,NULL);
	w.GetShortcuts();
#endif


	 //for each interface
	for (int c=0; c<interfacepool.n; c++) {
		interfacepool.e[c]->GetShortcuts(); //this will install in shortcutmanager if it is not already there
	}
}

//! Dump the list of known bound shortcuts to f with indentation.
int LaidoutApp::dump_out_shortcuts(FILE *f, int indent, int how)
{
	if (!f) return 1;

	//Strategy is to ensure all areas are defined within ShortcutManager
	//then use ShortcutManager->dump_out()

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (how==0) {
		fprintf(f,"%s#\n",spc);
		fprintf(f,"%s# Laidout %s Shortcuts\n",spc,LAIDOUT_VERSION);
		fprintf(f,"%s#\n",spc);
	}

	InitializeShortcuts();

	ShortcutManager *manager=GetDefaultShortcutManager();
	if (how==0) manager->dump_out(f,indent,0,NULL);
	else manager->SaveHTML("-");
	return 0;
}


} // namespace Laidout

