//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2007-2009,2012 by Tom Lechner
//

#include "filefilters.h"
#include "../core/document.h"
#include "../language.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../dataobjects/bboxvalue.h"

#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <unistd.h>

#define DBG
#include <iostream>
using namespace std;


using namespace Laxkit;


namespace Laidout {

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
 * \brief Which plugin, if any, the filter came from. nullptr if is built in.
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
 * These can currently be:
 *
 * - __"document"__ is for importing page based documents, and breaking down the page
 *   components into Laidout elements as possible. This might be svg (with or without pageSets),
 *   or Scribus documents. 
 *
 * - __"image"__ is for importing arbitrary files and treating them
 *   like bitmaps. For instance, some output filters can directly use EPS files, but EPS files
 *   are not understood as vectors by Laidout, so there is a filter to move and transform them
 *   like bitmaps, but the postscript exporter can use the original file. Other output filters
 *   would be able only to use a rasterized version. Same thing for PDF pages, or svg (coverted
 *   to a png by inkscape, perhaps).
 *
 * - __"object"__ is analogous to the "image" type, that is, it
 *   produces a block of things based on some arbitrary file, and the onscreen version is
 *   represented as a bunch of Laidout elements.
 *
 * TODO: LaidoutApp keeps a catalog of filters grouped by FilterClass(), and then by whether
 * they are for input or output.
 */
/*! \fn Laxkit::anXWindow *ConfigDialog()
 * \brief Return a configuration dialog for the filter.
 *
 * Default is to return nullptr.
 *
 * \todo *** implement this feature!
 */

FileFilter::FileFilter()
{
	plugin = nullptr; 
	flags  = 0;
}

FileFilter::~FileFilter()
{
	DBG cerr << "FileFilter destructor " <<endl;
}

/*! Return whether the filter is based on import or export from whole directories, rather than files.
 * Default is to return false.
 */
bool FileFilter::DirectoryBased()
{
	return false;
}

//------------------------------------- ImportFilter -----------------------------------
/*! \class ImportFilter
 * \brief Abstract base class of file import filters.
 */


/*! \fn const char *ImportFilter::FileType(const char *first100bytes)
 * \brief Return the version of the filter's format that the file seems to be, or nullptr if not recognized.
 */
/*! \fn int ImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log, const char *filecontents, int contentslen)
 * \brief The function that outputs the stuff.
 *
 * If file!=nullptr, then input from that single file, and ignore the files in context.
 * If file==nullptr and filecontents!=nullptr, then assume filecontents is a string containing file data of contentslen bytes.
 *
 * context must be a configuration object that the filter understands. For instance, this
 * might be a DocumentExportConfig object, or perhaps a parameter list from the scripter.
 *
 * On complete success, return 0.
 * If there are non-fatal warnings they are appended to log, and -1 is returned.
 * On failure, return 1, and append error messages to log.
 */

//! Return a new ObjectDef object with default import filter options.
/*! Subclasses will usually call this, then add any more options to that, and return
 * that in GetObjectDef(). It is up to the subclasses to install the ObjectDef in
 * stylemanager if they want to be available for scripting.
 */
ObjectDef *ImportFilter::makeObjectDef()
{
	return makeImportConfigDef();
}

//! Create and return a basic ImportConfig definition.
ObjectDef *makeImportConfigDef()
{
	ObjectDef *sd=new ObjectDef(nullptr,"Import",
			_("Import"),
			_("A filter that imports a vector file to an existing document or a group."),
			"class",
			nullptr,nullptr,
			nullptr,
			0, //new flags
			nullptr,
			createImportConfig);

	sd->push("file",
			_("File"),
			_("Path to file to import"),
			"string",
			nullptr, //range
			"1",  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->pushEnum("keepmystery",
			_("Keep mystery data"),
			_("Whether to attempt to preserve things not understood"),
			false, //whether is enumclass or enum instance
			"sometimes",  //defvalue
			nullptr,nullptr, //newfunc
			"no", _("No"), _("Ignore all mystery data"),
			"sometimes", _("Sometimes"), _("Convert what is possible, preserve other mystery data"),
			"always", _("Always"), _("Treat everything as mystery data"),
			nullptr);
	sd->push("range",
			_("Range"),
			_("Range of pages to import from a multipage file"),
			"int",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("topage",
			_("To page"),
			_("The page in the existing document to begin importing to"),
			"int",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("dpi",
			_("Default dpi"),
			_("Default dpi to use while importing if necessary"),
			"real",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("spread",
			_("Spread"),
			_("Index of the spread to import to"),
			"int",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("layout",
			_("Layout"),
			_("Type of layout in which to count the spread index. Depends on the imposition."),
			"any",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("document",
			_("Document"),
			_("Which document to import to, if not importing to a group"),
			"any",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("group",
			_("Group"),
			_("Group to import to, if not importing to a document"),
			"any",
			nullptr,
			nullptr,
			0,nullptr);

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
					   Value **value_ret, Laxkit::ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=nullptr;
		log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 1;
	}

	ImportConfig *config=nullptr;
	Value *v=nullptr;
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
		int instart = -1, inend = -1;

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

		 //---instart, DEPRECATED! here for backwards compat
		i=parameters->findInt("instart",-1,&e);
		if (e==0) instart=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"instart"); throw error; }

		 //---inend, DEPRECATED! here for backwards compat
		i=parameters->findInt("inend",-1,&e);
		if (e==0) inend=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"inend"); throw error; }

		const char *sv = parameters->findString("range",-1,&e);
		if (e == 0) {
			config->range.Clear();
			config->range.Parse(sv, nullptr, false);
		} else if (e == 2) { sprintf(error, _("Invalid format for %s!"),"range"); throw error; }
		
		if (instart >= 0 && inend < 0) config->range.AddRange(instart, instart);
		else if (instart >=0 && inend >= 0) config->range.AddRange(instart, inend);

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
		Document *doc=nullptr;
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
		Laxkit::anObject *r=parameters->findObject("group",-1,&e);
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
		} else *value_ret=nullptr;
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
	scaletopage = 2;
	filename    = nullptr;
	keepmystery = 0;
	doc         = nullptr;
	toobj       = nullptr;
	filter      = nullptr;
	dpi         = 300;
	topage = spread = layout = -1;

	range.parse_from_one = true;
}

/*! Increments count of ndoc and nobj if given.
 *
 * If dpi<=0, the use 300. Else use what's given.
 */
ImportConfig::ImportConfig(const char *file, double ndpi, int ins, int ine, int top, int spr, int lay,
				 Document *ndoc, Group *nobj)
{
	filename    = newstr(file);
	scaletopage = 2;
	keepmystery = 0;
	topage      = top;
	spread      = spr;
	layout      = lay;
	doc         = ndoc;
	toobj       = nobj;
	filter      = nullptr;
	if (dpi > 0) dpi = ndpi; else dpi = 300;

	range.parse_from_one = true;
	if (ins >= 0 && ine < 0) range.AddRange(ins,ins);
	else if (ins >= 0 && ine < 0) range.AddRange(ins,ine);


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

void ImportConfig::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

Laxkit::Attribute *ImportConfig::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Attribute;
	if (what==-1) {
		att->push("fromfile", "/file/to/import/from");
		att->push("format", "\"SVG 1.0\"", "the format to attempt import from");
		att->push("scaletopage", "yes",    "yes, no, or down. down does not scale up if smaller than page");
		att->push("range", "1-5", "Range of pages to import");
		att->push("topage", "0", "Document page to import to");
		att->push("spread", "0", "Spread number to import to");
		att->push("layout", "0", "Layout number to get spread from");
		att->push("dpi", "300", "Default dpi to import at");
		return att;
	}

	if (scaletopage==0) att->push("scaletopage", "no");
	else if (scaletopage==1) att->push("scaletopage", "down");
	else att->push("scaletopage", "yes");

	if (keepmystery) att->push("keepmystery");
	if (filter) att->push("format", filter->VersionName());
	//fprintf(f,"%sstart %d\n",spc,start);
	//fprintf(f,"%send   %d\n\n",spc,end);

	att->push("range", range.ToString(false, false, false));
	att->push("topage", topage);
	att->push("spread", spread);
	att->push("layout", layout);
	att->push("dpi",    dpi);

	return att;
}
	

void ImportConfig::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	char *name,*value;
	int c,c2;
	int start = -1, end = -1;
	for (c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"keepmystery")) {
			keepmystery=BooleanAttribute(value);

		} else if (!strcmp(name,"scaletopage")) {
			if (!strcasecmp(value,"no") || !strcmp(value,"0")) scaletopage=0;
			else if (!strcasecmp(value,"down")) scaletopage=1;
			else scaletopage=2;

		} else if (!strcmp(name,"format")) {
			filter=nullptr;
			 //search for exact format match first
			for (c2=0; c2<laidout->importfilters.n; c2++) {
				if (!strcmp(laidout->importfilters.e[c2]->VersionName(),value)) {
					filter=laidout->importfilters.e[c2];
					break;
				}
			}
			 //if no match, search for first case insensitive match
			if (filter==nullptr) {
				for (c2=0; c2<laidout->importfilters.n; c2++) {
					if (!strncasecmp(laidout->importfilters.e[c2]->VersionName(),value,strlen(value))) {
						filter=laidout->importfilters.e[c2];
						break;
					}
				}
			}

		} else if (!strcmp(name,"start")) { //for backwards compat
			IntAttribute(value,&start);

		} else if (!strcmp(name,"end")) { //for backwards compat
			IntAttribute(value,&end);

		} else if (!strcmp(name,"range")) {
			if (!isblank(value)) {
				range.Clear();
				if (!strcasecmp(value, "all")) range.AddRange(0,-1);
				else range.Parse(value, nullptr, false);
			}

		} else if (!strcmp(name,"topage")) { //for backwards compat
			IntAttribute(value,&topage);

		} else if (!strcmp(name,"spread")) { //for backwards compat
			IntAttribute(value,&spread);

		} else if (!strcmp(name,"layout")) { //for backwards compat
			IntAttribute(value,&layout);

		} else if (!strcmp(name,"dpi")) { //for backwards compat
			DoubleAttribute(value,&dpi);
		}
	}

	if (start >= 0 && end < 0) { range.Clear(); range.AddRange(start, start); }
	else if (start >=0 && end >= 0) { range.Clear(); range.AddRange(start, end); }
}

