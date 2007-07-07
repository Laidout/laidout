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
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "scribus.h"
#include "../laidout.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;




static void scribusdumpobj(FILE *f,double *mm,SomeData *obj,char **error_ret,int &warning);

//--------------------------------- install Scribus filter

//! Tells the Laidout application that there's a new filter in town.
void installScribusFilter()
{
	ScribusExportFilter *scribusout=new ScribusExportFilter;
	laidout->exportfilters.push(scribusout);
	
	//ScribusImportFilter *scribusin=new ScribusImportFilter;
	//laidout->importfilters(scribusin);
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
static int groupc=1,ongroup;

int countGroups(Group *g)
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
/*! 
 */
int ScribusExportFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->filename;
	
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paperstyle) return 1;
	
	FILE *f=NULL;
	char *file;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			*error_ret=newstr(_("Cannot save without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".sla");
	} else file=newstr(filename);
	
	f=fopen(file,"w");
	delete[] file; file=NULL;

	if (!f) {
		DBG cerr <<"**** cannot save, doc->saveas cannot be opened for writing."<<endl;
		*error_ret=newstr(_("Error opening file for writing."));
		return 3;
	}

	int warning=0;
	Spread *spread;
	Group *g;
	double m[6];
	//int c;
	int c,c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);

	if (start<0) start=0;
	else if (start>doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	if (end<start) end=start;
	else if (end>doc->docstyle->imposition->NumSpreads(layout))
		end=doc->docstyle->imposition->NumSpreads(layout)-1;
	
	 //find out how many groups there are for DOCUMENT->GROUPC
	 //**** is this necessary? marked as optional in 1.2 spec
	 //*** also grab all color references
	groupc=0;
	for (c=start; c<=end; c++) {
		spread=doc->docstyle->imposition->Layout(layout,c);
		 // for each page in spread layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg>=doc->pages.n) continue;
			 // for each layer on the page..
			//groupc++;
			g=&doc->pages.e[pg]->layers;
			groupc+=countGroups(g);
		}
		delete spread; spread=NULL;
	}
	
	 // write out header
	fprintf(f,"<SCRIBUSUTF8NEW Version=\"1.3.3.8\">\n");
	
	
	 //figure out paper orientation
	int landscape=0;
	if (layout==PAPERLAYOUT) {
		landscape=((doc->docstyle->imposition->paperstyle->flags&1)?1:0);
	} 
	
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
	fprintf(f,"    PAGEHEIGHT=\"%f\" \n",spread->path->maxy-spread->path->miny);
	fprintf(f,"    PAGEWIDTH=\"%f\" \n",spread->path->maxx-spread->path->minx);//***
	fprintf(f,"    TITLE=\"\" \n"
			  "    VHOCH=\"33\" \n"      //Percentage for Superscript
			  "    VHOCHSC=\"100\" \n"  // Percentage for scaling of the Glyphs in Superscript
			  "    VKAPIT=\"75\" \n"    //Percentage for scaling of the Glyphs in Small Caps
			  "    VTIEF=\"33\" \n"       //Percentage for Subscript
			  "    VTIEFSC=\"100\" \n"   //Percentage for scaling of the Glyphs in Subscript
			  "   >\n");
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
	//************PageSets  not 1.2
	//************MASTERPAGE not separate entity in 1.2
	
			
	//------------PAGE/PAGEOBJECTS
	

	 // Write out paper spreads....
	 //*****
	 //***** holy cow, scribus has EVERY object by reference!!!! still trying to
	 //***** figure out how objects are mapped onto groups, layers, and pages..
	 //*****
	groups.flush();
	ongroup=0;
	transform_set(m,1,0,0,1,0,0);
	for (c=start; c<=end; c++) {
		currentpage=c;
		spread=doc->docstyle->imposition->Layout(layout,c);
		 //------------page header
		fprintf(f,"  <PAGE \n"
				  "    Size=\"Custom\" \n");
		fprintf(f,"    PAGEHEIGHT=\"%f\" \n",72*spread->path->maxy-spread->path->miny);
		fprintf(f,"    PAGEWIDTH=\"%f\" \n",72*spread->path->maxx-spread->path->minx);//***
		fprintf(f,"    NUM=\"%d\" \n",c-start); //***check this is right  //number of the page, starting at 0
		fprintf(f,"    BORDERTOP=\"0\" \n"     //page margins?
				  "    BORDERLEFT=\"0\" \n"
				  "    BORDERBOTTOM=\"0\" \n"
				  "    BORDERRIGHT=\"0\" \n"
				  "    NAM=\"\" \n"            //name of master page, empty when normal
				  "    LEFT=\"0\" \n"          //if is left master page
				  "    Orientation=\"0\" \n"
				  "    PAGEXPOS=\"108\" \n"
				  "    PAGEYPOS=\"18\" \n"
				  "    MNAM=\"Normal\" \n"        //name of attached master page
				  "    HorizontalGuides=\"\" \n"
				  "    NumHGuides=\"0\" \n"
				  "    VerticalGuides=\"\" \n"
				  "    NumVGuides=\"0\" \n"
				  "   />\n");
				
		 // for each page in spread layout..
		for (c2=0; c2<spread->pagestack.n; c2++) {
			pg=spread->pagestack.e[c2]->index;
			if (pg>=doc->pages.n) continue;
			 // for each layer on the page..
			for (l=0; l<doc->pages[pg]->layers.n(); l++) {
				 // for each object in layer
				g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
				for (c3=0; c3<g->n(); c3++) {
					transform_copy(m,spread->pagestack.e[c2]->outline->m());
					scribusdumpobj(f,m,g->e(c3),error_ret,warning);
				}
			}
		}

		delete spread;
	}
		
	
	 // write out footer
	fprintf(f,"  </DOCUMENT>\n"
			  "</SCRIBUSUTF8NEW>");
	
	fclose(f);
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


	ImageData *img=NULL;
	GradientData *grad=NULL;
	int numpo=0;
	flatpoint *pocoor;
	int ptype=-1; //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path

	if (!strcmp(obj->whattype(),"ImageData")) {
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;
		ptype=2;
	} else if (!strcmp(obj->whattype(),"GradientData")) {
		grad=dynamic_cast<GradientData *>(obj);
		if (!grad) return;
		ptype=-2;
	} else if (!strcmp(obj->whattype(),"Group")) {
		 //must propogate transform...
		Group *g;
		g=dynamic_cast<Group *>(obj);
		if (!g) return;
		double m[6];
		transform_mult(m,mm,g->m());

		 // objects have GROUPS list for what groups they belong to, should
		 // maintain a list of current group nesting, this list will be the GROUPS
		 // element of subsequent objects
		 // global var groupc is a counter for how many distinct groups have been found,
		 // which has been found already

		ongroup++;
		groups.push(ongroup);
		for (int c=0; c<g->n(); c++) 
			scribusdumpobj(f,m,g->e(c),error_ret,warning);
		groups.pop();
		return;
	} else {
		char *tmp=new char[strlen(_("Warning: Cannot export %s to Scribus.\n"))+strlen(obj->whattype())+1];
		sprintf(tmp,_("Warning: Cannot export %s to Scribus.\n"),obj->whattype());
		appendstr(*error_ret,tmp);
		warning++;
		delete[] tmp;
		return;
	}

	//******HACK!
	numpo=16;
	pocoor=new flatpoint[numpo];
	flatpoint p;
	double mmm[6];
	//transform_mult(mmm,obj->m(),mm);
	transform_copy(mmm,mm);
	pocoor[14]=pocoor[15]=pocoor[ 0]=pocoor[ 1]=transform_point(mmm,flatpoint(obj->minx,obj->miny));
	pocoor[ 2]=pocoor[ 3]=pocoor[ 4]=pocoor[ 5]=transform_point(mmm,flatpoint(obj->maxx,obj->miny));
	pocoor[ 6]=pocoor[ 7]=pocoor[ 8]=pocoor[ 9]=transform_point(mmm,flatpoint(obj->maxx,obj->maxy));
	pocoor[10]=pocoor[11]=pocoor[12]=pocoor[13]=transform_point(mmm,flatpoint(obj->minx,obj->maxy));
	p=transform_point(mm,flatpoint(0,0));
	double rot,x,y,width,height;
	rot=atan2(pocoor[2].y-pocoor[1].y, pocoor[2].x-pocoor[1].x);
	x=p.x;
	y=p.y;
	//x=y=0;
	width=norm(pocoor[2]-pocoor[1]);
	height=norm(pocoor[6]-pocoor[2]);

	fprintf(f,"  <PAGEOBJECT \n"
			  "    ANNOTATION=\"0\" \n"   //1 if is pdf annotation
			  "    BOOKMARK=\"0\" \n"     //1 if obj is pdf bookmark
			  "    PFILE2=\"\" \n"          //(opt) file for pressed image in pdf button
			  "    PFILE3=\"\" \n"          //(opt) file for rollover image in pdf button

			  //"    CLIPEDIT=\"1\" \n"    //1 if shape was editted (opt)
			  "    doOverprint=\"0\" \n"   //not 1.2
			  "    fillRule=\"1\" \n"       //not 1.2
			  "    gHeight=\"551\" \n"      //not 1.2
			  "    gWidth=\"324\" \n"       //not 1.2
			  "    gXpos=\"74\" \n"         //not 1.2
			  "    gYpos=\"268\" \n"        //not 1.2
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
			  "    LOCALSCX=\"1\" \n"       //image scaling in x direction
			  "    LOCALSCY=\"1\" \n"       //image scaling in y direction
			  "    LOCALX=\"0\" \n"         //xpos of image in frame
			  "    LOCALY=\"0\" \n"         //ypos of image in frame
			  "    PFILE=\"%s\" \n",	    //file of image
			ptype==2?img->filename:"");

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
			  "    NUMCO=\"0\" \n"        //num coords in COCOOR==contour line==text wrap outline (opt) (vv opt)
			  "    COCOOR=\"\" \n"
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
}




//---------------------------- ScribusImportFilter --------------------------------
//Document *scribusin(const char *file,Document *doc,int startpage)
//{
//}




