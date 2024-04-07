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
// Copyright (C) 2007-2009,2011-2012,2015 by Tom Lechner
//


#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/textonpathinterface.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/svgcoord.h>
#include <lax/interfaces/somedataref.h>
#include <lax/transformmath.h>
#include <lax/units.h>
#include <lax/attributes.h>
#include <lax/utf8string.h>
#include <lax/cssutils.h>

#include "../language.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../dataobjects/mysterydata.h"
#include "svg.h"
#include "../ui/headwindow.h"
#include "../impositions/singles.h"
#include "../core/utils.h"
#include "../core/drawdata.h"
#include "../core/guides.h"
#include "../impositions/netimposition.h"


#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;



namespace Laidout {


double DEFAULT_PPINCH = 96;
//double DEFAULT_PPINCH = 90;


//-------forward decs for helper funcs
static int StyleToFillAndStroke(const char *inlinecss, LaxInterfaces::PathsData *paths,
		RefPtrStack<anObject> &gradients, SomeData **fillobj_ret,
		ValueHash *extra = nullptr);



//------------------------ Svg in/reimpose/out helpers -------------------------------------

//forward declarations:
Imposition *ParseInkscapeMultipage(Attribute *namedview, double *viewbox, double scalex, double scaley);

//! Creates a Laidout Document from a Svg file, and adds to laidout->project.
/*! Assumes something has already checked that file is an SVG, so things like failure to
 * open, and munched up file data will produce fatal errors.
 *
 * Return 0 for success, or nonzero for error.
 *
 * If existingdoc!=nullptr, then insert the file to that Document. In this case, it is not
 * pushed onto the project, as it is assumed it is elsewhere. Note that this will
 * basically wipe the existing document, and replace with the Svg document.
 */
int AddSvgDocument(const char *file, Laxkit::ErrorLog &log, Document *existingdoc)
{
	FILE *f = fopen(file,"r");
	if (!f) {
		log.AddMessage(_("Could not open file!"),ERROR_Fail);
		return 1;
	}
	char chunk[2000];
	size_t c = fread(chunk,1,1999,f); //note this is not a guarantee of finding width/height/viewbox!!
	chunk[c] = '\0';
	fclose(f);

	if (!strstr(chunk, "<svg")) {
		log.AddError(_("File does not seem to be an SVG"));
		return 2;
	}

	SvgImportFilter filter; //todo: this should be grabbed from somewhere in case users replace default svg importer
	ImportConfig config(file,300, 0,-1, 0,-1,-1, existingdoc,nullptr);
	config.keepmystery = 0;
	config.filter = &filter;

	if (filter.In(file,&config,log, nullptr,0) > 0) return 1;

	return 0;
}
// int AddSvgDocumentOLD(const char *file, Laxkit::ErrorLog &log, Document *existingdoc)
// {
// 	FILE *f = fopen(file,"r");
// 	if (!f) {
// 		log.AddMessage(_("Could not open file!"),ERROR_Fail);
// 		return 1;
// 	}
// 	char chunk[2000];
// 	size_t c = fread(chunk,1,1999,f); //note this is not a guarantee of finding width/height/viewbox!!
// 	chunk[c] = '\0';
// 	fclose(f);

// 	 //find default page width and height
// 	double width,height = -100;
// 	double scalex = 1, scaley = 1;
// 	bool needtoscale = false;
// 	UnitManager *unitm = GetUnitManager();

// 	 //width
// 	char *endptr = nullptr;
// 	const char *ptr = strstr(chunk,"width");
// 	if (!ptr) return 2;
// 	ptr += 5;
// 	while (isspace(*ptr) || *ptr=='=') ptr++;
// 	if (*ptr != '\"') return 3;
// 	ptr++;
// 	width = strtod(ptr,&endptr);
// 	ptr = endptr;
// 	if (*ptr != '\"') {
// 		 //need to parse units
// 		while (isspace(*ptr)) ptr++;
// 		const char *eptr=ptr;
// 		while (isalpha(*eptr)) eptr++;
// 		int units = unitm->UnitId(ptr, eptr-ptr);
// 		if (units != UNITS_None) {
// 			width = unitm->Convert(width, units, UNITS_Inches, nullptr);
// 			needtoscale = false;
// 		}
// 	} else {
// 		width /= DEFAULT_PPINCH;
// 		scalex = 1./DEFAULT_PPINCH;
// 	}
// 	if (width <= 0) {
// 		log.AddError(_("Bad width value"));
// 		return 4;
// 	}

// 	 //height
// 	ptr = strstr(chunk,"height");
// 	if (ptr) {
// 		ptr += 6;
// 		while (isspace(*ptr) || *ptr=='=') ptr++;
// 		if (*ptr != '\"') return 6;
// 		ptr++;
// 		if (!strncmp(ptr, "auto", 4)) height = -100;
// 		else {
// 			height = strtod(ptr,&endptr);
// 			ptr = endptr;
// 			if (*ptr != '\"') {
// 				 //need to parse units
// 				while (isspace(*ptr)) ptr++;
// 				const char *eptr=ptr;
// 				while (isalpha(*eptr)) eptr++;
// 				int units = unitm->UnitId(ptr, eptr-ptr);
// 				if (units!=UNITS_None) {
// 					height = unitm->Convert(height, units, UNITS_Inches, nullptr);
// 					needtoscale = false;
// 				}
// 			} else {
// 				height /= DEFAULT_PPINCH;
// 				scaley = 1./DEFAULT_PPINCH;
// 			}
// 		}
// 	}

// 	ptr = strstr(chunk,"viewBox"); // viewBox="0 0 1530 1530", map this rectangle to 0,0 -> width,height
// 	double viewbox_aspect = 1;
// 	double viewbox[4];
// 	if (ptr) {
// 		ptr += 7;
// 		while (isspace(*ptr) || *ptr=='=' || *ptr=='"') ptr++;
// 		int n = DoubleListAttribute(ptr, viewbox, 4, nullptr);
// 		if (n == 4) {
// 			scalex = width  / viewbox[2];
// 			scaley = height / viewbox[3]; //***note auto height not found yet
// 			needtoscale = true;
// 			if (fabs(viewbox[2] - viewbox[0]) < 1e-8) {
// 				log.AddError(_("Bad viewbox!"));
// 				return 8;
// 			}
// 			viewbox_aspect = (viewbox[3] - viewbox[1]) / (viewbox[2] - viewbox[0]);
// 		}
// 	}

// 	if (height == -100) { // was auto
// 		height = width * viewbox_aspect;
// 	}
// 	if (height<=0) {
// 		log.AddError(_("Bad height value!"));
// 		return 7;
// 	}
// 	PaperStyle paper("custom",width,height, 0,300, "in");
	
// 	// parse Inkscape multipage if present, we need to find a vaguely reasonable paper size
// 	// when 
// 	Imposition *imp = nullptr;
// 	bool parsing_multipage = false;
// 	char *papersPtr = strstr(chunk, "<sodipodi:namedview");
// 	if (papersPtr) {
// 		Attribute *namedview = XMLChunkToAttribute(nullptr, papersPtr, 2000-(papersPtr - chunk), nullptr, nullptr, nullptr);
// 		if (namedview) {
// 			Imposition *parsed_imp = ParseInkscapeMultipage(namedview, viewbox, scalex, scaley);
// 			if (parsed_imp) {
// 				imp = parsed_imp;
// 				parsing_multipage = true;
// 			}
// 			delete namedview;
// 		}
// 	}

// 	if (!imp) {
// 		// no valid multipage found, default to ordinary Singles
// 		imp = new Singles;
// 		imp->SetPaperSize(&paper);
// 		imp->NumPages(1);
// 	}

// 	Document *newdoc = existingdoc;
// 	int num_pages = imp->NumPages();

// 	if (newdoc) {
// 		makestr(newdoc->saveas,nullptr); //force rename later
// 		if (newdoc->imposition) newdoc->imposition->dec_count();
// 		newdoc->imposition = imp;
// 		imp->inc_count();
// 	} else {
// 		newdoc = new Document(imp,nullptr);//null file name to force rename on save
// 	}

// 	if (num_pages > newdoc->NumPages()) {
// 		newdoc->NewPages(newdoc->NumPages(), num_pages - newdoc->NumPages());
// 	}

// 	makestr(newdoc->name,"From ");
// 	appendstr(newdoc->name,file);
// 	imp->dec_count();

// 	if (!existingdoc) laidout->project->Push(newdoc);
// 	else newdoc->inc_count();

// 	SvgImportFilter filter;
// 	ImportConfig config(file,300, 0,-1, 0,-1,-1, newdoc,nullptr);
// 	config.keepmystery = 0;
// 	config.filter = &filter;
// 	filter.In(file,&config,log, nullptr,0);

// 	 //scale down
// //	if (needtoscale && (scalex != 1 || scaley != 1)) {
// //		Group *group = dynamic_cast<Group*>(newdoc->pages.e[0]->layers.e(0));
// //		group->maxx = width;
// //		group->maxy = height;
// //		for (int c=0; c<group->n(); c++) {
// //			group->e(c)->Scale(flatpoint(0,0), scalex, scaley);
// //		}
// //	}

// 	newdoc->dec_count();
// 	return 0;
// }



//--------------------------------- install SVG filter

//! Tells the Laidout application that there's a new filter in town.
void installSvgFilter()
{
	SvgOutputFilter *svgout=new SvgOutputFilter;
	laidout->PushExportFilter(svgout);
	
	SvgImportFilter *svgin=new SvgImportFilter;
	laidout->PushImportFilter(svgin);
}


//------------------------------------ SvgExportConfig ----------------------------------

/*! \class SvgExportConfig
 * \brief Holds extra config for svg export.
 */


//! Set the filter to the Image export filter stored in the laidout object.
SvgExportConfig::SvgExportConfig()
{
	use_mesh        = false;
	data_meta       = false;
	use_powerstroke = false;
	pixels_per_inch = DEFAULT_PPINCH;

	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Image")) {
			filter=laidout->exportfilters.e[c];
			break;
		}
	}
}

/*! Base on config, copy over its stuff.
 */
SvgExportConfig::SvgExportConfig(DocumentExportConfig *config)
	: DocumentExportConfig(config)
{
	SvgExportConfig *svgconf=dynamic_cast<SvgExportConfig*>(config);
	if (svgconf) {
		use_mesh        = svgconf->use_mesh;
		use_powerstroke = svgconf->use_powerstroke;
		pixels_per_inch = svgconf->pixels_per_inch;
		data_meta       = svgconf->data_meta;

	} else {
		use_mesh        = false;
		use_powerstroke = false;
		pixels_per_inch = DEFAULT_PPINCH;
		data_meta       = false;
	}
}

Value* SvgExportConfig::duplicate()
{
	SvgExportConfig *dup = new SvgExportConfig(this);
	return dup;
}


Value *SvgExportConfig::dereference(const char *extstring, int len)
{
	if (!strncmp(extstring,"use_mesh",8)) {
		return new BooleanValue(use_mesh);

	} else if (!strncmp(extstring,"use_powerstroke",15)) {
		return new BooleanValue(use_powerstroke);

	} else if (!strncmp(extstring,"data_meta",9)) {
		return new BooleanValue(data_meta);

	} else if (!strncmp(extstring,"pixels_per_inch",15)) {
		return new DoubleValue(pixels_per_inch);

	}
	return DocumentExportConfig::dereference(extstring,len);
}

int SvgExportConfig::assign(FieldExtPlace *ext,Value *v)
{
	if (ext && ext->n()==1) {
        const char *str=ext->e(0);
        int isnum;
        double d;
        if (str) {
            if (!strcmp(str,"use_mesh")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                use_mesh=(d==0 ? false : true);
                return 1;

			} else if (!strcmp(str,"use_powerstroke")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                use_powerstroke=(d==0 ? false : true);
                return 1;

			} else if (!strcmp(str,"data_meta")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
                data_meta = (d==0 ? false : true);
                return 1;

			} else if (!strcmp(str,"pixels_per_inch")) {
                d = getNumberValue(v, &isnum);
                if (!isnum) return 0;
				if (d==0) return 0;
                pixels_per_inch = d;
                return 1;
			}
		}
	}

	return DocumentExportConfig::assign(ext,v);

}

ObjectDef* SvgExportConfig::makeObjectDef()
{
    ObjectDef *def=stylemanager.FindDef("SvgExportConfig");
	if (def) {
		def->inc_count();
		return def;
	}

    ObjectDef *exportdef=stylemanager.FindDef("ExportConfig");
    if (!exportdef) {
        exportdef = makeExportConfigDef();
		stylemanager.AddObjectDef(exportdef,1);
    }

 
	def = new ObjectDef(exportdef,"SvgExportConfig",
            _("Svg Export Configuration"),
            _("Settings for an SVG filter that exports a document."),
            "class",
            nullptr,nullptr,
            nullptr,
            0, //new flags
            nullptr,
            nullptr);


     //define parameters
    def->push("use_mesh",
            _("Use Mesh"),
            _("Export color meshes using Coons Patchs, as structured in SVG 2's draft of meshes. These are used in Inkscape."),
            "boolean",
            nullptr,    //range
            "false", //defvalue
            0,      //flags
            nullptr); //newfunc

    def->push("use_powerstroke",
            _("Use powerstroke"),
            _("Export weighted paths using Inkscape's Powerstroke live path effects."),
            "boolean",
            nullptr,   //range
            "true", //defvalue
            0,     //flags
            nullptr);//newfunc
 
    def->push("data_meta",
            _("Meta to data-*"),
            _("Convert any object metadata starting with \"data-\" to data-* attributes in elements. Also class is used as the class attribute. Note letters for data are forced to lower case."),
            "boolean",
            nullptr,   //range
            "true", //defvalue
            0,     //flags
            nullptr);//newfunc
 
    def->push("pixels_per_inch",
            _("Pixels per inch"),
            _("Pixels per inch"),
            "real",
            nullptr,   //range
            "96", //defvalue
            0,     //flags
            nullptr);//newfunc
 

	stylemanager.AddObjectDef(def,0);
	return def;
}

//void SvgExportConfig::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
Laxkit::Attribute *SvgExportConfig::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	att = DocumentExportConfig::dump_out_atts(att, what, context);

	if (what == -1) {
		att->push("use_mesh","no",       "whether to output meshes as svg2 meshes");
		att->push("use_powerstroke","no","whether to use Inkscape's powerstroke LPE with paths where appropriate");
		att->push("pixels_per_inch","96","Pixels per inch. Usually 96 (css's value) is a safe bet.");
		att->push("data_meta","no",      "Whether to convert object metadata to data-* attributes.");
		return att;
	}

	att->push("use_mesh",        use_mesh?"yes":"no");
	att->push("use_powerstroke", use_powerstroke?"yes":"no");
	att->push("pixels_per_inch", pixels_per_inch);
	att->push("data_meta",       data_meta?"yes":"no");

	return att;
}

void SvgExportConfig::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	DocumentExportConfig::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"use_mesh")) use_mesh = BooleanAttribute(value);
		else if (!strcmp(name,"use_powerstroke")) use_powerstroke = BooleanAttribute(value);
		else if (!strcmp(name,"data_meta")) data_meta = BooleanAttribute(value);
		else if (!strcmp(name,"pixels_per_inch")) {
			double d=0;
			DoubleAttribute(value, &d);
			if (d!=0) pixels_per_inch = d;
		}
	}
}

//! Returns a new SvgExportConfig.
Value *newSvgExportConfig()
{
	SvgExportConfig *d=new SvgExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Svg"))
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}


//------------------------------------ SvgImportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newSvgImportConfig()
{
	ImportConfig *d=new ImportConfig;
	for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"Svg"))
			d->filter=laidout->importfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//------------------------------------ SvgOutputFilter ----------------------------------
	
/*! \class SvgOutputFilter
 * \brief Filter for exporting SVG 1.0.
 */


SvgOutputFilter::SvgOutputFilter()
{
	version=1.1;
	//flags=FILTERS_MULTIPAGE; //***not multipage yet!
}

const char *SvgOutputFilter::Version()
{
	return "1.1"; 
}

const char *SvgOutputFilter::VersionName()
{
	return _("Svg 1.1"); 
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *SvgOutputFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("SvgExportConfig");
	if (styledef) return styledef; 

	styledef = makeObjectDef();
	makestr(styledef->name,"SvgExportConfig");
	makestr(styledef->Name,_("Svg Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to an svg file."));
	styledef->newfunc = newSvgExportConfig;

//    styledef->push("usemesh",
//            _("Use Mesh"),
//            _("Use the format for the mesh branch of Inkscape for mesh gradients."),
//            "boolean",
//            nullptr, //range
//            "false",  //defvalue
//            0,    //flags
//            nullptr);//newfunc
//
//    styledef->push("usepowerstroke",
//            _("Use Powerstroke"),
//            _("Use the format for the powerstroke path effect features of Inkscape."),
//            "boolean",
//            nullptr, //range
//            "true",  //defvalue
//            0,    //flags
//            nullptr);//newfunc



	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}

/*! Output the svg "d" data.
 */
static int svgaddpath(FILE *f,Coordinate *path)
{
	Coordinate *p,*p2,*start;
	p=start=path->firstPoint(1);
	if (!p) return 0;

	 //build the path to draw
	flatpoint c1,c2;
	start=p;
	int n=1; //number of points seen

	fprintf(f,"M %.10g %.10g ",start->p().x,start->p().y);
	do { //one loop per vertex point
		p2=p->next; //p points to a vertex
		if (!p2) break;

		n++;

		//p2 now points to first Coordinate after the first vertex
		if (p2->flags&(POINT_TOPREV|POINT_TONEXT)) {
			 //we do have control points
			if (p2->flags&POINT_TOPREV) {
				c1=p2->p();
				p2=p2->next;
			} else c1=p->p();
			if (!p2) break;

			if (p2->flags&POINT_TONEXT) {
				c2=p2->p();
				p2=p2->next;
			} else { //otherwise, should be a vertex
				//p2=p2->next;
				c2=p2->p();
			}

			fprintf(f,"C %.10g %.10g %.10g %.10g %.10g %.10g ",
					c1.x,c1.y,
					c2.x,c2.y,
					p2->p().x,p2->p().y);
		} else {
			 //we do not have control points, so is just a straight line segment
			fprintf(f,"L %.10g %.10g ", p2->p().x,p2->p().y);
		}
		p=p2;
	} while (p && p->next && p!=start);
	if (p==start) fprintf(f,"z ");

	return n;
}

/*! Returns the number of points processed of points.
 */
static int svgaddpath(FILE *f,flatpoint *points,int n)
{
	if (n<=0) return 0;

	 //build the path to draw
	flatpoint c1,c2,p2;
	int np=1; //number of points seen
	bool onfirst=true;
	int ii=0;
	int ifirst=0;

	for (int i=ii; i<n; i++) {
		 //one loop per vertex point
		np++;

		if (onfirst) {
			onfirst=false;

			ifirst=i;
			while (i<n && (points[i].info&LINE_Bez)!=0 && (points[i].info&LINE_Vertex)==0) i++;
			fprintf(f,"M %.10g %.10g ",points[i].x,points[i].y);
			i++;
		}

		//i now points to first Coordinate after the first vertex
		if (points[i].info&LINE_Bez) {
			 //we do have control points
			 //by convention, there MUST be 2 cubic bezier controls
			c1=points[i];
			ii=-1;
			if (points[i].info&LINE_Open) ii=-2;
			else if (points[i].info&LINE_Closed) {
				ii=i;
				i=ifirst;
			} else i++;

			if (ii!=-2) {
				c2=points[i];

				if (points[i].info&LINE_Open) ii=-2;
				else if (points[i].info&LINE_Closed) {
					ii=i;
					i=ifirst;
				} else i++;

				if (ii!=-2) {
					p2=points[i];

					if (ii>=0) i=ii;

					fprintf(f,"C %.10g %.10g %.10g %.10g %.10g %.10g ",
							c1.x,c1.y,
							c2.x,c2.y,
							p2.x,p2.y);
				}
			}
		} else {
			 //we do not have control points, so is just a straight line segment
			fprintf(f,"L %.10g %.10g ", points[i].x,points[i].y);
			//i++;
		}

		if (points[i].info&LINE_Closed) {
			fprintf(f,"z ");
			onfirst=true;
		} else if (points[i].info&LINE_Open) {
			onfirst=true;
		} 
	}

	return np;
}

