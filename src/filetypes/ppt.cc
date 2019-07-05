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
// Copyright (C) 2004-2007 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../core/stylemanager.h"
#include "../ui/headwindow.h"
#include "../impositions/singles.h"
#include "../core/utils.h"
#include "../core/drawdata.h"
#include "../printing/psout.h"
#include "../dataobjects/mysterydata.h"
#include "ppt.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {



//--------------------------------- install Passepartout filter

//! Tells the Laidout application that there's a new filter in town.
void installPptFilter()
{
	PptoutFilter *pptout=new PptoutFilter;
	pptout->GetObjectDef();
	laidout->PushExportFilter(pptout);
	
	PptinFilter *pptin=new PptinFilter;
	pptin->GetObjectDef();
	laidout->PushImportFilter(pptin);
}


//------------------------------------ PptExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newPptExportConfig()
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Passepartout"))
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//------------------------------------ PptImportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newPptImportConfig()
{
	ImportConfig *d=new ImportConfig;
	for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"Passepartout"))
			d->filter=laidout->importfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}


//------------------------------- PptoutFilter --------------------------------------
/*! \class PptoutFilter
 * \brief Output filter for Passepartout files.
 */

PptoutFilter::PptoutFilter()
{
	flags=FILTER_MULTIPAGE;
}

const char *PptoutFilter::VersionName()
{
	return _("Passepartout");
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *PptoutFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("PassepartoutExportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"PassepartoutExportConfig");
	makestr(styledef->Name,_("Passepartout Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a Passepartout file."));
	styledef->newfunc=newPptExportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}

//! Internal function to dump out the obj if it is an ImageData.
/*! \todo deal with SomeDataRef
 */
static void pptdumpobj(FILE *f,double *mm,SomeData *obj,int indent,ErrorLog &log)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (!strcmp(obj->whattype(),"Group")) {
		Group *g;
		g=dynamic_cast<Group *>(obj);
		if (!g || !g->n()) return;

		double m[6];
		if (mm) transform_mult(m,g->m(),mm);
		else transform_copy(m,g->m());
		
		fprintf(f,"%s<frame type=\"group\" transform=\"%.10g %.10g %.10g %.10g %.10g %.10g\" >\n",
				spc, m[0], m[1], m[2], m[3], m[4], m[5]);
		transform_identity(m);
		for (int c=0; c<g->n(); c++) pptdumpobj(f,m,g->e(c),indent+2,log);

		fprintf(f,"%s</frame>\n",spc);
		return;

	} else if (!strcmp(obj->whattype(),"ImageData")) {
		ImageData *img;
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;

		double m[6];
		if (mm) transform_mult(m,img->m(),mm);
		else transform_copy(m,img->m());
		
		char *bname=basename(img->filename); // Warning! This assumes the GNU basename, which does
											 // not modify the string.
		fprintf(f,"%s<frame name=\"Raster %s\" matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
				spc, bname, m[0], m[1], m[2], m[3], m[4], m[5]);
		fprintf(f,"lock=\"false\" flowaround=\"false\" obstaclemargin=\"0\" type=\"raster\" file=\"%s\" />\n",
				img->filename);
		return;

//	} else if (!strcmp(obj->whattype(),"EpsData")) {
//		 // just like ImageData, but outputs as type="Image"
//		EpsData *eps;
//		eps=dynamic_cast<EpsData *>(obj);
//		if (!eps || !eps->filename) return;
//
//		double m[6];
//		if (mm) transform_mult(m,eps->m(),mm);
//		else transform_copy(m,eps->m());
//		
//		char *bname=basename(eps->filename); // Warning! This assumes the GNU basename, which does
//											 // not modify the string.
//		fprintf(f,"%s<frame name=\"Image %s\" matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
//				spc, bname, m[0], m[1], m[2], m[3], m[4], m[5]);
//		fprintf(f,"lock=\"false\" flowaround=\"false\" obstaclemargin=\"0\" type=\"image\" file=\"%s\" />\n",
//				eps->filename);
//		return;

	} else if (!strcmp(obj->whattype(),"MysteryData")) {
		MysteryData *mdata=dynamic_cast<MysteryData *>(obj);
		 //NOTE: mdata->importer should be the same as PptoutFilter::VersionName()
		if (!mdata || !mdata->importer 
				   || strcmp(mdata->importer,_("Passepartout"))
				   || !mdata->attributes) return;

		double m[6];
		if (mm) transform_mult(m,mdata->m(),mm);
		else transform_copy(m,mdata->m());

		fprintf(f,"%s<frame matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
				spc, m[0], m[1], m[2], m[3], m[4], m[5]);
		Attribute *att;
		for (int c=0; c<mdata->attributes->attributes.n; c++) {
			att=mdata->attributes->attributes.e[c];
			if (!att) continue;
			if (!strcmp(att->name,"matrix")) continue;
			fprintf(f,"%s=\"%s\" ",att->name,att->value);
		}
		fprintf(f,"/>\n");
	
		return;

	} else {
		setlocale(LC_ALL,"");
		char buffer[strlen(_("Cannot export %s objects to passepartout."))+strlen(obj->whattype())+1];
		sprintf(buffer,_("Cannot export %s objects to passepartout."),obj->whattype());
		log.AddMessage(obj->object_id,NULL,NULL,buffer,ERROR_Warning);
		setlocale(LC_ALL,"C");
	}
}

