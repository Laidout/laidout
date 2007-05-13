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




//! Internal function to dump out the obj.
/*! Can be Group, ImageData, or GradientData.
 *
 * \todo could have special mode where every non-recognizable object gets
 *   rasterized, and a new dir with all relevant files is created.
 */
static void scribusdumpobj(FILE *f,double *mm,SomeData *obj)
{
	fprintf(f,"<PAGEOBJECT "
			      "OnMasterPage=\"\" "
				  "isGroupControl=\"0\" "
				  "BottomLine=\"0\" "
				  "REXTRA=\"0\" "
				  "gHeight=\"551\" "
				  "gWidth=\"324\" "
				  "NUMPO=\"28\" "
				  "TransBlendS=\"0\" "
				  "PLINEART=\"1\" "
				  "doOverprint=\"0\" "
				  "RightLine=\"0\" "
				  "LOCALSCX=\"1\" "
				  "ROT=\"0\" "
				  "WIDTH=\"250\" "
				  "ImageRes=\"1\" "
				  "GROUPS=\"1\" "
				  "LOCKR=\"0\" "
				  "LOCALSCY=\"1\" "
				  "NAMEDLST=\"\" "
				  "isInline=\"0\" "
				  "AUTOTEXT=\"0\" "
				  "FLIPPEDV=\"0\" "
				  "PCOLOR=\"Burlywood2\" "
				  "RADRECT=\"0\" "
				  "REVERS=\"0\" "
				  "PRINTABLE=\"1\" "
				  "RATIO=\"1\" "
				  "FLIPPEDH=\"0\" "
				  "COLGAP=\"0\" "
				  "PCOLOR2=\"Black\" "
				  "NEXTITEM=\"-1\" "
				  "NUMGROUP=\"1\" "
				  "TransValue=\"0\" "
				  "textPathFlipped=\"0\" "
				  "PLINEEND=\"0\" "
				  "FRTYPE=\"7\" "
				  "PTYPE=\"6\" "
				  "ImageClip=\"\" "
				  "isTableItem=\"0\" "
				  "TEXTFLOW2=\"0\" "
				  "SHADE2=\"100\" "
				  "PWIDTH=\"1\" "
				  "HEIGHT=\"283\" "
				  "DASHOFF=\"0\" "
				  "PFILE2=\"\" "
				  "PFILE=\"\" "
				  "TEXTFLOW3=\"0\" "
				  "textPathType=\"0\" "
				  "PLTSHOW=\"0\" "
				  "CLIPEDIT=\"1\" "
				  "BACKITEM=\"-1\" "
				  "TransValueS=\"0\" "
				  "EMBEDDED=\"1\" "
				  "PFILE3=\"\" "
				  "ANNAME=\"\" "
				  "SHADE=\"100\" "
				  "fillRule=\"1\" "
				  "COCOOR=\"0 0 0 0 250 0 250 0 250 0 250 0 250 283 250 283 250 283 250 283 0 283 0 283 0 283 0 283 0 0 0 0 \" "
				  "BASEOF=\"0\" "
				  "PICART=\"1\" "
				  "COLUMNS=\"1\" "
				  "OwnPage=\"3\" "
				  "LAYER=\"0\" "
				  "BOOKMARK=\"0\" "
				  "gYpos=\"268\" "
				  "startArrowIndex=\"0\" "
				  "TopLine=\"0\" "
				  "LOCK=\"0\" "
				  "EPROF=\"\" "
				  "gXpos=\"74\" "
				  "DASHS=\"\" "
				  "IRENDER=\"1\" "
				  "TEXTFLOW=\"0\" "
				  "YPOS=\"2887\" "
				  "TEXTFLOWMODE=\"0\" "
				  "ANNOTATION=\"0\" "
				  "LOCALX=\"0\" "
				  "GRTYP=\"0\" "
				  "XPOS=\"330\" "
				  "NUMCO=\"16\" "
				  "POCOOR=\"0 70.75 0 70.75 125 70.75 125 70.75 125 70.75 125 70.75 125 0 125 0 125 0 125 0 250 141.5 "
				      "250 141.5 250 141.5 250 141.5 125 283 125 283 125 283 125 283 125 212.25 125 212.25 125 212.25 125 "
					  "212.25 0 212.25 0 212.25 0 212.25 0 212.25 0 70.75 0 70.75 \" "
				  "EXTRA=\"0\" "
				  "LOCALY=\"0\" "
				  "NUMDASH=\"0\" "
				  "LeftLine=\"0\" "
				  "PRFILE=\"\" "
				  "TEXTRA=\"0\" "
				  "SCALETYPE=\"1\" "
				  "endArrowIndex=\"0\" "
				  "TransBlend=\"0\" "
				  "BEXTRA=\"0\" "
				  "PLINEJOIN=\"0\" "
				  ">\n");

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
		
		return;	
	}
	
	if (!strcmp(obj->whattype(),"Group")) {
		Group *group=dynamic_cast<Group *>(obj);
		if (!group) return;

		return;
	}
		
}



