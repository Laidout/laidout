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
// Copyright (C) 2011 by Tom Lechner
//


// Map an equirectangular image onto a polyhedron.
// This currently depends on Graphicsmagick for image loading.


// **********NOTES
// for svg output, should have option of alpha mask internal to image, or using clip masks,
//  this is for easy integration of tabs, with printed image on tabs as well
//
// for no-generate, don't need poly file if net file supplied. Need only file prefix
//
// for rotation, specify which point is sphere should be at middle of which face, with extra rotation about that..
// ./spheretopoly --point-at 120.1,45.555,10,10.55
//

#include </usr/include/GraphicsMagick/Magick++.h>
#include <getopt.h>
#include <lax/strmanip.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/svgcoord.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include "poly.h"
#include "nets.h"
#include <lax/lists.cc>

#include <fstream>

#include <iostream>
#define DBG 


using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;
using namespace Magick;


#define MAXWIDTH    500

#define OUT_NONE       0
#define OUT_LAIDOUT    1
#define OUT_SVG        2
#define OUT_QTVR       3
#define OUT_QTVR_FACES 4
#define OUT_IMAGE      5

int AA=3; //***should be able to autocompute suitable value?

double pixPerUnit;
int generate_images=1;
basis *extra_basis=NULL;


//--------------------------------- Net2 ----------------------------
class Net2 : public Net
{
 public:
	int SaveSvgWithImages(const char *filename, const char *filebase, flatpoint *imagedims, flatpoint *imageoffset, char **error_ret);	
};

/*! Same as Net::SaveSvg(), only assumes images with names like filename001.png
 * should be mapped to each net face, with the image number corresponding to that
 * polyhedron face index.
 */

