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
// Copyright (C) 2007-2009 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../dataobjects/mysterydata.h"
#include "svg.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"
#include "../utils.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



//--------------------------------- install SVG filter

//! Tells the Laidout application that there's a new filter in town.
void installSvgFilter()
{
	SvgOutputFilter *svgout=new SvgOutputFilter;
	laidout->exportfilters.push(svgout);
	
	SvgImportFilter *svgin=new SvgImportFilter;
	laidout->importfilters.push(svgin);
}


//------------------------------------ SvgOutputFilter ----------------------------------
	
/*! \class SvgOutputFilter
 * \brief Filter for exporting SVG 1.0.
 *
 * \todo implement PAGE and PAGESET elements of draft svg 1.2
 */


SvgOutputFilter::SvgOutputFilter()
{
	version=1.1;
	//flags=FILTERS_MULTIPAGE; //***not multipage yet!
}

const char *SvgOutputFilter::Version()
{
	if (version<1.2) return "1.1"; 
	return "1.2";
}

const char *SvgOutputFilter::VersionName()
{
	if (version<1.2) return _("Svg 1.1"); 
	return _("Svg 1.2");
}

//! Function to dump out obj as svg.
/*! Return nonzero for fatal errors encountered, else 0.
 *
 * \todo put in indentation
 */