static const char *pptpaper[12]= {
		"A0",
		"A1",
		"A2",
		"A3",
		"A4",
		"A5",
		"A6",
		"Executive",
		"Legal",
		"Letter",
		"Tabloid/Ledger",
		NULL
	};

//! Save the document as a Passepartout file.
/*! This only saves groups and images, and the page size and orientation.
 *
 * If the paper name is not recognized as a Passepartout paper name, which are
 * A0-A6, Executive (7.25 x 10.5in), Legal, Letter, and Tabloid/Ledger, then
 * Letter is used.
 *
 * \todo if unknown paper, should really use some default paper size, if it is valid, 
 *   and then otherwise "Letter", or choose a size that is big enough to hold the spreads
 * \todo for singles, should figure out what paper size to export as..
 */
int PptoutFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DocumentExportConfig *config=dynamic_cast<DocumentExportConfig *>(context);
	if (!config) return 1;
	
	Document *doc =config->doc;
	int start     =config->start;
	int end       =config->end;
	int layout    =config->layout;
	Group *limbo  =config->limbo;
	PaperGroup *papergroup=config->papergroup;
	if (!filename) filename=config->filename;
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->imposition || !doc->imposition->paper)...
		log.AddMessage(_("Nothing to export!"),ERROR_Fail);
		return 1;
	}
	
	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			log.AddMessage(_("Cannot save without a filename."),ERROR_Fail);
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".ppt");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,&log);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}
	
	setlocale(LC_ALL,"C");
	
	 //figure out paper size and orientation
	const char *papersize=NULL, *landscape=NULL;
	const char **tmp;
	//double paperwidth; //,paperheight;

	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	 // The ps Orientation comment determines how onscreen viewers will show 
	 // pages. This can be overridden by the %%PageOrientation: comment
	landscape=(papergroup->papers.e[0]->box->paperstyle->flags&1)?"true":"false";
	//paperwidth=papergroup->papers.e[0]->box->paperstyle->width;

	 //match the first paper name with a passepartout paper name
	tmp=pptpaper;
	for (int c=0; *tmp; c++, tmp++) {
		if (!strcmp(papergroup->papers.e[0]->box->paperstyle->name,*tmp)) break;
	}
	if (*tmp) papersize=*tmp;
	else if (!strcmp(papergroup->papers.e[0]->box->paperstyle->name,"Tabloid")) {
		papersize="Tabloid/Ledger";
		if (landscape[0]=='t') landscape="false"; else landscape="true";
	} else if (!strcmp(papergroup->papers.e[0]->box->paperstyle->name,"Ledger")) {
		papersize="Tabloid/Ledger";
	} else papersize="Letter";
	
	 // write out header
	fprintf(f,"<?xml version=\"1.0\"?>\n");
	fprintf(f,"<document paper_name=\"%s\" doublesided=\"false\" landscape=\"%s\" first_page_num=\"%d\">\n",
				papersize, landscape, start);
	
	 // write out text_stream from doc->iohints, if any
	if (doc) {
		Attribute *att,*iohints=doc->iohints.find(_("Passepartout"));
		if (iohints) {
			for (int c2=0; c2<iohints->attributes.n; c2++) {
				if (strcmp(iohints->attributes.e[c2]->name,"text_stream")) continue;

				fprintf(f,"  <text_stream ");
				for (int c3=0; c3<iohints->attributes.e[c2]->attributes.n; c3++) {
					att=iohints->attributes.e[c2]->attributes.e[c3];
					fprintf(f,"%s=\"%s\" ",att->name?att->name:"",
										   att->value?att->value:"");
				}
				fprintf(f,"/>\n");
			}
		}
	}

	 // Write out paper spreads....
	Spread *spread=NULL;
	Group *g=NULL;
	double m[6],mm[6],mmm[6];
	int p,c2,l,pg,c3;
	
	transform_set(mm,72,0,0,72,0,0);
	psCtmInit();
	for (int c=start; c<=end; c++) {
		if (config->evenodd==DocumentExportConfig::Even && c%2==0) continue;
        if (config->evenodd==DocumentExportConfig::Odd && c%2==1) continue;

		if (doc) spread=doc->imposition->Layout(layout,c);
		for (p=0; p<papergroup->papers.n; p++) {
			fprintf(f,"  <page>\n");

			 //begin paper contents
			//fprintf(f,"[72 0 0 72 0 0] concat\n"); // convert to inches
			//psConcat(72.,0.,0.,72.,0.,0.);
			//if (plandscape) {
			//	//fprintf(f,"%.10g 0 translate\n90 rotate\n",paperwidth);
			//	psConcat(0.,1.,-1.,0., paperwidth,0.);
			//}

			psPushCtm();
			transform_invert(mmm,papergroup->papers.e[p]->m());
			transform_mult(m,mmm,mm);
			fprintf(f,"    <frame type=\"group\" transform=\"%.10g %.10g %.10g %.10g %.10g %.10g\" >\n",
				      m[0], m[1], m[2], m[3], m[4], m[5]);
			psConcat(m);

			if (limbo && limbo->n()) {
				//*** if limbo bbox inside paper bbox? could loop in limbo objs for more specific check
				pptdumpobj(f,NULL,limbo,4,log);
			}

			if (papergroup->objs.n()) {
				pptdumpobj(f,NULL,papergroup->objs.e(c3),6,log);
			}

			if (spread) {
				 // print out printer marks
				if ((spread->mask&SPREAD_PRINTERMARKS) && spread->marks) {
					//fprintf(f," .01 setlinewidth\n");
					//DBG cerr <<"marks data:\n";
					//DBG spread->marks->dump_out(stderr,2,0);
					pptdumpobj(f,m,spread->marks,4,log);
				}
				
				 // for each page in spread..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
					pg=spread->pagestack.e[c2]->index;
					if (pg>=doc->pages.n) continue;
					
					 // for each layer on the page..
					for (l=0; l<doc->pages[pg]->layers.n(); l++) {

						 // for each object in layer
						g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							transform_copy(mmm,spread->pagestack.e[c2]->outline->m());
							pptdumpobj(f,mmm,g->e(c3),6,log);
						}
					}
				}

			}
			psPopCtm(); //remove papergroup->paper transform
			fprintf(f,"    </frame>\n");
			fprintf(f,"  </page>\n");
		}
		if (spread) { delete spread; spread=NULL; }
	}
		
	 // write out footer
	fprintf(f,"</document>\n");
	
	fclose(f);
	setlocale(LC_ALL,"");
	delete[] file;
	return 0;
	
}

