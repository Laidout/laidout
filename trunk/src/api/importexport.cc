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





//------------------------------- Import --------------------------------
/*! \ingroup api */
StyleDef *makeImportStyleDef()
{
	 //define base
	StyleDef *sd=new StyleDef(NULL,"Import",
			_("Import"),
			_("Import a vector file to an existing document or a group. Each filter may have "
			  "additional options you can pass in. Usually the file format will be detected "
			  "automatically, but you may also force a specific filter to be used."),
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
	sd->push("filename",
			_("Filename"),
			_("Path to file to import"),
			Element_Int,
			NULL, //range
			"1",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("filter",
			_("Filter"),
			_("The import filter to use to import"),
			Element_Any,
			NULL,
			NULL,
			0,NULL);

	return sd;
}


//! Process an import command and try to import a document
/*! \ingroup api */
int ImportFunction(ValueHash *context, 
					 ValueHash *parameters,
					 Value **value_ret,
					 char **error_ret)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=NULL;
		if (error_ret) appendline(*error_ret,_("Easy for you to say!"));
		return 1;
	}

	int err=0;
	char *filename=NULL;
	ImportFilter *filter=NULL;
	try {
		 // get file to import
		const char *file=parameters->findString("filename");
		if (!file) throw _("Missing filename.");
		filename=newstr(file);


		 //determine what filter to use
		int v=parameters->findIndex("filter");
		if (v>=0) {
			const char *filtername=parameters->findString("filter",v);
			if (filtername) {
				int c2;
				if (filtername) {
					for (c2=0; c2<laidout->importfilters.n; c2++) {
						if (!strcmp(laidout->importfilters.e[c2]->VersionName(),filtername)) {
							filter=laidout->importfilters.e[c2];
							break;
						}
					}
					 //if no match, search for first case insensitive match
					if (filter==NULL) {
						for (c2=0; c2<laidout->importfilters.n; c2++) {
							if (!strncasecmp(laidout->importfilters.e[c2]->VersionName(),filtername,strlen(filtername))) {
								filter=laidout->importfilters.e[c2];
								break;
							}
						}
					}
				}
			} else {
				filter=dynamic_cast<ImportFilter *>(parameters->findObject("filter",v));
			}
			if (!filter) throw _("Unknown filter!");

		} else {
			 //find filter by querying the file
			int c;
			FILE *f=fopen(filename,"r");
			if (!f) throw _("Cannot open file!");
			char first100[200];
			c=fread(first100,1,200,f);
			fclose(f);

			const char *filtertype=NULL;
			for (c=0; c<laidout->importfilters.n; c++) {
				filtertype=laidout->importfilters.e[c]->FileType(first100);
				if (filtertype) {
					filter=laidout->importfilters.e[c];
					break;
				}
			}
		}
		if (filter==NULL) throw _("Filter not found!");

		 //must create an ImportConfig that will be passed to filter->In().
		StyleDef *def=filter->GetStyleDef();
		ImportConfig *config=NULL;
		Value *value=NULL;
		char *error=NULL;
		if (def->stylefunc) (def->stylefunc)(context,parameters,&value,&error);
		if (value->type()==VALUE_Object) config=dynamic_cast<ImportConfig*>(((ObjectValue*)value)->object);
		if (config) err=filter->In(filename,config,error_ret);
		if (value) value->dec_count();

	} catch (const char *str) {
		if (error_ret) appendline(*error_ret,str);
		err=1;
//	} catch (char *str) {
//		if (error_ret) appendline(*error_ret,str);
//		delete[] str;
//		err=1;
	}

	if (filename) delete[] filename;
	if (value_ret) *value_ret=NULL;
	return err;

}

