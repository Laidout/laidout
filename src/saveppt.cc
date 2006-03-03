/************ saveppt.cc *****************/

#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include "saveppt.h"
#include <iostream>
using namespace std;
using namespace Laxkit;


//! Internal function to dump out the obj if it is an ImageData.
void pptdumpobj(FILE *f,double *mm,SomeData *obj)
{
	ImageData *img;
	img=dynamic_cast<ImageData *>(obj);
	double m[6];
	transform_mult(m,img->m(),mm);
	if (!img || !img->filename) return;
	
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
	if (!doc->docstyle || !doc->docstyle->disposition || !doc->docstyle->disposition->paperstyle) return 1;
	
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
				doc->docstyle->disposition->paperstyle->name, 
				((doc->docstyle->disposition->paperstyle->flags&1)?"true":"false"));
	
	 // Write out paper spreads....
	Spread *spread;
	double m[6];
	int c,c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);
	for (c=0; c<doc->docstyle->disposition->numpapers; c++) {
		fprintf(f,"  <page>\n");
		spread=doc->docstyle->disposition->PaperLayout(c);
		 // for each page in paper layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n; l++) {
				 // for each object in layer
				for (c3=0; c3<doc->pages[pg]->layers.e[l]->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					pptdumpobj(f,m,doc->pages[pg]->layers.e[l]->e(c3));
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
