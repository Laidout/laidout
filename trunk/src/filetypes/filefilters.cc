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
// Copyright (C) 2007-2009,2012 by Tom Lechner
//

#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include "../language.h"
#include "filefilters.h"
#include "../laidout.h"


#define DBG
#include <iostream>
using namespace std;

using namespace LaxFiles;

//------------------------------------- FileFilter -----------------------------------
/*! \class FileFilter
 * \brief Abstract base class of input and output file filters.
 *
 * These filters act on whole files, not on chunks of them. They are for exporting
 * to an entire postscript file, for instance, or importing an entire svg file to 
 * an object or document context.
 *
 * Descendent classes of this class will have a whattype() of "FileInputFilter" or 
 * "FileOutputFilter". Other distinguishing tags are accessible through the proper
 * filter info functions, not through the whattype() function.
 */


/*! \var Plugin *FileFilter::plugin;
 * \brief Which plugin, if any, the filter came from. NULL if is built in.
 * \todo *** implement plugins!!
 */
/*! \fn ~FileFilter()
 * \brief Empty virtual destructor.
 */
/*! \fn const char *FileFilter::Author()
 * \brief Return who made this filter. For default ones, this is "Laidout".
 */
/*! \fn const char *FileFilter::FilterVersion()
 * \brief Return a string representing the filter version.
 */
/*! \fn const char *FileFilter::Format()
 * \brief Return the general file format the filter deals with.
 *
 * For instance, this would be "Postscript", "Svg", etc.
 */
/*! \fn const char *FileFilter::DefaultExtension()
 * \brief Return default file extension, something like "eps" or "svg".
 */
/*! \fn const char *FileFilter::Version()
 * \brief The version of Format() that the filter knows how to deal with.
 *
 * For PDF, for instance, this might be "1.4" or "1.3".
 * For SVG, this could even be broken down into "1.0", "1.1", or "1.0-inkscape", for instance.
 */
/*! \fn const char *FileFilter::VersionName()
 * \brief A name for the format and version for a screen dialog.
 *
 * This might be "Postscript LL3", or "Svg, version 1.1". In the latter case, the
 * format was "SVG" and the version was "1.1", but the name is something composite, which
 * is open to translations.
 */
/*! \fn const char *FileFilter::FilterClass()
 * \brief What the filter dumps from or to.
 *
 * \todo **** implement me!!!!!
 *
 * These can currently be "document", "object", "image" (a raster image),
 * or resources such as "gradient", "palette", or "net".
 *
 * The "document" type is for importing page based documents, and breaking down the page
 * components into Laidout elements as possible. This might be svg (with or without pageSets),
 * or Scribus documents. 
 *
 * The "image" type is for importing arbitrary files and treating them
 * like bitmaps. For instance, some output filters can directly use EPS files, but EPS files
 * are not innately understood by Laidout, so there is a filter to move and transform them
 * like bitmaps, but the postscript exporter can use the original file. Other output filters
 * would be able only to use a rasterized version. Same thing for PDF pages, or svg (coverted
 * to a png by inkscape, perhaps).
 *
 * "object" is analogous to the "image" type, that is, it
 * produces a block of things based on some arbitrary file, and the onscreen version is
 * represented as a bunch of Laidout elements.
 *
 * LaidoutApp keeps a catalog of filters grouped by FilterClass(), and then by whether
 * they are for input or output.
 */
/*! \fn Laxkit::anXWindow *ConfigDialog()
 * \brief Return a configuration dialog for the filter.
 *
 * Default is to return NULL.
 *
 * \todo *** implement this feature!
 */

FileFilter::FileFilter()
{
	plugin=NULL; 
	flags=0;
}

//------------------------------------- ImportFilter -----------------------------------
/*! \class ImportFilter
 * \brief Abstract base class of file import filters.
 */


/*! \fn const char *ImportFilter::FileType(const char *first100bytes)
 * \brief Return the version of the filter's format that the file seems to be, or NULL if not recognized.
 */
/*! \fn int ImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log)
 * \brief The function that outputs the stuff.
 *
 * If file!=NULL, then output to that single file, and ignore the files in context.
 *
 * context must be a configuration object that the filter understands. For instance, this
 * might be a DocumentExportConfig object, or perhaps a parameter list from the scripter.
 *
 * On complete success, return 0.
 * If there are non-fatal warnings they are appended to log, and -1 is returned.
 * On failure, return 1, and append error messages to log.
 */

