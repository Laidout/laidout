//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/************* extras.cc ****************/

// *** This file should hold the mechanism for installing and removing extras.
// *** Currently, the only thing sort of like an extra is the dump images facility,
// *** which ultimately should be in a different file....

/*! \defgroup extras Extras
 * %Document extraneous addon type of things in here.
 * Still must work out good mechanism for being able to add on any extras
 * via plugins...
 * 
 * 
 * Currently:
 * 
 * dumpImages plops down a whole lot of images into the document. Specify the start page,
 * how many per page, and either a list of image files, a directory that contains the images,
 * or a list of ImageData objects.
 *
 * Planned but not implemented yet is applyPageNumbers, which automatically inserts
 * particular images representing the page numbers on each page. You would specify
 * the directory that contains the images, and the code does the rest..
 *
 * *** must figure out how to integrate extras into menus, and incorporate callbacks?
 */

#include "extras.h"
#include <dirent.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxInterfaces;
//--------------------------------- Dump Images --------------------------------------------

//! Plop all images in directory pathtoimagedir into the document.
/*! \ingroup extras
 * Grabs all the regular file names in pathtoimagedir and passes them to dumpImages(...,char **,...).
 *
 * Returns the page index of the final page.
 * 
 * \todo *** need a dump images interface dialog
 */
int dumpImages(Document *doc, int startpage, const char *pathtoimagedir, int imagesperpage, int ddpi)
{
	 // prepare to read all images in directory pathtoimagedir....
	if (pathtoimagedir==NULL) pathtoimagedir=".";
	struct dirent **dirents;
	int n=scandir(pathtoimagedir,&dirents, NULL,alphasort);
	char **imagefiles=new char*[n];
	int c,i=0;
	for (c=0; c<n; c++) {
		if (dirents[c]->d_type==DT_REG) {
			imagefiles[i]=NULL;
			makestr(imagefiles[i],pathtoimagedir);
			if (pathtoimagedir[strlen(pathtoimagedir)-1]!='/') 
				appendstr(imagefiles[i],"/"); //***
			appendstr(imagefiles[i++],dirents[c]->d_name);
			//cout << "dump maybe image files: "<<imagefiles[i-1]<<endl;
		}
		free(dirents[c]);
	}
	free(dirents);
	c=dumpImages(doc,startpage,(const char **)imagefiles,i,imagesperpage,ddpi);
	deletestrs(imagefiles,i);
	return c;
}

//! Plop all images with paths in imagefiles into the document.
/*! \ingroup extras
 * Attempts to read in all the images among imagefiles and passes them
 * to dumpImages(..., ImageData**,...), where also their transform matrices are adjusted.
 *
 * Returns the page index of the final page.
 */
int dumpImages(Document *doc, int startpage, const char **imagefiles, int nfiles, int imagesperpage, int ddpi)
{
	ImageData **images=new ImageData*[nfiles];
	int i=0,c;
	LaxImage *image=NULL;
	for (c=0; c<nfiles; c++) {
		if (!imagefiles[c] || !strcmp(imagefiles[c],".") || !strcmp(imagefiles[c],"..")) continue;
		image=load_image(imagefiles[c]);
		if (image) {
			DBG cout << "dump image files: "<<imagefiles[c]<<endl;
			images[i]=new ImageData;//creates with one count
			images[i]->SetImage(image);
			i++;
		} else {
			DBG cout <<"** warning: bad image file "<<imagefiles[c]<<endl;
		}
	}
	c=-1;
	if (i) c=dumpImages(doc,startpage,images,i,imagesperpage,ddpi);
	for (c=0; c<i; c++) // remove extraneous count
		if (images[c]) images[c]->dec_count();
	delete[] images;
	return c;
}
	
//! Plop down images starting at startpage.
/*! \ingroup extras
 * If there are more images than pages, then add pages. Centers images on each page
 * with the page's default dpi. (assumes that the layer is not transformed in any way)
 *
 * Assumes that each image is to be managed with its inc_count() and dec_count() functions
 * in the future. Adds a count of 1 to the item's count. Does not
 * delete the images array, the calling code should do that.
 *
 * If imagesperpage==-2 then place all the images one one page. If imagesperpage==-1, then
 * put only as many as will fit onto each page.
 *
 * // *** perhaps dpi should be a per page feature? though gs only has it for whole doc, would be
 * reasonable to have page control it here. The imposition dpi is what gets sent to gs
 * 
 * \todo *** please note that i have big plans for this extra, involving being able to dump
 * stuff into 'arrangements' which will be kind of like template pages. Each spot of an
 * arrangement will be able to hold the item(s) centered, scaled to fit, scaled and rotated to fit,
 * or flowed into the spot. ultimately, this should also
 * be able to reasonably handle page shapes that are not rectangles..
 * 
 * Returns the page index of the final page.
 */
