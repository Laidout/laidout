/********** psout.cc ****************/


#include "psout.h"

#include "psfilters.h"
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/transformmath.h>

#include "pscolorpatch.h"

using namespace Laxkit;

//! Internal function to dump out the obj in postscript. Called by psout().
/*! \ingroup postscript
 * It is assumed that the transform of the object is applied here, rather than
 * before this function is called.
 *
 * Should be able to handle gradients, bez color patches, paths, and images
 * without significant problems, EXCEPT for the lack of decent transparency handling.
 *
 * \todo *** must be able to do color management
 *
 * \todo *** need integration of global units, assumes inches now. generally must work
 * out what shall be the default working units...
 *
 * \todo *** this should probably be broken down into object specific ps out functions
 *
 * \todo *** the output must be tailored to the specified dpi, otherwise the output
 * file will sometimes be quite enormous
 */
void psdumpobj(FILE *f,Laxkit::SomeData *obj)
{
	if (dynamic_cast<Group *>(obj)) {
		Group *g=dynamic_cast<Group *>(obj);
		 // push axes
		double m[6];
		transform_copy(m,obj->m());
		fprintf(f,"gsave\n [%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				m[0], m[1], m[2], m[3], m[4], m[5]); 
		
		 //draw
		for (int c=0; c<g->n(); c++) psdumpobj(f,g->e(c)); 
		
		 // pop axes
		fprintf(f,"grestore\n\n");
	} else if (dynamic_cast<ImageData *>(obj)) {
		ImageData *img=dynamic_cast<ImageData *>(obj);

		 // the image gets put in a postscript box with sides 1x1, and the matrix
		 // in the image is ??? so must set
		 // up proper transforms
		
		double m[6];
		transform_copy(m,img->m());
		if (!img || !img->imlibimage) return;
		
		char *bname=basename(img->filename); // Warning! This assumes the GNU basename, which does
											 // not modify the string.
		if (bname) fprintf(f," %% image %s\n",bname);
				
		fprintf(f,"gsave\n"
				  "[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				   m[0], m[1], m[2], m[3], m[4], m[5]); 
		fprintf(f,"[%.10g 0 0 %.10g 0 0] concat\n",
				 img->maxx,img->maxy);
		
		 // image out
		imlib_context_set_image(img->imlibimage);
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
		//int width=(int)img->maxx,
		//    height=(int)img->maxy;
		//double w,h;
		//w=200;
		//h=height*200./width;
		fprintf(f,
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
		Ascii85_out(f,rgbbuf,len,1,75);

		fprintf(f,"grestore\n");
	} else if (dynamic_cast<GradientData *>(obj)) {
		cout <<" *** GradientData ps out not implemented! "<<endl;
	} else if (dynamic_cast<ColorPatchData *>(obj)) {
		psColorPatch(f,dynamic_cast<ColorPatchData *>(obj));
		//cout <<" ColorPatchData ps out not implemented! "<<endl;
	} else if (dynamic_cast<PathsData *>(obj)) {
		cout <<" *** PathsData ps out not implemented! "<<endl;
		//psColorPatch(f,dynamic_cast<PathsData *>(obj));
	}
}

//! Open file, or 'output.ps' if file==NULL, and output postscript for doc via psout(FILE*,Document*).
/*! \ingroup postscript
 * \todo *** does not sanity checking on file, nor check for previous existence of file
 *
 * Return 1 if not saved, 0 if saved.
 */
int psout(Document *doc,const char *file)//file=NULL
{
	if (file==NULL) file="output.ps";

	FILE *f=fopen(file,"w");
	if (!f) return 1;
	int c=psout(f,doc);
	fclose(f);

	if (c) return 1;
	return 0;
}

//! Print a postscript file of doc to f.
/*! \ingroup postscript
 * Does not open or close f.
 *
 * Return 0 for no errors, nonzero for errors.
 * 
 * \todo *** this does not currently handle pages that bleed their contents
 * onto other pages correctly, nor does it otherwise clip the pages properly 
 *
 * \todo *** ps doc tag CreationDate: ?????, For: ????
 */
int psout(FILE *f,Document *doc)
{
	if (!f || !doc) return 1;

	 // print out header
	fprintf (f,
			"%%!PS-Adobe-3.0\n"
			"%%%%Orientation: ");
	fprintf (f,"%s\n",(doc->docstyle->disposition->paperstyle->flags&1)?"Landscape":"Portrait");
	fprintf(f,"%%%%Pages: %d\n",doc->docstyle->disposition->numpapers);
	fprintf(f,"%%%%PageOrder: Ascend\n"
			  "%%%%CreationDate: Sat Feb 11 21:12:07 2006\n"
			  "%%%%Creator: Laidout 0.1\n"
			  "%%%%For: whoever (i686, Linux 2.6.15-1-k7)\n");
	fprintf(f,"%%%%DocumentMedia: %s %.10g %.10g 75 white ( )\n", //75 g/m^2 = 20lb * 3.76 g/lb/m^2
				doc->docstyle->disposition->paperstyle->name, 
				72*doc->docstyle->disposition->paperstyle->width,  //width and height ignoring landscape/portrait
				72*doc->docstyle->disposition->paperstyle->height);
	fprintf(f,"%%%%EndComments\n"
			  "%%%%BeginDefaults\n"
			  "%%%%PageMedia: %s\n",doc->docstyle->disposition->paperstyle->name);
	fprintf(f,"%%%%EndDefaults\n"
			  "\n"
			  "%%%%BeginProlog\n"
			  "%%%%EndProlog\n"
			  "\n"
			  "%%%%BeginSetup\n"
			  "%%%%EndSetup\n"
			  "\n");
	
	 // Write out paper spreads....
	Spread *spread;
	double m[6];
	int c,c2,l,pg;
	transform_set(m,1,0,0,1,0,0);
	for (c=0; c<doc->docstyle->disposition->numpapers; c++) {
	     //print paper header
		fprintf(f, "%%%%Page: %d %d\n", c+1,c+1);
		fprintf(f, "save\n");
		fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
		if (doc->docstyle->disposition->paperstyle->flags&1) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->disposition->paperstyle->width);
		}
		
		 // for each page in paper layout..
		spread=doc->docstyle->disposition->PaperLayout(c);
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			
			 // transform to page
			fprintf(f,"gsave\n");
			transform_copy(m,spread->pagestack.e[c2]->outline->m());
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
				
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n; l++) {
				psdumpobj(f,doc->pages[pg]->layers.e[l]);
			}
			fprintf(f,"grestore\n");
		}

		delete spread;
		
		 // print out paper footer
		fprintf(f,"\n"
				  "restore\n"
				  "\n"
				  "showpage\n"
				  "\n");		
	}

	 //print out footer
	fprintf(f, "\n%%%%EOF\n");


	return 0;
}