//! Return a new StyleDef object with default import filter options.
/*! Subclasses will usually call this, then add any more options to that, and return
 * that in GetStyleDef(). It is up to the subclasses to install the StyleDef in
 * stylemanager if they want to be available for scripting.
 */
StyleDef *ImportFilter::makeStyleDef()
{
	return makeImportConfigDef();
}

//! Create and return a basic ImportConfig definition.
StyleDef *makeImportConfigDef()
{
	StyleDef *sd=new StyleDef(NULL,"Import",
			_("Import"),
			_("A filter that imports a vector file to an existing document or a group."),
			Element_Fields,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			createImportConfig);

	sd->push("file",
			_("File"),
			_("Path to file to import"),
			Element_String,
			NULL, //range
			"1",  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->pushEnum("keepmystery",
			_("Keep mystery data"),
			_("Whether to attempt to preserve things not understood"),
			"sometimes",  //defvalue
			NULL,NULL, //newfunc
			"no", _("No"), _("Ignore all mystery data"),
			"sometimes", _("Sometimes"), _("Convert what is possible, preserve other mystery data"),
			"always", _("Always"), _("Treat everything as mystery data"),
			NULL);
	sd->push("instart",
			_("Start"),
			_("First page to import from a multipage file"),
			Element_Int,
			NULL,
			NULL,
			0,NULL);
	sd->push("inend",
			_("End"),
			_("Last page to import from a multipage file"),
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
			Element_Real,
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
	sd->push("layout",
			_("Layout"),
			_("Type of layout in which to count the spread index. Depends on the imposition."),
			Element_Any,
			NULL,
			NULL,
			0,NULL);
	sd->push("document",
			_("Document"),
			_("Which document to import to, if not importing to a group"),
			Element_Any,
			NULL,
			NULL,
			0,NULL);
	sd->push("group",
			_("Group"),
			_("Group to import to, if not importing to a document"),
			Element_Any,
			NULL,
			NULL,
			0,NULL);

	return sd;
}

//! Return a ValueObject with an ImportConfig.
/*! If one of the parameters is "importconfig", a ValueObject with an ImportConfig,
 * then fill that with the other parameters.
 *
 * This does not throw an error for having an incomplete set of parameters.
 * It just fills what's given.
 */
int createImportConfig(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=NULL;
		log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 1;
	}

	ImportConfig *config=NULL;
	Value *v=NULL;
	if (parameters) v=parameters->find("importconfig");
	if (v) {
		config=dynamic_cast<ImportConfig*>(dynamic_cast<ObjectValue*>(v));
		if (config) config->inc_count();
	}
	if (!config) config=new ImportConfig();

	char error[100];
	int err=0;
	try {
		int i, e;

		 //---file
		const char *str=parameters->findString("file",-1,&e);
		if (e==0) { if (str) makestr(config->filename,str); }
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"file"); throw error; }

		 //---keepmystery
		i=parameters->findInt("keepmystery",-1,&e);
		if (e==0) config->keepmystery=i;
		else if (e==2) { 
			const char *km=parameters->findString("keepmystery",i,&e);
			if (e==2) {
				sprintf(error, _("Invalid format for %s!"),"keepmystery"); 
				throw error; 
			}
			if (!strcmp(km,"no")) config->keepmystery=0;
			else if (!strcmp(km,"sometimes")) config->keepmystery=1;
			else if (!strcmp(km,"always")) config->keepmystery=2;
		}

		 //---instart
		i=parameters->findInt("instart",-1,&e);
		if (e==0) config->instart=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"instart"); throw error; }

		 //---inend
		i=parameters->findInt("inend",-1,&e);
		if (e==0) config->inend=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"inend"); throw error; }

		 //---topage
		i=parameters->findInt("topage",-1,&e);
		if (e==0) config->topage=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"topage"); throw error; }

		 //---dpi
		double d=parameters->findIntOrDouble("dpi",-1,&e);
		if (e==0) config->dpi=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"dpi"); throw error; }

		 //---document
		v=parameters->find("document");
		Document *doc=NULL;
		if (v) {
			if (v->type()==VALUE_String) {
				str=dynamic_cast<StringValue*>(v)->str;
				doc=laidout->findDocument(str);
			} else if (v->type()==VALUE_Object) {
				doc=dynamic_cast<Document*>(dynamic_cast<ObjectValue*>(v)->object);
			} else {
				sprintf(error, _("Invalid format for %s!"),"document");
				throw error;
			}
			if (!doc) throw _("Document not found!");//yes, this is different than simply invalid format

			if (config->doc) config->doc->dec_count();
			doc->inc_count();
			config->doc=doc;
		}

		 //---spread
		i=parameters->findInt("spread",-1,&e);
		if (e==0) config->spread=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"spread"); throw error; }

		 //---layout
		 //***Layout is really an enum kept inside of a document's imposition. difficult
		 //to have a standard function lookup for it, since it depends an another parameter!!
		v=parameters->find("layout");
		if (v) {
			if (v->type()==VALUE_Int) {
				config->layout=dynamic_cast<IntValue*>(v)->i;
			} else if (doc && doc->imposition && v->type()==VALUE_String) {
				str=dynamic_cast<StringValue*>(v)->str;
				int c=0;
				for (c=0; c<doc->imposition->NumLayoutTypes(); c++) {
					if (!strcmp(str,doc->imposition->LayoutName(c))) {
						config->layout=c;
						break;
					}
				}
				if (c==doc->imposition->NumLayoutTypes()) throw _("Invalid layout name!");
			} else { sprintf(error, _("Invalid format for %s!"),"layout"); throw error; }
		}

		 //---group
		Laxkit::RefCounted *r=parameters->findObject("group",-1,&e);
		if (e==0) {
			if (dynamic_cast<Group *>(r)) {
				if (config->toobj) config->toobj->dec_count();
				config->toobj=dynamic_cast<Group *>(r);
				config->toobj->inc_count();
			} else  { sprintf(error, _("Invalid format for %s!"),"group"); throw error; }
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"group"); throw error; }


	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}
	//if (error) delete[] error;

	if (value_ret && err==0) {
		if (config) {
			*value_ret=new ObjectValue(config);
		} else *value_ret=NULL;
	}
	if (config) config->dec_count();

	return err;
}

