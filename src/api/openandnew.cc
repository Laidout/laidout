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


#include "openandnew.h"
#include "../language.h"
#include "../laidout.h"
#include "../headwindow.h"
#include "../papersizes.h"

#include <lax/fileutils.h>
#include <unistd.h>


using namespace LaxFiles;
using namespace Laxkit;


namespace Laidout {

//------------------------------- Newdoc --------------------------------
/*! \ingroup api */
ObjectDef *makeNewDocumentObjectDef()
{
	 //define base
	ObjectDef *sd=new ObjectDef(NULL,"NewDocument",
			_("New Document"),
			_("Create a document and install in the project"),
			"function",
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			NewDocumentFunction);

	 //define parameters
	sd->push("pages",
			_("Pages"),
			_("The number of pages"),
			"int",
			NULL, //range
			"1",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("imposition",
			_("New Imposition"),
			_("The new imposition to use"),
			"any",
			NULL, //range
			"Singles",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("paper",
			_("Paper"),
			_("Type of paper to use"),
			"any",
			NULL,
			"0",
			0,NULL);
	sd->pushEnum("orientation",
			_("Orientiation"),
			_("Either portrait (0) or landscape (1)"),
			"portrait",
			NULL,NULL,
			"portrait", _("Portrait"), _("Long direction is vertical"),
			"landscape", _("Landscape"), _("Short direction is vertical"),
			NULL
		);
	sd->push("saveas",
			_("Save as"),
			_("The path to save the document to, if any. It is not saved immediately."),
			"string",
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	//sd->pushEnum("colormode",...);
	//sd->push("defaultdpi",...);
	//sd->push("defaultunits",...);

	return sd;
}


//! Create a new document from values in parameters.
/*! \ingroup api
 * \todo should load default template sometimes
 */
int NewDocumentFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 ErrorLog &log)
{
	if (!parameters) {
		if (value_ret) *value_ret=NULL;
		log.AddMessage(_("Easy for you to say!"),ERROR_Fail);
		return 1;
	}


	int err=0;
	Document *doc=NULL;

	const char *saveas=NULL;
	int orientation=0;
	Imposition *imp=NULL;
	PaperStyle *paper=NULL;
	int numpages=1;

	try {
		Value *v;
		int i;


		 //----saveas
		saveas=parameters->findString("saveas");
		if (saveas) {
			if (file_exists(saveas,1,NULL)!=0) throw _("File exists already. Please remove first.");
			if (laidout->findDocument(saveas)) throw _("Document already loaded");
		}


		 //----pages
		numpages=parameters->findInt("pages",-1,&i);
		if (i==2 || (i==0 && numpages<=0)) throw _("Invalid number of pages!");
		else if (i==1) numpages=1;


		 //----imposition
		v=parameters->find("imposition");
		if (v) {
			if (v->type()==VALUE_String) {
				const char *str=(const char *)dynamic_cast<StringValue*>(v);
				if (!str) throw _("Invalid object for imposition!");

				for (int c=0; c<laidout->impositionpool.n; c++) {
					if (!strcmp(laidout->impositionpool.e[c]->name,str)) {
						imp=laidout->impositionpool.e[c]->Create();
						break;
					}
				}
			} else if (v->type()==VALUE_Object) {
				imp=dynamic_cast<Imposition*>(dynamic_cast<ObjectValue*>(v));
				if (imp) imp->inc_count();
			}
			if (!imp) throw _("Invalid object for imposition!");
		}


		 //----paper
		v=parameters->find("paper");
		if (v) {
			if (v->type()==VALUE_String) {
				const char *str=dynamic_cast<StringValue*>(v)->str;
				if (!str) throw _("Invalid object for paper!");
				for (int c=0; c<laidout->papersizes.n; c++) {
					if (strcmp(str,laidout->papersizes.e[c]->name)==0) {
						paper=(PaperStyle*)laidout->papersizes.e[c]->duplicate();
						break;
					}
				}
			
			} else if (v->type()==VALUE_Object) {
				paper=dynamic_cast<PaperStyle*>(dynamic_cast<ObjectValue*>(v));
				if (!paper) throw _("Invalid object for paper!");
				paper=(PaperStyle*)paper->duplicate();
			}
		}


		 //----orientation
		orientation=parameters->findInt("orientation",-1,&i);
		if (i==2) throw _("Invalid format for orientation!");


		// todo:
		 //----colormode
		 //----defaultdpi
		 //----defaultunits


		if (!paper) throw _("Missing a paper type!");
		if (!imp) throw _("Missing an imposition type!");

		 //correct orientation if necessary
		if (orientation && (paper->flags&1)==0) paper->flags|=1;
		else if (!orientation && (paper->flags&1)!=0) paper->flags&=~1;

		 //finally, create the document
		imp->NumPages(numpages);
		imp->SetPaperSize(paper);
		//doc=new Document(imp,paper);
		if (laidout->NewDocument(imp,saveas)!=0) throw _("Error creating document instance!");

		 //clean up
		imp->dec_count();
		paper->dec_count();

	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		if (imp) { imp->dec_count(); imp=NULL; }
		if (paper) { paper->dec_count(); paper=NULL; }
		err=1;
	} catch (char *str) {
		log.AddMessage(str,ERROR_Fail);
		delete[] str;
		if (imp) { imp->dec_count(); imp=NULL; }
		if (paper) { paper->dec_count(); paper=NULL; }
		err=1;
	}



	if (value_ret) {
		if (doc) *value_ret=new ObjectValue(doc);
		else *value_ret=NULL;
	}
	if (doc) doc->dec_count();

	//laidout->NewDocument(tmp)
	if (err==0) log.AddMessage(_("Document added."),ERROR_Ok);
	else log.AddMessage(_("Error adding document. Not added"),ERROR_Fail);

	return err;
}

//------------------------------- Open --------------------------------
/*! \ingroup api */
ObjectDef *makeOpenObjectDef()
{
	 //define base
	ObjectDef *sd=new ObjectDef(NULL,"Open",
			_("Open"),
			_("Open a document"),
			"function",
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			OpenFunction);

	 //define parameters
	sd->push("file",
			_("File"),
			_("Path to the file to open"),
			"string",
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("astemplate",
			_("Open from template"),
			_("Open a copy of the specified file"),
			"boolean",
			NULL, //range
			"no",  //defvalue
			0,    //flags
			NULL);//newfunc

	return sd;
}

/*! \ingroup api
 * Return 0 for success, -1 for success with warnings, or 1 for unredeemable failure.
 */
int OpenFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 ErrorLog &log)
{
	if (!parameters) {
		if (value_ret) *value_ret=NULL;
		log.AddMessage(_("Easy for you to say!"),ERROR_Fail);
		return 1;
	}

	int astemplate=parameters->findInt("astemplate");
	int err=0;

	char *filename=NULL;
	try {
		const char *file=parameters->findString("file");

		 // get filename potentially
		if (!file) throw _("Missing filename.");
		filename=newstr(file);

		 // try to open up the file
		if (*filename!='/' && strstr("file://",filename)!=filename) {
			 //assume a relative path. find a full file name
			char *dir=context?newstr(context->findString("current_directory")):NULL;
			char *temp;
			if (!dir) {
				//temp=get_current_dir_name(); <- GNU only, breaks mac build!
				temp=getcwd(NULL,0);
				dir=newstr(temp);
				free (temp);
			}
			appendstr(dir,"/");
			appendstr(dir,filename);
			delete[] filename;
			filename=dir;
			dir=NULL;
		}
		if (file_exists(filename,1,NULL)!=S_IFREG) throw _("Could not load that.");

		if (!astemplate && laidout->findDocument(filename)) throw _("Document already loaded");

		 //now load the document, creating a new view window if the document load
		 //does not create any new windows of its own
		int n=laidout->numTopWindows();
		Document *doc=NULL;
		int loadstatus=0;
		if (astemplate) doc=laidout->LoadTemplate(filename,log);
		else loadstatus=laidout->Load(filename,log);
		if (loadstatus>=0 && !doc) {
			 //on a successful load, laidout->curdoc is the document just loaded.
			doc=laidout->curdoc;
		}
		if (!doc) {
			throw _("Errors loading. Not loaded.");
		}

		 // create new window only if LoadDocument() didn't create new windows
		 // ***this is a little icky since any previously saved windows might not
		 // actually refer to the document opened
		if (n!=laidout->numTopWindows()) {
			Laxkit::anXWindow *win=newHeadWindow(doc,"ViewWindow");
			if (win) laidout->addwindow(win);
		}
		if (!log.Warnings()) {
			log.AddMessage(_("Opened."),ERROR_Ok);
		} else {
			log.AddMessage(_("Warnings encountered while loading. Loaded anyway."),ERROR_Warning);
			err=-1;
		}

	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	} catch (char *str) {
		log.AddMessage(str,ERROR_Fail);
		delete[] str;
		err=1;
	}

	if (filename) delete[] filename;

	if (value_ret) *value_ret=NULL;
	return err;
}



} // namespace Laidout

