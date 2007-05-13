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
// Copyright (C) 2004-2007 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../laidout.h"
#include "ppt.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


//! Internal function to dump out the obj if it is an ImageData.
void pptdumpobj(FILE *f,double *mm,SomeData *obj)
{
	ImageData *img;
	img=dynamic_cast<ImageData *>(obj);
	if (!img || !img->filename) return;

	double m[6];
	transform_mult(m,img->m(),mm);
	
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

//! Import a Passepartout file.
/*! If doc!=NULL, then import the pptout files to Document starting at page startpage.
 * Otherwise, create a brand new Singles based document.
 *
 * Does no check on the file to ensure that it is in fact a Passepartout file.
 *
 * It will be a file something like:
 * <pre>
 * <?xml version="1.0"?>
 * <document paper_name="Letter" doublesided="true" landscape="false" first_page_num="1">
 *   <page>
 *      <frame name="Raster beetile-501x538.jpg"
 *             matrix="0.812233 0 0 0.884649 98.6048 243.107"
 *             lock="false"
 *             flowaround="false"
 *             obstaclemargin="0" 
 *             type="raster"
 *             file="beetile-501x538.jpg"/>
 *   </page>
 * </document>         
 * </pre>
 *
 * \todo ***** finish imp me!
 * \todo there should be a way to preserve any elements that laidout doesn't understand, so
 *   when outputting as ppt, these elements would be written back out maybe...
 */
Document *pptin(const char *file,Document *doc,int startpage)
{
	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
	if (!att) return NULL;
	
	int c;
	Attribute *pptdoc=att->find("document"),
			  *page, *frame, *a;
	if (!pptdoc) { delete att; return NULL; }
	
	 //figure out the paper size, orientation
	a=pptdoc->find("paper_name");
	PaperStyle *paper=NULL;
	if (a) {
		for (c=0; c<laidout->papersizes.n; c++)
			if (!strcasecmp(laidout->papersizes.e[c]->name,a->value)) {
				paper=laidout->papersizes.e[c];
				break;
			}
	}
	if (!paper) paper=laidout->papersizes.e[0];

	 //figure out orientation
	int landscape;
	a=pptdoc->find("landscape");
	if (a) landscape=BooleanAttribute(a->value);
	else landscape=0;
	
	 // read in pages
	int pagenum=0;
	pptdoc=pptdoc->find("contents");
	if (!pptdoc) { delete att; return NULL; }

	 //create the document
	if (!doc) {
		Imposition *imp=new Singles;
		imp->SetPaperSize(paper);
		imp->paperstyle->flags=((imp->paperstyle->flags)&~1)|(landscape?1:0);
		DocumentStyle *docstyle=new DocumentStyle(imp);
		doc=new Document(docstyle,"untitled");//**** laidout should keep track of: untitled1, untitled2, ...
	}

	ImageData *image;
	LaxImage *img=NULL;
	Attribute *t,*n,*m;
	double M[6];
	
	for (c=0; c<pptdoc->attributes.n; c++) {
		if (!strcmp(pptdoc->attributes.e[c]->name,"page")) {
			if (pagenum>doc->pages.n) doc->NewPages(-1,1);
			page=pptdoc->attributes.e[c];
			for (int c2=0; c2<page->attributes.n; c2++) {
				if (!strcmp(page->attributes.e[c]->name,"frame")) {
					frame=page->attributes.e[c];
					a=frame->find("file");
					t=frame->find("type");
					n=frame->find("name");
					m=frame->find("matrix");
					if (a && a->value && t && !strcmp(t->value,"raster")) {
						img=load_image(a->value);
						if (img) {
							image=new ImageData;
							if (n) image->SetDescription(n->value);
							image->SetImage(img);
							if (m) DoubleListAttribute(m->value,M,6,NULL);
							dynamic_cast<Group *>(doc->pages.e[pagenum]->layers.e(0))->push(image,0);
							image->dec_count();
						}
					}
				}
			}
			pagenum++;
		}
	}
	
	 //*** set up page labels for "first_page_num"
	
	 //establish doc in project
	laidout->project->docs.push(doc);
	laidout->app->addwindow(newHeadWindow(doc));
	
	delete att;
	return doc;
}
