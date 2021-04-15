//
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
// Copyright (C) 2011 by Tom Lechner
//


// g++ `pkg-config GraphicsMagick++ --cflags` `pkg-config GraphicsMagick++ --libs` removetransparency.cc -o removetransparency



#include <iostream>
#include <GraphicsMagick/Magick++.h>

//#include <lax/vectors.h>
#include <cstring>

using namespace std;
using namespace Magick;

#define DBG 



int RemoveTransparency(Image image, const char *outfile)
{

	int width  = image.columns();
	int height = image.rows();

	Geometry geometry(width, height);
	//Color color;
	ColorRGB color;
	Image outimage(geometry,color);
	
	if (strstr(outfile, "png") || strstr(outfile, "PNG")) outimage.magick("PNG");
	else if (strstr(outfile, "jpg") || strstr(outfile, "JPG")) outimage.magick("JPG");
	else outimage.magick("TIFF");

		
	 // loop over width and height of target image for face
	for (int x=0; x < width; x++) {
		for (int y=0; y < height; y++) {
				  
			color = image.pixelColor(x,y);
			//color.alpha(1.); //fully transparent
			color.alpha(0.); //fully opaque

			outimage.pixelColor(x,y,color);
		}
	}

	 //save the image somewhere..
	outimage.compressType(LZWCompression);
	outimage.depth(8);
	outimage.write(outfile);

	outimage.magick("JPG");
	outimage.quality(75);
	outimage.write(outfile);

	//cout <<"done with image"<<endl;

	
	cout <<"All done!"<<endl;
	return 0;
}


int main(int argc,char **argv)
{
	InitializeMagick(*argv);

	cout <<"sizeof(Quantum)="<<sizeof(Quantum)<<endl<<endl;

//	if (argc<2) {
//		cerr << "Need a sphere to output!" <<endl;
//		cerr << "Options:\n spherefile.tiff\n prefix\n width\n which"<<endl;
//		return 1;
//	}

	Image image;
	try {
		image.read(argv[1]);
	} catch (Exception &error_ ) {
		cout <<"Error loading "<<argv[1]<<": "<<error_.what()<<endl;
		return 1;
	}

	cout <<"file:"<<argv[1];
	cout <<"  filesize: "<<image.fileSize();
	cout <<"     width: "<<image.baseColumns();
	cout <<"    height: "<<image.baseRows()<<endl;

	const char *outfile = "out.png";

	if (argc>2) outfile = argv[2];


	return RemoveTransparency(image, outfile);

}
