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
#include <lax/palette.h>
#include <lax/colors.h>

#include "pageatlas.h"
#include "../language.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../printing/psout.h"
#include "../core/utils.h"
#include "../ui/headwindow.h"
#include "../impositions/singles.h"
#include "../dataobjects/mysterydata.h"
#include "../core/drawdata.h"

//template implementation
#include <lax/lists.cc>
#include <lax/refptrstack.cc>


#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;



namespace Laidout {


//--------------------------------- install PageAtlas filter

//! Tells the Laidout application that there's a new filter in town.
void installPageAtlasFilter()
{
	PageAtlasExportFilter *PageAtlasout=new PageAtlasExportFilter;
	PageAtlasout->GetObjectDef();
	laidout->PushExportFilter(PageAtlasout);
	
	// PageAtlasImportFilter *PageAtlasin=new PageAtlasImportFilter;
	// PageAtlasin->GetObjectDef();
	// laidout->PushImportFilter(PageAtlasin);
}



//------------------------------------ PageAtlasExportConfig ----------------------------------

/*! \class PageAtlasExportConfig
 * \brief Holds extra config for export.
 */
class PageAtlasExportConfig : public DocumentExportConfig
{
  public:
	int pages_wide;
	int pages_tall;
	int px_width;
	int px_height;
	bool round_up_to_power_2;
	ColorValue *color;
	// Laxkit::Color *color;

	PageAtlasExportConfig();
	PageAtlasExportConfig(DocumentExportConfig *config);
	virtual ~PageAtlasExportConfig();
    virtual ObjectDef* makeObjectDef();
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
	virtual Laxkit::Attribute * dump_out_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Value* duplicate();
};

//! Set the filter to the Image export filter stored in the laidout object.
PageAtlasExportConfig::PageAtlasExportConfig()
{
	pages_wide          = 1;
	pages_tall          = 1;
	px_width            = 128;
	px_height           = 128;
	round_up_to_power_2 = true;
	color               = new ColorValue();
	color->color.SetRGB(1.,1.,1.,1.);

	// find the default filter for this config
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"PageAtlas")) {
			filter = laidout->exportfilters.e[c];
			break; 
		}
	}
}

/*! Base on config, copy over its stuff.
 */
PageAtlasExportConfig::PageAtlasExportConfig(DocumentExportConfig *config)
	: DocumentExportConfig(config)
{

	PageAtlasExportConfig *conf = dynamic_cast<PageAtlasExportConfig*>(config);
	if (conf) {
		pages_wide          = conf->pages_wide;
		pages_tall          = conf->pages_tall;
		px_width            = conf->px_width;
		px_height           = conf->px_height;
		round_up_to_power_2 = conf->round_up_to_power_2;
		color               = conf->color;
		if (color) color = dynamic_cast<ColorValue*>(color->duplicate());

	} else {
		pages_wide          = 1;
		pages_tall          = 1;
		px_width            = 128;
		px_height           = 128;
		round_up_to_power_2 = true;
		color               = nullptr;
	}
	// ... filter?
}

PageAtlasExportConfig::~PageAtlasExportConfig()
{
	if (color) color->dec_count();
}

Value* PageAtlasExportConfig::duplicate()
{
	PageAtlasExportConfig *dup = new PageAtlasExportConfig(this);
	return dup;
}

Value *PageAtlasExportConfig::dereference(const char *extstring, int len)
{
	if (IsName("pages_wide",extstring,len)) {
		return new IntValue(pages_wide);

	} else if (IsName("pages_tall",extstring,len)) {
		return new IntValue(pages_tall);

	} else if (IsName("px_width",extstring,len)) {
		return new IntValue(px_width);

	} else if (IsName("px_height",extstring,len)) {
		return new IntValue(px_height);

	} else if (IsName("round_up_to_power_2",extstring,len)) {
		return new BooleanValue(round_up_to_power_2);

	} else if (IsName("color",extstring,len)) {
		if (!color) return new ColorValue();
	 	return new ColorValue(color->color.Red(), color->color.Green(), color->color.Blue(), color->color.Alpha());

	}
	return DocumentExportConfig::dereference(extstring,len);
}