//------------------------------- Export --------------------------------
/*! \ingroup api */
StyleDef *makeExportStyleDef()
{
	 //define base
	StyleDef *sd=new StyleDef(NULL,"Export",
			_("Export"),
			_("Export a document or a group with the specified export filter to the specified file or files."),
			Element_Function,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			ExportFunction);


	 //define parameters
	sd->push("filename",
			_("Filename"),
			_("Path of exported file. For multiple files, use \"file##.svg\", for instance."),
			Element_String,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
//	sd->push("target",
//			_("Output target"),
//			_("Whether to try to save to a single file (0), or multiple files (1)."),
//			Element_Int,
//			NULL, //range
//			NULL,  //defvalue
//			0,    //flags
//			NULL);//newfunc
	sd->push("filter",
			_("Filter"),
			_("The filter to export with."),
			Element_Any,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
//	sd->push("doc",
//			_("Document"),
//			_("The document to export, if not exporting a group."),
//			Element_Any,
//			NULL, //range
//			NULL,  //defvalue
//			0,    //flags
//			NULL);//newfunc
//	sd->push("group",
//			_("Group"),
//			_("Group to export, if not exporting a document."),
//			Element_Any,
//			NULL, //range
//			NULL,  //defvalue
//			0,    //flags
//			NULL);//newfunc

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
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=NULL;
		if (error_ret) appendline(*error_ret,_("Easy for you to say!"));
		return 1;
	}

	int err=0;
	ExportFilter *filter=NULL;
	DocumentExportConfig *config=NULL;
	try {
		 //---get file to export
		const char *filename=parameters->findString("filename");


		 //---determine what filter to use, from string, or filter object
		Value *v=parameters->find("filter");
		if (v) {
			if (v->type()==VALUE_String) {
				const char *filtername=dynamic_cast<StringValue*>(v)->str;
				if (filtername) {
					int c2;
					for (c2=0; c2<laidout->exportfilters.n; c2++) {
						if (!strcmp(laidout->exportfilters.e[c2]->VersionName(),filtername)) {
							filter=laidout->exportfilters.e[c2];
							break;
						}
					}
					 //if no match, search for first case insensitive match
					if (filter==NULL) {
						for (c2=0; c2<laidout->exportfilters.n; c2++) {
							if (!strncasecmp(laidout->exportfilters.e[c2]->VersionName(),filtername,strlen(filtername))) {
								filter=laidout->exportfilters.e[c2];
								break;
							}
						}
					}
				}
			} else if (v->type()==VALUE_Object) {
				ObjectValue *vv=dynamic_cast<ObjectValue*>(v);
				if (vv) {
					filter=dynamic_cast<ExportFilter *>(vv->object);
					if (!filter) {
						config=dynamic_cast<DocumentExportConfig*>(vv->object);
						if (config) filter=config->filter;
					}
				}
			}

		} 
		if (v && !filter) throw _("Unknown filter!");

		if (filter==NULL) throw _("Filter not found!");

		 //----doc***
		 //----group***

		 //must create a DocumentExportConfig that will be passed to filter->Out().
		if (!config) {
			throw _("Missing an export config!");
			//******
			//StyleDef *def=filter->GetStyleDef();
			//Value *value=NULL;
			//char *error=NULL;
			//if (def->stylefunc) (def->stylefunc)(context,parameters,&value,&error);
			//if (value->type()==VALUE_Object) config=dynamic_cast<DocumentExportConfig*>(((ObjectValue*)value)->object);
			//if (config) err=filter->Out(filename,config,error_ret);
			//if (value) value->dec_count();
		}

		if (filename) makestr(config->filename,filename);
		if (!config->filename) throw _("Missing filename.");
		if (!config->doc && !config->limbo && context) {
			config->doc=dynamic_cast<Document*>(context->findObject("document"));
			if (config->doc) config->doc->inc_count();
		}

		err=export_document(config,error_ret);

	} catch (const char *str) {
		if (error_ret) appendline(*error_ret,str);
		err=1;
//	} catch (char *str) {
//		if (error_ret) appendline(*error_ret,str);
//		delete[] str;
//		err=1;
	}

	if (value_ret) *value_ret=NULL;
	return err;
}


