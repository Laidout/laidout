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
// Copyright (C) 2007 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>

#include "../language.h"
#include "scribus.h"
#include "../laidout.h"
#include "../printing/psout.h"
#include "../utils.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"
#include "../dataobjects/mysterydata.h"
#include "../drawdata.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;




static void scribusdumpobj(FILE *f,double *mm,SomeData *obj,char **error_ret,int &warning);

//1.5 inches and 1/4 inch
#define CANVAS_MARGIN_X 100.
#define CANVAS_MARGIN_Y 20.
#define CANVAS_GAP      40.


//--------------------------------- install Scribus filter

//! Tells the Laidout application that there's a new filter in town.
void installScribusFilter()
{
	ScribusExportFilter *scribusout=new ScribusExportFilter;
	laidout->exportfilters.push(scribusout);
	
	ScribusImportFilter *scribusin=new ScribusImportFilter;
	laidout->importfilters.push(scribusin);
}

//---------------------------- ScribusExportFilter --------------------------------

/*! \class ScribusExportFilter
 * \brief Export as 1.3.3.12 or thereabouts.
 *
 * <pre>
 *  current 1.3.3.* file format is supposedly at:
 *  http://docs.scribus.net/index.php?lang=en&sm=scribusfileformat&page=scribusfileformat
 *  but it only has fully written up 1.2 format
 * </pre>
 */

#define PTYPE_None                  -1
#define PTYPE_Image                  2
#define PTYPE_Text                   4
#define PTYPE_Line                   5
#define PTYPE_Polygon                6
#define PTYPE_Polyline               7
#define PTYPE_Text_On_Path           8
#define PTYPE_Laidout_Gradient       -2
#define PTYPE_Laidout_MysteryData    -3

ScribusExportFilter::ScribusExportFilter()
{
	flags=FILTER_MULTIPAGE;
}

//! "Scribus 1.3.3.12".
const char *ScribusExportFilter::VersionName()
{
	return _("Scribus 1.3.3.12");
}

static int currentpage;
static NumStack<int> groups;
//static int groupc=1;
static int ongroup;

static int countGroups(Group *g)
{
	int count=0;
	Group *grp;
	for (int c=0; c<g->n(); c++) {
		 // for each object in group
		grp=dynamic_cast<Group *>(g->e(c));
		if (grp) {
			count++;
			count+=countGroups(grp);
		}
	}
	return count;
}


//! Export the document as a Scribus file.
/*! error_ret is appended to if possible.
 */
int ScribusExportFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	Group *limbo  =out->limbo;
	PaperGroup *papergroup=out->papergroup;
	if (!filename) filename=out->filename;
	
	 //we must have something to export...
	if (!doc && !limbo) {
		//|| !doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paper)...
		if (error_ret) appendline(*error_ret,_("Nothing to export!"));
		return 1;
	}
	
	Attribute *scribushints=NULL;
	if (doc) scribushints=doc->iohints.find("Scribus");
	else scribushints=laidout->project->iohints.find("Scribus");
	
	 //we must be able to open the export file location...
	FILE *f=NULL;
	char *file=NULL;
	if (!filename) {
		if (isblank(doc->saveas)) {
			DBG cerr <<" cannot save, null filename, doc->saveas is null."<<endl;
			
			if (error_ret) appendline(*error_ret,_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".sla");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,error_ret);//appends any error string
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}
	

	setlocale(LC_ALL,"C");

	int warning=0;
	Spread *spread=NULL;
	Group *g=NULL;
	int c,c2,l,pg,c3;

	
