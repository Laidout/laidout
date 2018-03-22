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


const int AA=2; //how many pixels across to antialias



/*! Make flow such that motion is from one pole to the other.
 * Used for a uv sphere tilted 90 degrees so poles lay on the horizon.
 * Motion diminishes at poles, and also around horizon, which will
 * be a great circle connecting the poles.
 */
int MakeFlow(const char *outfile, int width, int height, int magnitude, int strategy)
{
	if (width<=0 || height<=0) {
		cerr <<"Bad dimensions for outfile in MakeFlow()"<<endl;
		return 1;
	}

	cout << "Make flowmap, "<<width<<" x "<<height<<endl;


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



	 //create the stuff
	Geometry geometry(width, height);
	ColorRGB color;
	Image outimage(geometry,color);


	double theta, gamma;
	double vpart, hpart, v;

	
	if (strategy == 1) {
		 //based on global wind speed v, compute actual displacement

		spacevector wind(1,0,0);
		spacevector pos, pos2;
		double theta2, gamma2, dtheta, dgamma;

		for (int x=0; x<width; x++) {
			// //xx and yy are parts of coordinates in xyz space, range [-1,1]
			//for (c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/defaultimagewidth*2-1;

			theta = x/(double)width * 2*M_PI;

			for (int y=0; y<height; y++) {
				gamma = (2.*y/height - 1) * M_PI/2;

				pos.z = sin(gamma);
				pos.x = cos(gamma) * cos(theta);
				pos.y = cos(gamma) * sin(theta);

				if (pos.z != 0) {
					pos.x /= pos.z;
					pos.y /= pos.z;
					pos.z = 1;

					pos += wind;
					pos.normalize();

				} else {
				}

				gamma2 = asin(pos.z);
				theta2 = atan2(pos.y, pos.x);

				dtheta = (theta2 - theta) / M_PI * 2;
				if (dtheta > .5) dtheta = .5; else if (dtheta < -.5) dtheta = -.5;

				dgamma = (gamma2 - gamma) / M_PI / 2;

				color.red  (.5 + dtheta);
				color.green(.5 + dgamma / M_PI * 2);
				color.blue (.5);
				color.alpha(.0);
				outimage.pixelColor(x,y,color);
			}
		}

	} else {
		 //only pole to pole
		for (int x=0; x<width; x++) {
			// //xx and yy are parts of coordinates in xyz space, range [-1,1]
			//for (c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/defaultimagewidth*2-1;

			theta = x/(double)width * 2*M_PI;
			hpart = (cos(2*theta)+1)/2;

			for (int y=0; y<height; y++) {
				gamma = 2.*y/height - 1;
				vpart = sqrt(1-gamma*gamma);

				v = hpart * vpart; //this is 0..1

				color.red  (.5);
				color.green(.5+v/2);
				color.blue (.5);
				color.alpha(.0);
				outimage.pixelColor(x,y,color);
			}
		}
	}



	 //save the image somewhere..
	if (tif) {
		cout <<"Saving as tiff to "<<outfile<<endl;

		outimage.magick("TIFF");
		outimage.compressType(LZWCompression);
		outimage.depth(8);
		outimage.write(outfile);

	} else if (jpg) {
		cout <<"Saving as jpg to "<<outfile<<endl;
		outimage.magick("JPG");
		outimage.quality(95);
		outimage.write(outfile);
	}

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
	"Create an equirectangular based flowmap to simulate cloud speed.\n"
	"Usage:\n"
	"    -m 100  #maximum magnitude\n"
	"    -w 2048 #width\n"
	"    -h 1024 #height\n"
	"    -o flowmap.tif  #output file\n"
	"  skyflow  -m 100 -o flowmap.jpg\n"
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
	int width = 0;
	int height = 0;
	int magnitude = 100;
	int strategy = 0;

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

//			} else if (!strcmp(argv[c], "-f")) {
//				 //make cloud-ish flow map
//				action = 'f';

			} else if (!strcmp(argv[c], "-w")) {
				c++;
				if (c < argc) {
					width = strtol(argv[c], NULL, 10);
				} else throw(3);

			} else if (!strcmp(argv[c], "-h")) {
				c++;
				if (c < argc) {
					height = strtol(argv[c], NULL, 10);
				} else throw(3);

			} else if (!strcmp(argv[c], "-m")) {
				c++;
				if (c < argc) {
					magnitude = strtol(argv[c], NULL, 10);
				} else throw(6);

			} else if (!strcmp(argv[c], "-s")) {
				strategy = strtol(argv[c], NULL, 10);


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


	 //Generate a spherical flowmap suitable for clouds

	if (!outfile) {
		cerr <<"Missing output file!\n" << usage << endl;
		exit(1);
	}

	if (infile) {
		width = sphere.baseColumns();
		height = sphere.baseRows();
	}

	if (width <=0 && height <=0) { width=2048; height=1024; }
	else if (width <= 0 && height>0) { width = height*2; }
	else if (width > 0 && height <= 0) { height = width/2; }

	return MakeFlow(outfile, width,height, magnitude, strategy);


}
