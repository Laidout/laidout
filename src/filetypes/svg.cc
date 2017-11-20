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

//for some reason compilation fails on some systems without:
#include <lax/refptrstack.cc>

#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../dataobjects/mysterydata.h"
#include "svg.h"
#include "../headwindow.h"
#include "../impositions/singles.h"
#include "../utils.h"
#include "../drawdata.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {


double DEFAULT_PPINCH = 96;
//double DEFAULT_PPINCH = 90;


//-------forward decs for helper funcs
int StyleToFillAndStroke(const char *inlinecss, LaxInterfaces::LineStyle *linestyle, LaxInterfaces::FillStyle *fillstyle);



//------------------------ Svg in/reimpose/out helpers -------------------------------------

//! Creates a Laidout Document from a Svg file, and adds to laidout->project.
/*! Return 0 for success or nonzero for error.
 *
 * If existingdoc!=NULL, then insert the file to that Document. In this case, it is not
 * pushed onto the project, as it is assumed it is elsewhere. Note that this will
 * basically wipe the existing document, and replace with the Svg document.
 */
int addSvgDocument(const char *file, Document *existingdoc)
{
	FILE *f=fopen(file,"r");
	if (!f) return 1;
	char chunk[2000];
	size_t c=fread(chunk,1,1999,f);
	chunk[c]='\0';
	fclose(f);

	 //find default page width and height
	double width,height;
	UnitManager *unitm = GetUnitManager();

	 //width
	char *endptr;
	const char *ptr=strstr(chunk,"width");
	if (!ptr) return 2;
	ptr+=5;
	while (isspace(*ptr) || *ptr=='=') ptr++;
	if (*ptr!='\"') return 3;
	ptr++;
	width=strtod(ptr,&endptr);
	ptr=endptr;
	if (*ptr!='\"') {
		 //need to parse units
		while (isspace(*ptr)) ptr++;
		const char *eptr=ptr;
		while (isalpha(*eptr)) eptr++;
		int units = unitm->UnitId(ptr, eptr-ptr);
		if (units!=UNITS_None) width = unitm->Convert(width, units, UNITS_Inches, NULL);
	} else width /= DEFAULT_PPINCH;
	if (width<=0) return 4;

	 //height
	ptr=strstr(chunk,"height");
	if (!ptr) return 2;
	ptr+=6;
	while (isspace(*ptr) || *ptr=='=') ptr++;
	if (*ptr!='\"') return 5;
	ptr++;
	height=strtod(ptr,&endptr);
	ptr=endptr;
	if (*ptr!='\"') {
		 //need to parse units
		while (isspace(*ptr)) ptr++;
		const char *eptr=ptr;
		while (isalpha(*eptr)) eptr++;
		int units = unitm->UnitId(ptr, eptr-ptr);
		if (units!=UNITS_None) height = unitm->Convert(height, units, UNITS_Inches, NULL);
	} else height /= DEFAULT_PPINCH;
	if (height<=0) return 6;


	PaperStyle paper("custom",width,height, 0,300, "in");
	
	Singles *imp=new Singles;
	imp->SetPaperSize(&paper);
	imp->NumPages(1);

	Document *newdoc=existingdoc;
	if (newdoc) {
		makestr(newdoc->saveas,NULL); //force rename later
		if (newdoc->imposition) newdoc->imposition->dec_count();
		newdoc->imposition=imp;
		imp->inc_count();
	} else {
		newdoc=new Document(imp,NULL);//null file name to force rename on save
	}

	makestr(newdoc->name,"From ");
	appendstr(newdoc->name,file);
	imp->dec_count();

	if (!existingdoc) laidout->project->Push(newdoc);
	else newdoc->inc_count();

	SvgImportFilter filter;
	ImportConfig config(file,300, 0,-1, 0,-1,-1, newdoc,NULL);
	config.keepmystery=0;
	config.filter=&filter;
	ErrorLog log;
	filter.In(file,&config,log);

	newdoc->dec_count();

	return 0;
}


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
 * \brief Holds extra config for export.
 */
class SvgExportConfig : public DocumentExportConfig
{
  public:
	bool use_powerstroke;
	bool use_mesh;
	double pixels_per_inch;

	SvgExportConfig();
	SvgExportConfig(DocumentExportConfig *config);
    virtual ObjectDef* makeObjectDef();
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);
};

/*! Base on config, copy over its stuff.
 * \todo *** this shouldn't really be necessary, right now is just a hack to make export work in a pinch
 */
SvgExportConfig::SvgExportConfig(DocumentExportConfig *config)
	: DocumentExportConfig(config)
{

	SvgExportConfig *svgconf=dynamic_cast<SvgExportConfig*>(config);
	if (svgconf) {
		use_mesh       =svgconf->use_mesh;
		use_powerstroke=svgconf->use_powerstroke;
		pixels_per_inch=svgconf->pixels_per_inch;
	} else {
		use_mesh=false;
		//use_powerstroke=false;
		use_powerstroke = true;
		pixels_per_inch = DEFAULT_PPINCH;
	}
}

//! Set the filter to the Image export filter stored in the laidout object.
SvgExportConfig::SvgExportConfig()
{
	use_mesh = false;
	use_powerstroke = true;
	//use_powerstroke=false;
	pixels_per_inch = DEFAULT_PPINCH;

	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Image")) {
			filter=laidout->exportfilters.e[c];
			break; 
		}
	}
}

Value *SvgExportConfig::dereference(const char *extstring, int len)
{
	if (!strncmp(extstring,"use_mesh",8)) {
		return new BooleanValue(use_mesh);

	} else if (!strncmp(extstring,"use_powerstroke",8)) {
		return new BooleanValue(use_powerstroke);

	} else if (!strncmp(extstring,"pixels_per_inch",8)) {
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
        exportdef=makeExportConfigDef();
		stylemanager.AddObjectDef(exportdef,1);
    }

 
	def=new ObjectDef(exportdef,"SvgExportConfig",
            _("Svg Export Configuration"),
            _("Settings for an SVG filter that exports a document."),
            "class",
            NULL,NULL,
            NULL,
            0, //new flags
            NULL,
            NULL);


     //define parameters
    def->push("use_mesh",
            _("Use Mesh"),
            _("Export color meshes using SVG 2's draft of meshes."),
            "boolean",
            NULL,    //range
            "false", //defvalue
            0,      //flags
            NULL); //newfunc

    def->push("use_powerstroke",
            _("Use powerstroke"),
            _("Export weighted paths using Inkscape's Powerstroke live path effects."),
            "boolean",
            NULL,   //range
            "true", //defvalue
            0,     //flags
            NULL);//newfunc
 
    def->push("pixels_per_inch",
            _("Pixels per inch"),
            _("Pixels per inch"),
            "real",
            NULL,   //range
            "96", //defvalue
            0,     //flags
            NULL);//newfunc
 

	stylemanager.AddObjectDef(def,0);
	return def;
}

