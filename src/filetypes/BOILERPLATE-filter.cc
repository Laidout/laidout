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
// Copyright (C) 2020 by Tom Lechner
//

#include <unistd.h>

#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/textonpathinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/gradientstrip.h>
#include <lax/colors.h>

#include "../language.h"
#include "pageatlas.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../printing/psout.h"
#include "../core/utils.h"
#include "../ui/headwindow.h"
#include "../impositions/singles.h"
#include "../dataobjects/mysterydata.h"
#include "../core/drawdata.h"


#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {


typedef DocumentExportConfig BoilerPlateExportConfig; //in case we change later...



//--------------------------------- install BoilerPlate filter

//! Tells the Laidout application that there's a new filter in town.
void installBoilerPlateFilter()
{
	BoilerPlateExportFilter *BoilerPlateout=new BoilerPlateExportFilter;
	BoilerPlateout->GetObjectDef();
	laidout->PushExportFilter(BoilerPlateout);
	
	// BoilerPlateImportFilter *BoilerPlatein=new BoilerPlateImportFilter;
	// BoilerPlatein->GetObjectDef();
	// laidout->PushImportFilter(BoilerPlatein);
}



//------------------------------------ BoilerPlateExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newBoilerPlateExportConfig()
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"BoilerPlate"))
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//! For now, just returns createExportConfig(), with filter forced to BoilerPlate.
int createBoilerPlateExportConfig(ValueHash *context,ValueHash *parameters,Value **v_ret,ErrorLog &log)
{
	DocumentExportConfig *d=NULL;
	Value *v=NULL;
	int status=createExportConfig(context,parameters,&v,log);
	if (status==0 && v && v->type()==VALUE_Object) d=dynamic_cast<DocumentExportConfig *>(((ObjectValue *)v)->object);

	if (d) for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"BoilerPlate")) {
			d->filter=laidout->exportfilters.e[c];
			break;
		}
	}
	*v_ret=v;

	return 0;
}

//------------------------------------ BoilerPlateImportConfig ----------------------------------

//! For now, just returns a new ImportConfig.
Value *newBoilerPlateImportConfig()
{
	ImportConfig *d=new ImportConfig;
	for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"BoilerPlate"))
			d->filter=laidout->importfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//! For now, just returns createImportConfig(), with filter forced to BoilerPlate.
int createBoilerPlateImportConfig(ValueHash *context,ValueHash *parameters,Value **v_ret,ErrorLog &log)
{
	ImportConfig *d=NULL;
	Value *v=NULL;
	int status=createImportConfig(context,parameters,&v,log);
	if (status==0 && v && v->type()==VALUE_Object) d=dynamic_cast<ImportConfig *>(((ObjectValue *)v)->object);

	if (d) for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"BoilerPlate")) {
			d->filter=laidout->importfilters.e[c];
			break;
		}
	}
	*v_ret=v;

	return 0;
}

//---------------------------- BoilerPlateExportFilter --------------------------------

/*! \class BoilerPlateExportFilter
 * \brief Export pages to a series of images arranged with n-up.
 *
 * These are meant to aid importing books to 3-d software.
 */

BoilerPlateExportFilter::BoilerPlateExportFilter()
{
	flags = FILTER_MULTIPAGE;
}

//! "BoilerPlate 1.4.5".
const char *BoilerPlateExportFilter::VersionName()
{
	return _("BoilerPlate");
}

const char *BoilerPlateExportFilter::Version()
{
	return "1.0"; //the max version handled. import+export acts more like a pass through
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *BoilerPlateExportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef = stylemanager.FindDef("BoilerPlateExportConfig");
	if (styledef) return styledef; 

	styledef = makeObjectDef();
	makestr(styledef->name,"BoilerPlateExportConfig");
	makestr(styledef->Name,_("BoilerPlate Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a BoilerPlate file."));
	styledef->newfunc   = newBoilerPlateExportConfig;
	styledef->stylefunc = createBoilerPlateExportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}