//! Returns a new PageAtlasExportConfig.
Value *newPageAtlasExportConfig()
{
	PageAtlasExportConfig *d = new PageAtlasExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"PageAtlas"))
			d->filter = laidout->exportfilters.e[c];
	}
	return d;
}

int createPageAtlasExportConfig(ValueHash *context,ValueHash *parameters,Value **value_ret,ErrorLog &log)
{
	PageAtlasExportConfig *config = new PageAtlasExportConfig;

	ValueHash *pp = parameters;
	if (pp == nullptr) pp = new ValueHash;

	pp->push("exportconfig",config);

	Value *v = nullptr;
	int status = createExportConfig(context,pp, &v, log);
	if (status == 0 && v && v->type()==VALUE_Object) config = dynamic_cast<PageAtlasExportConfig *>(((ObjectValue *)v)->object);
	if (status != 0) {
		config->dec_count();
		if (pp && pp != parameters) delete pp;
		log.AddMessage(_("Error initializing PageAtlaseExportConfig"), ERROR_Fail);
		return 1;
	}

	 //assign proper base filter
	if (config) for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"PageAtlas")) {
			config->filter = laidout->exportfilters.e[c];
			break;
		}
	}

	int err = 0;
	FieldExtPlace ext;
	for (int c=0; c<pp->n(); c++) {
		ext.push(pp->key(c));
		int status = config->assign(&ext, pp->value(c));
		ext.remove(0);
		if (status <= 0) {
			err=1;
			log.AddMessage(_("Error creating PageAtlaseExportConfig"), ERROR_Fail);
		}
	}
	
	if (value_ret && err == 0) {
		if (config) {
			*value_ret = config;
			config->inc_count();
		} else *value_ret = nullptr;
	}

	if (config) config->dec_count();
	if (pp != parameters) delete pp;

	return 0;
}


int PageAtlasExportConfig::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
        const char *str=ext->e(0);
        int isnum;
        double d;
        if (str) {
            if (!strcmp(str,"round_up_to_power_2")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                round_up_to_power_2 = (d==0 ? false : true);
                return 1;

			} else if (!strcmp(str,"pages_wide")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                if (d <= 0) return 0;
                pages_wide = (int)d;
                return 1;

			} else if (!strcmp(str,"pages_tall")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                if (d <= 0) return 0;
                pages_tall = (int)d;
                return 1;
			
			} else if (!strcmp(str,"px_width")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                if (d <= 0) return 0;
                px_width = (int)d;
                return 1;

            } else if (!strcmp(str,"px_height")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                if (d <= 0) return 0;
                px_height = (int)d;
                return 1;

            } else if (!strcmp(str,"color")) {
            	ColorValue *cv = dynamic_cast<ColorValue*>(v);
            	if (!cv) return 0;
            	if (color) color->dec_count();
				color = dynamic_cast<ColorValue*>(cv->duplicate());
                return 1;
            }
		}
	}

	return DocumentExportConfig::assign(ext,v);
}