////------------------------------------- PptinFilter -----------------------------------
/*! \class PptinFilter 
 * \brief Passepartout input filter.
 */


const char *PptinFilter::FileType(const char *first100bytes)
{
	if (!strncmp(first100bytes,"<?xml version=\"1.0\"?>\n<document",
						   strlen("<?xml version=\"1.0\"?>\n<document")))
		return "0.6";
	return NULL;
}

const char *PptinFilter::VersionName()
{
	return _("Passepartout");
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *PptinFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("PassepartoutImportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"PassepartoutImportConfig");
	makestr(styledef->Name,_("Passepartout Import Configuration"));
	makestr(styledef->description,_("Configuration to import a Passepartout file."));
	styledef->newfunc=newPptImportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
}

//! Import a Passepartout file.
/*! If doc!=NULL, then import the pptout files to Document starting at page startpage.
 * If doc==NULL, create a brand new Singles based document.
 *
 * Does no check on the file to ensure that it is in fact a Passepartout file.
 *
 * It will be a file something like:
 * \verbatim
  <?xml version="1.0"?>
  <document paper_name="Letter" doublesided="true" landscape="false" first_page_num="1">
    <text_stream name="stream1" file="8jeev10.txt" transform=""/>
    <page>
       <frame name="Raster beetile-501x538.jpg"
              matrix="0.812233 0 0 0.884649 98.6048 243.107"
              lock="false"
              flowaround="false"
              obstaclemargin="0" 
              type="raster"
              file="beetile-501x538.jpg"/>
       <frame name="Text stream1"
              matrix="1 0 0 1 140.833 145.245"
              lock="false"
              flowaround="false"
              obstaclemargin="0"
              type="text"
              width="200"
              height="300"
              num_columns="1"
              gutter_width="12"
              stream="stream1"/>
    </page>
  </document>         
 \endverbatim
 *
 * \todo ***** finish imp me!
 * \todo there should be a way to preserve any elements that laidout doesn't understand, so
 *   when outputting as ppt, these elements would be written back out maybe...
 * \todo *** if first_page_num exists, then must set up page labels
 * \todo *** implement dump into group, rather than doc
 */
int PptinFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log, const char *filecontents,int contentslen)
{
	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	Document *doc=in->doc;

	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
	if (!att) return 2;
	
	int c;
	Attribute *pptdoc=att->find("document"),
			  *page, *a;
	if (!pptdoc) { delete att; return 3; }
	
	 //figure out the paper size, orientation
	PaperStyle *paper=NULL;
	int landscape;

	a=pptdoc->find("paper_name");
	if (a) {
		for (c=0; c<laidout->papersizes.n; c++)
			if (!strcasecmp(laidout->papersizes.e[c]->name,a->value)) {
				paper=laidout->papersizes.e[c];
				break;
			}
	}
	if (!paper) paper=laidout->papersizes.e[0];

	 //figure out orientation
	a=pptdoc->find("landscape");
	if (a) landscape=BooleanAttribute(a->value);
	else landscape=0;
	
	 // find pages content
	int pagenum=in->topage; //the page in doc to start dumping into
	if (pagenum<0) pagenum=0;

	pptdoc=pptdoc->find("content:");
	if (!pptdoc) { delete att; return 4; }

	 //now pptdoc's subattributes should be one or more "page" or "text_stream" blocks 

	 //create the document
	if (!doc && !in->toobj) {
		Imposition *imp=new Singles;
		paper->flags=((paper->flags)&~1)|(landscape?1:0);
		imp->SetPaperSize(paper);
		doc=new Document(imp,Untitled_name());
		imp->dec_count();
	}

	Attribute *ppthints=NULL;
	
	Group *group=in->toobj;

	for (c=0; c<pptdoc->attributes.n; c++) {
		if (!strcmp(pptdoc->attributes.e[c]->name,"page")) {
			if (doc && pagenum>=doc->pages.n) doc->NewPages(-1,doc->pages.n-pagenum+1);

			if (doc) {
				 //update group to point to the document page's group
				group=dynamic_cast<Group *>(doc->pages.e[pagenum]->layers.e(0)); //pick layer 0 of the page
			}

			page=pptdoc->attributes.e[c]->find("content:");

			 //each page has zero or more "frame" attributes, but the type of the
			 //frame is within the frame, images and text do not exist as their very
			 //own objects. They are distinguished by the "type" field.
			pptDumpInGroup(page, group);

			pagenum++;
		} else if (in->keepmystery && !strcmp(pptdoc->attributes.e[c]->name,"text_stream")) {
			 //<text_stream name="stream1" file="8jeev10.txt" transform=""/>
			//***doc->attachFragment(VersionName(),pptdoc->attributes.e[c]);
			if (!ppthints) ppthints=new Attribute(_("Passepartout"), file);
			ppthints->push(pptdoc->attributes.e[c],-1);
			pptdoc->attributes.e[c]=NULL;//blank out original so it is not doubly deleted
		}
	}
	
	 //*** set up page labels for "first_page_num"
	
	 //install global hints if they exist
	if (ppthints) {
		 //remove the old iohint if it is there
		Attribute *iohints=(doc?&doc->iohints:&laidout->project->iohints);
		Attribute *oldppt=iohints->find(_("Passepartout"));
		if (oldppt) {
			iohints->attributes.remove(iohints->attributes.findindex(oldppt));
		}
		iohints->push(ppthints,-1);
		//remember, do not delete ppthints here!
	}

	 //if doc is new, push into the project
	if (doc && doc!=in->doc) {
		laidout->project->Push(doc);
		laidout->app->addwindow(newHeadWindow(doc));
	}
	
	delete att;
	return 0;
}