//------------------------------ ImportConfig ----------------------------
/*! \class ImportConfig
 */
/*! \var char *ImportConfig::filename
 * \brief The file to import.
 */
/*! \var Document *ImportConfig::doc
 * \brief The document to import to, if any.
 *
 * There should be either a document to import to, or an object.
 */
/*! \var Group *ImportConfig::toobj
 * \brief The document to import to, if any.
 *
 * There should be either a document to import to, or an object.
 */
/*! \var int ImportConfig::keepmystery
 * \brief How to deal with data Laidout doesn't understand.
 *
 * 0 is convert nothing and no mystery data is stored. 1 is convert as possible. 2 is do not convert, all
 * objects become mystery data.
 */
/*! \var int ImportConfig::instart
 * \brief The first page of the document to be imported. 0 is the first in the document.
 */
/*! \var int ImportConfig::inend
 * \brief The last page of the document to be imported. -1 mean import until the end.
 */
/*! \var int ImportConfig::topage
 * \brief The first page of the Laidout Document to import onto.
 */
/*! \var int ImportConfig::scaletopage
 * \brief How to scale imported things that don't fit the page.
 *
 * 0 means do no scaling. 1 means always scale so an imported page will fit
 * within the bounds of a document page. If the imported page is smaller,
 * then it is not scaled. 2 is like 1, except that if a page is smaller,
 * it IS scaled to the maximum size that still fits in the document page.
 */

ImportConfig::ImportConfig()
{
	scaletopage=0;
	filename=NULL;
	keepmystery=0;
	instart=inend=-1;
	topage=spread=layout=-1;
	doc=NULL;
	toobj=NULL;
	filter=NULL;
	dpi=300;
}

/*! Increments count of ndoc and nobj if given.
 *
 * If dpi<=0, the use 300. Else use what's given.
 */
ImportConfig::ImportConfig(const char *file, double ndpi, int ins, int ine, int top, int spr, int lay,
				 Document *ndoc, Group *nobj)
{
	filename=newstr(file);
	scaletopage=0;
	keepmystery=0;
	instart=ins;
	inend=ine;
	topage=top;
	spread=spr;
	layout=lay;
	doc=ndoc;
	toobj=nobj;
	filter=NULL;
	if (dpi>0) dpi=ndpi; else dpi=300;

	if (doc) doc->inc_count();
	if (toobj) toobj->inc_count();
}

ImportConfig::~ImportConfig()
{
	if (filename) delete[] filename;
	if (doc) doc->dec_count();
	if (toobj) toobj->dec_count();
	//if (filter) filter->dec_count(); ***filter assumed non-local, always living in laidoutapp?
}

