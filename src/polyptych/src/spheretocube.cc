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


int main(int argc,char **argv)
{
	InitializeMagick(*argv);

	cout <<"sizeof(Quantum)="<<sizeof(Quantum)<<endl<<endl;

	if (argc<2) {
		cerr << "Need a sphere to output!" <<endl;
		cerr << "Options:\n spherefile.tiff\n prefix\n width\n which"<<endl;
		return 1;
	}

	Image sphere;
	try {
		sphere.read(argv[1]);
	} catch (Exception &error_ ) {
		cout <<"Error loading "<<argv[1]<<endl;
		return 1;
	}

	cout <<"file:"<<argv[1];
	cout <<"  filesize: "<<sphere.fileSize();
	cout <<"     width: "<<sphere.baseColumns();
	cout <<"    height: "<<sphere.baseRows()<<endl;

	const char *filebase="cube";
	if (argc>2) filebase=argv[2];

	int width=0;
	if (argc>3) width=strtol(argv[3],NULL,10);
	if (width<=0) width=sphere.baseColumns()/M_PI;

	cout <<"Making cube faces "<<width<<" x "<<width<<endl;

	if (argc>4) strcpy(which,argv[4]);
	else strcpy(which,"012345");

	cout <<"Process "<<which<<endl;

	return SphereToCube(sphere,width,filebase);

}