void SvgExportConfig::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	DocumentExportConfig::dump_out(f,indent,what,context);

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%suse_mesh %s         #whether to output meshes as svg2 meshes\n",spc,use_mesh?"yes":"no");
		fprintf(f,"%suse_powerstroke %s  #whether to use Inkscape's powerstroke LPE with paths where appropriate\n",spc,use_powerstroke?"yes":"no");
		fprintf(f,"%spixels_per_inch %f  #Pixels per inch. Usually 96 (css's value) is a safe bet.\n",spc,pixels_per_inch);
		return;
	}

	fprintf(f,"%suse_mesh %s\n",spc,use_mesh?"yes":"no");
	fprintf(f,"%suse_powerstroke %s\n",spc,use_powerstroke?"yes":"no");
	fprintf(f,"%spixels_per_inch %.10g\n",spc,pixels_per_inch);
}

void SvgExportConfig::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DocumentExportConfig::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"use_mesh")) use_mesh=BooleanAttribute(value);
		else if (!strcmp(name,"use_powerstroke")) use_powerstroke=BooleanAttribute(value);
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

	styledef=makeObjectDef();
	makestr(styledef->name,"SvgExportConfig");
	makestr(styledef->Name,_("Svg Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to an svg file."));
	styledef->newfunc=newSvgExportConfig;

    styledef->push("usemesh",
            _("Use Mesh"),
            _("Use the format for the mesh branch of Inkscape for mesh gradients."),
            "boolean",
            NULL, //range
            "false",  //defvalue
            0,    //flags
            NULL);//newfunc

    styledef->push("usepowerstroke",
            _("Use Powerstroke"),
            _("Use the format for the powerstroke path effect features of Inkscape."),
            "boolean",
            NULL, //range
            "true",  //defvalue
            0,    //flags
            NULL);//newfunc



	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}

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
 * After retruning, user must supply closing quote mark.
 */
void svgStyleTagsDump(FILE *f, LineStyle *lstyle, FillStyle *fstyle)
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
	if (fstyle) {
		if  (    fstyle->fillrule==LAXFILL_EvenOdd) fprintf(f,"fill-rule:evenodd; ");
		else if (fstyle->fillrule==LAXFILL_Nonzero) fprintf(f,"fill-rule:nonzero; ");

		if (fstyle->hasFill()) {
			fprintf(f,"fill:#%02x%02x%02x; fill-opacity:%.10g; ",
						fstyle->color.red>>8, fstyle->color.green>>8, fstyle->color.blue>>8,
						fstyle->color.alpha/65535.);
		} else fprintf(f,"fill:none; ");
	} else fprintf(f,"fill:none; ");
}


//! Function to dump out obj as svg.
/*! Return nonzero for fatal errors encountered, else 0.
 *
 * Remember that connceted defs are collected in svgdumpdef().
 *
 * \todo put in indentation
 * \todo add warning when invalid radial gradient: one circle not totally contained in another
 */