/*! Assumes "style=" has already been output to f.
 * After returning, user must supply closing quote mark.
 */
void svgStyleTagsDump(FILE *f, LineStyle *lstyle, FillStyle *fstyle, DrawableObject *fillobj)
{
	 //---write style: style="fill:none;stroke:#000000;stroke-width:1px;stroke-opacity:1"

	 //stroke
	if (lstyle) { 
		if (lstyle->capstyle==LAXCAP_Butt) fprintf(f,"stroke-linecap:butt; ");
		else if (lstyle->capstyle==LAXCAP_Round) fprintf(f,"stroke-linecap:round; ");
		else if (lstyle->capstyle==LAXCAP_Projecting) fprintf(f,"stroke-linecap:square; ");

		if (lstyle->joinstyle==LAXJOIN_Miter) fprintf(f,"stroke-linejoin:miter; ");
		else if (lstyle->joinstyle==LAXJOIN_Round) fprintf(f,"stroke-linejoin:round; ");
		else if (lstyle->joinstyle==LAXJOIN_Bevel) fprintf(f,"stroke-linejoin:bevel; ");

		if (lstyle->width==0) fprintf(f,"stroke-width:.01; ");//hairline width not supported in svg
		else if (lstyle->widthtype == 0) fprintf(f,"stroke-width:%.10g; ",lstyle->width/DEFAULT_PPINCH);
		else fprintf(f,"stroke-width:%.10g; ",lstyle->width);

		 //dash or not
		if (lstyle->dotdash==0 || lstyle->dotdash==~0)
			fprintf(f,"stroke-dasharray:none; ");
		else fprintf(f,"stroke-dasharray:%.10g,%.10g; ",lstyle->width,2*lstyle->width);

		if (lstyle->hasStroke()) {
			fprintf(f,"stroke:#%02x%02x%02x; stroke-opacity:%.10g; ",
								lstyle->color.red>>8, lstyle->color.green>>8, lstyle->color.blue>>8,
								lstyle->color.alpha/65535.);

			fprintf(f,"stroke-miterlimit:%.10g; ",lstyle->miterlimit);
		} 
	} else fprintf(f,"stroke:none; ");


	 //fill
	if (fillobj) {
		GradientData *grad = dynamic_cast<GradientData*>(fillobj);
		if (grad) {
			fprintf(f,"fill:url(#%sGradient%ld);", grad->IsRadial() ? "radial" : "linear", fillobj->object_id);

		} else fprintf(f,"fill:url(#ColorPatchFill%ld);", fillobj->object_id);
		
	} else if (fstyle) {
		if  (    fstyle->fillrule==LAXFILL_EvenOdd) fprintf(f,"fill-rule:evenodd; ");
		else if (fstyle->fillrule==LAXFILL_Nonzero) fprintf(f,"fill-rule:nonzero; ");

		if (fstyle->hasFill()) {
			fprintf(f,"fill:#%02x%02x%02x; fill-opacity:%.10g; ",
						fstyle->color.red>>8, fstyle->color.green>>8, fstyle->color.blue>>8,
						fstyle->color.alpha/65535.);
		} else fprintf(f,"fill:none; ");
	} else fprintf(f,"fill:none; ");
}

static void svgMeshPathOut(FILE *f, const char *spc, ColorPatchData *patch, int ci, int i1, int i2, int i3)
{
	if (ci >= 0) fprintf(f, "%s          <stop path=\"C %.10g %.10g %.10g %.10g %.10g %.10g\" "
								 "stop-color=\"rgb(%d,%d,%d)\" stop-opacity=\"%.10g\" />\n",
						spc,
						patch->points[i1].x,patch->points[i1].y,
						patch->points[i2].x,patch->points[i2].y,
						patch->points[i3].x,patch->points[i3].y,
						(int)(patch->colors[ci].Red()*255),
						(int)(patch->colors[ci].Green()*255),
						(int)(patch->colors[ci].Blue()*255),
						patch->colors[ci].Alpha()
						);
	else fprintf(f, "%s          <stop path=\"C %.10g %.10g %.10g %.10g %.10g %.10g \" />\n",
						spc,
						patch->points[i1].x,patch->points[i1].y,
						patch->points[i2].x,patch->points[i2].y,
						patch->points[i3].x,patch->points[i3].y);
}

//! Function to dump out obj as svg.
/*! Return nonzero for fatal errors encountered, else 0.
 *
 * Remember that connected defs are collected in svgdumpdef().
 *
 * \todo add warning when invalid radial gradient: one circle not totally contained in another
 */