//! Save the document as a Scribus file to doc->saveas".sla"
/*! This only saves unnested images, and the page size and orientation.
 *
 */
int scribusout(const char *scribusversion, 
				Document *doc,const char *filename,	int layout,int start,int end)
{
	if (!doc->docstyle || !doc->docstyle->imposition || !doc->docstyle->imposition->paperstyle) return 1;
	
	FILE *f=NULL;
	char *file;
	if (!filename) {
		if (!doc->saveas || !strcmp(doc->saveas,"")) {
			cout <<"**** cannot save, doc->saveas is null."<<endl;
			return 2;
		}
		file=newstr(doc->saveas);
		appendstr(file,".sla");
	} else file=newstr(filename);
	
	f=fopen(file,"w");
	delete[] file; file=NULL;
	
	if (!f) {
		cout <<"**** cannot save, doc->saveas cannot be opened for writing."<<endl;
		return 3;
	}
	
	****sanity check start, end
	
	 // write out header
	fprintf(f,"<SCRIBUSUTF8NEW Version=\"1.3.3.8\"?>\n");
	
	 //------------ write out document attributes
	fprintf(f,"  <DOCUMENT "
			  "ABSTPALTEN=\"11\" "
			  "ANZPAGES=\"%d\" ",end-start);
	fprintf(f,"AUTHOR=\"\" "
			  "AUTOSPALTEN=1 "
			  "BOOK***** " *** has facing pages if present
			  "BORDERBOTTOM=******* "	
			  "BORDERLEFT=******* "	
			  "BORDERRIGHT=******* "	
			  "BORDERTOP=******* "	
			  "COMMENTS=\"\" "
			  "DFONT=\"*********\" "
			  "DSIZE=\"12\" "
			  "FIRSTLEFT " ********* doublesidedsingles->isleft
			  "FIRSTPAGENUM=\"0\" " ********could be custom for some pagerange styles
			  "KEYWORDS=\"\" "
			  "ORIENTATION=\"%d\" ",doc->docstyle->imposition->paperstyle->flags&1)?1:0);
	fprintf(f,"PAGEHEIGHT=\"*********\" "
			  "PAGEWIDTH=\"*********\" "
			  "TITLE=\"\" "
			  "VHOCH=\"33\" "
			  "VHOCHSC=\"100\" "
			  "VKAPIT=\"75\" "
			  "VTIEF=\"33\" "
			  "VTIEFSC=\"100\" "
			  ">");
	
			//***	doc->docstyle->imposition->paperstyle->name, 
	
				
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

	
	//*************DocItemAttributes
	//************TablesOfContents
	//************Sections
	//************PageSets
	//************MASTERPAGE
	
			
	//***********PAGE/PAGEOBJECTS
	

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
	for (c=start; c<end; c++) {
		 //------------page header
		fprintf(f,"  <PAGE "
				         "Size=\"Letter\" "
						 "NUM=\"0\" "
						 "BORDERTOP=\"18\" "
						 "NAM=\"\" "
						 "LEFT=\"0\" "
						 "BORDERBOTTOM=\"18\" "
						 "Orientation=\"0\" "
						 "BORDERRIGHT=\"18\" "
						 "NumVGuides=\"0\" "
						 "PAGEHEIGHT=\"792\" "
						 "PAGEWIDTH=\"612\" "
						 "PAGEYPOS=\"18\" "
						 "HorizontalGuides=\"\" "
						 "MNAM=\"Normal\" "
						 "PAGEXPOS=\"108\" "
						 "NumHGuides=\"0\" "
						 "VerticalGuides=\"\" "
						 "BORDERLEFT=\"18\" "
						 "/>\n");

				
		
		spread=doc->docstyle->imposition->GetLayout(layout,c);***
			
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

Document *scribusin(const char *file,Document *doc,int startpage)
{
}




