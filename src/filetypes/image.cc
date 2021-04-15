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
// Copyright (C) 2016 by Tom Lechner
//


#include <cups/cups.h>
#include <sys/wait.h>

#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>

#include "../language.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../printing/psout.h"
#include "image.h"
#include "../impositions/singles.h"
#include "../core/drawdata.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {



//--------------------------------- install Image filter

//! Tells the Laidout application that there's a new filter in town.
void installImageFilter()
{
	ImageExportFilter *imageout=new ImageExportFilter;
	imageout->GetObjectDef();
	laidout->PushExportFilter(imageout);
}


//----------------------------- ImageExportConfig -----------------------------
/*! \class ImageExportConfig
 * \brief Holds extra config for image export.
 *
 * Extras include image format, whether to use a transparent background, pixel width and height.
 */


//! Set the filter to the Image export filter stored in the laidout object.
ImageExportConfig::ImageExportConfig()
{
	format = newstr("png");
	background = ColorManager::newColor(LAX_COLOR_RGB, 4, 1.,1.,1.,1.);
	use_transparent_bg = true;
	width = height = 0;

	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Image")) {
			filter = laidout->exportfilters.e[c];
			break; 
		}
	}
}

ImageExportConfig::ImageExportConfig(DocumentExportConfig *config)
  : DocumentExportConfig(config)
{
	background = nullptr;

	ImageExportConfig *conf = dynamic_cast<ImageExportConfig*>(config);
	if (conf) {
		format = newstr(conf->format);
		use_transparent_bg = conf->use_transparent_bg;
		width  = conf->width;
		height = conf->height;
		if (conf->background) background = conf->background->duplicate();

	} else {
		format = newstr("png");
		use_transparent_bg = true;
		width = height = 0;
		background = ColorManager::newColor(LAX_COLOR_RGB, 4, 1.,1.,1.,1.);;
	}
}

Value* ImageExportConfig::duplicate()
{
	ImageExportConfig *dup = new ImageExportConfig(this);
	return dup;
}


ImageExportConfig::~ImageExportConfig()
{
	if (background) background->dec_count();
	delete[] format;
}

void ImageExportConfig::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

LaxFiles::Attribute *ImageExportConfig::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att = DocumentExportConfig::dump_out_atts(att,what,context);

	if (what == -1) {
		att->push("transparent", nullptr,      "Use a transparent background, not a rendered color.");
		att->push("background", "rgbf(1,1,1)", "The color to use as background when not explicitly transparent.");
		att->push("format",     "png",         "File format to use. Default is png");
		att->push("width",      "0",           "Pixel width of resulting image. 0 means auto calculate from dpi.");
		att->push("height",     "0",           "Pixel height of resulting image. 0 means auto calculate from dpi.");
		return att;
	}

	att->push("transparent", use_transparent_bg ? "yes" : "no");
	if (background) {
		char *str = background->dump_out_simple_string();
		att->push("color", str);
		delete[] str;
	}
	att->push("format", format);
	att->push("width", width);
	att->push("height", height);
	return att;
}

void ImageExportConfig::dump_in_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	DocumentExportConfig::dump_in_atts(att,what,context);

	char *value, *name;
	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name, "transparent")) {
			use_transparent_bg = BooleanAttribute(value);

		} else if (!strcmp(name, "format")) {
			makestr(format, value);

		} else if (!strcmp(name, "width")) {
			IntAttribute(value, &width, NULL);

		} else if (!strcmp(name, "height")) {
			IntAttribute(value, &height, NULL);

		} else if (!strcmp(name, "color")) { //overrides paper color?
			Color *ncolor = ColorManager::newColor(att->attributes.e[c]);
			if (ncolor) {
				if (background) background->dec_count();
				background = ncolor;
			}

		}
	}
}

Value *NewImageExportConfig() { return new ImageExportConfig(); }

ObjectDef *ImageExportConfig::makeObjectDef()
{
    ObjectDef *def=stylemanager.FindDef("ImageExportConfig");
	if (def) {
		def->inc_count();
		return def;
	}

    ObjectDef *exportdef=stylemanager.FindDef("ExportConfig");
    if (!exportdef) {
        exportdef=makeExportConfigDef();
		stylemanager.AddObjectDef(exportdef,1);
		exportdef->inc_count(); // *** this saves a crash, but why?!?!?
    }

 
	def=new ObjectDef(exportdef,"ImageExportConfig",
            _("Image Export Configuration"),
            _("Settings to export a document to image files."),
            "class",
            NULL,NULL,
            NULL,
            0, //new flags
            NewImageExportConfig, //newfunc
            NULL);


     //define parameters
    def->push("transparent",
            _("Transparent"),
            _("Render background as transparent, not as paper color."),
            "boolean",
            NULL,    //range
            "true", //defvalue
            0,      //flags
            NULL); //newfunc

    def->push("color",
            _("Color"),
            _("The color to use as background when not explicitly transparent."),
            "Color",
            NULL,    //range
            "rgbf(1,1,1)", //defvalue
            0,      //flags
            NULL); //newfunc

    def->push("format",
            _("File format"),
            _("What file format to export as."),
            "string",
            NULL,   //range
            "png", //defvalue
            0,     //flags
            NULL);//newfunc
 
    def->push("width",
            _("Width"),
            _("Pixel width of resulting image. 0 means autocalculate"),
            "int",
            NULL,   //range
            "0", //defvalue
            0,     //flags
            NULL);//newfunc

    def->push("height",
            _("Height"),
            _("Pixel height of resulting image. 0 means autocalculate"),
            "int",
            NULL,   //range
            "0", //defvalue
            0,     //flags
            NULL);//newfunc

	stylemanager.AddObjectDef(def,0);
	return def;
}

