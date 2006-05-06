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

#include "project.h"
#include "version.h"

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
 */
//class Project : public LaxFiles::DumpUtility
//{
// public:
//	char *name,*filename;
//	Laxkit::PtrStack<Document> docs;
//	
//	//StyleManager styles;
//	//Page scratchboard;
//	//Laxkit::PtrStack<char> project_notes;
//
//	Project();
//	virtual ~Project();
//
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//	virtual int Load(const char *file);
//	virtual int Save();
//};

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
 */
void Project::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (docs.n) {
		for (int c=0; c<docs.n; c++) {
			fprintf(f,"%sdocument",spc);
			if (docs.e[c]->saveas) fprintf(f," %s\n",docs.e[c]->saveas);
			else {
				fprintf(f,"\n");
				docs.e[c]->dump_out(f,indent+2,0);
			}
		}
	}
	//*** dump_out the window configs..
}

void Project::dump_in_atts(LaxFiles::Attribute *att)
{
}

/*! Returns 0 for success or nonzero error.
 */
int Project::Load(const char *file)
{
	return 1;
}

/*! Returns 0 for success or nonzero error.
 */
int Project::Save()
{
	if (!filename || !strcmp(filename,"")) {
		DBG cout <<"**** cannot save, filename is null."<<endl;
		return 2;
	}
	FILE *f=NULL;
	f=fopen(filename,"w");
	if (!f) {
		DBG cout <<"**** cannot save project, file \""<<filename<<"\" cannot be opened for writing."<<endl;
		return 3;
	}

	DBG cout <<"....Saving project to "<<filename<<endl;
//	f=stdout;//***
	fprintf(f,"#Laidout %s Project\n",LAIDOUT_VERSION);
	dump_out(f,0,0);
	
	fclose(f);
	return 0;
}

