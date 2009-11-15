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


#include "importexport.h"
#include "../language.h"
#include "../laidout.h"
#include "../headwindow.h"

#include <lax/fileutils.h>
using namespace LaxFiles;




//
// maybe have:
//  > export(doc='thing.laidout', tofile="blah.*", SvgExportFilter())
//  > import(file="blah.sla", laidout.docs.0)
//  > import(file="blah.sla", ScribusImportFilter())
//
//





//------------------------------- Newdoc --------------------------------
/*! \ingroup api */
StyleDef *makeImportStyleDef();
{***
	 //define base
	StyleDef *sd=new StyleDef(NULL,"Import",
			_("Import"),
			_("Import a vector file to an existing document or a group"),
			Element_Function,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			ImportFunction);

	//char *filename;
	//int keepmystery;
	//int instart,inend;
	//int topage,spread,layout;
	//Document *doc;
	//Group *toobj;
	//ImportFilter *filter;
	//double dpi;

	 //define parameters
	sd->push("file",
			_("File"),
			_("Path to file to import"),
			Element_Int,
			NULL, //range
			"1",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("keepmystery",
			_("Keep mystery data"),
			_("Whether to attempt to preserve things not understood"),
			Element_Enum, //no|sometimes|always
			NULL, //range
			"Singles",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("instart",
			_("Start"),
			_("First page to import from a multipage file"),
			Element_Int,
			NULL,
			NULL,
			0,NULL);
	sd->push("topage",
			_("To page"),
			_("The page in the existing document to begin importing to"),
			Element_Int,
			NULL,
			NULL,
			0,NULL);
	sd->push("dpi",
			_("Default dpi"),
			_("Default dpi to use while importing if necessary"),
			Element_Int,
			NULL,
			NULL,
			0,NULL);
	sd->push("spread",
			_("Spread"),
			_("Index of the spread to import to"),
			Element_Int,
			NULL,
			NULL,
			0,NULL);
	sd->push("document",
			_("Document"),
			_("Which document to import to, if not importing to a group"),
			Element_Document,
			NULL,
			NULL,
			0,NULL);
	sd->push("group",
			_("Group"),
			_("Group to import to, if not importing to a document"),
			Element_Group,
			NULL,
			NULL,
			0,NULL);
	sd->push("filter",
			_("Filter"),
			_("The import filter to use to import"),
			Element_ImportFilter,
			NULL,
			NULL,
			0,NULL);

	return sd;
}


/*! \ingroup api */
int ImportFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 const char **error_ret)
{***
	while (isspace(*in)) in++;
	tmp=newnstr(in,end-in);
	if (laidout->NewDocument(tmp)==0) appendline(str_ret,_("Document added."));
	else appendline(str_ret,_("Error adding document. Not added"));
	delete[] tmp;
	delete[] word;

}

//------------------------------- Open --------------------------------
/*! \ingroup api */
StyleDef *makeExportStyleDef()
{
	 //define base
	StyleDef *sd=new StyleDef(NULL,"ExportDocument",
			_("Export"),
			_("Export a document"),
			Element_Function,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			ExportFunction);


	 //define parameters
	sd->push("target",
			_("Output target"),
			_("Whether to try to save to a single file, multiple files, or send results to a shell command"),
			Element_File,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("start",
			_("Start"),
			_("Starting page of the document to export"),
			Element_Int,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("end",
			_("End"),
			_("Ending page of the document to export"),
			Element_Int,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("layout",
			_("Layout"),
			_("Type of layout to export as. Possibilities defined by the imposition."),
			Element_DynamicEnum, ***
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("doc",
			_("Document"),
			_("The document to export, if not exporting a group."),
			Element_****,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("group",
			_("Group"),
			_("Group to export, if not exporting a document."),
			Element_Boolean,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("filename",
			_("Filename"),
			_("Path of exported file. For multiple files, use \"file##.svg\", for instance."),
			Element_String,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("collect",
			_("Collect for out"),
			_("Whether to copy all the accessed resources to the same directory as the exported file."),
			Element_Boolean,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("papergroup",
			_("Paper group"),
			_("The paper group to export onto. Do not include if you want to use the default paper group."),
			Element_***,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("filter",
			_("Filter"),
			_("The filter to export with."),
			Element_DynamicEnum,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc

	return sd;
}

/*! \ingroup api
 * Return 0 for success, -1 for success with warnings, or 1 for unredeemable failure.
 */
int ExportFunction(ValueHash *context, 
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


