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

#include <lax/interfaces/dumpcontext.h>
#include <lax/fileutils.h>
#include <lax/refptrstack.cc>
#include "project.h"
#include "utils.h"
#include "version.h"
#include "headwindow.h"
#include "laidout.h"
#include "language.h"

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxInterfaces;
using namespace Laxkit;
using namespace LaxFiles;

//---------------------------- ProjDocument ---------------------------------------
/*! \class ProjDocument
 * \brief Node for document stack in Project.
 *
 * This allows Projects to contain many documents that may not be loaded all at once.
 */

/*! If ndoc!=NULL, its count is incremented.
 */
ProjDocument::ProjDocument(Document *ndoc,char *file,char *nme)
{
	name=newstr(nme);
	filename=newstr(file);
	doc=ndoc;
	if (doc) doc->inc_count();
	is_in_project=1;
}

ProjDocument::~ProjDocument()
{
	if (doc) doc->dec_count();
	if (filename) delete[] filename;
	if (name) delete[] name;
}

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
 * \todo perhaps have count of links to the document. when count==only 1 for project, then
 *       unload the document
 */
/*! \var char *Project::dir
 * \brief Normally the project directory which contains the project file and other project subdirectories.
 *
 * When filename!=NULL, normally dir is just the directory part of filename.
 * If dir==NULL wher filename!=NULL, then there is no specific project directory. Any
 * resources are assumed to be contained in the users laidout directory. This allows 
 * stand alone documents more complicated that single documents, but not so file heavy
 * as whole project directories.
 *
 * \todo finish implementing this!!
 */


Project::Project()
{ 
	name=filename=dir=NULL;
	defaultdpi=300;
}

/*! Flush docs (done manually to catch for debugging purposes).
 */
Project::~Project()
{
	docs.flush();
	if (name) delete[] name;
	if (dir) delete[] dir;
	if (filename) delete[] filename;
}

//! Return whether Laidout is being run project based (nonzero) or document based (0).
/*! Basically, return nonzero if filename is a an existing file.
 */
int Project::valid()
{
	return file_exists(filename,1,NULL)==S_IFREG;
}

/*! Return 0 for success or non-zero for error.
 */
int Project::Push(Document *doc)
{
	if (!doc) return 1;
	docs.push(new ProjDocument(doc,NULL,NULL));
	return 0;
}

int Project::Pop(Document *doc)
{
	if (!doc) { docs.remove(); return 0; }

	int c;
	for (c=0; c<docs.n; c++) if (doc==docs.e[c]->doc) break;
	if (c==docs.n) return 1;
	if (docs.remove(c)) return 0;
	return 1;
}

/*! If doc->saveas in not NULL ***make sure this works right!!!
 * then assume that doc is saved in its own file. Else dump_out
 * the doc.
 *
 * If what==-1, then dump out a pseudocode mockup of the file format.
 */
