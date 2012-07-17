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
// Copyright (C) 2012 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../headwindow.h"
#include "../impositions/singles.h"
#include "../utils.h"
#include "../drawdata.h"
#include "../printing/psout.h"
#include "../dataobjects/epsdata.h"
#include "../dataobjects/mysterydata.h"
#include "laidoutimport.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



//--------------------------------- install Laidout filter

//! Tells the Laidout application that there's a new filter in town.
void installLaidoutFilter()
{
//	LaidoutOutFilter *laidoutout=new LaidoutOutFilter;
//	laidoutout->GetStyleDef();
//	laidout->PushExportFilter(laidoutout);
	
	LaidoutInFilter *laidoutin=new LaidoutInFilter;
	laidoutin->GetStyleDef();
	laidout->PushImportFilter(laidoutin);
}


//------------------------------------ LaidoutExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Style *newLaidoutExportConfig(StyleDef*)
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Laidout"))
			d->filter=laidout->exportfilters.e[c];
	}
	return d;
}

//------------------------------------ LaidoutImportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Style *newLaidoutImportConfig(StyleDef*)
{
	ImportConfig *d=new ImportConfig;
	for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"Laidout"))
			d->filter=laidout->importfilters.e[c];
	}
	return d;
}


//------------------------------- LaidoutOutFilter --------------------------------------
/*! \class LaidoutOutFilter
 * \brief Output filter for Laidout files.
 *
 * \todo Maybe have option to export to new laidout document, rather than file on disk
 *    This would ease capturing various in-program things for custom processing
 */

LaidoutOutFilter::LaidoutOutFilter()
{
	flags=FILTER_MULTIPAGE;
}

const char *LaidoutOutFilter::VersionName()
{
	return _("Laidout");
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
StyleDef *LaidoutOutFilter::GetStyleDef()
{
	StyleDef *styledef;
	styledef=stylemanager.FindDef("LaidoutExportConfig");
	if (styledef) return styledef; 

	styledef=makeStyleDef();
	makestr(styledef->name,"LaidoutExportConfig");
	makestr(styledef->Name,_("Laidout Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a Laidout file."));
	styledef->newfunc=newLaidoutExportConfig;

	stylemanager.AddStyleDef(styledef);
	styledef->dec_count();

	return styledef;
}


//! Save the document as a Laidout file.
/*! This only saves groups and images, and the page size and orientation.
 *
 * If the paper name is not recognized as a Laidout paper name, which are
 * A0-A6, Executive (7.25 x 10.5in), Legal, Letter, and Tabloid/Ledger, then
 * Letter is used.
 *
 * \todo if unknown paper, should really use some default paper size, if it is valid, 
 *   and then otherwise "Letter", or choose a size that is big enough to hold the spreads
 * \todo for singles, should figure out what paper size to export as..
 */
int LaidoutOutFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;
	
	Document *doc =out->doc;
	//int start     =out->start;
	//int end       =out->end;
	//int layout    =out->layout;
	Group *limbo  =out->limbo;
	//PaperGroup *papergroup=out->papergroup;
	if (!filename) filename=out->filename;
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}
	
	 //we must be able to open the export file location...
	//FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (isblank(doc->saveas)) {
			//DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".laidout");
	} else file=newstr(filename);

	cerr <<"*** laidoutOutFilter not quite implemented!"<<endl;

//	f=open_file_for_writing(file,0,&log);//appends any error string
//	if (!f) {
//		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
//		delete[] file;
//		return 3;
//	}
//	
//	setlocale(LC_ALL,"C");
//	
//	****export full layout with optional outlines
//	
//	fclose(f);
//	setlocale(LC_ALL,"");
	delete[] file;
	return 0;
}

////------------------------------------- LaidoutInFilter -----------------------------------
/*! \class LaidoutInFilter 
 * \brief Laidout input filter.
 */


const char *LaidoutInFilter::FileType(const char *first100bytes)
{
	if (!strncmp(first100bytes,"#Laidout ",9) && strstr(first100bytes,"Document"));
	const char *version=first100bytes+9;
	if (!strncmp(version,"0.07", 4)) return "0.07";
	if (!strncmp(version,"0.08", 4)) return "0.08";
	if (!strncmp(version,"0.09", 4)) return "0.09";
	if (!strncmp(version,"0.091",5)) return "0.091";
	if (!strncmp(version,"0.092",5)) return "0.092";
	return NULL;
}

const char *LaidoutInFilter::VersionName()
{
	return _("Laidout");
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 *
 * \todo import only parts of a laidout file, like limbos only, palettes, other resources....
 *    might need a specialized import for this to happen reasonably
 */
StyleDef *LaidoutInFilter::GetStyleDef()
{
	StyleDef *styledef;
	styledef=stylemanager.FindDef("LaidoutImportConfig");
	if (styledef) return styledef; 

	styledef=makeStyleDef(); //creates default import config styledef, need to modify to suit
	makestr(styledef->name,"LaidoutImportConfig");
	makestr(styledef->Name,_("Laidout Import Configuration"));
	makestr(styledef->description,_("Configuration to import a Laidout file."));
	styledef->newfunc=newLaidoutImportConfig;

	stylemanager.AddStyleDef(styledef);
	styledef->dec_count();

	return styledef;
}

//! Import a Laidout file.
/*! If doc!=NULL, then import the file to Document starting at page startpage.
 * If doc==NULL, create a brand new Singles based document.
 *
 * Does no check on the file to ensure that it is in fact a Laidout file.
 *
 * \todo currently, this just imports (single) page data onto the new (single) document pages. You may
 *   not import different spread types onto particular spread types.
 * \todo one could implement dissected long document support by importing laidout data as mystery
 *   data, and final processing is activated as a render process, that proceeds incrementally
 */
int LaidoutInFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log)
{
	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	Document *doc=in->doc;

	Document fdoc;
	int status=fdoc.Load(file,log);
	if (status==0) return 1; //load fail!


//	 //create the document
//	if (!doc && !in->toobj) {
//		Imposition *imp=new Singles;
//		PaperStyle *paper=new PaperStyle;
//		paper->flags=((paper->flags)&~1)|(landscape?1:0);
//		imp->SetPaperSize(paper);
//		doc=new Document(imp,Untitled_name());
//		imp->dec_count();
//	}

	//Import pages [in->instart..in->inend] to in->topage.
	int curpage=in->topage;
	SomeData *contents=NULL;
	for (int c=in->instart; c<=in->inend; c++) {
		while (doc->pages.n<=curpage) doc->NewPages(doc->pages.n,1);

		for (int c2=0; c2<fdoc.pages.e[c]->layers.n(); c2++) {
			contents=fdoc.pages.e[c]->layers.e(c2);
			doc->pages.e[curpage]->layers.push(contents->duplicate());//dup forces new ids to current doc
			// *** tag with some meta to say where imported from?

			 // *** optionally scale/center to fit?
			if (in->scaletopage) {
			}
		}
		curpage++;
	}
	

	 //if doc is new, push into the project
	if (doc && doc!=in->doc) {
		laidout->project->Push(doc);
		laidout->app->addwindow(newHeadWindow(doc));
	}
	
	return 0;
}