//	 //find out how many groups there are for DOCUMENT->GROUPC
//	 //**** is this necessary? marked as optional in 1.2 spec
//	 //*** also grab all color references
//	groupc=0;
//	if (doc) {
//		for (c=start; c<=end; c++) {
//			spread=doc->docstyle->imposition->Layout(layout,c);
//			 // for each page in spread layout..
//			for (c2=0; c2<spread->pagestack.n; c2++) {
//				pg=spread->pagestack.e[c2]->index;
//				if (pg>=doc->pages.n) continue;
//				 // for each layer on the page..
//				//groupc++;
//				g=&doc->pages.e[pg]->layers;
//				groupc+=countGroups(g);
//			}
//			delete spread; spread=NULL;
//		}
//	}
	
	 // write out header
	Attribute *temp=NULL;
	const char *str="1.3.3.12";
	if (scribushints) {
		temp=scribushints->find("scribusVersion");
		if (temp) str=temp->value;
	}
	 //>=1.3.5svn needs the xml line, otherwise omit
	if (!strncmp(str,"1.3.5",5)) {
		fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				  "<SCRIBUSUTF8NEW Version=\"%s\">\n",str);
	} else fprintf(f,"<SCRIBUSUTF8NEW Version=\"%s\">\n",str);
	
	
	 //figure out paper size and orientation
	int landscape=0,plandscape;
	double paperwidth,paperheight;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	landscape=(papergroup->papers.e[0]->box->paperstyle->flags&1)?1:0;
	paperwidth= papergroup->papers.e[0]->box->paperstyle->width;
	paperheight=papergroup->papers.e[0]->box->paperstyle->height;
	
	 //------------ write out document attributes
	 //****** all the scribushints.slahead blocks are output as DOCUMENT attributes
	 //		  EXCEPT: ANZPAGES, PAGEHEIGHT, PAGEWIDTH, ORIENTATION
	fprintf(f,"  <DOCUMENT \n"
			  "    ANZPAGES=\"%d\" \n",end-start+1); //number of pages
	int dodefaultdoc=1;
	if (scribushints) {
		Attribute *slahead=scribushints->find("slahead");
		if (slahead) {
			dodefaultdoc=0;
			 //assume all the imported document attributes can be output as is
			 //the page size/orientation is just default page size and orientation,
			 //so it's ok if we don't intercept
			char *name,*value;
			for (int c=0; c<slahead->attributes.n; c++) {
				name =slahead->attributes.e[c]->name;
				value=slahead->attributes.e[c]->value;
				if (!strcmp(name,"ANZPAGES")) continue;
				if (!strcmp(name,"content:")) continue; //shouldn't happen, but just in case
				fprintf(f,"    %s=\"%s\"\n", name, value?value:"");
			}
		}
	} 
	if (dodefaultdoc) {

		 //default DOCUMENT attributes
		fprintf(f,"    AUTHOR=\"\" \n"
				  "    ABSTPALTEN=\"11\" \n"  //Distance between Columns in automatic Textframes
				  "    AUTOSPALTEN=\"1\" \n" //Number of Colums in automatic Textframes
				  //"    BOOK***** " *** has facing pages if present: doublesidedsingles
				  "    BORDERBOTTOM=\"0\" \n"	//default margins!
				  "    BORDERLEFT=\"0\" \n"	
				  "    BORDERRIGHT=\"0\" \n"	
				  "    BORDERTOP=\"0\" \n"	
				  "    COMMENTS=\"\" \n"
				  "    DFONT=\"Times-Roman\" \n" //default font
				  "    DSIZE=\"12\" \n"         //default font size
				  //"    FIRSTLEFT \n"  //*** doublesidedsingles->isleft
				  "    FIRSTPAGENUM=\"%d\" \n", start); //***check this is right
		fprintf(f,"    KEYWORDS=\"\" \n"
				  "    ORIENTATION=\"%d\" \n",landscape);
		fprintf(f,"    PAGEHEIGHT=\"%f\" \n",paperheight);
		fprintf(f,"    PAGEWIDTH=\"%f\" \n",paperwidth);//***default page width and height
		fprintf(f,"    TITLE=\"\" \n"
				  "    VHOCH=\"33\" \n"      //Percentage for Superscript
				  "    VHOCHSC=\"100\" \n"  // Percentage for scaling of the Glyphs in Superscript
				  "    VKAPIT=\"75\" \n"    //Percentage for scaling of the Glyphs in Small Caps
				  "    VTIEF=\"33\" \n"       //Percentage for Subscript
				  "    VTIEFSC=\"100\" \n");   //Percentage for scaling of the Glyphs in Subscript
		fprintf(f,"    ScratchTop=\"%f\"\n"    //scratch space gap at top
				  "    ScratchBottom=\"%f\"\n" //scratch space gap at bottom of whole page layout
				  "    ScratchRight=\"%f\"\n"  //scratch space gap at right of whole layout
				  "    ScratchLeft=\"%f\"\n",  //scratch space gap at left of whole layout
			  	CANVAS_MARGIN_Y, CANVAS_MARGIN_Y, CANVAS_MARGIN_X, CANVAS_MARGIN_X);
	}
	fprintf(f,    "   >\n"); //close DOCUMENT tag
	

	//now we are in the DOCUMENT element section...
	

	if (scribushints) {
		 //write out any docContent elements as is. assumes these cover everything in the default setup
		 //other than PAGE and PAGEOBJECT
		char *name;
		for (int c=0; c<scribushints->attributes.n; c++) {
			name =scribushints->attributes.e[c]->name;
			if (strcmp(name,"docContent")) continue;

			AttributeToXMLFile(f,scribushints->attributes.e[c],4);
		}
	} else {
		 //write out default elements of DOCUMENT

		//****write out <COLOR> sections
			
		
		 //----------Write layers, assuming just background. Everything else is grouping
		fprintf(f,"    <LAYERS DRUCKEN=\"1\" NUMMER=\"0\" EDIT=\"1\" NAME=\"Background\" SICHTBAR=\"1\" LEVEL=\"0\" />\n");

		
		//********write out <PDF> chunk
		fprintf(f,"    <PDF displayThumbs=\"0\" "
						   "ImagePr=\"0\" "
						   "fitWindow=\"0\" "
						   "displayBookmarks=\"0\" "
						   "BTop=\"18\" "
						   "UseProfiles=\"0\" "
						   "BLeft=\"18\" "
						   "PrintP=\"Fogra27L CMYK Coated Press\" "
						   "RecalcPic=\"0\" "
						   "UseSpotColors=\"1\" "
						   "ImageP=\"sRGB IEC61966-2.1\" SolidP=\"sRGB IEC61966-2.1\" "
						   "PicRes=\"300\" "
						   "Thumbnails=\"0\" "
						   "hideToolBar=\"0\" "
						   "CMethod=\"0\" "
						   "displayLayers=\"0\" "
						   "doMultiFile=\"0\" "
						   "UseLayers=\"0\" "
						   "Encrypt=\"0\" "
						   "BRight=\"18\" "
						   "Binding=\"0\" "
						   "Articles=\"0\" "
						   "InfoString=\"\" "
						   "RGBMode=\"1\" "
						   "Grayscale=\"0\" "
						   "PresentMode=\"0\" "
						   "openAction=\"\" "
						   "displayFullscreen=\"0\" "
						   "Permissions=\"-4\" "
						   "Intent=\"1\" "
						   "Compress=\"1\" "
						   "hideMenuBar=\"0\" "
						   "Version=\"14\" "
						   "Resolution=\"300\" "
						   "Bookmarks=\"0\" "
						   "UseProfiles2=\"0\" "
						   "RotateDeg=\"0\" "
						   "Clip=\"0\" "
						   "MirrorV=\"0\" "
						   "Quality=\"0\" "
						   "PageLayout=\"0\" "
						   "UseLpi=\"0\" "
						   "PassUser=\"\" "
						   "BBottom=\"18\" "
						   "Intent2=\"1\" "
						   "MirrorH=\"0\" "
						   "PassOwner=\"\" >\n"
				  "      <LPI Angle=\"45\" Frequency=\"75\" SpotFunction=\"2\" Color=\"Black\" />\n"
				  "      <LPI Angle=\"105\" Frequency=\"75\" SpotFunction=\"2\" Color=\"Cyan\" />\n"
				  "      <LPI Angle=\"75\" Frequency=\"75\" SpotFunction=\"2\" Color=\"Magenta\" />\n"
				  "      <LPI Angle=\"90\" Frequency=\"75\" SpotFunction=\"2\" Color=\"Yellow\" />\n"
				  "    </PDF>\n");

		
		//*************DocItemAttributes not 1.2
		//************TablesOfContents   not 1.2
		//************Sections  not 1.2
		
		//------------PageSets  not 1.2
		//This sets up so there is one column of pages, with CANVAS_GAP units between them.
		fprintf(f,"    <PageSets>\n"
				  "      <Set Columns=\"1\" GapBelow=\"%g\" Rows=\"1\" FirstPage=\"0\" GapHorizontal=\"0\"\n"
				  "         Name=\"Single Page\" GapVertical=\"0\" />\n"
				  "    </PageSets>\n", CANVAS_GAP);

		//************MASTERPAGE not separate entity in 1.2
	} //if !scribushints for non PAGE and PAGEOBJECT elements of DOCUMENT
			


	//------------PAGE and PAGEOBJECTS

	 //
	 // holy cow, scribus has EVERY object by reference!!!!
	 // In Scribus, pages sit on a canvas, and the page has coords PAGEWIDTH,HEIGHT,XPOS,YPOS.
	 // Positive x goes to the right. Positive y goes down.
	 //
	 // Page objects in the file have coordinates in the canvas space, not the page space,
	 // but their coordinates in the Scribus ui are relative to the page.
	 // Their xpos, ypos, rot, width, and height (notably NOT shear)
	 // refer to the object's bounding box. The pocoor and cocoor coordinates are relative to that
	 // rotated and translated bounding box.
	 //
	 // Scribus Groups are more like sets. Objects all lie directly on the page, and groups
	 // are simply a loose tag the objects have. Groups do not apply any additional transformation.
	 //
	 // Scribus Layers are basically groups that have a few extra properties....? (need to research that!)
	 //
	groups.flush();
	ongroup=0;
	int p;
	double m[6],mm[6],mmm[6],mmmm[6],ms[6];
	transform_identity(m);

	psFlushCtms();
	psCtmInit();
	double pageypos=CANVAS_MARGIN_Y;
	int pagec;
	for (c=start; c<=end; c++) { //for each spread
		if (doc) spread=doc->docstyle->imposition->Layout(layout,c);
		for (p=0; p<papergroup->papers.n; p++) { //for each paper
			paperwidth= 72*papergroup->papers.e[p]->box->paperstyle->w(); //scribus wants visual w/h
			paperheight=72*papergroup->papers.e[p]->box->paperstyle->h();
			plandscape=papergroup->papers.e[p]->box->paperstyle->landscape();
			pagec=(c-start)*papergroup->papers.n+p; //effective scribus page index
			currentpage=pagec;

			 //build transform from laidout space to current page on scribus canvas
			 //current laidout paper origin (lower left corner) must map to the
			 //lower left corner of current scribus page,
			 //which is (CANVAS_MARGIN_X, pageypos+paperheight) in scribus coords
			transform_set(ms,1,0,0,-1,CANVAS_MARGIN_X,pageypos+paperheight);
			transform_invert(m,ms); //m=ms^-1

			psCtmInit();
			psPushCtm(); //so we can always fall back to identity
			transform_invert(mmm,papergroup->papers.e[p]->m()); // papergroup->paper transform
			transform_set(mm,72.,0.,0.,72.,0.,0.); //correction for inches <-> ps points
			transform_mult(mmmm,mmm,mm);
			transform_mult(m,mmmm,ms); //m = mmmm * ms
			psConcat(m); //(scribus page coord) = (laidout paper coord) * psCTM()
			
			DBG cerr <<"spread:"<<c<<"  paper:"<<p<<"  paperwidth:"<<paperwidth<<"  paperheight:"<<paperheight<<endl;
			DBG dumpctm(psCTM());
			DBG flatpoint pt;
			DBG pt=transform_point(psCTM(),0,0);
			DBG cerr <<"ll: "<<pt.x<<","<<pt.y<<endl;
			DBG pt=transform_point(psCTM(),0,paperheight/72);
			DBG cerr <<"ul: "<<pt.x<<","<<pt.y<<endl;
			DBG pt=transform_point(psCTM(),paperwidth/72,paperheight/72);
			DBG cerr <<"ur: "<<pt.x<<","<<pt.y<<endl;
			DBG pt=transform_point(psCTM(),paperwidth/72,0);
			DBG cerr <<"lr: "<<pt.x<<","<<pt.y<<endl;

			 //------------page header
			fprintf(f,"  <PAGE \n"
					  "    Size=\"Custom\" \n");
			fprintf(f,"    PAGEHEIGHT=\"%f\" \n",paperheight);
			fprintf(f,"    PAGEWIDTH=\"%f\" \n",paperwidth);
			fprintf(f,"    PAGEXPOS=\"%f\" \n",CANVAS_MARGIN_X);
			fprintf(f,"    PAGEYPOS=\"%f\" \n",pageypos);
			fprintf(f,"    NUM=\"%d\" \n",(c-start)*papergroup->papers.n+p); //number of the page, starting at 0
			fprintf(f,"    BORDERTOP=\"0\" \n"     //page margins?
					  "    BORDERLEFT=\"0\" \n"
					  "    BORDERBOTTOM=\"0\" \n"
					  "    BORDERRIGHT=\"0\" \n"
					  "    NAM=\"\" \n"            //name of master page, empty when normal
					  "    LEFT=\"0\" \n"          //if is left master page
					  "    Orientation=\"%d\" \n"
					  "    MNAM=\"Normal\" \n"        //name of attached master page
					  "    HorizontalGuides=\"\" \n"
					  "    NumHGuides=\"0\" \n"
					  "    VerticalGuides=\"\" \n"
					  "    NumVGuides=\"0\" \n"
					  "   />\n", plandscape);
					
			if (limbo && limbo->n()) {
				scribusdumpobj(f,NULL,limbo,error_ret,warning);
			}

			if (spread) {
				if (spread->marks) {
					scribusdumpobj(f,NULL,spread->marks,error_ret,warning);
				}

				 // for each page in spread layout..
				for (c2=0; c2<spread->pagestack.n; c2++) {
					psPushCtm();
					pg=spread->pagestack.e[c2]->index;
					if (pg<0 || pg>=doc->pages.n) continue;

					 // for each layer on the page..
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					psConcat(m); //transform to page in spread
					for (l=0; l<doc->pages[pg]->layers.n(); l++) {
						 // for each object in layer
						g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							scribusdumpobj(f,NULL,g->e(c3),error_ret,warning);
						}
					}
					psPopCtm();

				}
			} //if (spread)
			psPopCtm();
			//pageypos+=72*(papergroup->papers.e[p]->box->media.maxy-papergroup->papers.e[p]->box->media.miny)
			//				+ CANVAS_GAP;
			pageypos+=paperheight + CANVAS_GAP;
		} //for each paper
		if (spread) { delete spread; spread=NULL; }
	} //for each spread
		
	
	 // write out footer
	fprintf(f,"  </DOCUMENT>\n"
			  "</SCRIBUSUTF8NEW>");
	
	fclose(f);
	setlocale(LC_ALL,"");
	return 0;
}