void ImportConfig::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	cout <<"***ImportConfig::dump_out() is incomplete!! Finish implementing!"<<endl;
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sfromfile /file/to/import/from \n",spc);
		fprintf(f,"%sformat  \"SVG 1.0\"    #the format to attempt import from\n",spc);
		return;
	}
	fprintf(f,"%sscaletopage %d\n",spc,scaletopage);//***
	if (keepmystery) fprintf(f,"%skeepmystery\n",spc);
	if (filter) fprintf(f,"%sformat  \"%s\"\n",spc,filter->VersionName());
	//fprintf(f,"%sstart %d\n",spc,start);
	//fprintf(f,"%send   %d\n\n",spc,end);
}

void ImportConfig::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	cout <<"***ImportConfig::dump_in_atts() is incomplete!! Finish implementing!"<<endl;

	char *name,*value;
	int c,c2;
	instart=inend=-1;
	for (c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"keepmystery")) {
			keepmystery=BooleanAttribute(value);
		} else if (!strcmp(name,"scaletopage")) {
			IntAttribute(value,&scaletopage);
		} else if (!strcmp(name,"format")) {
			filter=NULL;
			 //search for exact format match first
			for (c2=0; c2<laidout->importfilters.n; c2++) {
				if (!strcmp(laidout->importfilters.e[c2]->VersionName(),value)) {
					filter=laidout->importfilters.e[c2];
					break;
				}
			}
			 //if no match, search for first case insensitive match
			if (filter==NULL) {
				for (c2=0; c2<laidout->importfilters.n; c2++) {
					if (!strncasecmp(laidout->importfilters.e[c2]->VersionName(),value,strlen(value))) {
						filter=laidout->importfilters.e[c2];
						break;
					}
				}
			}
		}
	}
	if (instart<0) instart=0;
	if (inend<0) inend=1000000000;
}

StyleDef* ImportConfig::makeStyleDef()
{
	return makeImportConfigDef();
}

Style* ImportConfig::duplicate(Style*)
{
	ImportConfig *c=new ImportConfig;
	*c=*this; //warning, shallow copy!

	c->filename=newstr(c->filename);
	if (c->doc) c->doc->inc_count();
	if (c->toobj) c->toobj->inc_count();
	//if (c->filter) c->filter->inc_count();

	return c;
}


//------------------------------- import_document() ----------------------------------
//! Import a vector based file based on config.
/*! Return 0 for success, greater than zero for fatal error, less than zero for success with warnings.
 */
int import_document(ImportConfig *config, ErrorLog &log)
{
	if (!config || !config->filename || !config->filter || !(config->doc || config->toobj)) {
		log.AddMessage(_("Bad import configuration"),ERROR_Fail);
		return 1;
	}
	return config->filter->In(config->filename,config,log);
}


//------------------------------------- ExportFilter -----------------------------------
/*! \class ExportFilter
 * \brief Abstract base class of file export filters.
 */
/*! \fn int ExportFilter::Verify(Laxkit::anObject *context)
 * \brief Preflight checker.
 *
 * This feature is not thought out enough to even have decent documentation. Default just returns 1.
 *
 * \todo Ideally, this function should return some sort of set of objects that cannot be transfered
 *   in the given format, without losing information, maybe with hints for corrections
 */
/*! \fn int ExportFilter::Out(const char *file, Laxkit::anObject *context, ErrorLog &log)
 * \brief The function that outputs the stuff.
 *
 * context must be a configuration object that the filter understands. For instance, this
 * might be a DocumentExportConfig object.
 *
 * On complete success, return 0.
 * If there are non-fatal warnings they are appended to log, and -1 is returned.
 * On failure, return 1, and append error messages to log.
 *
 * You should not call Out directly, unless you are sure to set and perform all the things
 * that export_document() does. Normally, you should use that function instead.
 */
	

//! Return a new StyleDef object with default import filter options.
/*! Subclasses will usually call this, then add any more options to that, and return
 * that in GetStyleDef(). It is up to the subclasses to install the StyleDef in
 * stylemanager if they want to be available for scripting.
 */
StyleDef *ExportFilter::makeStyleDef()
{
	return makeExportConfigDef();
}