int svgdumpobj(FILE *f,double *mm,SomeData *obj,char **error_ret,int &warning, int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (!strcmp(obj->whattype(),"Group")) {
		fprintf(f,"%s<g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\">\n ",
					spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) 
			svgdumpobj(f,NULL,g->e(c),error_ret,warning,indent+2); 
		fprintf(f,"    </g>\n");
	} else if (!strcmp(obj->whattype(),"GradientData")) {
		GradientData *grad;
		grad=dynamic_cast<GradientData *>(obj);
		if (!grad) return 0;

		if (grad->style&GRADIENT_RADIAL) {
			fprintf(f,"%s<circle  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    fill=\"url(#radialGradient%ld)\"\n", spc,grad->object_id);
			fprintf(f,"%s    cx=\"%f\"\n",spc, fabs(grad->r1)>fabs(grad->r2)?grad->p1:grad->p2);
			fprintf(f,"%s    cy=\"0\"\n",spc);
			fprintf(f,"%s    r=\"%f\"\n",spc,fabs(grad->r1)>fabs(grad->r2)?fabs(grad->r1):fabs(grad->r2));
			fprintf(f,"%s  />\n",spc);
		} else {
			fprintf(f,"%s<rect  transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
						 spc,obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s    fill=\"url(#linearGradient%ld)\"\n", spc,grad->object_id);
			fprintf(f,"%s    x=\"%f\"\n", spc,grad->minx);
			fprintf(f,"%s    y=\"%f\"\n", spc,grad->miny);
			fprintf(f,"%s    width=\"%f\"\n", spc,grad->maxx-grad->minx);
			fprintf(f,"%s    height=\"%f\"\n", spc,grad->maxy-grad->miny);
			fprintf(f,"%s  />\n",spc);
		}

	} else if (!strcmp(obj->whattype(),"EpsData")) {
		setlocale(LC_ALL,"");
		appendstr(*error_ret,_("Cannot export Eps objects into svg.\n"));
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
		fprintf(f,"%s    xlink:href=\"%s\" \n", spc,img->filename);
		fprintf(f,"%s    x=\"%f\"\n", spc,img->minx);
		fprintf(f,"%s    y=\"%f\"\n", spc,img->miny);
		fprintf(f,"%s    width=\"%f\"\n", spc,img->maxx-img->minx);
		fprintf(f,"%s    height=\"%f\"\n", spc,img->maxy-img->miny);
		fprintf(f,"%s  />\n",spc);
		
	} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
		setlocale(LC_ALL,"");
		appendstr(*error_ret,_("Warning: interpolating a color patch object\n"));
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
			fprintf(f,"%s   clip-path=\"url(#colorPatchMask%ld)\" filter=\"url(#patchBlur%ld)\">\n", 
						spc, patch->object_id, patch->object_id);

			int c,r, cc,rr;
			int numdiv=5;
			flatpoint p[4];
			XRenderColor color;
			double s,ds, t,dt;
			ds=1./(patch->xsize/3)/numdiv;
			dt=1./(patch->ysize/3)/numdiv;
			double fudge=1.05; //so there are no transparent boundaries between the divided up rects

			 //create border colors so as to not have transparent outer edge sections.
			 //this has to be done before the actual patch, so that the real colors lay
			 //on top of this stuff.
			flatpoint pcenter=patch->getPoint(.5,.5);
			double extend=2;
			for (r=0; r<patch->ysize/3; r++) {
			    for (rr=0; rr<numdiv; rr++) {
					//s=(c+(float)cc/numdiv)/(patch->xsize/3);
					t=(r+(float)rr/numdiv)/(patch->ysize/3);

					 //---------left side
				     //get color for point (r+rr,c+cc)
					//patch->getColor(0,t/(1-dt),&color);
					patch->getColor(t/(1-dt),0,&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(0   ,t);
					p[1]=patch->getPoint(0   ,t+fudge*dt);
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
					//patch->getColor(1.,t/(1-dt),&color);
					patch->getColor(t/(1-dt),1,&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(1.   ,t);
					p[1]=patch->getPoint(1.   ,t+fudge*dt);
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
					//patch->getColor(s/(1-ds),0,&color);
					patch->getColor(0,s/(1-ds),&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(s,          0);
					p[1]=patch->getPoint(s+fudge*ds, 0);
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
					//patch->getColor(s/(1-ds),1,&color);
					patch->getColor(1,s/(1-ds),&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(s,          1);
					p[1]=patch->getPoint(s+fudge*ds, 1);
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
					patch->getColor(tt,ss,&color);

			  		 //get coords for that little rect
					p[0]=patch->getPoint(s   ,t);
					p[1]=patch->getPoint(s+fudge*ds,t);
					p[2]=patch->getPoint(s+fudge*ds,t+fudge*dt);
					p[3]=patch->getPoint(s   ,t+fudge*dt);

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
		
	} else if (!strcmp(obj->whattype(),"ImagePatchData")) {
		//***if (config->collect_for_out) { rasterize, and put image in out directory }
		setlocale(LC_ALL,"");
		appendstr(*error_ret,_("Cannot export Image Patch objects into svg.\n"));
		setlocale(LC_ALL,"C");
		warning++;
	}
	return 0;
}

//! Function to dump out any gradients to the defs section of an svg.
/*! Return nonzero for fatal errors encountered, else 0.
 *
 * \todo fix radial gradient output for inner circle empty
 */
int svgdumpdef(FILE *f,double *mm,SomeData *obj,char **error_ret,int &warning)
{

	if (!strcmp(obj->whattype(),"Group")) {
		Group *g=dynamic_cast<Group *>(obj);
		for (int c=0; c<g->n(); c++) 
			svgdumpdef(f,NULL,g->e(c),error_ret,warning); 
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

			fprintf(f,"      <feGaussianBlur in=\"SourceAlpha\" stdDeviation=\".5\"/>\n");

			fprintf(f,"    </filter>\n");
		}
	}
	return 0;
}


////--------------------------------*************
//class SvgFilterConfig
//{
// public:
//	char untranslatables;   //whether laidout objects not suitable for svg should be ignored, rasterized, or approximated
//	char dont_clobber_file;
//	char plain;             //akin to inkscapes plain vs. inkscape svg?
//	char preserveunknown;   //any unknown attributes should be attached as "metadata" to the object in question on readin
//							//these would potentially be written back out on an svg export?
//							
//	StyleDef *OutputStyleDef(); //for auto config dialog creation
//	StyleDef *InputStyleDef();
//};
////--------------------------------


//! Save the document as SVG.
/*! This only saves images, groups, linear and radial gradients, and the page size and orientation.
 * Files are not checked for existence. They are clobbered if they already exist, and are writable.
 *
 * Return 0 for success, 1 for error and nothing written, 2 for error, and corrupted file possibly written.
 * 2 is mainly for debugging purposes, and will be perhaps be removed in the future.
 *
 * error_ret is appended to if possible.
 * 
 * \todo *** should have option of rasterizing or approximating the things not supported in svg, such 
 *    as patch gradients
 */
int SvgOutputFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	Document *doc =out->doc;
	int start     =out->start;
	//int end       =out->end;
	int layout    =out->layout;
	Group *limbo  =out->limbo;
	PaperGroup *papergroup=out->papergroup;
	if (!filename) filename=out->filename;
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paper)...
		if (error_ret) appendline(*error_ret,_("Nothing to export!"));
		return 1;
	}
	
	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			if (error_ret) appendline(*error_ret,_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".svg");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,error_ret);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
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

	if (doc) spread=doc->docstyle->imposition->Layout(layout,start);
	
	 // write out header
	double height;
	if (spread) height=spread->path->maxy-spread->path->miny;
	else height=papergroup->papers.e[0]->box->paperstyle->h();

	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	fprintf(f,"<!-- Created with Laidout, http://www.laidout.org -->\n");
	fprintf(f,"<svg \n"
			  "     xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
			  "     xmlns=\"http://www.w3.org/2000/svg\"\n"
			  "     xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
			  "     version=\"1.0\"\n");
	fprintf(f,"     width=\"%fin\"\n", spread->path->maxx-spread->path->minx); //***inches by default?
	fprintf(f,"     height=\"%fin\"\n", height);
	fprintf(f,"   >\n");
			

	 //write out global defs section
	fprintf(f,"  <defs>\n");
	//*************** gradients and such

	 //dump out defs for limbo objects if any
	if (limbo && limbo->n()) {
		svgdumpdef(f,m,limbo,error_ret,warning);
	}

	if (spread) {
		if (spread->marks) svgdumpdef(f,m,spread->marks,error_ret,warning);

		 // for each page in spread..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
				for (c3=0; c3<g->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					svgdumpdef(f,m,g->e(c3),error_ret,warning);
				}
			}
		}
	}
	fprintf(f,"  </defs>\n");
			
	
	 // Write out objects....

	 //transform to paper * conversion to left handed system, and 1/90th of an inch per unit
	transform_set(mm,90,0,0,-90,0,height*72*1.25);
	transform_invert(mmm,papergroup->papers.e[0]->m());
	transform_mult(m,mmm,mm);
	fprintf(f,"  <g transform=\"matrix(90,0,0,-90, 0,%f)\">\n", height*72*1.25);

	 //dump out limbo objects if any
	if (limbo && limbo->n()) {
		transform_set(m,1,0,0,1,0,0);
		svgdumpobj(f,m,limbo,error_ret,warning,4);
	}


	if (spread) {
		 //write out printer marks
		transform_set(m,1,0,0,1,0,0);
		if (spread->marks) svgdumpobj(f,m,spread->marks,error_ret,warning,4);

		 // for each page in spread..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
				for (c3=0; c3<g->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					svgdumpobj(f,m,g->e(c3),error_ret,warning,4);
				}
			}
		}
	}

	delete spread;
	fprintf(f,"  </g>\n"); //from unit correction and paper

