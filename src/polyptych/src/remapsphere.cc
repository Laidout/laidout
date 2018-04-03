//
// remapsphere.cc: rotate an equirectangular panorama, output the rotated equirectangular
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
// Copyright (C) 2018 by Tom Lechner
//

#include <iostream>
#include <GraphicsMagick/Magick++.h>

#include <lax/vectors.h>
#include <cstring>

using namespace std;
using namespace Magick;

#define DBG


const int AA=1; //how many pixels across to antialias


int RemapSphere(const char *outfile, Image spheremap, Basis &basis)
{
	 //figure out extension
	bool tif = false, jpg = false; //, png = false;

	const char *ext = strrchr(outfile, '.');
	if (ext) ext++;
	if (!ext) {
		cerr <<"Missing extension on outfile! assuming tif."<<endl;
		tif = true;

	} else if (!strcasecmp(ext, "tiff") || !strcasecmp(ext, "tif")) {
		tif = true;

	} else if (!strcasecmp(ext, "jpg") || !strcasecmp(ext, "jpeg")) {
		jpg = true;

	} else {
		cerr <<"Arg! Unknown output file type!! Aborting!"<<endl;
		return 1;
	}


	int spherewidth  = spheremap.columns();
	int sphereheight = spheremap.rows();

	Geometry geometry(spherewidth, sphereheight);
	ColorRGB color;
	Image newimage(geometry, color);
	newimage.magick("TIFF");

	//Basis sphere_basis(spacepoint(0,0,0),sphere_z,sphere_x);;
	//Basis sphere_basis;

	spacepoint p;
	//double theta,gamma;
	int x,y,sx,sy,cx,cy;
	double aai=1./AA;
	double xxa, yya;
	double AA2 = AA*AA;

	//unsigned int rq,gq,bq;
	double rq,gq,bq,aq;
	double theta, theta2,  gamma, gamma2;


	 // loop over width and height of target image for face
	for (x=0; x < spherewidth; x++) {
		 //xx and yy are parts of coordinates in xyz space, range [-1,1]
		//for (c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/defaultimagewidth*2-1;

		xxa = x;

		for (y=0; y < sphereheight; y++) {
			//for (c2=0; c2<AA; c2++) yy[c2]=((double)y+aai*(c2+.5))/defaultimagewidth*2-1;
			rq = gq = bq = aq = 0;

			 //oversample to get pixel color
			for (cx=0; cx<AA; cx++) {
			  xxa = x + cx*aai;
			  theta = xxa/(double)spherewidth * 2*M_PI;

			  for (cy=0; cy<AA; cy++) {
			    yya = y + cy*aai;
				gamma = (2.*yya/sphereheight - 1) * M_PI/2;

				p.z = sin(gamma);
				p.x = cos(gamma) * cos(theta);
				p.y = cos(gamma) * sin(theta);

				transform(p,basis);

				 //transform (x,y,z) -> (sx,sy)
				gamma2 = asin(p.z);
				theta2 = atan2(p.y, p.x);
				
				sy = gamma2/M_PI*2 + .5;
				sx = theta2/2/M_PI;

				if (sx >= spherewidth) sx -= spherewidth;
				else if (sx<0) sx += spherewidth;

				if (sy >= sphereheight) sy=sphereheight;
				else if (sy <= 0) sy = 0;


				//--------------------------------
				try { //using ColorRGB
					//cout <<".";
					color = spheremap.pixelColor(sx,sy);
					//cerr <<"alpha: "<<color.alpha()<<" ";

					rq += color.red();
					gq += color.green();
					bq += color.blue();
					aq += color.alpha();

				} catch (Exception &error_ ) {
					color.red(0);
					color.green(0);
					color.blue(0);
					color.alpha(0);
					cout <<"*";

				} catch (exception &error) {
					color.red(0);
					color.green(0);
					color.blue(0);
					color.alpha(0);
					cout <<"%";
				}
			  } //cy
			} //cx

			if (AA2 > 1) {
				rq /= AA2;
				gq /= AA2;
				bq /= AA2;
				aq /= AA2;
			}

			color.red(rq);
			color.green(gq);
			color.blue(bq);
			color.alpha(aq);

			newimage.pixelColor(x,y,color);

		}
	}

	 //save the image somewhere..
	 //save the image somewhere..
	if (tif) {
		cout <<"Saving as tiff to "<<outfile<<endl;

		newimage.magick("TIFF");
		newimage.compressType(LZWCompression);
		newimage.depth(8);
		newimage.write(outfile);

	} else if (jpg) {
		cout <<"Saving as jpg to "<<outfile<<endl;
		newimage.magick("JPG");
		newimage.quality(95);
		newimage.write(outfile);
	}


	cout <<"All done!"<<endl;
	return 0;
}


