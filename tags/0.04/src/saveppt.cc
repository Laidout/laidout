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
/************ saveppt.cc *****************/

#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include "saveppt.h"
#include <iostream>
using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;


//! Internal function to dump out the obj if it is an ImageData.
void pptdumpobj(FILE *f,double *mm,SomeData *obj)
{
	ImageData *img;
	img=dynamic_cast<ImageData *>(obj);
	if (!img || !img->filename) return;

	double m[6];
	transform_mult(m,img->m(),mm);
	
	//<frame name="Raster sewage1.tiff" matrix="0.218182 0 0 0.218182 30.4246 36.7684" 
	//	lock="false" flowaround="false" obstaclemargin="0" type="raster" file="/home/tom/cartoons/graphic/sewage/tiffs/sewage1.tiff"/>
		
	char *bname=basename(img->filename); // Warning! This assumes the GNU basename, which does
										 // not modify the string.
	fprintf(f,"    <frame name=\"Raster %s\" matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
			bname, m[0]*72, m[1]*72, m[2]*72, m[3]*72, m[4]*72, m[5]*72);
	fprintf(f,"lock=\"false\" flowaround=\"false\" obstaclemargin=\"0\" type=\"raster\" file=\"%s\" />\n",
			img->filename);
}


//! Save the document as a Passepartout file to doc->saveas".ppt"
/*! This only saves unnested images, and the page size and orientation.
 *
 * \todo *** just dumps out paper name, does not check to ensure it
 * is valid for ppt.
 */
int pptout(Document *doc)
{
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paperstyle) return 1;
	
	FILE *f=NULL;
	if (!doc->saveas || !strcmp(doc->saveas,"")) {
		cout <<"**** cannot save, doc->saveas is null."<<endl;
		return 2;
	}
	char *filename=newstr(doc->saveas);
	appendstr(filename,".ppt");
	f=fopen(filename,"w");
	delete[] filename;
	if (!f) {
		cout <<"**** cannot save, doc->saveas cannot be opened for writing."<<endl;
		return 3;
	}
	
	 // write out header
	fprintf(f,"<?xml version=\"1.0\"?>\n");
	fprintf(f,"<document paper_name=\"%s\" doublesided=\"false\" landscape=\"%s\" first_page_num=\"1\">\n",
				doc->docstyle->imposition->paperstyle->name, 
				((doc->docstyle->imposition->paperstyle->flags&1)?"true":"false"));
	
	 // Write out paper spreads....
	Spread *spread;
	Group *g;
	double m[6];
	int c,c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);
	for (c=0; c<doc->docstyle->imposition->numpapers; c++) {
		fprintf(f,"  <page>\n");
		spread=doc->docstyle->imposition->PaperLayout(c);
		 // for each page in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
				for (c3=0; c3<g->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					pptdumpobj(f,m,g->e(c3));
				}
			}
		}

		delete spread;
		fprintf(f,"  </page>\n");
	}
		
	 // write out footer
	fprintf(f,"</document>\n");
	
	fclose(f);
	return 0;
	
}