//! Export the document as a BoilerPlate file.
int BoilerPlateExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DBG cerr <<"-----BoilerPlate export start-------"<<endl;

	BoilerPlateExportConfig *config = dynamic_cast<BoilerPlateExportConfig *>(context);
	if (!config) return 1;

	Document *  doc        = config->doc;
	IndexRange *range      = &config->pagerange;
	int         layout     = config->layout;
	Group *     limbo      = config->limbo;
	bool        rev        = config->reverse_order;
	PaperGroup *papergroup = config->papergroup;
	if (!filename) filename = config->filename;

	// we must have something to export...
	if (!doc) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Page atlas require a document!"),ERROR_Fail);
		return 1;
	}

	 //we must be able to open the export file location...
	Utf8String file;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 2;
		}
		file = doc->saveas;
		// appendstr(file,".sla");
	} else file = filename;

	FILE *f = nullptr;
	f = open_file_for_writing(file.c_str(),0,&log);
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		log.AddMessage(_("Could not open file for writing."),ERROR_Fail);
		return 3;
	}
	

	setlocale(LC_ALL,"C");

	int     warning = 0;
	Spread *spread  = NULL;
	Group * g       = NULL;

	// figure out paper size and orientation
	int landscape=0,plandscape;
	double paperwidth,paperheight;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	PaperStyle *defaultpaper = papergroup->papers.e[0]->box->paperstyle;

	landscape   = defaultpaper->landscape();
	paperwidth  = defaultpaper->width;
	paperheight = defaultpaper->height;

	int totalnumpages = 0;

	if (config->evenodd == DocumentExportConfig::Even) {
		for (int c = pagerange->Start(); c >= 0; c = pagerange->Next())
			if (c % 2 == 0) totalnumpages++;
	} else if (config->evenodd == DocumentExportConfig::Odd) {
		for (int c = pagerange->Start(); c >= 0; c = pagerange->Next())
			if (c % 2 == 1) totalnumpages++;
	} else {
 		totalnumpages = (pagerange->NumInRanges());
	}

	totalnumpages *= papergroup->papers.n;

	groups.flush();
	ongroup=0;
	int p;
	double m[6],mm[6],mmm[6],mmmm[6],ms[6];
	transform_identity(m);

	int paperrotation;

	for (int c = (rev ? pagerange->End() : pagerange->Start());
		 c >= 0;
		 c = (rev ? pagerange->Previous() : pagerange->Next())) 
	{ //for each spread
		if (config->evenodd == DocumentExportConfig::Even && c % 2 == 0) continue;
		if (config->evenodd == DocumentExportConfig::Odd  && c % 2 == 1) continue;

		if (doc) spread = doc->imposition->Layout(layout,c);

		for (p=0; p<papergroup->papers.n; p++) { //for each paper
					
			if (papergroup->objs.n()) {
				// .... output papergroup objects
			}

			//if (limbo && limbo->n()) {
			//	appendobjfordumping(config, pageobjects,palette,limbo);
			//}

			if (spread) {
				if (spread->marks) {
					//objects created by the imposition
					appendobjfordumping(config, pageobjects,palette,spread->marks);
				}

				// for each page in spread layout..
				for (c2 = 0; c2 < spread->pagestack.n(); c2++) {
					pg = spread->pagestack.e[c2]->index;
					if (pg < 0 || pg >= doc->pages.n) continue;

					// for each layer on the page..
					for (l = 0; l < doc->pages[pg]->layers.n(); l++) {
						// for each object in layer
						g = dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (c3 = 0; c3 < g->n(); c3++) {
							appendobjfordumping(config, pageobjects, palette, g->e(c3));
						}
					}
				}
			} //if (spread)
		} //for each paper
		if (spread) { delete spread; spread = nullptr; }
	} //for each spread

	
	fclose(f);
	setlocale(LC_ALL,"");

	DBG cerr <<"-----BoilerPlate export end success-------"<<endl;

	return 0;
}





// //------------------------------------ BoilerPlateImportFilter ----------------------------------
// /*! \class BoilerPlateImportFilter
//  * \brief Filter to, amazingly enough, import svg files.
//  *
//  * \todo perhaps when dumping in to an object, just bring in all objects as they are arranged
//  *   in the scratch space... or if no doc or obj, option to import that way, but with
//  *   a custom papergroup where the pages are... perhaps need a freestyle imposition type
//  *   that is more flexible with differently sized pages.
//  */


// const char *BoilerPlateImportFilter::VersionName()
// {
// 	return _("BoilerPlate");
// }

// /*! \todo can do more work here to get the actual version of the file...
//  */
// const char *BoilerPlateImportFilter::FileType(const char *first100bytes)
// {
// 	*** ok if pingable image file
// 	if (!strstr(first100bytes,"<BoilerPlateUTF8NEW")) return NULL;
// 	return "1.0";

// }

// //! Try to grab from stylemanager, and install a new one there if not found.
// /*! The returned def need not be dec_counted.
//  */
// ObjectDef *BoilerPlateImportFilter::GetObjectDef()
// {
// 	ObjectDef *styledef;
// 	styledef = stylemanager.FindDef("BoilerPlateImportConfig");
// 	if (styledef) return styledef; 

// 	styledef = makeObjectDef();
// 	makestr(styledef->name,"BoilerPlateImportConfig");
// 	makestr(styledef->Name,_("BoilerPlate Import Configuration"));
// 	makestr(styledef->description,_("Configuration to import a BoilerPlate file."));
// 	styledef->newfunc=newBoilerPlateImportConfig;
// 	styledef->stylefunc=createBoilerPlateImportConfig;

// 	stylemanager.AddObjectDef(styledef,0);
// 	styledef->dec_count();

// 	return styledef;
// }

// //! Import from BoilerPlate files.
// /*! If in->doc==NULL and in->toobj==NULL, then create a new document.
//  */
// int BoilerPlateImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log, const char *filecontents,int contentslen)
// {
// 	DBG cerr <<"-----BoilerPlate import start-------"<<endl;

// 	BoilerPlateImportConfig *in=dynamic_cast<BoilerPlateImportConfig *>(context);
// 	if (!in) {
// 		log.AddMessage(_("Missing BoilerPlateImportConfig!"),ERROR_Fail);
// 		return 1;
// 	}

// 	Document *doc = in->doc;


// 	 //create the document if necessary
// 	if (!doc && !in->toobj) {
// 		Imposition *imp = new Singles;
// 		paper->flags    = 0;
// 		imp->SetPaperSize(paper);
// 		doc = new Document(imp, Untitled_name());  // incs imp count
// 		imp->dec_count();                          // remove initial count
// 	}

// 	if (doc && docpagenum+(end-start)>=doc->pages.n) //create enough pages to hold the BoilerPlate pages
// 		doc->NewPages(-1,(docpagenum+(end-start+1))-doc->pages.n);

	




// 	 //if doc is new, push into the project
// 	if (doc && doc != in->doc) {
// 		laidout->project->Push(doc);
// 		laidout->app->addwindow(newHeadWindow(doc));
// 		doc->dec_count();
// 	}
	
// 	DBG cerr <<"-----BoilerPlate import end successfully-------"<<endl;
// 	delete att;
// 	return 0;

// }



} // namespace Laidout