StyleDef *makeExportConfigDef()
{
	StyleDef *sd=new StyleDef(NULL,"ExportConfig",
			_("Export Configuration"),
			_("Settings for a filter that exports a document to one or more files of various formats."),
			Element_Fields,
			NULL,NULL,
			NULL,
			0, //new flags
			NULL,
			NULL);

	 //define parameters
	sd->push("filename",
			_("Filename"),
			_("Path of exported file. For multiple files, use \"file##.svg\", for instance."),
			Element_String,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc

	sd->pushEnum("target",
			_("Output target"),
			_("Whether to try to save to a single file (0), or multiple files (1)"),
			"one_file",  //defvalue
			NULL,NULL, //newfunc
			"one_file", _("One file"), _("Export all pages to a single file if possible"),
			"many_files", _("Many files"), _("Export each page to different files"),
			NULL);
	sd->push("document",
			_("Document"),
			_("The document to export, if not exporting a group."),
			Element_Any,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("start",
			_("Start"),
			_("Starting index of a document spread to export"),
			Element_Int,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("end",
			_("End"),
			_("Ending index of a document spread to export"),
			Element_Int,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("layout",
			_("Layout"),
			_("Type of spread layout to export as. Possibilities defined by the imposition."),
			Element_Enum, 
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("group",
			_("Group"),
			_("Group to export, if not exporting a document."),
			Element_Any,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc
	sd->push("rasterize",
			_("Rasterize"),
			_("Whether to rasterize objects that cannot be otherwise dealt with natively in the target format."),
			Element_Boolean,
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
			Element_Any,
			NULL, //range
			NULL,  //defvalue
			0,    //flags
			NULL);//newfunc

	return sd;
}

//! Return a ValueObject with an ExportConfig.
/*! If one of the parameters is ".config", a ValueObject with an ExportConfig,
 * then fill that with the other parameters.
 *
 * This does not throw an error for having an incomplete set of parameters.
 * It just fills what's given.
 *
 * The document is taken from context if it is not a parameter, and there is to
 * "group" parameter.
 */
int createExportConfig(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=NULL;
		log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 1;
	}

	DocumentExportConfig *config=NULL;
	Value *v=NULL;
	if (parameters) v=parameters->find("exportconfig");
	if (v) {
		config=dynamic_cast<DocumentExportConfig*>(dynamic_cast<ObjectValue*>(v)->object);
		if (config) config->inc_count();
	}
	if (!config) config=new DocumentExportConfig();

	char error[100];
	int err=0;
	try {
		int i, e;

		 //---file
		const char *str=parameters->findString("filename",-1,&e);
		if (e==0) { if (str) makestr(config->filename,str); }
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"file"); throw error; }

		 //---rasterize
		i=parameters->findInt("rasterize",-1,&e);
		if (e==0) config->rasterize=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"rasterize"); throw error; }

		 //---collect
		i=parameters->findInt("collect",-1,&e);
		if (e==0) config->collect_for_out=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"collect"); throw error; }

		 //---start
		i=parameters->findInt("start",-1,&e);
		if (e==0) config->start=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"start"); throw error; }

		 //---end
		i=parameters->findInt("end",-1,&e);
		if (e==0) config->end=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"end"); throw error; }

		 //---target
		i=parameters->findInt("target",-1,&e);
		if (e==0) {
			if (i!=0 && i!=1) throw _("Invalid target value!");
			config->target=i;
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"end"); throw error; }

		 //---group
		Laxkit::RefCounted *r=parameters->findObject("group",-1,&e);
		if (e==0) {
			if (dynamic_cast<Group *>(r)) {
				if (config->limbo) config->limbo->dec_count();
				config->limbo=dynamic_cast<Group *>(r);
				config->limbo->inc_count();
			} else  { sprintf(error, _("Invalid format for %s!"),"group"); throw error; }
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"group"); throw error; }

		 //---document
		v=parameters->find("document");
		Document *doc=NULL;
		if (v) {
			if (v->type()==VALUE_String) {
				str=dynamic_cast<StringValue*>(v)->str;
				doc=laidout->findDocument(str);
			} else if (v->type()==VALUE_Object) {
				doc=dynamic_cast<Document*>(dynamic_cast<ObjectValue*>(v)->object);
			} else {
				sprintf(error, _("Invalid format for %s!"),"document");
				throw error;
			}
		}
		if (!doc && !config->limbo && context) {
			v=context->find("document");
			if (v && v->type()==VALUE_Object) {
				doc=dynamic_cast<Document*>(dynamic_cast<ObjectValue*>(v)->object);
			}
		}
		if (doc) {
			if (config->doc) config->doc->dec_count();
			doc->inc_count();
			config->doc=doc;
		}
		if (!doc && !config->limbo) throw _("Need something to export!");


		 //---layout
		 //***Layout is really an enum kept inside of a document's imposition. difficult
		 //to have a standard function lookup for it, since it depends an another parameter!!
		v=parameters->find("layout");
		if (v) {
			if (v->type()==VALUE_Int) {
				config->layout=dynamic_cast<IntValue*>(v)->i;
			} else if (doc && doc->imposition && v->type()==VALUE_String) {
				str=dynamic_cast<StringValue*>(v)->str;
				int c=0;
				for (c=0; c<doc->imposition->NumLayoutTypes(); c++) {
					if (!strcmp(str,doc->imposition->LayoutName(c))) {
						config->layout=c;
						break;
					}
				}
				if (c==doc->imposition->NumLayoutTypes()) throw _("Invalid layout name!");
			} else { sprintf(error, _("Invalid format for %s!"),"layout"); throw error; }
		}

		 //---papergroup
		PaperGroup *pg=dynamic_cast<PaperGroup *>(parameters->findObject("papergroup",-1,&i));
		if (pg) {
			if (config->papergroup) config->papergroup->dec_count();
			config->papergroup=pg;
			pg->inc_count();
		}

	} catch (const char *str) {
		log.AddMessage(str,ERROR_Fail);
		err=1;
	}
	//if (error) delete[] error;

	if (value_ret && err==0) {
		if (config) {
			*value_ret=new ObjectValue(config);
		} else *value_ret=NULL;
	}
	if (config) config->dec_count();

	return err;
}