//! Internal function to dump out the obj.
/*! Can be Group, ImageData, or GradientData.
 *
 * \todo could have special mode where every non-recognizable object gets
 *   rasterized, and a new dir with all relevant files is created.
 */
static void scribusdumpobj(FILE *f,double *mm,SomeData *obj,char **error_ret,int &warning)
{
	//***possibly set: ANNAME NUMGROUP GROUPS NUMPO POCOOR PTYPE ROT WIDTH HEIGHT XPOS YPOS
	//	gradients: GRTYP GRSTARTX GRENDX GRSTARTY GRENDY
	//	images: LOCALSCX LOCALSCY PFILE

	psPushCtm();
	psConcat(obj->m());

	ImageData *img=NULL;
	GradientData *grad=NULL;
	double localscx=1,localscy=1;
	int ptype=-1; //>0 is translatable to scribus object.
				  //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
	              //-1 is not handled, -2 is laidout gradient, -3 is MysteryData
	Attribute *mysteryatts=NULL;

	if (!strcmp(obj->whattype(),"ImageData") || !strcmp(obj->whattype(),"EpsData")) {
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;
		ptype=PTYPE_Image;

	//} else if (!strcmp(obj->whattype(),"GradientData")) {
	//	grad=dynamic_cast<GradientData *>(obj);
	//	if (!grad) return;
	//	ptype=PTYPE_Laidout_Gradient;
	
	} else if (!strcmp(obj->whattype(),"Group")) {
		 //must propogate transform...
		Group *g;
		g=dynamic_cast<Group *>(obj);
		if (!g) return;

		 // objects have GROUPS list for what groups they belong to, 
		 // we maintain a list of current group nesting, this list will be the GROUPS
		 // element of subsequent objects
		 // global var groupc is a counter for how many distinct groups have been found,
		 // which has been found already

		ongroup++;
		groups.push(ongroup);
		for (int c=0; c<g->n(); c++) 
			scribusdumpobj(f,NULL,g->e(c),error_ret,warning);
		groups.pop();
		psPopCtm();
		return;

	} else if (!strcmp(obj->whattype(),"MysteryData")) {
		MysteryData *mdata=dynamic_cast<MysteryData *>(obj);
		if (!strcmp(mdata->importer,"Scribus")) {
			mysteryatts=mdata->attributes;
			//***need to refigure position and orientation, pocoor, cocoor to scale to current
			ptype=PTYPE_Laidout_MysteryData;
		} //else is someone else's mystery data
	} 

	if (ptype==PTYPE_None) {
		setlocale(LC_ALL,"");
		char *tmp=new char[strlen(_("Warning: Cannot export %s to Scribus.\n"))+strlen(obj->whattype())+1];
		sprintf(tmp,_("Warning: Cannot export %s to Scribus.\n"),obj->whattype());
		appendstr(*error_ret,tmp);
		setlocale(LC_ALL,"C");
		warning++;
		delete[] tmp;
		psPopCtm();
		return;
	}

	 //there is no image shearing in Scribus, so images must map to an unsheared variant,
	 //  and stuck in an appropriate box
	 //Gradients must have one circle totally inside another. Sheared gradients must be converted
	 //  to ellipses.
	 //Object bounding boxes in scribus have no shear, only position, scale, and rotation
	 //
	 //in the sla, XPOS,YPOS is the upper left corner of an object, in scratch space coordinates.
	 //ROT adds additional rotation around that point, clockwise as you see it in Scribus,
	 //which is a left handed space (+x to the right, +y is down).
	 //WIDTH and HEIGHT are bounding box measurements from that corner, and are in scratch space units.
	 //
	//***this is still a bit hacky
	 
	 //figure out the COCOOR and POCOOR for an object
	flatpoint p,p1,p2, vx,vy;
	double *ctm=psCTM(); //(scratch space coords)=(object coords)*(object->m())*ctm
	double rot,x,y,width,height;

	vx=transform_vector(ctm,flatpoint(1,0));
	vy=transform_vector(ctm,flatpoint(0,1));
	rot=atan2(vx.y, vx.x)/M_PI*180; //rotation in degrees
	//p=transform_point(ctm,flatpoint(0,0));
	p=transform_point(ctm,flatpoint(obj->minx,obj->maxy));
	x=p.x;
	y=p.y;

	 //create pocoor outline
	int        numpo=0,      numco=0;
	flatpoint *pocoor=NULL, *cocoor=NULL;
	int createrect=1;
	if (mysteryatts) {
		 //map old pocoor coordinates based on potentially new position of the mystery data
		Attribute *pocooratt=mysteryatts->find("POCOOR");
		//Attribute *pocooratt=mysteryatts->find("COCOOR");
		if (pocooratt) {
			createrect=0;
			//Attribute *tmp=mysteryatts->find("NUMPO");<--get directly from pocoor
			double *coords=NULL;
			DoubleListAttribute(pocooratt->value,&coords,&numpo);
			numpo/=2;
			pocoor=new flatpoint[numpo];
			for (int c=0; c<numpo; c++) {
				pocoor[c]=transform_point(ctm,coords[c*2]/72,coords[c*2+1]/72);
				//pocoor[c]=flatpoint(coords[c*2],coords[c*2+1]);
				DBG cerr <<"pocoor to canvas: "<<pocoor[c].x<<','<<pocoor[c].y<<endl;
			}
			//note that these are raw coordinates read on input, they still have to be scaled
			// to current bounding box, which is done below
		}
	}
	if (createrect) {
		 //no coordinate path otherwise found, so create a rectangle based on the object bounding box
		numpo=16;
		pocoor=new flatpoint[numpo];
		pocoor[14]=pocoor[15]=pocoor[ 0]=pocoor[ 1]=transform_point(ctm,flatpoint(obj->minx,obj->miny));
		pocoor[ 2]=pocoor[ 3]=pocoor[ 4]=pocoor[ 5]=transform_point(ctm,flatpoint(obj->maxx,obj->miny));
		pocoor[ 6]=pocoor[ 7]=pocoor[ 8]=pocoor[ 9]=transform_point(ctm,flatpoint(obj->maxx,obj->maxy));
		pocoor[10]=pocoor[11]=pocoor[12]=pocoor[13]=transform_point(ctm,flatpoint(obj->minx,obj->maxy));
	}

	 //find bounds of pocoor which by now should be in canvas coordinates
	flatpoint min=pocoor[0], max=pocoor[0];
	for (int c=1; c<numpo; c++) {
		if (pocoor[c].x<min.x) min.x=pocoor[c].x;
		else if (pocoor[c].x>max.x) max.x=pocoor[c].x;
		if (pocoor[c].y<min.y) min.y=pocoor[c].y;
		else if (pocoor[c].y>max.y) max.y=pocoor[c].y;
	}

	width =norm(transform_point(ctm,flatpoint(obj->maxx,obj->miny))-transform_point(ctm,flatpoint(obj->minx,obj->miny)));
	height=norm(transform_point(ctm,flatpoint(obj->minx,obj->miny))-transform_point(ctm,flatpoint(obj->minx,obj->maxy)));
	DBG cerr <<"object dimensions: "<<width<<" x "<<height<<endl;

	 //create a basis for the object, which has same scaling as the scratch space, but
	 //possibly different translation and rotation
	double m[6],mmm[6];
	vx=vx/norm(vx);
	vy=vy/norm(vy);
	p=transform_point(ctm,flatpoint(0,0));
	transform_from_basis(mmm,p,vx,vy);
	transform_invert(m,mmm);
	DBG cerr<<"transform back to object:"; dumpctm(m);

	 //pocoor are in canvas coordinates, need to
	 //make pocoor coords relative to the object origin, not the canvas
	for (int c=0; c<numpo; c++) {
		DBG cerr <<"pocoor: "<<pocoor[c].x<<','<<pocoor[c].y;
		pocoor[c]=transform_point(m,pocoor[c]);
		if (fabs(pocoor[c].x)<1e-10) pocoor[c].x=0;
		if (fabs(pocoor[c].y)<1e-10) pocoor[c].y=0;
		DBG cerr <<" -->  "<<pocoor[c].x<<','<<pocoor[c].y<<endl;
	}
	if (ptype==PTYPE_Image) { //image
		localscx=width /(img->maxx-img->minx); //assumes maxx-minx==file width
		localscy=height/(img->maxy-img->miny);
		if (!strcmp(obj->whattype(),"EpsData")) {
			localscx/=5;
			localscy/=5;
		}
	}


	fprintf(f,"  <PAGEOBJECT \n");
	int content=-1;
	if (mysteryatts) {
		char *name,*value;
		for (int c=0; c<mysteryatts->attributes.n; c++) {
			name=mysteryatts->attributes.e[c]->name;
			value=mysteryatts->attributes.e[c]->value;
			if (!strcmp(name,"OwnPage")) continue;
			if (!strcmp(name,"LOCALSCX")) continue;
			if (!strcmp(name,"LOCALSCY")) continue;
			if (!strcmp(name,"ROT")) continue;
			if (!strcmp(name,"XPOS")) continue;
			if (!strcmp(name,"YPOS")) continue;
			if (!strcmp(name,"WIDTH")) continue;
			if (!strcmp(name,"HEIGHT")) continue;
			if (!strcmp(name,"NUMGROUP")) continue;
			if (!strcmp(name,"GROUPS")) continue;
			if (!strcmp(name,"NUMPO")) continue; //***need to remap pocoor and cocoor
			if (!strcmp(name,"NUMCO")) continue;
			if (!strcmp(name,"POCOOR")) continue;
			if (!strcmp(name,"COCOOR")) continue;
			if (!strcmp(name,"content:")) { content=c; continue; }
			fprintf(f,"    %s=\"%s\"\n", name, value?value:"");
		}
	} 

	if (!mysteryatts) {
		fprintf(f,
				  "    ANNOTATION=\"0\" \n"   //1 if is pdf annotation
				  "    BOOKMARK=\"0\" \n"     //1 if obj is pdf bookmark
				  "    PFILE2=\"\" \n"          //(opt) file for pressed image in pdf button
				  "    PFILE3=\"\" \n"          //(opt) file for rollover image in pdf button

				  //"    CLIPEDIT=\"1\" \n"     //1 if shape was editted (opt)
				  "    doOverprint=\"0\" \n"    //not 1.2
				  "    fillRule=\"1\" \n"       //not 1.2
				  "    gHeight=\"0\" \n"        //not 1.2
				  "    gWidth=\"0\" \n"         //not 1.2
				  "    gXpos=\"0\" \n"          //not 1.2
				  "    gYpos=\"0\" \n"          //not 1.2
				  "    isGroupControl=\"0\" \n" //not 1.2
				  "    isInline=\"0\" \n"       //not 1.2
				  "    OnMasterPage=\"\" \n");    //not 1.2
	}
	fprintf(f,    "    OwnPage=\"%d\" \n",currentpage);  //not 1.2, the page on object is on? ****
	if (!mysteryatts) {
		fprintf(f,"    AUTOTEXT=\"0\" \n"     //1 if object is auto text frame
				  "    BACKITEM=\"-1\" \n"    //Number of the previous frame of linked textframe
				  "    NEXTITEM=\"-1\" \n"      //number of next frame for linked text frames

				  "    PLTSHOW=\"0\" \n"        //(opt) 1 if path for text on path should be visible
				  "    RADRECT=\"0\" \n"        //(opt) corner radius of rounded rectangle

				  "    isTableItem=\"0\" \n"    //1 if object belongs to table
				  "    RightLine=\"0\" \n"      //(opt) 1 it table object has right line
				  "    LeftLine=\"0\" \n"       //(opt) 1 it table object has left line
				  "    TopLine=\"0\" \n"         //(opt) 1 it table object has top line
				  "    BottomLine=\"0\" \n"    //1 if table item has bottom line

				  "    endArrowIndex=\"0\" \n"  //not 1.2
				  "    startArrowIndex=\"0\" \n" //not 1.2

				  "    TransBlend=\"0\" \n"      //not 1.2
				  "    TransBlendS=\"0\" \n"     //not 1.2
			//---------text tags:
				  "    COLGAP=\"0\" \n"        //Gap between text columns
				  "    COLUMNS=\"1\" \n"       //Number of columns in text
				  "    EXTRA=\"0\" \n"          //Distance of text from the left edge of the frame
				  //"    BASEOF=\"0\" \n"     //text on a line offset (opt)
				  //"    BEXTRA=\"0\" \n"       //dist of text from bottom of frame (opt)
				  "    TEXTRA=\"0\" \n"          //Distance of text from the top edge of the frame
				  "    TEXTFLOW=\"0\" \n"        //1 for text flows around object
				  "    TEXTFLOW2=\"0\" \n"       //(opt) 1 for text flows around bounding box
				  "    TEXTFLOW3=\"0\" \n"       //(opt) 1 for text flows around contour
				  "    TEXTFLOWMODE=\"0\" \n"    //not 1.2
				  "    textPathFlipped=\"0\" \n" //not 1.2
				  "    textPathType=\"0\" \n"    //not 1.2
				  "    REVERS=\"0\" \n"         //(opt) text is rendered reverse
				  "    REXTRA=\"0\" \n"         //(opt) Distance of text from the right edge of the frame
			//---------eps tags:
				  //"    BBOXH=\"0\" \n"      //height of eps object (opt)
				  //"    BBOXX=\"0\" \n"      //width of eps object (opt)
			//---------path tags, fill/stroke
				  "    NAMEDLST=\"\" \n"        //(opt) name of the custom line style
				  //"    DASHOFF=\"0\" \n"     //(opt) offset for first dash
				  "    DASHS=\"\" \n"          //List of dash values, see the postscript manual for details
				  "    NUMDASH=\"0\" \n"        //number of entries in dash
				  "    PLINEART=\"1\" \n"       //how line is dashed, 1=solid, 2=- - -, 3=..., 4=-.-.-, 5=-..-..-
				  "    PLINEEND=\"0\" \n"       //(opt) linecap 0 flatcap, 16 square, 32 round
				  "    PLINEJOIN=\"0\" \n"      //(opt) line join, 0 miter, 64 bevel, 128 round
				  "    PWIDTH=\"1\" \n"         //line width of object
				  "    SHADE=\"100\" \n"         //shading for fill
				  "    SHADE2=\"100\" \n"        //shading for stroke
				  "    TransValue=\"0\" \n"      //(opt) transparency value for fill
				  "    TransValueS=\"0\" \n"     //(opt) Transparency value for stroke
				  "    PCOLOR2=\"Black\" \n"    //color of stroke
				  "    PCOLOR=\"None\" \n");    //color of fill
			//---------gradient tags:
		if (ptype==PTYPE_Laidout_Gradient) { //is a gradient
			fprintf(f,"    GRTYP=\"%d\" \n",          // 	Type of the gradient fill
					(grad->style&GRADIENT_RADIAL)?7:6);	//  0 = No gradient fill,       1 = Horizontal gradient
												//  2 = Vertical gradient,      3 = Diagonal gradient
												//  4 = Cross diagonal gradient 5 = Radial gradient
												//  6 = Free linear gradient    7 = Free radial gradient
			fprintf(f,"    GRSTARTX=\"***\" \n"       //(grad only)X-Value of the start position of the gradient
					  "    GRENDX=\"***\"   \n"       //(grad only)X-Value of the end position of the gradient
					  "    GRSTARTY=\"***\" \n"       //(grad only)Y-Value of the start position of the gradient
					  "    GRENDY=\"***\"   \n");      //(grad only)Y-Value of the end position of the gradient
		} else { // not a gradient
			fprintf(f,
				  "    GRTYP=\"0\" \n" 
				  "    GRSTARTX=\"0\" \n"       //(grad only)X-Value of the start position of the gradient
				  "    GRENDX=\"0\"   \n"       //(grad only)X-Value of the end position of the gradient
				  "    GRSTARTY=\"0\" \n"       //(grad only)Y-Value of the start position of the gradient
				  "    GRENDY=\"0\"   \n");       //(grad only)Y-Value of the end position of the gradient
		 }
			//-------------image related
		fprintf(f,"    IRENDER=\"1\" \n"        //Rendering Intent for Images 
											//  0=Perceptual 1=Relative Colorimetric 2=Saturation 3=Absolute Colorimetric
				  "    PRFILE=\"\" \n"          //(opt) icc profile for image
				  "    EMBEDDED=\"1\" \n"       //(opt) Set to 1 if embedded ICC-Profiles should be used
				  //"    EPROF=\"\" \n"         //(opt) Embedded ICC-Profile for images
				  "    RATIO=\"1\" \n"          //(opt) 1 if image scaling should respect aspect
				  "    ImageClip=\"\" \n"       //not 1.2 ??????
				  "    ImageRes=\"1\" \n"       //not 1.2 ??????
				  "    SCALETYPE=\"1\" \n"      //(opt) how image can scale,0=free, 1=bound to frame
				  "    PICART=\"1\" \n"         //1 if image should be shown
				  "    LOCALX=\"0\" \n"         //xpos of image in frame
				  "    LOCALY=\"0\" \n");       //ypos of image in frame
	} //if mysteryatts
	fprintf(f,    "    LOCALSCX=\"%g\" \n"      //image scaling in x direction
				  "    LOCALSCY=\"%g\" \n"      //image scaling in y direction
				  "    PFILE=\"%s\" \n",	    //file of image
				localscx,localscy,ptype==PTYPE_Image?img->filename:"");

		//-------------general object tags:
	 // fix ptype to be more accurate
	if (!mysteryatts) {
		if (ptype==PTYPE_Laidout_Gradient) ptype=PTYPE_Polyline;
		fprintf(f,"    PTYPE=\"%d\" \n",ptype); //object type, 2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
		fprintf(f,"    ANNAME=\"\" \n"          //field name, also object name
				  "    FLIPPEDH=\"0\" \n"       //Set to an uneven number if object is flipped horizontal
				  "    FLIPPEDV=\"0\" \n"       //Set to an uneven number if object is flipped vertical
				  "    PRINTABLE=\"1\" \n"      //1 for object should be printed
				  "    LAYER=\"0\" \n"          //layer number object belongs to
				  "    LOCK=\"0\" \n"           //(opt) 1 if object locked
				  "    LOCKR=\"0\" \n"          //(opt) 1 if object protected against resizing
				  "    FRTYPE=\"3\" \n");       //shape of obj: 0=rect, 1=ellipse, 2=rounded rect, 3=free
	} //else all those were output above already

	 //following tags are redefined even for mystery data
	fprintf(f,"    NUMPO=\"%d\" \n",numpo); //num coords in POCOOR==stroke
	fprintf(f,"    POCOOR=\"");
	for (int c=0; c<numpo; c++) fprintf(f,"%g %g ",pocoor[c].x,pocoor[c].y);
	fprintf(f,"\" \n"
			  "    NUMCO=\"%d\" \n",(cocoor?numco:numpo)); //num coords in COCOOR==contour line==text wrap outline (opt) (vv opt
	fprintf(f,"    COCOOR=\"");
	if (cocoor) for (int c=0; c<numco; c++) fprintf(f,"%g %g ",cocoor[c].x,cocoor[c].y);
	else for (int c=0; c<numpo; c++) fprintf(f,"%g %g ",pocoor[c].x,pocoor[c].y);

	 //groups
	fprintf(f,"\" \n"
			  "    NUMGROUP=\"%d\" \n",groups.n);       //number of entries in GROUPS
	fprintf(f,"    GROUPS=\"");             //List of group identifiers
	for (int c=0; c<groups.n; c++) fprintf(f,"%d ",groups.e[c]);

	 //object metrics
	fprintf(f,"\" \n"			
				  "    ROT=\"%g\" \n"         //rotation of object
				  "    XPOS=\"%g\" \n"        //x of object
				  "    YPOS=\"%g\" \n"        //y of object
				  "    WIDTH=\"%g\" \n"       //width of object
				  "    HEIGHT=\"%g\" \n",     //height of object
								 rot,x,(mysteryatts?y-height:y),width,height);

	 //close the tag
	fprintf(f,">\n"); //close PAGEOBJECT opening tag
	 //output PAGEOBJECT elements
	if (mysteryatts && content>=0) {
		AttributeToXMLFile(f,mysteryatts->attributes.e[content],6);
	}
	fprintf(f,"  </PAGEOBJECT>\n");  //end of PAGEOBJECT


	delete[] pocoor;
	psPopCtm();
}