ObjectDef* PageAtlasExportConfig::makeObjectDef()
{
	ObjectDef *def = stylemanager.FindDef("PageAtlasExportConfig");
	if (def) {
		def->inc_count();
		return def;
	}

	ObjectDef *exportdef = stylemanager.FindDef("ExportConfig");
	if (!exportdef) {
        exportdef = makeExportConfigDef();
		stylemanager.AddObjectDef(exportdef,1);
    }

 	def = new ObjectDef(exportdef,"PageAtlasExportConfig",
            _("PageAtlas Export Configuration"),
            _("Configuration to export a document to a PageAtlas file."),
            "class",
            nullptr,nullptr,
            nullptr,
            0, //new flags
            newPageAtlasExportConfig,
            createPageAtlasExportConfig);

     //define parameters
    def->push("pages_wide",
            _("Pages wide"),
            _("Number of pages across to fit in single image"),
            "int",
            nullptr,    //range
            "1", //defvalue
            0,      //flags
            nullptr); //newfunc

    def->push("pages_tall",
            _("Pages tall"),
            _("Number of pages to fit vertically in single image"),
            "int",
            nullptr,    //range
            "1", //defvalue
            0,      //flags
            nullptr); //newfunc
 
    def->push("px_width",
            _("Image pixel width"),
            _("Image pixel width"),
            "int",
            nullptr,    //range
            "256", //defvalue
            0,     //flags
            nullptr); //newfunc
 
    def->push("px_height",
            _("Image pixel height"),
            _("Image pixel height"),
            "int",
            nullptr,    //range
            "256", //defvalue
            0,      //flags
            nullptr); //newfunc
 
    def->push("round_up_to_power_2",
            _("Round up to power of 2"),
            _("Whatever is in width and height, round up, for texture use in 3d software"),
            "boolean",
            nullptr,   //range
            "true", //defvalue
            0,     //flags
            nullptr);//newfunc

    def->push("color",
            _("Background color"),
            _("Background color"),
            "Color",
            nullptr,   //range
            nullptr, //defvalue
            0,     //flags
            nullptr);//newfunc
 
	stylemanager.AddObjectDef(def,0);
	return def;
}

//void PageAtlasExportConfig::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
Laxkit::Attribute *PageAtlasExportConfig::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	att = DocumentExportConfig::dump_out_atts(att, what, context);

	if (what == -1) {
		att->push("pages_wide", "1",             "whether to output meshes as svg2 meshes");
		att->push("pages_tall", "1",             "whether to use Inkscape's powerstroke LPE with paths where appropriate");
		att->push("px_width", "256",             "Pixels per inch. Usually 96 (css's value) is a safe bet.");
		att->push("px_height", "256",            "Whether to convert object metadata to data-* attributes.");
		att->push("round_up_to_power_2", "true", "Whether to convert object metadata to data-* attributes.");
		att->push("color", "rgba(1,1,1,0)",      "Background color");
		return att;
	}

	att->push("pages_wide",          pages_wide         );
	att->push("pages_tall",          pages_tall         );
	att->push("px_width",            px_width           );
	att->push("px_height",           px_height          );
	att->push("round_up_to_power_2", round_up_to_power_2);

	if (color) {
		char buffer[50];
		color->getValueStr(buffer, 50);
		att->push("color", buffer);
	} 

	return att;
}

void PageAtlasExportConfig::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DocumentExportConfig::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"round_up_to_power_2")) {
			round_up_to_power_2 = BooleanAttribute(value);

		} else if (!strcmp(name,"pages_wide")) {
			double d=0;
			DoubleAttribute(value, &d);
			if (d > 0) pages_wide = d;

		} else if (!strcmp(name,"pages_tall")) {
			double d=0;
			DoubleAttribute(value, &d);
			if (d > 0) pages_tall = d;

		} else if (!strcmp(name,"px_width")) {
			double d=0;
			DoubleAttribute(value, &d);
			if (d > 0) px_width = d;

		} else if (!strcmp(name,"px_height")) {
			double d=0;
			DoubleAttribute(value, &d);
			if (d > 0) px_height = d;

		} else if (!strcmp(name,"color")) {
			if (!isblank(value)) {
				if (!color) color = new ColorValue;
				color->Parse(value);
			}
		}
	}
}



//------------------------------------ PageAtlasImportConfig ----------------------------------

// //! For now, just returns a new ImportConfig.
// Value *newPageAtlasImportConfig()
// {
// 	ImportConfig *d=new ImportConfig;
// 	for (int c=0; c<laidout->importfilters.n; c++) {
// 		if (!strcmp(laidout->importfilters.e[c]->Format(),"PageAtlas"))
// 			d->filter=laidout->importfilters.e[c];
// 	}
// 	ObjectValue *v=new ObjectValue(d);
// 	d->dec_count();
// 	return v;
// }

