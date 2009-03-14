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

#include "../language.h"
#include "scribus.h"
#include "../laidout.h"
#include "../printing/psout.h"
#include "../utils.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"
#include "../dataobjects/mysterydata.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;




static void scribusdumpobj(FILE *f,double *mm,SomeData *obj,char **error_ret,int &warning);

//1.5 inches and 1/4 inch
#define CANVAS_MARGIN_X 108.
#define CANVAS_MARGIN_Y 18.


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
 * \brief Export as 1.3.3.8 or thereabouts.
 *
 * <pre>
 *  current 1.3.3.* file format is supposedly at:
 *  http://docs.scribus.net/index.php?lang=en&sm=scribusfileformat&page=scribusfileformat
 *  but it only has fully written up 1.2 format
 * </pre>
 */

ScribusExportFilter::ScribusExportFilter()
{
	flags=FILTER_MULTIPAGE;
}

//! "Scribus 1.3.3.8".
const char *ScribusExportFilter::VersionName()
{
	return _("Scribus 1.3.3.8");
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
	fprintf(f,"<SCRIBUSUTF8NEW Version=\"1.3.3.8\">\n");
	
	
	 //figure out paper size and orientation
	int landscape=0,plandscape;
	double paperwidth,paperheight;
	 // note this is orientation for only the first paper in papergroup.
	 // If there are more than one papers, this may not work as expected...
	 // The ps Orientation comment determines how onscreen viewers will show 
	 // pages. This can be overridden by the %%PageOrientation: comment
	landscape=(papergroup->papers.e[0]->box->paperstyle->flags&1)?1:0;
	paperwidth= papergroup->papers.e[0]->box->paperstyle->width;
	paperheight=papergroup->papers.e[0]->box->paperstyle->height;
	
	 //------------ write out document attributes
	spread=doc->docstyle->imposition->Layout(layout,start); //grab for page size
	fprintf(f,"  <DOCUMENT \n"
			  "    ABSTPALTEN=\"11\" \n"  //Distance between Columns in automatic Textframes
			  "    ANZPAGES=\"%d\" \n",end-start+1); //number of pages
	fprintf(f,"    AUTHOR=\"\" \n"
			  "    AUTOSPALTEN=\"1\" \n" //Number of Colums in automatic Textframes
			  //"    BOOK***** " *** has facing pages if present: doublesidedsingles
			  "    BORDERBOTTOM=\"0\" \n"	//margins!
			  "    BORDERLEFT=\"0\" \n"	
			  "    BORDERRIGHT=\"0\" \n"	
			  "    BORDERTOP=\"0\" \n"	
			  "    COMMENTS=\"\" \n"
			  "    DFONT=\"Times-Roman\" \n" //default font
			  "    DSIZE=\"12\" \n"
			  //"    FIRSTLEFT \n"  //*** doublesidedsingles->isleft
			  "    FIRSTPAGENUM=\"%d\" \n", start); //***check this is right
	fprintf(f,"    KEYWORDS=\"\" \n"
			  "    ORIENTATION=\"%d\" \n",landscape);
	fprintf(f,"    PAGEHEIGHT=\"%f\" \n",paperheight);
	fprintf(f,"    PAGEWIDTH=\"%f\" \n",paperwidth);//***
	fprintf(f,"    TITLE=\"\" \n"
			  "    VHOCH=\"33\" \n"      //Percentage for Superscript
			  "    VHOCHSC=\"100\" \n"  // Percentage for scaling of the Glyphs in Superscript
			  "    VKAPIT=\"75\" \n"    //Percentage for scaling of the Glyphs in Small Caps
			  "    VTIEF=\"33\" \n"       //Percentage for Subscript
			  "    VTIEFSC=\"100\" \n");   //Percentage for scaling of the Glyphs in Subscript
	fprintf(f,"    ScratchTop=\"%f\"\n"
			  "    ScratchBottom=\"%f\"\n"
			  "    ScratchRight=\"%f\"\n"
			  "    ScratchLeft=\"%f\"\n"
			  "   >\n",
			  	CANVAS_MARGIN_Y, CANVAS_MARGIN_Y, CANVAS_MARGIN_X, CANVAS_MARGIN_X);
	delete spread;
	
	
				
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
	//This sets up so there is one column of pages, with 40 units between them.
	fprintf(f,"    <PageSets>\n"
			  "      <Set Columns=\"1\" GapBelow=\"40\" Rows=\"1\" FirstPage=\"0\" GapHorizontal=\"0\"\n"
			  "         Name=\"Single Page\" GapVertical=\"0\" />\n"
			  "    </PageSets>\n");

	//************MASTERPAGE not separate entity in 1.2
	
			
	//------------PAGE and PAGEOBJECTS

	 //
	 // holy cow, scribus has EVERY object by reference!!!!
	 // In Scribus, pages sit on a canvas, and the page has coords PAGEWIDTH,HEIGHT,XPOS,YPOS.
	 //
	 // Page objects have coordinates in the canvas space, not the paper space.
	 // Their xpos, ypos, rot, width, and height (notably NOT shear)
	 // refer to its bounding box. The pocoor and cocoor coordinates are relative to that
	 // transformed bounding box (????).
	 //
	 // Scribus Groups are more like sets. Objects all lie directly on the page, and groups
	 // are simply a loose tag the objects have. Groups do not apply any additional transformation.
	 //
	groups.flush();
	ongroup=0;
	int p;
	double m[6],mm[6],mmm[6],mmmm[6],ms[6];
	transform_set(m,1,0,0,1,0,0);

	psFlushCtms();
	psCtmInit();
	double pageypos=CANVAS_MARGIN_Y;
	int pagec;
	for (c=start; c<=end; c++) {
		if (doc) spread=doc->docstyle->imposition->Layout(layout,c);
		for (p=0; p<papergroup->papers.n; p++) {
			psCtmInit();
			paperwidth= 72*papergroup->papers.e[p]->box->paperstyle->w(); //scribus wants visual w/h
			paperheight=72*papergroup->papers.e[p]->box->paperstyle->h();
			plandscape=(papergroup->papers.e[p]->box->paperstyle->flags&1)?1:0;
			pagec=(c-start)*papergroup->papers.n+p;
			currentpage=pagec;

			//if (landscape) {
			//	psConcat(0.,1.,-1.,0., paperwidth,0.);
			//}

			transform_set(ms,1,0,0,-1,CANVAS_MARGIN_X,pageypos+paperheight); //transform to scribus canvas
			transform_invert(m,ms);

			psPushCtm(); //starts at identity
			transform_invert(mmm,papergroup->papers.e[p]->m()); // papergroup->paper transform
			transform_set(mm,72.,0.,0.,72.,0.,0.);
			transform_mult(mmmm,mmm,mm);
			transform_mult(m,mmmm,ms);
			psConcat(m);
			
			DBG cerr <<"spread:"<<c<<"  paper:"<<p<<"  :";
			dumpctm(psCTM());

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
					if (pg>=doc->pages.n) continue;

					 // for each layer on the page..
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					psConcat(m);
					for (l=0; l<doc->pages[pg]->layers.n(); l++) {
						 // for each object in layer
						g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							transform_copy(m,spread->pagestack.e[c2]->outline->m());
							scribusdumpobj(f,m,g->e(c3),error_ret,warning);
						}
					}
					psPopCtm();

				}
			}
			psPopCtm();
			pageypos+=72*(papergroup->papers.e[p]->box->media.maxy-papergroup->papers.e[p]->box->media.miny)+40;
		}
		if (spread) { delete spread; spread=NULL; }
	}
		
	
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
	int numpo=0;
	flatpoint *pocoor;
	double localscx=1,localscy=1;
	int ptype=-1; //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path

	if (!strcmp(obj->whattype(),"ImageData") || !strcmp(obj->whattype(),"EpsData")) {
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;
		ptype=2;
	//} else if (!strcmp(obj->whattype(),"GradientData")) {
	//	grad=dynamic_cast<GradientData *>(obj);
	//	if (!grad) return;
	//	ptype=-2;
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
	} else {
		char *tmp=new char[strlen(_("Warning: Cannot export %s to Scribus.\n"))+strlen(obj->whattype())+1];
		setlocale(LC_ALL,"");
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
	//***this is still a bit hacky
	numpo=16;
	pocoor=new flatpoint[numpo];
	flatpoint p,p1,p2, vx,vy;
	double *ctm=psCTM();
	double rot,x,y,width,height;

	vx=transform_vector(ctm,flatpoint(1,0));
	vy=transform_vector(ctm,flatpoint(0,1));
	rot=atan2(vx.y, vx.x)/M_PI*180;
	//p=transform_point(ctm,flatpoint(0,0));
	p=transform_point(ctm,flatpoint(obj->minx,obj->maxy));
	x=p.x;
	y=p.y;

	pocoor[14]=pocoor[15]=pocoor[ 0]=pocoor[ 1]=transform_point(ctm,flatpoint(obj->minx,obj->miny));
	pocoor[ 2]=pocoor[ 3]=pocoor[ 4]=pocoor[ 5]=transform_point(ctm,flatpoint(obj->maxx,obj->miny));
	pocoor[ 6]=pocoor[ 7]=pocoor[ 8]=pocoor[ 9]=transform_point(ctm,flatpoint(obj->maxx,obj->maxy));
	pocoor[10]=pocoor[11]=pocoor[12]=pocoor[13]=transform_point(ctm,flatpoint(obj->minx,obj->maxy));

	width=norm(pocoor[2]-pocoor[1]);
	height=norm(pocoor[6]-pocoor[2]);

	double m[6],mmm[6];
	vx=vx/norm(vx);
	vy=vy/norm(vy);
	p=transform_point(ctm,flatpoint(0,0));
	transform_from_basis(mmm,p,vx,vy);
	transform_invert(m,mmm);

	 //make pocoor coords relative to the object origin, not the canvas
	for (int c=0; c<16; c++) {
		pocoor[c]=transform_point(m,pocoor[c]);
	}
	if (ptype==2) { //image
		localscx=width /(img->maxx-img->minx);
		localscy=height/(img->maxy-img->miny);
		if (!strcmp(obj->whattype(),"EpsData")) {
			localscx/=5;
			localscy/=5;
		}
	}


	fprintf(f,"  <PAGEOBJECT \n"
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
			  "    OnMasterPage=\"\" \n"    //not 1.2
			  "    OwnPage=\"%d\" \n",currentpage);  //not 1.2, the page on object is on? ****

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
	if (ptype==-2) { //is a gradient
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
	fprintf(f,"    LOCALSCX=\"%f\" \n"      //image scaling in x direction
			  "    LOCALSCY=\"%f\" \n"      //image scaling in y direction
			  "    PFILE=\"%s\" \n",	    //file of image
			localscx,localscy,ptype==2?img->filename:"");

		//-------------general object tags:
	 // fix ptype to be more accurate
	if (ptype==-2) ptype=7;
	fprintf(f,"    PTYPE=\"%d\" \n",ptype);  //object type, 2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
	fprintf(f,"    ANNAME=\"\" \n"        //field name, also object name
			  "    FLIPPEDH=\"0\" \n"       //Set to an uneven number if object is flipped horizontal
			  "    FLIPPEDV=\"0\" \n"       //Set to an uneven number if object is flipped vertical
			  "    PRINTABLE=\"1\" \n"      //1 for object should be printed
			  "    NUMPO=\"%d\" \n",numpo); //num coords in POCOOR==stroke
	fprintf(f,"    POCOOR=\"");
	for (int c=0; c<numpo; c++) fprintf(f,"%f %f ",pocoor[c].x,pocoor[c].y);
	fprintf(f,"\" \n"
			  "    NUMCO=\"%d\" \n",numpo); //num coords in COCOOR==contour line==text wrap outline (opt) (vv opt
	fprintf(f,"    COCOOR=\"");
	for (int c=0; c<numpo; c++) fprintf(f,"%f %f ",pocoor[c].x,pocoor[c].y);
	fprintf(f,"\" \n"
			  "    NUMGROUP=\"%d\" \n",groups.n);       //number of entries in GROUPS
	fprintf(f,"    GROUPS=\"");             //List of group identifiers
	for (int c=0; c<groups.n; c++) 
		fprintf(f,"%d ",groups.e[c]);
	fprintf(f,"\" \n"			
			  "    LAYER=\"0\" \n"          //layer number object belongs to
			  "    LOCK=\"0\" \n"           //(opt) 1 if object locked
			  "    LOCKR=\"0\" \n"          //(opt) 1 if object protected against resizing
			  "    FRTYPE=\"3\" \n"         //shape of obj: 0=rect, 1=ellipse, 2=rounded rect, 3=free
			  "    ROT=\"%f\" \n"           //rotation of object
			  "    XPOS=\"%f\" \n"        //x of object
			  "    YPOS=\"%f\" \n"       //y of object
			  "    WIDTH=\"%f\" \n"       //width of object
			  "    HEIGHT=\"%f\" \n"      //height of object
			  "   />\n", rot,x,y,width,height);

	delete[] pocoor;
	psPopCtm();
}





//------------------------------------ ScribusImportFilter ----------------------------------
/*! \class ScribusImportFilter
 * \brief Filter to, amazingly enough, import svg files.
 */


const char *ScribusImportFilter::VersionName()
{
	return _("Scribus");
}

const char *ScribusImportFilter::FileType(const char *first100bytes)
{
	if (!strstr(first100bytes,"<SCRIBUSUTF8NEW")) return NULL;
	return "1.3.3.*";

	//***ANZPAGES is num of pages
}

//! Import Scribus document.
/*! If in->doc==NULL and in->toobj==NULL, then create a new document.
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
	if (in->keepmystery) scribushints=new Attribute(VersionName(),file);


	 //figure out the paper size, orientation
	PaperStyle *paper=NULL;
	int landscape;

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

	 //find the number of pages
	a=scribusdoc->find("ANZPAGES");
	int numpages=-1;
	if (a) IntAttribute(a->value,&numpages);//***should error check here!
	int start,end;
	if (in->instart<0) start=0; else start=in->instart;
	if (in->inend<0 || in->inend>=numpages) end=numpages-1; 
		else end=in->inend;

	DoubleBBox pagebounds[end-start+1];
	if (doc && docpagenum+(end-start)>=doc->pages.n) 
		doc->NewPages(-1,doc->pages.n-(docpagenum+(end-start))+1);

	if (scribushints) {
		Attribute *slahead=new Attribute("slahead",NULL);
		for (int c=0; c<scribusdoc->attributes.n; c++) {
			 //store all document xml attributes in scribus hints, 
			 //but not the whole content
			if (!strcmp(scribusdoc->attributes.e[c]->name,"content:")) continue;
			slahead->push(scribusdoc->attributes.e[c]->duplicate(),-1);
		}
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

	for (c=0; c<scribusdoc->attributes.n; c++) {
		if (!strcmp(scribusdoc->attributes.e[c]->name,"COLOR")) {
			 //this will be something like:
			 // NAME "White"
			 // CMYK "#00000000"  (or RGB "#000000")
			 // Spot "0"
			 // Register "0"
			//***** finish me!
		} else if (!strcmp(scribusdoc->attributes.e[c]->name,"PAGE")) {
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
			a=page->duplicate();
			sprintf(scratch,"%d",pagenum);
			makestr(a->value,scratch);
			if (scribushints) scribushints->push(a,-1);

		} else if (!strcmp(scribusdoc->attributes.e[c]->name,"PAGEOBJECT")) {
			object=scribusdoc->attributes.e[c];
			IntAttribute(object->find("OwnPage")->value,&pagenum);
			if (pagenum<start || pagenum>end) continue; //***watch out for object bleeding!!
			if (doc) {
				 //update group to point to the document page's group
				curdocpage=docpagenum+(pagenum-start);
				group=dynamic_cast<Group *>(doc->pages.e[curdocpage]->layers.e(0)); //pick layer 0 of the page
			}
			double x,y,rot,w,h;
			DoubleAttribute(object->find("XPOS")->value  ,&x);//***this could be att->doubleValue("XPOS",&x) for safety
			DoubleAttribute(object->find("YPOS")->value  ,&y);
			DoubleAttribute(object->find("ROT")->value   ,&rot);
			DoubleAttribute(object->find("WIDTH")->value ,&w);
			DoubleAttribute(object->find("HEIGHT")->value,&h);
			x-=pagebounds[pagenum].minx;
			y-=pagebounds[pagenum].miny;
			int ptype=atoi(object->find("PTYPE")->value); //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path

			mdata=new MysteryData(VersionName());
			if (ptype==2) makestr(mdata->name,"Image Frame");
			else if (ptype==4) makestr(mdata->name,"Text Frame");
			else if (ptype==5) makestr(mdata->name,"Line");
			else if (ptype==6) makestr(mdata->name,"Polygon");
			else if (ptype==7) makestr(mdata->name,"Polyline");
			else if (ptype==8) makestr(mdata->name,"Text on path");
			mdata->m(0,cos(rot));
			mdata->m(1,sin(rot));
			mdata->m(2,sin(rot));
			mdata->m(3,-cos(rot));
			mdata->m(4,x/72);
			mdata->m(5,y/72);
			mdata->maxx=w/72;
			mdata->maxy=h/72;
			group->push(mdata,-1);

			cout <<"**** finish implementing scribus in!!"<<endl;

		} else if (scribushints) {
			 //push any other blocks into scribushints.. we can usually safely ignore them
			Attribute *more=new Attribute("docContent",NULL);
			more->push(scribusdoc->attributes.e[c]->duplicate(),-1);
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




