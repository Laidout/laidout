//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/********** psout.cc ****************/


#include "psout.h"
#include "../version.h"

#include "psfilters.h"
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/somedataref.h>
#include <lax/transformmath.h>

#include "psgradient.h"
#include "pscolorpatch.h"
#include "pspathsdata.h"


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxInterfaces;

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
 *  (done for ColorPatchData and PathsData...)
 *
 * \todo *** the output must be tailored to the specified dpi, otherwise the output
 * file will sometimes be quite enormous
 */
void psdumpobj(FILE *f,LaxInterfaces::SomeData *obj)
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
		 //*** for some reason if I just do uchar rgbbuf[len], it will crash for large images
		unsigned char *rgbbuf=new unsigned char[len]; //***this could be redone to not need new huge array like this
		unsigned char r,g,b;
		DATA32 bt;
		//DBG cout <<"rgbbug"<<rgbbuf[0]<<endl;//***
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

		delete[] rgbbuf;

		fprintf(f,"grestore\n");
	} else if (dynamic_cast<GradientData *>(obj)) {
		fprintf(f,"gsave\n"
				  "[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				   obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
		psGradient(f,dynamic_cast<GradientData *>(obj));
		fprintf(f,"grestore\n");
	} else if (dynamic_cast<ColorPatchData *>(obj)) {
		fprintf(f,"gsave\n"
				  "[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
				   obj->m(0), obj->m(1), obj->m(2), obj->m(3), obj->m(4), obj->m(5)); 
		psColorPatch(f,dynamic_cast<ColorPatchData *>(obj));
		fprintf(f,"grestore\n");
	} else if (dynamic_cast<PathsData *>(obj)) {
		psPathsData(f,dynamic_cast<PathsData *>(obj));
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

//! Output a postscript clipping path from outline.
/*! \ingroup postscript
 * outline can be a group of PathsData, a SomeDataRef to a PathsData, 
 * or a single PathsData.
 *
 * Non-PathsData elements in a group does not break the printing.
 * Those extra objects are just ignored.
 *
 * Returns the number of single paths interpreted.
 *
 * If iscontinuing!=0, then doesn't write 'clip' at the end.
 *
 * \todo *** currently, uses all points (vertex and control points)
 * in the paths as a polyline, not as the full curvy business 
 * that PathsData are capable of. when ps output of paths is 
 * actually more implemented, this will change..
 */
int psSetClipToPath(FILE *f,LaxInterfaces::SomeData *outline,int iscontinuing)//iscontinuing=0
{
	PathsData *path=dynamic_cast<PathsData *>(outline);

	 //If is not a path, but is a reference to a path
	if (!path && dynamic_cast<SomeDataRef *>(outline)) {
		SomeDataRef *ref;
		 // skip all nested SomeDataRefs
		do {
			ref=dynamic_cast<SomeDataRef *>(outline);
			if (ref) outline=ref->thedata;
		} while (ref);
		if (outline) path=dynamic_cast<PathsData *>(outline);
	}

	int n=0; //the number of objects interpreted
	
	 // If is not a path, and is not a ref to a path, but is a group,
	 // then check that its elements 
	if (!path && dynamic_cast<Group *>(outline)) {
		Group *g=dynamic_cast<Group *>(outline);
		SomeData *d;
		double m[6];
		for (int c=0; c<g->n(); c++) {
			d=g->e(c);
			 //add transform of group element
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					d->m(0), d->m(1), d->m(2), d->m(3), d->m(4), d->m(5)); 
			n+=psSetClipToPath(f,g->e(c),1);
			transform_invert(m,d->m());
			 //reverse the transform
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 
		}
	}
	
	if (!path) return n;
	
	 // finally append to clip path
	Coordinate *start,*p;
	for (int c=0; c<path->paths.n; c++) {
		start=p=path->paths.e[c]->path;
		if (!p) continue;
		do { p=p->next; } while (p && p!=start);
		if (p==start) { // only include closed paths
			n++;
			p=start;
			do {
				fprintf(f,"%.10g %.10g ",p->x(),p->y());
				if (p==start) fprintf(f,"moveto\n");
				else fprintf(f,"lineto\n");
				p=p->next;	
			} while (p && p!=start);
			fprintf(f,"closepath\n");
		}
	}
	
	if (n && !iscontinuing) {
		//fprintf(f,".1 setlinewidth stroke\n");
		fprintf(f,"clip\n");
	}
	return n;
}

//! Print a postscript file of doc to already open f.
/*! \ingroup postscript
 * Does not open or close f.
 *
 * Return 0 for no errors, nonzero for errors.
 * 
 * \todo *** this does not currently handle pages that bleed their contents
 * onto other pages correctly, nor does it otherwise clip the pages properly 
 *
 * \todo *** ps doc tag CreationDate: ?????, For: ????
 *
 * \todo *** for tiled pages, or multiples of same object each instance is
 * rendered independently right now. should make a function to display such
 * things, thus reduce ps file size substantially..
 */
int psout(FILE *f,Document *doc)
{
	if (!f || !doc) return 1;

	 // print out header
	fprintf (f,
			"%%!PS-Adobe-3.0\n"
			"%%%%Orientation: ");
	fprintf (f,"%s\n",(doc->docstyle->imposition->paperstyle->flags&1)?"Landscape":"Portrait");
	fprintf(f,"%%%%Pages: %d\n",doc->docstyle->imposition->numpapers);
	fprintf(f,"%%%%PageOrder: Ascend\n"
			  "%%%%CreationDate: Sat Feb 11 21:12:07 2006\n"
			  "%%%%Creator: Laidout %s\n"
			  "%%%%For: whoever (i686, Linux 2.6.15-1-k7)\n",LAIDOUT_VERSION);
	fprintf(f,"%%%%DocumentMedia: %s %.10g %.10g 75 white ( )\n", //75 g/m^2 = 20lb * 3.76 g/lb/m^2
				doc->docstyle->imposition->paperstyle->name, 
				72*doc->docstyle->imposition->paperstyle->width,  //width and height ignoring landscape/portrait
				72*doc->docstyle->imposition->paperstyle->height);
	fprintf(f,"%%%%EndComments\n"
			  "%%%%BeginDefaults\n"
			  "%%%%PageMedia: %s\n",doc->docstyle->imposition->paperstyle->name);
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
	Page *page;
	for (c=0; c<doc->docstyle->imposition->numpapers; c++) {
	     //print paper header
		fprintf(f, "%%%%Page: %d %d\n", c+1,c+1);
		fprintf(f, "save\n");
		fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
		if (doc->docstyle->imposition->paperstyle->flags&1) {
			fprintf(f,"%.10g 0 translate\n90 rotate\n",doc->docstyle->imposition->paperstyle->width);
		}
		
		spread=doc->docstyle->imposition->PaperLayout(c);
		
		 // print out printer marks
		if (spread->mask&SPREAD_PRINTERMARKS && spread->marks) {
			fprintf(f," .01 setlinewidth\n");
			////DBG cout <<"marks data:\n";
			////DBG spread->marks->dump_out(stdout,2);
			psdumpobj(f,spread->marks);
		}
		
		 // for each paper in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg<0 || pg>=doc->pages.n) continue;
			page=doc->pages.e[pg];
			
			 // transform to page
			fprintf(f,"gsave\n");
			transform_copy(m,spread->pagestack.e[c2]->outline->m());
			fprintf(f,"[%.10g %.10g %.10g %.10g %.10g %.10g] concat\n ",
					m[0], m[1], m[2], m[3], m[4], m[5]); 

			//*** set clipping region

			//DBG cout <<"page flags "<<c2<<":"<<spread->pagestack[c2]->index<<" ==  "<<page->pagestyle->flags<<endl;
			if (page->pagestyle->flags&PAGE_CLIPS) {
				//DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" clips"<<endl;
				psSetClipToPath(f,spread->pagestack.e[c2]->outline,0);
			} else {
				//DBG cout <<"page "<<c2<<":"<<spread->pagestack[c2]->index<<" does not clip"<<endl;
			}
				
			 // for each layer on the page..
			for (l=0; l<page->layers.n; l++) {
				psdumpobj(f,page->layers.e[l]);
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