// //! For now, just returns createImportConfig(), with filter forced to PageAtlas.
// int createPageAtlasImportConfig(ValueHash *context,ValueHash *parameters,Value **v_ret,ErrorLog &log)
// {
// 	ImportConfig *d=NULL;
// 	Value *v=NULL;
// 	int status=createImportConfig(context,parameters,&v,log);
// 	if (status==0 && v && v->type()==VALUE_Object) d=dynamic_cast<ImportConfig *>(((ObjectValue *)v)->object);

// 	if (d) for (int c=0; c<laidout->importfilters.n; c++) {
// 		if (!strcmp(laidout->importfilters.e[c]->Format(),"PageAtlas")) {
// 			d->filter=laidout->importfilters.e[c];
// 			break;
// 		}
// 	}
// 	*v_ret=v;

// 	return 0;
// }


//---------------------------- PageAtlasExportFilter --------------------------------

/*! \class PageAtlasExportFilter
 * \brief Export pages to a series of images arranged with n-up.
 *
 * These are meant to aid importing books to 3-d software.
 */

PageAtlasExportFilter::PageAtlasExportFilter()
{
	// flags = FILTER_MULTIPAGE; <- this is an import flag
	flags = FILTER_MANY_FILES;
}

//! "PageAtlas 1.4.5".
const char *PageAtlasExportFilter::VersionName()
{
	return _("Page Atlas");
}

const char *PageAtlasExportFilter::Version()
{
	return "1.0"; //the max version handled. import+export acts more like a pass through
}

DocumentExportConfig *PageAtlasExportFilter::CreateConfig(DocumentExportConfig *fromconfig)
{
	PageAtlasExportConfig* conf = new PageAtlasExportConfig(fromconfig);
	conf->filter = this;
	return conf;
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *PageAtlasExportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef = stylemanager.FindDef("PageAtlasExportConfig");
	if (styledef) return styledef;

	PageAtlasExportConfig config;
	styledef = config.GetObjectDef();

	return styledef;
}