//-------or----------adapt to multipagesvg with page and pagesets
//	 // Write out spread....
//	if (version==1.2) {
//		fprintf(f,"  <pageSet>\n");
//		*********needs work
//		if (doc) {
//			for (c=0; c<doc->docstyle->imposition->numpapers; c++) {
//				fprintf(f,"    <page>\n");
//				fprintf(f,"      <g>\n");
//				spread=doc->docstyle->imposition->PaperLayout(c);
//				 // for each page in paper layout..
//				for (c2=0; c2<spread->pagestack.n; c2++) {
//					pg=spread->pagestack.e[c2]->index;
//					if (pg>=doc->pages.n) continue;
//					 // for each layer on the page..
//					for (l=0; l<doc->pages[pg]->layers.n(); l++) {
//						 // for each object in layer
//						g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
//						for (c3=0; c3<g->n(); c3++) {
//							transform_copy(m,spread->pagestack.e[c2]->outline->m());
//							svgdumpobj(f,m,g->e(c3));
//						}
//					}
//				}
//
//				delete spread;
//				fprintf(f,"      </g>\n");
//				fprintf(f,"    </page>\n");
//			}
//			fprintf(f,"    </pageSet>\n");
//		}
//	}
//--------------------
		
	 // write out footer
	fprintf(f,"</svg>\n");
	
	fclose(f);
	setlocale(LC_ALL,"");
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

