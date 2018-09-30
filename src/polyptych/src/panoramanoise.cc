//
// panoramanoise.cc: generate an equirectangular panorama filled with opensimplex 3 dimensional noise
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
#include <lax/noise.h>
#include <cstring>

using namespace std;
using namespace Magick;
using namespace Laxkit;


#define DBG


const int AA=2; //how many pixels across to antialias
//const char *filesuf="021354";
const char *filesuf="012345";
char which[10];

/*! If inalpha == 1, set noise to alpha channel, and color to white.
 * If ==-1, use black instead. inalpha == 0 means opaque, but set value same in rgb.
 */
int PanoramaNoise(const char *outfile, int width, int height, double scalex, double scaley, double scalez, long seed, int inalpha)
{
	if (width<=0 || height<=0) {
		cerr <<"Bad dimensions for outfile in MakeFlow()"<<endl;
		return 1;
	}

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

	cout << "Computing noise for "<<width << " x "<<height <<", scale: "<<scalex <<","<<scaley<<','<<scalez<<"..."<<endl;


	//int spherewidth  = spheremap.columns();
	//int sphereheight = spheremap.rows();
	int spherewidth  = width;
	int sphereheight = height;

	Geometry geometry(spherewidth, sphereheight);
	ColorRGB color;
	Image outimage(geometry, color);
	outimage.magick("TIFF");

	//Basis sphere_basis(spacepoint(0,0,0),sphere_z,sphere_x);;
	//Basis sphere_basis;

	double xx,yy,zz;
	double theta,gamma;
	double widthf = spherewidth, heightf = sphereheight;
	double value;

	OpenSimplexNoise noise(seed);

	if (inalpha == 1) {
		color.red(1.);
		color.green(1.);
		color.blue(1.);

	} else if (inalpha == -1) {
		color.red(0.);
		color.green(0.);
		color.blue(0.);
	}
	color.alpha(0.);

	 // loop over width and height of target image for face
	for (int x=0; x < spherewidth; x++) {

		theta = x/(double)widthf * 2*M_PI;

		for (int y=0; y<height; y++) {
			gamma = (2.*y/heightf - 1) * M_PI/2;

			zz = scalez * sin(gamma);
			xx = scalex * cos(gamma) * cos(theta);
			yy = scaley * cos(gamma) * sin(theta);

			//transform(p,basis);
			value = .5*(noise.Evaluate(xx,yy,zz)+1);


			//--------------------------------
			try { //using ColorRGB

				if (inalpha) {
					color.alpha(value);
				} else {
					color.red  (value);
					color.green(value);
					color.blue (value);
				}

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

			outimage.pixelColor(x,y,color);
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
	"Fill an equirectangular image with 3-d opensimplex noise.\n"
	"-a 1 or -1 means write the noise to alpha instead of color, and otherwise use white or black\n"
	"You can set xyz scale individually with -X, -Y, -Z\n"
	"Usage:\n"
	"  panoramanoise infile.jpg -w 2048 -h 1024 [-s (float scale value)] [-S (integer seed value)] [-a 0] -o outfile.jpg\n"
  ;


int main(int argc,char **argv)
{
	InitializeMagick(*argv);


	if (argc<2) {
		cerr << usage <<endl;
		return 1;
	}

	//const char *infile = NULL;
	const char *outfile = NULL;

	Basis basis;
	int width = 0;
	int height = 0;
	double scalex=100, scaley=100, scalez=100;
	int seed = 0;
	int inalpha = 0;

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

			} else if (!strcmp(argv[c], "-X")) {
				c++;
				if (c < argc) {
					scalex = strtod(argv[c], NULL);
				} else throw(1);

			} else if (!strcmp(argv[c], "-Y")) {
				c++;
				if (c < argc) {
					scaley = strtod(argv[c], NULL);
				} else throw(2);

			} else if (!strcmp(argv[c], "-Z")) {
				c++;
				if (c < argc) {
					scalez = strtod(argv[c], NULL);
				} else throw(3);

			} else if (!strcmp(argv[c], "-o")) {
				c++;
				if (c < argc) {
					outfile = argv[c];
				} else throw(4);

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

			} else if (!strcmp(argv[c], "-s")) {
				c++;
				if (c < argc) {
					scalex = scaley = scalez = strtod(argv[c], NULL);
				} else throw(6);

			} else if (!strcmp(argv[c], "-S")) {
				c++;
				if (c < argc) {
					seed = strtol(argv[c], NULL, 10);
				} else throw(6);

			} else if (!strcmp(argv[c], "-a")) {
				c++;
				if (c < argc) {
					inalpha = strtol(argv[c], NULL, 10);
				} else throw(6);

			//} else if (argv[c][0] != '-') {
			//	infile = argv[c];

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



	return PanoramaNoise(outfile, width, height, scalex, scaley, scalez, seed, inalpha);

}