int svgdumpobj(FILE *f,double *mm,SomeData *obj,int &warning, int indent, ErrorLog &log, SvgExportConfig *out)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (!strcmp(obj->whattype(),"Group")) {
		fprintf(f,"%s<g id=\"%s\" transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					spc, obj->Id(), obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) 
			svgdumpobj(f,NULL,g->e(c),warning,indent+2,log, out); 
		fprintf(f,"    </g>\n");

	} else if (!strcmp(obj->whattype(),"GradientData")) {
		GradientData *grad;
		grad=dynamic_cast<GradientData *>(obj);
		if (!grad) return 0;

		if (grad->style&GRADIENT_RADIAL) {
			fprintf(f,"%s<circle  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    id=\"%s\"\n", spc,grad->Id());
			fprintf(f,"%s    fill=\"url(#radialGradient%ld)\"\n", spc,grad->object_id);
			fprintf(f,"%s    cx=\"%f\"\n",spc, fabs(grad->r1)>fabs(grad->r2)?grad->p1:grad->p2);
			fprintf(f,"%s    cy=\"0\"\n",spc);
			fprintf(f,"%s    r=\"%f\"\n",spc,fabs(grad->r1)>fabs(grad->r2)?fabs(grad->r1):fabs(grad->r2));
			fprintf(f,"%s  />\n",spc);
		} else {
			fprintf(f,"%s<rect  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc,obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    id=\"%s\"\n", spc,grad->Id());
			fprintf(f,"%s    fill=\"url(#linearGradient%ld)\"\n", spc,grad->object_id);
			fprintf(f,"%s    x=\"%f\"\n", spc,grad->minx);
			fprintf(f,"%s    y=\"%f\"\n", spc,grad->miny);
			fprintf(f,"%s    width=\"%f\"\n", spc,grad->maxx-grad->minx);
			fprintf(f,"%s    height=\"%f\"\n", spc,grad->maxy-grad->miny);
			fprintf(f,"%s  />\n",spc);
		}

	} else if (!strcmp(obj->whattype(),"CaptionData")) {
		CaptionData *caption=dynamic_cast<CaptionData*>(obj);
		if (!caption) return 0;

		if (out->textaspaths) {
			SomeData *path = caption->ConvertToPaths(false, NULL);
			svgdumpobj(f,mm,path,warning,indent,log,out);
			path->dec_count();
			return 0;
		}

		double rr,gg,bb,aa;
		Palette *palette=dynamic_cast<Palette*>(caption->font->GetColor());

		int layer=0;
		for (LaxFont *font=caption->font; font; font=font->nextlayer) {
			fprintf(f,"%s<text transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));

			fprintf(f,"%s   id=\"%s\"\n", spc, caption->Id());
			fprintf(f,"%s   x =\"0\"\n", spc);
			fprintf(f,"%s   y =\"%.10g\"\n", spc, caption->font->ascent());

			if (palette && layer<palette->colors.n) {
				rr=palette->colors.e[layer]->channels[0]/(double)palette->colors.e[layer]->maxcolor;
				gg=palette->colors.e[layer]->channels[1]/(double)palette->colors.e[layer]->maxcolor;
				bb=palette->colors.e[layer]->channels[2]/(double)palette->colors.e[layer]->maxcolor;
				aa=palette->colors.e[layer]->channels[3]/(double)palette->colors.e[layer]->maxcolor; 

			} else {
				rr=caption->red;
				gg=caption->green;
				bb=caption->blue;
				aa=caption->alpha;
			}

			fprintf(f,"%s   style=\"fill:#%02x%02x%02x; fill-opacity:%.10g; ",
						spc, int(rr*255+.5), int(gg*255+.5), int(bb*255+.5), aa);
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
			SomeData *path = text->ConvertToPaths(false, NULL);
			svgdumpobj(f,mm,path,warning,indent,log,out);
			path->dec_count();
			return 0;
		}

		 //first dump out a blank path:
		svgdumpobj(f, NULL, text->paths, warning, indent+2, log, out);

		 //now dump out the text, refering to that path..
		double rr=0,gg=0,bb=0,aa=1.0;
		Palette *palette=dynamic_cast<Palette*>(text->font->GetColor());

		int layer=0;
		for (LaxFont *font=text->font; font; font=font->nextlayer) {
			//fprintf(f,"%s<text transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
			//			spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s<text\n",
						spc);

			fprintf(f,"%s   id=\"%s\"\n", spc, text->Id());
			//fprintf(f,"%s   x =\"0\"\n", spc);
			//fprintf(f,"%s   y =\"%.10g\"\n", spc, text->font->ascent());

			if (palette && layer<palette->colors.n) {
				rr=palette->colors.e[layer]->channels[0]/(double)palette->colors.e[layer]->maxcolor;
				gg=palette->colors.e[layer]->channels[1]/(double)palette->colors.e[layer]->maxcolor;
				bb=palette->colors.e[layer]->channels[2]/(double)palette->colors.e[layer]->maxcolor;
				aa=palette->colors.e[layer]->channels[3]/(double)palette->colors.e[layer]->maxcolor; 

			} else if (text->color) {
				if (text->color->ColorSystemId()==LAX_COLOR_RGB) {
					rr=text->color->values[0];
					gg=text->color->values[1];
					bb=text->color->values[2];
					aa=text->color->values[3];
				}

			} else {
				rr=gg=bb=0;
				aa=1.0;
			}

			fprintf(f,"%s   style=\"fill:#%02x%02x%02x; fill-opacity:%.10g; ",
						spc, int(rr*255+.5), int(gg*255+.5), int(bb*255+.5), aa);
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

	} else if (!strcmp(obj->whattype(),"EpsData")) {
		setlocale(LC_ALL,"");
		log.AddMessage(_("Cannot export Eps objects into svg.\n"),ERROR_Warning);
		setlocale(LC_ALL,"C");
		warning++;
		
	} else if (!strcmp(obj->whattype(),"ImageData")) {
		ImageData *img;
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return 0;

		flatpoint o=obj->origin();
		o+=obj->yaxis()*(obj->maxy-obj->miny);
		
		fprintf(f,"%s<image  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
				     spc, obj->m(0), obj->m(1), -obj->m(2), -obj->m(3), o.x, o.y);
		fprintf(f,"%s    id=\"%s\"\n", spc,img->Id());
		fprintf(f,"%s    xlink:href=\"%s\" \n", spc,img->filename);
		fprintf(f,"%s    x=\"%f\"\n", spc,img->minx);
		fprintf(f,"%s    y=\"%f\"\n", spc,img->miny);
		fprintf(f,"%s    width=\"%f\"\n", spc,img->maxx-img->minx);
		fprintf(f,"%s    height=\"%f\"\n", spc,img->maxy-img->miny);
		fprintf(f,"%s  />\n",spc);
		
	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		setlocale(LC_ALL,"");
		log.AddMessage(obj->object_id,NULL,NULL,_("Warning: interpolating a color patch object\n"),ERROR_Warning);
		setlocale(LC_ALL,"C");
		warning++;
		//---------
		//if (***config allows it) {
		if (1) {
			 //approximate gradient with svg elements.
			 //in Inkscape, its blur slider is 1 to 100, with 100 corresponding to a blurring
			 //radius (standard deviation of Gaussian function) of 1/8 of the
			 //object's bounding box' perimeter (that is, for a square, a blur of
			 //100% will have the radius equal to half a side, which turns any shape
			 //into an amorphous cloud).
			ColorPatchData *patch=dynamic_cast<ColorPatchData *>(obj);
			if (!patch) return 0;

			 //make a group with a mask of outline of original patch, and blur filter
			fprintf(f,"%s<g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
				     spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
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
		if (lstyle && lstyle->hasStroke()==0) lstyle=NULL;
		FillStyle *fstyle=pdata->fillstyle;
		if (fstyle && fstyle->hasFill()==0) fstyle=NULL;
		if (!lstyle && !fstyle) return 0;


		int weighted=0;
		bool open=true;
		for (int c=0; c<pdata->paths.n; c++) {
			if (!pdata->paths.e[c]->path) continue;
			if (pdata->paths.e[c]->Weighted()) weighted++;
			if (pdata->paths.e[c]->IsClosed()) open=false;
		}

		if (!weighted) {
			 //plain, ordinary path with no offset and constant width

			fprintf(f,"%s<path  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s       id=\"%s\"\n", spc,obj->Id());

			fprintf(f,"%s       d=\"",spc);

			Path *path;
			for (int c=0; c<pdata->paths.n; c++) {
				path=pdata->paths.e[c];
				if (!path->path) continue;

				svgaddpath(f,path->path); // <- ordinary path, no special treatment
			}
			fprintf(f,"\"\n");//end of "d" for non-weighted or original-d for weighted

			fprintf(f,"%s       style=\"",spc);
			svgStyleTagsDump(f, lstyle, fstyle);
			fprintf(f,"\"\n");//end of "style"

			fprintf(f,"%s />\n",spc);//end of PathsData!


		} else {
			//at least one weighted path, may or may not use powerstroke

			 //a path based on centercache, with live path effect for powerstroke. Another for offset?
			//*** how to map weight nodes to centercache????

			if (fstyle && fstyle->hasFill() && !open) {
				fprintf(f,"%s<path  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
							 spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
				fprintf(f,"%s       id=\"%s-fill\"\n", spc,obj->Id());

				 //---write style for fill within centercache.. no stroke to that, as we apply artificial stroke
				fprintf(f,"%s       style=\"",spc);
				svgStyleTagsDump(f, NULL, fstyle);
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
				fprintf(f,"%s<path  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
							 spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
				fprintf(f,"%s       id=\"%s\"\n", spc,obj->Id());

				 //---write style: no linestyle, but fill style is based on linestyle
				fprintf(f,"%s       style=\"",spc);
				FillStyle fillstyle;
				fillstyle.color=lstyle->color;
				svgStyleTagsDump(f, NULL, &fillstyle);
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
			fprintf(f,"%s   id=\"%s\"\n", spc,obj->Id());
			fprintf(f,"%s   xlink:href=\"#%s\"\n", spc,ref->thedata->Id());
			fprintf(f,"%s />\n",spc);//end of clone!
		}


	} else {
		DrawableObject *dobj=dynamic_cast<DrawableObject*>(obj);
		SomeData *dobje=NULL;
		if (dobj) dobje=dobj->EquivalentObject();

		if (dobje) {
			dobje->Id(obj->Id());
			svgdumpobj(f,mm,dobje,warning, indent, log, out);
			dobje->dec_count();

		} else {

			setlocale(LC_ALL,"");
			char buffer[strlen(_("Cannot export %s objects into svg."))+strlen(obj->whattype())+1];
			sprintf(buffer,_("Cannot export %s objects into svg."),obj->whattype());
			log.AddMessage(obj->object_id,obj->nameid,NULL, buffer,ERROR_Warning);
			setlocale(LC_ALL,"C");
			warning++;
		}
	}

	return 0;
}

/*! Function to dump out any gradients to the defs section of an svg. Remember that
 * actual object dump is svgdumpobj().
 *
 * Return nonzero for fatal errors encountered, else 0.
 *
 * \todo fix radial gradient output for inner circle empty
 */
int svgdumpdef(FILE *f,double *mm,SomeData *obj,int &warning,ErrorLog &log, SvgExportConfig *out)
{

	if (!strcmp(obj->whattype(),"Group")) {
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) 
			svgdumpdef(f,NULL,g->e(c),warning,log, out); 

	} else if (!strcmp(obj->whattype(),"PathsData")) {
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

				fprintf(f,"\"\n"
						  "  sort_points=\"true\"\n"
						  "  interpolator_type=\"CentripetalCatmullRom\"\n"
						  "  interpolator_beta=\"0.2\"\n"
						  "  start_linecap_type=\"butt\"\n"
						  "  linejoin_type=\"extrapolated\"\n"
						  "  miter_limit=\"4\"\n"
						  "  end_linecap_type=\"butt\" />\n"
						);


				delete[] name;
			}
		}

	} else if (!strcmp(obj->whattype(),"GradientData")) {
		GradientData *grad;
		grad=dynamic_cast<GradientData *>(obj);
		if (!grad) return 0;

		if (grad->style&GRADIENT_RADIAL) {
			double r1,r2,p1,p2;
			int rr;
			if (fabs(grad->r1)>fabs(grad->r2)) { 
				p2=grad->p1;
				p1=grad->p2;
				r2=fabs(grad->r1);
				r1=fabs(grad->r2);
				rr=1; 
			} else {
				p1=grad->p1;
				p2=grad->p2;
				r1=fabs(grad->r1);
				r2=fabs(grad->r2); 
				rr=0; 
			}

			 // now figure out the color spots
			double clen=grad->colors.e[grad->colors.n-1]->t-grad->colors.e[0]->t,
				   plen=MAX((p2-r2)-(p1-r1),(p2+r2)-(p1+r1)),
				   chunk,
				   c0=grad->colors.e[(rr?grad->colors.n-1:0)]->t,
				   c1;

			int cc;
			if (r1!=0) {
				 // need extra 2 stops for transparent inner circle
				chunk=r1/plen;
				c1=grad->colors.e[(rr?grad->colors.n-1:0)]->t;
				if (rr) {
					c1+=1e-4;
					c0=c1+clen*chunk;
					clen+=clen*chunk;
				} else {
					c1+=1e-4;
					c0=c1-clen*chunk;
					clen+=clen*chunk;
				}
			}

			fprintf(f,"    <radialGradient  id=\"radialGradient%ld\"\n", grad->object_id);
			fprintf(f,"        cx=\"%f\"\n", p2);
			fprintf(f,"        cy=\"0\"\n");
			fprintf(f,"        fx=\"%f\"\n", p1); //**** wrong!!
			if (r1!=0) cout <<"*** need to fix placement of fx in svg out for radial gradients"<<endl;
			fprintf(f,"        fy=\"0\"\n");
			fprintf(f,"        r=\"%f\"\n", r2);
			fprintf(f,"        gradientUnits=\"userSpaceOnUse\">\n");

			for (int c=(r1==0?0:-2); c<grad->colors.n; c++) {
				if (rr && c>=0) cc=grad->colors.n-1-c; else cc=c;
				if (cc==-2) fprintf(f,"      <stop offset=\"0\" stop-color=\"#ffffff\" stop-opacity=\"0\" />\n");
				else if (cc==-1) fprintf(f,"      <stop offset=\"%f\" stop-color=\"#ffffff\" stop-opacity=\"0\" />\n",
											fabs(c1-c0)/clen); //offset
				else fprintf(f,"      <stop offset=\"%f\" stop-color=\"#%02x%02x%02x\" stop-opacity=\"%f\" />\n",
								fabs(grad->colors.e[cc]->t - c0)/clen, //offset
								grad->colors.e[cc]->color.red>>8, //color
								grad->colors.e[cc]->color.green>>8, 
								grad->colors.e[cc]->color.blue>>8, 
								grad->colors.e[cc]->color.alpha/65535.); //opacity
			}
			fprintf(f,"    </radialGradient>\n");

		} else {
			fprintf(f,"    <linearGradient  id=\"linearGradient%ld\"\n", grad->object_id);
			fprintf(f,"        x1=\"%f\"\n", grad->p1);
			fprintf(f,"        y1=\"0\"\n");
			fprintf(f,"        x2=\"%f\"\n", grad->p2);
			fprintf(f,"        y2=\"0\"\n");
			fprintf(f,"        gradientUnits=\"userSpaceOnUse\">\n");
			double clen=grad->colors.e[grad->colors.n-1]->t-grad->colors.e[0]->t;
			for (int c=0; c<grad->colors.n; c++) {
				fprintf(f,"      <stop offset=\"%f\" stop-color=\"#%02x%02x%02x\" stop-opacity=\"%f\" />\n",
								(grad->colors.e[c]->t-grad->colors.e[0]->t)/clen, //offset
								grad->colors.e[c]->color.red>>8, //color
								grad->colors.e[c]->color.green>>8, 
								grad->colors.e[c]->color.blue>>8, 
								grad->colors.e[c]->color.alpha/65535.); //opacity
			}
			fprintf(f,"    </linearGradient>\n");
		}

	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		//if (***config allows it) {
		if (1) {
			 //insert mask for patch
			ColorPatchData *patch=dynamic_cast<ColorPatchData *>(obj);
			if (!patch) return 0;

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
		}
	}

	return 0;
}




DocumentExportConfig *SvgOutputFilter::CreateConfig(DocumentExportConfig *fromconfig)
{
	return new SvgExportConfig(fromconfig);
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

	SvgExportConfig *out=dynamic_cast<SvgExportConfig *>(context);
	
	if (!out) {
		out=new SvgExportConfig(dout);
	} else out->inc_count();

	Document *doc =out->doc;
	int start     =out->start;
	//int end       =out->end;
	int layout    =out->layout;
	Group *limbo  =out->limbo;
	PaperGroup *papergroup=out->papergroup;
	if (!filename) filename=out->filename;
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		out->dec_count();
		return 1;
	}
	
	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			out->dec_count();
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".svg");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,&log);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		out->dec_count();
		return 3;
	}

	setlocale(LC_ALL,"C");

	int warning=0;
	Spread *spread=NULL;
	Group *g=NULL;
	double m[6],mm[6],mmm[6];
	//int c;
	int c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);

	if (doc) spread=doc->imposition->Layout(layout,start);
	
	 // write out header
	double height=0,width=0;
	if (spread) {
		height=spread->path->maxy-spread->path->miny;
		width =spread->path->maxx-spread->path->minx;
	}
	if (height==0) height=papergroup->papers.e[0]->box->paperstyle->h(); //takes in to account landscape status
	if (width==0)  width =papergroup->papers.e[0]->box->paperstyle->w();

	if (out->curpaperrotation==90 || out->curpaperrotation==270) {
		double tt=height;
		height=width;
		width=tt;
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
	fprintf(f,"     height=\"%fin\"\n", height);
	fprintf(f,"   >\n");

	 //set default units for use later in Inkscape
	char *units=NULL;
	GetUnitManager()->UnitInfoId(laidout->prefs.default_units, NULL, &units,NULL,NULL);
	if (units) {
		fprintf(f,"  <sodipodi:namedview\n"
				  "      id=\"base\"\n"
				  "      inkscape:document-units=\"%s\"\n"
				  "      units=\"%s\"\n"
				  "  />\n",
				 units, units);
	}
			

	 //write out global defs section
	 //   ..gradients and such
	fprintf(f,"  <defs>\n");

	 //dump out defs for limbo objects if any
	if (limbo && limbo->n()) {
		svgdumpdef(f,m,limbo,warning,log, out);
	}

	if (papergroup->objs.n()) {
		svgdumpdef(f,m,&papergroup->objs,warning,log, out);
	}


	if (spread) {
		if (spread->marks) svgdumpdef(f,m,spread->marks,warning,log, out);

		 // for each page in spread..
		for (c2=0; c2<spread->pagestack.n(); c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
				for (c3=0; c3<g->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					svgdumpdef(f,m,g->e(c3),warning,log, out);
				}
			}
		}
	}
	fprintf(f,"  </defs>\n");
			
	
	 // Write out objects....
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
	fprintf(f,"    <g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 

	// *** need to adjust for multipaper...
	//transform_invert(mmm,papergroup->papers.e[0]->m());
	//transform_mult(mm, m,mmm);



	 //dump out limbo objects if any
	if (limbo && limbo->n()) {
		transform_set(m,1,0,0,1,0,0);
		svgdumpobj(f,m,limbo,warning,4,log, out);
	}

	if (papergroup->objs.n()) {
		transform_set(m,1,0,0,1,0,0);
		svgdumpobj(f,m,&papergroup->objs,warning,4,log, out);
	}



	if (spread) {
		 //write out printer marks
		transform_set(m,1,0,0,1,0,0);
		if (spread->marks) svgdumpobj(f,m,spread->marks,warning,4,log, out);

		 // for each page in spread..
		for (c2=0; c2<spread->pagestack.n(); c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
				transform_copy(mm,spread->pagestack.e[c2]->outline->m());
				fprintf(f,"    <g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					mm[0], mm[1], mm[2], mm[3], mm[4], mm[5]); 
				for (c3=0; c3<g->n(); c3++) {
					svgdumpobj(f,NULL,g->e(c3),warning,6,log, out);
				}
				fprintf(f,"    </g>\n ");
			}
		}

		delete spread;
	}

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
	if (!strstr(first100bytes,"<svg")) return NULL;
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
	styledef=stylemanager.FindDef("SvgImportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"SvgImportConfig");
	makestr(styledef->Name,_("Svg Import Configuration"));
	makestr(styledef->description,_("Configuration to import a Svg file."));
	styledef->newfunc=newSvgImportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}



//forward declaration:
int svgDumpInObjects(int top,Group *group, Attribute *element, PtrStack<Attribute> &powerstrokes, ErrorLog &log);
GradientData *svgDumpInGradientDef(Attribute *att, Attribute *defs, int type, GradientData *gradient);

int SvgImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log)
{
	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	Document *doc=in->doc;

	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
	if (!att) return 2;
	
	 //create repository for hints if necessary
	Attribute *svghints=NULL,
			  *svg=NULL; //points to the "svg" section of svghints. Do not delete!!
	//if (in->keepmystery) svghints=new Attribute(VersionName(),file);  ***disable svghints for now
	try {

		 //add xml preamble, and anything not under "svg" to hints if it exists...
		if (svghints) {
			for (int c=0; c<att->attributes.n; c++) {
				if (!strcmp(att->attributes.e[c]->name,"svg")) continue;
				svghints->push(att->attributes.e[c]->duplicate(),-1);
			}
			svg=new Attribute("svg",NULL);
			svghints->push(svg,-1);
		}

		int c;
		char *name,*value;
		double width=0, height=0;
		Attribute *svgdoc=att->find("svg");
		if (!svgdoc) {
			log.AddMessage(_("Could not find svg tag.\n"),ERROR_Fail);
			throw 3;
		}

		for (c=0; c<svgdoc->attributes.n; c++) {
			name=svgdoc->attributes.e[c]->name;
			value=svgdoc->attributes.e[c]->value;
			if (!strcmp(name,"content:")) continue;

			if (svghints) svg->push(svgdoc->attributes.e[c]->duplicate(),-1);
			 
			 //find the width and height of the document
			if (!strcmp(name,"width")) { // *** warning!!!! could be named units here!!!
				char *endptr=NULL;
				DoubleAttribute(value,&width, &endptr);
				if (*endptr) {
					 //parse units 
					UnitManager *unitm = GetUnitManager();
					while (isspace(*endptr)) endptr++;
					const char *ptr=endptr;
					while (isalpha(*endptr)) endptr++;
					int units = unitm->UnitId(ptr, endptr-ptr);
					if (units!=UNITS_None) width = unitm->Convert(width, units, UNITS_Inches, NULL);

				} else width /= DEFAULT_PPINCH; //no specified units, assume svg pts

			} else if (!strcmp(name,"height")) {
				char *endptr=NULL;
				DoubleAttribute(value,&height, &endptr);
				if (*endptr) {
					 //parse units 
					UnitManager *unitm = GetUnitManager();
					while (isspace(*endptr)) endptr++;
					const char *ptr=endptr;
					while (isalpha(*endptr)) endptr++;
					int units = unitm->UnitId(ptr, endptr-ptr);
					if (units!=UNITS_None) height = unitm->Convert(height, units, UNITS_Inches, NULL);

				} else height /= DEFAULT_PPINCH; //no specified units, assume svg pts
				
			} else if (!strcmp(name,"viewBox")) {
				// *** also need to look out for other transform defined on base svg level, maybe nonstandard, but sometimes it's present
			}
		}

		svgdoc=svgdoc->find("content:");
		if (!svgdoc) {
			log.AddMessage(_("Empty svg tag!\n"),ERROR_Fail); 
			throw 4;
		}


		 //create a new document if necessary
		if (!doc && !in->toobj) {
			if (width==0 || height==0) {
				 //use default paper size, if no width or height found
				PaperStyle *pp = laidout->GetDefaultPaper();
				width  = pp->w();
				height = pp->h();

			}

			 //figure out the paper size, orientation
			PaperStyle *paper=NULL;
			int landscape=0;

			 //svg/inkscape uses width and height, but not paper names as far as I can see
			 //search for paper size known to laidout within certain approximation
			for (c=0; c<laidout->papersizes.n; c++) {
				if (     fabs(width- laidout->papersizes.e[c]->width)<.0001
					  && fabs(height-laidout->papersizes.e[c]->height)) {
					paper=laidout->papersizes.e[c];
					break;
				}
				if (     fabs(height-laidout->papersizes.e[c]->width)<.0001
					  && fabs(width -laidout->papersizes.e[c]->height)) {
					paper=laidout->papersizes.e[c];
					landscape=1;
					break;
				}
			}
			if (paper) paper=dynamic_cast<PaperStyle*>(paper->duplicate());
			else {
				paper=new PaperStyle(_("Custom"), width,height, 0, 300, "in");
			}
			
			 //preliminary start and end pages for the svg
	//		int start,end;
	//		if (in->instart<0) start=0; else start=in->instart;
	//		if (in->inend<0) end=10000000; 
	//			else end=in->inend; 


			 //now svgdoc's subattributes should be a combination of 
			 //defs, sodipodi:namedview, metadata
			 // PLUS any number of graphic elements, such as g, rect, image, text....

			Imposition *imp=new Singles;
			paper->landscape(landscape);
			imp->SetPaperSize(paper);
			paper->dec_count();
			doc=new Document(imp,Untitled_name());
			imp->dec_count();
		} //if (!doc && !in->toobj)


		Group *group=in->toobj;

		if (!group && doc) {
			 //document page to start dumping onto
			int docpagenum=in->topage; //the page in laidout doc to start dumping into
			int curdocpage; //the current page in the laidout document, used in loop below
			if (docpagenum<0) docpagenum=0; 

			 //update group to point to the document page's group
			curdocpage=docpagenum;
			if (curdocpage>=doc->pages.n) {
				doc->NewPages(-1,(curdocpage+1)-doc->pages.n);
			}
			group=dynamic_cast<Group *>(doc->pages.e[curdocpage]->layers.e(0)); //pick layer 0 of the page
		}

		RefPtrStack<anObject> gradients;
		PtrStack<Attribute> powerstrokes;

		for (c=0; c<svgdoc->attributes.n; c++) {
			name =svgdoc->attributes.e[c]->name;
			value=svgdoc->attributes.e[c]->value;
			 //first check for document level things like gradients in defs or metadata,
			 //then check for drawable things
			 //then push any other stuff unchanged
			if (!strcmp(name,"metadata")
				     || !strcmp(name,"sodipodi:namedview")) {
				 //just copy over "metadata" and "sodipodi:namedview" to svghints
				if (svghints) {
					svg->push(svgdoc->attributes.e[c]->duplicate(),-1);
				}
				continue;

			} else if (!strcmp(name,"defs")) {
				 //need to read in gradient and filter data...
				Attribute *defsatt=svgdoc->find("content:");
				Attribute *def;
				if (!defsatt || !defsatt->attributes.n) continue;

				for (int c2=0; c2<defsatt->attributes.n; c2++) {
					def=defsatt->attributes.e[c2];
					name =def->name;
					value=def->value;

					if (!strcmp(name,"linearGradient")) {
						GradientData *gradient=svgDumpInGradientDef(def, defsatt, GRADIENT_LINEAR, NULL);
						gradients.push(gradient);
						gradient->dec_count();

					} else if (!strcmp(name,"radialGradient")) {
						GradientData *gradient=svgDumpInGradientDef(def, defsatt, GRADIENT_RADIAL, NULL);
						gradients.push(gradient);
						gradient->dec_count();

					} else if (!strcmp(name,"inkscape:path-effect")) {
						Attribute *power=def->find("effect");
						if (power && power->value && !strcmp(power->value,"powerstroke")) {
							powerstrokes.push(def,0);
						}

					} else if (!strcmp(name,"mask")) {
					} else if (!strcmp(name,"filter")) {
					//} else if (!strcmp(name,"font")) {
					//} else if (!strcmp(name,"font-face")) {
					//} else if (!strcmp(name,"image")) {
					//} else if (!strcmp(name,"pattern")) {
					//} else if (!strcmp(name,"text")) {
					//} else if (!strcmp(name,"a")) {
					//} else if (!strcmp(name,"altGlyphDef")) {
					//} else if (!strcmp(name,"clipPath")) {
					//} else if (!strcmp(name,"color-profile")) {
					//} else if (!strcmp(name,"cursor")) {
					//} else if (!strcmp(name,"foreignObject")) {
					//} else if (!strcmp(name,"marker")) {
					//} else if (!strcmp(name,"script")) {
					//} else if (!strcmp(name,"style")) {
					//} else if (!strcmp(name,"switch")) {
					//} else if (!strcmp(name,"view")) {
					}
				}
				continue;

			} 
			
			if (svgDumpInObjects(height,group,svgdoc->attributes.e[c],powerstrokes,log)) continue;

			 //push any other blocks into svghints.. not expected, but you never know
			if (svghints) {
				Attribute *more=new Attribute("docContent",NULL);
				more->push(svgdoc->attributes.e[c]->duplicate(),-1);
				svghints->push(more,-1);
			}
		}
		

		 //install global hints if they exist
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

		 //if doc is new, push into the project
		if (doc && doc!=in->doc) {
			laidout->project->Push(doc);
			laidout->app->addwindow(newHeadWindow(doc));
		}
	
	} catch (int error) {
		if (svghints) delete svghints;
		delete att;
		return 1;
	}
	return 0;
}