//***********forward declaration:
int svgDumpInObjects(Group *group, Attribute *element, char **error_ret);

int SvgImportFilter::In(const char *file, Laxkit::anObject *context, char **error_ret)
{
	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	Document *doc=in->doc;

	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
	if (!att) return 2;
	
	 //create repository for hints if necessary
	Attribute *svghints=NULL,
			  *svg=NULL; //points to the "svg" section of svghints. Do not delete!!
	if (in->keepmystery) svghints=new Attribute(VersionName(),file);
	try {

		 //add xml preamble, and anything not under "svg" to hints if it exists...
		if (svghints) {
			for (int c=0; c<att->attributes.n; att++) {
				if (!strcmp(att->attributes.e[c]->name,"svg")) continue;
				svghints->push(att->attributes.e[c]->duplicate(),-1);
			}
			svg=new Attribute("svg",NULL);
			svghints->push(svg,-1);
		}

		int c;
		double width=0, height=0;
		Attribute *svgdoc=att->find("svg"),
				  *version;
		if (!svgdoc) throw 3;

		for (int c=0; c<svgdoc->attributes.n; att++) {
			if (!strcmp(svgdoc->attributes.e[c]->name,"content:")) continue;

			if (svghints) svg->push(svgdoc->attributes.e[c]->duplicate(),-1);
			if (!strcmp(svgdoc->attributes.e[c]->name,"width"))
				DoubleAttribute(svgdoc->attributes.e[c]->value,&width);
			else if (!strcmp(svgdoc->attributes.e[c]->name,"height"))
				DoubleAttribute(svgdoc->attributes.e[c]->value,&height);
		}
		svgdoc=svgdoc->find("content:");
		if (!svgdoc || width==0 || height==0) throw 4;
		
		 //svg units == 1.2 * (postscript units == 72 / inch)
		width*=.8/72; //convert width and height to inches *** use postscript units instead??
		height*=.8/72;

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
		if (!paper) paper=laidout->papersizes.e[0];
		
		 //pagenum to start dumping onto
		int docpagenum=in->topage; //the page in laidout doc to start dumping into
		int pagenum,
			curdocpage; //the current page in the laidout document, used in loop below
		if (docpagenum<0) docpagenum=0;

		 //preliminary start and end pages for the svg
		int numpages=0;
		int start,end;
		if (in->instart<0) start=0; else start=in->instart;
		if (in->inend<0) end=10000000; 
			else end=in->inend;



		 //now svgdoc's subattributes should be a combination of 
		 //defs, sodipodi:namedview, metadata,  and from svg 1.2: pageSet, masterPage
		 // PLUS any number of graphic elements, such as g, rect, image, text....

		 //create the document if necessary
		if (!doc && !in->toobj) {
			Imposition *imp=new Singles;
			paper->flags=((paper->flags)&~1)|(landscape?1:0);//***note this changes the default paper flag!!
			imp->SetPaperSize(paper);
			DocumentStyle *docstyle=new DocumentStyle(imp);
			doc=new Document(docstyle,Untitled_name());
		}

		Group *group=in->toobj;
		Attribute *page,*object;
		char scratch[50];
		MysteryData *mdata=NULL;

		if (!group && doc) {
			 //update group to point to the document page's group
			curdocpage=docpagenum+(pagenum-start);
			group=dynamic_cast<Group *>(doc->pages.e[curdocpage]->layers.e(0)); //pick layer 0 of the page
		}

		for (c=0; c<svgdoc->attributes.n; c++) {
			if (!strcmp(svgdoc->attributes.e[c]->name,"metadata")
				     || !strcmp(svgdoc->attributes.e[c]->name,"sodipodi:namedview")) {
				 //just copy over "metedata" and "sodipodi:namedview" to svghints
				if (svghints) {
					svg->push(svgdoc->attributes.e[c]->duplicate(),-1);
				}
				continue;

			} else if (!strcmp(svgdoc->attributes.e[c]->name,"defs")) {
				 //need to read in gradient and filter data...
				//***
				continue;

			} else if (!strcmp(svgdoc->attributes.e[c]->name,"masterPage")) {
				 //masterPages are printed on any page that (somehow!) refers to them...
				//***not sure how to use these!!
				//contains g elements...
				continue;

			} else if (!strcmp(svgdoc->attributes.e[c]->name,"pageSet")) {
				 //in Svg 1.2, "pageSet"s contain "page" elements in "content:"
				//***
				continue;

			} 
			
			if (svgDumpInObjects(group,svgdoc->attributes.e[c],error_ret)) continue;

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

//! Return 1 for attribute used, else 0.
int svgDumpInObjects(Group *group, Attribute *element, char **error_ret)
{
	if (!strcmp(element->name,"g")) {
		Group *g=new Group;
		for (int c=0; c<element->attributes.n; c++) {
			svgDumpInObjects(g,element->attributes.e[c],error_ret);
		}
		group->push(g,0);
		return 1;

	} else if (!strcmp(element->name,"path")) {
		for (int c=0; c<element->attributes.n; c++) {
			if (!strcmp(element->attributes.e[c]->name,"id")) {
			} else if (!strcmp(element->attributes.e[c]->name,"transform")) {
			} else if (!strcmp(element->attributes.e[c]->name,"style")) {
			} else if (!strcmp(element->attributes.e[c]->name,"d")) {
			}
		}
	} else if (!strcmp(element->name,"rect")) {
	} else if (!strcmp(element->name,"circle")) {
	} else if (!strcmp(element->name,"ellipse")) {
	} else if (!strcmp(element->name,"line")) {
	} else if (!strcmp(element->name,"polyline")) {
	} else if (!strcmp(element->name,"polygon")) {
	} else if (!strcmp(element->name,"text")) {
		for (int c=0; c<element->attributes.n; c++) {
			if (!strcmp(element->attributes.e[c]->name,"id")) {
			} else if (!strcmp(element->attributes.e[c]->name,"transform")) {
			} else if (!strcmp(element->attributes.e[c]->name,"style")) {
			} else if (!strcmp(element->attributes.e[c]->name,"x")) {
			} else if (!strcmp(element->attributes.e[c]->name,"y")) {
			} else if (!strcmp(element->attributes.e[c]->name,"content:")) {
			}
		}
	} else if (!strcmp(element->name,"image")) {
		for (int c=0; c<element->attributes.n; c++) {
			if (!strcmp(element->attributes.e[c]->name,"id")) {
			} else if (!strcmp(element->attributes.e[c]->name,"x")) {
			} else if (!strcmp(element->attributes.e[c]->name,"y")) {
			} else if (!strcmp(element->attributes.e[c]->name,"width")) {
			} else if (!strcmp(element->attributes.e[c]->name,"height")) {
			} else if (!strcmp(element->attributes.e[c]->name,"xlink:href")) {
			} else if (!strcmp(element->attributes.e[c]->name,"transform")) {
			}
		}
	} else if (!strcmp(element->name,"use")) {
	}

	return 0;
}

