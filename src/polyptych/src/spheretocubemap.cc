//
//  make a cube texture map from an equirectangular image
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
int SphereToCubeMap(Image spheremap,
				 //spacepoint sphere_z,
				 //spacepoint sphere_x,
				 int defaultimagewidth, //of one side
				 const char *tofile,
				 const char *which
				)
{

	int spherewidth=spheremap.columns();
	int sphereheight=spheremap.rows();

	Geometry geometry(defaultimagewidth*3,defaultimagewidth*2);
	//Color color;
	ColorRGB color;
	Image faceimage(geometry,color);
	faceimage.magick("PNG");
	faceimage.read("xc:transparent");

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

	//unsigned int rq,gq,bq;
	double rq,gq,bq,aq;
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
				rq=gq=bq=aq=0;

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
						//if (color.alpha()>0) { //defaults to jpeg file, can't have alpha...
						//	rq=gq=bq=0;
						//	color.alpha(0);
						//	break;
						//}

						rq+=color.red();
						gq+=color.green();
						bq+=color.blue();
						aq+=color.alpha();

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
				aq/=AA*AA;
				//cerr <<"r:"<<rq<<" g:"<<gq<<" b:"<<bq<<endl;
				//color.redQuantum(rq);
				//color.greenQuantum(gq);
				//color.blueQuantum(bq);
				color.red(rq);
				color.green(gq);
				color.blue(bq);
				color.alpha(aq);

				 //finally write the pixel
				faceimage.pixelColor(x+offx,y+offy, color);

			}
		}

	}

	//sprintf(filename,"%s.jpg",tofile);
	sprintf(filename,"%s.png",tofile);
	sprintf(filenametiff,"%s.tiff",tofile);

	 //save the image somewhere..
	//faceimage.compressType(LZWCompression);
	faceimage.depth(8);
	//faceimage.write(filenametiff);

	cout <<"Writing "<<filename<<"..."<<endl;
	faceimage.magick("PNG");
	//faceimage.quality(80);
	faceimage.write(filename);

	//cout <<"done with image"<<endl;

	//faceimage.magick("TIFF");
	
	cout <<"All done!"<<endl;
	return 0;
}


int main(int argc,char **argv)
{
	InitializeMagick(*argv);

	cout <<"sizeof(Quantum)="<<sizeof(Quantum)<<endl<<endl;

	if (argc<2) {
		cerr << "Need a sphere to output!" <<endl;
		cerr << "Options:\n spherefile.tiff [prefix [which [width]]]"<<endl;
		return 1;
	}

	Image sphere;
	try {
		sphere.read(argv[1]);
	} catch (Exception &error_ ) {
		cout <<"Error loading "<<argv[1]<<endl;
		return 1;
	}

	cout <<"equirectangular file:"<<argv[1];
	cout <<"  filesize: "<<sphere.fileSize();
	cout <<"     width: "<<sphere.baseColumns();
	cout <<"    height: "<<sphere.baseRows()<<endl;

	char filebase[strlen(argv[1])+20];
	if (argc>2) sprintf(filebase,"%s",argv[2]);
	else sprintf(filebase, "%s-cubemap", argv[1]);

	if (argc>3) strcpy(which,argv[3]);
	else strcpy(which,"012345");
	cout <<"Process "<<which<<endl;

	int width=500;
	if (argc>4) width=strtol(argv[4],NULL,10);
	else width=sphere.baseColumns()/4;

	cout <<"Making cube faces in image "<<width*3<<" x "<<width*2<<endl;


	return SphereToCubeMap(sphere,width,filebase, "012345");

}