//---------------------------- DocumentExportConfig ------------------------------
/*! \class DocumentExportConfig
 * \brief Holds basic settings for exporting a document.
 *
 * If filename==NULL and tofiles!=NULL, then write out one spread per file, and tofiles
 * must be a file name template.
 *
 * \todo On exporting, should have option to collect for out to a directory. This includes
 *   possibly rasterizing certain portions of a document, and making references to it in
 *   the exported document. If this happens, also output an output log of warnings/errors
 *   and a list of applied work arounds to the output directory. Finally, collect for out should 
 *   have option to zip up the output directory?
 */
/*! \var int DocumentExportConfig::target
 * 
 * 0 for filename,
 * 1 for tofiles: 1 spread (or paper slice) per file,
 * 2 for command.
 */
/*! \var char DocumentExportConfig::collect_for_out
 * \brief Whether to gather needed extra files to one place.
 *
 * If you export to svg, for instance, there will be references to files whereever they are,
 * plus things that can only be rasterized. This option lets you put copies of all the 
 * extra files, plus new files resulting from rasterizing all in one directory for later use.
 * That directory is just the dirname of filename or tofiles.
 *
 * \todo *** implement me!!
 */

DocumentExportConfig::DocumentExportConfig()
{
	filter=NULL;
	target=0;
	filename=NULL;
	tofiles=NULL;
	start=end=-1;
	layout=0;
	doc=NULL;
	papergroup=NULL;
	limbo=NULL;
	collect_for_out=COLLECT_Dont_Collect;
	rasterize=0;
}

/*! Increments count on ndoc if it exists.
 */
DocumentExportConfig::DocumentExportConfig(Document *ndoc,
										   Group *lmbo,
										   const char *file,
										   const char *to,
										   int l,int s,int e,
										   PaperGroup *group)
{
	target=0;
	filename=newstr(file);
	tofiles=newstr(to);
	start=s;
	end=e;
	layout=l;
	doc=ndoc;
	limbo=lmbo;
	collect_for_out=COLLECT_Dont_Collect;

	filter=NULL;
	if (doc) doc->inc_count();
	if (limbo) limbo->inc_count();
	papergroup=group;
	if (papergroup) papergroup->inc_count();
}

/*! Decrements doc if it exists.
 */
DocumentExportConfig::~DocumentExportConfig()
{
	if (filename) delete[] filename;
	if (tofiles)  delete[] tofiles;
	if (doc) doc->dec_count();
	if (limbo) limbo->dec_count();
	if (papergroup) papergroup->dec_count();
}