ObjectDef* ImportConfig::makeObjectDef()
{
	return makeImportConfigDef();
}

Value* ImportConfig::duplicate()
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
int import_document(ImportConfig *config, Laxkit::ErrorLog &log, const char *filecontents,int contentslen)
{
	if (!config || !config->filename || !config->filter || !(config->doc || config->toobj)) {
		log.AddMessage(_("Bad import configuration"),ERROR_Fail);
		return 1;
	}
	return config->filter->In(config->filename,config,log, filecontents,contentslen);
}


//------------------------------------- ProjectExportSettings -----------------------------------

/*! \class ProjectExportSettings
 * 
 * TODO!! 
 * Settings to copy extra files and process basic templates in addition to
 * the files exported normally by the filter.
 */
class ProjectExportSettings : public Value
{
  public:
  	// Laxkit::Utf8String label;
  	ExportFilter *filter;
  	DocumentExportConfig *config;

  	class CopyFiles
  	{
  	  public:
  	  	enum CopyType { Copy, Template, TemplaceEachSpread };
  	  	CopyType type;
  	  	Laxkit::Utf8String pass_id;
  	    Laxkit::Utf8String from_pattern;
  	    Laxkit::Utf8String to;
  	    ValueHash vars;
  	};

  	ProjectExportSettings();
  	virtual ~ProjectExportSettings();

  	//from value:
  	virtual int type();
	virtual Value *duplicate();
  	virtual ObjectDef *makeObjectDef(); //calling code responsible for ref

