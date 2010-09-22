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
// Copyright (C) 2004-2010 by Tom Lechner
//


//----------------<< Builtin Imposition Instances >>--------------------
//
//This file's main purpose is to define GetBuiltinImpositionPool.
//
//To compile in your own imposition types, you must:
// 1. Write code, compile and put its object file in ???
// 2. Include its header here,
// 3. push an initial instance onto existingpool in function GetBuiltinImpositionPool
// 
//--- 



#include "../laidout.h"
#include "utils.h"
#include "configured.h"

#include "../stylemanager.h"
#include "imposition.h"
#include "impositioninst.h"
#include "netimposition.h"
#include "signatures.h"

#include <lax/fileutils.h>
#include <dirent.h>

using namespace Laxkit;
using namespace LaxFiles;

//! Return a new Imposition instance that is like the imposition resource named impos.
/*! \ingroup objects
 * Searches laidout->impositionpool.
 *
 * The imposition returned will have a count of 1.
 */
Imposition *newImpositionByResource(const char *impos)
{
	if (!impos) return NULL;
	int c;
	for (c=0; c<laidout->impositionpool.n; c++) {
		if (!strcmp(impos,laidout->impositionpool.e[c]->name)) {
			return laidout->impositionpool.e[c]->Create();
		}
	}
	return NULL;
}

//! Return a new Imposition instance that is like the imposition resource named impos.
/*! \ingroup objects
 *
 * Returns for "Singles", "Double Sided Singles", "Booklet", "Net".
 *
 * The imposition returned will have a count of 1.
 *
 * \todo *** this needs to be automated!!
 */
Imposition *newImpositionByType(const char *impos)
{
	if (!strcmp(impos,"Singles")) return new Singles;
	//if (!strcmp(impos,"DoubleSidedSingles")) return new DoubleSidedSingles;
	//if (!strcmp(impos,"Booklet")) return new BookletImposition;
	if (!strcmp(impos,"NetImposition")) return new NetImposition;
	if (!strcmp(impos,"SignatureImposition")) return new SignatureImposition;

	return NULL;
}

//--------------------------------- GetBuiltinImpositionPool -------------------------------------

//! Return a stack of defined impositions.
/*! \ingroup pools
 * 
 * If existingpool==NULL, then return a new pool. Otherwise, add to it.
 */
PtrStack<ImpositionResource> *GetBuiltinImpositionPool(PtrStack<ImpositionResource> *existingpool)
{
	 //first install basic imposition styledefs, since they do not otherwise get installed
	 //unless the imposition is instantiated
	StyleDef *def;
	def=stylemanager.FindDef("Singles");
	if (!def) {
		def=makeSinglesStyleDef();
		stylemanager.AddStyleDef(def,1);
	}
	def=stylemanager.FindDef("Signature");
	if (!def) {
		def=makeSignatureImpositionStyleDef();
		stylemanager.AddStyleDef(def,1);
	}
	def=stylemanager.FindDef("NetImposition");
	if (!def) {
		def=makeNetImpositionStyleDef();
		stylemanager.AddStyleDef(def,1);
	}

	 //read in imposition resources from specified directory, and add to stack

	if (!existingpool) existingpool=new PtrStack<ImpositionResource>;
	
	char *globalresourcedir=newstr(SHARED_DIRECTORY);
	char *localresourcedir=newstr(laidout->config_dir);
	char *projectresourcedir=newstr(laidout->project->dir);

	appendstr(localresourcedir,"impositions/");
	appendstr(globalresourcedir,"impositions/");
	if (projectresourcedir) appendstr(globalresourcedir,"impositions/");

	AddToImpositionPool(existingpool,globalresourcedir);
	AddToImpositionPool(existingpool,localresourcedir);
	if (projectresourcedir) AddToImpositionPool(existingpool,projectresourcedir);

	ImpositionResource **rr;
	if (!existingpool->n) {
		 //there were no resources found, so add some built in defaults
		rr=Singles::getDefaultResources();
		if (rr) {
			for (int c=0; rr[c]; c++) existingpool->push(rr[c],1);
			delete rr;
		}

		rr=SignatureImposition::getDefaultResources();
		if (rr) {
			for (int c=0; rr[c]; c++) existingpool->push(rr[c],1);
			delete rr;
		}

		rr=NetImposition::getDefaultResources();
		if (rr) {
			for (int c=0; rr[c]; c++) existingpool->push(rr[c],1);
			delete rr;
		}
	}

	return existingpool;
}

//! Add any imposition resources found in the specified directory to existing pool.
/*! Returns the number of imposition resources added.
 */
int AddToImpositionPool(PtrStack<ImpositionResource> *existingpool, const char *directory)
{
	if (!directory) return 0;

	DIR *dir=opendir(directory);
	if (!dir) return 0;

	int errorcode;
	char *str=NULL;
	struct dirent entry,*result;
	char *name,*desc,*temp=NULL;
	int numadded=0;
	do {
		errorcode=readdir_r(dir,&entry,&result);
		if (!strcmp(entry.d_name,".") || !strcmp(entry.d_name,"..")) continue;
		if (result) {
			if (str) delete[] str;
			str=full_path_for_file(entry.d_name, directory);
			FILE *f=open_laidout_file_to_read(str,"Imposition",NULL);
			if (!f) {
				cerr << " Warning! Non imposition file in imposition resource directory " <<endl
					 << "   file: "<<entry.d_name<<"   directory: "<<directory<<endl;
				continue;
			}

			resource_name_and_desc(f,&name,&desc);
			if (isblank(name)) temp=make_id("imposition");
			numadded++;
			existingpool->push(new ImpositionResource(NULL,   //styledef name
													  name?name:temp,//instance name
													  str,    //filename
													  desc,//desc
													  NULL,0) //attribute
							  );
			if (temp) delete[] temp; temp=NULL;
			if (name) delete[] name; name=NULL;
			if (desc) delete[] desc; desc=NULL;
			fclose(f);
		}
	} while (result);
	if (str) delete[] str;
	closedir(dir);

	return numadded;
}