void DocumentExportConfig::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%stofile /file/to/export/to \n",spc);
		fprintf(f,"%stofiles  \"/files/like###.this\"  #the # section is replaced with the page index\n",spc);
		fprintf(f,"%s                                #Only one of filename or tofiles should be present\n",spc);
		fprintf(f,"%sformat  \"SVG 1.0\"    #the format to export as\n",spc);
		fprintf(f,"%simposition  Booklet  #the imposition used. This is set automatically when exporting a document\n",spc);
		fprintf(f,"%slayout pages         #this is particular to the imposition used by the document\n",spc);
		fprintf(f,"%sstart 3              #the starting index to export, counting from 0\n",spc);
		fprintf(f,"%send   5              #the ending index to export, counting from 0\n",spc);
		return;
	}
	if (filename) fprintf(f,"%stofile %s\n",spc,filename);
	if (tofiles) fprintf(f,"%stofiles  \"%s\"\n",spc,tofiles);
	if (filter) fprintf(f,"%sformat  \"%s\"\n",spc,filter->VersionName());
	if (doc && doc->imposition) {
		fprintf(f,"%simposition \"%s\"\n",spc,doc->imposition->whattype());
		fprintf(f,"%slayout \"%s\"\n",spc,doc->imposition->LayoutName(layout));
	}
	fprintf(f,"%sstart %d\n",spc,start);
	fprintf(f,"%send   %d\n\n",spc,end);
}

void DocumentExportConfig::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	char *name,*value;
	int c,c2;
	start=end=-1;
	for (c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"tofile")) {
			target=0;
			makestr(filename,value);
		} else if (!strcmp(name,"tofiles")) {
			target=1;
			makestr(tofiles,value);
		} else if (!strcmp(name,"format")) {
			filter=NULL;
			 //search for exact format match first
			for (c2=0; c2<laidout->exportfilters.n; c2++) {
				if (!strcmp(laidout->exportfilters.e[c2]->VersionName(),value)) {
					filter=laidout->exportfilters.e[c2];
					break;
				}
			}
			 //if no match, search for first case insensitive match
			if (filter==NULL) {
				for (c2=0; c2<laidout->exportfilters.n; c2++) {
					if (!strncasecmp(laidout->exportfilters.e[c2]->VersionName(),value,strlen(value))) {
						filter=laidout->exportfilters.e[c2];
						break;
					}
				}
			}
		} else if (!strcmp(name,"imposition")) {
			//***
			cout <<"Need to implement export with alternate imposition.."<<endl;
		} else if (!strcmp(name,"layout")) {
			if (!doc || isblank(value)) { layout=0; continue; }
			for (c2=0; c2<doc->imposition->NumLayoutTypes(); c2++) {
				if (!strcmp(value,doc->imposition->LayoutName(c2))) break;
			}
			if (c2==doc->imposition->NumLayoutTypes()) {
				for (c2=0; c2<doc->imposition->NumLayoutTypes(); c2++) {
					if (!strncasecmp(value,doc->imposition->LayoutName(c2),strlen(value))) break;
				}
			}
			if (c2==doc->imposition->NumLayoutTypes()) c2=0;
			layout=c2;
		} else if (!strcmp(name,"start")) {
			IntAttribute(value,&start);
		} else if (!strcmp(name,"end")) {
			IntAttribute(value,&end);
		}
	}
	if (start<0) start=0;
	if (end<0) end=1000000000;
}

StyleDef *DocumentExportConfig::makeStyleDef()
{
	return makeExportConfigDef();
}

Style* DocumentExportConfig::duplicate(Style*)
{
	DocumentExportConfig *c=new DocumentExportConfig;
	*c=*this; //shallow copy!!

	//if (c->filter) c->filter->inc_count();
	if (c->doc) c->doc->inc_count();
	if (c->limbo) c->limbo->inc_count();
	if (c->papergroup) c->papergroup->inc_count();

	c->filename=newstr(c->filename);
	c->tofiles =newstr(c->tofiles);

	return c;
}


//------------------------------- export_document() ----------------------------------

//! Export a document from a file or a live Document.
/*! Return 0 for export successful. -1 is returned If there are non-fatal errors, in which case
 * the warning messages get appended to log. If there are fatal errors, then log
 * gets appended with a message, and 1 is returned.
 *
 * If the filter cannot support multiple file output, but there are multiple files to be output,
 * then this function will call filter->Out() with the correct data for each file.
 *
 * Also does sanity checking on config->papergoup, config->start, and config->end. Ensures that
 * config->papergroup is never NULL, that start<=end, and that start and end are proper for
 * the requested spreads. If end<0, then make end the last spread.
 *
 * If no doc is specified, then start=end=0 is passed to the filter. Also ensures that at least
 * one of doc and limbo is not NULL before calling the filter.
 *
 * \todo perhaps command facility should be here... currently it sits in ExportDialog.
 */