  	//from DumpUtility:
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};

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
/*! \fn int ExportFilter::Out(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log)
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
	


/*! Create a config object based on fromconfig.
 */
DocumentExportConfig *ExportFilter::CreateConfig(DocumentExportConfig *fromconfig)
{
	DocumentExportConfig* conf = new DocumentExportConfig(fromconfig);
	conf->filter = this;
	return conf;
}


//! Return a new ObjectDef object with default import filter options.
/*! Subclasses will usually call this, then add any more options to that, and return
 * that in GetObjectDef(). It is up to the subclasses to install the ObjectDef in
 * stylemanager if they want to be available for scripting.
 */
ObjectDef *ExportFilter::makeObjectDef()
{
    ObjectDef *sd=stylemanager.FindDef("ExportConfig");
    if (!sd) {
        sd=makeExportConfigDef();
		stylemanager.AddObjectDef(sd,0);
    }

	return sd;
}

Value *NewExportConfig() { return new DocumentExportConfig(); }

ObjectDef *makeExportConfigDef()
{
	ObjectDef *sd=new ObjectDef(nullptr,"ExportConfig",
			_("Export Configuration"),
			_("Settings for a filter that exports a document to one or more files of various formats."),
			"class",
			nullptr,nullptr,
			nullptr,
			0, //new flags
			NewExportConfig, //newfunc
			nullptr); //stylefunc

	 //define parameters
	sd->push("filename",
			_("Filename"),
			_("Path of exported file. For multiple files, use \"file##.svg\", for instance."),
			"string",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc

	sd->pushEnum("target",
			_("Output target"),
			_("Whether to try to save to a single file (0), or multiple files (1)"),
			false, //whether is enumclass or enum instance
			"one_file",  //defvalue
			nullptr,nullptr, //newfunc, objectfunc
			"one_file", _("One file"), _("Export all pages to a single file if possible"),
			"many_files", _("Many files"), _("Export each page to different files"),
			nullptr);
	sd->push("document",
			_("Document"),
			_("The document to export, if not exporting a group."),
			"any",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("range",
			_("Range"),
			_("Range of pages to export."),
			"int",
			nullptr,
			nullptr,
			0,nullptr);
	sd->push("batches",
			_("Batches"),
			_("For multi-page capable targets, how many spreads to include in a single file. Repeat to cover whole range."),
			"int",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->pushEnum("evenodd",
			_("Even or odd"),
			_("Whether to export even, odd, or all spread indices in range"),
			false, //whether is enumclass or enum instance
			"all",  //defvalue
			nullptr,nullptr, //newfunc
			"all", _("All"), _("Export all spreads"),
			"even", _("Even"), _("Export only even spreads"),
			"odd", _("Odd"), _("Export only odd spreads"),
			nullptr);
	sd->pushEnum("paperrotation",
			_("Rotate paper"),
			_("Whether to rotate the final exported paper on export"),
			false, //whether is enumclass or enum instance
			"0",  //defvalue
			nullptr,nullptr, //newfunc
			"0", _("0"), _("No rotation"),
			"90", _("90"), _("90 degree rotation"),
			"180", _("180"), _("180 degree rotation"),
			"270", _("270"), _("270 degree rotation"),
			nullptr);
	sd->push("rotate180",
			_("Alternate 180 degrees"),
			_("Whether to rotate every other paper by 180 degrees."),
			"boolean",
			nullptr, //range
			"false",  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("reverse",
			_("Reverse order"),
			_("Whether to export in reverse order or not."),
			"boolean",
			nullptr, //range
			"false",  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("layout",
			_("Layout"),
			_("Type of spread layout to export as. Possibilities defined by the imposition."),
			"enum", 
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("group",
			_("Group"),
			_("Group to export, if not exporting a document."),
			"any",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("textaspaths",
			_("Text as paths"),
			_("Whether to export any text based objects as path objects, instead of text."),
			"boolean",
			nullptr, //range
			"false",  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("rasterize",
			_("Rasterize"),
			_("Whether to rasterize objects that cannot be otherwise dealt with natively in the target format."),
			"boolean",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("collect",
			_("Collect for out"),
			_("Whether to copy all the accessed resources to the same directory as the exported file."),
			"boolean",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("papergroup",
			_("Paper group"),
			_("The paper group to export onto. Do not include if you want to use the default paper group."),
			"any",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc
	sd->push("crop",
			_("Crop"),
			_("Cropping boundary, applied per spread."),
			"BBox",
			nullptr, //range
			nullptr,  //defvalue
			0,    //flags
			nullptr);//newfunc

	return sd;
}

//! Return a ValueObject with a DocumentExportConfig.
/*! If one of the parameters is ".config", a ValueObject with an ExportConfig,
 * then fill that with the other parameters.
 *
 * This does not throw an error for having an incomplete set of parameters.
 * It just fills what's given.
 *
 * The document is taken from context if it is not a parameter, and there is no
 * "group" parameter.
 */
int createExportConfig(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, Laxkit::ErrorLog &log)
{
	if (!parameters || !parameters->n()) {
		if (value_ret) *value_ret=nullptr;
		log.AddMessage(_("Missing parameters!"),ERROR_Fail);
		return 1;
	}

	DocumentExportConfig *config = nullptr;
	Value * v = nullptr;
	if (parameters) v = parameters->find("exportconfig");
	if (v) {
		if (dynamic_cast<ObjectValue*>(v))
			config = dynamic_cast<DocumentExportConfig *>(dynamic_cast<ObjectValue *>(v)->object);
		else config = dynamic_cast<DocumentExportConfig *>(v);
		if (config) config->inc_count();
	}
	if (!config) config = new DocumentExportConfig();

	char error[100];
	int err=0;
	try {
		int i, e;
		int start = -1, end = -1;

		 //---file
		const char *str=parameters->findString("filename",-1,&e);
		if (e==0) { if (str) makestr(config->filename,str); }
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"file"); throw error; }

		 //---textaspaths
		i=parameters->findInt("textaspaths",-1,&e);
		if (e==0) config->textaspaths=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"textaspaths"); throw error; }

		 //---rasterize
		i=parameters->findInt("rasterize",-1,&e);
		if (e==0) config->rasterize=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"rasterize"); throw error; }

		 //---collect
		i=parameters->findInt("collect",-1,&e);
		if (e==0) config->collect_for_out=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"collect"); throw error; }

		 //---start
		i=parameters->findInt("start",-1,&e); //for backwards compatibility
		if (e==0) start=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"start"); throw error; }

		 //---end
		i=parameters->findInt("end",-1,&e); //for backwards compatibility
		if (e==0) end=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"end"); throw error; }

		if (start >= 0 && end < 0) config->range.AddRange(start, start);
		else if (start >=0 && end >= 0) config->range.AddRange(start, end);

		const char *sv = parameters->findString("range",-1,&e);
		if (e == 0) {
			config->range.Clear();
			config->range.Parse(sv, nullptr, false);
		} else if (e == 2) { sprintf(error, _("Invalid format for %s!"),"range"); throw error; }
		
		 //---batches
		i=parameters->findInt("batches",-1,&e);
		if (e==0) config->batches=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"batches"); throw error; }

		 //---evenodd
		i=parameters->findInt("evenodd",-1,&e);
		if (e==0) {
			if (i==0) config->evenodd=DocumentExportConfig::All;
			else if (i==1) config->evenodd=DocumentExportConfig::Even;
			else if (i==2) config->evenodd=DocumentExportConfig::Odd;
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"evenodd"); throw error; }

		 //---rotate180
		i = parameters->findInt("rotate180",-1,&e);
		if (e==0) {
			if (i==0) config->rotate180=0;
			else if (i==1) config->rotate180=1;
			else if (i==2) config->rotate180=2;
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"rotate180"); throw error; }

		 //---paperrotation
		i=parameters->findInt("paperrotation",-1,&e);
		if (e==0) config->paperrotation=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"paperrotation"); throw error; }

		 //---reverse
		i=parameters->findInt("reverse",-1,&e);
		if (e==0) config->reverse_order=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"reverse"); throw error; }

		 //---target
		i=parameters->findInt("target",-1,&e);
		if (e==0) {
			if (i!=0 && i!=1) throw _("Invalid target value!");
			config->target=i;
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"target"); throw error; }

		 //---crop
		BBoxValue *crop=dynamic_cast<BBoxValue*>(parameters->findObject("crop",-1,&e));
		if (e==0) {
			if (i!=0 && i!=1) throw _("Invalid crop value!");
			config->crop=*crop;
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"crop"); throw error; }

		 //---group
		Laxkit::anObject *r=parameters->findObject("group",-1,&e);
		if (e==0) {
			if (dynamic_cast<Group *>(r)) {
				if (config->limbo) config->limbo->dec_count();
				config->limbo=dynamic_cast<Group *>(r);
				config->limbo->inc_count();
			} else  { sprintf(error, _("Invalid format for %s!"),"group"); throw error; }
		} else if (e==2) { sprintf(error, _("Invalid format for %s!"),"group"); throw error; }

		 //---document
		v=parameters->find("document");
		Document *doc=nullptr;
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
		} else *value_ret=nullptr;
	}
	if (config) config->dec_count();

	return err;
}


