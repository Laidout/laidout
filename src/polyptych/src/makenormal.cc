//g++ -I /usr/include/GraphicsMagick/ makenormal.cc -lGraphicsMagick++ -lGraphicsMagick -o gm


#include <Magick++.h>
#include <iostream>
#include <cstring>

#define LAX_VECTORS_STANDALONE
#include <lax/vectors-indep.cc>


using namespace std;
using namespace Magick;


typedef spacevector vec3;
typedef flatvector vec2;

void MakeNormal(Image in, const char *outfile, double scale)
{
	int spherewidth  = in.columns();
	int sphereheight = in.rows();

	Geometry geometry(spherewidth, sphereheight);

	//Color color;
	ColorRGB color, s11, s01, s21, s10, s12;
	Image out(geometry,color);

	int xx,yy;

	double max=1.0;

	double tenpercent = spherewidth/10.0;
	int nextpercent = tenpercent;

	cout <<"Starting..."<<endl;
	
	 // loop over width and height of target image for face
	for (int x=0; x<spherewidth; x++) {

		if (x > nextpercent) {
			cout << "  "<<int(.5+nextpercent/(float)spherewidth*100)<<"%"<<endl;
			nextpercent += tenpercent;
		}

		for (int y=0; y<sphereheight; y++) {
			s11 = in.pixelColor(x,y);

			xx = x-1; yy = y; if (xx<0) xx = spherewidth-1;
			s01 = in.pixelColor(xx,yy);

			xx = x+1; yy = y; if (xx == spherewidth) xx = 0;
			s21 = in.pixelColor(xx,yy);

			xx = x; yy = y-1; if (yy<0) yy = 1;
			s10 = in.pixelColor(xx,yy);

			xx = x; yy = y+1; if (yy>=sphereheight) yy = sphereheight-2;
			s12 = in.pixelColor(xx,yy);

			vec3 va = vec3(2.0, 0.0, scale * (s21.red() - s01.red()));
			va.normalize();
			vec3 vb = vec3(0.0, 2.0, scale * (s12.red() - s10.red()));
			vb.normalize();
			vec3 normal = va / vb;

			color.red  ((.5 + normal.x * .5) * max);
			color.green((.5 + normal.y * .5) * max);
			color.blue ((.5 + normal.z * .5) * max);
			color.alpha(0.0);
			
			out.pixelColor(x,y,color);
		}
	}

	 //save the image somewhere..
	out.compressType(LZWCompression);
	out.depth(8);
	cout << "  100%"<<endl;
	cout << "Saving..."<<endl;
	out.write(outfile);
	
	cout <<"All done!"<<endl;
}

int main(int argc,char **argv)
{
	InitializeMagick(*argv);

	cout << "Make equirectangular normal map..."<<endl;
	cout <<"sizeof(Quantum)="<<sizeof(Quantum)<<endl<<endl;

	if (argc<2) {
		cerr << "Need a sphere to output!" <<endl;
		cerr << "Options:\n spherefile.tiff outfilename [scale]"<<endl;
		return 1;
	}
	
	char *outfile = new char[1000];

	if (argc > 2) strcpy(outfile, argv[2]);
	else {
		strcpy(outfile, argv[1]);
		*strrchr(outfile, '.') = '\0';
		strcat(outfile, "-normal.png");
	}

	double scale = 1;
	if (argc > 3) {
		scale = strtod(argv[3], nullptr);
		if (scale <= 0) {
			cerr << "Bad scale value "<<scale<<"!"<<endl;
			exit(1);
		}
	}


	Image sphere;
	try {
		sphere.read(argv[1]);
	} catch (Exception &error_ ) {
		cout <<"Error loading "<<argv[1]<<endl;
		return 1;
	}

	cout <<"file: "<<argv[1]<<endl;
	cout <<"  filesize: "<<sphere.fileSize()<<endl;
	cout <<"     width: "<<sphere.baseColumns()<<endl;
	cout <<"    height: "<<sphere.baseRows()<<endl;
	cout <<"Save to: "<<outfile<<endl;

	MakeNormal(sphere, outfile, scale);

	return 0;
} 
