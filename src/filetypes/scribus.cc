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


#include "scribus.h"



****************** NOT ACTIVE ******************************




/*! \file 
 *
 * <pre>
 *  current 1.3.3.* file format is at:
 *  http://docs.scribus.net/index.php?lang=en&sm=scribusfileformat&page=scribusfileformat
 *
 *  for gradients: all optional tags:
 *  	DOCUMENT/PAGE/PAGEOBJECT:
 *  	  GRSTARTX
 *  	  GRENDX
 *  	  GRSTARTY
 *  	  GRENDY
 *  	  GRTYP: 6==free linear, 7==free radial
 *  	DOCUMENT/PAGE/PAGEOBJECT/CSTOP: (whole thing is optional)
 *  	  NAME   of the stop color
 *  	  RAMP   [0..1]
 *  	  SHADE  in percent
 *  	  TRANS  [0..1]
 *  for images:
 *  	 
 * </pre>
 */

const char *ScribusOutputFilter::VersionName()
{
	return _("Scribus 1.3.3.8");
}


//! Export the document as a Scribus file.
/*! 
 */
int ScribusOutputFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{***
	DocumentExportConfig *out=dynamic_cast<DocumentExportConfig *>(context);
	if (!out) return 1;

	if (error_ret) *error_ret=NULL;
	Document *doc =out->doc;
	int start     =out->start;
	//int end       =out->end;
	int layout    =out->layout;
	if (!filename) filename=out->filename;
	
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paperstyle) return 1;
	
	FILE *f=NULL;
	char *file;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			DBG cerr <<"**** cannot save, doc->saveas is null."<<endl;
			*error_ret=newstr(_("Cannot save to SVG without a filename."));
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".svg");
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
	int c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);

	if (start<0) start=0;
	else if (start>doc->docstyle->imposition->NumSpreads(layout))
		start=doc->docstyle->imposition->NumSpreads(layout)-1;
	spread=doc->docstyle->imposition->Layout(layout,start);
	
	
	 // write out header
	fprintf(f,"<SCRIBUSUTF8NEW Version=\"1.3.3.8\"?>\n");
	
	
	 //figure out paper orientation
	int landscape=0;
	int c;
	if (layout==PAPERLAYOUT) {
		landscape=((doc->docstyle->imposition->paperstyle->flags&1)?1:0);
	} 
	
	 //------------ write out document attributes
	fprintf(f,"  <DOCUMENT "
			  "    ABSTPALTEN=\"11\" \n"  //Distance between Columns in automatic Textframes
			  "    ANZPAGES=\"%d\" \n",end-start+1); //number of pages
	fprintf(f,"    AUTHOR=\"\" \n"
			  "    AUTOSPALTEN=1 \n" \\Number of Colums in automatic Textframes
			  //"    BOOK***** " *** has facing pages if present: doublesidedsingles
			  "    BORDERBOTTOM=\"0\" \n"	//margins!
			  "    BORDERLEFT=\"0\" \n"	
			  "    BORDERRIGHT=\"0\" \n"	
			  "    BORDERTOP=\"0\" \n"	
			  "    COMMENTS=\"\" \n"
			  "    DFONT=\"Times-Roman\" \n" //default font
			  "    DSIZE=\"12\" \n"
			  //"    FIRSTLEFT \n"  //*** doublesidedsingles->isleft
			  "    FIRSTPAGENUM=\"%d\" \n", start); ***check this is right
	fprintf(f,"    KEYWORDS=\"\" "
			  "    ORIENTATION=\"%d\" ",landscape);
	fprintf(f,"    PAGEHEIGHT=\"%f\" \n"
			  "    PAGEWIDTH=\"%f\" \n",width,height);****
	fprintf(f,"    TITLE=\"\" \n"
			  "    VHOCH=\"33\" \n"      //Percentage for Superscript
			  "    VHOCHSC=\"100\" \n"  // Percentage for scaling of the Glyphs in Superscript
			  "    VKAPIT=\"75\" \n"    //Percentage for scaling of the Glyphs in Small Caps
			  "    VTIEF=\"33\" \n"       //Percentage for Subscript
			  "    VTIEFSC=\"100\" \n"   //Percentage for scaling of the Glyphs in Subscript
			  "    >\n");
	
	
				
	//****write out <COLOR> sections
		
	
	 //----------Write layers, assuming just background. Everything else is grouping
	fprintf(f,"	   <LAYERS DRUCKEN=\"1\" NUMMER=\"0\" EDIT=\"1\" NAME=\"Background\" SICHTBAR=\"1\" LEVEL=\"0\" />\n");

	
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
	Spread *spread;
	Group *g;
	double m[6];
	int c,c2,l,pg,c3;
	transform_set(m,1,0,0,1,0,0);
	for (c=start; c<=end; c++) {
		 //------------page header
		fprintf(f,"  <PAGE "
				  "    Size=\"Letter\" \n" *****
				  "    PAGEHEIGHT=\"792\" \n" ****
				  "    PAGEWIDTH=\"612\" \n"****
				  "    NUM=\"%d\" \n",c); ***check this is right  //number of the page, starting at 0
		fprintf(f,"    BORDERTOP=\"18\" \n"     //page margins?
				  "    BORDERLEFT=\"18\" \n"
				  "    BORDERBOTTOM=\"18\" \n"
				  "    BORDERRIGHT=\"18\" \n"
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
				  "    />\n");

				
		
		spread=doc->docstyle->imposition->Layout(layout,c);
			
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
					scribusdumpobj(f,m,g->e(c3));
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
static void scribusdumpobj(FILE *f,double *mm,SomeData *obj)
{
	***possibly set: ANNAME NUMGROUP GROUPS NUMPO POCOOR PTYPE ROT WIDTH HEIGHT XPOS YPOS
		gradients: GRTYP GRSTARTX GRENDX GRSTARTY GRENDY
		images: LOCALSCX LOCALSCY PFILE


	fprintf(f,"  <PAGEOBJECT \n"
			  "    ANNAME=\"\" \n"        //field name, also object name
			  "    ANNOTATION=\"0\" \n"   //1 if is pdf annotation
			  "    AUTOTEXT=\"0\" \n"     //1 if object is auto text frame
			  "    BACKITEM=\"-1\" \n"    //Number of the previous frame of linked textframe
			  //"    BASEOF=\"0\" \n"     //text on a line offset (opt)
			  //"    BBOXH=\"0\" \n"      //height of eps object (opt)
			  //"    BBOXX=\"0\" \n"      //width of eps object (opt)
			  //"    BEXTRA=\"0\" \n"       //dist of text from bottom of frame (opt)
			  "    BOOKMARK=\"0\" \n"     //1 if obj is pdf bookmark
			  "    BottomLine=\"0\" \n"    //1 if table item has bottom line
			  //"    CLIPEDIT=\"1\" \n"    //1 if shape was editted (opt)
			  "    NUMCO=\"16\" \n"        //num coords in COCOOR==contour line==text wrap outline (opt) (vv opt)
			  "    COCOOR=\"0 0 0 0 250 0 250 0 250 0 250 0 250 283 250 283 250 283 250 283 0 283 0 283 0 283 0 283 0 0 0 0 \" \n"
			  "    COLGAP=\"0\" \n"        //Gap between text columns
			  "    COLUMNS=\"1\" \n"       //Number of columns in text
			  //"    DASHOFF=\"0\" \n"     //(opt) offset for first dash
			  "    DASHS=\"\" \n"          //List of dash values, see the postscript manual for details
			  "    NUMDASH=\"0\" \n"        //number of entries in dash
			  "    doOverprint=\"0\" \n"   //not 1.2
			  "    EMBEDDED=\"1\" \n"       //(opt) Set to 1 if embedded ICC-Profiles should be used
			  "    endArrowIndex=\"0\" \n"  //not 1.2
			  //"    EPROF=\"\" \n"         //(opt) Embedded ICC-Profile for images
			  "    EXTRA=\"0\" \n"          //Distance of text from the left edge of the frame
			  "    fillRule=\"1\" \n"       //not 1.2
			  "    FLIPPEDH=\"0\" \n"       //Set to an uneven number if object is flipped horizontal
			  "    FLIPPEDV=\"0\" \n"       //Set to an uneven number if object is flipped vertical
			  "    FRTYPE=\"7\" \n"         //shape of obj: 0=rect, 1=ellipse, 2=rounded rect, 3=free
			  "    gHeight=\"551\" \n"      //not 1.2
			  "    NUMGROUP=\"1\" \n"       //number of entries in GROUPS
			  "    GROUPS=\"1\" \n"         //List of group identifiers
			  "    GRTYP=\"0\" \n"          // 	Type of the gradient fill
											//  0 = No gradient fill,       1 = Horizontal gradient
											//  2 = Vertical gradient,      3 = Diagonal gradient
											//  4 = Cross diagonal gradient 5 = Radial gradient
											//  6 = Free linear gradient    7 = Free radial gradient
			  "    GRSTARTX=\"0\" \n"       //(grad only)X-Value of the start position of the gradient
			  "    GRENDX=\"0\"   \n"       //(grad only)X-Value of the end position of the gradient
			  "    GRSTARTY=\"0\" \n"       //(grad only)Y-Value of the start position of the gradient
			  "    GRENDY=\"0\"   \n"       //(grad only)Y-Value of the end position of the gradient
			  "    gWidth=\"324\" \n"       //not 1.2
			  "    gXpos=\"74\" \n"         //not 1.2
			  "    gYpos=\"268\" \n"        //not 1.2
			  "    ImageClip=\"\" \n"       //not 1.2
			  "    ImageRes=\"1\" \n"       //not 1.2
			  "    IRENDER=\"1\" \n"        //Rendering Intent for Images 
			  								//   0 = Perceptual 1 = Relative Colorimetric 2 = Saturation 3 = Absolute Colorimetric
			  "    isGroupControl=\"0\" \n" //not 1.2
			  "    isInline=\"0\" \n"       //not 1.2
			  "    isTableItem=\"0\" \n"    //1 if object belongs to table
			  "    LAYER=\"0\" \n"          //layer number object belongs to
			  "    LeftLine=\"0\" \n"       //(opt) 1 it table object has left line
			  "    LOCALSCX=\"1\" \n"       //image scaling in x direction
			  "    LOCALSCY=\"1\" \n"       //image scaling in y direction
			  "    LOCALX=\"0\" \n"         //xpos of image in frame
			  "    LOCALY=\"0\" \n"         //ypos of image in frame
			  "    LOCK=\"0\" \n"           //(opt) 1 if object locked
			  "    LOCKR=\"0\" \n"          //(opt) 1 if object protected against resizing
			  "    NAMEDLST=\"\" \n"        //(opt) name of the custom line style
			  "    NEXTITEM=\"-1\" \n"      //number of next frame for linked text frames
			  "    OnMasterPage=\"\" \n"    //not 1.2
			  "    OwnPage=\"3\" \n"        //not 1.2
			  "    PCOLOR2=\"Black\" \n"    //color of stroke
			  "    PCOLOR=\"Burlywood2\" \n"//color of fill
			  "    PFILE2=\"\" \n"          //(opt) file for pressed image in pdf button
			  "    PFILE3=\"\" \n"          //(opt) file for rollover image in pdf button
			  "    PFILE=\"\" \n"           //file of image
			  "    PICART=\"1\" \n"         //1 if image should be shown
			  "    PLINEART=\"1\" \n"       //how line is dashed, 1=solid, 2=- - -, 3=..., 4=-.-.-, 5=-..-..-
			  "    PLINEEND=\"0\" \n"       //(opt) linecap 0 flatcap, 16 square, 32 round
			  "    PLINEJOIN=\"0\" \n"      //(opt) line join, 0 miter, 64 bevel, 128 round
			  "    PLTSHOW=\"0\" \n"        //(opt) 1 if path for text on path should be visible
			  "    NUMPO=\"28\" \n"           //number of points in POCOOR, object shape coords
			  "    POCOOR=\"0 70.75 0 70.75 125 70.75 125 70.75 125 70.75 125 70.75 125 0 125 0 125 0 125 0 250 141.5 \n"
			  "    PRFILE=\"\" \n"          //(opt) icc profile for image
			  "    PRINTABLE=\"1\" \n"      //1 for object should be printed
			  "    PTYPE=\"6\" \n"          //object type, 2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
			  "    PWIDTH=\"1\" \n"         //line width of object
			  "    RADRECT=\"0\" \n"        //(opt) corner radius of rounded rectangle
			  "    RATIO=\"1\" \n"          //(opt) 1 if image scaling should respect aspect
			  "    REVERS=\"0\" \n"         //(opt) text is rendered reverse
			  "    REXTRA=\"0\" \n"         //(opt) Distance of text from the right edge of the frame
			  "    RightLine=\"0\" \n"      //(opt) 1 it table object has right line
			  "    SCALETYPE=\"1\" \n"      //(opt) how image can scale,0=free, 1=bound to frame
			  "    SHADE=\"100\" \n"         //shading for fill
			  "    SHADE2=\"100\" \n"        //shading for stroke
			  "    startArrowIndex=\"0\" \n" //not 1.2
			  "    TEXTFLOW=\"0\" \n"        //1 for text flows around object
			  "    TEXTFLOW2=\"0\" \n"       //(opt) 1 for text flows around bounding box
			  "    TEXTFLOW3=\"0\" \n"       //(opt) 1 for text flows around contour
			  "    TEXTFLOWMODE=\"0\" \n"    //not 1.2
			  "    textPathFlipped=\"0\" \n" //not 1.2
			  "    textPathType=\"0\" \n"    //not 1.2
			  "    TEXTRA=\"0\" \n"          //Distance of text from the top edge of the frame
			  "    TopLine=\"0\" \n"         //(opt) 1 it table object has top line
			  "    TransBlend=\"0\" \n"      //not 1.2
			  "    TransBlendS=\"0\" \n"     //not 1.2
			  "    TransValue=\"0\" \n"      //(opt) transparency value for fill
			  "    TransValueS=\"0\" \n"     //(opt) Transparency value for stroke
			  "    ROT=\"0\" \n"           //rotation of object
			  "    WIDTH=\"250\" \n"       //width of object
			  "    HEIGHT=\"283\" \n"      //height of object
			  "    XPOS=\"330\" \n"        //x of object
			  "    YPOS=\"2887\" \n"       //y of object
			  "    >\n");

	--------------
	if (!strcmp(obj->whattype(),"ImageData")) {
		ImageData *img;
		img=dynamic_cast<ImageData *>(obj);
		if (img && img->filename) return;

		double m[6];
		transform_mult(m,img->m(),mm);
		
		char *bname=basename(img->filename); // Warning! This assumes the GNU basename, which does
											 // not modify the string.
		fprintf(f,"    <frame name=\"Raster %s\" matrix=\"%.10g %.10g %.10g %.10g %.10g %.10g\" ",
				bname, m[0]*72, m[1]*72, m[2]*72, m[3]*72, m[4]*72, m[5]*72);
		fprintf(f,"lock=\"false\" flowaround=\"false\" obstaclemargin=\"0\" type=\"raster\" file=\"%s\" />\n",
				img->filename);
		return;
	}
	
	if (!strcmp(obj->whattype(),"GradientData")) {
		GradientData *grad;
		grad=dynamic_cast<GradientData *>(obj);
		if (!grad) return;
		
		***

		return;	
	}
	
	if (!strcmp(obj->whattype(),"Group")) {
		Group *group=dynamic_cast<Group *>(obj);
		if (!group) return;

		***

		return;
	}
		
	char *tmp=new char[strlen(_("Cannot export %s to Scribus.\n"))+strlen(obj->whattype())+1]
	sprintf(tmp,_("Cannot export %s to Scribus.\n"),obj->whattype());
	appendstr(*error_ret,tmp);
	delete[] tmp;
}




//Document *scribusin(const char *file,Document *doc,int startpage)
//{
//}