//---------------------------- DocumentExportConfig ------------------------------
/*! \class DocumentExportConfig
 * \brief Holds basic settings for exporting a document.
 *
 * If filename==nullptr and tofiles!=nullptr, then write out one spread per file, and tofiles
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
 */
/*! \var int DocumentExportConfig::collect_for_out
 * \brief Whether to gather needed extra files to one place.
 *
 * If you export to svg, for instance, there will be references to files whereever they are,
 * plus things that can only be rasterized. This option lets you put copies of all the 
 * extra files, plus new files resulting from rasterizing all in one directory for later use.
 * That directory is just the dirname of filename or tofiles.
 *
 * \todo *** implement me!!
 */
/*! \var int DocumentExportConfig::batches
 * For multi-page capable targets, whether to create new files for several spreads in one. Single file targets ignore batches.
 * For instance, say batches==4, then
 * for every 4 spreads, create one file (or export). Continue for the whole range of papers.
 * If batches<=0 then do not export in batches.
 */
/*! \var EvenOdd DocumentExportConfig::evenodd
 * Whether to export all, only even indices, or only odd indices of spreads.
 */
/*! \var int DocumentExportConfig::paperrotation
 * Whether to rotate each paper on export. Must be 0, 90, 180, or 270.
 */

DocumentExportConfig::DocumentExportConfig()
{
	curpaperrotation= 0;
	paperrotation   = 0;
	rotate180       = 0;
	reverse_order   = 0;
	evenodd         = All;
	batches         = 0;
	filter          = nullptr;
	target          = TARGET_Single;
	send_to_command = false;
	del_after_command = false;
	command         = nullptr;
	custom_command  = nullptr;
	filename        = nullptr;
	tofiles         = nullptr;
	layout          = 0;
	doc             = nullptr;
	papergroup      = nullptr;
	limbo           = nullptr;
	collect_for_out = COLLECT_Dont_Collect;
	rasterize       = 0;
	textaspaths     = true; // *** change to false when text is better implemented!!
	range.parse_from_one = true;
}

/*! Increments count on ndoc if it exists.
 */
DocumentExportConfig::DocumentExportConfig(Document *ndoc,
										   Group *lmbo,
										   const char *file,
										   const char *to,
										   int l,int s,int e,
										   PaperGroup *group)
  : DocumentExportConfig()
{
	filename   = newstr(file);
	tofiles    = newstr(to);
	layout     = l;
	doc        = ndoc;   if (doc) doc->inc_count();
	limbo      = lmbo;   if (limbo) limbo->inc_count();
	papergroup = group;  if (papergroup) papergroup->inc_count();

	if (s >= 0 && e < 0) range.AddRange(s,s);
	else if (s >= 0 && e >= 0) range.AddRange(s,e);
}