GradientData *svgDumpInGradientDef(Attribute *def, Attribute *defs, int type, GradientData *gradient)
{
	if (!gradient) gradient=dynamic_cast<GradientData *>(newObject("GradientData"));

	double cx,cy,fx,fy,r;
	flatpoint p1,p2;
	int foundf=0;
	if (!def) return NULL;
	char *name, *value;
	//int units=0;//0 is user space, 1 is bounding box
	double gm[6];
	transform_identity(gm);

	for (int c3=0; c3<def->attributes.n; c3++) {
		name =def->attributes.e[c3]->name;
		value=def->attributes.e[c3]->value;

		if (!strcmp(name,"xlink:href")) {
			 // might be color spots, need to scan in the ref
			if (!value || value[0]!='#') continue;
			char *xlinkid=value+1;
			Attribute *xlink=NULL;

			for (int c=0; c<defs->attributes.n; c++) {
				name =def->attributes.e[c]->name;
				value=def->attributes.e[c]->value;
				if (!strcmp(name,"linearGradient") || !strcmp(name,"radialGradient")) {
					Attribute *g=def->attributes.e[c]->find("id");
					if (!strcmp(g->value,xlinkid)) {
						xlink=def->attributes.e[c];
						break;
					}
				}
			}
			if (xlink) svgDumpInGradientDef(xlink, defs, type, gradient);

		} else if (!strcmp(name,"id")) {
			if (!isblank(value)) gradient->Id(value);

		} else if (!strcmp(name,"gradientUnits")) {
			 //gradientUnits = "userSpaceOnUse | objectBoundingBox"
			//if (!strcmp(value,"userSpaceOnUse")) units=0;
			//else if (!strcmp(value,"objectBoundingBox")) units=1;
			DBG cerr <<" warning: ignoring gradientUnits on svg gradient in"<<endl;

		} else if (!strcmp(name,"gradientTransform")) {
			svgtransform(value,gm);

		} else if (!strcmp(name,"cx")) {
			DoubleAttribute(value,&cx,NULL);

		} else if (!strcmp(name,"cy")) {
			DoubleAttribute(value,&cy,NULL);

		} else if (!strcmp(name,"fx")) {
			DoubleAttribute(value,&fx,NULL);
			foundf=1;

		} else if (!strcmp(name,"fy")) {
			DoubleAttribute(value,&fy,NULL);
			foundf=1;

		} else if (!strcmp(name,"r")) {
			DoubleAttribute(value,&r,NULL);

		} else if (!strcmp(name,"x1")) {
			DoubleAttribute(value,&p1.x,NULL);

		} else if (!strcmp(name,"y1")) {
			DoubleAttribute(value,&p1.y,NULL);

		} else if (!strcmp(name,"x2")) {
			DoubleAttribute(value,&p2.x,NULL);

		} else if (!strcmp(name,"y2")) {
			DoubleAttribute(value,&p2.y,NULL);

		} else if (!strcmp(name,"content:")) {
			Attribute *spot=def->attributes.e[c3];
			double offset=0,alpha=0;
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
							char *str=strstr(value,"stop-color:");
							if (str) {
								str+=11;
								HexColorAttributeRGB(str,&color,NULL);
							}
							str=strstr(value,"stop-opacity:");
							if (str) {
								str+=13;
								DoubleAttribute(str,&alpha,NULL);
							}

						} else if (!strcmp(name,"offset")) {
							DoubleAttribute(value,&offset,NULL);

						} else if (!strcmp(name,"stop-color")) {
							HexColorAttributeRGB(value,&color,NULL);

						} else if (!strcmp(name,"stop-opacity")) {
							DoubleAttribute(value,&alpha,NULL);

						} else if (!strcmp(name,"id")) {
							//ignore stop id
						}
					}
					gradient->AddColor(offset,&color);
				}
			}
		}
	}

	if (type==GRADIENT_RADIAL) {
		p1=transform_point(gm,flatpoint(cx,cy));
		p2=(foundf ? transform_point(gm,flatpoint(fx,fy)) : p1);
		gradient->Set(p1,p2, r,0, NULL,NULL, GRADIENT_RADIAL);
	} else {
		gradient->Set(p1,p2, 1,-1, NULL,NULL, GRADIENT_LINEAR);
	}
	
	return gradient;
}

