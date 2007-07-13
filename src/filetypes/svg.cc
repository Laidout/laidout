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


#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "svg.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"

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
	
	//SvgInputFilter *svgin=new SvgInputFilter;
	//laidout->importfilters(svgin);
}


//------------------------------------ SvgOutputFilter ----------------------------------
	
/*! \class SvgOutputFilter
 * \brief Filter for exporting SVG 1.0.
 *
 * \todo implement PAGE and PAGESET elements of draft svg 1.2
 */


SvgOutputFilter::SvgOutputFilter()
{
	//flags=FILTERS_MULTIPAGE; //***not multipage yet!
}

const char *SvgOutputFilter::VersionName()
{
	return _("Svg 1.0");
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
		appendstr(*error_ret,_("Cannot export Eps objects into svg.\n"));
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
		appendstr(*error_ret,_("Warning: interpolating a color patch object\n"));
		warning++;
		//---------
		//if (***config allows it) {
		if (1) {
			 //approximate gradient with svg elements
			ColorPatchData *patch=dynamic_cast<ColorPatchData *>(obj);
			if (!patch) return 0;

			 //make a group with a mask of outline of original patch, and blur filter
			fprintf(f,"%s<g transform=\"matrix(%.10g %.10g %.10g %.10g %.10g %.10g)\" \n",
				     spc, obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5));
			fprintf(f,"%s   clip-path=\"url(#colorPatchMask%ld)\" filter=\"url(#patchBlur%ld)\">\n", 
						spc, patch->object_id, patch->object_id);

			 //for each subpatch, break down into many sub-rectangles
			int c,r, cc,rr;
			int numdiv=5;
			flatpoint p[4];
			XRenderColor color;
			double s,ds, t,dt;
			ds=1./(patch->xsize/3)/numdiv;
			dt=1./(patch->ysize/3)/numdiv;
			double fudge=1.05; //so there are no transparent boundaries between the divided up rects
			for (r=0; r<patch->ysize/3; r++) {
			  for (c=0; c<patch->xsize/3; c++) {
			    for (rr=0; rr<numdiv; rr++) {
			      for (cc=0; cc<numdiv; cc++) {
					s=(c+(float)cc/numdiv)/(patch->xsize/3);
					t=(r+(float)rr/numdiv)/(patch->ysize/3);
					DBG cerr <<" point s,t:"<<s<<','<<t<<endl;

				     //get color for point (r+rr,c+cc)
					patch->getColor(s/(1-ds),t/(1-dt),&color);

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
		appendstr(*error_ret,_("Cannot export Image Patch objects into svg.\n"));
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
 * \todo *** should have option of rasterizing or approximating the things not supported in svg, such 
 *    as patch gradients
 */
int SvgOutputFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	//int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->filename;
	
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paper) return 1;
	
	FILE *f=NULL;
	char *file;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			DBG cerr <<"**** cannot save, doc->saveas is null."<<endl;
			*error_ret=newstr(_("Cannot save to SVG without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".svg");
	} else file=newstr(filename);
	
	f=fopen(file,"w");
	delete[] file; file=NULL;

	if (!f) {
		DBG cerr <<"**** cannot save, doc->saveas cannot be opened for writing."<<endl;
		*error_ret=newstr(_("Error opening file for writing."));
		return 3;
	}

	int warning=0;
	Spread *spread;
	Group *g;
	double m[6];
	//int c;
	int c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);

	if (start<0) start=0;
	else if (start>doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	spread=doc->docstyle->imposition->Layout(layout,start);
	
	
	 // write out header
	double height=spread->path->maxy-spread->path->miny;
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
	fprintf(f,"  </defs>\n");
			
	
	 // Write out spread....
	fprintf(f,"  <g transform=\"matrix(90,0,0,-90, 0,%f)\">\n", height*72*1.25);

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

	delete spread;
	fprintf(f,"  </g>\n");
//-------or----------adapt to multipagesvg with page and pagesets
//	 // Write out spread....
//	fprintf(f,"  <g>\n");
//	*********
//	for (c=0; c<doc->docstyle->imposition->numpapers; c++) {
//		fprintf(f,"  <page>\n");
//		spread=doc->docstyle->imposition->PaperLayout(c);
//		 // for each page in paper layout..
//		for (c2=0; c2<spread->pagestack.n; c2++) {
//			pg=spread->pagestack.e[c2]->index;
//			if (pg>=doc->pages.n) continue;
//			 // for each layer on the page..
//			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
//				 // for each object in layer
//				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
//				for (c3=0; c3<g->n(); c3++) {
//					transform_copy(m,spread->pagestack.e[c2]->outline->m());
//					svgdumpobj(f,m,g->e(c3));
//				}
//			}
//		}
//
//		delete spread;
//		fprintf(f,"  </page>\n");
//	}
//	fprintf(f,"  </g>\n");
//--------------------
		
	 // write out footer
	fprintf(f,"</svg>\n");
	
	fclose(f);
	return 0;
	
}

////------------------------------------ SvgInputFilter ----------------------------------
///*! \class SvgInputFileFilter
// * \brief Filter to, amazingly enough, import svg files.
// */
//
//class SvgInputFilter
//{
// public:
//	virtual ~FileFilter() {}
//	virtual const char *Author() = 0;
//	virtual const char *FilterVersion() = 0;
//	
//	virtual const char *DefaultExtension() { return "svg"; }
//	virtual const char *Format() = 0;
//	virtual const char **FormatVersions(int *n) = 0;
//	virtual const char *VersionName(const char *version) = 0;
//	virtual const char *FilterClass() = 0;
//
//	virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
//	
//	
//	virtual const char *FileType(const char *first100bytes) = 0;
//	virtual int In(const char *file, Laxkit::anObject *context) = 0;
//};
//
//
//
////-----------------------------------------------------------
//
////! Import an SVG file.
///*! If doc!=NULL, then import the svg file to Document starting at page startpage.
// * Otherwise, create a brand new Singles based document.
// *
// * Does no check on the file to ensure that it is in fact an svg file.
// *
// * It will be a file something like:
// * <pre>
// *   ??????
// * </pre>
// *
// * \todo ***** finish imp me!
// * \todo there should be a way to preserve any elements that laidout doesn't understand, so
// *   when outputting as svg, these elements would be written back out maybe...
// */
//Document *svgin(const char *file,Document *doc,int startpage,char **error_ret)
//{
//	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
//	if (!att) {
//		if (error_ret) error_ret=newstr(_("Could not open file for reading."));
//		return NULL;
//	}
//	
//	int c;
//	Attribute *svgdoc=att->find("svg"),
//			  *page, *frame, *a;
//	if (!svgdoc) {
//		delete att; 
//		if (error_ret) error_ret=newstr(_("Could not open file for reading."));
//		return NULL; 
//	}
//	
//	 //figure out the paper size, orientation
//	a=svgdoc->find("width");  //8.5in
//	a=svgdoc->find("height"); //11in...
//
//	 // create doc if not exist already with specified dimensions
//	a=svgdoc->find("paper_name");
//	PaperStyle *paper=NULL;
//	if (a) {
//		for (c=0; c<laidout->papersizes.n; c++)
//			if (!strcasecmp(laidout->papersizes.e[c]->name,a->value)) {
//				paper=laidout->papersizes.e[c];
//				break;
//			}
//	}
//	if (!paper) paper=laidout->papersizes.e[0];
//
//	
//	 //figure out orientation
//	int landscape;
//	a=svgdoc->find("landscape");
//	if (a) landscape=BooleanAttribute(a->value);
//	else landscape=0;
//	
//	
//	 // read in defs, which normally includes gradients and other globals...
//	a=svgdoc->find("defs");
//	
//
//	
//	 // read in pages
//	int pagenum=0;
//	svgdoc=pptdoc->find("contents");
//	if (!svgdoc) { delete att; return NULL; }
//
//	 //create the document
//	if (!doc) {
//		Imposition *imp=new Singles;
//		imp->SetPaperSize(paper);
//		imp->paperstyle->flags=((imp->paperstyle->flags)&~1)|(landscape?1:0);
//		DocumentStyle *docstyle=new DocumentStyle(imp);
//		doc=new Document(docstyle,"untitled");//**** laidout should keep track of: untitled1, untitled2, ...
//	}
//
//	ImageData *image;
//	LaxImage *img=NULL;
//	Attribute *t,*n,*m;
//	double M[6];
//	
//	for (c=0; c<svgdoc->attributes.n; c++) {
//		if (!strcmp(svgdoc->attributes.e[c]->name,"page")) {
//			if (pagenum>doc->pages.n) doc->NewPages(-1,1);
//			page=svgdoc->attributes.e[c];
//			for (int c2=0; c2<page->attributes.n; c2++) {
//				if (!strcmp(page->attributes.e[c]->name,"frame")) {
//					frame=page->attributes.e[c];
//					a=frame->find("file");
//					t=frame->find("type");
//					n=frame->find("name");
//					m=frame->find("matrix");
//					if (a && a->value && t && !strcmp(t->value,"raster")) {
//						img=load_image(a->value);
//						if (img) {
//							image=new ImageData;
//							if (n) image->SetDescription(n->value);
//							image->SetImage(img);
//							if (m) DoubleListAttribute(m->value,M,6,NULL);
//							dynamic_cast<Group *>(doc->pages.e[pagenum]->layers.e(0))->push(image,0);
//							image->dec_count();
//						}
//					}
//				}
//			}
//			pagenum++;
//		}
//	}
//	
//	 //*** set up page labels for "first_page_num"
//	
//	 //establish doc in project
//	laidout->project->docs.push(doc);
//	laidout->app->addwindow(newHeadWindow(doc));
//	
//	delete att;
//	return doc;
//}