DocumentExportConfig::DocumentExportConfig(DocumentExportConfig *config) 
  : DocumentExportConfig()
{
	if (config == nullptr) {
		return;
	}

	paperrotation  = config->paperrotation;
	rotate180      = config->rotate180;
	reverse_order  = config->reverse_order;
	evenodd        = config->evenodd;
	batches        = config->batches;
	target         = config->target;
	range          = config->range;
	layout         = config->layout;
	collect_for_out= config->collect_for_out;
	rasterize      = config->rasterize;
	textaspaths    = config->textaspaths;

	filename       = newstr(config->filename);
	tofiles        = newstr(config->tofiles);
	custom_command = newstr(config->custom_command);
	command        = config->command;
	if (command) command->inc_count();

	send_to_command   = config->send_to_command;
	del_after_command = config->del_after_command;

	filter         = config->filter; //object, but does not get inc_counted

	doc            = config->doc;
	papergroup     = config->papergroup;
	limbo          = config->limbo;

	if (doc)        doc->inc_count();
	if (limbo)      limbo->inc_count();
	if (papergroup) papergroup->inc_count();
}

/*! Decrements doc if it exists.
 */
DocumentExportConfig::~DocumentExportConfig()
{
	delete[] custom_command;
	delete[] filename;
	delete[] tofiles;
	if (command) command->dec_count();
	if (doc) doc->dec_count();
	if (limbo) limbo->dec_count();
	if (papergroup) papergroup->dec_count();
}

Value* DocumentExportConfig::duplicate()
{
	DocumentExportConfig *c = new DocumentExportConfig(this);
	return c;
}

/*! Copy references to doc, limbo, and papergroup.
 * Nulls are copied also.
 */
void DocumentExportConfig::CopySource(DocumentExportConfig *config)
{
	if (!config) return;

	if (doc != config->doc) {
		if (doc) doc->dec_count();
		doc = config->doc;
		if (doc) doc->inc_count();
	}

	if (limbo != config->limbo) {
		if (limbo) limbo->dec_count();
		limbo = config->limbo;
		if (limbo) limbo->inc_count();
	}

	if (papergroup != config->papergroup) {
		if (papergroup) papergroup->dec_count();
		papergroup = config->papergroup;
		if (papergroup) papergroup->inc_count();
	}
}

Value *DocumentExportConfig::dereference(const char *extstring, int len)
{
	if (IsName("filename", extstring, len)) return new StringValue(filename);
	if (IsName("tofiles", extstring, len)) return new StringValue(tofiles);
	if (IsName("custom_command", extstring, len)) return new StringValue(custom_command);
	if (IsName("textaspaths", extstring, len)) return new BooleanValue(textaspaths);
	if (IsName("rasterize", extstring, len)) return new BooleanValue(rasterize);
	if (IsName("collect", extstring, len)) return new BooleanValue(collect_for_out);
	if (IsName("range", extstring, len)) return new StringValue(range.ToString(true, false, false));
	if (IsName("batches", extstring, len)) return new IntValue(batches);
	if (IsName("evenodd", extstring, len)) return new StringValue(evenodd==Odd ? "odd" : (evenodd == Even ? "even" : "all"));
	if (IsName("rotate180", extstring, len)) return new BooleanValue(rotate180);
	if (IsName("paperrotation", extstring, len)) return new DoubleValue(paperrotation);
	if (IsName("reverse", extstring, len)) return new BooleanValue(reverse_order);
	if (IsName("target", extstring, len)) return new StringValue(target == TARGET_Single ? "single" : (target == TARGET_Multi ? "multi" : "command"));
	if (IsName("crop", extstring, len)) return new BBoxValue(crop);
	if (IsName("document", extstring, len))  { if (doc) doc->inc_count(); return doc; }
	if (IsName("layout", extstring, len)) return new IntValue(layout);
	if (IsName("papergroup", extstring, len)) return new ObjectValue(papergroup); //{ if (papergroup) papergroup->inc_count(); return papergroup; }
	// if (IsName("command")) return ???? ;

	// if (log) log->AddError(_("Could not dereference!"));
	return nullptr;
}

/*! Return 1 for success, 2 for success, but other contents changed too, -1 for unknown extension.
 * 0 for total fail, as when v is wrong type.
 */