//! Export the document as a PageAtlas file.
int PageAtlasExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DBG cerr <<"-----PageAtlas export start-------"<<endl;

	PageAtlasExportConfig *config = dynamic_cast<PageAtlasExportConfig *>(context);
	if (!config) {
		log.AddError(_("Missing proper config!"));
		return 1;
	}

	Document *  doc        = config->doc;
	IndexRange *range      = &config->range;
	int         layout     = config->layout;
	// Group *     limbo      = config->limbo;
	bool        rev        = config->reverse_order;
	PaperGroup *papergroup = config->papergroup;
	if (!filename) filename = config->filename;

	// we must have something to export...
	if (!doc) {
		log.AddError(_("Page atlas requires a document!"));
		return 1;
	}

	 //we must be able to open the export file location...
	Utf8String basename;
	Utf8String file;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 2;
		}
		basename = doc->saveas;
	} else basename = filename;
	char *bname = newstr(basename.c_str());
	chop_extension(bname);
	basename = bname;
	delete[] bname;


	int px_width = config->px_width;
	int px_height = config->px_height;

	if (config->round_up_to_power_2) {
		px_width = pow(2, ceil(log10(px_width)/log10(2)));
		px_height = pow(2, ceil(log10(px_height)/log10(2)));
		DBG cerr << " Round up to power of two: "<<px_width<<" x "<<px_height<<endl;
	}

	if (px_width <= 0 || px_height <= 0) {
		log.AddError(_("Width and height must be positive"));
		return 2;
	}

	// if (layout != SINGLELAYOUT) {
	// 	log.AddWarning(_("Using Singles layout. Page Atlas only supports Singles layout."));
	// 	layout = SINGLELAYOUT;
	// }

	if (layout != PAPERLAYOUT) {
		papergroup = nullptr; //force using spread bounds
	}

	int num_subs_per_image = config->pages_wide * config->pages_tall;
	double sub_width  = (double)px_width  / config->pages_wide;
	double sub_height = (double)px_height / config->pages_tall;
	LaxImage *wholeimg = ImageLoader::NewImage(px_width, px_height);
	LaxImage *subimg   = ImageLoader::NewImage(sub_width, sub_height);

	int totalnumpages = 0;

	if (config->evenodd == DocumentExportConfig::Even) {
		for (int c = range->Start(); c >= 0; c = range->Next())
			if (c % 2 == 0) totalnumpages++;
	} else if (config->evenodd == DocumentExportConfig::Odd) {
		for (int c = range->Start(); c >= 0; c = range->Next())
			if (c % 2 == 1) totalnumpages++;
	} else {
 		totalnumpages = (range->NumInRanges());
	}

	if (papergroup) totalnumpages *= papergroup->papers.n;

	int subs_on_image = 0; //num on whole image so far
	int img_num = 1;
	int img_start_num = 1;

	// set up Displayer
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	Displayer *dpw = imanager->GetDisplayer(DRAWS_Hires);
	DoubleBBox bounds;

	dpw->MakeCurrent(wholeimg);
	dpw->PushAxes();
	// dpw->defaultRighthanded(true);
	// dpw->NewTransform(1,0,0,-1,0,-height);
	if (config->color) {
		ScreenColor col(config->color->color.Red(), config->color->color.Green(), config->color->color.Blue(), config->color->color.Alpha());
		dpw->NewBG(col);
		dpw->ClearWindow();
	} else dpw->ClearTransparent();

	Displayer *dp = imanager->GetDisplayer(DRAWS_Hires); //for pages

	int numpages = range->NumInRanges();
	int fmt_wide = 1+log10(numpages);
	if (fmt_wide < 1) fmt_wide = 1;
	Utf8String fname_fmt;
	const char *img_fmt = "png";
	fname_fmt.Sprintf("%%s-%%0%dd-%%0%dd.%s", fmt_wide, fmt_wide, img_fmt);

	for (int c = (rev ? range->End() : range->Start());
		 c >= 0;
		 c = (rev ? range->Previous() : range->Next())) 
	{ //for each spread
		if (config->evenodd == DocumentExportConfig::Even && c % 2 == 0) continue;
		if (config->evenodd == DocumentExportConfig::Odd  && c % 2 == 1) continue;

		Spread *spread = doc->imposition->Layout(layout, c);


		for (int p = 0; p<(papergroup ? papergroup->papers.n : 1); p++) { //for each paper

			if (papergroup) papergroup->FindPaperBBox(&bounds);
			else {
				bounds.setbounds(spread->path);
			}

			dp->MakeCurrent(subimg);
			dp->SetSpace(bounds.minx,bounds.maxx, bounds.miny,bounds.maxy);
			dp->Center(bounds.minx,bounds.maxx, bounds.miny,bounds.maxy);
			// if (config->color) {
			// 	ScreenColor col(config->color->color.Red(), config->color->color.Green(), config->color->color.Blue(), config->color->color.Alpha());
			// 	dp->NewBG(col);
			// 	dp->ClearWindow();
			// } else dp->ClearTransparent();
			dp->ClearTransparent();
					
			if (papergroup && papergroup->objs.n()) {
				// .... output papergroup objects
				DrawData(dp, &papergroup->objs);
			}

			//if (limbo && limbo->n()) {
			//	appendobjfordumping(config, pageobjects,palette,limbo);
			//}

			if (spread) {
				//spread->GetBounds(bounds);

				if (spread->marks) {
					//objects created by the imposition
					DrawData(dp, spread->marks);
				}

				// for each page in spread layout..
				for (int c2 = 0; c2 < spread->pagestack.n(); c2++) {
					int pg = spread->pagestack.e[c2]->index;
					if (pg < 0 || pg >= doc->pages.n) continue;

					dp->PushAndNewTransform(spread->pagestack.e[c2]->outline->m());

					// for each layer on the page..
					for (int l = 0; l < doc->pages[pg]->layers.n(); l++) {
						// for each object in layer
						Group *g = dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (int c3 = 0; c3 < g->n(); c3++) {
							DrawData(dp, g->e(c3));
						}
					}
					dp->PopAxes();
				}

				// write subimg to proper place on wholeimg
				int x = (subs_on_image % config->pages_wide) * sub_width; //float cuts down on placement rounding errors
				int y = int(subs_on_image / config->pages_wide) * sub_height;
				dpw->imageout(subimg, x,y);

			} //if (spread)

			subs_on_image++;
			if (subs_on_image == num_subs_per_image || img_num == totalnumpages) {
				//save image
				// file.Sprintf("%s-%d-%d.png", basename.c_str(), img_start_num, img_num);
				file.Sprintf(fname_fmt.c_str(), basename.c_str(), img_start_num, img_num);
				wholeimg->Save(file.c_str());

				subs_on_image = 0;
				img_start_num = img_num+1;

				if (img_num != totalnumpages) {
					if (config->color) {
						ScreenColor col(config->color->color.Red(), config->color->color.Green(), config->color->color.Blue(), config->color->color.Alpha());
						dpw->NewBG(col);
						dpw->ClearWindow();
					} else dpw->ClearTransparent();
				}
			}
			img_num++;
		} //for each paper
		if (spread) { delete spread; spread = nullptr; }
	} //for each spread

	
	setlocale(LC_ALL,"");

	//clean up
	wholeimg->dec_count();
	subimg->dec_count();

	DBG cerr <<"-----PageAtlas export end success-------"<<endl;

	return 0;
}