Value *ImageExportConfig::dereference(const char *extstring, int len)
{
	if (len<0) len=strlen(extstring);

	if (!strncmp(extstring,"transparent",8)) {
		return new BooleanValue(use_transparent_bg);

	} else if (!strncmp(extstring,"format",8)) {
		return new StringValue(format);

	} else if (!strncmp(extstring,"width",8)) {
		return new IntValue(width);

	} else if (!strncmp(extstring,"height",8)) {
		return new IntValue(height);

	} else if (!strncmp(extstring,"color",8)) {
		return new ColorValue(*background);

	}
	return DocumentExportConfig::dereference(extstring,len);
}

int ImageExportConfig::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
        const char *str=ext->e(0);
        int isnum;
        double d;
        if (str) {
            if (!strcmp(str,"transparent")) {
                d=getNumberValue(v, &isnum);
                if (!isnum) return 0;
                use_transparent_bg = (d==0 ? false : true);
                return 1;

			} else if (!strcmp(str,"format")) {
				StringValue *str=dynamic_cast<StringValue*>(v);
				if (!str) return 0;
				makestr(format, str->str);
                return 1;

			} else if (!strcmp(str,"width")) {
				int w=getNumberValue(v, &isnum);
                if (!isnum) return 0;
				width=w;
                return 1;

			} else if (!strcmp(str,"height")) {
				int h=getNumberValue(v, &isnum);
                if (!isnum) return 0;
				height=h;
                return 1;

            } else if (!strcmp(str,"color")) {
            	ColorValue *col = dynamic_cast<ColorValue*>(v);
            	if (!col) return 0;
            	if (background) background->dec_count();
            	background = new Color;
            	col->GetColor(background);
            	return 1;
			}
		}
	}

	return DocumentExportConfig::assign(ext,v);
}


//------------------------------------ ImageExportFilter ----------------------------------
	
/*! \class ImageExportFilter
 * \brief Filter for exporting pages to images via a Displayer.
 *
 * Uses ImageExportConfig.
 */


ImageExportFilter::ImageExportFilter()
{
	//flags=FILTER_MANY_FILES;
}

/*! \todo if other image formats get implemented, then this would return
 *    the proper extension for that image type
 */
const char *ImageExportFilter::DefaultExtension()
{
	return "png";
}

const char *ImageExportFilter::VersionName()
{
	return _("Image");
}


Value *newImageExportConfig()
{
	ImageExportConfig *o=new ImageExportConfig;
	ObjectValue *v=new ObjectValue(o);
	o->dec_count();
	return v;
}

/*! \todo *** this needs a going over for safety's sake!! track down ref counting
 */
int createImageExportConfig(ValueHash *context,ValueHash *parameters,Value **value_ret,ErrorLog &log)
{
	ImageExportConfig *config=new ImageExportConfig;

	ValueHash *pp=parameters;
	if (pp==NULL) pp=new ValueHash;

	Value *vv=NULL, *v=NULL;
	vv=new ObjectValue(config);
	pp->push("exportconfig",vv);

	int status=createExportConfig(context,pp, &v, log);
	if (status==0 && v && v->type()==VALUE_Object) config=dynamic_cast<ImageExportConfig *>(((ObjectValue *)v)->object);

	 //assign proper base filter
	if (config) for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Image")) {
			config->filter=laidout->exportfilters.e[c];
			break;
		}
	}

	char error[100];
	int err=0;
	try {
		int i, e;

		 //---format
		const char *str=parameters->findString("format",-1,&e);
		if (e==0) { if (str) makestr(config->format, str); }
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"format"); throw error; }

		 //---use_transparent_bg
		i=parameters->findInt("transparent",-1,&e);
		if (e==0) config->use_transparent_bg=i;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"transparent"); throw error; }

		 //---width
		double d=parameters->findIntOrDouble("width",-1,&e);
		if (e==0) config->width=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"width"); throw error; }

		 //---height
		d = parameters->findIntOrDouble("height",-1,&e);
		if (e==0) config->height=d;
		else if (e==2) { sprintf(error, _("Invalid format for %s!"),"height"); throw error; }

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


	if (pp!=parameters) delete pp;

	return 0;
}