int DocumentExportConfig::assign(FieldExtPlace *ext,Value *v)
{
	DBG cerr <<" *** Need to implement DocumentExportConfig::assign()!!"<<endl;

	if (ext->n()!=1) return -1;
	const char *str=ext->e(0);


	if (!strcmp("filename", str)) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv) return 0;
		makestr(filename, sv->str);
		return 1;
	}
	if (!strcmp("tofiles", str)) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv) return 0;
		makestr(tofiles, sv->str);
		return 1;
	}
	if (!strcmp("custom_command", str)) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv) return 0;
		makestr(custom_command, sv->str);
		return 1;
	}
	if (!strcmp("textaspaths", str)) {
		BooleanValue *vv = dynamic_cast<BooleanValue*>(v);
		if (!vv) return 0;
		textaspaths = vv->i;
		return 1;
	}
	if (!strcmp("rasterize", str)) {
		BooleanValue *vv = dynamic_cast<BooleanValue*>(v);
		if (!vv) return 0;
		rasterize = vv->i;
		return 1;
	}
	if (!strcmp("collect", str)) {
		BooleanValue *vv = dynamic_cast<BooleanValue*>(v);
		if (!vv) return 0;
		collect_for_out = vv->i;
		return 1;
	}
	if (!strcmp("range", str)) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv || !sv->str) return 0;
		range.Parse(sv->str, nullptr, false);
		return 1;
	}
	if (!strcmp("batches", str)) {
		IntValue *vv = dynamic_cast<IntValue*>(v);
		if (!vv) return 0;
		batches = vv->i;
		return 1;
	}
	if (!strcmp("evenodd", str)) {
		StringValue *sv = dynamic_cast<StringValue*>(v);
		if (!sv || !sv->str) return 0;
		if (!strcasecmp(sv->str, "odd")) evenodd = Odd;
		else if (!strcasecmp(sv->str, "even")) evenodd = Even;
		else evenodd = All;
		return 1;
	}
	if (!strcmp("rotate180", str)) {
		BooleanValue *vv = dynamic_cast<BooleanValue*>(v);
		if (!vv) return 0;
		rotate180 = vv->i;
		return 1;
	}
	if (!strcmp("paperrotation", str)) {
		double vv;
		if (!isNumberType(v, &vv)) return 0;
		int pr = vv + .5;
		if (pr != 0 && pr != 90 && pr != 180 && pr != 270 && pr != 360) return 0;
		paperrotation = pr;
		return 0;
	}
	if (!strcmp("reverse", str)) {
		BooleanValue *vv = dynamic_cast<BooleanValue*>(v);
		if (!vv) return 0;
		reverse_order = vv->i;
		return 1;
	}
	if (!strcmp("target", str)) {
		StringValue *vv = dynamic_cast<StringValue*>(v);
		if (!vv || !vv->str) return 0;
		if      (!strcasecmp(vv->str, "single"))  target = TARGET_Single;
		else if (!strcasecmp(vv->str, "multi"))   target = TARGET_Multi;
		else if (!strcasecmp(vv->str, "command")) {
			target = TARGET_Single;
			send_to_command = true;
		}
		else return 0;
		return 1;
	}
	if (!strcmp("crop", str)) {
		BBoxValue *vv = dynamic_cast<BBoxValue*>(v);
		if (!vv) return 0;
		crop.setbounds(vv);
		return 1;
	}
	if (!strcmp("document", str)) {
		Document *d = dynamic_cast<Document*>(v);
		if (!d) return 0;
		if (doc != d) {
			if (doc) doc->dec_count();
			doc = d;
		}
		doc->inc_count();
		return 1;
	}
	if (!strcmp("layout", str)) {
		IntValue *vv = dynamic_cast<IntValue*>(v);
		if (!vv) return 0;
		layout = vv->i;
		return 1;
	}

	if (!strcmp("papergroup", str)) {
		cerr << "Aaa!!! Need to implement PaperGroup as a Value!!"<<endl;
		return 0;
	}

	return -1;
}

Laxkit::Attribute *DocumentExportConfig::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what == -1) {
		att->push("tofile","/file/to/export/to");
		att->push("tofiles","\"/files/like###.this\"","the # section is replaced with the page index.");
		att->push("format","\"SVG 1.0\""   ,"the format to export as");
		att->push("imposition","SignatureImposition","the imposition used. This is set automatically when exporting a document");
		att->push("layout","papers"        ,"this is particular to the imposition used by the document");
		att->push("range","3-5,10-15"      ,"Spread index range(s) to export, counting from 0");
		att->push("batches","4"            ,"for multi-page capable targets, the number of spreads to put in a single file, repeat for whole range");
		att->push("evenodd","odd"          ,"all|even|odd. Based on spread index, maybe export only even or odd spreads.");
		att->push("paperrotation","0"      ,"0|90|180|270. Whether to rotate each exported (final) paper by that number of degrees");
		att->push("rotate180","yes"        ,"or no. Whether to rotate every other paper by 180 degrees, in addition to paperrotation");
		att->push("target","single"        ,"or multi. Whether to try to output to a single file or many");

		return att;
	}

	if (filename) att->push("tofile", filename);
	if (tofiles) att->push("tofiles", tofiles);
	att->push("target", target == TARGET_Single ? "single" : (target == TARGET_Multi ? "multi" : "?"));

	att->push("send_to_command", send_to_command ? "yes" : "no");
	att->push("del_after_command", del_after_command ? "yes" : "no");
	if (custom_command) att->push("custom_command", custom_command);

	if (command) {
		const char *toolname = nullptr;
		laidout->prefs.GetToolCategoryName(command->category, &toolname);
		char *str = newstr(toolname);
		appendstr(str, ": ");
		appendstr(str, command->command_name);
		att->push("command", str);
		delete[] str;
	}

	if (filter) att->push("format", filter->VersionName());
	if (doc && doc->imposition) {
		att->push("imposition", doc->imposition->whattype());
		att->push("layout", doc->imposition->LayoutName(layout));
	}

	att->push("range",range.ToString(false, false, false));

	att->push("paperrotation", paperrotation);
	att->push("rotate180", rotate180 == 0 ? "no" : "yes"); 
	att->push("reverse", reverse_order ? "yes" : "no");
	att->push("batches", batches);
	if (evenodd==Odd) att->push("evenodd","odd");
	else if (evenodd==Even) att->push("evenodd","even");
	else att->push("evenodd","all");

	att->push("textaspaths", textaspaths ? "yes" : "no");

	return att;
}

void DocumentExportConfig::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