int svgdumpobj(FILE *f,double *mm,SomeData *obj,int &warning, int indent, ErrorLog &log, SvgExportConfig *out, 
				bool ignore_filter = false, bool data_meta = false)
{
	Group *g=dynamic_cast<Group *>(obj);
	if (g && g->filter && !ignore_filter) {
		obj = g->FinalObject();
		if (obj) return svgdumpobj(f,mm,obj,warning,indent,log,out, true, data_meta);
		return 0;
	}

	DrawableObject *dobj = dynamic_cast<DrawableObject*>(obj);
	const char *hiddenstr = (obj->Visible() ? "" : "display:none;");

	Utf8String datameta;
	if (data_meta && dobj != nullptr) {
		Attribute *obj_meta = dobj->metadata;
		if (obj_meta != nullptr) {
			//construct data-* out of the meta
			Utf8String str;
			int ch;
			for (int c=0; c<obj_meta->attributes.n; c++) {
				if (strncmp(obj_meta->attributes.e[c]->name, "data-", 5)) {
					if (!strcmp(obj_meta->attributes.e[c]->name, "class")) {
						datameta.Append("class=\"");
						datameta.Append(obj_meta->attributes.e[c]->value);
						datameta.Append("\"");
					}
					continue;
				}
				str = obj_meta->attributes.e[c]->name;
				str.Replace(" ","_",true);
				for (int c2=0; c2<str.Bytes(); c2++) {
					ch = str.byte(c2);
					if (ch >= 'A' && ch <= 'Z') ch = tolower(ch);
					if (!isalnum(ch) && ch != '-' && ch != '_') str.byte(c2, '-');
				}
				//datameta.Append("data-");
				datameta.Append(str+"=\""+obj_meta->attributes.e[c]->value+"\" "); // *** really need to do some sanity checking on value as well
			}
		}
	}

	char clipid[100];
	clipid[0] = '\0';
	if (dobj->clip_path) sprintf(clipid, "clip-path=\"url(#clipPath%lu)\"", dobj->clip_path->object_id);

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (!strcmp(obj->whattype(),"Group")) {
		fprintf(f,"%s<g %s id=\"%s\" %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					spc, 
					datameta.c_str_nonnull(),
					obj->Id(), clipid, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) 
			svgdumpobj(f,nullptr,g->e(c),warning,indent+2,log, out, false, data_meta); 
		fprintf(f,"    </g>\n");

	} else if (!strcmp(obj->whattype(),"GradientData")) {
		GradientData *grad;
		grad=dynamic_cast<GradientData *>(obj);
		if (!grad) return 0;

		const char *spreadMethod = "pad";
		if (grad->spread_method == LAXSPREAD_Reflect) spreadMethod = "reflect";
		else if (grad->spread_method == LAXSPREAD_Repeat) spreadMethod = "repeat";

		if (grad->IsRadial()) {
			fprintf(f,"%s<circle %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc,
						 datameta.c_str_nonnull(),
						 obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    id=\"%s\" %s\n", spc,grad->Id(), clipid);
			fprintf(f,"%s    style=\"%sstroke:none;fill:url(#radialGradient%ld)\"\n", spc, hiddenstr, grad->object_id);
			fprintf(f,"%s    cx=\"%f\"\n",spc, fabs(grad->R1()) > fabs(grad->R2()) ? grad->P1().x : grad->P2().x);
			fprintf(f,"%s    cy=\"%f\"\n",spc, fabs(grad->R1()) > fabs(grad->R2()) ? grad->P1().y : grad->P2().y);
			fprintf(f,"%s    r=\"%f\"\n",spc,  fabs(grad->R1()) > fabs(grad->R2()) ? fabs(grad->R1()) : fabs(grad->R2()));
			fprintf(f,"%s    spreadMethod=\"%s\"\n",spc, spreadMethod);
			fprintf(f,"%s  />\n",spc);
		} else {
			fprintf(f,"%s<rect %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, datameta.c_str_nonnull(),
						 obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    id=\"%s\" %s\n", spc,grad->Id(), clipid);
			fprintf(f,"%s    style=\"%sstroke:none;fill:url(#linearGradient%ld)\"\n", spc, hiddenstr, grad->object_id);
			fprintf(f,"%s    x=\"%f\"\n", spc,grad->minx);
			fprintf(f,"%s    y=\"%f\"\n", spc,grad->miny);
			fprintf(f,"%s    width=\"%f\"\n", spc,grad->maxx-grad->minx);
			fprintf(f,"%s    height=\"%f\"\n", spc,grad->maxy-grad->miny);
			fprintf(f,"%s    spreadMethod=\"%s\"\n",spc, spreadMethod);
			fprintf(f,"%s  />\n",spc);
		}

	} else if (!strcmp(obj->whattype(),"CaptionData")) {
		CaptionData *caption=dynamic_cast<CaptionData*>(obj);
		if (!caption) return 0;

		if (out->textaspaths) {
			SomeData *path = caption->ConvertToPaths(false, nullptr);
			svgdumpobj(f,mm,path,warning,indent,log,out, false,data_meta);
			path->dec_count();
			return 0;
		}

		double rr,gg,bb,aa;
		Palette *palette=dynamic_cast<Palette*>(caption->font->GetColor());

		int layer=0;
		for (LaxFont *font=caption->font; font; font=font->nextlayer) {
			fprintf(f,"%s<text %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));

			fprintf(f,"%s   id=\"%s\" %s\n", spc, caption->Id(), clipid);
			fprintf(f,"%s   x =\"0\"\n", spc);
			fprintf(f,"%s   y =\"%.10g\"\n", spc, caption->font->ascent());

			if (palette && layer<palette->colors.n) {
				rr = palette->colors.e[layer]->color->values[0];
				gg = palette->colors.e[layer]->color->values[1];
				bb = palette->colors.e[layer]->color->values[2];
				aa = palette->colors.e[layer]->color->values[3]; 

			} else {
				rr=caption->red;
				gg=caption->green;
				bb=caption->blue;
				aa=caption->alpha;
			}

			fprintf(f,"%s   style=\"%sfill:#%02x%02x%02x; fill-opacity:%.10g; ",
						spc, hiddenstr, int(rr*255+.5), int(gg*255+.5), int(bb*255+.5), aa);
			//if (caption->xcentering==0) fprintf(f,"text-anchor:start; ");
			//else if (caption->xcentering==50) fprintf(f,"text-anchor:middle; ");
			//else if (caption->xcentering==100) fprintf(f,"text-anchor:end; ");

			fprintf(f,"font-family:%s; font-style:%s; font-size:%.10g; line-height:%.10g%%;\">\n",
						font->Family(), font->Style()?font->Style():"normal", font->Msize(), 100*font->textheight()/font->Msize()*caption->linespacing);

			//double h=caption->maxy-caption->miny;
			double x,y;
			//double y=caption->origin().y - h*caption->ycentering/100 + caption->font->ascent();
			//double y=caption->origin().y - h*caption->ycentering/100;

			 //now do the lines, each line gets a tspan
			for (int c=0; c<caption->lines.n; c++) {
				x = -caption->xcentering/100*(caption->linelengths[c]);
				y = -font->ascent() + c*caption->font->textheight()*caption->linespacing;

				 //the sodipodi bit is necessary to make Inkscape (at least to 0.091) let you access lines beyond the first
				//fprintf(f,"%s  <tspan sodipodi:role=\"line\" dx=\"%.10g\" y=\"%.10g\" textLength=\"%.10g\">%s</tspan>\n",
				//		spc, x,caption->font->Msize()*caption->linespacing, caption->linelengths[c], caption->lines.e[c]);
				DBG fprintf(f,"<!--  ascent:%.10g  descent:%.10g  textheight:%.10g  msize:%.10g  -->\n",
				DBG 		caption->font->ascent(),
				DBG 		caption->font->descent(),
				DBG 		caption->font->textheight(),
				DBG 		caption->font->Msize());
				fprintf(f,"%s  <tspan sodipodi:role=\"line\" dx=\"%.10g\" y=\"%.10g\" textLength=\"%.10g\">%s</tspan>\n",
						spc, x,y, caption->linelengths[c], caption->lines.e[c]);

				//y+=caption->fontsize*caption->LineSpacing();
			}
			fprintf(f,"%s</text>\n", spc);

			layer++;
		}

	} else if (!strcmp(obj->whattype(),"TextOnPath")) {
		TextOnPath *text=dynamic_cast<TextOnPath*>(obj);
		if (!text || text->end-text->start<=0) return 0;
		if (!text->paths) { log.AddMessage(_("Text on path object missing path, ignoring!"),ERROR_Warning); return 0; }

		if (out->textaspaths) {
			SomeData *path = text->ConvertToPaths(false, nullptr);
			svgdumpobj(f,mm,path,warning,indent,log,out);
			path->dec_count();
			return 0;
		}

		 //first dump out a blank path:
		svgdumpobj(f, nullptr, text->paths, warning, indent+2, log, out);

		 //now dump out the text, refering to that path..
		double rr=0,gg=0,bb=0,aa=1.0;
		Palette *palette=dynamic_cast<Palette*>(text->font->GetColor());

		int layer=0;
		for (LaxFont *font=text->font; font; font=font->nextlayer) {
			//fprintf(f,"%s<text transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
			//			spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s<text %s\n",
						spc, datameta.c_str_nonnull());

			fprintf(f,"%s   id=\"%s\"\n", spc, text->Id());
			//fprintf(f,"%s   x =\"0\"\n", spc);
			//fprintf(f,"%s   y =\"%.10g\"\n", spc, text->font->ascent());

			if (palette && layer<palette->colors.n) {
				rr = palette->colors.e[layer]->color->values[0];
				gg = palette->colors.e[layer]->color->values[1];
				bb = palette->colors.e[layer]->color->values[2];
				aa = palette->colors.e[layer]->color->values[3]; 

			} else if (text->color) {
				if (text->color->ColorSystemId()==LAX_COLOR_RGB) {
					rr = text->color->values[0];
					gg = text->color->values[1];
					bb = text->color->values[2];
					aa = text->color->values[3];
				}

			} else {
				rr=gg=bb=0;
				aa=1.0;
			}

			fprintf(f,"%s   style=\"%sfill:#%02x%02x%02x; fill-opacity:%.10g; ",
						spc, hiddenstr, int(rr*255+.5), int(gg*255+.5), int(bb*255+.5), aa);
			//if (text->xcentering==0) fprintf(f,"text-anchor:start; ");
			//else if (text->xcentering==50) fprintf(f,"text-anchor:middle; ");
			//else if (text->xcentering==100) fprintf(f,"text-anchor:end; ");

			fprintf(f,"font-family:%s; font-style:%s; font-size:%.10g; \">\n",
						font->Family(), font->Style()?font->Style():"normal", font->Msize()/72); // *** no idea if the 72 is accurate!!! check this!!!!

			//double h=text->maxy-text->miny;
			//double x,y;
			//double y=text->origin().y - h*text->ycentering/100 + text->font->ascent();
			//double y=text->origin().y - h*text->ycentering/100;

			 //now do the actual text in a <textPath><tspan>
			//x = -text->xcentering/100*(text->linelengths[c]);
			//y = -font->ascent() + c*text->font->textheight()*text->linespacing;

			 //the sodipodi bit is necessary to make Inkscape (at least to 0.091) let you access lines beyond the first
			DBG fprintf(f,"<!--  ascent:%.10g  descent:%.10g  textheight:%.10g  msize:%.10g  -->\n",
			DBG 		text->font->ascent(),
			DBG 		text->font->descent(),
			DBG 		text->font->textheight(),
			DBG 		text->font->Msize());

			//fprintf(f,"%s  <textPath xlink:href=\"#%s\"><tspan dx=\"%.10g\" y=\"%.10g\" textLength=\"%.10g\">%s</tspan></textPath>\n",
			char txt[text->end-text->start+1];
			strncpy(txt, text->text+text->start, text->end-text->start);
			txt[text->end - text->start] = '\0';
			fprintf(f,"%s  <textPath xlink:href=\"#%s\" startOffset=\"%.10g\"><tspan>%s</tspan></textPath>\n",
					spc, text->paths->Id(), text->start_offset, txt);

			//y+=text->fontsize*text->LineSpacing();

			fprintf(f,"%s</text>\n", spc);

			layer++;
		}

	} else if (!strcmp(obj->whattype(),"ImageData")) {
		ImageData *img;
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return 0;

		flatpoint o=obj->origin();
		o+=obj->yaxis()*(obj->maxy-obj->miny);
		
		fprintf(f,"%s<image %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
				     spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), -obj->m(2), -obj->m(3), o.x, o.y);
		fprintf(f,"%s    id=\"%s\" %s\n", spc,img->Id(), clipid);
		fprintf(f,"%s    xlink:href=\"%s\" \n", spc,img->filename);
		fprintf(f,"%s    x=\"%f\"\n", spc,img->minx);
		fprintf(f,"%s    y=\"%f\"\n", spc,img->miny);
		fprintf(f,"%s    width=\"%f\"\n", spc,img->maxx-img->minx);
		fprintf(f,"%s    height=\"%f\"\n", spc,img->maxy-img->miny);
		fprintf(f,"%s  />\n",spc);
		
	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		if (!out->use_mesh) {
			setlocale(LC_ALL,"");
			log.AddMessage(obj->object_id,nullptr,nullptr,_("Warning: interpolating a color patch object\n"),ERROR_Warning);
			setlocale(LC_ALL,"C");
			warning++;

			 //approximate gradient with svg elements.
			 //in Inkscape, its blur slider is 1 to 100, with 100 corresponding to a blurring
			 //radius (standard deviation of Gaussian function) of 1/8 of the
			 //object's bounding box' perimeter (that is, for a square, a blur of
			 //100% will have the radius equal to half a side, which turns any shape
			 //into an amorphous cloud).
			ColorPatchData *patch=dynamic_cast<ColorPatchData *>(obj);
			if (!patch) return 0;

			 //make a group with a mask of outline of original patch, and blur filter
			fprintf(f,"%s<g %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
				     spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    id=\"%s\"\n", spc,patch->Id());
			fprintf(f,"%s   clip-path=\"url(#colorPatchMask%ld)\" filter=\"url(#patchBlur%ld)\">\n", 
						spc, patch->object_id, patch->object_id);

			int c,r, cc,rr;
			int numdiv=5;
			flatpoint p[4];
			ScreenColor color;
			double s,ds, t,dt;
			ds=1./(patch->xsize/3)/numdiv;
			dt=1./(patch->ysize/3)/numdiv;
			double fudge=1.05; //so there are no transparent boundaries between the divided up rects

			 //create border colors so as to not have transparent outer edge sections.
			 //this has to be done before the actual patch, so that the real colors lay
			 //on top of this stuff.
			flatpoint pcenter=patch->getPoint(.5,.5,false);
			double extend=2;
			for (r=0; r<patch->ysize/3; r++) {
			    for (rr=0; rr<numdiv; rr++) {
					//s=(c+(float)cc/numdiv)/(patch->xsize/3);
					t=(r+(float)rr/numdiv)/(patch->ysize/3);

					 //---------left side
				     //get color for point (r+rr,c+cc)
					patch->WhatColor(0,t/(1-dt),&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(0   ,t,          false);
					p[1]=patch->getPoint(0   ,t+fudge*dt, false);
					p[2]=pcenter + extend*(p[1]-pcenter);
					p[3]=pcenter + extend*(p[0]-pcenter);

			  		fprintf(f,"%s  <path d=\"M %f %f L %f %f L %f %f L %f %f z\" stroke=\"none\" "
							  "fill=\"#%02x%02x%02x\" fill-opacity=\"%f\"/>\n",
			  					spc, p[0].x,p[0].y,
			  					p[1].x,p[1].y,
			  					p[2].x,p[2].y,
			  					p[3].x,p[3].y,
			  					color.red>>8, color.green>>8, color.blue>>8,
								color.alpha/65535.);

					 //---------right side
				     //get color for point (r+rr,c+cc)
					patch->WhatColor(1.,t/(1-dt),&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(1.   ,t,          false);
					p[1]=patch->getPoint(1.   ,t+fudge*dt, false);
					p[2]=pcenter+extend*(p[1]-pcenter);
					p[3]=pcenter+extend*(p[0]-pcenter);

			  		fprintf(f,"%s  <path d=\"M %f %f L %f %f L %f %f L %f %f z\" stroke=\"none\" "
							  "fill=\"#%02x%02x%02x\" fill-opacity=\"%f\"/>\n",
			  					spc, p[0].x,p[0].y,
			  					p[1].x,p[1].y,
			  					p[2].x,p[2].y,
			  					p[3].x,p[3].y,
			  					color.red>>8, color.green>>8, color.blue>>8,
								color.alpha/65535.);
			    }
			}
			for (c=0; c<patch->xsize/3; c++) {
			    for (cc=0; cc<numdiv; cc++) {
					s=(c+(float)cc/numdiv)/(patch->xsize/3);

					 //---------top side
				     //get color for point (r+rr,c+cc)
					patch->WhatColor(s/(1-ds),0,&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(s,          0, false);
					p[1]=patch->getPoint(s+fudge*ds, 0, false);
					p[2]=pcenter+extend*(p[1]-pcenter);
					p[3]=pcenter+extend*(p[0]-pcenter);

			  		fprintf(f,"%s  <path d=\"M %f %f L %f %f L %f %f L %f %f z\" stroke=\"none\" "
							  "fill=\"#%02x%02x%02x\" fill-opacity=\"%f\"/>\n",
			  					spc, p[0].x,p[0].y,
			  					p[1].x,p[1].y,
			  					p[2].x,p[2].y,
			  					p[3].x,p[3].y,
			  					color.red>>8, color.green>>8, color.blue>>8,
								color.alpha/65535.);

					 //---------bottom side
				     //get color for point (r+rr,c+cc)
					patch->WhatColor(s/(1-ds),1,&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(s,          1, false);
					p[1]=patch->getPoint(s+fudge*ds, 1, false);
					p[2]=pcenter+extend*(p[1]-pcenter);
					p[3]=pcenter+extend*(p[0]-pcenter);

			  		fprintf(f,"%s  <path d=\"M %f %f L %f %f L %f %f L %f %f z\" stroke=\"none\" "
							  "fill=\"#%02x%02x%02x\" fill-opacity=\"%f\"/>\n",
			  					spc, p[0].x,p[0].y,
			  					p[1].x,p[1].y,
			  					p[2].x,p[2].y,
			  					p[3].x,p[3].y,
			  					color.red>>8, color.green>>8, color.blue>>8,
								color.alpha/65535.);
			    }
			}

			 //for each subpatch, break down into many sub-rectangles
			double tt,ss;
			for (r=0; r<patch->ysize/3; r++) {
			  for (c=0; c<patch->xsize/3; c++) {
			    for (rr=0; rr<numdiv; rr++) {
			      for (cc=0; cc<numdiv; cc++) {
					s=(c+(float)cc/numdiv)/(patch->xsize/3);
					t=(r+(float)rr/numdiv)/(patch->ysize/3);
					DBG cerr <<" point s,t:"<<s<<','<<t<<endl;

				     //get color for point (r+rr,c+cc)
					tt=t/(1-dt);
					if (tt<0) tt=0; else if (tt>1) tt=1;
					ss=s/(1-ds);
					if (ss<0) ss=0; else if (ss>1) ss=1;
					patch->WhatColor(ss,tt,&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(s         ,t,          false);
					p[1]=patch->getPoint(s+fudge*ds,t,          false);
					p[2]=patch->getPoint(s+fudge*ds,t+fudge*dt, false);
					p[3]=patch->getPoint(s         ,t+fudge*dt, false);

			  		fprintf(f,"%s  <path d=\"M %f %f L %f %f L %f %f L %f %f z\" stroke=\"none\" "
							  "fill=\"#%02x%02x%02x\" fill-opacity=\"%f\"/>\n",
			  					spc, p[0].x,p[0].y,
			  					p[1].x,p[1].y,
			  					p[2].x,p[2].y,
			  					p[3].x,p[3].y,
			  					color.red>>8, color.green>>8, color.blue>>8,
								color.alpha/65535.);
			      }
			    }		
			  }
			}
		

			fprintf(f,"%s</g>\n",spc);

		} else { //use_mesh
			ColorPatchData *patch=dynamic_cast<ColorPatchData *>(obj);
			if (!patch) return 0;

			fprintf(f,"%s<rect %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    id=\"%s\"\n", spc,patch->Id());
			fprintf(f,"%s    style=\"%sfill:url(#ColorPatchFill%ld)\"\n", spc, hiddenstr, patch->object_id);
			fprintf(f,"%s    x=\"%f\"\n", spc, patch->minx);
			fprintf(f,"%s    y=\"%f\"\n", spc, patch->miny);
			fprintf(f,"%s    width=\"%f\"\n",  spc, patch->maxx-patch->minx);
			fprintf(f,"%s    height=\"%f\"\n", spc, patch->maxy-patch->miny);
			fprintf(f,"%s  />\n",spc);
		}
		

	} else if (!strcmp(obj->whattype(),"PathsData")) {
		 //for weighted paths (any offset, nonzero angle, or variable width), there are 2 options:
		 //  1. output as powerstroke LPE
		 //  2. output 2 paths from original weighted path: fill in path->centercache and fill with stroke color in: path->outlinecache 
		 //Otherwise, is plain old path
		 //
		 // \todo **** really this svg export should be part of Path and PathsData classes, it being a common need

		PathsData *pdata=dynamic_cast<PathsData*>(obj);
		if (pdata->paths.n==0) return 0; //ignore empty path objects

		LineStyle *lstyle=pdata->linestyle;
		if (lstyle && lstyle->hasStroke()==0) lstyle=nullptr;
		FillStyle *fstyle=pdata->fillstyle;
		if (fstyle && fstyle->hasFill()==0) fstyle=nullptr;
		if (!lstyle && !fstyle) return 0;

		int weighted=0;
		bool open=true;
		for (int c=0; c<pdata->paths.n; c++) {
			if (!pdata->paths.e[c]->path) continue;
			if (pdata->paths.e[c]->Weighted()) weighted++;
			if (pdata->paths.e[c]->IsClosed()) open=false;
		}

		DrawableObject *fillobj = nullptr;
		if (dobj && dobj->NumKids()) {
			DrawableObject *kid = dynamic_cast<DrawableObject*>(dobj->Child(0));
			GradientData *grad = dynamic_cast<GradientData*>(kid);
			if (grad) {
				fillobj = kid;
			} else {
				ColorPatchData *mesh = dynamic_cast<ColorPatchData *>(kid);
				if (mesh) {
					fillobj = kid;
				}
			}
			if (dobj->NumKids()-(fillobj ? 1 : 0) > 0) log.AddWarning(_("Ignoring path children"));
		}

		if (!weighted) {
			 //plain, ordinary path with no offset and constant width

			fprintf(f,"%s<path %s  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s       id=\"%s\" %s\n", spc,obj->Id(), clipid);

			fprintf(f,"%s       d=\"",spc);

			Path *path;
			for (int c=0; c<pdata->paths.n; c++) {
				path=pdata->paths.e[c];
				if (!path->path) continue;

				svgaddpath(f,path->path); // <- ordinary path, no special treatment
			}
			fprintf(f,"\"\n");//end of "d" for non-weighted or original-d for weighted

			fprintf(f,"%s       style=\"%s",spc, hiddenstr);
			svgStyleTagsDump(f, lstyle, fstyle, fillobj);
			fprintf(f,"\"\n");//end of "style"

			fprintf(f,"%s />\n",spc);//end of PathsData!


		} else {
			//at least one weighted path, may or may not use powerstroke

			 //a path based on centercache, with live path effect for powerstroke. Another for offset?
			//*** how to map weight nodes to centercache????

			if (fstyle && fstyle->hasFill() && !open) {
				fprintf(f,"%s<path %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
							 spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
				fprintf(f,"%s       id=\"%s-fill\" %s\n", spc,obj->Id(), clipid);

				 //---write style for fill within centercache.. no stroke to that, as we apply artificial stroke
				fprintf(f,"%s       style=\"%s",spc, hiddenstr);
				svgStyleTagsDump(f, nullptr, fstyle, fillobj);
				fprintf(f,"\"\n");//end of "style"

				fprintf(f,"%s       d=\"",spc);
				Path *path;
				for (int c=0; c<pdata->paths.n; c++) {
					path=pdata->paths.e[c];
					if (path->needtorecache) path->UpdateCache();
					if (!path->path) continue;

					if (path->Weighted()) svgaddpath(f,path->centercache.e,path->centercache.n);
					else svgaddpath(f,path->path);
				}
				fprintf(f,"\"\n");//end of "d" for weighted

				fprintf(f,"%s />\n",spc);//end of fill PathsData!
			}

			if (lstyle) {
				 //second "stroke" path to be filled, or to be set up as powerstroke base
				fprintf(f,"%s<path %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
							 spc, datameta.c_str_nonnull(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
				fprintf(f,"%s       id=\"%s\" %s\n", spc,obj->Id(), clipid);

				 //---write style: no linestyle, but fill style is based on linestyle
				fprintf(f,"%s       style=\"%s",spc, hiddenstr);
				FillStyle fillstyle;
				fillstyle.color=lstyle->color;
				svgStyleTagsDump(f, nullptr, &fillstyle, fillobj);
				fprintf(f,"\"\n");//end of "style"


				 //both powerstroke and regular use outlinecache...
				fprintf(f,"%s       d=\"",spc);
				for (int c=0; c<pdata->paths.n; c++) {
					Path *path=pdata->paths.e[c];
					if (!path->path) continue;
					if (path->needtorecache) path->UpdateCache();

					svgaddpath(f,path->outlinecache.e,path->outlinecache.n);
				}
				fprintf(f,"\"\n");//end of "d" for weighted


				 // Warning!! currently powerstroke LPE for inkscape only works on first path!!
				if (out->use_powerstroke) {
					fprintf(f,"%s       inkscape:original-d=\"",spc);

					for (int c=0; c<pdata->paths.n; c++) {
						Path *path=pdata->paths.e[c];
						if (!path->path) continue;
						if (path->needtorecache) path->UpdateCache();

						svgaddpath(f,path->centercache.e,path->centercache.n);
					}
					fprintf(f,"\"\n");//end of "original-d" 


					 //Add ref to the powerstroke inkscape lpe
					 // *** Warning!! currently powerstroke LPE for inkscape only works on first path!!
					char *name=new char[30];
					sprintf(name,"stroke-%ld-%d",pdata->object_id, 0);
					fprintf(f,"%s       inkscape:path-effect=\"#%s\"\n", spc,name);
					delete[] name;
				}

				fprintf(f,"%s />\n",spc);//end of fill PathsData!
			}
		} //end if weighted path


//	} else if (!strcmp(obj->whattype(),"ImagePatchData")) {
//		// *** if (config->collect_for_out) { rasterize, and put image in out directory }
//		setlocale(LC_ALL,"");
//		log.AddMessage(_("Cannot export Image Patch objects into svg."),ERROR_Warning);
//		setlocale(LC_ALL,"C");
//		warning++;


	} else if (!strcmp(obj->whattype(),"SomeDataRef")) {
		SomeDataRef *ref=dynamic_cast<SomeDataRef*>(obj);
		if (!ref->thedata) {
			DBG cerr <<" WARNING! missing thedata in a somedataref id:"<<ref->object_id<<endl;
		} else {
			double m[6],m2[6];
			transform_invert(m,ref->thedata->m());
			transform_mult(m2,m,ref->m());
			fprintf(f,"%s<use  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, m2[0], m2[1], m2[2], m2[3], m2[4], m2[5]);

			 //write path
			fprintf(f,"%s   id=\"%s\" %s\n", spc,obj->Id(), clipid);
			fprintf(f,"%s   xlink:href=\"#%s\"\n", spc,ref->thedata->Id());
			fprintf(f,"%s />\n",spc);//end of clone!
		}


	} else {
		DrawableObject *dobj=dynamic_cast<DrawableObject*>(obj);
		SomeData *dobje=nullptr;
		if (dobj) dobje=dobj->EquivalentObject();

		if (dobje) {
			dobje->Id(obj->Id());
			svgdumpobj(f,mm,dobje,warning, indent, log, out, false, data_meta);
			dobje->dec_count();

		} else {

			setlocale(LC_ALL,"");
			char buffer[strlen(_("Cannot export %s objects into svg."))+strlen(obj->whattype())+1];
			sprintf(buffer,_("Cannot export %s objects into svg."),obj->whattype());
			log.AddMessage(obj->object_id,obj->nameid,nullptr, buffer,ERROR_Warning);
			setlocale(LC_ALL,"C");
			warning++;
		}
	}

	return 0;
}

/*! Use this during svgdumpdef to output a particular clipping path.
 */
void DumpClipPath(FILE *f, const char *clipid, PathsData *obj, const double *extra_m, int &warning,ErrorLog &log, SvgExportConfig *out)
{
	fprintf(f,"    <clipPath clipPathUnits=\"userSpaceOnUse\" id=\"%s\" >\n", clipid);
	fprintf(f,"      <path ");
	if (extra_m) {
		fprintf(f,"transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" ",
					extra_m[0], extra_m[1], extra_m[2], extra_m[3], extra_m[4], extra_m[5]); 
	}
	fprintf(f," d=\"");
	for (int c=0; c<obj->paths.n; c++) {
		svgaddpath(f, obj->paths.e[c]->path);
	}
	fprintf(f," \"/>\n    </clipPath>\n");
}

/*! Function to dump out any gradients to the defs section of an svg. Remember that
 * actual object dump is svgdumpobj().
 *
 * Return nonzero for fatal errors encountered, else 0.
 *
 * \todo fix radial gradient output for inner circle empty
 */
int svgdumpdef(FILE *f,double *mm,SomeData *obj,int &warning,ErrorLog &log, SvgExportConfig *out, bool ignore_filter=false)
{
	DrawableObject *dobj = dynamic_cast<DrawableObject*>(obj);
	if (dobj->clip_path) {
		 //not really sure why, but clip path has to be flipped vertically relative to the clipped object.
		char clipid[100];
		sprintf(clipid, "clipPath%lu", dobj->clip_path->object_id);
		Affine a(dobj->clip_path->m());
		flatpoint f1 = dobj->BBoxPoint(0,.5, false);
		flatpoint f2 = dobj->BBoxPoint(1,.5, false);
		a.Flip(f1,f2);
		DumpClipPath(f, clipid, dobj->clip_path, a.m(), warning, log, out);
	}

	Group *g=dynamic_cast<Group *>(obj);
	if (g && g->filter && !ignore_filter) {
		obj = g->FinalObject();
		if (obj) return svgdumpdef(f,mm,obj,warning,log,out, true);
		return 0;
	}

	if (!strcmp(obj->whattype(),"PathsData")) {
		 //write out a powerstroke def section. When there is an offset,
		 //not just a width change, the actual path is later
		 //converted to the "area path", not the defined path

		if (out->use_powerstroke) {
			PathsData *pdata=dynamic_cast<PathsData*>(obj);
			Path *path;

			for (int c=0; c<pdata->paths.n; c++) {
				if (!pdata->paths.e[c]->Weighted()) continue;
				path=pdata->paths.e[c];

				char *name=new char[30];
				sprintf(name,"stroke-%ld-%d",obj->object_id, c);

				fprintf(f,"<inkscape:path-effect\n"
						  "  effect=\"powerstroke\"\n"
						  "  id=\"%s\"\n"
						  "  is_visible=\"true\"\n",
							name);
				fprintf(f,"  offset_points=\"");

				for (int c2=0; c2<path->pathweights.n; c2++) {
					fprintf(f,"%.10g,%.10g ", path->pathweights.e[c2]->t, path->pathweights.e[c2]->width);
					if (c2<path->pathweights.n-1) fprintf(f,"| ");
				}

				//determine caps
				int startcap = (path->linestyle ? path->linestyle->capstyle : (pdata->linestyle ? pdata->linestyle->capstyle : LAXCAP_Butt));
				int endcap   = (path->linestyle ? path->linestyle->endcapstyle : (pdata->linestyle ? pdata->linestyle->endcapstyle : LAXCAP_Butt));
				if (endcap == 0) endcap = startcap;
				const char *startstr = "butt";
				const char *endstr   = "butt";

						//if      (!strcasecmp(value, "round"))     start_linecap = LAXCAP_Round;
						//else if (!strcasecmp(value, "peak"))      start_linecap = LAXCAP_Peak;
						//else if (!strcasecmp(value, "butt"))      start_linecap = LAXCAP_Butt;
						//else if (!strcasecmp(value, "square"))    start_linecap = LAXCAP_Square;
						//else if (!strcasecmp(value, "zerowidth")) start_linecap = LAXCAP_Zero_Width;
				if      (startcap == LAXCAP_Round)      startstr="round";
				else if (startcap == LAXCAP_Peak)       startstr="peak";
				else if (startcap == LAXCAP_Square)     startstr="square";
				else if (startcap == LAXCAP_Zero_Width) startstr="zerowidth";

				if      (endcap == LAXCAP_Round)      endstr="round";
				else if (endcap == LAXCAP_Peak)       endstr="peak";
				else if (endcap == LAXCAP_Square)     endstr="square";
				else if (endcap == LAXCAP_Zero_Width) endstr="zerowidth";

				fprintf(f,"\"\n"
						  "  sort_points=\"true\"\n"
						  "  interpolator_type=\"CentripetalCatmullRom\"\n"
						  "  interpolator_beta=\"0.2\"\n"
						  "  linejoin_type=\"extrapolated\"\n"
						  "  miter_limit=\"4\"\n"
						  "  start_linecap_type=\"%s\"\n"
						  "  end_linecap_type=\"%s\"\n"
						  " />\n",
						  startstr, endstr
						);


				delete[] name;
			}
		}

	} else if (!strcmp(obj->whattype(),"GradientData")) {
		GradientData *grad;
		grad = dynamic_cast<GradientData *>(obj);
		if (!grad) return 0;

		const char *spreadMethod = "pad";
		if (grad->spread_method == LAXSPREAD_Reflect)     spreadMethod = "reflect";
		else if (grad->spread_method == LAXSPREAD_Repeat) spreadMethod = "repeat";
		//svg doesn't have a "none"

		if (grad->IsRadial()) {
			double r1,r2;
			double p1,p2;
			int rr;
			double mm[6];
			grad->GradientTransform(mm, false);
			//pp = transform_point_inverse(mm, pp);
			flatpoint pp1 = transform_point_inverse(mm, grad->P1()); //pp1.x should be 0
			flatpoint pp2 = transform_point_inverse(mm, grad->P2());
			flatpoint opp1, opp2;

			if (fabs(grad->R1()) > fabs(grad->R2())) { 
				opp1 = grad->P2();
				opp2 = grad->P1();
				p2 = pp1.x;
				p1 = pp2.x;
				r2 = fabs(grad->R1());
				r1 = fabs(grad->R2());
				rr=1; 
			} else {
				opp1 = grad->P1();
				opp2 = grad->P2();
				p1 = pp1.x;
				p2 = pp2.x;
				r1 = fabs(grad->R1());
				r2 = fabs(grad->R2()); 
				rr=0; 
			}


			 // now figure out the color spots
			double clen = grad->strip->colors.e[grad->strip->colors.n-1]->t - grad->strip->colors.e[0]->t,
				   plen = MAX((p2-r2)-(p1-r1), (p2+r2)-(p1+r1)),
				   chunk,
				   c0 = grad->strip->colors.e[(rr ? grad->strip->colors.n-1 : 0)]->t,
				   c1;

			int cc;
			if (r1 != 0) {
				 // need extra 2 stops for transparent inner circle
				chunk = r1/plen;
				c1 = grad->strip->colors.e[(rr ? grad->strip->colors.n-1 : 0)]->t;
				if (rr) {
					c1 += 1e-4;
					c0 = c1+clen*chunk;
					clen += clen*chunk;
				} else {
					c1 += 1e-4;
					c0 = c1-clen*chunk;
					clen += clen*chunk;
				}
			}

			fprintf(f,"    <radialGradient  id=\"radialGradient%ld\"\n", grad->object_id);
			fprintf(f,"        cx=\"%f\"\n", opp2.x);
			fprintf(f,"        cy=\"%f\"\n", opp2.y);
			fprintf(f,"        fx=\"%f\"\n", opp1.x); //**** wrong!!
			if (r1!=0) cout <<"*** need to fix placement of fx in svg out for radial gradients"<<endl;
			fprintf(f,"        fy=\"%f\"\n", opp1.y);
			fprintf(f,"        r=\"%f\"\n", r2);
			fprintf(f,"        spreadMethod=\"%s\"\n", spreadMethod);
			fprintf(f,"        gradientUnits=\"userSpaceOnUse\">\n");

			for (int c=(r1==0?0:-2); c<grad->strip->colors.n; c++) {
				if (rr && c>=0) cc=grad->strip->colors.n-1-c; else cc=c;
				if (cc==-2) fprintf(f,"      <stop offset=\"0\" stop-color=\"#ffffff\" stop-opacity=\"0\" />\n");
				else if (cc==-1) fprintf(f,"      <stop offset=\"%f\" stop-color=\"#ffffff\" stop-opacity=\"0\" />\n",
											fabs(c1-c0)/clen); //offset
				else fprintf(f,"      <stop offset=\"%f\" stop-color=\"#%02x%02x%02x\" stop-opacity=\"%f\" />\n",
								fabs(grad->strip->colors.e[cc]->t - c0)/clen, //offset
								grad->strip->colors.e[cc]->color->screen.red>>8, //color
								grad->strip->colors.e[cc]->color->screen.green>>8, 
								grad->strip->colors.e[cc]->color->screen.blue>>8, 
								grad->strip->colors.e[cc]->color->screen.alpha/65535.); //opacity
			}
			fprintf(f,"    </radialGradient>\n");

		} else {
			fprintf(f,"    <linearGradient  id=\"linearGradient%ld\"\n", grad->object_id);
			fprintf(f,"        x1=\"%f\"\n", grad->transformPoint(grad->P1()).x);
			fprintf(f,"        y1=\"%f\"\n", grad->transformPoint(grad->P1()).y);
			fprintf(f,"        x2=\"%f\"\n", grad->transformPoint(grad->P2()).x);
			fprintf(f,"        y2=\"%f\"\n", grad->transformPoint(grad->P2()).y);
			fprintf(f,"        spreadMethod=\"%s\"\n", spreadMethod);
			fprintf(f,"        gradientUnits=\"userSpaceOnUse\">\n");
			double clen=grad->strip->colors.e[grad->strip->colors.n-1]->t-grad->strip->colors.e[0]->t;
			for (int c=0; c<grad->strip->colors.n; c++) {
				fprintf(f,"      <stop offset=\"%f\" stop-color=\"#%02x%02x%02x\" stop-opacity=\"%f\" />\n",
								(grad->strip->colors.e[c]->t-grad->strip->colors.e[0]->t)/clen, //offset
								grad->strip->colors.e[c]->color->screen.red>>8, //color
								grad->strip->colors.e[c]->color->screen.green>>8, 
								grad->strip->colors.e[c]->color->screen.blue>>8, 
								grad->strip->colors.e[c]->color->screen.alpha/65535.); //opacity
			}
			fprintf(f,"    </linearGradient>\n");
		}

	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		ColorPatchData *patch=dynamic_cast<ColorPatchData *>(obj);
		if (!patch) return 0;

		if (!out->use_mesh) {
			 //insert mask for patch

			 //get outline of patch, insert a bezier path object of it
			int n=2*(patch->xsize-1) + 2*(patch->ysize-1);
			flatpoint points[n];
			patch->bezOfPatch(points, 0,patch->ysize/3, 0,patch->xsize/3);
			fprintf(f,"    <clipPath id=\"colorPatchMask%ld\" >\n", patch->object_id);
			fprintf(f,"      <path d=\"");
			fprintf(f,"M %f %f ",points[1].x,points[1].y);
			for (int c=2; c<n-1; c+=3) {
				fprintf(f,"C %f %f %f %f %f %f ",
						points[c  ].x,points[c  ].y,
						points[c+1].x,points[c+1].y,
						points[c+2].x,points[c+2].y);
			}
			fprintf(f,"C %f %f %f %f %f %f ",
						points[n-1].x,points[n-1].y,
						points[0  ].x,points[0  ].y,
						points[1  ].x,points[1  ].y);
			fprintf(f,"z\" />\n");

			fprintf(f,"    </clipPath>\n");

			 //insert blur filter
			fprintf(f,"    <filter id=\"patchBlur%ld\" \n"
					  "       filterUnits=\"objectBoundingBox\"\n", patch->object_id);
			fprintf(f,"       x=\"0\"\n"
					  "       y=\"0\"\n"
					  "       width=\"1\"\n"
					  "       height=\"1\">\n"); 

			fprintf(f,"      <feGaussianBlur  stdDeviation=\".5\"/>\n");
			//fprintf(f,"      <feGaussianBlur in=\"SourceAlpha\" stdDeviation=\".5\"/>\n");

			fprintf(f,"    </filter>\n");

		} else { //use_mesh for meshgradient fills
			const char *spc = "";

			fprintf(f, "%s    <meshgradient gradientUnits=\"userSpaceOnUse\" id=\"ColorPatchFill%ld\" "
							" gradientTransform=\"matrix(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)\" x=\"%.10g\" y=\"%.10g\">\n",
					spc, patch->object_id,
					patch->m(0), patch->m(1), patch->m(2), patch->m(3), patch->m(4), patch->m(5),
					patch->points[0].x, patch->points[0].y); 

			int xsize = patch->xsize;
			int i, ci;
			for (int y=0; y<patch->ysize-1; y+=3) {
				fprintf(f, "%s      <meshrow>\n", spc);

				for (int x=0; x<patch->xsize-1; x+=3) {
					fprintf(f, "%s        <meshpatch>\n", spc);
					i = y*patch->xsize + x;
					ci = (y/3)*(patch->xsize/3 + 1) + (x/3);

					 //top curve
					if (y==0 && x==0) svgMeshPathOut(f, spc, patch, ci, i+1, i+2, i+3);
					else if (y==0)    svgMeshPathOut(f, spc, patch, -1, i+1, i+2, i+3);

					 //right curve
					if (y==0) svgMeshPathOut(f, spc, patch, ci+1, i+3+xsize, i+3+2*xsize, i+3+3*xsize);
					else      svgMeshPathOut(f, spc, patch, -1,   i+3+xsize, i+3+2*xsize, i+3+3*xsize);

					 //bottom curve
					svgMeshPathOut(f, spc, patch, ci+1+(xsize/3+1), i+2+3*xsize, i+1+3*xsize, i+3*xsize);

					 //left curve
					if (x==0) svgMeshPathOut(f, spc, patch, ci+(xsize/3+1), i+2*xsize, i+xsize, i);

					fprintf(f, "%s        </meshpatch>\n", spc);
				}
				fprintf(f, "%s      </meshrow>\n", spc);
			}

			fprintf(f, "%s    </meshgradient>\n", spc);
		}
	}

	//check for kids no matter what type it is
	if (g) {
		for (int c=0; c<g->n(); c++) 
			svgdumpdef(f,nullptr,g->e(c),warning,log, out);
	}


	return 0;
}


DocumentExportConfig *SvgOutputFilter::CreateConfig(DocumentExportConfig *fromconfig)
{
	SvgExportConfig* conf = new SvgExportConfig(fromconfig);
	conf->filter = this;
	return conf;
}

//! Save the document as SVG.
/*! This only saves images, groups, linear and radial gradients, and the page size and orientation.
 * Files are not checked for existence. They are clobbered if they already exist, and are writable.
 *
 * Return 0 for success, 1 for error and nothing written, 2 for error, and corrupted file possibly written.
 * 2 is mainly for debugging purposes, and will be perhaps be removed in the future.
 *
 * \todo *** should have option of rasterizing or approximating the things not supported in svg, such 
 *    as patch gradients
 */
int SvgOutputFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *dout=dynamic_cast<DocumentExportConfig *>(context);
	if (!dout) return 1;

	SvgExportConfig *out = dynamic_cast<SvgExportConfig *>(context);
	
	if (!out) {
		out = new SvgExportConfig(dout);
	} else out->inc_count();

	Document *doc = out->doc;
	int start     = out->range.Start(); //we assume there's only one, so rest of range if any doesn't matter
	int layout    = out->layout;
	Group *limbo  = out->limbo;
	PaperGroup *papergroup = out->papergroup;
	if (!filename) filename = out->filename;
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		out->dec_count();
		return 1;
	}
	
	 //we must be able to open the export file location...
	FILE *f = nullptr;
	char *file = nullptr;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			out->dec_count();
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".svg");
	} else file = newstr(filename);

	f = open_file_for_writing(file,0,&log);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		log.AddError(0,0,0, _("%s cannot be opened for writing"), file);
		delete[] file;
		out->dec_count();
		return 3;
	}

	setlocale(LC_ALL,"C");

	int warning = 0;
	Spread *spread = nullptr;
	Group *g = nullptr;
	double m[6], mm[6], mmm[6];
	int c2,pg,c3;
	transform_set(m,1,0,0,1,0,0);

	if (doc) spread = doc->imposition->Layout(layout,start);
	if (!papergroup) papergroup = spread->papergroup;
	
	 // write out header
	double height = 0, width = 0;
	double paper_m[6];
	transform_identity(paper_m);
	if (papergroup) {
		transform_invert(paper_m, papergroup->papers.e[0]->m());
		height = papergroup->papers.e[0]->box->paperstyle->h(); //takes into account landscape status
		width  = papergroup->papers.e[0]->box->paperstyle->w();
	} else if (spread) {
		transform_set(paper_m, 1,0,0,1, -spread->path->minx, -spread->path->miny);
		height = spread->path->maxy - spread->path->miny;
		width  = spread->path->maxx - spread->path->minx;
	} else if (limbo) {
		transform_invert(paper_m, limbo->m());
		width  = limbo->boxwidth();
		height = limbo->boxheight();
	}


	if (out->curpaperrotation == 90 || out->curpaperrotation == 270) {
		double tt = height;
		height = width;
		width  = tt;
	}

	 //write out header, sodipodi is extra stuff for assumed use in Inkscape
	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	fprintf(f,"<!-- Created with Laidout, http://www.laidout.org -->\n");
	fprintf(f,"<svg \n"
			  "     xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
			  "     xmlns=\"http://www.w3.org/2000/svg\"\n"
			  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
			  "     xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n"
			  "     xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
			  "     version=\"1.0\"\n");
	fprintf(f,"     width=\"%fin\"\n", width); //***inches by default?
	// fprintf(f,"     height=\"%fin\"\n", height);
	fprintf(f,"     height=\"%fin\"\n" //was auto?!
	          "     viewBox=\"0 0 %f %f\"\n", height, width*96, height*96);
	fprintf(f,"   >\n");

	 //set default units for use later in Inkscape
	char *units=nullptr;
	GetUnitManager()->UnitInfoId(laidout->prefs.default_units, nullptr, &units,nullptr,nullptr,nullptr);
	if (units) {
		fprintf(f,"  <sodipodi:namedview\n"
				  "      id=\"base\"\n"
				  "      inkscape:document-units=\"%s\"\n"
				  "      units=\"%s\"\n"
				  "  />\n",
				 units, units);
	}
			

	 //----write out global defs section
	 //   ..gradients and such
	fprintf(f,"  <defs>\n");

	 //dump out defs for limbo objects if any
	if (limbo && limbo->n()) {
		svgdumpdef(f,m,limbo,warning,log, out);
	}

	if (papergroup && papergroup->objs.n()) {
		svgdumpdef(f,m,&papergroup->objs,warning,log, out);
	}


	if (spread) {
		if (spread->marks) svgdumpdef(f,m,spread->marks,warning,log, out);

		 // for each page in spread..
		for (c2=0; c2<spread->pagestack.n(); c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			Page *page = doc->pages.e[pg];

			if (page->pagestyle->Flag(PAGE_CLIPS) || layout == PAPERLAYOUT) {
				PathsData *clippath = dynamic_cast<PathsData*>(spread->pagestack.e[c2]->outline);
				if (clippath) {
					char clipstr[100];
					sprintf(clipstr, "pageClip%lu", page->object_id);
					DumpClipPath(f, clipstr, clippath, nullptr, warning, log, out);
				}
			}

			//include copies of objects bleeding from other adjacent pages
			if (page->pagebleeds.n && (layout == PAPERLAYOUT || layout == SINGLELAYOUT)) {
				for (int pb=0; pb<page->pagebleeds.n; pb++) {
					PageBleed *bleed = page->pagebleeds[pb];
					if (bleed->index < 0 || bleed->index >= doc->pages.n) continue;
					Page *otherpage = doc->pages[bleed->index];
					if (!otherpage || !otherpage->HasObjects()) continue;


					// *** bleeds should be optimized to only have to deal with acually bleeding objects, not all objs
					 // for each layer on the page..
					for (int l=0; l<otherpage->layers.n(); l++) {
						 // for each object in layer
						g=dynamic_cast<Group *>(otherpage->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							transform_copy(m,spread->pagestack.e[c2]->outline->m());
							transform_mult(mm, bleed->matrix, m);
							svgdumpdef(f,mm,g->e(c3),warning,log, out);
						}
					}
				}
			}

			 // for each layer on the page..
			for (int l=0; l<page->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(page->layers.e(l));
				for (c3=0; c3<g->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					svgdumpdef(f,m,g->e(c3),warning,log, out);
				}
			}
		}
	}
	fprintf(f,"  </defs>\n");
			
	
	 //----Write out objects....
	double PPINCH = DEFAULT_PPINCH;
	PPINCH = out->pixels_per_inch;

	 //transform to paper * conversion to left handed system, and 1/96th of an inch per unit
	transform_set(m,PPINCH,0,0,-PPINCH, 0,0);
	if (out->curpaperrotation>0) {
		transform_identity(mm);
		transform_rotate(mm, out->curpaperrotation*M_PI/180.);
		transform_mult(mmm,m,mm);
		transform_copy(m,mmm);
	}

	if (out->curpaperrotation==0) { m[5]=height*PPINCH; }
	else if (out->curpaperrotation==90) { m[4]=width*PPINCH; m[5]=height*PPINCH; }
	else if (out->curpaperrotation==180) { m[4]=width*PPINCH; }
	else if (out->curpaperrotation==270) { }

	//fprintf(f,"  <g transform=\"matrix(90,0,0,-90, 0,%f)\">\n", height*72*1.25);
	fprintf(f,"    <g id=\"paper_rotation\" transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 

	// paper transform
	fprintf(f,"    <g id=\"paper\" transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					paper_m[0], paper_m[1], paper_m[2], paper_m[3], paper_m[4], paper_m[5]);


	 //dump out limbo objects if any
	if (limbo && limbo->n()) {
		transform_set(m,1,0,0,1,0,0);
		svgdumpobj(f,m,limbo,warning,4,log, out, false, out->data_meta);
	}

	if (papergroup && papergroup->objs.n()) {
		transform_set(m,1,0,0,1,0,0);
		svgdumpobj(f,m,&papergroup->objs,warning,4,log, out, false, out->data_meta);
	}



	if (spread) {
		 //write out printer marks
		transform_set(m,1,0,0,1,0,0);
		if (spread->marks) svgdumpobj(f,m,spread->marks,warning,4,log, out, false, out->data_meta);

		 // for each page in spread..
		char clipstr[100];
		for (c2=0; c2<spread->pagestack.n(); c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			Page *page = doc->pages.e[pg];

			//set up page clipping if necessary
			if (page->pagestyle->Flag(PAGE_CLIPS) || out->layout == PAPERLAYOUT) {
				sprintf(clipstr, "clip-path=\"url(#pageClip%lu)\"", page->object_id);
			} else {
				clipstr[0] = '\0';
			}


			//page transform
			transform_copy(mm,spread->pagestack.e[c2]->outline->m());
			fprintf(f,"    <g %s transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
				clipstr, mm[0], mm[1], mm[2], mm[3], mm[4], mm[5]); 

			//include copies of objects bleeding from other adjacent pages
			if (page->pagebleeds.n && (layout == PAPERLAYOUT || layout == SINGLELAYOUT)) {
				for (int pb=0; pb<page->pagebleeds.n; pb++) {
					PageBleed *bleed = page->pagebleeds[pb];
					Page *otherpage = doc->pages[bleed->index];
					if (!otherpage->HasObjects()) continue;

					fprintf(f,"    <g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\"><!--page object bleed-->\n ",
						bleed->matrix[0], bleed->matrix[1], bleed->matrix[2], bleed->matrix[3], bleed->matrix[4], bleed->matrix[5]); 

					// *** bleeds should be optimized to only have to deal with acually bleeding objects, not all objs
					 // for each layer on the page..
					for (int l=0; l<otherpage->layers.n(); l++) {
						 // for each object in layer
						g=dynamic_cast<Group *>(otherpage->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							svgdumpobj(f,nullptr,g->e(c3),warning, 8,log, out, false, out->data_meta);
						}
					}

					fprintf(f,"    </g>\n ");
				}
			}

			 // for each layer on the page..
			for (int l=0; l<page->layers.n(); l++) {
				 // for each object in layer
				g = dynamic_cast<Group *>(page->layers.e(l));

				for (c3=0; c3<g->n(); c3++) {
					svgdumpobj(f,nullptr,g->e(c3),warning,6,log, out, false, out->data_meta);
				}
			}

			 //end page transform
			fprintf(f,"    </g>\n ");
		}

		delete spread;
	}

	fprintf(f,"  </g>\n"); //paper transform
	fprintf(f,"  </g>\n"); //from unit correction and paper


	 // write out footer
	fprintf(f,"</svg>\n");
	
	fclose(f);
	setlocale(LC_ALL,"");
	out->dec_count();
	return 0;
	
}

//------------------------------------ SvgImportFilter ----------------------------------
/*! \class SvgImportFilter
 * \brief Filter to, amazingly enough, import svg files.
 */


const char *SvgImportFilter::VersionName()
{
	return _("Svg 1.1");
}

const char *SvgImportFilter::FileType(const char *first100bytes)
{
	if (!strstr(first100bytes,"<svg")) return nullptr;
	return "1.1";

	//*** inkscape has inkscape:version tag
	// also xmlns:svg="http://www.w3.org/2000/svg
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *SvgImportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef = stylemanager.FindDef("SvgImportConfig");
	if (styledef) return styledef; 

	styledef = makeObjectDef();
	makestr(styledef->name,"SvgImportConfig");
	makestr(styledef->Name,_("Svg Import Configuration"));
	makestr(styledef->description,_("Configuration to import a Svg file."));
	styledef->newfunc=newSvgImportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}


GridGuide *ParseGrid(Attribute *def)
{
	char *name,*value;
	double d;
	GridGuide *grid = new GridGuide;

	for (int c=0; c<def->attributes.n; c++) {
		name  = def->attributes.e[c]->name;
		value = def->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			grid->Id(value);

		} else if (!strcmp(name,"type")) {
			if (value && !strcmp(value, "xygrid")) {
				grid->gridtype = GridGuide::Grid;
			}

		} else if (!strcmp(name,"originx")) {
			DoubleAttribute(value, &grid->offset.x, nullptr);

		} else if (!strcmp(name,"originy")) {
			DoubleAttribute(value, &grid->offset.y, nullptr);

		} else if (!strcmp(name,"spacingx")) {
			DoubleAttribute(value, &grid->xspacing, nullptr);

		} else if (!strcmp(name,"spacingy")) {
			DoubleAttribute(value, &grid->yspacing, nullptr);

		} else if (!strcmp(name,"color")) {
			SimpleColorAttribute(value, nullptr, &grid->majorcolor, nullptr);

		} else if (!strcmp(name,"opacity")) {
			if (DoubleAttribute(value, &d, nullptr))
				grid->majorcolor.Alpha(d);

		} else if (!strcmp(name,"empcolor")) {
			SimpleColorAttribute(value, nullptr, &grid->minorcolor, nullptr);

		} else if (!strcmp(name,"empopacity")) {
			if (DoubleAttribute(value, &d, nullptr))
				grid->minorcolor.Alpha(d);

		} else if (!strcmp(name,"empspacing")) {
			IntAttribute(value, &grid->majorinterval, nullptr);

		} else if (!strcmp(name,"visible")) {
			grid->visible = BooleanAttribute(value);

		} else if (!strcmp(name,"enabled")) {
			grid->enabled = BooleanAttribute(value);

		} else if (!strcmp(name,"units")) {
			grid->SetUnits(value);

		} else if (!strcmp(name,"snapvisiblegridlinesonly")) {
			grid->snaponlytovisible = BooleanAttribute(value);
		}
	}

	return grid;
}


//forward declarations:
int svgDumpInObjects(int top,Group *group, Attribute *element,
					 PtrStack<Attribute> &powerstrokes, RefPtrStack<anObject> &gradients,
					 ErrorLog &log, const char *filedir, double docwidth,double docheight);
GradientData *svgDumpInGradientDef(Attribute *att, Attribute *defs, RefPtrStack<anObject> &gradients, int depth);
ColorPatchData *svgDumpInMeshGradientDef(Attribute *att, Attribute *defs);
void CompoundTransformForRef(SomeDataRef *ref, Group *top);
void CompoundTransforms(Group *group, Group *top);


/*! Parse a sodipodi:namedview block, and create a Singles imposition from it.
 */
Imposition *ParseInkscapeMultipage(Attribute *namedview, double *viewbox, double scalex, double scaley)
{
	if (!namedview) return nullptr;
	Attribute *content = namedview->find("sodipodi:namedview");
	if (!content) content = namedview;
	content = content->find("content:");
	if (!content) return nullptr;

	PaperGroup *papergroup = nullptr;
	//Polyhedron *hedron = nullptr;
	int pages_found = 0;
	double m[6];
	const char *name, *value;
	//transform_set(m, 1.0/DEFAULT_PPINCH, 0,0, 1.0/DEFAULT_PPINCH, 0,0);
	transform_identity(m);

	for (int c=0; c<content->attributes.n; c++) {
		name  = content->attributes.e[c]->name;
		value = content->attributes.e[c]->value;

		if (!strcmp(name,"inkscape:page")) {
			double x=0, y=0, width=0, height=0;
			const char *id = nullptr;

			for (int c2=0; c2<content->attributes.e[c]->attributes.n; c2++) {
				name  = content->attributes.e[c]->attributes.e[c2]->name;
				value = content->attributes.e[c]->attributes.e[c2]->value;


				if (!strcmp(name,"x")) {
					DoubleAttribute(value, &x);
				} else if (!strcmp(name,"y")) {
					DoubleAttribute(value, &y);
				} else if (!strcmp(name,"width")) {
					DoubleAttribute(value, &width);
				} else if (!strcmp(name,"height")) {
					DoubleAttribute(value, &height);
				} else if (!strcmp(name,"id")) {					
					id = value;
				}
			}

			if (width > 0 && height > 0) {
				//add page
				
				y = viewbox[3] - y - height;
				x *= scalex;
				y *= scaley;
				width *= scalex;
				height *= scaley;
				
				//if (!hedron) hedron = new Polyhedron();
				//int pt1 = hedron->AddPoint(x,y);
				//int pt2 = hedron->AddPoint(x+width,y);
				//int pt3 = hedron->AddPoint(x+width,y+height);
				//int pt4 = hedron->AddPoint(x,y+height);
				//hedron->AddFace(4, pt1, pt2, pt3, pt4);

				if (!papergroup) papergroup = new PaperGroup();
				//m[4] = x / DEFAULT_PPINCH;
				//m[5] = y / DEFAULT_PPINCH;
				m[4] = x;
				m[5] = y;
				papergroup->AddPaper(id, width, height, m);

				pages_found++;
			}
		}
	}

	//---- hedron based:
	//if (!hedron) return nullptr;
	//hedron->makeedges();	 
	//Net *net = new Net;
	//makestr(net->netname, poly->name);
	//net->basenet = hedron;
	//net->TotalUnwrap();
	//net->rebuildLines();
	//NetImposition *netimp = new NetImposition(hedron);

	//---- papergroup based:
	if (!papergroup) return nullptr;
	Singles *imp = new Singles(papergroup, true);
	//NetImposition *netimp = new NetImposition(papergroup);
	//papergroup->dec_count();

	imp->NumPages(pages_found);
	return imp;
}

int SvgImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log, const char *filecontents,int contentslen)
{
	ImportConfig *in = dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	Document *doc = in->doc;

	Attribute att;
	if (!XMLFileToAttribute(&att,file,nullptr)) return 2;
	
	 //create repository for hints if necessary
	Attribute *svghints = nullptr,  // anything outside "svg" element, plus all "svg" attributes
	          *svg      = nullptr;  // points to the "svg" lax attribute of svghints. Do not delete!!
	// if (in->keepmystery) svghints=new Attribute(VersionName(),file);  ***disable svghints for now

	char *filedir = lax_dirname(file, 1);

	try {

		 //add xml preamble, and anything not under "svg" to hints if it exists...
		if (svghints) {
			for (int c=0; c<att.attributes.n; c++) {
				if (!strcmp(att.attributes.e[c]->name,"svg")) continue;
				svghints->push(att.attributes.e[c]->duplicate(),-1);
			}
			svg = new Attribute("svg", nullptr);
			svghints->push(svg,-1);
		}

		int c;
		char *name,*value;
		double width = 0, height = 0;
		double scalex = 1, scaley = 1;

		Attribute *svgdoc = att.find("svg");
		if (!svgdoc) {
			log.AddError(_("Could not find svg tag."));
			throw 3;
		}

		 // parse main "svg" attributes
		double viewbox[4];
		viewbox[0] = -1;
		Attribute *aatt = svgdoc->find("viewBox");
		if (aatt) {
			int n = DoubleListAttribute(aatt->value, viewbox, 4, nullptr);
			if (n != 4) viewbox[0] = -1;
			else if (fabs(viewbox[2] - viewbox[0]) < 1e-8) {
				log.AddError(_("Bad viewbox!"));
				throw 8;
			}
		}

		 // check width and height first since viewBox depends on them.
		aatt = svgdoc->find("width");
		if (aatt) {
			char *endptr = nullptr;
			DoubleAttribute(aatt->value, &width, &endptr);
			if (*endptr) {
				 //parse units 
				UnitManager *unitm = GetUnitManager();
				while (isspace(*endptr)) endptr++;
				const char *ptr = endptr;
				while (isalpha(*endptr)) endptr++;
				int units = unitm->UnitId(ptr, endptr-ptr);
				double raw_width = width;
				if (units != UNITS_None) width = unitm->Convert(width, units, UNITS_Inches, nullptr);
				scalex = width / raw_width;

			} else {
				width /= DEFAULT_PPINCH; //no specified units, assume svg pts
				scalex = 1./DEFAULT_PPINCH;
			}
		}
		aatt = svgdoc->find("height");
		if (aatt) {
			if (!strcmp_safe(aatt->value, "auto")) {
				if (viewbox[0] != -1) {
					height = width * (viewbox[3] - viewbox[1]) / (viewbox[2] - viewbox[0]);
				} else height = width;

			} else {
				char *endptr = nullptr;
				DoubleAttribute(aatt->value, &height, &endptr);
				if (*endptr) {
					 //parse units 
					UnitManager *unitm = GetUnitManager();
					while (isspace(*endptr)) endptr++;
					const char *ptr = endptr;
					while (isalpha(*endptr)) endptr++;
					int units = unitm->UnitId(ptr, endptr-ptr);
					double raw_height = height;
					if (units != UNITS_None) height = unitm->Convert(height, units, UNITS_Inches, nullptr);
					scaley = height / raw_height;

				} else {
					height /= DEFAULT_PPINCH; //no specified units, assume svg pts
					scaley = 1./DEFAULT_PPINCH;
				}
			}
		}

		for (c = 0; c < svgdoc->attributes.n; c++) {
			name  = svgdoc->attributes.e[c]->name;
			value = svgdoc->attributes.e[c]->value;
			if (!strcmp(name, "content:")) continue;

			if (svghints) svg->push(svgdoc->attributes.e[c]->duplicate(),-1);
		}

		// now svgdoc's subattributes should be a combination of 
		// defs, sodipodi:namedview, metadata
		//  PLUS any number of graphic elements, such as g, rect, image, text....

		if (viewbox[0] != -1) {
			scalex = width  / viewbox[2];
			scaley = height / viewbox[3];
		}

		if (scalex <= 0 || scaley <= 0) {
			log.AddError(_("Bad dimensions!")); 
			throw 3;
		}

		svgdoc = svgdoc->find("content:");
		if (!svgdoc) {
			log.AddError(_("Empty svg tag!")); 
			throw 4;
		}

		Attribute *namedview = svgdoc->find("sodipodi:namedview");
		bool parsing_multipage = false; //only gets true when we are reading in whole brand new doc
		
		 //create a new document if necessary
		if ((!doc || (doc && !doc->imposition)) && !in->toobj) {
			Imposition *imp = nullptr;
			int num_pages_needed = 1;
			int docpagenum = in->topage; //the page in laidout doc to start dumping into
			
			if (width == 0 || height == 0) { //mmm, this shouldn't happen? width or height == 0 caught above?
				 //use default paper size, if no width or height found
				PaperStyle *pp = laidout->GetDefaultPaper();
				width  = pp->w();
				height = pp->h();

			}

			// for multipage, make sure doc has enough pages
			if (namedview) {
				Imposition *multipage_imp = ParseInkscapeMultipage(namedview, viewbox, scalex, scaley);
				if (multipage_imp) {
					imp = multipage_imp;
					parsing_multipage = true;

					if (docpagenum < 0) docpagenum = 0;
					num_pages_needed = imp->papergroup->papers.n;
				}
			}

			if (imp == nullptr) {
				// we seem to not be multipage, so figure out the paper size and
				// orientation for ordinary single page document
				PaperStyle *paper =nullptr;
				int landscape = 0;

				 //svg/inkscape uses width and height, but not paper names as far as I can see
				 //search for paper size known to laidout within certain approximation
				for (c=0; c<laidout->papersizes.n; c++) {
					if (     fabs(width  - laidout->papersizes.e[c]->width)  < .0001
						  && fabs(height - laidout->papersizes.e[c]->height) < .0001) {
						paper=laidout->papersizes.e[c];
						break;
					}
					if (     fabs(height - laidout->papersizes.e[c]->width)  < .0001
						  && fabs(width  - laidout->papersizes.e[c]->height) < .0001) {
						paper=laidout->papersizes.e[c];
						landscape=1;
						break;
					}
				}
				if (paper) paper=dynamic_cast<PaperStyle*>(paper->duplicate());
				else {
					paper = new PaperStyle(_("Custom"), width,height, 0, 300, "in");
				}

				imp = new Singles;
				paper->landscape(landscape);
				imp->SetPaperSize(paper);
				paper->dec_count();
			}

			if (!doc) doc = new Document(imp, Untitled_name());
			else if (!doc->imposition) doc->ReImpose(imp, 0);
			imp->dec_count();

			if (docpagenum + num_pages_needed >= doc->pages.n) {
				doc->NewPages(-1, (docpagenum+num_pages_needed)-doc->pages.n);
			}
		} //if (!doc && !in->toobj)

		Group *group = in->toobj;

		if (!group && doc) {
			 //document page to start dumping onto
			int docpagenum = in->topage; //the page in laidout doc to start dumping into
			int curdocpage; //the current page in the laidout document, used in loop below
			if (docpagenum < 0) docpagenum = 0;

			 //update group to point to the document page's group
			curdocpage = docpagenum;
			if (curdocpage >= doc->pages.n) {
				doc->NewPages(-1, (curdocpage+1)-doc->pages.n);
			}
			group = dynamic_cast<Group *>(doc->pages.e[curdocpage]->layers.e(0));  // pick layer 0 of the page
		}

		RefPtrStack<anObject> gradients;
		PtrStack<Attribute> powerstrokes;

		if (namedview) {
			for (c=0; c<namedview->attributes.n; c++) {
				name  = namedview->attributes.e[c]->name;
				value = namedview->attributes.e[c]->value;

				if (!strcmp(name, "units")) {
					if (doc) doc->properties.push("doc_units", value);

				} else if (!strcmp(name, "inkscape:document-units")) {
					if (doc) doc->properties.push("view_units", value);

				//} else if (!strcmp(name, "content:")) {
				//	for (c2=0; c2<namedview->attributes.e[c]->attributes.n; c2++) {
				//		name  =   namedview->attributes.e[c]->attributes.e[c2]->name;
				//		value =   namedview->attributes.e[c]->attributes.e[c2]->value;
				//
				//		if (!strcmp(name,"inkscape:grid")) {
				//			//extract inkscape:grid, a child of namedview
				//			GridGuide *grid = ParseGrid(vatt->attributes.e[c]);
				//			if (grid) document->guides.push(grid);
				//			
				//		} else if (!strcmp(name,"inkscape:page")) {
				//		   *** will need to check for object overlap with defined pages in order to parse
				//		}
				//	}
				}
			}
		}

		 //first check for document level things like gradients in defs or metadata.
		 //then check for drawable things
		 //then push any other stuff unchanged
		for (c=0; c<svgdoc->attributes.n; c++) {
			name  = svgdoc->attributes.e[c]->name;
			value = svgdoc->attributes.e[c]->value;

			if (!strcmp(name,"metadata")
				     || !strcmp(name,"sodipodi:namedview")) {
				 //just copy over to svghints:
				 //  "metadata"
				 //  "sodipodi:namedview"
				if (svghints) {
					svg->push(svgdoc->attributes.e[c]->duplicate(),-1);
				}

				continue;

			} else if (!strcmp(name,"defs")) {
				 //need to read in gradient and filter data...
				Attribute *defsatt = svgdoc->attributes.e[c]->find("content:");
				Attribute *def;
				if (!defsatt || !defsatt->attributes.n) continue;

				for (int c2=0; c2<defsatt->attributes.n; c2++) {
					def   = defsatt->attributes.e[c2];
					name  = def->name;
					value = def->value;

					if (!strcmp(name,"linearGradient") || !strcmp(name,"radialGradient")) {
						svgDumpInGradientDef(def, defsatt, gradients, 0);

					} else if (!strcmp(name,"meshgradient")) {
						ColorPatchData *mesh = svgDumpInMeshGradientDef(def, defsatt);
						if (mesh) {
							gradients.push(mesh);
							mesh->dec_count();
						}

					} else if (!strcmp(name,"inkscape:path-effect")) {
						Attribute *power=def->find("effect");
						if (power && power->value && !strcmp(power->value,"powerstroke")) {
							powerstrokes.push(def,0);
						}

					} else if (!strcmp(name,"clipPath")) {
					//} else if (!strcmp(name,"mask")) {
					//} else if (!strcmp(name,"filter")) {
					//} else if (!strcmp(name,"font")) {
					//} else if (!strcmp(name,"font-face")) {
					//} else if (!strcmp(name,"image")) {
					//} else if (!strcmp(name,"pattern")) {
					//} else if (!strcmp(name,"text")) {
					//} else if (!strcmp(name,"a")) {
					//} else if (!strcmp(name,"altGlyphDef")) {
					//} else if (!strcmp(name,"color-profile")) {
					//} else if (!strcmp(name,"cursor")) {
					//} else if (!strcmp(name,"foreignObject")) {
					//} else if (!strcmp(name,"marker")) {
					//} else if (!strcmp(name,"script")) {
					//} else if (!strcmp(name,"style")) {
					//} else if (!strcmp(name,"switch")) {
					//} else if (!strcmp(name,"view")) {
						//svg2ish:
					//} else if (!strcmp(name,"solidcolor")) {
					}
				}
				continue;

			} 
			
			// read in any drawable svg objects
			int oldn = group->n();
			if (svgDumpInObjects(1,group,svgdoc->attributes.e[c],powerstrokes,gradients,log, filedir, width/scalex,height/scaley)) {
				DrawableObject *obj;
				SomeData test;
				Affine a;
				Affine aa;

				for (int c = group->n()-1; c >= oldn; c--) {
					obj = dynamic_cast<DrawableObject*>(group->e(c));
					if (scalex != 1 || scaley != 1) {
						//obj->Scale(flatpoint(0,0), 1./DEFAULT_PPINCH);
						obj->Scale(scalex, scaley);

						obj->m(5, height-obj->m(5)); //flip in page
						obj->m(2, -obj->m(2));
						obj->m(3, -obj->m(3));
					}

					if (parsing_multipage) { //we need to figure out what page the object needs to be on
						a = obj->GetTransformToContext(false, 0);
						
						for (int c2 = 1; c2 < doc->imposition->papergroup->papers.n; c2++) {
							PaperBoxData *paperdata = doc->imposition->papergroup->papers.e[c2];
							test.setbounds(paperdata);
							aa = paperdata->Inversion();
							aa.PreMultiply(a);
							if (test.intersect(aa.m(), obj, true, false)) {
								cout << "premult change svg object "<<obj->Id()<<" to paper "<<c2<<endl;
							}

							aa = paperdata->Inversion();
							aa.Multiply(a);
							if (test.intersect(aa.m(), obj, true, false)) {
								cout << "mult change svg object "<<obj->Id()<<" to paper "<<c2<<endl;
							}
						}
					}
				}

				continue;
			}

			 //push any other blocks into svghints.. not expected, but you never know
			if (svghints) {
				Attribute *more = new Attribute("docContent",nullptr);
				more->push(svgdoc->attributes.e[c]->duplicate(),-1);
				svghints->push(more,-1);
			}
		} // each svgdoc attribute
		

		// install global hints if they exist
		if (svghints) {
			 //remove the old iohint if it is there
			Attribute *iohints=(doc?&doc->iohints:&laidout->project->iohints);
			Attribute *oldsvg=iohints->find(VersionName());
			if (oldsvg) {
				iohints->attributes.remove(iohints->attributes.findindex(oldsvg));
			}
			iohints->push(svghints,-1);
			//remember, do not delete svghints here! they become part of the doc/project
		}

		// if doc is new, push into the project
		if (doc && doc != in->doc) {
			laidout->project->Push(doc);
			laidout->app->addwindow(newHeadWindow(doc));
		}

		//fix clone transform complications
		CompoundTransforms(group, group);
		//laidout->project->ClarifyRefs(log);
	
	} catch (int error) {
		if (svghints) delete svghints;
		delete[] filedir;
		return 1;
	}

	delete[] filedir;
	return 0;
}


/*! Overcome linking issues for nested refs, by multiplying the transforms of any nested referencing.
 * This is necessary because Laidout SomeDataRef overwrites referenced object transforms, but SVG
 * "use" objects multiplie them all together.
 */
void CompoundTransformForRef(SomeDataRef *ref, Group *top)
{
	SomeData *found = top->FindObject(ref->thedata_id);
	if (!found) return;

	SomeDataRef *foundref = dynamic_cast<SomeDataRef*>(found);
	if (foundref) {
		if (!foundref->thedata) {
			CompoundTransformForRef(foundref, top);
		}
		if (!foundref->thedata) {
			DBG cerr <<" *** WARNING! broken ref links!! No link for: "<<foundref->thedata_id<<endl;
			return;
		}
	}

	ref->Set(found, 1);
	ref->PreMultiply(found->m());
}


/*! SVG use elements compound transforms, while Laidout SomeDataRef supercedes, so we need to step
 * through and compond where necessary.
 */
void CompoundTransforms(Group *group, Group *top)
{
	for (int c=0; c<group->n(); c++) {
		SomeDataRef *ref = dynamic_cast<SomeDataRef*>(group->e(c));
		if (ref && !ref->thedata && ref->thedata_id) {
			CompoundTransformForRef(ref, top);
		}

		DrawableObject *dobj = dynamic_cast<DrawableObject*>(group->e(c));
		if (dobj && dobj->n()) {
			CompoundTransforms(dobj, top);
		}
	}
}


/*! If gradient, then apply anything in the def to that existing gradient.
 * gradient will also be returned in this case, or nullptr for error.
 */
GradientData *svgDumpInGradientDef(Attribute *def, Attribute *defs, RefPtrStack<anObject> &gradients, int depth)
{
	if (!def) return nullptr;

	int type = (!strcmp(def->name, "linearGradient") ? GradientData::GRADIENT_LINEAR : GradientData::GRADIENT_RADIAL);

	const char *id = def->findValue("id");
	if (id && depth == 0) {  //check to see if we have already processed this gradient
		for (int c=0; c<gradients.n; c++) {
			if (!strcmp(gradients.e[c]->Id(), id)) return dynamic_cast<GradientData*>(gradients.e[c]);
		}
	}

	double      cx, cy, fx, fy, r;
	flatpoint   p1, p2;
	int         spreadMethod = LAXSPREAD_Pad;
	bool        foundf = false;
	bool foundp1 = false;
	bool foundp2 = false;
	GradientStrip *strip = nullptr;
	// int units=0;//0 is user space, 1 is bounding box

	double gm[6];
	transform_identity(gm);
	char *name, *value;
	GradientData *gradient = nullptr;

	const char *xlink = def->findValue("xlink:href");
	if (!isblank(xlink) && xlink[0] == '#') {
		 // the link might contain the color spots, need to scan in the ref for them.
		 // For instance, radialGradient often links to a linearGradient in inkscape docs

		const char *xlinkid = xlink+1;

		for (int c=0; c<defs->attributes.n; c++) {
			Attribute *d = defs->attributes.e[c];
			name = d->name;
			if (strcmp(name,"linearGradient") && strcmp(name,"radialGradient"))
				continue;

			const char *did = d->findValue("id");
			if (did && !strcmp(did, xlinkid)) {
				gradient = svgDumpInGradientDef(d, defs, gradients, depth+1);
				break;
			}
		}
	}

	for (int c3=0; c3<def->attributes.n; c3++) {
		name  = def->attributes.e[c3]->name;
		value = def->attributes.e[c3]->value;

		if (!strcmp(name,"id")) {
			id = value;

		} else if (!strcmp(name,"gradientUnits")) {
			 //gradientUnits = "userSpaceOnUse | objectBoundingBox"
			//if (!strcmp(value,"userSpaceOnUse")) units=0;
			//else if (!strcmp(value,"objectBoundingBox")) units=1;
			DBG cerr <<" warning: ignoring gradientUnits on svg gradient In"<<endl;

		} else if (!strcmp(name,"gradientTransform")) {
			svgtransform(value,gm);

		} else if (!strcmp(name,"spreadMethod")) {
			//pad | repeat | reflect
			if (value) {
				if (!strcmp(value, "pad")) {
					spreadMethod = LAXSPREAD_Pad;
				} else if (!strcmp(value, "repeat")) {
					spreadMethod = LAXSPREAD_Repeat;
				} else if (!strcmp(value, "reflect")) {
					spreadMethod = LAXSPREAD_Reflect;
				}
			}

		} else if (!strcmp(name,"cx")) {
			DoubleAttribute(value,&cx,nullptr);
			foundp1 = true;

		} else if (!strcmp(name,"cy")) {
			DoubleAttribute(value,&cy,nullptr);
			foundp1 = true;

		} else if (!strcmp(name,"fx")) {
			DoubleAttribute(value,&fx,nullptr);
			foundf=1;

		} else if (!strcmp(name,"fy")) {
			DoubleAttribute(value,&fy,nullptr);
			foundf=1;

		} else if (!strcmp(name,"r")) {
			DoubleAttribute(value,&r,nullptr);

		} else if (!strcmp(name,"x1")) {
			DoubleAttribute(value,&p1.x,nullptr);
			foundp1 = true;

		} else if (!strcmp(name,"y1")) {
			DoubleAttribute(value,&p1.y,nullptr);
			foundp1 = true;

		} else if (!strcmp(name,"x2")) {
			DoubleAttribute(value,&p2.x,nullptr);
			foundp2 = true;

		} else if (!strcmp(name,"y2")) {
			DoubleAttribute(value,&p2.y,nullptr);
			foundp2 = true;

		} else if (!strcmp(name,"content:")) {
			Attribute *spot=def->attributes.e[c3];
			double offset = 0, opacity = 0;
			bool found_opacity = false;
			ScreenColor color;

			for (int c4=0; c4<spot->attributes.n; c4++) {
				name =spot->attributes.e[c4]->name;
				value=spot->attributes.e[c4]->value;

				if (!strcmp(name,"stop")) {
					for (int c5=0; c5<spot->attributes.e[c4]->attributes.n; c5++) {
						name =spot->attributes.e[c4]->attributes.e[c5]->name;
						value=spot->attributes.e[c4]->attributes.e[c5]->value;

						if (!strcmp(name,"style")) {
							 //style="stop-color:#ffffff;stop-opacity:1;"
							char *str = strstr(value,"stop-color:");
							if (str) {
								str += 11;
								SimpleColorAttribute(str, nullptr, &color, nullptr);
								//HexColorAttributeRGB(str,&color,nullptr);
							}
							str=strstr(value,"stop-opacity:");
							if (str) {
								str += 13;
								DoubleAttribute(str, &opacity, nullptr);
								found_opacity = true;
							}

						} else if (!strcmp(name,"offset")) {
							char *endptr = nullptr;
							if (DoubleAttribute(value,&offset,&endptr)) {
								if (endptr && *endptr == '%') offset /= 100;
								offset = 1-offset;
							}

						} else if (!strcmp(name,"stop-color")) {
							SimpleColorAttribute(value, nullptr, &color, nullptr);
							//HexColorAttributeRGB(value,&color,nullptr);

						} else if (!strcmp(name,"stop-opacity")) {
							DoubleAttribute(value,&opacity,nullptr);
							found_opacity = true;

						} else if (!strcmp(name,"id")) {
							//ignore stop id
						}
					}

					if (found_opacity) color.Alpha(opacity);
					if (!strip) strip = new GradientStrip();
					strip->AddColor(offset,&color);
				}
			}
		}
	}

	if (!gradient) gradient = dynamic_cast<GradientData *>(newObject("GradientData"));

	if (type == GradientData::GRADIENT_RADIAL) {
		//p1 = transform_point(gm, flatpoint(cx, cy));
		//p2 = (foundf ? transform_point(gm, flatpoint(fx, fy)) : p1);
		p1 = flatpoint(cx, cy);
		p2 = (foundf ? flatpoint(fx, fy) : p1);
		if (foundp1 || foundf) {
			gradient->SetRadial(p1, p2, r, 0);
		} else gradient->SetRadial();
	} else {
		if (!foundp2) p2 = p1;
		if (foundp1 || foundf) {
			//p1 = transform_point(gm, p1);
			//p2 = transform_point(gm, p2);
			r = (p1-p2).norm();
			gradient->SetLinear(p2, p1, r, r); //for some reason reversed
		} else gradient->SetLinear();
	}
	gradient->spread_method = spreadMethod;
	gradient->fill_parent = true;
	gradient->m(gm);
	if (strip) {
		strip->Id(id);
		gradient->Set(strip, 1, true, true);
	}
	if (!isblank(id)) gradient->Id(id);
	
	DBG cerr << "svg gradient def: "<<endl;
	DBG gradient->dump_out(stderr, 2, 0, nullptr);

	if (depth == 0) {
		gradients.push(gradient);
		gradient->dec_count();
	}

	return gradient;
}


#define TOP    0
#define RIGHT  1
#define BOTTOM 2
#define LEFT   3

ColorPatchData *svgDumpInMeshGradientDef(Attribute *def, Attribute *defs)
{
	ColorPatchData *mesh = dynamic_cast<ColorPatchData *>(newObject("ColorPatchData"));
	mesh->Set(0,0,1,1,1,1,PATCH_SMOOTH);

	flatpoint p1,p2;
	if (!def) return nullptr;
	// bool visible = true;
	// bool locked = false;
	char *name, *value;
	//int units=0;//0 is user space, 1 is bounding box
	double gm[6];
	transform_identity(gm);

	flatpoint initial;

	try {
	  for (int c=0; c<def->attributes.n; c++) {
		name =def->attributes.e[c]->name;
		value=def->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) mesh->Id(value);

		} else if (!strcmp(name,"gradientUnits")) {
			 //gradientUnits = "userSpaceOnUse | objectBoundingBox"
			if (!strcmp(value,"userSpaceOnUse"))  ; //the default
			else if (!strcmp(value,"objectBoundingBox")) mesh->flags |= PATCH_Units_BBox;

		} else if (!strcmp(name,"gradientTransform")) {
			svgtransform(value,gm);

		} else if (!strcmp(name,"x")) {
			DoubleAttribute(value,&initial.x,nullptr);

		} else if (!strcmp(name,"y")) {
			DoubleAttribute(value,&initial.y,nullptr);

		} else if (!strcmp(name,"content:")) {
			Attribute *rows = def->attributes.e[c];

			char command = 0;
			ScreenColor color;
			double pts[6];
			int rowi = -1, coli = -1, xsize = 4;
			Affine tr;
			flatpoint v, lastp;

			mesh->points[0] = initial;

			//Order is: traverse clockwise from upper left, skip any edges already taken
			//  1  2  3  4
			//  5  6  7  8
			//  9 10 11 12

			for (int c2=0; c2<rows->attributes.n; c2++) {
				name  = rows->attributes.e[c2]->name;
				value = rows->attributes.e[c2]->value;

				if (strcmp(name,"meshrow")) continue; //there should only be meshrow children


				Attribute *row = rows->attributes.e[c2]->find("content:");
				if (!row) continue;

				rowi++;
				if (rowi >= mesh->ysize/3) {
					mesh->grow(LAX_TOP, tr.m(), false);
				}
				coli = -1;

				for (int c3=0; c3<row->attributes.n; c3++) {
					name  = row->attributes.e[c3]->name;
					value = row->attributes.e[c3]->value;

					if (!strcmp(name,"meshpatch")) {
						Attribute *patch = row->attributes.e[c3]->find("content:");
						int edge = TOP;

						coli++;
						if (coli >= mesh->xsize/3) {
							mesh->grow(LAX_RIGHT, tr.m(), false);
							xsize = mesh->xsize;
						}

						int pi = 3 * rowi * mesh->xsize + 3 * coli; //point index, upper left
						int ci = rowi * (mesh->xsize/3+1) + coli; //color index, upper left
						int cic = ci; //where next color should go
						int stopi = -1;
						if (rowi == 0) lastp = mesh->points[pi];
						else lastp = mesh->points[pi + 3];

						if (!patch) continue;
						for (int c4=0; c4<patch->attributes.n; c4++) {
							name  = patch->attributes.e[c4]->name; //note this might be a comment
							value = patch->attributes.e[c4]->value;

							if (strcmp(name, "stop")) continue;

							stopi++;
							edge = stopi;
							if (rowi>0) edge++;

							Attribute *stop = patch->attributes.e[c4];;
							//bool colorfound = false;

							//style has to be processed after path which updates cic, so:
							Attribute *pathatt = stop->find("path");
							if (!pathatt) throw 100; //malformed!

							//paths that are "l x y" "L x y" "c x y x y x y" "C x y x y"(last one can ignore final)
							value = pathatt->value;
							if (!value || !*value) continue;
							while (isspace(*value)) value++;
							command = value[0];
							value++;
							int n = DoubleListAttribute(value, pts, 6);

							if (edge == TOP ) {
								if (command == 'l' || command == 'L') { //relative line
									if (n!=2) throw(1);

									if (command == 'l') v = flatpoint(pts[0],pts[1])/3;
									else v = (flatpoint(pts[0],pts[1]) - lastp)/3;

									mesh->points[pi+1] = lastp + v;
									mesh->points[pi+2] = lastp + 2*v;
									mesh->points[pi+3] = lastp + 3*v;

								} else if (command == 'c') {
									if (n != 6) throw(2);
									mesh->points[pi+1].set(pts[0]+lastp.x, pts[1]+lastp.y); //lastp = mesh->points[pi+1];
									mesh->points[pi+2].set(pts[2]+lastp.x, pts[3]+lastp.y); //lastp = mesh->points[pi+2];
									mesh->points[pi+3].set(pts[4]+lastp.x, pts[5]+lastp.y);

								} else if (command == 'C') {
									if (n != 6) throw(3);
									mesh->points[pi+1].set(pts[0], pts[1]);
									mesh->points[pi+2].set(pts[2], pts[3]);
									mesh->points[pi+3].set(pts[4], pts[5]);
								}

								cic = ci;
								lastp = mesh->points[pi+3];

							} else if (edge == RIGHT) {
								if (command == 'l' || command == 'L') { //relative line
									if (n!=2) throw(4);

									if (command == 'l') v = flatpoint(pts[0],pts[1])/3;
									else v = (flatpoint(pts[0],pts[1]) - lastp)/3;

									mesh->points[pi+3+  xsize] = lastp + v;
									mesh->points[pi+3+2*xsize] = lastp + 2*v;
									mesh->points[pi+3+3*xsize] = lastp + 3*v;

								} else if (command == 'c') {
									if (n != 6) throw(5);
									mesh->points[pi+3+  xsize].set(pts[0]+lastp.x, pts[1]+lastp.y); //lastp = mesh->points[pi+3+  xsize];
									mesh->points[pi+3+2*xsize].set(pts[2]+lastp.x, pts[3]+lastp.y); //lastp = mesh->points[pi+3+2*xsize];
									mesh->points[pi+3+3*xsize].set(pts[4]+lastp.x, pts[5]+lastp.y);

								} else if (command == 'C') {
									if (n != 6) throw(6);
									mesh->points[pi+3+  xsize].set(pts[0], pts[1]);
									mesh->points[pi+3+2*xsize].set(pts[2], pts[3]);
									mesh->points[pi+3+3*xsize].set(pts[4], pts[5]);
								}

								cic = ci + 1;
								lastp = mesh->points[pi+3+3*xsize];

							} else if (edge == BOTTOM) {
								if (command == 'l' || command == 'L') { //relative line
									if (n!=2) throw(7);

									if (command == 'l') v = flatpoint(pts[0],pts[1])/3;
									else v = (flatpoint(pts[0],pts[1]) - lastp)/3;

									mesh->points[pi+2+3*xsize] = lastp + v;
									mesh->points[pi+1+3*xsize] = lastp + 2*v;
									mesh->points[pi+  3*xsize] = lastp + 3*v;

								} else if (command == 'c') {
									if (n != 6 && n != 4) throw(8);
									mesh->points[pi+2+3*xsize].set(pts[0]+lastp.x, pts[1]+lastp.y); //lastp = mesh->points[pi+2+3*xsize];
									mesh->points[pi+1+3*xsize].set(pts[2]+lastp.x, pts[3]+lastp.y); //lastp = mesh->points[pi+1+3*xsize];
									if (n==6) mesh->points[pi+  3*xsize].set(pts[4]+lastp.x, pts[5]+lastp.y);

								} else if (command == 'C') {
									if (n != 6 && n != 4) throw(9);
									mesh->points[pi+2+3*xsize].set(pts[0], pts[1]);
									mesh->points[pi+1+3*xsize].set(pts[2], pts[3]);
									if (n==6) mesh->points[pi+  3*xsize].set(pts[4], pts[5]);
								}

								cic = ci + 1 + (xsize/3+1);
								lastp = mesh->points[pi+3*xsize];

							} else { //LEFT
								if (command == 'l' || command == 'L') { //relative line
									if (n!=2) throw(10);

									if (command == 'l') v = flatpoint(pts[0],pts[1])/3;
									else v = (flatpoint(pts[0],pts[1]) - lastp)/3;

									mesh->points[pi+2*xsize] = lastp + v;
									mesh->points[pi+1*xsize] = lastp + 2*v;
									mesh->points[pi        ] = lastp + 3*v;

								} else if (command == 'c') {
									if (n != 6 && n != 4) throw(11);
									mesh->points[pi+2*xsize].set(pts[0]+lastp.x, pts[1]+lastp.y); //lastp = mesh->points[pi+2*xsize];
									mesh->points[pi+1*xsize].set(pts[2]+lastp.x, pts[3]+lastp.y); //lastp = mesh->points[pi+1*xsize];
									if (n==6) mesh->points[pi].set(pts[4]+lastp.x, pts[5]+lastp.y);

								} else if (command == 'C') {
									if (n != 6 && n != 4) throw(12);
									mesh->points[pi+2*xsize].set(pts[0], pts[1]);
									mesh->points[pi+1*xsize].set(pts[2], pts[3]);
									if (n==6) mesh->points[pi].set(pts[4], pts[5]);
								}

								cic = ci + (xsize/3+1);
								lastp = mesh->points[pi];

							} //end process path

							//now process style:
							for (int c5=0; c5<stop->attributes.n; c5++) {
								name  = stop->attributes.e[c5]->name;
								value = stop->attributes.e[c5]->value;

								if (!strcmp(name,"stop-color")) {
									SimpleColorAttribute(value, nullptr, &color, nullptr);
									mesh->colors[cic] = color;
									//colorfound = true;

								} else if (!strcmp(name,"stop-opacity")) {
									double a;
									DoubleAttribute(value,&a,nullptr);
									color.alpha = mesh->colors[cic].alpha = a*65535;
									//colorfound = true;

								} else if (!strcmp(name,"style")) {
									// *** parse the style string
									Attribute satt;
									NameValueToAttribute(&satt, value, ':', ';');

									for (int c6=0; c6<satt.attributes.n; c6++) {
										name  = satt.attributes.e[c6]->name;
										value = satt.attributes.e[c6]->value;

										if (!strcmp(name, "stop-color")) {
											SimpleColorAttribute(value, nullptr, &color, nullptr);
											mesh->colors[cic] = color;

										} else if (!strcmp(name, "stop-opacity")) {
											double a;
											DoubleAttribute(value,&a,nullptr);
											color.alpha = mesh->colors[cic].alpha = a*65535;
										}
									}
								} 
							} //stop atts

							//mesh->colors[cic] = color; //uses last color if color wasn't found

						} //foreach patchatts
					} //if rowatt is meshpatch
				} //foreach row atts
			} //foreach meshgradient subatt
		} //if "content:"
	  }	

	} catch (int error) {
		  DBG cerr <<"Caught error parsing meshgradient! "<<error<<endl;
		  mesh->dec_count();
		  return nullptr;
	}

	mesh->m(gm);
	mesh->InterpolateControls(Patch_Coons);
	mesh->controls = Patch_Coons;
	mesh->FindBBox();
	mesh->NeedToUpdateCache(0,-1,0,-1);
	mesh->touchContents();
	return mesh;
}


void InsertFillobj(SomeData *fillobj, SomeData *obj, Group *group)
{
	if (!fillobj) return;

	//*** need to push a reference to the def?
	ColorPatchData *mesh = dynamic_cast<ColorPatchData*>(fillobj);
	if (mesh) {
		if ((mesh->flags & PATCH_Units_BBox)) {
			//need to scale to object bbox, 0..1 maps to min,max
			double mm[6];
			transform_set(mm, obj->maxx-obj->minx, 0,0, obj->maxy-obj->miny, obj->minx,obj->miny);
			mesh->Multiply(mm);
		}
	} else {
		GradientData *grad = dynamic_cast<GradientData*>(fillobj);
		if (grad && grad->IsLinear() && (grad->P1()-grad->P2()).norm() < 1e-5) {
			//probably gradient was not supplied with p1 and p2, so arbitrarily insert default to fill parent
			grad->P2(grad->transformPointInverse(obj->BBoxPoint(0,.5,false)));
			grad->P1(grad->transformPointInverse(obj->BBoxPoint(1,.5,false)));
		}
	}

	DrawableObject *ddata = dynamic_cast<DrawableObject*>(obj);
	if (ddata) {
		ddata->push(fillobj);
		ddata->child_clip_type = CLIP_From_Parent_Area;
	} else {
		//can't add as child for some reason.. this should probably be a bug if it happens
		fillobj->m(obj->m());
		group->push(fillobj);
	}
	fillobj->dec_count();
}


//! Return 1 for attribute used, else 0.
/*! If top!=0, then top is the height of the document. We need to flip elements up,
 * since down is positive y in svg. We also need to scale by .8/72 to convert svg units to Laidout units.
 */
int svgDumpInObjects(int top,Group *group, Attribute *element, PtrStack<Attribute> &powerstrokes, RefPtrStack<anObject> &gradients,
		ErrorLog &log, const char *filedir, double docwidth,double docheight)
{
	char *name,*value;
	ValueHash extra;

	if (!strcmp(element->name,"g")) {
		Group *g=new Group;
		for (int c=0; c<element->attributes.n; c++) {
			name=element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) g->Id(value);

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				g->m(m);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) g->Lock(OBJLOCK_Selectable);

			} else if (!strcmp(name,"style")) {
				if (value && strstr(value, "display:none")) {
					g->Visible(false);
				}

			} else if (!strcmp(name,"content:")) {
				for (int c2=0; c2<element->attributes.e[c]->attributes.n; c2++) 
					svgDumpInObjects(0,g,element->attributes.e[c]->attributes.e[c2],powerstrokes,gradients,log, filedir, docwidth,docheight);
			}
		}

		//if (top) {
		//	for (int c=0; c<6; c++) g->m(c,g->m(c)/DEFAULT_PPINCH); //correct for svg scaling
        //
		//	g->m(5,top-g->m(5)); //flip in page
		//	g->m(2, -g->m(2));
		//	g->m(3, -g->m(3));
		//}

		 //do not add empty groups
		if (g->n()!=0) {
			g->FindBBox();
			group->push(g);
		}
		g->dec_count();
		return 1;

	} else if (!strcmp(element->name,"image")) {
		ImageData *image=dynamic_cast<ImageData *>(newObject("ImageData"));
		int foundcoord=0;
		double x=0,y=0,w=0,h=0;
		int err=0;

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) image->Id(value);

			} else if (!strcmp(name,"x")) {
				foundcoord|=1;
				DoubleAttribute(value,&x,nullptr);

			} else if (!strcmp(name,"y")) {
				foundcoord|=2;
				DoubleAttribute(value,&y,nullptr);

			} else if (!strcmp(name,"width")) {
				foundcoord|=4;
				DoubleAttribute(value,&w,nullptr);
				if (strstr(value, "%")) w = w/100*docwidth;

			} else if (!strcmp(name,"height")) {
				foundcoord|=8;
				DoubleAttribute(value,&h,nullptr);
				if (strstr(value, "%")) h = h/100*docheight;

			} else if (!strcmp(name,"xlink:href")) {
				char *file = newstr(filedir);
				appendstr(file, value);
				err = image->LoadImage(file);
				delete[] file;
				if (err) break;

			} else if (!strcmp(name,"transform")) {
				double m[6];
				transform_identity(m);
				svgtransform(value,m);
				if (!is_degenerate_transform(m)) image->m(m);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) image->Lock(OBJLOCK_Selectable);

			} else if (!strcmp(name,"style")) {
				if (value && strstr(value, "display:none")) {
					image->Visible(false);
				}
			}
		}

		if (err==0) {
			DoubleBBox bbox(x,x+w, y,y+h);
			image->fitto(nullptr, &bbox, 50, 50);

			image->Flip(0);
			group->push(image);
		} //else error loading image

		image->dec_count();
		return 1;

	} else if (!strcmp(element->name,"path")) {
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));
		int d_index=-1;
		Attribute *powerstroke = nullptr;
		SomeData *fillobj = nullptr;

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				StyleToFillAndStroke(value, paths, gradients, &fillobj, &extra);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) paths->Lock(OBJLOCK_Selectable);

			} else if (!strcmp(name,"inkscape:path-effect")) {
				//path-effect is something like: "#path-effect3338;#path-effect3343"
				//we currently can only process powerstroke..

				while (value && *value) {
					if (*value=='#') value++;

					char *endptr=lax_strchrnul(value, ';');
					const char *id;

					for (int c2=0; c2<powerstrokes.n; c2++) {
						id = powerstrokes.e[c2]->findValue("id");
						if (!id) continue;
						if (!strncmp(value, id, endptr - value)) {
							powerstroke = powerstrokes.e[c2];
							break;
						}
					}
					if (powerstroke) break;
					value=endptr;
					if (*value==';') value++;
				}

			//} else if (!strcmp(name,"d")) {
				//d_index=c;

			//} else if (!strcmp(name,"x")) {
			//} else if (!strcmp(name,"y")) {
			}
		}

		if (powerstroke) element->find("inkscape:original-d", &d_index);
		if (d_index == -1) element->find("d", &d_index);
		if (d_index >= 0) {
			SvgToPathsData(paths, element->attributes.e[d_index]->value, nullptr, powerstroke);
		}

		if (powerstroke && paths->NumPaths() > 0) {
			paths->paths.e[0]->linestyle->function = LAXOP_Over;
			paths->fillstyle->function = LAXOP_None;

			int    start_linecap = LAXCAP_Round;
			int    end_linecap   = 0;
			double scale_width   = 1;

			const char *visible = powerstroke->findValue("is_visible");

			if (BooleanAttribute(visible)) {
				for (int c=0; c < powerstroke->attributes.n; c++) {
					name  = powerstroke->attributes.e[c]->name;
					value = powerstroke->attributes.e[c]->value;

					if (!strcmp(name, "start_linecap_type")) {
						if      (!strcasecmp(value, "round"))     start_linecap = LAXCAP_Round;
						else if (!strcasecmp(value, "peak"))      start_linecap = LAXCAP_Peak;
						else if (!strcasecmp(value, "butt"))      start_linecap = LAXCAP_Butt;
						else if (!strcasecmp(value, "square"))    start_linecap = LAXCAP_Square;
						else if (!strcasecmp(value, "zerowidth")) start_linecap = LAXCAP_Zero_Width;
						paths->linestyle->capstyle = start_linecap;

					} else if (!strcmp(name, "end_linecap_type")) {
						if      (!strcasecmp(value, "round"))     end_linecap = LAXCAP_Round;
						else if (!strcasecmp(value, "peak"))      end_linecap = LAXCAP_Peak;
						else if (!strcasecmp(value, "butt"))      end_linecap = LAXCAP_Butt;
						else if (!strcasecmp(value, "square"))    end_linecap = LAXCAP_Square;
						else if (!strcasecmp(value, "zerowidth")) end_linecap = LAXCAP_Zero_Width;
						paths->linestyle->endcapstyle = end_linecap;

					} else if (!strcmp(name, "scale_width")) {
						DoubleAttribute(value, &scale_width);

					} else if (!strcmp(name, "offset_points")) {
						//offset_points="0,0.19207847 | 0.98943653,0.55041926 | 1.5510075,0.25217076" <- t, width
						const char *p = value;
						char *end_ptr = nullptr;
						double d[2];
						while (p && *p) {
							int n = DoubleListAttribute(p, d, 2, &end_ptr);
							if (n != 2) break;
							paths->paths.e[0]->AddWeightNode(d[0], 0, 2 * scale_width * d[1], 0);
							p = end_ptr;
							while (isspace(*p) || *p == '|') p++;
						}
						
					//linejoin_type="round" //beveled, extrapolated arc, mitered, rounded, spiro
					//lpeversion="1"
					//miter_limit="4"
					//interpolator_beta="0.2"
					//interpolator_type="CentripetalCatmullRom"
					//sort_points="true"
					//id="path-effect2802"
					//effect="powerstroke" />
					}
				}
			}

		}

		if (paths->paths.n) { //only add non-empty paths
			const char *disp = extra.findString("display");
			if (disp && !strcmp(disp, "none")) {
				paths->Visible(false);
			}

			paths->FindBBox();
			int err = 0;
			double alpha = extra.findIntOrDouble("opacity", -1, &err);
			if (err == 0) {
				DrawableObject *ddata = dynamic_cast<DrawableObject*>(paths);
				if (ddata) ddata->opacity = alpha;
			}
			group->push(paths);

			if (fillobj) InsertFillobj(fillobj, paths, group);
		}
		paths->dec_count();

		return 1;

	} else if (!strcmp(element->name,"rect")) {
		double x=0,y=0,w=0,h=0;
		double rx=-1, ry=-1;
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));
		SomeData *fillobj = nullptr;

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);
			} else if (!strcmp(name,"rx")) {
				DoubleAttribute(value,&rx,nullptr);
			} else if (!strcmp(name,"ry")) {
				DoubleAttribute(value,&ry,nullptr);
			} else if (!strcmp(name,"x")) {
				DoubleAttribute(value,&x,nullptr);
			} else if (!strcmp(name,"y")) {
				DoubleAttribute(value,&y,nullptr);
			} else if (!strcmp(name,"width")) {
				DoubleAttribute(value,&w,nullptr);
				if (strstr(value, "%")) w = w/100*docwidth; //bit of a hack in lieu of units parsing here
			} else if (!strcmp(name,"height")) {
				DoubleAttribute(value,&h,nullptr);
				if (strstr(value, "%")) h = h/100*docheight;

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				StyleToFillAndStroke(value, paths, gradients, &fillobj, &extra);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) paths->Lock(OBJLOCK_Selectable);
			}
		}

		 //rx and ry are the x and y radii of an ellipse at the corners
		if (rx<0) rx=ry;
		if (ry<0) ry=rx;
		if (rx>w/2) rx=w/2;
		if (ry>h/2) ry=h/2;

		 //put a possible rounded rectangle in x,y,w,h
		if (w>0 && h>0) {
			if (rx>0 && ry>0) {
				double yb=ry-ry*4/3*(sqrt(2)-1), //based on length of bez handle for circle approximation
					   xb=rx-rx*4/3*(sqrt(2)-1);
				paths->append(x+xb,y,POINT_TONEXT);
				paths->append(x+rx,y  );
				paths->append(x+w-rx,y  );
				paths->append(x+w-xb,y,   POINT_TOPREV);
				paths->append(x+w,      y+yb,POINT_TONEXT);
				paths->append(x+w       ,y+ry);
				//paths->append(x+w       ,y+h/2);
				paths->append(x+w       ,y+h-ry);
				paths->append(x+w       ,y+h-yb,POINT_TOPREV);
				paths->append(x+w-xb    ,y+h,POINT_TONEXT);
				paths->append(x+w-rx    ,y+h);
				paths->append(x+rx      ,y+h);
				paths->append(x+xb      ,y+h, POINT_TOPREV);
				paths->append(x         ,y+h-yb, POINT_TONEXT);
				paths->append(x         ,y+h-ry);
				paths->append(x         ,y+ry);
				paths->append(x         ,y+yb, POINT_TOPREV);
				paths->close();

			} else {
				paths->append(x  ,y  );
				paths->append(x+w,y  );
				paths->append(x+w,y+h);
				paths->append(x  ,y+h);
				paths->close();
			}
		}

		if (paths->paths.n) { //only add non-empty paths
			const char *disp = extra.findString("display");
			if (disp && !strcmp(disp, "none")) {
				paths->Visible(false);
			}

			paths->FindBBox();
			group->push(paths);

			if (fillobj) InsertFillobj(fillobj, paths, group);
		}
		paths->dec_count();

		return 1;

	} else if (!strcmp(element->name,"circle") || !strcmp(element->name,"ellipse")) {
		 //using 4 vertices as bez points, the vector length is 4*(sqrt(2)-1)/3 = about .5523 with radius 1

		double cx=0,cy=0,r=-1,rx=-1, ry=-1;
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));
		SomeData *fillobj = nullptr;

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);
			} else if (!strcmp(name,"rx")) { //present in ellipses
				DoubleAttribute(value,&rx,nullptr);
			} else if (!strcmp(name,"ry")) { //present in ellipses 
				DoubleAttribute(value,&ry,nullptr);
			} else if (!strcmp(name,"r")) { //present in circles
				DoubleAttribute(value,&r,nullptr);
			} else if (!strcmp(name,"cx")) {
				DoubleAttribute(value,&cx,nullptr);
			} else if (!strcmp(name,"cy")) {
				DoubleAttribute(value,&cy,nullptr);

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				StyleToFillAndStroke(value, paths, gradients, &fillobj, &extra);
			
			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) paths->Lock(OBJLOCK_Selectable);
			}
		}

		 //rx and ry are the x and y radii of an ellipse at the corners
		if (r>0 || (rx>0 && ry>0)) {
			if (rx<0) { rx=r; ry=r; }

			double yv=ry*4/3*(sqrt(2)-1), //based on length of bez handle for circle approximation
				   xv=rx*4/3*(sqrt(2)-1);
			paths->append(cx-xv, cy+ry, POINT_TONEXT);
			paths->append(cx   , cy+ry);
			paths->append(cx+xv, cy+ry, POINT_TOPREV);
			paths->append(cx+rx, cy+yv, POINT_TONEXT);
			paths->append(cx+rx, cy  );
			paths->append(cx+rx, cy-yv, POINT_TOPREV);
			paths->append(cx+xv, cy-ry, POINT_TONEXT);
			paths->append(cx   , cy-ry);
			paths->append(cx-xv, cy-ry, POINT_TOPREV);
			paths->append(cx-rx, cy-yv, POINT_TONEXT);
			paths->append(cx-rx, cy  );
			paths->append(cx-rx, cy+yv, POINT_TOPREV);
			paths->close();

		}

		if (paths->paths.n) { //only add non-empty paths
			const char *disp = extra.findString("display");
			if (disp && !strcmp(disp, "none")) {
				paths->Visible(false);
			}

			paths->FindBBox();
			group->push(paths);

			if (fillobj) InsertFillobj(fillobj, paths, group);
		}

		paths->dec_count();
		return 1;

	} else if (!strcmp(element->name,"line")) {
		double x1=0,y1=0, x2=0,y2=0;
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));
		SomeData *fillobj = nullptr;

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);

			} else if (!strcmp(name,"x1")) {
				DoubleAttribute(value,&x1,nullptr);

			} else if (!strcmp(name,"x2")) {
				DoubleAttribute(value,&x2,nullptr);

			} else if (!strcmp(name,"y1")) {
				DoubleAttribute(value,&y1,nullptr);

			} else if (!strcmp(name,"y2")) {
				DoubleAttribute(value,&y2,nullptr);

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				StyleToFillAndStroke(value, paths, gradients, &fillobj, &extra);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) paths->Lock(OBJLOCK_Selectable);
			}
		}

		paths->append(x1,y1);
		paths->append(x2,y2);

		if (paths->paths.n) { //only add non-empty paths
			const char *disp = extra.findString("display");
			if (disp && !strcmp(disp, "none")) {
				paths->Visible(false);
			}
			paths->FindBBox();
			group->push(paths);

			if (fillobj) InsertFillobj(fillobj, paths, group);
		}

		paths->dec_count();
		return 1;

	} else if (!strcmp(element->name,"polyline") || !strcmp(element->name,"polygon")) {
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));
		SomeData *fillobj = nullptr;

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);

			} else if (!strcmp(name,"points")) {
				int n = 0;
				double *pts = nullptr;
				if (DoubleListAttribute(value, &pts, &n)) {
					for (int c=0; c<n-1; c+=2) {
						paths->append(pts[c],pts[c+1]);
					}
				}

				if (!strcmp(element->name,"polygon")) paths->close();

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				StyleToFillAndStroke(value, paths, gradients, &fillobj, &extra);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) paths->Lock(OBJLOCK_Selectable);

			}
		}

		if (paths->paths.n) { //only add non-empty paths
			const char *disp = extra.findString("display");
			if (disp && !strcmp(disp, "none")) {
				paths->Visible(false);
			}

			paths->FindBBox();
			group->push(paths);

			if (fillobj) InsertFillobj(fillobj, paths, group);
		}
		paths->dec_count();
		return 1;


	} else if (!strcmp(element->name,"text")) {

		CaptionData *textobj = dynamic_cast<CaptionData *>(newObject("CaptionData"));

		char *name;
		char *value;
		double x=0, y=0;
		double m[6];
		double font_size = -1;
		const char *font_family = nullptr, *font_style = nullptr;
		Attribute styleatt;
		transform_identity(m);

		for (int c=0; c<element->attributes.n; c++) {
			name  = element->attributes.e[c]->name;
			value = element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				textobj->Id(value);

			} else if (!strcmp(name,"x")) {
				DoubleAttribute(value, &x);

			} else if (!strcmp(name,"y")) {
				DoubleAttribute(value, &y);

			} else if (!strcmp(name,"transform")) {
				svgtransform(value,m);
				textobj->m(m);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				bool locked = BooleanAttribute(value);
				if (locked) textobj->Lock(OBJLOCK_Selectable);

			} else if (!strcmp(name,"style")) {
				InlineCSSToAttribute(value, &styleatt);
				//int foundfill = 0;

				for (int c2=0; c2<styleatt.attributes.n; c2++) {
					name  = styleatt.attributes.e[c2]->name;
					value = styleatt.attributes.e[c2]->value;

					if (!strcmp(name, "line-height")) {
						DoubleAttribute(value, &textobj->linespacing); // *** need to compute units!!!

					} else if (!strcmp(name, "font-family")) {
						font_family = value;

					} else if (!strcmp(name, "font-style")) {
						font_style = value;

					} else if (!strcmp(name, "font-variant")) {
						//font_variant = value;

					} else if (!strcmp(name, "font-size")) {
						//font-size:medium|xx-small|x-small|small|large|x-large|xx-large|smaller|larger|length|initial|inherit;
						DoubleAttribute(value, &font_size);

					} else if (!strcmp(name, "text-align")) {
						//left | right | center | justify | inherit
						if (!strcasecmp(value, "left")) textobj->xcentering = 0;
						else if (!strcasecmp(value, "center")) textobj->xcentering = 50;
						else if (!strcasecmp(value, "right")) textobj->xcentering = 100;
						else if (!strcasecmp(value, "start")) textobj->xcentering = 0;
						else if (!strcasecmp(value, "end")) textobj->xcentering = 100;

					} else if (!strcmp(name, "fill-opacity")) {
						double a;
						if (DoubleAttribute(value, &a)) {
							textobj->alpha = a;
						}

					} else if (!strcmp(name, "fill")) {
						double fillcolor[4];
						if (value && !strcmp(value,"none")) {
							//foundfill = -1;

						} else {
							//d = fillcolor[3];
							SimpleColorAttribute(value, fillcolor, nullptr);
							//if (foundfill==2) fillcolor[3]=d; //opacity was found first, but SCA overwrites
							//foundfill=1;
							textobj->red   = fillcolor[0];
							textobj->green = fillcolor[1];
							textobj->blue  = fillcolor[2];
						}

					} else if (!strcmp(name, "display")) {
						if (value && !strcmp(value, "none")) textobj->Visible(false);
					}
				}

			} else if (!strcmp(name,"content:")) {
				int nl=0,pos=0;

				for (int c2=0; c2<element->attributes.e[c]->attributes.n; c2++) {
					name  = element->attributes.e[c]->attributes.e[c2]->name;
					value = element->attributes.e[c]->attributes.e[c2]->value;

					if (!strcmp(name, "tspan")) {
						Attribute *att = element->attributes.e[c]->attributes.e[c2]->find("content:");
						if (att && !isblank(att->value)) textobj->InsertString(att->value,-1, 0, -1, &nl,&pos);

						for (int c3=0; c3<element->attributes.e[c]->attributes.e[c2]->attributes.n; c3++) {
							name  = element->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->name;
							value = element->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->value;

							if (!strcmp(name, "style")) {
								Attribute spanstyle;
								ValueHash extra;
								InlineCSSToAttribute(value, &spanstyle);

								for (int c4=0; c4<spanstyle.attributes.n; c4++) {
									name  = spanstyle.attributes.e[c4]->name;
									value = spanstyle.attributes.e[c4]->value;

									if (!strcmp(name, "font-size")) {
										DoubleAttribute(value, &font_size); // *** note this is wrong, ignores keywords and units
										if (font_size == 0) font_size = -1;
									}
								}
							}
						}

					} else if (!strcmp(name, "cdata:")) {
						if (!isblank(value)) textobj->InsertString(value,-1, 0, -1, &nl,&pos);

					}
				}
			}
		}

		if (textobj->IsBlank()) {
			textobj->dec_count();
			DBG cerr <<"ignoring blank text object "<<(textobj->Id()?textobj->Id():"unnamed")<<endl;

		} else {
			if (font_size == -1) font_size = 12; //arbitrary number to play nice with CaptionData.
			if (font_family) textobj->Font(nullptr, font_family, font_style, font_size);
			else if (font_size != -1) textobj->Size(font_size);
			textobj->origin(textobj->transformPoint(flatpoint(x,y-font_size)));
			group->push(textobj);
			textobj->dec_count();
		}

		DBG cerr <<" *** need to finish implementing svg in:  text"<<endl;
		return 1;

	} else if (!strcmp(element->name,"use")) {
		 //references to other objects, create SomeDataRef

		double x=0,y=0,w=1,h=1;
		int hasx=0, hasy=0, hasw=0, hash=0;
		const char *link = nullptr;
		const char *id = nullptr;
		double m[6];
		bool selectable = true;
		bool hidden = false;
		transform_identity(m);

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) id = value;

			} else if (!strcmp(name,"x")) {
				hasx = DoubleAttribute(value,&x,nullptr);

			} else if (!strcmp(name,"y")) {
				hasy = DoubleAttribute(value,&y,nullptr);

			} else if (!strcmp(name,"width")) {
				//note: width and height are only supposed to be used when the use element is an svg or a symbol
				hasw = DoubleAttribute(value,&w,nullptr); // *** could be like "100%"

			} else if (!strcmp(name,"height")) {
				hash = DoubleAttribute(value,&h,nullptr);

			} else if (!strcmp(name,"transform")) {
				svgtransform(value,m);

			} else if (!strcmp(name,"sodipodi:insensitive")) {
				selectable = BooleanAttribute(value);

			} else if (!strcmp(name,"style")) {
				if (value && strstr(value, "display:none")) {
					hidden = true;
				}
			} else if (!strcmp(name,"xlink:href") || !strcmp(name,"href")) { //note xlink:href deprecated as of svg2
				if (value && *value=='#') link = value+1; //for now, only use links to objects

			}
		}

		if (link) {
			SomeDataRef *ref = dynamic_cast<SomeDataRef *>(newObject("SomeDataRef"));
			ref->Id(id);
			makestr(ref->thedata_id, link);
			ref->m(m);
			if (!selectable) ref->Lock(OBJLOCK_Selectable);
			if (hidden) ref->Visible(false);
			if (hasx || hasy) {
				// *** additional translate x,y
				cerr << " todo: implement extra x,y translate on svg use x,y"<<endl;
			}
			if (hasw) ref->maxx = w;
			if (hash) ref->maxy = h;

			group->push(ref);
			ref->dec_count();
		}

		return 1;

	} else if (!strcmp(element->name,"symbol")) {
		// ***
		cerr <<"***need to implement svg symbol in!"<<endl;
	}

	return 0;
}