// //------------------------------------ PageAtlasImportFilter ----------------------------------
// /*! \class PageAtlasImportFilter
//  * \brief Filter to, amazingly enough, import svg files.
//  *
//  * \todo perhaps when dumping in to an object, just bring in all objects as they are arranged
//  *   in the scratch space... or if no doc or obj, option to import that way, but with
//  *   a custom papergroup where the pages are... perhaps need a freestyle imposition type
//  *   that is more flexible with differently sized pages.
//  */


// const char *PageAtlasImportFilter::VersionName()
// {
// 	return _("PageAtlas");
// }

// /*! \todo can do more work here to get the actual version of the file...
//  */
// const char *PageAtlasImportFilter::FileType(const char *first100bytes)
// {
// 	*** ok if pingable image file
// 	if (!strstr(first100bytes,"<PageAtlasUTF8NEW")) return NULL;
// 	return "1.0";

// }

// //! Try to grab from stylemanager, and install a new one there if not found.
// /*! The returned def need not be dec_counted.
//  */
// ObjectDef *PageAtlasImportFilter::GetObjectDef()
// {
// 	ObjectDef *styledef;
// 	styledef = stylemanager.FindDef("PageAtlasImportConfig");
// 	if (styledef) return styledef; 

// 	styledef = makeObjectDef();
// 	makestr(styledef->name,"PageAtlasImportConfig");
// 	makestr(styledef->Name,_("PageAtlas Import Configuration"));
// 	makestr(styledef->description,_("Configuration to import a PageAtlas file."));
// 	styledef->newfunc=newPageAtlasImportConfig;
// 	styledef->stylefunc=createPageAtlasImportConfig;

// 	stylemanager.AddObjectDef(styledef,0);
// 	styledef->dec_count();

// 	return styledef;
// }

// //! Import from PageAtlas files.
// /*! If in->doc==NULL and in->toobj==NULL, then create a new document.
//  */
// int PageAtlasImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log, const char *filecontents,int contentslen)
// {
// 	DBG cerr <<"-----PageAtlas import start-------"<<endl;

// 	PageAtlasImportConfig *in=dynamic_cast<PageAtlasImportConfig *>(context);
// 	if (!in) {
// 		log.AddMessage(_("Missing PageAtlasImportConfig!"),ERROR_Fail);
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

// 	if (doc && docpagenum+(end-start)>=doc->pages.n) //create enough pages to hold the PageAtlas pages
// 		doc->NewPages(-1,(docpagenum+(end-start+1))-doc->pages.n);



// 	 //if doc is new, push into the project
// 	if (doc && doc != in->doc) {
// 		laidout->project->Push(doc);
// 		laidout->app->addwindow(newHeadWindow(doc));
// 		doc->dec_count();
// 	}
	
// 	DBG cerr <<"-----PageAtlas import end successfully-------"<<endl;
// 	delete att;
// 	return 0;

// }


} // namespace Laidout