/*! Return the number of objects pushed in the group
 */
int PptinFilter::pptDumpInGroup(Attribute *att, Group *group)
{
	Attribute *t, *a, *n, *m;
	Attribute *frame;
	double M[6];
	int numobjs=0;
	ImageData *image=NULL;
	LaxImage *img=NULL;

	for (int c=0; c<att->attributes.n; c++) {
		if (strcmp(att->attributes.e[c]->name,"frame")) continue;
		
		frame=att->attributes.e[c];
		t=frame->find("type");
		a=frame->find("file");
		n=frame->find("name");
		m=frame->find("matrix");
		if (!m) m=frame->find("transform");

		if (m) {
			DoubleListAttribute(m->value,M,6,NULL);
			for (int c2=0; c2<6; c2++) 
				M[c2]/=72;
		}

		if (!t) continue; //missing a type

		if (!strcmp(t->value,"raster")) {
			img = ImageLoader::LoadImage(a->value);
			if (img) {
				image=new ImageData;
				if (n) image->SetDescription(n->value);
				image->SetImage(img, NULL);
				img->dec_count();
				if (m) image->m(M);
				group->push(image);
				image->dec_count();
				numobjs++;
			}

		} else if (!strcmp(t->value,"group")) {
			Group *newgroup=new Group;
			if (m) newgroup->m(M);
			if (pptDumpInGroup(frame->find("content:"),newgroup)>0) {
				group->push(newgroup);
				numobjs++;
			}
			newgroup->dec_count();

		} else if (!strcmp(t->value,"image")) {
			cout <<"*** need to implement \"image\" type for passepartout import!!"<<endl;
			//***for eps objects

		} else if (!strcmp(t->value,"text")) {
			MysteryData *d=dynamic_cast<MysteryData *>(newObject("MysteryData"));
			d->importer=newstr(VersionName());
			if (n) d->name=newstr(n->value);
			 //<frame name="Text stream1"
			 //       matrix="1 0 0 1 140.833 145.245"
			 //       lock="false"
			 //       flowaround="false"
			 //       obstaclemargin="0"
			 //       type="text"
			 //       width="200"
			 //       height="300"
			 //       num_columns="1"
			 //       gutter_width="12"
			 //       stream="stream1"/>
			if (m) d->m(M);
			char *name,*value;
			for (int c3=0; c3<frame->attributes.n; c3++) {
				name= frame->attributes.e[c3]->name;
				value=frame->attributes.e[c3]->value;
				if (!strcmp(name,"name")) {
					//if (!isblank(value)) makestr(d->id,value);
				} else if (!strcmp(name,"width")) {
					DoubleAttribute(value,&d->maxx);
				} else if (!strcmp(name,"height")) {
					DoubleAttribute(value,&d->maxy);
				//} else if (!strcmp(name,"lock")) {
				//} else if (!strcmp(name,"flowaround")) {
				//} else if (!strcmp(name,"obstaclemargin")) {
				//} else if (!strcmp(name,"num_columns")) {
				//} else if (!strcmp(name,"gutter_width")) {
				//} else if (!strcmp(name,"stream")) {
				}
			}

			d->installAtts(frame);
			att->attributes.e[c]=NULL; //since this was frame
			group->push(d);
			d->dec_count();
			numobjs++;
		}
	} //loop over page contents

	return numobjs;
}


} // namespace Laidout

