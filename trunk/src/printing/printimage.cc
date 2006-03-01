// functions to output postscript images
// at the moment does Imlib_Images
//
// the stuff here is not actually used in Laidout.. bits of it have been
// transported elsewhere

#include <iostream>
#include <X11/Xlib.h>
#include <Imlib2.h>

#include "psfilters.h"

using namespace std;

//-------------------------------- Imlib_Image to ps ------------------------------------

//! Output an Imlib_Image as postscript with Ascii85 encoding.
/*! \ingroup postscript
 */
void Imlib_ImageToPS(FILE *psout,Imlib_Image image)
{
	if (!psout || !image) return;
	imlib_context_set_image(image);
	DATA32 *buf=imlib_image_get_data_for_reading_only(); // ARGB
	int width,height;
	width=imlib_image_get_width();
	height=imlib_image_get_height();
	int len=3*width*height;
	unsigned char rgbbuf[len]; //***this could be redone to not need new huge array like this
	unsigned char r,g,b;
	DATA32 bt;
	for (int x=0; x<width; x++) {
		for (int y=0; y<height; y++) {
			bt=buf[y*width+x];
			r=(buf[y*width+x]&((unsigned long)255<<16))>>16;
			g=(buf[y*width+x]&((unsigned long)255<<8))>>8;
			b=(buf[y*width+x]&(unsigned long)255);
			rgbbuf[y*width*3+x*3  ]=r;
			rgbbuf[y*width*3+x*3+1]=g;
			rgbbuf[y*width*3+x*3+2]=b;
			//printf("%x=%x,%x,%x ",bt,r,g,b);
		}
	}

	cout <<endl;
	fprintf(psout,
			"/DeviceRGB setcolorspace\n"
			"<<\n"
			"  /ImageType 1  /Width %d  /Height %d\n"
			"  /BitsPerComponent 8\n"
			"  /Decode [0 1 0 1 0 1]\n"
			"  /ImageMatrix [%d 0 0 -%d 0 %d]\n"
			"  /DataSource currentfile\n"
			"  /ASCII85Decode filter \n"
			">> image\n", width, height, width, height, height);

	 // do the Ascii85Encode filter
	Ascii85_out(psout,rgbbuf,len,1,75);

}

//------------------------------- output a full postscript file with an image -----------------------

//! Output a full postscript file with an Imlib_Image in it. 
/*! \ingroup postscript
 * outputs to temp.ps
 */
void ps_Imlib_Image_out(Imlib_Image image)
{
	FILE *f=fopen("temp.ps","w");
	if (!f) {
		cout <<"Failed to open temp.ps."<<endl;
		return;
	}

	 // write out header
	fprintf (f,
			"%%!PS-Adobe-3.0\n"
			"%%%%Orientation: Portrait\n"
			"%%%%Pages: 1\n"
			"%%%%PageOrder: Ascend\n"
			"%%%%CreationDate: Sat Feb 11 21:12:07 2006\n"
			"%%%%Creator: Test program\n"
			"%%%%For: tom <tom@doublian> (i686, Linux 2.6.15-1-k7)\n"
			"%%%%EndComments\n"
			"\n"
			"%%%%BeginProlog\n"
			"%%%%EndProlog\n"
			"\n"
			"%%%%BeginSetup\n"
			"%%%%EndSetup\n"
			"\n"
			"%%%%Page: \"1\" 1\n"
			"gsave  %% group\n"
			"[1 0 0 1 0 0] concat\n"
			"%% Raster image: blah.jpg\n"
			"save\n");

	 // write out image data
	double w,h;
	w=200;
	h=height*200./width;
	fprintf (f,"[%f 0 0 %f 165.786 325.676] concat\n", w,h);
	Imlib_ImageToPS(f,image);

	 // draw image border
	fprintf(f,"\nnewpath\n"
			"-.1 -.1 moveto 1.1 -.1 lineto 1.1 1.1 lineto -.1 1.1 lineto -.1 -.1 lineto closepath\n"
			".1 setlinewidth\n"
			"stroke\n");
	
	 // write out footer
	fprintf(f,
			"\n"
			"restore\n"
			"grestore %% /group\n"
			"\n"
			"showpage\n"
			"\n"
			"%%%%EOF\n");

	fclose(f);
}