void Project::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s#You can specify any number of scratch spaces (limbo) that can be attached to views.\n",spc);
		fprintf(f,"%slimbo IdOfLimbo\n",spc);
		fprintf(f,"%s  object ... #each limbo is really just a Group of objects\n\n",spc);

		fprintf(f,"%s# Documents can be included in the project file itself, or\n",spc);
		fprintf(f,"%s# merely referenced, which is usually the more convenient way.\n",spc);
		fprintf(f,"%s# If merely referenced, the line will look like:\n\n",spc);
		fprintf(f,"%sDocument blah.doc\n\n",spc);
		fprintf(f,"%s#and the file, if relative pathname is given, is relative to the project file itself.\n",spc);
		fprintf(f,"%s#Any other paths used by objects in the document, if they are within or under the\n",spc);
		fprintf(f,"%s#project directory (the directory containing the project file), then the paths\n",spc);
		fprintf(f,"%s#are written out as being relative to that project directory.\n\n",spc);

		fprintf(f,"%s#If the document is embedded, it follows the normal Document format, as follows:\n\n",spc);
		fprintf(f,"%sDocument\n",spc);
		if (docs.n) docs.e[0]->doc->dump_out(f,indent+2,-1,NULL);
		else {
			Document d;
			d.dump_out(f,indent+2,-1,NULL);
		}
		return;
	}
	if (limbos.n()) {
		Group *gg;
		for (int c=0; c<limbos.n(); c++) {
			gg=dynamic_cast<Group *>(limbos.e(c));
			fprintf(f,"%slimbo %s\n",spc,(gg->id?gg->id:""));
			//fprintf(f,"%s  object %s\n",spc,limbos.e(c)->whattype());
			limbos.e(c)->dump_out(f,indent+2,0,context);
		}
	}
	if (docs.n) {
		for (int c=0; c<docs.n; c++) {
			fprintf(f,"%sDocument",spc);
			if (docs.e[c]->doc && docs.e[c]->doc->saveas)
				fprintf(f," %s\n",docs.e[c]->doc->saveas);
			else if (docs.e[c]->doc) {
				fprintf(f,"\n");
				docs.e[c]->doc->dump_out(f,indent+2,0,context);
			} else {
				fprintf(f," %s\n",docs.e[c]->filename);
			}
		}
	}

	 //dump_out the window configs..
	laidout->DumpWindows(f,0,NULL);
}

void Project::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
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
				char *file=NULL;
				DumpContext *dump=dynamic_cast<DumpContext *>(context);
				if (dump && dump->basedir) 
					file=full_path_for_file(value,dump->basedir,1);
				Document *doc=new Document;
				if (doc->Load(file?file:value,&error)) Push(doc);
				else {
					delete doc;
					doc=NULL;
					DBG cerr <<"error loading project:"<<(error?error:"(unknown error)")<<endl;
				}
				if (file) delete[] file;
				if (error) delete[] error;
			} else {
				 // assume document is embedded
				Document *doc=new Document;
				doc->dump_in_atts(att->attributes.e[c],flag,context);
				Push(doc);
			}
		} else if (!strcmp(name,"limbo")) {
			Group *g=new Group;  //count=1
			g->dump_in_atts(att->attributes.e[c],flag,context);
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

	char *dir=lax_dirname(filename,0);
	DumpContext context(dir,1);
	dump_in(f,0,0,&context,NULL);
	if (!name) makestr(name,filename);

	fclose(f);
	setlocale(LC_ALL,"");
	return 0;
}

/*! Returns 0 for success or nonzero error.
 */
int Project::Save(char **error_ret)
{
	if (isblank(filename)) {
		if (error_ret) appendline(*error_ret,_("Cannot save to blank file name."));
		DBG cerr <<"**** cannot save, filename is null."<<endl;
		return 2;
	}
	FILE *f=NULL;
	f=fopen(filename,"w");
	if (!f) {
		if (error_ret) 	appendline(*error_ret,_("Cannot open file for writing"));
		DBG cerr <<"**** cannot save project, file \""<<filename<<"\" cannot be opened for writing."<<endl;
		return 3;
	}

	DBG cerr <<"....Saving project to "<<filename<<endl;
	setlocale(LC_ALL,"C");
//	f=stdout;//***
	fprintf(f,"#Laidout %s Project\n",LAIDOUT_VERSION);

	char *dir=lax_dirname(filename,0);
	DumpContext context(dir,1);
	dump_out(f,0,0,&context);
	delete[] dir;
	
	fclose(f);
	setlocale(LC_ALL,"");
	return 0;
}

//! Initialize the directory of filename.
/*! If filename is not a valid path, then it is an error, and 1 is returned.
 * If filename exists, but it is not a regular file, then 2 is returned.
 *
 * On success, 0 is returned.
 */
int Project::initDirs()
{
	if (isblank(filename)) return 1;
	int c=file_exists(filename,1,NULL);
	if (c==S_IFREG) return 0;
	if (c) return 2;
	char *dir=lax_dirname(filename,0);
	check_dirs(dir,1);
	delete[] dir;
	return 0;
}

