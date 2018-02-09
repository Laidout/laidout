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
// Copyright (C) 2018 by Tom Lechner
//

#include <iostream>
#include <GraphicsMagick/Magick++.h>

#include <lax/vectors.h>
#include <cstring>

using namespace std;
using namespace Magick;

#define DBG 


const int AA=2;
//const char *filesuf="021354";
const char *filesuf="012345";
char which[10];

//! Create several small images, 1 per face of a polyhedron from a sphere map.
int SphereToCube(Image spheremap,
				 //spacepoint sphere_z,
				 //spacepoint sphere_x,
				 int defaultimagewidth,
				 const char *filebase
				)
{

	int spherewidth=spheremap.columns();
	int sphereheight=spheremap.rows();

	Geometry geometry(defaultimagewidth,defaultimagewidth);
	//Color color;
	ColorRGB color;
	Image faceimage(geometry,color);
	faceimage.magick("TIFF");

	//Basis sphere_basis(spacepoint(0,0,0),sphere_z,sphere_x);;
	Basis sphere_basis;

	spacepoint p;
	//double theta,gamma;
	int c,x,y,sx,sy,c2,cx,cy;
	double aai=1./AA;
	double xx[AA],yy[AA],r;
	char filename[500],filenametiff[500];

	//unsigned int rq,gq,bq;
	double rq,gq,bq;

	for (c=0; c<6; c++) {
		if (!strchr(which,filesuf[c])) {
			cout <<"Skipping number "<<filesuf[c]<<endl;
			continue;
		}


		sprintf(filename,"%s%c.jpg",filebase,filesuf[c]);
		sprintf(filenametiff,"%s%c.tiff",filebase,filesuf[c]);
		cout <<"Working on "<<filename<<"..."<<endl;
		
		 // loop over width and height of target image for face
		for (x=0; x<defaultimagewidth; x++) {
			 //xx and yy are parts of coordinates in xyz space, range [-1,1]
			for (c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/defaultimagewidth*2-1;

			for (y=0; y<defaultimagewidth; y++) {
				for (c2=0; c2<AA; c2++) yy[c2]=((double)y+aai*(c2+.5))/defaultimagewidth*2-1;
				rq=gq=bq=0;

				 //oversample to get pixel color
				for (cx=0; cx<AA; cx++) {
				  for (cy=0; cy<AA; cy++) {
					  
					 //transform (xx,yy) -> (x,y,z)
					 //const char *filesuf="021354";
					switch (c) {
						case 0: p=spacepoint(      1, xx[cx], yy[cy]); break;
						case 2: p=spacepoint(     -1,-xx[cx], yy[cy]); break;
						case 1: p=spacepoint(-xx[cy],      1, yy[cy]); break;
						case 3: p=spacepoint( xx[cy],     -1, yy[cy]); break;
						case 5: p=spacepoint(-yy[cy], xx[cy],      1); break;
						case 4: p=spacepoint( yy[cy], xx[cy],     -1); break;
					}
//					----------------
//					switch (c) {
//						case 0: p=spacepoint(      1, xx[cx], yy[cy]); break;
//						case 1: p=spacepoint(     -1,-xx[cx], yy[cy]); break;
//						case 2: p=spacepoint(-xx[cy],      1, yy[cy]); break;
//						case 3: p=spacepoint( xx[cy],     -1, yy[cy]); break;
//						case 4: p=spacepoint(-yy[cy], xx[cy],      1); break;
//						case 5: p=spacepoint( yy[cy], xx[cy],     -1); break;
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

					//--------------------------------
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
//					--------------------------------
//					try {
//						//cout <<".";
//						color=spheremap.pixelColor(sx,sy);
//						//cout <<":";
//					} catch (Exception &error_ ) {
//						color.redQuantum(0);
//						color.greenQuantum(0);
//						color.blueQuantum(0);
//					}
//					try {
//						rq+=color.redQuantum();
//					} catch (Exception &error_ ) {
//						cout <<"red error"<<endl;
//					}
//
//					try {
//						gq+=color.greenQuantum();
//					} catch (Exception &error_ ) {
//						cout <<"green error"<<endl;
//					}
//
//					try {
//						bq+=color.blueQuantum();
//					} catch (Exception &error_ ) {
//						cout <<"blue error"<<endl;
//					}
//					--------------------------------
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
				faceimage.pixelColor(x,y,color);

			}
		}

		 //save the image somewhere..
		faceimage.compressType(LZWCompression);
		faceimage.depth(8);
		faceimage.write(filenametiff);

		faceimage.magick("JPG");
		faceimage.quality(75);
		faceimage.write(filename);

		//cout <<"done with image"<<endl;

		faceimage.magick("TIFF");
	}
	
	cout <<"All done!"<<endl;
	return 0;
}

/*! Make flow such that motion is from one pole to the other.
 * Used for a uv sphere tilted 90 degrees so poles lay on the horizon.
 * Motion diminishes at poles, and also around horizon, which will
 * be a great circle connecting the poles.
 */
int MakeFlow(const char *outfile, int width, int height, int magnitude)
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

	for (int x=0; x<width; x++) {
		// //xx and yy are parts of coordinates in xyz space, range [-1,1]
		//for (c2=0; c2<AA; c2++) xx[c2]=((double)x+aai*(c2+.5))/defaultimagewidth*2-1;
		
		theta = x/(double)width * 2*M_PI;
		hpart = (cos(2*theta)+1)/2;

		for (int y=0; y<height; y++) {
			gamma = (y - height/2.0)/height*2.0;
			vpart = sqrt(1-gamma*gamma);

			v = hpart * vpart; //this is 0..1

			color.red  (.5);
			color.green(.5+v/2);
			color.blue (.5);
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
		outimage.quality(75);
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
	"Take an equirectangular image, rotate around x, y, z axes, and ouput the rotated image.\n"
	"Rotations occur in order listed.\n"
	"Usage:\n"
	"  remapsphere infile.jpg -x 15 -y 90 -z 0 -o outfile.jpg\n"
	"  remapsphere -w 2048 -h 1024 -f -m 100 -o flowmap.jpg  #h defaults to half width. m is pixel magnitude of flow\n"
	"  remapsphere infile.jpg -f -m 100 -o flowmap.jpg  #take dimensions from it\n"
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
	char action = 0;
	int width = 0;
	int height = 0;
	int magnitude = 100;

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

			} else if (!strcmp(argv[c], "-f")) {
				 //make cloud-ish flow map
				action = 'f';

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


	if (action == 'f') {
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

		return MakeFlow(outfile, width,height, magnitude);
	}


	if (!infile) {
		cerr <<"Missing input file!\n" << usage << endl;
		exit(1);
	}


	cout <<"input file:"<< infile<<endl;
	cout <<"  filesize: " << sphere.fileSize()    << endl;
	cout <<"     width: " << sphere.baseColumns() << endl;
	cout <<"    height: " << sphere.baseRows()    << endl;


	//return RemapSphere(sphere, basis);

	return 0;
}