//! Return 1 for attribute used, else 0.
/*! If top!=0, then top is the height of the document. We need to flip elements up,
 * since down is positive y in svg. We also need to scale by .8/72 to convert svg units to Laidout units.
 */
int svgDumpInObjects(int top,Group *group, Attribute *element, PtrStack<Attribute> &powerstrokes, ErrorLog &log)
{
	char *name,*value;

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

			} else if (!strcmp(name,"content:")) {
				for (int c2=0; c2<element->attributes.e[c]->attributes.n; c2++) 
					svgDumpInObjects(0,g,element->attributes.e[c]->attributes.e[c2],powerstrokes,log);
			}
		}

		if (top) {
			for (int c=0; c<6; c++) g->m(c,g->m(c)/DEFAULT_PPINCH); //correct for svg scaling

			g->m(5,top-g->m(5)); //flip in page
			g->m(2, -g->m(2));
			g->m(3, -g->m(3));
		}

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
				DoubleAttribute(value,&x,NULL);

			} else if (!strcmp(name,"y")) {
				foundcoord|=2;
				DoubleAttribute(value,&y,NULL);

			} else if (!strcmp(name,"width")) {
				foundcoord|=4;
				DoubleAttribute(value,&w,NULL);

			} else if (!strcmp(name,"height")) {
				foundcoord|=8;
				DoubleAttribute(value,&h,NULL);

			} else if (!strcmp(name,"xlink:href")) {
				err=image->LoadImage(value);
				if (err) break;

			} else if (!strcmp(name,"transform")) {
				double m[6];
				transform_identity(m);
				svgtransform(value,m);
				if (!is_degenerate_transform(m)) image->m(m);
			}
		}

		if (err==0) {
			DoubleBBox bbox(x,x+w, y,y+h);
			image->fitto(NULL, &bbox, 50, 50);

			image->Flip(0);
			group->push(image);
		} //else error loading image

		image->dec_count();
		return 1;

	} else if (!strcmp(element->name,"path")) {
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));
		int d_index=-1;
		Attribute *powerstroke=NULL;

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
				LineStyle *linestyle = paths->linestyle;
				FillStyle *fillstyle = paths->fillstyle;

				if (!linestyle) {
					linestyle=new LineStyle;
					paths->InstallLineStyle(linestyle);
					linestyle->dec_count();
				}
				if (!fillstyle) {
					fillstyle=new FillStyle;
					paths->InstallFillStyle(fillstyle);
					fillstyle->dec_count();
				}

				StyleToFillAndStroke(value, linestyle, fillstyle);

			} else if (!strcmp(name,"inkscape:path-effect")) {
				//path-effect is something like: "#path-effect3338;#path-effect3343"
				//we currently can only process powerstroke..

				while (value && *value) {
					if (*value=='#') value++;

					char *endptr=strchrnul(value,';');
					const char *id;

					for (int c2=0; c2<powerstrokes.n; c2++) {
						id=powerstrokes.e[c]->findValue("id");
						if (!id) continue;
						if (!strncmp(value,id,endptr-value)) {
							powerstroke=powerstrokes.e[c];
							break;
						}
					}
					if (powerstroke) break;
					value=endptr;
					if (*value==';') value++;
				}

			} else if (!strcmp(name,"d")) {
				d_index=c;

			//} else if (!strcmp(name,"x")) {
			//} else if (!strcmp(name,"y")) {
			}
		}

		if (d_index>=0) {
			SvgToPathsData(paths, element->attributes.e[d_index]->value, NULL, powerstroke);
		}

		if (paths->paths.n) {
			paths->FindBBox();
			group->push(paths);
		}
		paths->dec_count();
		return 1;

	} else if (!strcmp(element->name,"rect")) {
		double x=0,y=0,w=0,h=0;
		double rx=-1, ry=-1;
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);
			} else if (!strcmp(name,"rx")) {
				DoubleAttribute(value,&rx,NULL);
			} else if (!strcmp(name,"ry")) {
				DoubleAttribute(value,&ry,NULL);
			} else if (!strcmp(name,"x")) {
				DoubleAttribute(value,&x,NULL);
			} else if (!strcmp(name,"y")) {
				DoubleAttribute(value,&y,NULL);
			} else if (!strcmp(name,"width")) {
				DoubleAttribute(value,&w,NULL);
			} else if (!strcmp(name,"height")) {
				DoubleAttribute(value,&h,NULL);

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				LineStyle *linestyle = paths->linestyle;
				FillStyle *fillstyle = paths->fillstyle;

				if (!linestyle) {
					linestyle=new LineStyle;
					paths->InstallLineStyle(linestyle);
					linestyle->dec_count();
				}
				if (!fillstyle) {
					fillstyle=new FillStyle;
					paths->InstallFillStyle(fillstyle);
					fillstyle->dec_count();
				}

				StyleToFillAndStroke(value, linestyle, fillstyle); 
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
		if (paths->paths.n) {
			paths->FindBBox();
			group->push(paths);
		}
		paths->dec_count();
		return 1;

	} else if (!strcmp(element->name,"circle") || !strcmp(element->name,"ellipse")) {
		 //using 4 vertices as bez points, the vector length is 4*(sqrt(2)-1)/3 = about .5523 with radius 1

		double cx=0,cy=0,r=-1,rx=-1, ry=-1;
		PathsData *paths=dynamic_cast<PathsData *>(newObject("PathsData"));

		for (int c=0; c<element->attributes.n; c++) {
			name =element->attributes.e[c]->name;
			value=element->attributes.e[c]->value;

			if (!strcmp(name,"id")) {
				if (!isblank(value)) paths->Id(value);
			} else if (!strcmp(name,"rx")) { //present in ellipses
				DoubleAttribute(value,&rx,NULL);
			} else if (!strcmp(name,"ry")) { //present in ellipses 
				DoubleAttribute(value,&ry,NULL);
			} else if (!strcmp(name,"r")) { //present in circles
				DoubleAttribute(value,&r,NULL);
			} else if (!strcmp(name,"cx")) {
				DoubleAttribute(value,&cx,NULL);
			} else if (!strcmp(name,"cy")) {
				DoubleAttribute(value,&cy,NULL);

			} else if (!strcmp(name,"transform")) {
				double m[6];
				svgtransform(value,m);
				paths->m(m);

			} else if (!strcmp(name,"style")) {
				LineStyle *linestyle = paths->linestyle;
				FillStyle *fillstyle = paths->fillstyle;

				if (!linestyle) {
					linestyle=new LineStyle;
					paths->InstallLineStyle(linestyle);
					linestyle->dec_count();
				}
				if (!fillstyle) {
					fillstyle=new FillStyle;
					paths->InstallFillStyle(fillstyle);
					fillstyle->dec_count();
				}

				StyleToFillAndStroke(value, linestyle, fillstyle); 
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

			if (paths->paths.n) {
				paths->FindBBox();
				group->push(paths);
			}
		}
		paths->dec_count();
		return 1;

	} else if (!strcmp(element->name,"line")) {
		cout <<"***need to implement svg in:  line"<<endl;

	} else if (!strcmp(element->name,"polyline")) {
		cout <<"***need to implement svg in:  polyline"<<endl;

	} else if (!strcmp(element->name,"polygon")) {
		cout <<"***need to implement svg in:  polygon"<<endl;

	} else if (!strcmp(element->name,"text")) {
		//SomeData *data = SvgTextIn(element);
		//group->push(data);
		//data->dec_count();
		//----
		CaptionData *textobj=dynamic_cast<CaptionData *>(newObject("CaptionData"));

		char *name;
		char *value;
		double x=0, y=0;
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
			} else if (!strcmp(name,"style")) {
				Attribute att;
				InlineCSSToAttribute(value, &att);

				for (int c2=0; c2<att.attributes.n; c2++) {
					name  = att.attributes.e[c2]->name;
					value = att.attributes.e[c2]->value;

					if (!strcmp(name, "line-height")) {
						DoubleAttribute(value, &textobj->linespacing); // *** need to compute units!!!
					} else if (!strcmp(name, "font-family")) {
					} else if (!strcmp(name, "font-style")) {
					} else if (!strcmp(name, "font-size")) {
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
			textobj->origin(flatpoint(x,y));
			group->push(textobj);
		}

		cout <<"***need to finish implementing svg in:  text"<<endl;

	} else if (!strcmp(element->name,"use")) {
		 //references to other objects
		cout <<"***need to implement svg in:  use"<<endl;
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
 * linestyle and fillstyle must not be NULL.
 */
int StyleToFillAndStroke(const char *inlinecss, LaxInterfaces::LineStyle *linestyle, LaxInterfaces::FillStyle *fillstyle)
{
	if (!inlinecss) return 1;

	Attribute att;
	InlineCSSToAttribute(inlinecss, &att);

	const char *name, *value;
	double d;
	double strokecolor[4], fillcolor[4], defaultcolor[4];
	int founddefcol=0, foundstroke=0, foundfill=0;

	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;

		if (!strcmp(name,"color")) { //default in absence of fill/stroke colors.. #000000
			if (value && !strcmp(value,"none")) {
				founddefcol=-1;
			} else {
				SimpleColorAttribute(value, defaultcolor, NULL);
				founddefcol=1;
			}

		} else if (!strcmp(name,"stroke")) { //the stroke color: #021bd9
			if (value && !strcmp(value,"none")) {
				foundstroke=-1;
			} else {
				d=strokecolor[3];
				SimpleColorAttribute(value, strokecolor, NULL);
				if (foundstroke==2) strokecolor[3]=d;
				foundstroke=1;
			}

		} else if (!strcmp(name,"stroke-width")) { //35.29999924
			if (DoubleAttribute(value, &d)) linestyle->width = d;

		} else if (!strcmp(name,"stroke-opacity")) { //0..1
			if (DoubleAttribute(value, &d)) strokecolor[3]=d;
			if (foundstroke==0) foundstroke=2;

		} else if (!strcmp(name,"fill")) { //fill color #ff0000
			if (value && !strcmp(value,"none")) {
				foundfill=-1;
			} else {
				d=fillcolor[3];
				SimpleColorAttribute(value, fillcolor, NULL);
				if (foundfill==2) fillcolor[3]=d; //opacity was found first, but SCA overwrites
				foundfill=1;
			}

		} else if (!strcmp(name,"fill-opacity")) { //0..1
			if (DoubleAttribute(value, &d)) fillcolor[3]=d;
			if (foundfill==0) foundfill=2;

		} else if (!strcmp(name,"fill-rule") && value) { //evenodd | nonzero | inherit
			if (!strcmp(value, "evenodd")) fillstyle->fillrule = LAXFILL_EvenOdd;
			else if (!strcmp(value, "nonzero")) fillstyle->fillrule = LAXFILL_Nonzero;

		} else if (!strcmp(name,"stroke-linecap")) { //butt
			// *** todo!!
		} else if (!strcmp(name,"stroke-linejoin")) { //miter
			// *** todo!!
		} else if (!strcmp(name,"stroke-miterlimit")) { //4
			// *** todo!!
		} else if (!strcmp(name,"stroke-dasharray")) { //none
			// *** todo!!
		} else if (!strcmp(name,"stroke-dashoffset")) { //0
			// *** todo!!
		}

		 //other stuff found in inkscape svgs:
		//} else if (!strcmp(name,opacity)) { //1
		//} else if (!strcmp(name,clip-rule)) { //nonzero
		//} else if (!strcmp(name,display)) { //inline
		//} else if (!strcmp(name,overflow)) { //visible
		//} else if (!strcmp(name,visibility)) { //visible
		//} else if (!strcmp(name,isolation)) { //auto
		//} else if (!strcmp(name,mix-blend-mode)) { //normal
		//} else if (!strcmp(name,color-interpolation)) { //sRGB
		//} else if (!strcmp(name,color-interpolation-filters)) { //linearRGB
		//} else if (!strcmp(name,solid-color)) { //#000000
		//} else if (!strcmp(name,solid-opacity)) { //1
		//} else if (!strcmp(name,marker)) { //none
		//} else if (!strcmp(name,color-rendering)) { //auto
		//} else if (!strcmp(name,image-rendering)) { //auto
		//} else if (!strcmp(name,shape-rendering)) { //auto
		//} else if (!strcmp(name,text-rendering)) { //auto
		//} else if (!strcmp(name,enable-background)) { //accumulate"
	}

	if (founddefcol) {
		 // *** note: fails when opacity but not color specified
		if (!foundstroke) memcpy(strokecolor, defaultcolor, 4*sizeof(double));
		if (!foundfill)   memcpy(fillcolor,   defaultcolor, 4*sizeof(double));
	}

	if (foundstroke) {
		if (foundstroke==-1) linestyle->function = LAXOP_None;
		else linestyle->Colorf(strokecolor[0],strokecolor[1],strokecolor[2],strokecolor[3]);
	}
	if (foundfill) {
		if (foundfill==-1) fillstyle->function = LAXOP_None;
		fillstyle->Colorf(fillcolor[0],fillcolor[1],fillcolor[2],fillcolor[3]);
	}


	return 0;
}





} // namespace Laidout

