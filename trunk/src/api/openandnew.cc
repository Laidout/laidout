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

#include <lax/fileutils.h>
using namespace LaxFiles;

//------------------------------- Newdoc --------------------------------
/*! \ingroup api */
//StyleDef *makeNewdocStyleDef();
//{***
//	 //define base
//	StyleDef *sd=new StyleDef(NULL,"Newdoc",
//			_("New Document"),
//			_("Create a document and install in the project"),
//			Element_Function,
//			NULL,NULL,
//			NULL,
//			0, //new flags
//			NULL,
//			NewdocFunction);
//
//	 //define parameters
//	sd->push("pages",
//			_("Pages"),
//			_("The number of pages"),
//			Element_Int,
//			NULL, //range
//			"1",  //defvalue
//			0,    //flags
//			NULL);//newfunc
//	sd->push("imposition",
//			_("New Imposition"),
//			_("The new imposition to use"),
//			Element_DynamicEnum, //major hack shortcut
//			NULL, //range
//			"Singles",  //defvalue
//			0,    //flags
//			NULL);//newfunc
//	sd->push("paper",
//			_("Paper"),
//			_("Type of paper to use"),
//			Element_DynamicEnum, ***
//			NULL,
//			"0",
//			0,NULL);
//	sd->push("orientation",
//			_("Orientiation"),
//			_("Either portrait or landscape"),
//			Element_Enum,  ***
//			NULL,
//			"portrait",
//			0,NULL);
//
//	return sd;
//}
//
//
///*! \ingroup api */
//int NewdocFunction(ValueHash *context, 
//					 ValueHash *parameters,
//					 Value **value_ret,
//					 const char **error_ret)
//{***
//	while (isspace(*in)) in++;
//	tmp=newnstr(in,end-in);
//	if (laidout->NewDocument(tmp)==0) appendline(str_ret,_("Document added."));
//	else appendline(str_ret,_("Error adding document. Not added"));
//	delete[] tmp;
//	delete[] word;
//
//}

//------------------------------- Open --------------------------------
/*! \ingroup api */
StyleDef *makeOpenStyleDef()
{
	 //define base
	StyleDef *sd=new StyleDef(NULL,"Open",
			_("Open"),
			_("Open a document"),
			Element_Function,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			OpenFunction);

	 //define parameters
	sd->push("file",
			_("File"),
			_("Path to the file to open"),
			Element_File,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("astemplate",
			_("Open from template"),
			_("Open a copy of the specified file"),
			Element_Boolean,
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
					 char **error_ret)
{
	if (!parameters) return 1;
	if (!context) return 1;

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
			char *dir=newstr(context->findString("current_directory"));
			char *temp;
			if (!dir) {
				temp=get_current_dir_name();
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
		char *error=NULL;
		Document *doc=NULL;
		int loadstatus=0;
		if (astemplate) doc=laidout->LoadTemplate(filename,&error);
		else loadstatus=laidout->Load(filename,&error);
		if (loadstatus>=0 && !doc) {
			 //on a successful load, laidout->curdoc is the document just loaded.
			doc=laidout->curdoc;
		}
		if (!doc) {
			prependstr(error,_("Errors loading:\n"));
			appendstr(error,_("Not loaded."));
			throw error;
		}

		 // create new window only if LoadDocument() didn't create new windows
		 // ***this is a little icky since any previously saved windows might not
		 // actually refer to the document opened
		if (n!=laidout->numTopWindows()) {
			Laxkit::anXWindow *win=newHeadWindow(doc,"ViewWindow");
			if (win) laidout->addwindow(win);
		}
		if (!error) {
			if (error_ret) appendline(*error_ret,_("Opened."));
		} else {
			prependstr(error,_("Warnings encountered while loading:\n"));
			appendstr(error,_("Loaded anyway."));
			if (error_ret) appendline(*error_ret,error);
			delete[] error;
			err=-1;
		}

	} catch (char *str) {
		if (error_ret) appendline(*error_ret,str);
		delete[] str;
		err=1;
	} catch (const char *str) {
		if (error_ret) appendline(*error_ret,str);
		err=1;
	}

	if (filename) delete[] filename;

	if (value_ret) *value_ret=NULL;
	return err;
}