int dumpImages(Document *doc, int startpage, ImageData **images, int nimages, int imagesperpage, int ddpi)
{
	DBG cout<<"---dump "<<nimages<<" ImageData..."<<endl;
	if (nimages<=0) return -1;
	if (startpage<0) startpage=0;

	Group *g;
	int endpage;
	int dpi;
	if (ddpi>50) dpi=ddpi; else dpi=doc->docstyle->imposition->paperstyle->dpi;
	endpage=startpage-1;
	int maxperpage=imagesperpage;
	if (maxperpage==-2) maxperpage=1000001;
	if (maxperpage<0) maxperpage=1000000;
	double x,y,w,h,ww,hh,s,rw,rh,rrh;
	int c,c2,c3,n,nn,nr=1;
	n=0; // total number of images placed, nn is placed for page
	SomeData *outline=NULL;
	for (c=0; c<nimages; c++) {
		DBG cout <<"  starting page "<<endpage+1<<" with index "<<c<<endl;
		if (!images[c]) continue;
		endpage++;
		if (endpage>=doc->pages.n) { 
			DBG cout <<" adding new page..."<<endl;
			doc->NewPages(-1,1); // add 1 extra page at end
		}
		
		 // figure out page characteristics: dpi, w, h, and scaling
		s=1./dpi; 
		if (outline) { outline->dec_count(); outline=NULL; }
		outline=doc->docstyle->imposition->GetPage(endpage,0); //adds 1 count already
		ww=outline->maxx-outline->minx;
		hh=outline->maxy-outline->miny;;
		//cout <<"  image("<<images[c]->object_id<<") "<<images[c]->filename<<": ww,hh:"<<
		//	ww<<','<<hh<<"  x,y,w,h"<<x<<','<<y<<','<<w<<','<<h<<endl;
		
		 // figure out placement
		rw=rh=rrh=0;
		nn=0;
		nr=0;
		do { //rows
			rw=rh=0;
			DBG cout <<"  row number "<<++nr<<endl;
			for (c2=0; c2<maxperpage-nn && c2<nimages-n-nn; c2++) {
				images[nn+c+c2]->xaxis(flatpoint(s,0));
				images[nn+c+c2]->yaxis(flatpoint(0,s));
				w=(images[nn+c+c2]->maxx-images[nn+c+c2]->minx)*s;
				h=(images[nn+c+c2]->maxy-images[nn+c+c2]->miny)*s;
				if (c2 && rw+w>ww) break; // fit all that could be fit on row
				rw+=w;
				if (h>rh) rh=h;
			}
			if (nn && rrh+rh>hh && maxperpage!=1000001) break; // row would be off page, so go on to next page
			x=(ww-rw)/2+outline->minx;
			y=hh-rrh-rh/2+outline->miny;
			for (c3=0; c3<c2; c3++) {
				w=(images[nn+c+c3]->maxx-images[nn+c+c3]->minx)*s;
				h=(images[nn+c+c3]->maxy-images[nn+c+c3]->miny)*s;
				images[nn+c+c3]->origin(flatpoint(x,y-h/2));
				x+=w;
			}
			nn+=c2;
			rrh+=rh;
		} while (nn<maxperpage && n+nn<nimages); // continue doing row

		 // push images onto the page
		DBG cout <<"  add "<<nn<<" images to page "<<endpage<<endl;
		for (c2=0; c2<nn; c2++) {
			DBG cout <<"   adding image index "<<c+c2<<endl;
			images[c+c2]->origin(images[c+c2]->origin()+flatpoint(0,(rrh-hh)/2));
			g=doc->pages.e[endpage]->e(doc->pages.e[endpage]->layers.n()-1);
			g->push(images[c+c2],0); //incs the obj's count
		}
		n+=nn;
		c+=nn-1; //the for loop adds on 1 more
	} // end loop block for page
	DBG cout <<"-----------------end dump images[]----------------"<<endl;
	return endpage;
}


//------------------------------ Images for page numbers------------------------------------

////! Apply images of page numbers to the pages of the document.
///*! Say you draw little pictures to be the page numbers, and put the images in the directory
// *  pathtonums, then use the files in there as page numbers..
// *
// * *** this must be an automatic placement for new pages!!
// */
//int applyPageNumbers(Document *doc,const char *pathtonums=NULL)
//{
//}