void DocumentExportConfig::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	char *name,*value;
	int c,c2;
	int start = -1, end = -1;

	for (c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"tofile")) {
			makestr(filename,value);

		} else if (!strcmp(name,"tofiles")) {
			makestr(tofiles,value);

		} else if (!strcmp(name,"custom_command")) {
			makestr(custom_command,value);

		} else if (!strcmp(name,"send_to_command")) {
			send_to_command = BooleanAttribute(value);

		} else if (!strcmp(name,"del_after_command")) {
			del_after_command = BooleanAttribute(value);

		} else if (!strcmp(name,"command")) {
			//something like "Print: lp"
			ExternalTool *tool = laidout->prefs.FindExternalTool(value);
			if (tool) {
				if (command) command->dec_count();
				command = tool;
				tool->inc_count();
			}

		} else if (!strcmp(name,"target")) {
			if (isblank(value)) target = TARGET_Single;
			else if (!strcmp(value, "single"))  target = TARGET_Single;
			else if (!strcmp(value, "multi"))   target = TARGET_Multi;
			else if (!strcmp(value, "command")) {
				//DEPRECATED!!
				target = TARGET_Single;
				send_to_command = true;
			}

		} else if (!strcmp(name,"format")) {
			if (!filter) filter = laidout->FindExportFilter(value,false);

		} else if (!strcmp(name,"imposition")) {
			//***
			cerr <<"Need to implement export with alternate imposition.."<<endl;

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

		} else if (!strcmp(name,"start")) { //for backwards compat
			IntAttribute(value,&start);

		} else if (!strcmp(name,"end")) { //for backwards compat
			IntAttribute(value,&end);

		} else if (!strcmp(name,"range")) {
			if (!isblank(value)) {
				range.Clear();
				if (!strcasecmp(value, "all")) range.AddRange(0,-1);
				else range.Parse(value, nullptr, false);
			}

		} else if (!strcmp(name,"reverse")) {
			reverse_order=BooleanAttribute(value);

		} else if (!strcmp(name,"paperrotation")) {
			IntAttribute(value,&paperrotation);
			if (paperrotation != 0 && paperrotation!= 90 && paperrotation != 180 && paperrotation != 270)
				paperrotation=0;

		} else if (!strcmp(name,"rotate180")) {
			rotate180 = BooleanAttribute(value);
			//if (isblank(value) || !strcasecmp(value,"none")) rotate180=0;
			//else if (!strcasecmp(value,"odd")) rotate180=1;
			//else rotate180=2;

		} else if (!strcmp(name,"batches")) {
			IntAttribute(value,&batches);

		} else if (!strcmp(name,"evenodd")) {
			if (isblank(value)) evenodd=All;
			else if (!strcmp(value,"odd")) evenodd=Odd;
			else if (!strcmp(value,"even")) evenodd=Even;
			else evenodd=All;

		} else if (!strcmp(name,"textaspaths")) {
			textaspaths = BooleanAttribute(value);

		} else if (!strcmp(name,"crop")) {
			if (isblank(value)) continue;
			//char bracket=0;
			//while (isspace(*value)) value++;
			if (*value=='[' || *value=='(') value++;
			double d[4];
			int n=DoubleListAttribute(value, d, 4, nullptr);
			if (n==4) {
				crop.minx=(d[0]<d[1] ? d[0] : d[1]);
				crop.maxx=(d[0]>d[1] ? d[0] : d[1]);
				crop.miny=(d[2]<d[3] ? d[2] : d[3]);
				crop.maxy=(d[2]>d[3] ? d[2] : d[3]); 
			}
		}
	}

	if (start >= 0 && end < 0) { range.Clear(); range.AddRange(start, start); }
	else if (start >=0 && end >= 0) { range.Clear(); range.AddRange(start, end); }
}