DocumentExportConfig *ImageExportFilter::CreateConfig(DocumentExportConfig *fromconfig)
{
	return new ImageExportConfig(fromconfig);
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 *
 * \todo implement background color. currently defaults to transparent
 */
ObjectDef *ImageExportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("ImageExportConfig");
	if (styledef) return styledef; 

	ImageExportConfig config;
	styledef = config.GetObjectDef();

	return styledef;
}




/*! Save the document as image files with optional transparency.
 * 
 * Return 0 for success, or nonzero error.
 * 
 * Currently uses an ImageExportConfig.
 */
int ImageExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	ImageExportConfig *out=dynamic_cast<ImageExportConfig *>(context);
	if (!out) {
		log.AddMessage(_("Wrong type of output config!"),ERROR_Fail);
		return 1;
	}

	Document *doc =out->doc;
	//int start     =out->start;
	//int end       =out->end;
	//int layout    =out->layout;
	if (!filename) filename=out->filename;
	
	 //we must have something to export...
	if (!doc && !out->limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}
	
	char *file=NULL;
	if (!filename) {
		if (!doc || isblank(doc->saveas)) {
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 3;
		}
		file=newstr(doc->saveas);
		appendstr(file,".");
		if (isblank(out->format)) appendstr(file,"png");
		else appendstr(file,out->format);

		filename = file;
	}
	

	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	Displayer *dp = imanager->GetDisplayer(DRAWS_Hires);
	DoubleBBox bounds;

	int width  = out->width;
	int height = out->height;

	if (out->crop.validbounds()) {
		 //use crop to override any bounds derived from PaperGroup
		bounds.setbounds(&out->crop);

	} else	if (out->papergroup) {
		out->papergroup->FindPaperBBox(&bounds);
	}

	if (!bounds.validbounds() && out->limbo) {
		bounds.setbounds(out->limbo);
	}

	if (!bounds.validbounds()) {
		log.AddMessage(_("Invalid output bounds! Either set crop or papergroup."),ERROR_Fail);
		delete[] file;
		return 4;
	}

	//now bounds is the real bounds on a papergroup/view space.
	//We need to figure out how big an image to use
	if (width>0 && out->height>0) {
		//nothing to do, dimensions already specified

	} else if (width == 0 && out->height == 0) {
		 //both zero means use dpi to compute
		double dpi=300; // *** maybe make this an option??
		if (out->papergroup) dpi=out->papergroup->GetBasePaper(0)->dpi;
		width =bounds.boxwidth ()*dpi;
		height=bounds.boxheight()*dpi;

	} else if (width == 0) {
		 //compute based on height, which must be zonzero
		width = height/bounds.boxheight()*bounds.boxwidth();

	} else { //if (height == 0) {
		 //compute based on width, which must be zonzero
		height = width/bounds.boxwidth()*bounds.boxheight();

	}

	if (width==0 || height==0) {
		if (width==0)  log.AddMessage(_( "Null width, nothing to output!"),ERROR_Fail);
		if (height==0) log.AddMessage(_("Null height, nothing to output!"),ERROR_Fail);
		delete[] file;
		return 5;
	}

	 //set displayer port to the proper area.
	 //The area in bounds must map to [0..width, 0..height]
	dp->CreateSurface(width, height);
	dp->PushAxes();
	dp->defaultRighthanded(true);
	dp->NewTransform(1,0,0,-1,0,-height);

	//dp->SetSpace(0,width, 0,height);
	dp->SetSpace(bounds.minx,bounds.maxx, bounds.miny,bounds.maxy);
	dp->Center(bounds.minx,bounds.maxx, bounds.miny,bounds.maxy);
	//dp->defaultRighthanded(false);

	 //now output everything
	if (!out->use_transparent_bg) {
		 //fill output with an appropriate background color
		 // *** this should really color papers according to their characteristics
		 // *** and have a default for non-transparent limbo color
		if (out->papergroup && out->papergroup->papers.n) {
			dp->NewBG(&out->papergroup->papers.e[0]->color);
		} else dp->NewBG(1.0, 1.0, 1.0);

		dp->ClearWindow();
	}

	//DBG dp->NewFG(.5,.5,.5);
	//DBG dp->drawline(bounds.minx,bounds.miny, bounds.maxx,bounds.maxy);
	//DBG dp->drawline(bounds.minx,bounds.maxy, bounds.maxx,bounds.miny);

	 //limbo objects
	if (out->limbo) DrawData(dp, out->limbo, NULL,NULL,DRAW_HIRES);
	//if (out->limbo) imanager->DrawData(dp, out->limbo, NULL,NULL,DRAW_HIRES);
	
	 //papergroup objects
	if (out->papergroup && out->papergroup->objs.n()) {
		for (int c=0; c<out->papergroup->objs.n(); c++) {
			  //imanager->DrawData(dp, out->papergroup->objs.e(c), NULL,NULL,DRAW_HIRES);
			  DrawData(dp, out->papergroup->objs.e(c), NULL,NULL,DRAW_HIRES);
		}
	}

	 //spread objects
	Spread *spread = NULL;
	if (doc) {
		spread = doc->imposition->Layout(out->layout, out->range.Start());
	}

	if (spread) {
		dp->BlendMode(LAXOP_Over);

		 // draw the page's objects and margins
		Page *page=NULL;
		int pagei=-1;
		flatpoint p;
		SomeData *sd=NULL;

		for (int c=0; c<spread->pagestack.n(); c++) {
			DBG cerr <<" drawing from pagestack.e["<<c<<"], which has page "<<spread->pagestack.e[c]->index<<endl;
			page=spread->pagestack.e[c]->page;
			pagei=spread->pagestack.e[c]->index;

			if (!page) { // try to look up page in doc using pagestack->index
				if (spread->pagestack.e[c]->index>=0 && spread->pagestack.e[c]->index<doc->pages.n) {
					page = spread->pagestack.e[c]->page = doc->pages.e[pagei];
				}
			}

			if (!page) continue;

			 //else we have a page, so draw it all
			sd=spread->pagestack.e[c]->outline;
			dp->PushAndNewTransform(sd->m()); // transform to page coords
			

			if ((page->pagestyle->flags&PAGE_CLIPS) || out->layout == PAPERLAYOUT) {
				 // setup clipping region to be the page
				dp->PushClip(1);
				//SetClipFromPaths(dp,sd,dp->Getctm());
				SetClipFromPaths(dp,sd,NULL);
			}

            if (page->pagebleeds.n && (out->layout == PAPERLAYOUT || out->layout == SINGLELAYOUT)) {
                 //assume PAGELAYOUT already renders bleeds properly, since that's where the bleed objects come from

                //if (out->layout == PAPERLAYOUT) {
                //    //only clip in paper view
                //    dp->PushClip(1);
                //    SetClipFromPaths(dp,sd,NULL);
                //}

                for (int pb=0; pb<page->pagebleeds.n; pb++) {
                    PageBleed *bleed = page->pagebleeds[pb];
                    Page *otherpage = doc->pages[bleed->index];

                    dp->PushAndNewTransform(bleed->matrix);

                    for (int c2 = 0; c2 < otherpage->layers.n(); c2++) {
                        DrawData(dp,otherpage->e(c2),NULL,NULL,0);
                    }

                    dp->PopAxes();
                }

                //if (out->layout == PAPERLAYOUT) dp->PopClip();
            }
			

			 //*** debuggging: draw X over whole page...
	//		DBG dp->NewFG(255,0,0);
	//		DBG dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->miny));
	//		DBG dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->maxx,sd->maxy));
	//		DBG dp->drawrline(flatpoint(sd->maxx,sd->maxy), flatpoint(sd->minx,sd->maxy));
	//		DBG dp->drawrline(flatpoint(sd->minx,sd->maxy), flatpoint(sd->minx,sd->miny));
	//		DBG dp->drawrline(flatpoint(sd->minx,sd->miny), flatpoint(sd->maxx,sd->maxy));
	//		DBG dp->drawrline(flatpoint(sd->maxx,sd->miny), flatpoint(sd->minx,sd->maxy));
	

			 // Draw all the page's objects.
			for (int c2=0; c2<page->layers.n(); c2++) {
				DBG cerr <<"  num layers in page: "<<page->n()<<", num objs:"<<page->e(c2)->n()<<endl;
				DBG cerr <<"  Layer "<<c2<<", objs.n="<<page->e(c2)->n()<<endl;
				//imanager->DrawData(dp, page->e(c2), NULL,NULL,DRAW_HIRES);
				DrawData(dp, page->e(c2),NULL,NULL,DRAW_HIRES);
			}
			
			if (page->pagestyle->flags&PAGE_CLIPS) {
				 //remove clipping region
				dp->PopClip();
			}

			dp->PopAxes(); // remove page transform
		} //foreach in pagestack
	} //if spread


	dp->PopAxes(); //initial dp protection


	 //Now save the page
	LaxImage *img = dp->GetSurface();
	int err = img->Save(filename, out->format);
	if (err) {
		log.AddMessage(_("Could not save the image"), ERROR_Fail);
	}
	img->dec_count();

	delete[] file;
	if (log.Errors()) return -1;

	return 0; 
}


} // namespace Laidout