int Net2::SaveSvgWithImages(const char *filename, const char *filebase, flatpoint *imagedims, flatpoint *imageoffset, char **error_ret)
{
	if (!lines.n) return 1;

	ofstream svg(filename);
	if (!svg.is_open()) {
		cout <<"Problem opening "<<(filename?filename:"(no name)")<<"for writing."<<endl;
		return 1;
	}
	cout <<"Writing SVG to "<<filename<<"..."<<endl;

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
	if (paper.validbounds()) {
		bbox.maxx=paper.maxx;
		bbox.minx=paper.minx;
		bbox.maxy=paper.maxy;
		bbox.miny=paper.miny;
	} else {
		bbox.maxx=maxx-minx;
		bbox.minx=0;
		bbox.maxy=maxy-miny;
		bbox.miny=0;
		transform_copy(bbox.m(),m()); //map same as the net
		bbox.origin(flatpoint(minx,miny));      //...except for the origin
	}

	double t[6];
	transform_invert(t,bbox.m());
	transform_mult(M,m(),t);

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

	for (c=0; c<lines.n; c++) {
		line=lines.e[c];
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
	 //coordinate system is net coordinates at this point
	char file[500];
	flatpoint xaxis, yaxis, point, p1, p2, p3, po;
	//double I[6];
	//transform_identity(I);
	int orig;
	for (c=0; c<faces.n; c++) {
		if (faces.e[c]->original<0) continue;
		orig=faces.e[c]->original;
	
		sprintf(file,"%s%03d.png",filebase,orig);

		p1=faces.e[c]->edges.e[0]->points->p();
		p2=faces.e[c]->edges.e[1]->points->p();
		p3=faces.e[c]->edges.e[2]->points->p();
		po=p1-=imageoffset[orig];
		//p3=faces.e[c]->edges.e[faces.e[c]->edges.n-1]->points->p();
		if (faces.e[c]->matrix) {
			p1=transform_point(faces.e[c]->matrix,p1);
			p2=transform_point(faces.e[c]->matrix,p2);
			p3=transform_point(faces.e[c]->matrix,p3);
			po=transform_point(faces.e[c]->matrix,po);
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

	 // Close the net grouping
	svg <<"\t</g>\n";
	svg <<"\t</g>\n";

	 // Print out footer
	svg << "\n</svg>\n";

	cout <<"Done writing SVG to "<<filename<<"..."<<endl;
	return 0;
}


//------------------------------------ SphereToPoly() -----------------------------------

//! Create several small images, 1 per face of a polyhedron from a sphere map.
/*! \todo maxwidth is currently used here as the width of the first face image,
 *     not the actual max width of face images
 */
int SphereToPoly(Image spheremap,
				 Polyhedron *poly,
				 Net2 *net, 
				 double poletheta, //!< The theta coordinate of the sphere's north pole.
				 double polegamma,  //!< The gamma coordinate of the sphere's north pole.
				 int maxwidth,
				 const char *filebase,
				 int output,
				 basis *extra_basis
				)
{
	if (!poly || (output!=OUT_NONE && !net)) return 1;

	int spherewidth=spheremap.columns(),
		sphereheight=spheremap.rows();

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

	basis b;
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
	
	for (c=0; c<poly->faces.n; c++) { //for each face...
	//for (c=0; c<1; c++) { //for each face...
		
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
		for (int c2=0; c2<poly->faces.e[c]->pn; c2++) {
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
				color.alpha(0);
				faceimage.pixelColor(x,y,color);
			//}
		  }
		  //DBG cout <<"\\\n";
		}


		 //save the image somewhere..
		if (generate_images) {
			sprintf(filename,"%s%03d.png",filebase,c);
			cout <<"writing "<<filename<<"..."<<endl;
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
		sprintf(filename,"%s.laidout",filebase);
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
			sprintf(filename,"%s%03d.png",filebase,facei);
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
		cout <<"outputting svg...."<<endl;
		sprintf(filename,"%s.svg",filebase);
		pixPerUnit=pixperunit;
		return net->SaveSvgWithImages(filename,filebase,imagedims,imageoffset,NULL);

	} else if (output==OUT_IMAGE) {
		sprintf(filename,"%s.tiff",filebase);
		//FILE *f=fopen(filename,"w");
		cout <<"*** still need to implement image out!!"<<endl;
		//fclose(f);
		return 0;
	} else if (output==OUT_QTVR) {
		sprintf(filename,"%s.mov",filebase);
		cout <<"*** still need to implement qtvr out!!"<<endl;
		return 0;
	} else if (output==OUT_QTVR_FACES) { 
		cout <<"*** still need to implement qtvrfaces out!!"<<endl;
		sprintf(filename,"%s.jpg",filebase);
		return 0;
	} else {
		cout <<"*** outputting nothing!"<<endl;
	}


	return 0;
}

//***better to use nona/hugin?
//Imlib_Image rollspheremap(double rotatetheta, double rotategamma, Imlib_Image oldsphere)
//{***
//}

//--------------------------------- main(), help(), version() ----------------------------

void help()
{
	cout << "\nspheretopoly  [options] spherefile.tiff"<<endl
		 << "\n"
		 << "Project an equirectangular sphere map onto a polyhedron."<<endl
		 << "Makes one png file for each face of the polyhedron, with the outline"<<endl
		 << "of each face acting as an alpha mask in the png. Also"<<endl
		 << "assembles these images into a file that can be printed out and built."<<endl
		 << "\n"
		 << "For the rotation and basis options below, the positive z axis points up in a\n"
		 << "sphere photo, positive x points to the middle of the left edge of the photo,\n"
		 << "positive y points to the point (w/4,h/2) where w and h are width and height\n"
		 << "of the photo, etc.\n"
		 << "\n"
		 << "  spherefile.tiff        An equirectangular sphere image"<<endl
		 << "  --rotate-X, -X 10      Rotate the sphere image around the X axis by this many degrees"<<endl
		 << "  --rotate-Y, -Y 10      Rotate the sphere image around the Y axis by this many degrees"<<endl
		 << "  --rotate-Z, -Z 10      Rotate the sphere image around the Z axis by this many degrees"<<endl
		 << "  --point-at-ang 1,2,3,4 In place of 1 and 2, put the lattitude and longitude in the"<<endl
		 << "              -L           sphere that should appear in the middle of the face listed in"<<endl
		 << "                           the 3rd spot. If 4 is provided, then add that rotation about that point."<<endl
		 //<< "                            If 5 is provided, then move the projection point toward the face by"<<endl
		 //<< "                            this percentage of the original distance to the face."<<endl
		 << "  --point-at-pix 1,2,3,4 Just like --point-at-ang, but use pixels of the sphere photo, not degrees."<<endl
		 << "              -P           The 4th number if any should be in degrees."<<endl
		 << "  --basis '(12 numbers)' 4 3-d coordinates for a basis for the sphere map: point, x,y,z"<<endl
		 << "  --fileprefix, -f name  Prefix for output png files, will be name01.png, etc."<<endl
		 << "                           Default is basename of sphere file'"<<endl
		 << "  --maxwidth, -w         The maximum pixel width for any face image. default is "<<MAXWIDTH<<"."<<endl
		 << "                           For image output, maxwidth is width of final image, not the"<<endl
		 << "                           individual faces."<<endl
		 << "  --output, -o           Format of assembled net. Currently, \"image\", \"svg\", \"laidout\","<<endl
		 << "                           or \"none\". Default is svg."<<endl
		 << "  --range, -r 1-4,10     When generating faces, which to generate. Default is all faces."<<endl
		 << "  --polyptych, -l file   Polyptych file containing info about net, polyhedron, image, orientation."<<endl
		 << "  --polyhedron, -p file  File containing a polyhedron. Default is a cube. The file"<<endl
		 << "                           format can be an OFF file, or a Laidout Polyhedron file."<<endl
		 << "  --net, -n file         File containing a net for the polyhedron"<<endl
		 << "  --no-generate, -N      Do not (re)generate images. Assume they exist already"<<endl
		 << "  --oversample, -s 2     This number squared is how many subpixels to compute for each actual pixel."<<endl
		 << "  --version, -v          Print out version of the program and exit"<<endl
		 << "  --help, -h             Print out this help and exit"<<endl
		;


	exit(0);

}

void version()
{
	cout <<"spheretopoly version 0.1"<<endl
		 <<"written by Tom Lechner, tomlechner.com"<<endl;
	exit(0);
}

//! Read in a Polyptych file, and set variables accordingly.
void setFromPolyptych(const char *polyptychfile, char *&imagefile, char *&polyhedronfile, char *&netfile, Net *net, basis *b)
{
	if (!polyptychfile) return;
	Attribute att;
	if (att.dump_in(polyptychfile,NULL)) return;
	char *name,*value;
	for (int c=0; c<att.attributes.n; c++) {
		name=att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;
		if (!strcmp(name,"polyhedronfile")) {
			makestr(polyhedronfile,value);
		} else if (!strcmp(name,"spherefile")) {
			makestr(imagefile,value);
		} else if (!strcmp(name,"netfile")) {
			makestr(netfile,value);
		} else if (!strcmp(name,"net")) {
			net->dump_in_atts(att.attributes.e[c],0,NULL);
		} else if (!strcmp(name,"basis")) {
			for (int c2=0; c2<att.attributes.e[c]->attributes.n; c2++) {
				name= att.attributes.e[c]->attributes.e[c2]->name;
				value=att.attributes.e[c]->attributes.e[c2]->value;
				double d[3];
				int n=DoubleListAttribute(value,d,3,NULL);
				if (n!=3) continue;
				if (!strcmp(name,"p")) {
					b->p=spacepoint(d[0],d[1],d[2]);
				} else if (!strcmp(name,"x")) {
					b->x=spacepoint(d[0],d[1],d[2]);
				} else if (!strcmp(name,"y")) {
					b->y=spacepoint(d[0],d[1],d[2]);
				} else if (!strcmp(name,"z")) {
					b->z=spacepoint(d[0],d[1],d[2]);
				}
			}
		}
	}
}

int main(int argc,char **argv)
{
	//cout <<"sizeof(Quantum)="<<sizeof(Quantum)<<endl<<endl;

	InitializeMagick(*argv);

	char *filename=NULL,
		 *filebase=NULL,
		 *netfile=NULL,
		 *polyhedronfile=NULL;
	int out=OUT_SVG;
	//int out=OUT_IMAGE;
	int maxwidth=MAXWIDTH;
	Net2 net;

	if (argc==1) { help(); }

	 // parse options
	static struct option long_options[] = {
			{ "basis",               1, 0, 'b' },
			{ "fileprefix",          1, 0, 'f' },
			{ "image",               1, 0, 'i' },
			{ "maxwidth",            1, 0, 'w' },
			{ "net",                 1, 0, 'n' },
			{ "no-generate",         0, 0, 'N' },
			{ "output",              1, 0, 'o' },
			{ "oversample",          1, 0, 's' },
			{ "polyhedron",          1, 0, 'p' },
			{ "polyptych",           1, 0, 'l' },
			{ "point-at-ang",        1, 0, 'L' },
			{ "point-at-pix",        1, 0, 'P' },
			{ "range",               1, 0, 'r' },
			{ "rotate-x",            1, 0, 'X' },
			{ "rotate-y",            1, 0, 'Y' },
			{ "rotate-z",            1, 0, 'Z' },
			{ "version",             0, 0, 'v' },
			{ "help",                0, 0, 'h' },
			{ 0,0,0,0 }
		};
	int c,index;

	int       figure_pointat=0, 
		      pointat_face;
	flatpoint pointat;
	double    pointat_rotation;

	while (1) {
		c=getopt_long(argc,argv,":P:L:r:X:Y:Z:i:l:b:f:w:o:p:n:s:Nvh",long_options,&index);
		if (c==-1) break;
		switch(c) {
			case ':': cerr <<"Missing parameter..."<<endl; exit(1); // missing parameter
			case '?': cerr <<"Unknown option"<<endl; exit(1);  // unknown option
			case 'h': help();    // Show usage summary, then exit
			case 'v': version(); // Show version info, then exit

			case 'f':
				makestr(filebase,optarg);
				break;
			case 'N': //no-generate
				generate_images=0;
				break;
			case 'w': 
				maxwidth=strtol(optarg,NULL,10);
				break;
			case 'o':
				if (!strcasecmp(optarg,"laidout")) out=OUT_LAIDOUT;
				else if (!strcasecmp(optarg,"image")) out=OUT_IMAGE;
				else if (!strcasecmp(optarg,"svg")) out=OUT_SVG;
				else if (!strcasecmp(optarg,"none")) out=OUT_NONE;
				else if (!strcasecmp(optarg,"qtvr")) out=OUT_QTVR;
				else if (!strcasecmp(optarg,"qtvrfaces")) out=OUT_QTVR_FACES;
				else {
					cerr<<"Unknown output format "<<optarg<<"!"<<endl;
					exit(1);
				}
				break;
			case 'l':
				if (!extra_basis) extra_basis=new basis;
				setFromPolyptych(optarg, filename, polyhedronfile, netfile, &net, extra_basis);
				break;
			case 'i':
				makestr(filename,optarg);
				break;
			case 'p':
				makestr(polyhedronfile,optarg);
				break;
			case 'n':
				makestr(netfile,optarg);
				break;
			case 's': 
				AA=strtol(optarg,NULL,10);
				if (AA<0) AA=2;
			    break;
			case 'X': { //rotate around X
				if (!extra_basis) extra_basis=new basis;
				double angle=strtod(optarg,NULL);
				if (angle!=0) rotate(*extra_basis,'x',angle,1.);
			  } break;
			case 'Y': { //rotate around Y
				if (!extra_basis) extra_basis=new basis;
				double angle=strtod(optarg,NULL);
				if (angle!=0) rotate(*extra_basis,'y',angle,1.);
			  } break;
			case 'Z': { //rotate around Z
				if (!extra_basis) extra_basis=new basis;
				double angle=strtod(optarg,NULL);
				if (angle!=0) rotate(*extra_basis,'z',angle,1.);
			  } break;
			case 'b': { //has basis of form "p,x,y,z"=="x,y,z, x,y,z, x,y,z, x,y,z"
				double l[12];
				char *p=optarg;
				while (p && *p) { if (*p==',') *p=' '; p++; }
				int c=LaxFiles::DoubleListAttribute(optarg,l,12,NULL);
				if (c!=12) {
					cerr<<"Not enough numbers for basis option! You need to supply 12 numbers,"<<endl
						<<"4 vectors with x, y, and z components. The order is the point,"<<endl
						<<"X axis, Y axis, then Z axis."<<endl;
					exit(1);
				}
				if (!extra_basis) extra_basis=new basis;
				extra_basis->p=spacepoint(l[ 0],l[ 1],l[ 2]);
				extra_basis->x=spacepoint(l[ 3],l[ 4],l[ 5]);
				extra_basis->y=spacepoint(l[ 6],l[ 7],l[ 8]);
				extra_basis->z=spacepoint(l[ 9],l[10],l[11]);
			  } break;
			case 'P': //point-at      "x,y, face, 
			case 'L': { //point-at-pix
				char *p=optarg;
				while (p && *p) { if (*p==',') *p=' '; p++; }

				figure_pointat=(c=='P'?2:1);
				double d[4];
				int n=LaxFiles::DoubleListAttribute(optarg,d,4,NULL);
				if (n<3) {
					cerr <<"Need more numbers for point-at:\n  x,y, (face number), (optional rotation)"<<endl;
					exit(1);
				}
				pointat.x=d[0];
				pointat.y=d[1];
				pointat_face=(int)(d[2]+.5);
				if (n>=4) pointat_rotation=d[3]/180*M_PI; else pointat_rotation=0;
			  } break;
			case 'r': { //range
//				char *p=optarg;
//				char *endp;
//				while (p && *p) { if (*p==',') *p=' '; p++; }
//				p=optarg;
//
//				int start,end;
//				int error=0;
//				while (p && *p) {
//					start=strtol(p,&endp,10);
//					if (endp==p) { error=1; break; }
//					p=endp;
//					while (isspace(*p)) p++;
//					if (*p=='-') {
//						p++;
//						end=strtol(p,&endp,10);
//						if (endp==p) { error=1; break; }
//						p=endp;
//					} else end=start;
//					if (end<0 || start<0 || end>=poly.faces.n || start>=poly.faces.n) { error=1; break; }
//					if (end<start) { c=start; start=end; end=c; }
//					//for (c=start; c<=end; c++) range.pushnodup(c);
//				}
//				if (error) {
//					cerr<<"Error processing range!"<<endl;
//					exit(1);
//				}
				cout <<"*** must finish implementing range!"<<endl;
			  } break;

		}
	}

	if (optind<argc && !filename) {
		filename=newstr(argv[optind]);
		//cout <<"found sphere file name "<<filename<<endl;
	}


	if (!filename) {
		cout <<"Error: no sphere file given!"<<endl;
		return 1;
	}

	Image sphere;
	if (generate_images) try {
		sphere.read(filename);
	} catch (Exception &error_ ) {
		cout <<"Error loading "<<filename<<endl;
		return 1;
	}

	cout <<"\n"
		 <<"Using sphere file:"<<filename<<endl
	     <<"  filesize: "<<sphere.fileSize()/1024<<" kb"<<endl
		 <<"     width: "<<sphere.baseColumns()<<endl
		 <<"    height: "<<sphere.baseRows()<<endl;


	 // Automatically create a base file name from the sphere image file name
	 // with the suffix removed.
	if (!filebase) {
		filebase=newstr(lax_basename(filename));
		char *ptr=strrchr(filebase,'.');
		if (ptr!=NULL && ptr!=filebase) *ptr='\0';
	}


	cout <<"-----------Polyhedron--------------\n";
	Polyhedron poly;
	char *error=NULL;
	if (polyhedronfile) {
		cout <<"Reading in "<<polyhedronfile<<"..."<<endl;
		if (poly.dumpInFile(polyhedronfile,&error)!=0) {
			cerr <<"Could not load polyhedron: "<<polyhedronfile<<endl;
			if (error) cerr <<" with error: "<<error<<endl;
			exit(1);
		}
	} else {
		if (out!=OUT_QTVR && out!=OUT_QTVR_FACES) 
			cerr << "No polyhedron file given, using a cube..."<<endl;
		makestr(poly.name,"Cube");

		poly.vertices.push(spacepoint(1,1,1));
		poly.vertices.push(spacepoint(-1,1,1));
		poly.vertices.push(spacepoint(-1,-1,1));
		poly.vertices.push(spacepoint(1,-1,1));
		poly.vertices.push(spacepoint(-1,1,-1));
		poly.vertices.push(spacepoint(1,1,-1));
		poly.vertices.push(spacepoint(1,-1,-1));
		poly.vertices.push(spacepoint(-1,-1,-1));

		poly.faces.push(new Face("0 1 2 3",""));
		poly.faces.push(new Face("1 0 5 4",""));
		poly.faces.push(new Face("2 1 4 7",""));
		poly.faces.push(new Face("0 3 6 5",""));
		poly.faces.push(new Face("3 2 7 6",""));
		poly.faces.push(new Face("4 5 6 7",""));
		poly.connectFaces();
	}

	//cout <<"-----------Polyhedron dump--------------\n";
	//poly.dump_out(stdout,0,0,NULL);
	//cout <<"-----------end Polyhedron--------------\n";


	cout <<"-----------Net--------------\n";
	if (netfile) { //overrides any net in a polyptych file
		if (net.faces.n>0) net.clear();
		if (net.LoadFile(netfile,&error)!=0) {
			cerr <<"Could not load net file: "<<netfile<<endl;
			exit(1);
		}
	} else if (out!=OUT_NONE && net.faces.n==0) {
		 //autounwrap only when we don't have net info yet
		cerr <<"No net file given. Autounwrapping..."<<endl;
		
		poly.inc_count();
		net.basenet=&poly;
		net.TotalUnwrap();
	}
	
	DoubleBBox paper(0,8.5,0,11);
	net.rebuildLines();
	net.FitToData(&paper,.5,1);

	//cout <<"-----------Net dump--------------\n";
	//net.dump_out(stdout,0,0,NULL);
	//net.rebuildLines();
	//net.SaveSvg("test.svg",NULL);
	cout <<"-----------end Net--------------\n";

	if (figure_pointat) {
		DBG cerr<<"figure pointat: "<<figure_pointat<<"="<<(figure_pointat==2?"to pix":"to angle")<<endl;

		if (pointat_face>=poly.faces.n) {
			cerr<<"Out of range face number for point-at option!"<<endl;
			exit(1);
		}
		double facetheta, 
			   facegamma,
			   r;
		spacepoint facecenter;
		for (c=0; c<poly.faces.e[pointat_face]->pn; c++) {
			facecenter+=poly.vertices[poly.faces.e[pointat_face]->p[c]];
		}
		facecenter/=poly.faces.e[pointat_face]->pn;
		r=sqrt(facecenter.x*facecenter.x + facecenter.y*facecenter.y);
		facetheta=atan2(facecenter.y,facecenter.x); //theta
		facegamma=atan(facecenter.z/r);            //gamma

		int pixx=(int)(pointat.x+.5),
			pixy=(int)(pointat.y+.5);
		if (figure_pointat==2) { //pointat is pixel coord in sphere
			pointat.x=(pointat.x/sphere.baseColumns()-.5)*2*M_PI; //theta
			pointat.y=(pointat.y/sphere.baseRows()-.5)*M_PI;     //gamma
		}

		if (extra_basis) delete extra_basis;
		extra_basis=new basis;

		double rotatetheta=facetheta-pointat.x,
			   rotategamma=facegamma-pointat.y;

		spacepoint p;
		p=spacepoint(cos(pointat.x)*cos(pointat.y),
					 sin(pointat.x)*cos(pointat.y),
					 sin(pointat.y));

		DBG cerr<<"-----point-at info---------"<<endl;
		DBG cerr<<"face number: "<<pointat_face<<endl;
		DBG cerr<<"face center: "<<facecenter.x<<','<<facecenter.y<<','<<facecenter.z<<endl<<endl;
		DBG cerr<<"face  theta,gamma rad: "<<facetheta         <<", "<<facegamma         <<endl;
		DBG cerr<<"face  theta,gamma deg: "<<facetheta/M_PI*180<<", "<<facegamma/M_PI*180<<endl<<endl;
		DBG cerr<<"point theta,gamma rad: "<<pointat.x<<", "<<pointat.y<<endl;
		DBG cerr<<"point theta,gamma deg: "<<pointat.x/M_PI*180<<", "<<pointat.y/M_PI*180<<endl;
		DBG cerr<<"point pix: "<<pixx<<", "<<pixy<<endl;
		DBG cerr<<"point: "<<p.x<<','<<p.y<<','<<p.z<<endl<<endl;
		DBG cerr<<"point rotate: "<<pointat_rotation<<",  ("<<(pointat_rotation)/M_PI*180<<" deg)"<<endl;
		DBG cerr<<"rotate theta: "<<facetheta-pointat.x<<",  ("<<(facetheta-pointat.x)/M_PI*180<<" deg)"<<endl;
		DBG cerr<<"rotate gamma: "<<facegamma-pointat.y<<",  ("<<(facegamma-pointat.y)/M_PI*180<<" deg)"<<endl;

		 //line up point
		if (rotatetheta) {
			rotate(*extra_basis,'z',rotatetheta,0); //rotate theta
			rotate(p,spacepoint(0,0,1),rotatetheta,0);
		}
		if (rotategamma) { 
			spacepoint gammaaxis=-spacepoint(0,0,1)/facecenter;
			if (norm(gammaaxis)<1e-10) {
				gammaaxis=-spacepoint(0,0,1)/p;
			}
			DBG cerr<<"gamma axis: "<<gammaaxis.x<<", "<<gammaaxis.y<<", "<<gammaaxis.z<<endl;
			rotate(*extra_basis, gammaaxis, rotategamma);
		}
		 
//		------------
//		 //line up point---alternate method
//		spacepoint p,axis;
//		p=spacepoint(cos(pointat.x)*cos(pointat.y),
//					 sin(pointat.x)*cos(pointat.y),
//					 sin(pointat.y));
//		axis=p/facecenter;
//		------------

		//DBG cout <<"--before"<<endl;
		//DBG cout <<"x*y "<<(extra_basis->x)*(extra_basis->y)<<endl;
		//DBG cout <<"y*z "<<(extra_basis->y)*(extra_basis->z)<<endl;
		//DBG cout <<"x*z "<<(extra_basis->x)*(extra_basis->z)<<endl;
		//DBG cout <<"angles x: "<<angle(extra_basis->x,facecenter,1)<<endl;
		//DBG cout <<"angles y: "<<angle(extra_basis->y,facecenter,1)<<endl;
		//DBG cout <<"angles z: "<<angle(extra_basis->z,facecenter,1)<<endl;
		//DBG cout <<"angles x-y: "<<angle(extra_basis->x,extra_basis->y,1)<<endl;
		//DBG cout <<"angles y-z: "<<angle(extra_basis->y,extra_basis->z,1)<<endl;
		//DBG cout <<"angles z-y: "<<angle(extra_basis->z,extra_basis->x,1)<<endl;

		 //extra rotation
		if (pointat_rotation) rotate(*extra_basis, facecenter, pointat_rotation);
		//DBG cout <<"--after"<<endl;
		//DBG cout <<"x*y "<<(extra_basis->x)*(extra_basis->y)<<endl;
		//DBG cout <<"y*z "<<(extra_basis->y)*(extra_basis->z)<<endl;
		//DBG cout <<"x*z "<<(extra_basis->x)*(extra_basis->z)<<endl;
		//DBG extra_basis->x=extra_basis->y/extra_basis->z;
		//DBG extra_basis->y=-extra_basis->y;
		//DBG extra_basis->z=-extra_basis->z;
		//DBG cout <<"--after2"<<endl;
		//DBG cout <<"x*y "<<(extra_basis->x)*(extra_basis->y)<<endl;
		//DBG cout <<"y*z "<<(extra_basis->y)*(extra_basis->z)<<endl;
		//DBG cout <<"x*z "<<(extra_basis->x)*(extra_basis->z)<<endl;
		//DBG cout <<"angles x: "<<angle(extra_basis->x,facecenter,1)<<endl;
		//DBG cout <<"angles y: "<<angle(extra_basis->y,facecenter,1)<<endl;
		//DBG cout <<"angles z: "<<angle(extra_basis->z,facecenter,1)<<endl;

		//DBG cout <<"angles x-y: "<<angle(extra_basis->x,extra_basis->y,1)<<endl;
		//DBG cout <<"angles y-z: "<<angle(extra_basis->y,extra_basis->z,1)<<endl;
		//DBG cout <<"angles z-y: "<<angle(extra_basis->z,extra_basis->x,1)<<endl;

	}

	cout <<"-----------generate faces--------\n";
	return SphereToPoly(sphere,
						&poly,
						&net,
						0,
						M_PI/2,
						maxwidth,
						filebase,
						out,
						extra_basis
						//OUT_LAIDOUT
					);
}