ObjectDef *DocumentExportConfig::makeObjectDef()
{
	return makeExportConfigDef();
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
 * config->papergroup is never nullptr, that start<=end, and that start and end are proper for
 * the requested spreads. If end<0, then make end the last spread. WARNING! This will
 * modify contents of config to have those sane values.
 *
 * If no doc is specified, then start=end=0 is passed to the filter. Also ensures that at least
 * one of doc and limbo is not nullptr before calling the filter.O
 *
 * For single file targets, obeys config->evenodd. For multifile targets, the output filter
 * must account for it.
 */
int export_document(DocumentExportConfig *config, Laxkit::ErrorLog &log)
{
	if (!config || !config->filter) {
		log.AddMessage(_("Missing export filter!"),ERROR_Fail);
		return 1;
	} else if (!(config->doc || config->limbo)) {
		log.AddMessage(_("Missing export source!"),ERROR_Fail);
		return 1;
	}

	DBG cerr << "export_document begin to \""<<config->filter->VersionName()<<"\"......."<<endl;

	PtrStack<char> files_outputted(LISTS_DELETE_Array);

//	if (config->target == DocumentExportConfig::TARGET_Command) {
//		// TODO! *** this currently bypasses all the safety checks normally done below...
//		if (isblank(config->command)) {
//			log.AddMessage(_("Expected command!"),ERROR_Fail);
//			return 1;
//		}
//		if (!config->filter) {
//			log.AddMessage(_("Missing filter!"),ERROR_Fail);
//			return 1;
//		}
//
//        char *cm = newstr(config->command);
//        appendstr(cm," ");
//
//        char tmp[256];
//        //cupsTempFile2(tmp,sizeof(tmp));
//		//tmpnam(tmp);
//		sprintf(tmp, "laidoutTmpXXXXXX");
//		mkstemp(tmp);
//        DBG cerr <<"attempting to write temp file "<<tmp<<" for export by command "<< cm <<endl;
//
//		FILE *f = fopen(tmp, "w");
//		if (f) { //make sure it is writable
//            fclose(f);
//
//            if (config->filter->Out(tmp,config,log)==0) {
//                appendstr(cm,tmp);
//
//                 //now do the actual command
//                int c=system(cm); //-1 for error, else the return value of the call
//                if (c!=0) {
//					log.AddMessage(_("Error running command!"),ERROR_Fail);
//					return 1;
//                } else {
//					//done, no error!
//                }
//                //***maybe should have to delete (unlink) tmp, but only after actually done printing?
//                //does cups keep file in place, or copy when queueing?
//
//            } else {
//                 //there was an error during filter export
//				log.AddMessage(_("Error exporting with command!"),ERROR_Fail);
//				return 1;
//            }
//            
//        } else {
//        	log.AddError(_("Could not open temp file!"));
//        	return 1;
//        }
//        return 0;
//    } //if command


	 //figure out what paper arrangement to print out on
	PaperGroup *papergroup=config->papergroup;
	if (papergroup && papergroup->papers.n==0) papergroup=nullptr;
	if (!papergroup && config->doc) papergroup=config->doc->imposition->papergroup;
	if (!papergroup && config->doc) papergroup=new PaperGroup(config->doc->imposition->GetDefaultPaper());
	if (papergroup && papergroup->papers.n==0) papergroup=nullptr;
	if (!papergroup) {
		 //use global default paper
		int c;
		for (c=0; c<laidout->papersizes.n; c++) {
			if (!strcasecmp(laidout->prefs.defaultpaper,laidout->papersizes.e[c]->name)) 
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
		config->range.Clear();
	} else {
		config->range.Max(config->doc->imposition->NumSpreads(config->layout), true);
	}

	int numoutput = config->range.NumInRanges() * papergroup->papers.n; //number of output "pages"
	//int numdigits=log10(numoutput);
	if (numoutput>1 && config->target == DocumentExportConfig::TARGET_Single && !(config->filter->flags&FILTER_MULTIPAGE)) {
		log.AddMessage(_("Filter cannot export more than one page to a single file."),ERROR_Fail);
		return 1;
	}

	int err=0;
	if (numoutput>1 && config->target == DocumentExportConfig::TARGET_Multi && !(config->filter->flags&FILTER_MANY_FILES)) {
		 //filter does not support outputting to many files, so loop over each paper and spread,
		 //exporting 1 file per each spread-paper combination

		int oldtarget = config->target;
		config->target = DocumentExportConfig::TARGET_Single;
		PaperGroup *pg;
		char *filebase=nullptr;
		if (config->tofiles) filebase=Laxkit::make_filename_base(config->tofiles);//###.ext -> %03d.ext
		else filebase=Laxkit::make_filename_base(config->filename);//###.ext -> %03d.ext
		if (papergroup->papers.n>1) {
			 // basically make base###.ps --> base(spread number)-(paper number).ps
			char *pos=strchr(filebase,'%'); //pos will never be 0
			while (*pos!='d') pos++;
			replace(filebase,"d-%d",pos-filebase,1,nullptr);
		}
		char filename[strlen(filebase)+20];

		PaperGroup *oldpg = config->papergroup;
		IndexRange range = config->range;
		IndexRange range_orig = config->range;
		int filenum = 0;

		for (int c = (config->reverse_order ? range.End() : range.Start());
				 c >= 0;
			 	 c = (config->reverse_order ? range.Previous() : range.Next())) //loop over each spread
		{
			if (config->evenodd == DocumentExportConfig::Even && c%2==0) continue;
			if (config->evenodd == DocumentExportConfig::Odd  && c%2==1) continue;

			for (int p=0; p<papergroup->papers.n; p++) { //loop over each paper in a spread
				config->range.Clear();
				config->range.AddRange(c,c);

				config->curpaperrotation = config->paperrotation;
				if (config->rotate180 && c % 2 == 1) {
					config->curpaperrotation += 180;
					if (config->curpaperrotation >= 360) config->curpaperrotation -= 360;
				}

				if (papergroup->papers.n == 1)
					sprintf(filename, filebase, c);
				else
					sprintf(filename, filebase, c, p);

				pg = new PaperGroup(papergroup->papers.e[p]);
				if (papergroup->objs.n()) {
					for (int o = 0; o < papergroup->objs.n(); o++) pg->objs.push(papergroup->objs.e(o));
				}
				config->papergroup = pg;

				err = config->filter->Out(filename, config, log);
				pg->dec_count();
				if (err > 0) break;

				if (err == 0 && config->send_to_command) files_outputted.push(newstr(filename));
				filenum++;
			}
			if (err > 0) break;
		}

		//restore config, since we messed with it
		config->range      = range_orig;
		config->papergroup = oldpg;
		config->target     = oldtarget;
		delete[] filebase;

		if (err) {
			// char scratch[strlen(_("Export failed at file %d out of %d"))+20];
			// sprintf(scratch,_("Export failed at file %d out of %d"), filenum, numoutput);
			// log.AddMessage(scratch,ERROR_Fail);
			// ---
			log.AddError(0,0,0, _("Export failed at file %d out of %d"), filenum, numoutput);
		}

	} else {
		 //output filter can handle multiple pages...
		 //
		if (config->target == DocumentExportConfig::TARGET_Single && config->batches > 0 && config->batches < config->range.NumInRanges()) {
			 //divide into batches

			IndexRange range_orig = config->range;
			IndexRange range = config->range;
			char *oldfilename = config->filename;
			char *fname       = nullptr;
			char  str[20];
			char *ext = strrchr(oldfilename, '.');

			for (int c = range.Start(); err == 0 && c >= 0; /*no inc step*/) {
				config->range.Clear();
				int start = c, end = c;
				for (int cc = 0; c >= 0 && cc < config->batches; cc++) {
					config->range.AddRange(c,c); //this is rather hideous, but at least don't have to subdivide ranges
					end = c;
					c = range.Next();
				}

				sprintf(str, "%d-%d", start, end);
				if (ext) {
					fname = newnstr(oldfilename, ext - oldfilename + 1);
					appendstr(fname, str);
					appendstr(fname, ext);
				} else {
					fname = newstr(oldfilename);
					appendstr(fname, str);
				}

				config->filename = fname;
				err              = config->filter->Out(nullptr, config, log);
				if (err == 0 && config->send_to_command) files_outputted.push(newstr(fname));
				delete[] fname;
			}

			config->range = range_orig;
			config->filename=oldfilename;

		} else {
			err = config->filter->Out(nullptr,config,log); //send all pages at once to filter
			if (err == 0 && config->send_to_command) files_outputted.push(newstr(config->filename));
		}
	}
	
	if (err == 0 && config->send_to_command) {
		if (files_outputted.n == 0) {
			err = 1;
			log.AddWarning(_("No files to send to command!"));
		} else {
			//run command
			bool run_in_background = true;

			if (!config->command) {
				log.AddError(_("Config is missing command!"));
				err = 1;
			} else {
				config->command->RunCommand(files_outputted, log, run_in_background);
				if (log.Errors()) err = 1;
			}
			
			// delete if necessary
			if (!run_in_background && config->del_after_command) {
				for (int c=0; c<files_outputted.n; c++) {
					DBG cerr << "Deleting temporary exported file "<<files_outputted.e[c]<<endl;
					unlink(files_outputted.e[c]);
				}
			}
		}
	}

	DBG cerr << "export_document end."<<endl;

	if (err>0) {
		log.AddMessage(_("Export failed."),ERROR_Fail);
		return 1;
	} 
	return 0;
}


} // namespace Laidout