//---------------------------- Helper Functions --------------------------------------------

/*! value should be a simple css like list, as you might find in the "style" part of an svg path.
 *
 * Basically just something like "color:#000000;clip-rule:nonzero;display:inline;".
 * A string of name:value pairs, semicolon separated.
 *
 * It is NOT full css.
 *
 * linestyle and fillstyle must not be nullptr.
 */
int StyleToFillAndStroke(const char *inlinecss, LaxInterfaces::PathsData *paths,
		RefPtrStack<anObject> &gradients, SomeData **fillobj_ret,
		ValueHash *extra)
{
	LineStyle *linestyle = paths->linestyle;
	FillStyle *fillstyle = paths->fillstyle;

	if (!linestyle) {
		linestyle = new LineStyle;
		linestyle->width = 1./96;
		linestyle->function = LAXOP_None;
		paths->InstallLineStyle(linestyle);
		linestyle->dec_count();
	}
	if (!fillstyle) {
		fillstyle = new FillStyle;
		fillstyle->function = LAXOP_None;
		paths->InstallFillStyle(fillstyle);
		fillstyle->dec_count();
	}

	if (!inlinecss) return 1;

	Attribute att;
	InlineCSSToAttribute(inlinecss, &att);

	const char *name, *value;
	double d;
	int founddefcol=0, foundstroke=0, foundfill=0;
	double strokecolor[4], fillcolor[4], defaultcolor[4];
	strokecolor[0] = strokecolor[1] = strokecolor[2] = 0;
	strokecolor[3] = 1.0;
	fillcolor[0] = fillcolor[1] = fillcolor[2] = 0;
	fillcolor[3] = 1.0;

	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;

		if (!strcmp(name,"color")) { //default in absence of fill/stroke colors.. #000000
			if (value && !strcmp(value,"none")) {
				founddefcol=-1;
			} else {
				SimpleColorAttribute(value, defaultcolor, nullptr);
				founddefcol=1;
			}

		} else if (!strcmp(name,"stroke")) { //the stroke color: #021bd9
			if (value && !strcmp(value,"none")) {
				foundstroke=-1;
			} else {
				d=strokecolor[3];
				SimpleColorAttribute(value, strokecolor, nullptr);
				if (foundstroke==2) strokecolor[3]=d;
				foundstroke=1;
			}

		} else if (!strcmp(name,"stroke-width")) { //35.29999924
			if (DoubleAttribute(value, &d)) linestyle->width = d;

		} else if (!strcmp(name,"stroke-opacity")) { //0..1
			if (DoubleAttribute(value, &d)) strokecolor[3]=d;
			if (foundstroke==0) foundstroke=2;

		} else if (!strcmp(name,"fill") || !strcmp(name,"solid-color")) {
			//fill color #ff0000. solid-color is svg2-ish def element
			if (value && !strncmp(value, "url(#", 5)) {
				//"url(#someDefId)"! contains gradients and meshgradient
				char *id = newstr(value+5);
				if (id[strlen(id)-1] == ')') id[strlen(id)-1] = '\0';

				SomeData *fillobj = nullptr;
				for (int c=0; c<gradients.n; c++) {
					if (!strcmp(gradients.e[c]->Id(), id)) {
						fillobj = dynamic_cast<SomeData*>(gradients.e[c]);
						if (fillobj) {
							//DBG cerr << "-----------fillobj pre"<<endl;
							//fillobj->dump_out(stdout, 2, 0, nullptr);
							const char *id = fillobj->Id();
							fillobj = fillobj->duplicate(nullptr);
							fillobj->Id(id);
							fillobj->FindBBox();
							//DBG cerr << "-----------fillobj post"<<endl;
							//fillobj->dump_out(stdout, 2, 0, nullptr);
						}
						break;
					}
				}

				foundfill = -1;

				if (fillobj) *fillobj_ret = fillobj;
				
			} else if (value && !strcmp(value,"none")) {
				foundfill=-1;

			} else {
				d=fillcolor[3];
				SimpleColorAttribute(value, fillcolor, nullptr);
				if (foundfill==2) fillcolor[3]=d; //opacity was found first, but SCA overwrites
				foundfill=1;
			}

		} else if (!strcmp(name,"fill-opacity") || !strcmp(name,"solid-opacity")) { //1
			if (DoubleAttribute(value, &d)) fillcolor[3]=d;
			if (foundfill==0) foundfill=2;

		} else if (!strcmp(name,"fill-rule") && value) { //evenodd | nonzero | inherit
			if (!strcmp(value, "evenodd")) fillstyle->fillrule = LAXFILL_EvenOdd;
			else if (!strcmp(value, "nonzero")) fillstyle->fillrule = LAXFILL_Nonzero;

		} else if (!strcmp(name,"stroke-linecap")) { //butt
			if (!strcmp(value, "butt")) {              linestyle->capstyle = LAXCAP_Butt;
			} else if (!strcmp(value, "round")) {      linestyle->capstyle = LAXCAP_Round;
			} else if (!strcmp(value, "square")) {     linestyle->capstyle = LAXCAP_Square;
			} else if (!strcmp(value, "projecting")) { linestyle->capstyle = LAXCAP_Projecting; //not pure svg?
			//} else if (!strcmp(value, "inherit")) {
			}

		} else if (!strcmp(name,"stroke-linejoin")) { //miter
			if (!strcmp(value, "miter")) {              linestyle->joinstyle = LAXJOIN_Miter;
			} else if (!strcmp(value, "round")) {       linestyle->joinstyle = LAXJOIN_Round;
			} else if (!strcmp(value, "bevel")) {       linestyle->joinstyle = LAXJOIN_Bevel;
			} else if (!strcmp(value, "extrapolate")) { linestyle->joinstyle = LAXJOIN_Extrapolate;
			//} else if (!strcmp(value, "inherit")) {
			}

		} else if (!strcmp(name,"stroke-miterlimit")) { //4
			// *** todo!!
		} else if (!strcmp(name,"stroke-dasharray")) { //none
			// *** todo!!
		} else if (!strcmp(name,"stroke-dashoffset")) { //0
			// *** todo!!
			
		} else if (extra) {

			 //TODo: other stuff found in inkscape svgs:
			//} else if (!strcmp(name,"clip-rule")) { //nonzero
			//} else if (!strcmp(name,"display")) { //inline for ?, none for hidden
			//} else if (!strcmp(name,"overflow")) { //visible
			//} else if (!strcmp(name,"visibility")) { //visible
			//} else if (!strcmp(name,"isolation")) { //auto
			//} else if (!strcmp(name,"mix-blend-mode")) { //normal
			//} else if (!strcmp(name,"color-interpolation")) { //sRGB
			//} else if (!strcmp(name,"color-interpolation-filters")) { //linearRGB
			//} else if (!strcmp(name,"marker")) { //none
			//} else if (!strcmp(name,"color-rendering")) { //auto
			//} else if (!strcmp(name,"image-rendering")) { //auto
			//} else if (!strcmp(name,"shape-rendering")) { //auto
			//} else if (!strcmp(name,"text-rendering")) { //auto
			//} else if (!strcmp(name,"enable-background")) { //accumulate"

			if (!strcmp(name,"opacity")) { //1
				double o = 1.0;
				DoubleAttribute(value, &o);
				extra->push("opacity", new DoubleValue(o), -1, true);
				
			} else if (!strcmp(name,"font-style")) {
				extra->push(name, new StringValue(value), -1, true); //normal italic oblique

			} else if (!strcmp(name,"font-variant")) { // normal | small-caps | inherit
				extra->push(name, new StringValue(value), -1, true);

			} else if (!strcmp(name,"font-weight")) { // normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
				bool relative = false;
				const char *endptr = nullptr;
				int weight = CSSFontWeight(value, &endptr, &relative);
				extra->push(name, new IntValue(weight), -1, true);

			} else if (!strcmp(name,"font-stretch")) {
				// normal = 100% | ultra-condensed 50% | extra-condensed 62.5% | condensed 75% | semi-condensed 87.5% | semi-expanded 112.5% | expanded 125% | extra-expanded 150% | ultra-expanded 200%
				// CSS4 also allows percentages >= 0
				// Only works if font family has width-variant faces
				double stretch = 100;
				if      (!strcmp(value, "normal"))          stretch = 100;
				else if (!strcmp(value, "ultra-condensed")) stretch = 50; 
				else if (!strcmp(value, "extra-condensed")) stretch = 62.5; 
				else if (!strcmp(value, "condensed"))       stretch = 75; 
				else if (!strcmp(value, "semi-condensed"))  stretch = 87.5; 
				else if (!strcmp(value, "semi-expanded"))   stretch = 112.5; 
				else if (!strcmp(value, "expanded"))        stretch = 125; 
				else if (!strcmp(value, "extra-expanded"))  stretch = 150; 
				else if (!strcmp(value, "ultra-expanded"))  stretch = 200;
				extra->push(name, new DoubleValue(stretch), -1, true);

			} else if (!strcmp(name,"line-height")) {
				//number, length, percentage, or "normal"==1.2
				double ems = 1.2;
				if (!strcmp(value, "normal")) ems = 1.2;
				else {
					char *endptr = nullptr;
					if (DoubleAttribute(value, &ems, &endptr)) {
						char *v = endptr;
						while (isspace(*v)) v++;
						if (*v == '%') {
							ems /= 100;
						} else if (*v && !isspace(*v)) {
							 //parse units
							UnitManager *unitm = GetUnitManager();
							int units = unitm->UnitId(v);
							if (units != UNITS_None && units != UNITS_em) {
								ems = unitm->Convert(ems, units, UNITS_em, nullptr);
							}
						}
					}
				}
				extra->push(name, new DoubleValue(ems), -1, true);

			} else if (!strcmp(name,"sodipodi-insensitive")) {
				extra->push(name, new BooleanValue(value), -1, true);

			} else if (!strcmp(name,"display")) {
				extra->push(name, value);

			//ToDO:
			//} else if (!strcmp(name, "text-indent")) {
			//} else if (!strcmp(name, "text-align")) {
			//} else if (!strcmp(name, "text-decoration")) {
			//} else if (!strcmp(name, "text-decoration-line")) {
			//} else if (!strcmp(name, "letter-spacing")) {
			//} else if (!strcmp(name, "word-spacing")) {
			//} else if (!strcmp(name, "text-transform")) {
			//} else if (!strcmp(name, "writing-mode")) {
			//} else if (!strcmp(name, "direction")) {
			//} else if (!strcmp(name, "baseline-shift")) {
			//} else if (!strcmp(name, "text-anchor")) {

			}
		}
	}

	if (founddefcol) {
		 // *** note: fails when opacity but not color specified
		if (!foundstroke) { memcpy(strokecolor, defaultcolor, 4*sizeof(double)); foundstroke = founddefcol; }
		if (!foundfill)   memcpy(fillcolor,   defaultcolor, 4*sizeof(double));
	}

	if (foundstroke) {
		if (foundstroke==-1) linestyle->function = LAXOP_None;
		else {
			linestyle->function = LAXOP_Over;
			linestyle->Colorf(strokecolor[0],strokecolor[1],strokecolor[2],strokecolor[3]);
		}
	}
	if (foundfill) {
		if (foundfill==-1) fillstyle->function = LAXOP_None;
		else {
			fillstyle->function = LAXOP_Over;
			fillstyle->Colorf(fillcolor[0],fillcolor[1],fillcolor[2],fillcolor[3]);
		}
	}


	return 0;
}





} // namespace Laidout