void RotateAroundX(Basis &basis, double degrees)
{
	spaceline l(spacevector(0,0,0), spacevector(1,0,0));
	basis.x = rotate(basis.x, l, degrees, 1);
	basis.y = rotate(basis.y, l, degrees, 1);
	basis.z = rotate(basis.z, l, degrees, 1);
}

void RotateAroundY(Basis &basis, double degrees)
{
	spaceline l(spacevector(0,0,0), spacevector(0,1,0));
	basis.x = rotate(basis.x, l, degrees, 1);
	basis.y = rotate(basis.y, l, degrees, 1);
	basis.z = rotate(basis.z, l, degrees, 1);
}

void RotateAroundZ(Basis &basis, double degrees)
{
	spaceline l(spacevector(0,0,0), spacevector(0,0,1));
	basis.x = rotate(basis.x, l, degrees, 1);
	basis.y = rotate(basis.y, l, degrees, 1);
	basis.z = rotate(basis.z, l, degrees, 1);
}

const char *usage =
	"Take an equirectangular image, rotate around x, y, z axes, and ouput the rotated image.\n"
	"Rotations occur in order listed.\n"
	"Usage:\n"
	"  remapsphere infile.jpg -x 15 -y 90 -z 0 -o outfile.jpg\n"
	"  remapsphere -w 2048 -h 1024 -f -m 100 -o flowmap.jpg  #h defaults to half width. m is pixel magnitude of flow\n"
  ;


int main(int argc,char **argv)
{
	InitializeMagick(*argv);


	if (argc<2) {
		cerr << usage <<endl;
		return 1;
	}

	const char *infile = NULL;
	const char *outfile = NULL;

	Basis basis;
	//int width, height;

	try {
		double angle;
		for (int c=1; c<argc; c++) {
			if (!strcmp(argv[c], "-x")) {
				c++;
				if (c < argc) {
					angle = strtod(argv[c], NULL);
					if (angle != 0) RotateAroundX(basis, angle);
				} else throw(1);

			} else if (!strcmp(argv[c], "-y")) {
				c++;
				if (c < argc) {
					angle = strtod(argv[c], NULL);
					if (angle != 0) RotateAroundY(basis, angle);
				} else throw(2);

			} else if (!strcmp(argv[c], "-z")) {
				c++;
				if (c < argc) {
					angle = strtod(argv[c], NULL);
					if (angle != 0) RotateAroundZ(basis, angle);
				} else throw(3);

			} else if (!strcmp(argv[c], "-o")) {
				c++;
				if (c < argc) {
					outfile = argv[c];
				} else throw(4);

//			} else if (!strcmp(argv[c], "-w")) {
//				c++;
//				if (c < argc) {
//					width = strtol(argv[c], NULL, 10);
//				} else throw(3);
//
//			} else if (!strcmp(argv[c], "-h")) {
//				c++;
//				if (c < argc) {
//					height = strtol(argv[c], NULL, 10);
//				} else throw(3);

			} else if (argv[c][0] != '-') {
				infile = argv[c];

			} else if (!strcmp(argv[c], "-h")) {
				cerr << usage<<endl;
				exit(0);

			} else throw(5);
		}
	} catch (int e) {
		cerr <<"Error!\n"<<usage<<endl;
		exit(1);
	}

	if (!outfile) {
		cerr << "Missing outfile!"<<endl<<usage<<endl;
		exit(1);
	}

	Image sphere;
	if (infile) {
		try {
			sphere.read(infile);

		} catch (Exception &error_ ) {
			cout << "Error loading " << infile << endl;
			cout << error_.what()<<endl;
			return 1;
		}
	}




	if (!infile) {
		cerr <<"Missing input file!\n" << usage << endl;
		exit(1);
	}


	cout <<"input file:"<< infile<<endl;
	cout <<"  filesize: " << sphere.fileSize()    << endl;
	cout <<"     width: " << sphere.baseColumns() << endl;
	cout <<"    height: " << sphere.baseRows()    << endl;


	return RemapSphere(outfile, sphere, basis);

}