int export_document(DocumentExportConfig *config, ErrorLog &log)
{
	if (!config || !config->filter) {
		log.AddMessage(_("Missing export filter!"),ERROR_Fail);
		return 1;
	} else if (!(config->doc || config->limbo)) {
		log.AddMessage(_("Missing export source!"),ERROR_Fail);
		return 1;
	}

	DBG cerr << "export_document begin to \""<<config->filter->VersionName()<<"\"......."<<endl;

	 //figure out what paper arrangement to print out on
	PaperGroup *papergroup=config->papergroup;
	if (papergroup && papergroup->papers.n==0) papergroup=NULL;
	if (!papergroup && config->doc) papergroup=config->doc->imposition->papergroup;
	if (papergroup && papergroup->papers.n==0) papergroup=NULL;
	if (!papergroup) {
		int c;
		for (c=0; c<laidout->papersizes.n; c++) {
			if (!strcasecmp(laidout->defaultpaper,laidout->papersizes.e[c]->name)) 
				break;
		}
		PaperStyle *ps;
		if (c==laidout->papersizes.n) c=0;
		ps=(PaperStyle *)laidout->papersizes.e[0]->duplicate();
		papergroup=new PaperGroup(ps);
		ps->dec_count();
	} else papergroup->inc_count();
	if (config->papergroup) config->papergroup->dec_count();
	config->papergroup=papergroup;

	 //establish starting and ending spreads. If no doc, then use only limbo (1 spread)
	if (!config->doc) {
		config->start=config->end=0;
	} else {
		 //clamp start
		if (config->start<0) config->start=0;
		else if (config->start>=config->doc->imposition->NumSpreads(config->layout))
			config->start=config->doc->imposition->NumSpreads(config->layout)-1;
		 //clamp end, but if end<0, make it the maximum instead of 0
		if (config->end<0 || config->end>=config->doc->imposition->NumSpreads(config->layout))
			config->end=config->doc->imposition->NumSpreads(config->layout)-1;
		if (config->end<config->start) config->end=config->start;
	}

	int n=(config->end-config->start+1)*papergroup->papers.n;
	if (n>1 && config->target==0 && !(config->filter->flags&FILTER_MULTIPAGE)) {
		log.AddMessage(_("Filter cannot export more than one page to a single file."),ERROR_Fail);
		return 1;
	}

	int err=0;
	if (n>1 && config->target==1 && !(config->filter->flags&FILTER_MANY_FILES)) {
		 //filter does not support outputting to many files, so loop over each paper

		config->target=0;
		PaperGroup *pg;
		char *filebase=NULL;
		if (config->tofiles) filebase=LaxFiles::make_filename_base(config->tofiles);//###.ext -> %03d.ext
		else filebase=LaxFiles::make_filename_base(config->filename);//###.ext -> %03d.ext
		if (papergroup->papers.n>1) {
			 // basically make base###.ps --> base(spread number)-(paper number).ps
			char *pos=strchr(filebase,'%'); //pos will never be 0
			while (*pos!='d') pos++;
			replace(filebase,"d-%d",pos-filebase,1,NULL);
		}
		char filename[strlen(filebase)+20];

		int start=config->start,
			end=config->end;
		PaperGroup *oldpg=config->papergroup;
		int left=0;
		for (int c=start; c<=end; c++) { //loop over each spread
			for (int p=0; p<papergroup->papers.n; p++) { //loop over each paper in a spread
				config->start=config->end=c;

				if (papergroup->papers.n==1) sprintf(filename,filebase,c);
				else sprintf(filename,filebase,c,p);

				pg=new PaperGroup(papergroup->papers.e[p]);
				if (papergroup->objs.n()) {
					for (int o=0; o<papergroup->objs.n(); o++) pg->objs.push(papergroup->objs.e(o));
				}
				config->papergroup=pg;

				err=config->filter->Out(filename,config,log);
				pg->dec_count();
				if (err>0) { left=papergroup->papers.n-p; break; }
			}
			if (err>0) { left+=(end-c+1)*papergroup->papers.n; break; }
		}
		config->papergroup=oldpg;
		config->start=start;
		config->end=end;
		delete[] filebase;

		if (left) {
			char scratch[strlen(_("Export failed at file %d out of %d"))+20];
			sprintf(scratch,_("Export failed at file %d out of %d"),n-left,n);
			log.AddMessage(scratch,ERROR_Fail);
		}
	} else err=config->filter->Out(NULL,config,log);
	
	DBG cerr << "export_document end."<<endl;

	if (err>0) {
		log.AddMessage(_("Export failed."),ERROR_Fail);
		return 1;
	} 
	return 0;
}


