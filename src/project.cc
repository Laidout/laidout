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
// Copyright (C) 2004-2007 by Tom Lechner
//

#include "project.h"
#include "utils.h"
#include "version.h"
#include "headwindow.h"
#include "laidout.h"

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

//---------------------------- Project ---------------------------------------
/*! \class Project
 * \brief Class holding several documents, as well as various other settings.
 *
 * When laidout is opened and a new document is started, everything goes into
 * the default Project. The project maintains the scratchboard, any project notes,
 * the documents of the project, default directories, and other little tidbits
 * the user might care to associate with the project.
 *
 * Also, the Project maintains its own StyleManager***??????
 *
 * \todo *** implement a filelist, rather than stack of Document instances.. each filelist
 *   element would have:
 *     path to the document,
 *     pointer to document if loaded,
 *     title of document, should be same as name in document
 *     count of links to the document. when count==only 1 for project, then
 *       unload the document
 */


//! Constructor, just set name=filename=NULL.
Project::Project()
{ 
	name=filename=NULL;
}

/*! Flush docs (done manually to catch for debugging purposes).
 */
Project::~Project()
{
	docs.flush();
	if (name) delete[] name;
	if (filename) delete[] filename;
}

/*! If doc->saveas in not NULL ***make sure this works right!!!
 * then assume that doc is saved in its own file. Else dump_out
 * the doc.
 *
 * If what==-1, then dump out a pseudocode mockup of the file format.
 */
void Project::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s# Documents can be included in the project file itself, or\n",spc);
		fprintf(f,"%s# merely referenced, which is usually the more convenient way.\n",spc);
		fprintf(f,"%s# If merely referenced, the line will look like:\n\n",spc);
		fprintf(f,"%sDocument blah.doc\n\n",spc);
		fprintf(f,"%s#and the file, if relative pathname is given, is relative to the project file itself.\n\n",spc);
		fprintf(f,"%s#You can specify any number of scratch spaces (limbo) that can be attached to views.\n",spc);
		fprintf(f,"%slimbo IdOfLimbo\n",spc);
		fprintf(f,"%s  object ... #each limbo is really just a Group of objects\n\n",spc);
		fprintf(f,"%s#Moving on, there can be any number of Documents, in the following format:\n\n",spc);
		fprintf(f,"%sDocument\n",spc);
		if (docs.n) docs.e[0]->dump_out(f,indent+2,-1);
		else {
			Document d;
			d.dump_out(f,indent+2,-1);
		}
		return;
	}
	if (limbos.n()) {
		Group *gg;
		for (int c=0; c<limbos.n(); c++) {
			gg=dynamic_cast<Group *>(limbos.e(c));
			fprintf(f,"%slimbo %s\n",spc,(gg->id?gg->id:""));
			//fprintf(f,"%s  object %s\n",spc,limbos.e(c)->whattype());
			limbos.e(c)->dump_out(f,indent+2,0);
		}
	}
	if (docs.n) {
		for (int c=0; c<docs.n; c++) {
			fprintf(f,"%sDocument",spc);
			if (docs.e[c]->saveas) fprintf(f," %s\n",docs.e[c]->saveas);
			else {
				fprintf(f,"\n");
				docs.e[c]->dump_out(f,indent+2,0);
			}
		}
	}
	//*** dump_out the window configs..
}

void Project::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	if (!att) return;
	char *name,*value;
	char *error=NULL;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"Document")) {
			if (att->attributes.e[c]->attributes.n==0) {
				 // assume file name is value
				if (isblank(value)) continue;
				Document *doc=new Document;
				if (doc->Load(value,&error)) docs.push(doc);
				else {
					delete doc;
					doc=NULL;
					DBG cerr <<"error loading project:"<<(error?error:"(unknown error)")<<endl;
				}
				if (error) delete[] error;
			} else {
				 // assume document is embedded
				Document *doc=new Document;
				doc->dump_in_atts(att->attributes.e[c],flag);
				docs.push(doc);
			}
		} else if (!strcmp(name,"limbo")) {
			Group *g=new Group;  //count=1
			g->dump_in_atts(att->attributes.e[c],flag);
			if (isblank(g->id) && !isblank(value)) makestr(g->id,value);
			limbos.push(g,0); // incs count
			g->dec_count();   //remove extra first count
		}
	}

	 // search for windows to create after reading in everything else
	HeadWindow *head;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"window")) {
			head=static_cast<HeadWindow *>(newHeadWindow(att->attributes.e[c]));
			if (head) laidout->addwindow(head);
		}
	}
}

/*! \todo imp me...
 */
int Project::clear()
{
	cerr << " *** must implement Project::clear()"<<endl;
	return 0;

}

/*! Returns 0 for success or nonzero error.
 */
int Project::Load(const char *file,char **error_ret)
{
	FILE *f=open_laidout_file_to_read(file,"Project",error_ret);
	if (!f) return 1;
	
	clear();
	makestr(filename,file);
	setlocale(LC_ALL,"C");
	dump_in(f,0,0,NULL);
	if (!name) makestr(name,filename);
	fclose(f);
	setlocale(LC_ALL,"");
	return 0;
}

/*! Returns 0 for success or nonzero error.
 */
int Project::Save(char **error_ret)
{
	if (!filename || !strcmp(filename,"")) {
		DBG cerr <<"**** cannot save, filename is null."<<endl;
		return 2;
	}
	FILE *f=NULL;
	f=fopen(filename,"w");
	if (!f) {
		DBG cerr <<"**** cannot save project, file \""<<filename<<"\" cannot be opened for writing."<<endl;
		return 3;
	}

	DBG cerr <<"....Saving project to "<<filename<<endl;
	setlocale(LC_ALL,"C");
//	f=stdout;//***
	fprintf(f,"#Laidout %s Project\n",LAIDOUT_VERSION);
	dump_out(f,0,0);
	
	fclose(f);
	setlocale(LC_ALL,"");
	return 0;
}