//------------------------------------ ScribusImportFilter ----------------------------------
/*! \class ScribusImportFilter
 * \brief Filter to, amazingly enough, import svg files.
 *
 * \todo perhaps when dumping in to an object, just bring in all objects as they are arranged
 *   in the scratch space... or if no doc or obj, option to import that way, but with
 *   a custom papergroup where the pages are... perhaps need a freestyle imposition type
 *   that is more flexible with differently sized pages.
 */


const char *ScribusImportFilter::VersionName()
{
	return _("Scribus");
}

/*! \todo can do more work here to get the actual version of the file...
 */
const char *ScribusImportFilter::FileType(const char *first100bytes)
{
	if (!strstr(first100bytes,"<SCRIBUSUTF8NEW")) return NULL;
	return "1.3.3.*";

	//***ANZPAGES is num of pages
}

//! Import Scribus document.
/*! If in->doc==NULL and in->toobj==NULL, then create a new document.
 *
 * If saving mystery data, it will push onto project (or doc) iohints:
 * <pre>
 * Scribus  (VersionName())
 *   scribusVersion  (whatever the Scribus file version was)
 *   originalFile    originalfile.sla
 *   slahead         #<-- has all the attributes of DOCUMENT, the element of SCRIBUSUTF8NEW
 *   docContent      #<-- these were elements of SCRIBUSUTF8NEW.DOCUMENT that were not
 *                   #    PAGE, or PAGEOBJECT
 *   scribusPageHint #store original page information just in case
 * </pre>
 *
 * \todo COLOR, master pages
 */
