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
// Copyright (C) 2007 by Tom Lechner
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

DocumentExportConfig::DocumentExportConfig()
{
	target=0;
	filename=NULL;
	tofiles=NULL;
	start=end=0;
	layout=0;
	doc=NULL;
	filter=NULL;
	papergroup=NULL;
	limbo=NULL;
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
	if (doc && doc->docstyle && doc->docstyle->imposition) {
		fprintf(f,"%simposition \"%s\"\n",spc,doc->docstyle->imposition->whattype());
		fprintf(f,"%slayout \"%s\"\n",spc,doc->docstyle->imposition->LayoutName(layout));
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
			for (c2=0; c2<doc->docstyle->imposition->NumLayouts(); c2++) {
				if (!strcmp(value,doc->docstyle->imposition->LayoutName(c2))) break;
			}
			if (c2==doc->docstyle->imposition->NumLayouts()) {
				for (c2=0; c2<doc->docstyle->imposition->NumLayouts(); c2++) {
					if (!strncasecmp(value,doc->docstyle->imposition->LayoutName(c2),strlen(value))) break;
				}
			}
			if (c2==doc->docstyle->imposition->NumLayouts()) c2=0;
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
 * \todo These can currently be "document", "object", "image" (a raster image),
 * or resources such as "gradient", "palette", or "net".
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
/*! \fn int ImportFilter::In(const char *file, Laxkit::anObject *context, char **error_ret)
 * \brief The function that outputs the stuff.
 *
 * If file!=NULL, then output to that single file, and ignore the files in context.
 *
 * context must be a configuration object that the filter understands. For instance, this
 * might be a DocumentExportConfig object.
 *
 * On success, return 0. If there are any warnings they are put in error_ret.
 * On failure, return nonzero, and appends error messages to error_ret.
 */

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
 *   in the given format, and other objects that can be transfered, but only in a lossless way.
 */
/*! \fn int ExportFilter::Out(const char *file, Laxkit::anObject *context, char **error_ret)
 * \brief The function that outputs the stuff.
 *
 * context must be a configuration object that the filter understands. For instance, this
 * might be a DocumentExportConfig object.
 *
 * On success, return 0. If there are any warnings they are put in error_ret.
 * On failure, return nonzero, and append error messages to error_ret.
 */
	



//------------------------------- export_document() ----------------------------------

//! Export a document from a file or a live Document.
/*! Return 0 for export successful. 0 is also returned If there are non-fatal errors, in which case
 * the warning messages get appended to error_ret. If there are fatal errors, then error_ret
 * gets appended with a message, and 1 is returned.
 *
 * If the filter cannot support multiple file output, but there are multiple files to be output,
 * then this function will call filter->Out() with the correct data for each file.
 *
 * Also does sanity checking on config->papergoup, config->start, and config->end. Ensures that
 * config->papergroup is never NULL, that start<=end, and that start and end are proper for
 * the requested spreads.
 *
 * If no doc is specified, then start=end=0 is passed to the filter. Also ensures that at least
 * one of doc and limbo is not NULL before calling the filter.
 *
 * \todo perhaps command facility should be here... currently it sits in ExportDialog.
 */
int export_document(DocumentExportConfig *config,char **error_ret)
{
	if (!config->filter || !(config->doc || config->limbo)) {
		if (error_ret) appendline(*error_ret,_("Bad export configuration"));
		return 1;
	}

	DBG cerr << "export_document begin to \""<<config->filter->VersionName()<<"\"......."<<endl;

	 //figure out what paper arrangement to print out on
	PaperGroup *papergroup=config->papergroup;
	if (papergroup && papergroup->papers.n==0) papergroup=NULL;
	if (!papergroup && config->doc) papergroup=config->doc->docstyle->imposition->papergroup;
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
		if (config->start<0) config->start=0;
		else if (config->start>=config->doc->docstyle->imposition->NumSpreads(config->layout))
			config->start=config->doc->docstyle->imposition->NumSpreads(config->layout)-1;
		if (config->end<config->start) config->end=config->start;
		else if (config->end>=config->doc->docstyle->imposition->NumSpreads(config->layout))
			config->end=config->doc->docstyle->imposition->NumSpreads(config->layout)-1;
	}

	int n=(config->end-config->start+1)*papergroup->papers.n;
	if (n>1 && config->target==0 && !(config->filter->flags&FILTER_MULTIPAGE)) {
		if (error_ret) appendline(*error_ret,_("Filter cannot export more than one page to a single file."));
		return 1;
	}

	int err=0;
	if (n>1 && config->target==1 && !(config->filter->flags&FILTER_MANY_FILES)) {
		 //filter does not support outputting to many files, so loop over each paper
		config->target=0;
		PaperGroup *pg;
		char *filebase=LaxFiles::make_filename_base(config->filename);//###.ext -> %03d.ext
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
		for (int c=start; c<=end; c++) {
			for (int p=0; p<papergroup->papers.n; p++) {
				config->start=config->end=c;

				if (papergroup->papers.n==1) sprintf(filename,filebase,c);
				else sprintf(filename,filebase,c,p);

				pg=new PaperGroup(papergroup->papers.e[c]);
				config->papergroup=pg;

				err=config->filter->Out(filename,config,error_ret);
				pg->dec_count();
				if (err) { left=papergroup->papers.n-p; break; }
			}
			if (err) { left+=(end-c+1)*papergroup->papers.n; break; }
		}
		config->papergroup=oldpg;
		config->start=start;
		config->end=end;
		delete[] filebase;

		if (error_ret && left) {
			char scratch[strlen(_("Export failed at file %d out of %d"))+20];
			sprintf(scratch,_("Export failed at file %d out of %d"),n-left,n);
			appendline(*error_ret,scratch);
		}
	} else err=config->filter->Out(NULL,config,error_ret);
	
	DBG cerr << "export_document end."<<endl;

	if (err) {
		if (error_ret) appendline(*error_ret,_("Export failed."));
		return 1;
	} 
	//if (error_ret) appendline(*error_ret,_("Exported."));
	return 0;
}


//------------------------------ ImportConfig ----------------------------
/*! \class ImportConfig
 */

ImportConfig::ImportConfig()
{
	filename=NULL;
	instart=inend=-1;
	topage=spread=layout=-1;
	doc=NULL;
	toobj=NULL;
	filter=NULL;
}

ImportConfig::~ImportConfig()
{
	if (filename) delete[] filename;
	if (doc) doc->dec_count();
	if (toobj) toobj->dec_count();
	//if (filter) filter->dec_count(); ***filter assumed non-local, always living in laidoutapp?
}

