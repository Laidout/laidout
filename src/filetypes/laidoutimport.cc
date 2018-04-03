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



namespace Laidout {



//--------------------------------- install Laidout filter

//! Tells the Laidout application that there's a new filter in town.
void installLaidoutFilter()
{
//	LaidoutOutFilter *laidoutout=new LaidoutOutFilter;
//	laidoutout->GetObjectDef();
//	laidout->PushExportFilter(laidoutout);
	
	LaidoutInFilter *laidoutin=new LaidoutInFilter;
	laidoutin->GetObjectDef();
	laidout->PushImportFilter(laidoutin);
}


//------------------------------------ LaidoutExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newLaidoutExportConfig()
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Laidout"))
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//------------------------------------ LaidoutImportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newLaidoutImportConfig()
{
	ImportConfig *d=new ImportConfig;
	for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"Laidout"))
			d->filter=laidout->importfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
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
ObjectDef *LaidoutOutFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("LaidoutExportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"LaidoutExportConfig");
	makestr(styledef->Name,_("Laidout Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a Laidout file."));
	styledef->newfunc=newLaidoutExportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}


//! Save the document as a Laidout file.
/*!
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
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
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

/*! Unrecognized version numbers assume the latest format.
 */
const char *LaidoutInFilter::FileType(const char *first100bytes)
{
	if (!strncmp(first100bytes,"#Laidout ",9) && strstr(first100bytes,"Document"));
	const char *version=first100bytes+9;
	if (!strncmp(version,"0.092",5)) return "0.092";
	if (!strncmp(version,"0.091",5)) return "0.091";
	if (!strncmp(version,"0.07", 4)) return "0.07";
	if (!strncmp(version,"0.08", 4)) return "0.08";
	if (!strncmp(version,"0.09", 4)) return "0.09";

	return LAIDOUT_VERSION;
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
ObjectDef *LaidoutInFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("LaidoutImportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef(); //creates default import config styledef, need to modify to suit
	makestr(styledef->name,"LaidoutImportConfig");
	makestr(styledef->Name,_("Laidout Import Configuration"));
	makestr(styledef->description,_("Configuration to import a Laidout file."));
	styledef->newfunc=newLaidoutImportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}

//! Import a Laidout file.
/*! If doc!=NULL, then import the file to Document starting at page startpage.
 * If doc==NULL, create a brand new document copied from the loaded document, with blank pages for any out of range.
 *
 * Does no check on the file to ensure that it is in fact a Laidout file.
 *
 * \todo currently, this just imports (single) page data onto the new (single) document pages. You may
 *   not import different spread types onto particular spread types.
 * \todo one could implement dissected long document support by importing laidout data as mystery
 *   data, and final processing is activated as a render process, that proceeds incrementally
 */
int LaidoutInFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log, const char *filecontents,int contentslen)
{
	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	 // *** needs to be put in a custom ImportConfig:
	bool merge_to_existing_layers = true;
	//bool append_new_pages = false; // todo: insert new pages instead of adding to current
	//bool scale_to_margins = true; <- use in->scaletopage


	Document *doc=in->doc;

	Document *fdoc=new Document(NULL, file);
	int status=fdoc->Load(file,log);
	if (status==0) {
		delete fdoc;
		return 1; //load fail!
	}

	if (in->inend<0) in->inend=fdoc->pages.n-1;

	 //create the document
	//if (!doc && !in->toobj) {
	if (!doc) {
		 //read in chosen pages to a new document
		doc=fdoc;
		for (int c=in->inend+1; c<doc->pages.n; ) doc->pages.remove(c);
		for (int c=in->instart; c>0; c++) doc->pages.remove(c);
		if (in->topage>0) doc->NewPages(0,in->topage);

		laidout->project->Push(doc);
		laidout->app->addwindow(newHeadWindow(doc));
		doc->dec_count();
		return 0;
	}

	//Import pages [in->instart..in->inend] to in->topage
	int curpage=in->topage;
	if (curpage<0) curpage=0;
	SomeData *obj=NULL;
	SomeData origpagedims, newpagedims;
	DrawableObject *layer;
	SomeData *layerdup, *kid;
	double tt[6];

	for (int c=in->instart; c<=in->inend; c++) {
		while (doc->pages.n<=curpage) doc->NewPages(doc->pages.n,1);
		newpagedims.setIdentity();
		newpagedims.setbounds(0,doc->pages.e[curpage]->pagestyle->w(), 0,doc->pages.e[curpage]->pagestyle->h());
		origpagedims.setIdentity();
		origpagedims.setbounds(0,fdoc->pages.e[c]->pagestyle->w(), 0,fdoc->pages.e[c]->pagestyle->h());

		if (in->scaletopage) {
			 //center and scale
			origpagedims.fitto(NULL,&newpagedims, 50,50, in->scaletopage);

		} else {
			 //only center
			origpagedims.origin(origpagedims.origin()
					+(flatpoint(doc->pages.e[curpage]->pagestyle->w(),doc->pages.e[curpage]->pagestyle->h())
						- flatpoint(origpagedims.maxx,origpagedims.maxy))/2);
		}

		 //copy page contents
		for (int c2=0; c2<fdoc->pages.e[c]->layers.n(); c2++) {
			obj      = fdoc->pages.e[c]->layers.e(c2);

			layerdup = obj->duplicate(NULL); 
			layer=dynamic_cast<DrawableObject*>(layerdup);

			if (layer) {
				//apply any shift to layer contents, not the layer itself
				for (int c3=0; c3<layer->n(); c3++) {
					kid=layer->e(c3);
					transform_mult(tt, kid->m(),origpagedims.m());
					kid->m(tt);
				}
			}

			if (merge_to_existing_layers) {
				Page *page = doc->pages.e[curpage];
				while (page->n() < layer->n()) page->PushLayer(NULL);

				for (int c3=0; c3<layer->n(); c3++) {
					page->e(c2)->push(layer->e(c3));
				}

			} else {
				 //insert brand new layer
				doc->pages.e[curpage]->layers.push(layer);//dup forces new ids to current doc(?)
			}
			// *** tag with some meta to say where imported from?

			layer->dec_count();
		}
		curpage++;
	}
	
	fdoc->dec_count();

	 //if doc is new, push into the project
	if (doc && doc!=in->doc) {
		laidout->project->Push(doc);
		//laidout->app->addwindow(newHeadWindow(doc));
	}
	
	return 0;
}

} // namespace Laidout