int ScribusImportFilter::In(const char *file, Laxkit::anObject *context, char **error_ret)
{
	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) return 1;

	Document *doc=in->doc;

	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
	if (!att) return 2;
	
	int c;
	Attribute *scribusdoc=att->find("SCRIBUSUTF8NEW"),
			  *version;
	if (!scribusdoc) { delete att; return 3; }
	version=scribusdoc->find("Version");
	scribusdoc=scribusdoc->find("content:");
	if (!scribusdoc) { delete att; return 4; }
	scribusdoc=scribusdoc->find("DOCUMENT");
	if (!scribusdoc) { delete att; return 4; }
	
	 //create repository for hints if necessary
	Attribute *scribushints=NULL;
	if (in->keepmystery) {
		scribushints=new Attribute("Scribus", VersionName());
		scribushints->push("scribusVersion",version->value);
		scribushints->push("originalFile",file);
	}


	 //figure out the paper size, orientation
	PaperStyle *paper=NULL;
	int landscape=0;

	 //****setup paper based on scribus pagesize only if creating a new document....
	Attribute *a=scribusdoc->find("PAGESIZE");
	if (a && a->value) {
		for (c=0; c<laidout->papersizes.n; c++)
			if (!strcasecmp(laidout->papersizes.e[c]->name,a->value)) {
				paper=laidout->papersizes.e[c];
				break;
			}
	}
	if (!paper) paper=laidout->papersizes.e[0];

	 //figure out orientation
	a=scribusdoc->find("ORIENTATION");
	if (a) landscape=BooleanAttribute(a->value);
	else landscape=0;
	
	 //pagenum to start dumping onto
	int docpagenum=in->topage; //the page in doc to start dumping into
	int pagenum,
		curdocpage; //the current page, used in loop below
	if (docpagenum<0) docpagenum=0;

	 //find the number of pages to expect in the scribus document
	a=scribusdoc->find("ANZPAGES");
	int numpages=-1;
	if (a) IntAttribute(a->value,&numpages);//***should error check here!
	int start,end;
	if (in->instart<0) start=0; else start=in->instart;
	if (in->inend<0 || in->inend>=numpages) end=numpages-1; 
		else end=in->inend;

	DoubleBBox pagebounds[end-start+1];
	if (doc && docpagenum+(end-start)>=doc->pages.n) 
		doc->NewPages(-1,(docpagenum+(end-start+1))-doc->pages.n);

	if (scribushints) {
		Attribute *slahead=new Attribute("slahead",NULL);
		for (int c=0; c<scribusdoc->attributes.n; c++) {
			 //store all document xml attributes in scribus hints, 
			 //but not the whole content, 
			if (!strcmp(scribusdoc->attributes.e[c]->name,"content:")) continue;
			slahead->push(scribusdoc->attributes.e[c]->duplicate(),-1);
		}
		scribushints->push(slahead,-1);
	}

	 //get to the contents of the scribus document
	scribusdoc=scribusdoc->find("content:");
	if (!scribusdoc) { delete att; return 5; }


	 //now scribusdoc's subattributes should be many things. We are primarily interested in
	 //"PAGE", "PAGEOBJECT", and "COLOR" fields, and maybe "PageSets"
	 //all the others we can safely ignore, and let pass into iohints

	 //create the document if necessary
	if (!doc && !in->toobj) {
		Imposition *imp=new Singles; //*** this is not necessarily so! uses PageSets??
		paper->flags=((paper->flags)&~1)|(landscape?1:0);
		imp->SetPaperSize(paper);
		DocumentStyle *docstyle=new DocumentStyle(imp);
		doc=new Document(docstyle,Untitled_name());
	}

	Group *group=in->toobj;
	Attribute *page,*object;
	char scratch[50];
	MysteryData *mdata=NULL;
	char *name, *value;


	 //changedir to directory of file to correctly parse relative links
	 //***warning, not thread safe!!
	char *dir=lax_dirname(file,0);
	if (dir) {
		chdir(dir);
		delete[] dir; dir=NULL;
	}

	 //1st pass, scan for PAGE attributes
	for (c=0; c<scribusdoc->attributes.n; c++) {
		name=scribusdoc->attributes.e[c]->name;
		value=scribusdoc->attributes.e[c]->value;
		if (!strcmp(name,"PAGE")) {
			page=scribusdoc->attributes.e[c];
			a=page->find("NUM");
			IntAttribute(a->value,&pagenum); //*** could use some error checking here so corrupt files dont crash laidout!!
			if (pagenum<start || pagenum>end) continue; //only store pages that'll really be imported
			DoubleAttribute(page->find("PAGEXPOS")->value,&pagebounds[pagenum].minx);
			DoubleAttribute(page->find("PAGEYPOS")->value,&pagebounds[pagenum].miny);
			DoubleAttribute(page->find("PAGEWIDTH")->value,&pagebounds[pagenum].maxx);
			DoubleAttribute(page->find("PAGEHEIGHT")->value,&pagebounds[pagenum].maxy);
			pagebounds[pagenum].maxx+=pagebounds[pagenum].minx;
			pagebounds[pagenum].maxy+=pagebounds[pagenum].miny;

			 //remaining stuff is iohint 
			if (scribushints) {
				a=page->duplicate();
				sprintf(scratch,"%d",pagenum);
				makestr(a->name,"scribusPageHint");
				makestr(a->value,scratch);
				scribushints->push(a,-1);
			}
			continue;
		}
	}
	 //now scan for everything other than PAGE
	for (c=0; c<scribusdoc->attributes.n; c++) {
		name=scribusdoc->attributes.e[c]->name;
		value=scribusdoc->attributes.e[c]->value;

		if (!strcmp(name,"PAGE")) {
			continue;

		} else if (!strcmp(name,"PAGEOBJECT")) {
			object=scribusdoc->attributes.e[c];

			 //figure out what page it is supposed to be on..
			 //***need some way to compensate for bleeding!!!
			Attribute *tmp=object->find("OwnPage");
			if (tmp) IntAttribute(tmp->value,&pagenum);
			if (pagenum<start || pagenum>end) continue; //***watch out for object bleeding!!
			if (doc) {
				 //update group to point to the document page's group
				curdocpage=docpagenum+(pagenum-start);
				group=dynamic_cast<Group *>(doc->pages.e[curdocpage]->layers.e(0)); //pick layer 0 of the page
			}
			double x,y,rot,w,h;
			double matrix[6];
			DoubleAttribute(object->find("XPOS")->value  ,&x);//***this could be att->doubleValue("XPOS",&x) for safety
			DoubleAttribute(object->find("YPOS")->value  ,&y);
			DoubleAttribute(object->find("ROT")->value   ,&rot); //rotation is in degrees
			DoubleAttribute(object->find("WIDTH")->value ,&w);
			DoubleAttribute(object->find("HEIGHT")->value,&h);
			x-=pagebounds[pagenum].minx;
			y-=pagebounds[pagenum].miny;
			y=(pagebounds[pagenum].maxy-pagebounds[pagenum].miny)-y; //pageheight-y, needed to flip y around
			int ptype=atoi(object->find("PTYPE")->value); //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
			rot*=-M_PI/180;

			matrix[0]=cos(rot);
			matrix[1]=sin(rot);
			matrix[2]=sin(rot);
			matrix[3]=-cos(rot);
			matrix[4]=x/72;
			matrix[5]=y/72;

			if (ptype==2) {
				 //we found an image
				Attribute *pfile=object->find("PFILE");
				ImageData *image=static_cast<ImageData *>(newObject("ImageData"));
				char *fullfile=full_path_for_file(pfile->value,NULL);
				image->LoadImage(fullfile); //this will set maxx, maxy to dimensions of the image
				delete[] fullfile;
				transform_copy(image->m(),matrix);
				image->m()[0]*=w/image->maxx/72.;
				image->m()[1]*=w/image->maxx/72.;
				image->m()[2]*=h/image->maxy/72.;
				image->m()[3]*=h/image->maxy/72.;
				image->Flip(0);
				group->push(image,-1);

			} else if (scribushints) { 
				 //undealt with object, push as MysteryData if in->keepmystery

				mdata=new MysteryData("Scribus"); //note, this is untranslated "Scribus"
				if (ptype==4) makestr(mdata->name,"Text Frame");
				else if (ptype==5) makestr(mdata->name,"Line");
				else if (ptype==6) makestr(mdata->name,"Polygon");
				else if (ptype==7) makestr(mdata->name,"Polyline");
				else if (ptype==8) makestr(mdata->name,"Text on path");
				transform_copy(mdata->m(),matrix);
				mdata->maxx=w/72;
				mdata->maxy=h/72;
				mdata->attributes=object->duplicate();
				group->push(mdata,-1);
			}

		//} else if (!strcmp(scribusdoc->attributes.e[c]->name,"COLOR")) {
			 //this will be something like:
			 // NAME "White"
			 // CMYK "#00000000"  (or RGB "#000000")
			 // Spot "0"
			 // Register "0"
			//***** finish me!

		} else if (scribushints) {
			 //push any other blocks into scribushints.. we can usually safely ignore them
			Attribute *more=new Attribute("docContent",NULL);
			more->push(scribusdoc->attributes.e[c]->duplicate(),-1);
			scribushints->push(more,-1);
			continue;
		}
	}
	

	 //install global hints if they exist
	if (scribushints) {
		 //remove the old iohint if it is there
		Attribute *iohints=(doc?&doc->iohints:&laidout->project->iohints);
		Attribute *oldscribus=iohints->find(VersionName());
		if (oldscribus) {
			iohints->attributes.remove(iohints->attributes.findindex(oldscribus));
		}
		iohints->push(scribushints,-1);
		//remember, do not delete scribushints here! they become part of the doc/project
	}

	 //if doc is new, push into the project
	if (doc && doc!=in->doc) {
		laidout->project->Push(doc);
		laidout->app->addwindow(newHeadWindow(doc));
	}
	
	delete att;
	return 0;
}




