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
// Copyright (C) 2011-2012 by Tom Lechner
//




#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/svgcoord.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/language.h>

#include "poly.h"
#include "nets.h"
#include "polyrender.h"
#include <lax/lists.cc>

#include <fstream>

#include <iostream>
#define DBG 


using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;
using namespace Magick;



namespace Polyptych {



//--------------------------- PaperBound -------------------------------
/*! \class PaperBound
 * \brief Simple class to hold info about laying out on papers.
 */

PaperBound::PaperBound()
{
	id=-1;
	name=NULL;
	width=height=0;
	units=NULL;
}

PaperBound::PaperBound(const char *nname,double w,double h,const char *unit)
{
	id=-1;
	name=newstr(nname);
	width=w;
	height=h;
	units=newstr(unit);
}

PaperBound::PaperBound(const PaperBound &p)
{
	id=p.id;
	name=newstr(p.name);
	width=p.width;
	height=p.height;
	units=newstr(p.units);
}

PaperBound::~PaperBound()
{
	if (name) delete[] name;
	if (units) delete[] units;
}

PaperBound &PaperBound::operator=(PaperBound &p)
{
	id=p.id;
	makestr(name,p.name);
	width=p.width;
	height=p.height;
	makestr(units,p.units);
	return p;
}

//------------------------ RenderConfig -------------------------------
/*! \class RenderConfig
 * \brief Hold info pertaining to rendering.
 */

RenderConfig::RenderConfig()
{
	poly=NULL;
	net=NULL;
	nets=NULL;
	numnets=0;
	maxwidth=500;
	filebase=newstr("render");
	output=OUT_SVG;
	oversample=3;
	generate_images=1;

	spherefile=NULL;
	//spheremap=NULL;
	//destination=NULL;
}

RenderConfig::~RenderConfig()
{
	if (filebase)   delete[] filebase;
	if (spherefile) delete[] spherefile;
	//if (spheremap) delete spheremap;
	//if (destination) delete destination;
}


//--------------------------------- SphereMapper ------------------------------------

/*! \class SphereMapper
 * Class to encapsulate rendering to and from equirectangular images.
 */
SphereMapper::SphereMapper()
{
	config=new RenderConfig;
}

SphereMapper::~SphereMapper()
{
	if (config) config->dec_count();
}



//------------------------------------ ImageToSphere() -----------------------------------

int SphereMapper::ImageToSphere(Magick::Image image,
				 int sx,int sy,int sw,int sh, //!< Subset the source image
				 double x_fov, double y_fov, //!< Scaling to apply to the source image before mapping, in radians. <=0 for no extra.
				 double theta, //!< Starting from image center being at (1,0,0), rotate by theta radians around the z axis
				 double gamma, //!< After theta, rotate by gamma radians toward the z axis, around an axis in the xy plane, through the origin
				 double rotation, //!< Once offset by theta and gamma, rotate the image by this many radians before mapping

				 const char *tofile, //!< If not null, then save to this file. Else only render to config->spheremap.
				 int oversample, //!< oversample*oversample per image pixel. 3 seems pretty good in practice. -1 for default
				 Laxkit::ErrorLog *log   //!< Some descriptive error message. NULL means you'll ignore any return message.
				)
{
	cerr <<" ImageToSphere(): TO TODO!"<<endl;
	
	int AA=oversample;
	if (AA<=0) AA=config->oversample;
	if (AA<=0) AA=1;

	//int spherewidth =config->spheremap.columns();
	//int sphereheight=config->spheremap.rows();

	return 1;
}

/*! Given a polyhedron, render a wirefram version to config->spheremap, where that sphere is the unit sphere centered
 * at the origin.
 *
 * If necessary, you should have called poly.makeedges() before running this.
 */
int SphereMapper::PolyWireframeToSphere(const char *tofile, //!< If not null, save to this file in addition to rendering to config->spheremap
				 double red,double green,double blue, double alpha, //!< color of the wireframe
				 double edge_width, //!< One unit is one pixel at equator of equirectangular config->spheremap
				 Polyhedron &poly,
				 int oversample,      //!< oversample*oversample per image pixel. 3 seems pretty good in practice.
				 Basis *extra_basis, //!< How to rotate the hedron before putting on the image
				 Laxkit::ErrorLog *log   //!< Some descriptive error message. NULL means you'll ignore any return message.
				)
{

	//*** TO DO
	cerr <<" PolyWireframeToSphere(): TO TODO!"<<endl;

	ColorRGB color,color2;
	Image wireimage;
	wireimage.depth(8);
	wireimage.magick("TIFF");
	wireimage.matte(true);

	color.alpha(alpha);
	color.red(red);
	color.blue(blue);
	color.green(green);
	wireimage.fillColor(color);
	wireimage.strokeColor(color);
	wireimage.strokeWidth(2);
	//wireimage.draw(DrawablePolygon(pgonpoints));

	//double rq,gq,bq;        //rgb are doubles

//	int AA=1;
//	double aai=1./AA;
//	double xx[AA],yy[AA],r;

	spacepoint p1,pm,p2;
//	double theta1, gamma1;
//	double theta2, gamma2;
//	double r;
	
	for (int c=0; c<poly.edges.n; c++) {
		 //draw a line for each edge in the hedron
//		p1=poly.vertices[poly.edges[c]->p1];
//		p2=poly.vertices[poly.edges[c]->p2];
//		
//
//		if (extra_basis) {
//			p1=extra_basis->transformTo(p1);
//			p2=extra_basis->transformTo(p2);
//			pm=(p1+p2)/2;
//		}
//
//		 //transform (x,y,z) -> (theta,gamma) -> (sx,sy)
//		r=sqrt(p1.x*p1.x+p1.y*p1.y);
//		theta1=atan2(p1.y,p1.x);
//		gamma1=atan(p1.z/r);  
//
//		r=sqrt(p2.x*p2.x+p2.y*p2.y);
//		theta2=atan2(p2.y,p2.x);
//		gamma2=atan(p2.z/r);  

		DBG cerr <<"Edge "<<c<<": "<<p1.x<<','<<p1.y<<" -> "<<p2.x<<','<<p2.y<<endl;

		//sx=(theta1/M_PI+1) *spherewidth;
		//sy=(gamma1=M_PI+.5)*sphereheight
		//if (sx<0) sx=0;
		//else if (sx>=spherewidth) sx=spherewidth-1;
		//if (sy<0) sy=0;
		//else if (sy>=sphereheight) sy=sphereheight-1;
		//color=spheremap.pixelColor(sx,sy);
		//rq+=color.red();
		//gq+=color.green();
		//bq+=color.blue();

		// *** figure out how to draw from 1 to 2...

	}


	 //save the image somewhere..
	if (tofile) {
		cerr <<"writing wireframe to "<<tofile<<"..."<<endl;
		config->spheremap.write(tofile);
	}



	return 1; // ***
}


//--------------------------------- Net svg saving ----------------------------


/*! Same as Net::SaveSvg(), only assumes existing images with names like filename001.png
 * should be mapped to each net face, with the image number corresponding to that
 * polyhedron face index.
 *
 * filebase should be something like "filename###.png".
 */
int SaveSvgWithImages(Net *net,
					  const char *filename,
					  const char *filebase,
					  flatpoint *imagedims,
					  flatpoint *imageoffset,
					  double pixPerUnit,      //!< How many image pixels per unit in net space
				 	  int net_only,           //!< If true, draw the lines only, not the images
					  int num_papers,         //!< Number of predefined papers
					  PaperBound **papers,    //!< net->whichpaper points to papers in this stack
				 	  Laxkit::ErrorLog *log   //!< Some descriptive error message. NULL means you'll ignore any return message.
					)
{
	if (!net || !net->lines.n) return 1;

	ofstream svg(filename);
	if (!svg.is_open()) {
		cout <<"Problem opening "<<(filename?filename:"(no name)")<<"for writing."<<endl;
		return 1;
	}
	cerr <<"Writing SVG to "<<filename<<"..."<<endl;


	 // Define the transformation matrix: net to paper.
	 // For simplicity, we assume that the bounds in paper hold the size of the paper,
	 // so Letter is maxx=8.5, maxy=11, and minx=miny=0.
	 //
	 // bbox shall have comparable bounds. When paper is invalid, then bbox has the same
	 // size as the net, but the origin is shifted so that point (0,0) is at a corner
	 // of the net's bounding box.
	 //
	 // The net transforms into parent space, and the paper also transforms into parent space.
	 // So the transform from net to paper space is net.m()*paper.m()^-1
	double M[6]; 
	SomeData bbox;
	if (net->whichpaper>=0 && num_papers>0) {
		bbox.maxx=papers[net->whichpaper]->width;
		bbox.maxy=papers[net->whichpaper]->height;
		bbox.minx=bbox.miny=0;
	} else if (net->paper.validbounds()) {
		bbox.maxx=net->paper.maxx;
		bbox.minx=net->paper.minx;
		bbox.maxy=net->paper.maxy;
		bbox.miny=net->paper.miny;
	} else {
		bbox.maxx=net->maxx - net->minx;
		bbox.minx=0;
		bbox.maxy=net->maxy - net->miny;
		bbox.miny=0;
		bbox.m(net->m()); //map same as the net
		bbox.origin(flatpoint(net->minx,net->miny));      //...except for the origin
	}

	double t[6];
	transform_invert(t,bbox.m());
	transform_mult(M,net->m(),t);

	 //figure out decent line width scaling factor. *** this is a hack!! must use linestyle stuff.
	 //In SVG, 1in = 90px = 72pt
	double linewidth=.01; //inches, where 1 inch == 1 paper unit
	double scaling=1/sqrt(M[0]*M[0]+M[1]*M[1]); //supposed to scale to within the M group
	DBG cout <<"******--- Scaling="<<scaling<<endl;

	 //define some repeating header stuff
	char pathheader[400];
	sprintf(pathheader,"\t<path\n\t\tstyle=\"fill:none;fill-opacity:0.75;fill-rule:evenodd;stroke:#000000;stroke-width:%.6fpt;stroke-linecap:round;stroke-linejoin:round;stroke-opacity:.20;\"\n\t\t",scaling*linewidth);
	const char *pathclose="\n\t/>\n";
	
			
	 // Print out header
	svg << "<svg"<<endl
		<< "\twidth=\""<<bbox.maxx<<"in\"\n\theight=\""<<bbox.maxy<<"in\""<<endl
		<< "\txmlns:sodipodi=\"http://inkscape.sourceforge.net/DTD/sodipodi-0.dtd\""<<endl
		<< "\txmlns:xlink=\"http://www.w3.org/1999/xlink\""<<endl
		<<">"<<endl;
	
	 // Write matrix
	svg <<"\t<g transform=\"scale(90)\">"<<endl;
	//svg <<"\t<g >"<<endl;
	//svg <<"\t<g transform=\"matrix(1, 0, 0, 1, "<<M[4]<<','<<M[5]<<")\">"<<endl;
	svg <<"\t<g transform=\"matrix("<<M[0]<<','<<M[1]<<','<<M[2]<<','<<M[3]<<','<<M[4]<<','<<M[5]<<")\">"<<endl;


	
	 // ---------- draw lines 
	svg <<"<g>"<<endl;
	int c;
	NetLine *line;

	for (c=0; c<net->lines.n; c++) {
		line=net->lines.e[c];
		if (line->points) {
			svg << pathheader << "d=\"";
			char *d=CoordinateToSvg(line->points);

			//DBG cerr <<"--output line: "<<d<<endl;

			if (d) {
				svg <<d<<"\""<< pathclose <<endl;
				delete[] d;
			}
		}
	}
	svg <<"</g>"<<endl;

	 //-----------draw images
	if (!net_only) {
		 //coordinate system is net coordinates at this point
		char file[500];
		flatpoint xaxis, yaxis, point, p1, p2, p3, po;
		//double I[6];
		//transform_identity(I);
		int orig;
		for (c=0; c<net->faces.n; c++) {
			if (net->faces.e[c]->original<0) continue;
			orig=net->faces.e[c]->original;
		
			sprintf(file,"%s%03d.png",filebase,orig);

			p1=net->faces.e[c]->edges.e[0]->points->p();
			p2=net->faces.e[c]->edges.e[1]->points->p();
			p3=net->faces.e[c]->edges.e[2]->points->p();
			po=p1-=imageoffset[orig];
			//p3=faces.e[c]->edges.e[faces.e[c]->edges.n-1]->points->p();
			if (net->faces.e[c]->matrix) {
				p1=transform_point(net->faces.e[c]->matrix,p1);
				p2=transform_point(net->faces.e[c]->matrix,p2);
				p3=transform_point(net->faces.e[c]->matrix,p3);
				po=transform_point(net->faces.e[c]->matrix,po);
			}

			point =p1;
			xaxis =p2 - point;
			xaxis/=norm(xaxis);
			yaxis =(p3 - point) |= xaxis;
			yaxis/=norm(yaxis);
			//cout <<"*** face "<<c<<", angle x to y: "<<acos(xaxis*yaxis)*180/M_PI<<endl;

			xaxis/=pixPerUnit;
			yaxis/=pixPerUnit;

			//cout <<"in svg: imagedims["<<c<<"]= "<<imagedims[c].x<<" x "<<imagedims[c].y<<endl;

			//cout <<"writing "<<file<<"..."<<endl;
			svg <<"<image"<<endl
				<<"    xlink:href=\""<<file<<"\""<<endl
				<<"    y=\"0\""<<endl
				<<"    x=\"0\""<<endl
				<<"    width=\""<<imagedims[orig].x<<"\""<<endl
				<<"    height=\""<<imagedims[orig].y<<"\""<<endl
				<<"    transform=\"matrix("<<xaxis.x<<","
										   <<xaxis.y<<","
										   <<yaxis.x<<","
										   <<yaxis.y<<","
										   <<po.x<<","
										   <<po.y<<")\""<<endl
				<<"  />"<<endl;
		}
	}

	 // Close the net grouping
	svg <<"\t</g>\n";
	svg <<"\t</g>\n";

	 // Print out footer
	svg << "\n</svg>\n";

	cout <<"Done writing SVG to "<<filename<<"..."<<endl;
	return 0;
}


//------------------------------------ SphereToPoly() -----------------------------------

/*! Return 0 for success, nonzero for error.
 *
 * If there is an error, then return some description of it in log.
 *
 */
int SphereToPoly(const char *spherefile,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,          //!< Render images no more than this many pixels wide
				 const char *filebase, //!< Say filebase="blah", files will be blah000.png, ...
				 int output,          //!< Output format
				 int oversample,      //!< oversample*oversample per image pixel. 3 seems pretty good in practice.
				 int generate_images, //!< Whether to actually render images, or just output new svg, for intance.
				 int net_only,       //!< Whether to just draw the lines, or include file names
				 Basis *extra_basis, //!< How to rotate the hedron before putting on the image
				 int num_papers,     //!< Number of predefined papers
				 PaperBound **papers, //!< net->whichpaper points to papers in this stack
				 Laxkit::ErrorLog *log   //!< Some descriptive error message. NULL means you'll ignore any return message.
				)
{
	Image sphere;
	if (!spherefile) generate_images=0;

	if (generate_images) try {
		sphere.read(spherefile);
	} catch (Exception &error_ ) {

		char errormes[300];
		errormes[0]=0;
		sprintf(errormes,"Error loading %s.",spherefile?spherefile:"(no file given)");

		cerr <<errormes<<endl;

		if (log) log->AddMessage(errormes, ERROR_Fail);
		return 1;
	}

	DBG cerr <<"\n"
	DBG 	 <<"Using sphere file:"<<spherefile<<endl
	DBG      <<"  filesize: "<<sphere.fileSize()/1024<<" kb"<<endl
	DBG 	 <<"     width: "<<sphere.baseColumns()<<endl
	DBG 	 <<"    height: "<<sphere.baseRows()<<endl;

	char *fbase=newstr(filebase);
	if (!fbase) {
		fbase=newstr(lax_basename(spherefile));
		char *ptr=strrchr(fbase,'.');
		if (ptr!=NULL && ptr!=fbase) *ptr='\0';
	}

	int status=SphereToPoly(sphere, poly, net, maxwidth, filebase, output, oversample, generate_images, net_only, extra_basis,
							num_papers,
							papers,
							log);
	if (fbase!=filebase) delete[] fbase;

	return status;
}

//! Create several small images, 1 per face of a polyhedron from a sphere map.
/*! Note that OUT_NONE renders the individual images, but does not create a final single file.
 *
 * If filebase is something like "blah####", then the '#' will be replaced by the image number.
 * The default is 3 characters for the number.
 *
 * \todo maxwidth is currently used here as the width of the first face image,
 *     not the actual max width of face images should probably do a max pixel area of image or something
 */
int SphereToPoly(Image spheremap,
				 Polyhedron *poly,
				 Net *net, 
				 int maxwidth,      //!< Render images no more than this many pixels wide
				 const char *filebase, //!< Say filebase="blah", files will be blah000.png, ...
				 int output,          //!< Output format
				 int oversample, //!< oversample*oversample per image pixel. 3 seems pretty good in practice.
				 int generate_images, //!< Whether to actually render images, or just output new svg, for intance.
				 int net_only,       //!< Whether to just draw the lines, or include file names
				 Basis *extra_basis, //!< How to rotate the hedron before putting on the image
				 int num_papers,     //!< Number of predefined papers
				 PaperBound **papers, //!< net->whichpaper points to papers in this stack
				 Laxkit::ErrorLog *log   //!< Some descriptive error message. NULL means you'll ignore any return message.
				)
{
	if (!poly || (output!=OUT_NONE && !net)) {
		if (log) {
			if (!poly) log->AddMessage(_("No polyhedron specified. Render failed."), ERROR_Fail);
			else log->AddMessage(_("No net specified. Render failed."), ERROR_Fail);
		}
		return 1;
	}

	int AA=oversample;
	int spherewidth=spheremap.columns(),
		sphereheight=spheremap.rows();

	char *fnumbase=make_filename_base(filebase);
	string filenumbase=fnumbase;
	delete[] fnumbase;

	 // *** truncates at first '#' for filenamebase. This could be more intelligent
	string filenamebase=filebase;
	size_t pos=filenamebase.find("#");
	if (pos!=string::npos) filenamebase=filenamebase.substr(0,pos);

	//figure out an orientation for the face in 3d.
	//
	// for each face of the polyhedron, create an image around the 
	// bounding box. foreach pixel in each image that is inside the face,
	// find the corresponding theta,gamma, and read off pixel from spheremap.
	// possibly oversample.


	ColorRGB color,color2;
	Image faceimage;
	faceimage.depth(8);
	faceimage.magick("TIFF");
	faceimage.matte(true);

	double pixperunit=-1; // net units
	int c, sx,sy, facei;
	DoubleBBox bbox;
	double width, height;
	int pixelwidth, pixelheight;

	Basis b;
	//unsigned int rq,gq,bq; //quantums are ints
	double rq,gq,bq;        //rgb are doubles

	double scale;
	double aai=1./AA;
	double xx[AA],yy[AA],r;

	PtrStack<Pgon> pgons;
	Pgon *pgon;
	spacepoint p;
	flatpoint netp,netx;
	flatpoint imagedims[poly->faces.n],imageoffset[poly->faces.n];
	char filename[300],scratch[500];
	int neti;
	
	for (c=0; c<poly->faces.n; c++) { //for each face...
		 //only render faces in the current net
		if (!net->findOriginalFace(c,1,0,&neti)) continue;
		
		 //find transform and bounding box for face in polyhedron
		 // b is basis of face:
		 //   b.p  is the first point of the face's outline
		 //   b.x  points to the second point from the first point
		 //   b.y  points perpendicularly away from b.x in the plane of points 0,1,2
		 //   b.z  is b.x (cross) b.y
		b.p=poly->vertices.e[poly->faces.e[c]->p[0]];
		b.x=poly->vertices.e[poly->faces.e[c]->p[1]] - b.p;
		b.x/=norm(b.x);
		b.y=(poly->vertices.e[poly->faces.e[c]->p[poly->faces.e[c]->pn-1]] - b.p) |= b.x;
		b.y/=norm(b.y);
		b.z=b.x / b.y;

		bbox.clear();
		pgon=new Pgon();
		pgon->setup(poly->faces.e[c]->pn,c);
		for (int c2=0; c2<poly->faces.e[c]->pn; c2++) {
			pgon->p[c2]=flatten(poly->vertices.e[poly->faces.e[c]->p[c2]],b);
			bbox.addtobounds(pgon->p[c2]);
		}

		if (pixperunit<0) pixperunit=maxwidth/(bbox.maxx-bbox.minx);

		width      =bbox.maxx-bbox.minx;
		height     =bbox.maxy-bbox.miny;
		pixelwidth =int((bbox.maxx-bbox.minx)*pixperunit);
		pixelheight=int((bbox.maxy-bbox.miny)*pixperunit);
		imagedims[c].x=pixelwidth;
		imagedims[c].y=pixelheight;
		imageoffset[c]=pgon->p[0]-flatpoint(bbox.minx,bbox.miny);
		//cout <<"imagedims["<<c<<"]= "<<imagedims[c].x<<" x "<<imagedims[c].y<<endl;

		if (generate_images) {
			faceimage.magick("TIFF");
			sprintf(scratch,"%dx%d",pixelwidth,pixelheight);
			faceimage.size(scratch);
			faceimage.read("xc:transparent");
			//faceimage.read("xc:#ff000000");
		}

		 //scale points so that they are in face image space
		for (int c2=0; c2<pgon->pn; c2++) {
			pgon->p[c2]=(pgon->p[c2]-flatpoint(bbox.minx,bbox.miny))/width*pixelwidth;
		}
		pgons.push(pgon,1); //add to list of pgons


		 //construct and draw the polygon mask
		if (generate_images) {
			std::list<Magick::Coordinate> pgonpoints;
			for (int c2=0; c2<pgon->pn; c2++) {
				pgonpoints.push_back(Magick::Coordinate(pgon->p[c2].x,pgon->p[c2].y));
			}
			color.alpha(0);
			color.red(1.0);
			color.blue(1.0);
			color.green(1.0);
			faceimage.fillColor(color);
			faceimage.strokeColor(color);
			faceimage.strokeWidth(2);
			faceimage.draw(DrawablePolygon(pgonpoints));
		}

		
		 // transform b.p to be at lower left of image
		 // and b.x and b.y to span the bounding box of the face image
		b.p+= bbox.minx*b.x + bbox.miny*b.y;
		b.x*=width;
		b.y*=height;
		
		
		 // for each pixel in face image, find corresponding point on sphere
		if (generate_images) for (int x=0; x<pixelwidth; x++) {
		  for (int c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/pixelwidth;

		  for (int y=0; y<pixelheight; y++) {
			for (int c2=0; c2<AA; c2++) yy[c2]=((double)y+aai*(c2+.5))/pixelheight;

			 //only work on pixels that are in the polygon
			color2=faceimage.pixelColor(x,y);
			if (color2.alpha()==1.) continue;

			for (int xa=0; xa<AA; xa++) {
			  for (int ya=0; ya<AA; ya++) {
				p=b.p+xx[xa]*b.x+yy[ya]*b.y;
				
				if (extra_basis) {
					p-=extra_basis->p;
					p=spacepoint(p*extra_basis->x,p*extra_basis->y,p*extra_basis->z);
				}

				 //transform (x,y,z) -> (sx,sy)
				r=sqrt(p.x*p.x+p.y*p.y);
				sx=(int)((atan2(p.y,p.x)/M_PI+1)/2*spherewidth); //theta
				sy=(int)((atan(p.z/r)/M_PI+.5)*sphereheight);   //gamma
				if (sx<0) sx=0;
				else if (sx>=spherewidth) sx=spherewidth-1;
				if (sy<0) sy=0;
				else if (sy>=sphereheight) sy=sphereheight-1;

				color=spheremap.pixelColor(sx,sy);
				rq+=color.red();
				gq+=color.green();
				bq+=color.blue();
			  }
			}
			rq/=AA*AA;
			gq/=AA*AA;
			bq/=AA*AA;
			if (rq>1) rq=1;
			if (gq>1) gq=1;
			if (bq>1) bq=1;
			color.red(rq);
			color.green(gq);
			color.blue(bq);

			//if (color2.alpha()!=1.) { //this is checked for above...
				color.alpha(0); //fully opaque
				faceimage.pixelColor(x,y,color);
			//}
		  }
		  //DBG cout <<"\\\n";
		}


		 //save the image somewhere..
		if (generate_images) {
			sprintf(filename,filenumbase.c_str(),c);
			cout <<"writing "<<filename<<" ("<<pixelwidth<<"x"<<pixelheight<<")..."<<endl;
			faceimage.write(filename);
		}

		if (output==OUT_IMAGE) {
			//***composite to master image
		}
	}

	 //lay out into net!!
	if (output==OUT_NONE) return 0;

	double m[6];

	if (output==OUT_LAIDOUT) {
		sprintf(filename,"%s.laidout",filenamebase.c_str());
		FILE *f=fopen(filename,"w");
		if (!f) {
			cerr <<"Could not open "<<filename<<"for writing."<<endl;
			return -1;
		}

		fprintf(f,"#Laidout 0.08 Document\n"
				  "docstyle\n"
				  "  imposition Net\n"
				  "    net\n");
		net->dump_out(f,6,0,NULL);

		for (int c=0; c<net->faces.n; c++) {
			facei=c; //***what if net has fewer faces than the poly??
			sprintf(filename,filenumbase.c_str(),facei);
			//faceimage.read(filename);
		

			netp=net->faces.e[c]->edges.e[0]->points->fp;
			netx=net->faces.e[c]->edges.e[1]->points->fp - netp;
			//netx/=norm(netx);
			//nety=(net->points[net->faces[c]->points[net->faces[c]->np-1]] - netp) |= nx;
			//nety/=norm(nety);

			scale=norm(netx) / norm(pgons.e[facei]->p[1] - pgons.e[facei]->p[0]);

			m[0]=scale;
			m[1]=0;
			m[2]=0;
			m[3]=scale;
			m[4]=pgons.e[facei]->p[0].x*scale;
			m[5]=pgons.e[facei]->p[0].y*scale;


			fprintf(f,"page %d\n",c);
			fprintf(f,"  layer\n"
					  "    visible\n"
					  "    prints\n");
			fprintf(f,"    object %d ImageData\n",c);

			fprintf(f,"      filename %s\n",filename);
			fprintf(f,"      width %d\n",(int)imagedims[c].x);
			fprintf(f,"      height %d\n",(int)imagedims[c].y);
			fprintf(f,"      matrix %f %f %f %f %f %f\n",
					m[0], m[1], m[2], m[3], m[4], m[5]);
		}
		fclose(f);

	} else if (output==OUT_SVG) {
		cerr <<"outputting svg...."<<endl;
		sprintf(filename,"%s.svg",filenamebase.c_str());

		return SaveSvgWithImages(net, filename,filenamebase.c_str(),imagedims,imageoffset,pixperunit,net_only,num_papers,papers,NULL);

	} else if (output==OUT_CUBE_MAP) {
		cerr <<"*** still need to implement cube image out!!"<<endl;
		//fclose(f);
		return 0;

	} else if (output==OUT_IMAGE) {
		sprintf(filename,"%s.tif",filenamebase.c_str());
		//FILE *f=fopen(filename,"w");
		cerr <<"*** still need to implement image out!!"<<endl;
		//fclose(f);
		return 0;

	} else if (output==OUT_QTVR) {
		sprintf(filename,"%s.mov",filenamebase.c_str());
		cerr <<"*** still need to implement qtvr out!!"<<endl;
		return 0;

	} else if (output==OUT_QTVR_FACES) { 
		cerr <<"*** still need to implement qtvrfaces out!!"<<endl;
		sprintf(filename,"%s.jpg",filenamebase.c_str());
		return 0;

	} else {
		cerr <<"*** outputting nothing!"<<endl;
	}


	return 0;
}

/*! Creates a compact cube representation of equirectangular image spheremap.
 * Writes a jpg to "tofile.jpg".
 *
 * faces in cube map will be:
 * <pre>
 *   5*            012
 *   4* 0 1 2  ->  345 (*rot 90ccw)
 *   3*
 * </pre>
 *
 */
int SphereToCubeMap(Magick::Image spheremap,
				 //spacepoint sphere_z,
				 //spacepoint sphere_x,
				 int defaultimagewidth, //of one side of the cube. final image is w*3 by w*2
				 const char *tofile,
				 int AA, //!< amount to oversample, default 2
				 const char *which, //!< default "012345", a list of which faces to remap
				 Laxkit::ErrorLog *log
				)
{

	int spherewidth=spheremap.columns();
	int sphereheight=spheremap.rows();

	Geometry geometry(defaultimagewidth*3,defaultimagewidth*2);
	//Color color;
	ColorRGB color;
	Image faceimage(geometry,color);
	faceimage.magick("TIFF");

	//Basis sphere_basis(spacepoint(0,0,0),sphere_z,sphere_x);;
	Basis sphere_basis;
	sphere_basis.x=rotate(sphere_basis.x, spacevector(0,0,1),M_PI*.75,0);
	sphere_basis.y=rotate(sphere_basis.y, spacevector(0,0,1),M_PI*.75,0);

	spacepoint p;
	//double theta,gamma;
	int c,x,y,sx,sy,c2,cx,cy;
	double aai=1./AA;
	double xx[AA],yy[AA],r;
	char filename[500],filenametiff[500];
	const char *filesuf="012345";

	//unsigned int rq,gq,bq;
	double rq,gq,bq;
	int offx, offy;

	for (c=0; c<6; c++) {
		offx=defaultimagewidth*(c%3);
		offy=defaultimagewidth*(c/3);

		if (!strchr(which,filesuf[c])) {
			cout <<"Skipping number "<<filesuf[c]<<endl;
			continue;
		}


		cout <<"Working on face #"<<c<<"..."<<endl;
		
		 // loop over each pixel width and height of target image for face
		for (x=0; x<defaultimagewidth; x++) {
			 //xx and yy are parts of coordinates in xyz space, range [-1,1]
			for (c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/defaultimagewidth*2-1;

			for (y=0; y<defaultimagewidth; y++) {
				for (c2=0; c2<AA; c2++) yy[c2]=((double)y+aai*(c2+.5))/defaultimagewidth*2-1;
				rq=gq=bq=0;

				 //for antialiasing, break down the pixel to a region AA x AA
				 //oversample to get pixel color
				for (cx=0; cx<AA; cx++) {
				  for (cy=0; cy<AA; cy++) {
					  
					 //transform (xx,yy) -> (x,y,z)
					 //const char *filesuf="021354";
					switch (c) {
						 //faces in cube map will be:
						 //  5*            012
						 //  4* 0 1 2  ->  345 (*rot 90ccw)
						 //  3*
						case 0: p=spacepoint(      1, xx[cx], yy[cy]); break;
						case 2: p=spacepoint(     -1,-xx[cx], yy[cy]); break; 
						case 1: p=spacepoint(-xx[cy],      1, yy[cy]); break;
						case 4: p=spacepoint( yy[cy],     -1, -xx[cx]); break;
						case 5: p=spacepoint( yy[cy], xx[cx],     -1); break;
						case 3: p=spacepoint( yy[cy], -xx[cx],      1); break;
					}
//					----------------
//					switch (c) {
//						case 0: p=spacepoint(      1, xx[cx], yy[cy]); break;
//						case 2: p=spacepoint(     -1,-xx[cx], yy[cy]); break;
//						case 1: p=spacepoint(-xx[cx],      1, yy[cy]); break;
//						case 3: p=spacepoint( xx[cx],     -1, yy[cy]); break;
//						case 5: p=spacepoint(-yy[cy], xx[cx],      1); break;
//						case 4: p=spacepoint( yy[cy], xx[cx],     -1); break;
//					}
					transform(p,sphere_basis);

					 //transform (x,y,z) -> (sx,sy)
					r=sqrt(p.x*p.x+p.y*p.y);
					sx=(int)((atan2(p.y,p.x)/M_PI+1)/2*spherewidth);
					sy=(int)((atan(p.z/r)/M_PI+.5)*sphereheight);
					if (sx<0) sx=0;
					else if (sx>=spherewidth) sx=spherewidth-1;
					if (sy<0) sy=0;
					else if (sy>=sphereheight) sy=sphereheight-1;
					
					//cout <<"p="<<p.x<<','<<p.y<<','<<p.z<<"  -->  "<<sx<<","<<sy<<endl;
					//cout <<x<<','<<y<<"  -->  "<<sx<<","<<sy<<endl;

					try { //using ColorRGB
						//cout <<".";
						color=spheremap.pixelColor(sx,sy);
						//cerr <<"alpha: "<<color.alpha()<<" ";
						if (color.alpha()>0) { //defaults to jpeg file, can't have alpha...
							rq=gq=bq=0;
							color.alpha(0);
							break;
						}

						rq+=color.red();
						gq+=color.green();
						bq+=color.blue();
					} catch (Exception &error_ ) {
						color.red(0);
						color.green(0);
						color.blue(0);
						cout <<"*";
					} catch (exception &error) {
						color.red(0);
						color.green(0);
						color.blue(0);
						cout <<"%";
					}
				  }
				}
				rq/=AA*AA;
				gq/=AA*AA;
				bq/=AA*AA;
				//cerr <<"r:"<<rq<<" g:"<<gq<<" b:"<<bq<<endl;
				//color.redQuantum(rq);
				//color.greenQuantum(gq);
				//color.blueQuantum(bq);
				color.red(rq);
				color.green(gq);
				color.blue(bq);

				 //finally write the pixel
				faceimage.pixelColor(x+offx,y+offy, color);

			}
		}

	}

	sprintf(filename,"%s.jpg",tofile);
	sprintf(filenametiff,"%s.tiff",tofile);

	 //save the image somewhere..
	//faceimage.compressType(LZWCompression);
	faceimage.depth(8);
	//faceimage.write(filenametiff);

	faceimage.magick("JPG");
	faceimage.quality(80);
	faceimage.write(filename);

	//cout <<"done with image"<<endl;

	//faceimage.magick("TIFF");
	
	cout <<"All done!"<<endl;
	return 0;
}

} //namespace Polyptych


