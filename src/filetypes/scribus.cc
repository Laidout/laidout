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
// Copyright (C) 2007,2010-2012 by Tom Lechner
//

#include <unistd.h>

#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/palette.h>
#include <lax/colors.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>

#include "../language.h"
#include "scribus.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../printing/psout.h"
#include "../utils.h"
#include "../headwindow.h"
#include "../impositions/singles.h"
#include "../dataobjects/mysterydata.h"
#include "../drawdata.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {


//1.5 inches and 1/4 inch
#define CANVAS_MARGIN_X 100.
#define CANVAS_MARGIN_Y 20.
#define CANVAS_GAP      40.


//------------------------ Scribus in/reimpose/out helpers -------------------------------------

//! Creates a Laidout Document from a Scribus file, and adds to laidout->project.
/*! Return 0 for success or nonzero for error.
 *
 * If existingdoc!=NULL, then insert the file to that Document. In this case, it is not
 * pushed onto the project, as it is assumed it is elsewhere. Note that this will
 * basically wipe the existing document, and replace with the Scribus document.
 */
int addScribusDocument(const char *file, Document *existingdoc)
{
	FILE *f=fopen(file,"r");
	if (!f) return 1;
	char chunk[2000];
	size_t c=fread(chunk,1,1999,f);
	chunk[c]='\0';
	fclose(f);

	 //find default page width and height
	double w,h;
	const char *ptr=strstr(chunk,"PAGEWIDTH");
	if (!ptr) return 2;
	ptr+=11;
	w=strtod(ptr,NULL);
	if (w==0) return 3;

	ptr=strstr(chunk,"PAGEHEIGHT");
	if (!ptr) return 4;
	ptr+=12;
	h=strtod(ptr,NULL);
	if (h==0) return 5;

	PaperStyle paper("custom",w,h,0,300,"pt");
	
	Singles *imp=new Singles;
	imp->SetPaperSize(&paper);
	imp->NumPages(1);

	Document *newdoc=existingdoc;
	if (newdoc) {
		makestr(newdoc->saveas,NULL); //force rename later
		if (newdoc->imposition) newdoc->imposition->dec_count();
		newdoc->imposition=imp;
		imp->inc_count();
	} else {
		newdoc=new Document(imp,NULL);//null file name to force rename on save
	}

	makestr(newdoc->name,"From ");
	appendstr(newdoc->name,file);
	imp->dec_count();

	if (!existingdoc) laidout->project->Push(newdoc);
	else newdoc->inc_count();

	ScribusImportFilter filter;
	ImportConfig config(file,300, 0,-1, 0,-1,-1, newdoc,NULL);
	config.keepmystery=2;
	config.filter=&filter;
	ErrorLog log;
	filter.In(file,&config,log);

	newdoc->dec_count();

	return 0;
}

int exportImposedScribus(Document *doc,const char *imposeout)
{
	ScribusExportFilter filter;

	cerr <<" Warning: devs need to examine paper/page sizes more closely for scribus export.."<<endl;
	PaperStyle *paper=doc->imposition->GetDefaultPaper();
	PaperGroup papergroup(paper);
	DocumentExportConfig config(doc,NULL,imposeout,NULL,PAPERLAYOUT,0,-1,&papergroup);
	//DocumentExportConfig config(doc,NULL,imposeout,NULL,PAPERLAYOUT,0,-1,doc->imposition->papergroup);

	config.filter=&filter;
	ErrorLog log;
	int err=export_document(&config,log);
	return err;
}

//--------------------------------- install Scribus filter

//! Tells the Laidout application that there's a new filter in town.
void installScribusFilter()
{
	ScribusExportFilter *scribusout=new ScribusExportFilter;
	scribusout->GetObjectDef();
	laidout->PushExportFilter(scribusout);
	
	ScribusImportFilter *scribusin=new ScribusImportFilter;
	scribusin->GetObjectDef();
	laidout->PushImportFilter(scribusin);
}


//--------------------------------- helper stuff -----------------------------
class PageObject
{
  public:
	LaxInterfaces::SomeData *data;
	int count;
	int cur;
	int links;
	int l,r,t,b, next,prev;
	int nativeid;
	PageObject(LaxInterfaces::SomeData *d, int native,int ll,int rr,int tt,int bb,int nn,int pp);
	~PageObject();
};

#define LINK_Left    1
#define LINK_Right   2
#define LINK_Top     4
#define LINK_Bottom  8
#define LINK_Next    16
#define LINK_Prev    32

PageObject::PageObject(SomeData *d, int native,int ll,int rr,int tt,int bb,int nn,int pp)
{
	nativeid=native;
	l=ll; r=rr; t=tt; b=bb; next=nn; prev=pp;
	links=0; //bits say if l..prev are original (0) or new (1)
	data=d;
	if (d) d->inc_count();
	count=cur=0;
}

PageObject::~PageObject()
{
	if (data) data->dec_count();
}


static void scribusdumpobj(FILE *f,int &curobj,PtrStack<PageObject> &pageobjects,double *mm,SomeData *obj,ErrorLog &log,int &warning);
static void appendobjfordumping(PtrStack<PageObject> &pageobjects, Palette &palette, SomeData *obj);
static int findobj(PtrStack<PageObject> &pageobjects, int nativeid, int what);
static int findobjnumber(Attribute *att, const char *what);


//------------------------------------ ScribusExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Value *newScribusExportConfig()
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Scribus"))
			d->filter=laidout->exportfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//! For now, just returns createExportConfig(), with filter forced to Scribus.
int createScribusExportConfig(ValueHash *context,ValueHash *parameters,Value **v_ret,ErrorLog &log)
{
	DocumentExportConfig *d=NULL;
	Value *v=NULL;
	int status=createExportConfig(context,parameters,&v,log);
	if (status==0 && v && v->type()==VALUE_Object) d=dynamic_cast<DocumentExportConfig *>(((ObjectValue *)v)->object);

	if (d) for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Scribus")) {
			d->filter=laidout->exportfilters.e[c];
			break;
		}
	}
	*v_ret=v;

	return 0;
}

//------------------------------------ ScribusImportConfig ----------------------------------

//! For now, just returns a new ImportConfig.
Value *newScribusImportConfig()
{
	ImportConfig *d=new ImportConfig;
	for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"Scribus"))
			d->filter=laidout->importfilters.e[c];
	}
	ObjectValue *v=new ObjectValue(d);
	d->dec_count();
	return v;
}

//! For now, just returns createImportConfig(), with filter forced to Scribus.
int createScribusImportConfig(ValueHash *context,ValueHash *parameters,Value **v_ret,ErrorLog &log)
{
	ImportConfig *d=NULL;
	Value *v=NULL;
	int status=createImportConfig(context,parameters,&v,log);
	if (status==0 && v && v->type()==VALUE_Object) d=dynamic_cast<ImportConfig *>(((ObjectValue *)v)->object);

	if (d) for (int c=0; c<laidout->importfilters.n; c++) {
		if (!strcmp(laidout->importfilters.e[c]->Format(),"Scribus")) {
			d->filter=laidout->importfilters.e[c];
			break;
		}
	}
	*v_ret=v;

	return 0;
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

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *ScribusExportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("DocumentExportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"ScribusExportConfig");
	makestr(styledef->Name,_("Scribus Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a Scribus file."));
	styledef->newfunc=newScribusExportConfig;
	styledef->stylefunc=createScribusExportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
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
int ScribusExportFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	DBG cerr <<"-----Scribus export start-------"<<endl;

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

	Attribute *scribushints=NULL;
	if (doc) scribushints=doc->iohints.find("Scribus");
	else scribushints=laidout->project->iohints.find("Scribus");

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
		appendstr(file,".sla");
	} else file=newstr(filename);

	f=open_file_for_writing(file,0,&log);
	if (!f) {
		DBG cerr <<" cannot save, "<<file<<" cannot be opened for writing."<<endl;
		delete[] file;
		return 3;
	}
	

	setlocale(LC_ALL,"C");

	int warning=0;
	Spread *spread=NULL;
	Group *g=NULL;
	Palette palette;
	int c,c2,l,pg,c3;

	
//	 //find out how many groups there are for DOCUMENT->GROUPC
//	 //**** is this necessary? marked as optional in 1.2 spec
//	 //*** also grab all color references
//	groupc=0;
//	if (doc) {
//		for (c=start; c<=end; c++) {
//			spread=doc->imposition->Layout(layout,c);
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
	PaperStyle *defaultpaper=papergroup->papers.e[0]->box->paperstyle;
	landscape=( defaultpaper->flags&1)?1:0;
	paperwidth= defaultpaper->width;
	paperheight=defaultpaper->height;

	int totalnumpages=(end-start+1)*papergroup->papers.n;
	if (config->evenodd==DocumentExportConfig::Even) {
		totalnumpages/=2;
		if (config->end%2==0) totalnumpages++;
	} else if (config->evenodd==DocumentExportConfig::Odd) {
		totalnumpages/=2;
		if (config->end%2==1) totalnumpages++;
	}
	
	 //------------ write out document attributes
	 //****** all the scribushints.slahead blocks are output as DOCUMENT attributes
	 //		  EXCEPT: BOOK, ANZPAGES, PAGEHEIGHT, PAGEWIDTH, ORIENTATION
	fprintf(f,"  <DOCUMENT \n"
			  "    ANZPAGES=\"%d\" \n",totalnumpages); //number of pages
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
				if (!strcmp(name,"BOOK")) continue;
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
				  "    BOOK=\"0\" \n"       //has facing pages if present, assume export will always be single scribus pages
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
		fprintf(f,"    PAGEHEIGHT=\"%f\" \n",72*paperheight);
		fprintf(f,"    PAGEWIDTH=\"%f\" \n",72*paperwidth);//***default page width and height
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

		//****write out default <COLOR> sections?
			
		
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

		//------------PageSets  (not in 1.2)
		//This sets up so there is one column of pages, with CANVAS_GAP units between them.
		fprintf(f,"    <PageSets>\n"
				  "      <Set Columns=\"1\" GapBelow=\"%g\" Rows=\"1\" FirstPage=\"0\" GapHorizontal=\"0\"\n"
				  "         Name=\"Single Page\" GapVertical=\"0\" />\n"
				  "    </PageSets>\n", CANVAS_GAP);

		//************MASTERPAGE not separate entity in 1.2
	} //if !scribushints for non PAGE and PAGEOBJECT elements of DOCUMENT
			
	 //Not quite sure when Sections were introduced. Section blocks determine page number format:
	 //<Sections>
	 //  <Section Number="0" Name="string" From="0" To="10" Type="..." Start="1" Reversed="0" Active="1"/>
	 //</Sections>
	 //  Type can be: Type_A_B_C, Type_a_b_c, Type_1_2_3, Type_I_II_III, Type_i_ii_iii, Type_None

	 //Sections are always output fresh. Sections are analogous to PageRanges, BUT they do not track
	 //when the layout is anything other than single, consecutive pages on a single sheet of paper.
	fprintf(f,"    <Sections>\n");
	fprintf(f,"      <Section Name=\"0\"\n"
			  "               Active=\"1\"\n"
			  "               Number=\"0\"\n"
			  "               Type=\"Type_1_2_3\"\n"
			  "               From=\"0\"\n"
			  "               To=\"%d\"\n"
			  "               Start=\"1\"\n"
			  "               Reversed=\"0\">\n"
			  "      </Section>\n", totalnumpages-1);
	fprintf(f,"    </Sections>\n");


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

	 //find object to pageobject link mapping
	PtrStack<PageObject> pageobjects; //we need to keep track of pageobject correspondence, as scribus docs
									 //object id is the order they appear in the file, so for linked objects,
									 //we need to know the order that they will appear!
	for (c=start; c<=end; c++) { //for each spread
		if (config->evenodd==DocumentExportConfig::Even && c%2==0) continue;
		if (config->evenodd==DocumentExportConfig::Odd && c%2==1) continue;

		if (doc) spread=doc->imposition->Layout(layout,c);
		for (p=0; p<papergroup->papers.n; p++) { //for each paper
					
			if (papergroup->objs.n()) {
				appendobjfordumping(pageobjects,palette,&papergroup->objs);
			}

			//if (limbo && limbo->n()) {
			//	appendobjfordumping(pageobjects,palette,limbo);
			//}

			if (spread) {
				if (spread->marks) {
					appendobjfordumping(pageobjects,palette,spread->marks);
				}

				 // for each page in spread layout..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
					pg=spread->pagestack.e[c2]->index;
					if (pg<0 || pg>=doc->pages.n) continue;

					 // for each layer on the page..
					for (l=0; l<doc->pages[pg]->layers.n(); l++) {
						 // for each object in layer
						g=dynamic_cast<Group *>(doc->pages[pg]->layers.e(l));
						for (c3=0; c3<g->n(); c3++) {
							appendobjfordumping(pageobjects,palette,g->e(c3));
						}
					}
				}
			} //if (spread)
		} //for each paper
		if (spread) { delete spread; spread=NULL; }
	} //for each spread

	 //establish correct linking
	int ll,o;
	PageObject *obj;
	for (int c=0; c<pageobjects.n; c++) {
		DBG cerr <<"pageobject "<<c<<": "<<pageobjects.e[c]->data->whattype()<<", count="<<pageobjects.e[c]->count<<endl;

		obj=pageobjects.e[c];
		ll=obj->links;

		 //it is necessary to check each link, not just one of a pair, since there may be a partial export.
		 //in that case, links are just terminated.
		
		if (!(ll&LINK_Right) && obj->r>=0) {
			o=findobj(pageobjects,obj->r,LINK_Left);
			if (o>=0) {
				obj->links|=LINK_Right;
				obj->r=o;
				pageobjects.e[o]->links|=LINK_Left;
				pageobjects.e[o]->l=c;
			} else {
				 //object not found, so zap the link
				obj->r=-1;
			}
		}

		if (!(ll&LINK_Left) && obj->l>=0) {
			o=findobj(pageobjects,obj->l,LINK_Right);
			if (o>=0) {
				obj->links|=LINK_Left;
				obj->l=o;
				pageobjects.e[o]->links|=LINK_Right;
				pageobjects.e[o]->r=c;
			} else {
				 //object not found, so zap the link
				obj->l=-1;
			}
		}

		if (!(ll&LINK_Top) && obj->t>=0) {
			o=findobj(pageobjects,obj->t,LINK_Bottom);
			if (o>=0) {
				obj->links|=LINK_Top;
				obj->t=o;
				pageobjects.e[o]->links|=LINK_Bottom;
				pageobjects.e[o]->b=c;
			} else {
				 //object not found, so zap the link
				obj->t=-1;
			}
		}

		if (!(ll&LINK_Bottom) && obj->b>=0) {
			o=findobj(pageobjects,obj->b,LINK_Top);
			if (o>=0) {
				obj->links|=LINK_Bottom;
				obj->b=o;
				pageobjects.e[o]->links|=LINK_Top;
				pageobjects.e[o]->t=c;
			} else {
				 //object not found, so zap the link
				obj->b=-1;
			}
		}

		if (!(ll&LINK_Next) && obj->next>=0) {
			o=findobj(pageobjects,obj->next,LINK_Prev);
			if (o>=0) {
				obj->links|=LINK_Next;
				obj->next=o;
				pageobjects.e[o]->links|=LINK_Prev;
				pageobjects.e[o]->prev=c;
			} else {
				 //object not found, so zap the link
				obj->next=-1;
			}
		}

		if (!(ll&LINK_Prev) && obj->prev>=0) {
			o=findobj(pageobjects,obj->prev,LINK_Next);
			if (o>=0) {
				obj->links|=LINK_Prev;
				obj->prev=o;
				pageobjects.e[o]->links|=LINK_Next;
				pageobjects.e[o]->next=c;
			} else {
				 //object not found, so zap the link
				obj->prev=-1;
			}
		}
	}

	 //--------output COLOR sections gleened above
	if (palette.colors.n) {
		PaletteEntry *color;
		for (int c=0; c<palette.colors.n; c++) {
			color=palette.colors.e[c];
			if (color->color_space==LAX_COLOR_RGB) {
				fprintf(f,"    <COLOR NAME=\"%d,%d,%d\" RGB=\"#%02x%02x%02x\" Spot=\"0\" Register=\"0\" />\n",
					palette.colors.e[c]->channels[0],  //r
					palette.colors.e[c]->channels[1],  //g
					palette.colors.e[c]->channels[2],  //b

					palette.colors.e[c]->channels[0]>>8,  //r hex
					palette.colors.e[c]->channels[1]>>8,  //g
					palette.colors.e[c]->channels[2]>>8); //b

			} else if (color->color_space==LAX_COLOR_CMYK) {
				fprintf(f,"    <COLOR NAME=\"%d,%d,%d,%d\" CMYK=\"#%02x%02x%02x%02x\" Spot=\"0\" Register=\"0\" />\n",
					palette.colors.e[c]->channels[0],  //c
					palette.colors.e[c]->channels[1],  //m
					palette.colors.e[c]->channels[2],  //y
					palette.colors.e[c]->channels[3],  //k

					palette.colors.e[c]->channels[0]>>8,  //c hex
					palette.colors.e[c]->channels[1]>>8,  //m
					palette.colors.e[c]->channels[2]>>8,  //y
					palette.colors.e[c]->channels[3]>>8); //k

			} else if (color->color_space==LAX_COLOR_GRAY) {
				fprintf(f,"    <COLOR NAME=\"%d,%d,%d\" RGB=\"#%02x%02x%02x\" Spot=\"0\" Register=\"0\" />\n",
					palette.colors.e[c]->channels[0],  //r
					palette.colors.e[c]->channels[0],  //g
					palette.colors.e[c]->channels[0],  //b

					palette.colors.e[c]->channels[0]>>8,  //r hex
					palette.colors.e[c]->channels[0]>>8,  //g
					palette.colors.e[c]->channels[0]>>8); //b
			}
		}
	}
		 


	int curobj=0;

	 //------now dump pages and objects to the file
	for (c=start; c<=end; c++) { //for each spread
		if (doc) spread=doc->imposition->Layout(layout,c);
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

			//if (limbo && limbo->n()) {
			//	scribusdumpobj(f,curobj,pageobjects,NULL,limbo,log,warning);
			//}

			if (papergroup->objs.n()) {
				scribusdumpobj(f,curobj,pageobjects,NULL,&papergroup->objs,log,warning);
			}


			if (spread) {
				if (spread->marks) {
					scribusdumpobj(f,curobj,pageobjects,NULL,spread->marks,log,warning);
				}

				 // for each page in spread layout..
				for (c2=0; c2<spread->pagestack.n(); c2++) {
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
							scribusdumpobj(f,curobj,pageobjects,NULL,g->e(c3),log,warning);
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

	DBG cerr <<"-----Scribus export end success-------"<<endl;

	return 0;
}

//! Add colors (without transparency) to palette, creating palette if palette==NULL.
/*! Return 0 for exists already, 1 for added.
 *
 * \todo *** really need to implement better color management, right now ONLY using screen colors rgb!!
 */
int addColor(Palette &palette, ScreenColor *color)
{
	 //search for existing color
	char name[100];
	sprintf(name,"%d,%d,%d", color->red, color->green, color->blue);
	for (int c=0; c<palette.colors.n; c++) {
		if (!strcmp(palette.colors.e[c]->name,name)) return 0;
	}
	palette.AddRGB(name, color->red, color->green, color->blue, 65535);
	return 1;
}

//! Internal function to find object to pageobject mapping.
/*! This adds one entry per object that will actually be dumped out is scribusdumpobj().
 */
static void appendobjfordumping(PtrStack<PageObject> &pageobjects, Palette &palette, SomeData *obj)
{
	//WARNING! This function must mirror scribusdumpobj() for what objects actually get output..

	//GradientData *grad=NULL;
	int ptype=PTYPE_None; //>0 is translatable to scribus object.
				  		 //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
	             		//-1 is not handled, -2 is laidout gradient, -3 is MysteryData

	int l=-1,r=-1,t=-1,b=-1, next=-1,prev=-1;
	int nativeid=-1;

	if (!strcmp(obj->whattype(),"ImageData") || !strcmp(obj->whattype(),"EpsData")) {
		ImageData *img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;
		ptype=PTYPE_Image;

	} else if (!strcmp(obj->whattype(),"PathsData")) {
		PathsData *paths=dynamic_cast<PathsData *>(obj);
		ptype=PTYPE_Polygon; // *** could be polygon, polyline, or line!!!

		if (paths->linestyle) addColor(palette,&paths->linestyle->color);
		if (paths->fillstyle) addColor(palette,&paths->fillstyle->color);
		for (int c=0; c<paths->paths.n; c++) {
			if (paths->paths.e[c]->linestyle) addColor(palette,&paths->paths.e[c]->linestyle->color);
		}

	//} else if (!strcmp(obj->whattype(),"GradientData")) {
	//	grad=dynamic_cast<GradientData *>(obj);
	//	if (!grad) return;
	//	ptype=PTYPE_Laidout_Gradient;
	//	*** attach colors

	//} else if (!strcmp(obj->whattype(),"ColorPatchData")) {
	//	grad=dynamic_cast<ColorPatchData *>(obj);
	//	if (!grad) return;
	//	ptype=PTYPE_Laidout_Gradient;
	//	*** attach colors, create mesh shading
	
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

		for (int c=0; c<g->n(); c++) appendobjfordumping(pageobjects,palette,g->e(c));
		return;

	} else if (!strcmp(obj->whattype(),"MysteryData")) {
		MysteryData *mdata=dynamic_cast<MysteryData *>(obj);
		if (!strcmp(mdata->importer,"Scribus")) {
			ptype=PTYPE_Laidout_MysteryData;
			next=findobjnumber(mdata->attributes,"NEXTITEM");
			prev=findobjnumber(mdata->attributes,"BACKITEM");
			l   =findobjnumber(mdata->attributes,"LeftLINK");
			r   =findobjnumber(mdata->attributes,"RightLINK");
			t   =findobjnumber(mdata->attributes,"TopLINK");
			b   =findobjnumber(mdata->attributes,"BottomLINK");
			nativeid=mdata->nativeid;
		} //else is someone else's mystery data
	} 

	if (ptype==PTYPE_None) return;

	int c,count=0;
	for (c=0; c<pageobjects.n; c++) {
		if (obj==pageobjects.e[c]->data) {
			 //If the object has already been encountered, then add another instance of it.
			 //This happens when we are outputting tiled impositions, or clone objects for instance.
			pageobjects.e[c]->count++;
			count++;
			//pageobjects.push(pageobjects.e[c],0);
			//return;
		}
	}
	 //Object not found, so add new reference
	PageObject *o=new PageObject(obj, nativeid,l,r,t,b,next,prev);
	o->count=count;
	pageobjects.push(o,1);
}

//! Find the original link number for an object, if any.
static int findobjnumber(Attribute *att, const char *what)
{
	if (!att) return -1;

	Attribute *a;
	a=att->find(what); //something like "NEXTITEM" in mysterydata info
	if (!a) return -1;

	int i; //the original linked object number, as recorded from the original Scribus file
	if (!IntAttribute(a->value,&i)) return -1;

	return i;
}

//! Find scribus objects that have been refered to, returning index in pageobjects.
/*! For instance, the NEXTITEM is an object number of the next object in a Scribus text object chain.
 * If there is a MysteryData object with that original object number, then that is what is returned.
 *
 * Return value is the index into the pageobjects stack.
 * 
 * For tiled impositions, there is potential trouble linking items. The PageObject class
 * helps against that at least a little, to ensure that resulting links on export point to
 * things that are at least consistent.
 */
static int findobj(PtrStack<PageObject> &pageobjects, int nativeid, int what)
{
	if (nativeid<0) return -1;

	PageObject *obj;
	for (int c=0; c<pageobjects.n; c++) {
		obj=pageobjects.e[c];

		if (obj->nativeid==nativeid) {
			if (!(obj->links&what)) return c; //return on finding an unassigned link
		}
	}

	return -1;
}

static int scribusaddpath(NumStack<flatpoint> &pts, Coordinate *path)
{
	Coordinate *p,*p2,*start;
	p=start=path->firstPoint(1);
	if (!p) return 0;

	 //build the path to draw
	double *ctm=psCTM(); //(scratch space coords)=(object coords)*(object->m())*ctm
	flatpoint c1,c2;
	start=p;
	int n=1; //number of points seen

	//pts.push(transform_point(ctm,start->p())); <-- points are all added below!!

	do { //one loop per vertex point
		p2=p->next; //p points to a vertex
		if (!p2) break;

		n++;

		//p2 now points to first Coordinate after the first vertex
		if (p2->flags&(POINT_TOPREV|POINT_TONEXT)) {
			 //we do have control points
			if (p2->flags&POINT_TOPREV) {
				c1=p2->p();
				p2=p2->next;
			} else c1=p->p();
			if (!p2) break;

			if (p2->flags&POINT_TONEXT) {
				c2=p2->p();
				p2=p2->next;
			} else { //otherwise, should be a vertex
				//p2=p2->next;
				c2=p2->p();
			}

			//Coordinates of the object shape, a list of groups of four points, ordered
            //vertex point - previous bezier control point - vertex point - next bezier control point.
            //However, the list starts with a vertex point - next control, so after those 2 points, the
            //the next 4 relate to the second vertex.
			pts.push(transform_point(ctm,p->p()));
			pts.push(transform_point(ctm,c1));
			pts.push(transform_point(ctm,p2->p()));
			pts.push(transform_point(ctm,c2));
		} else {
			 //we do not have control points, so is just a straight line segment
			pts.push(transform_point(ctm,p->p())); //vertex
			pts.push(transform_point(ctm,p->p())); //toprev control
			pts.push(transform_point(ctm,p2->p())); //vertex2
			pts.push(transform_point(ctm,p2->p())); //tonext control
		}
		p=p2;
	} while (p && p->next && p!=start);

	//if (p==start) fprintf(f,"z "); *** how to close paths? same point begin and end maybe??

	return n;
}

//! Internal function to dump out the obj.
/*! Can be Group, ImageData, or GradientData.
 *
 * \todo could have special mode where every non-recognizable object gets
 *   rasterized, and a new dir with all relevant files is created.
 */
static void scribusdumpobj(FILE *f,int &curobj,PtrStack<PageObject> &pageobjects,double *mm,SomeData *obj,
							ErrorLog &log,int &warning)
{
	//possibly set: ANNAME NUMGROUP GROUPS NUMPO POCOOR PTYPE ROT WIDTH HEIGHT XPOS YPOS
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
	int leftlink=-1, rightlink=-1, toplink=-1, bottomlink=-1; //for table grids
	int nextitem=-1, backitem=-1; //for text object chains
	int        numpo=0,      numco=0;
	flatpoint *pocoor=NULL, *cocoor=NULL;
	LineStyle *lstyle=NULL;
	FillStyle *fstyle=NULL;
	int createrect=1;

	if (!strcmp(obj->whattype(),"ImageData") || !strcmp(obj->whattype(),"EpsData")) {
		img=dynamic_cast<ImageData *>(obj);
		if (!img || !img->filename) return;
		ptype=PTYPE_Image;

	//} else if (!strcmp(obj->whattype(),"GradientData")) {
	//	grad=dynamic_cast<GradientData *>(obj);
	//	if (!grad) return;
	//	ptype=PTYPE_Laidout_Gradient;
	
	} else if (!strcmp(obj->whattype(),"PathsData")) {
		PathsData *pdata=dynamic_cast<PathsData *>(obj);
		if (!pdata) return;
		createrect=0;
		ptype=PTYPE_Polygon;

		 //build path
		NumStack<flatpoint> pts;
		lstyle=pdata->linestyle;
		fstyle=pdata->fillstyle;
		Coordinate *p;
		int n=0;

		for (int c=0; c<pdata->paths.n; c++) {
			p=pdata->paths.e[c]->path;
			if (!p) continue;

			n+=scribusaddpath(pts,p);

			//p=transform_point(ctm,p);
			//pts.push(p);

			if (c!=pdata->paths.n-1) {
				for (int c2=0; c2<4; c2++) pts.push(flatpoint(999999,999999));//path delimiter!
			}
		}
		if (!n) return; //no points to output!!

		numpo=numco=pts.n;
		pocoor=pts.extractArray();
		cocoor=new flatpoint[numco];
		memcpy(cocoor,pocoor,sizeof(flatpoint)*numpo);

	
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
			scribusdumpobj(f,curobj,pageobjects,NULL,g->e(c),log,warning);
		groups.pop();
		psPopCtm();
		return;

	} else if (!strcmp(obj->whattype(),"MysteryData")) {
		MysteryData *mdata=dynamic_cast<MysteryData *>(obj);
		if (!strcmp(mdata->importer,"Scribus")) {
			mysteryatts=mdata->attributes;
			//need to refigure position and orientation, pocoor, cocoor to scale to current, done below
			ptype=PTYPE_Laidout_MysteryData;
		} //else is someone else's mystery data
	} 

	if (ptype==PTYPE_None) {
		setlocale(LC_ALL,"");
		char *tmp=new char[strlen(_("Warning: Cannot export %s to Scribus.\n"))+strlen(obj->whattype())+1];
		sprintf(tmp,_("Warning: Cannot export %s to Scribus.\n"),obj->whattype());
		log.AddMessage(tmp,ERROR_Warning);
		setlocale(LC_ALL,"C");
		warning++;
		delete[] tmp;
		psPopCtm();
		return;
	}

	leftlink  =pageobjects.e[curobj]->l;
	rightlink =pageobjects.e[curobj]->r;
	toplink   =pageobjects.e[curobj]->t;
	bottomlink=pageobjects.e[curobj]->b;
	nextitem  =pageobjects.e[curobj]->next;
	backitem  =pageobjects.e[curobj]->prev;

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
	 
	 //figure out the COCOOR and POCOOR for an object
	flatpoint p,p1,p2, vx,vy;
	double *ctm=psCTM(); //(scratch space coords)=(object coords)*(object->m())*ctm
	double rot,x,y,width,height;

	vx=transform_vector(ctm,flatpoint(1,0));
	vy=transform_vector(ctm,flatpoint(0,1));
	rot=atan2(vx.y, vx.x)/M_PI*180; //rotation in degrees
	//p=transform_point(ctm,flatpoint(0,0));
	p=transform_point(ctm,flatpoint(obj->minx,obj->maxy)); //scribus origin is upper left
	x=p.x;
	y=p.y;

	 //create pocoor outline if necessary
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

		Attribute *cocooratt=mysteryatts->find("COCOOR");
		if (cocooratt) {
			createrect=0;
			//Attribute *tmp=mysteryatts->find("NUMCO");<--get directly from cocoor
			double *coords=NULL;
			DoubleListAttribute(cocooratt->value,&coords,&numco);
			numco/=2;
			cocoor=new flatpoint[numco];
			for (int c=0; c<numco; c++) {
				cocoor[c]=transform_point(ctm,coords[c*2]/72,coords[c*2+1]/72);
				DBG cerr <<"cocoor to canvas: "<<cocoor[c].x<<','<<cocoor[c].y<<endl;
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
		if (pocoor[c].x==999999) continue;
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
	p=transform_point(ctm,flatpoint(obj->minx,obj->miny));
	transform_from_basis(mmm,p,vx,vy);
	transform_invert(m,mmm);
	DBG cerr<<"transform back to object:"; dumpctm(m);

	 //pocoor and cocoor are in canvas coordinates, need to
	 //make pocoor and cocoor coords relative to the object origin, not the canvas
	for (int c=0; c<numpo; c++) {
		DBG cerr <<"pocoor: "<<pocoor[c].x<<','<<pocoor[c].y;
		if (pocoor[c].x!=999999) pocoor[c]=transform_point(m,pocoor[c]);
		if (fabs(pocoor[c].x)<1e-10) pocoor[c].x=0;
		if (fabs(pocoor[c].y)<1e-10) pocoor[c].y=0;
		DBG cerr <<" -->  "<<pocoor[c].x<<','<<pocoor[c].y<<endl;
	}
	for (int c=0; c<numco; c++) {
		DBG cerr <<"cocoor: "<<cocoor[c].x<<','<<cocoor[c].y;
		if (pocoor[c].x!=999999) cocoor[c]=transform_point(m,cocoor[c]);
		if (fabs(cocoor[c].x)<1e-10) cocoor[c].x=0;
		if (fabs(cocoor[c].y)<1e-10) cocoor[c].y=0;
		DBG cerr <<" -->  "<<cocoor[c].x<<','<<cocoor[c].y<<endl;
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
	const char *pfile=(ptype==PTYPE_Image?img->filename:NULL);
	if (mysteryatts) {
		char *name,*value;
		for (int c=0; c<mysteryatts->attributes.n; c++) {
			name=mysteryatts->attributes.e[c]->name;
			value=mysteryatts->attributes.e[c]->value;

			 //these are overwritten for all objects, even mystery objects
			if (!strcmp(name,"OwnPage")) continue;
			if (!strcmp(name,"LOCALSCX")) continue;
			if (!strcmp(name,"LOCALSCY")) continue;
			if (!strcmp(name,"PFILE")) { if (ptype!=PTYPE_Image) pfile=value; continue; }
			if (!strcmp(name,"ROT")) continue;
			if (!strcmp(name,"XPOS")) continue;
			if (!strcmp(name,"YPOS")) continue;
			if (!strcmp(name,"WIDTH")) continue;
			if (!strcmp(name,"HEIGHT")) continue;
			if (!strcmp(name,"NUMGROUP")) continue;
			if (!strcmp(name,"GROUPS")) continue;
			if (!strcmp(name,"NUMPO")) continue;
			if (!strcmp(name,"NUMCO")) continue;
			if (!strcmp(name,"POCOOR")) continue;
			if (!strcmp(name,"COCOOR")) continue;
			if (!strcmp(name,"NEXTITEM")) continue;
			if (!strcmp(name,"BACKITEM")) continue;
			if (!strcmp(name,"LeftLINK")) continue;
			if (!strcmp(name,"RightLINK")) continue;
			if (!strcmp(name,"TopLINK")) continue;
			if (!strcmp(name,"BottomLINK")) continue;
			if (!strcmp(name,"content:")) { content=c; continue; }

			 //otherwise, output the item!
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
				  "    gHeight=\"0\" \n"        //not 1.2
				  "    gWidth=\"0\" \n"         //not 1.2
				  "    gXpos=\"0\" \n"          //not 1.2
				  "    gYpos=\"0\" \n"          //not 1.2
				  "    isGroupControl=\"0\" \n" //not 1.2
				  "    isInline=\"0\" \n"       //not 1.2
				  "    OnMasterPage=\"\" \n");    //not 1.2
	}
	fprintf(f,    "    OwnPage=\"%d\" \n",currentpage);  //not 1.2, the page on object is on? ****

	 //always override object links:
	fprintf(f,    "    BACKITEM=\"%d\" \n"     //Number of the previous frame of linked textframe
				  "    NEXTITEM=\"%d\" \n"     //number of next frame for linked text frames
				  "    LeftLINK=\"%d\" \n"     //object number in table layout
				  "    RightLINK=\"%d\" \n"     //object number in table layout
				  "    TopLINK=\"%d\" \n"       //object number in table layout
				  "    BottomLINK=\"%d\" \n",   //object number in table layout
				  		backitem,nextitem,
				  		leftlink,rightlink,toplink,bottomlink);

	  //various such as stroke, fill, text options
	if (!mysteryatts) {
		fprintf(f,"    AUTOTEXT=\"0\" \n");     //1 if object is auto text frame

		fprintf(f,"    PLTSHOW=\"0\" \n"        //(opt) 1 if path for text on path should be visible
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
				  "    REXTRA=\"0\" \n");       //(opt) Distance of text from the right edge of the frame
			//---------eps tags:
		//fprintf(f,"    BBOXH=\"0\" \n"      //height of eps object (opt)
				  //"    BBOXX=\"0\" \n"      //width of eps object (opt)

			//---------path tags, fill/stroke
		if (!(lstyle && lstyle->hasStroke())) {
			fprintf(f,
				  "    NAMEDLST=\"\" \n"        //(opt) name of the custom line style
				  "    DASHOFF=\"0\" \n"        //(opt) offset for first dash
				  "    DASHS=\"\" \n"           //List of dash values, see the postscript manual for details
				  "    NUMDASH=\"0\" \n"        //number of entries in dash
				  "    PLINEART=\"1\" \n"       //how line is dashed, 1=solid, 2=- - -, 3=..., 4=-.-.-, 5=-..-..-
				  "    PLINEEND=\"0\" \n"       //(opt) linecap 0 flatcap, 16 square, 32 round
				  "    PLINEJOIN=\"0\" \n"      //(opt) line join, 0 miter, 64 bevel, 128 round
				  "    PWIDTH=\"1\" \n"         //line width of object
				  "    SHADE2=\"100\" \n"       //shading for stroke
				  "    TransValueS=\"0\" \n"    //(opt) Transparency value for stroke
				  "    PCOLOR2=\"None\" \n");  //color of stroke
		} else {
			fprintf(f,
				  "    NAMEDLST=\"\" \n"        //(opt) name of the custom line style
				  "    SHADE2=\"100\" \n"       //shading for stroke
				  "    DASHOFF=\"0\" \n");        //(opt) offset for first dash

			if (lstyle->dotdash) {
				fprintf(f,
					  "    DASHS=\"\" \n"           //List of dash values, see the postscript manual for details
					  "    NUMDASH=\"0\" \n"        //number of entries in dash
					  "    PLINEART=\"3\" \n");     //how line is dashed, 1=solid, 2=- - -, 3=..., 4=-.-.-, 5=-..-..-
			} else {
				fprintf(f,
					  "    DASHS=\"\" \n"           //List of dash values, see the postscript manual for details
					  "    NUMDASH=\"0\" \n"        //number of entries in dash
					  "    PLINEART=\"1\" \n");     //how line is dashed, 1=solid, 2=- - -, 3=..., 4=-.-.-, 5=-..-..-
			}


			 //stroke
			if (lstyle->capstyle==CapButt) fprintf(f,"    PLINEEND=\"0\"\n");
			else if (lstyle->capstyle==CapRound) fprintf(f,"    PLINEEND=\"32\"\n");
			else if (lstyle->capstyle==CapProjecting) fprintf(f,"    PLINEEND=\"16\"\n");

			if (lstyle->joinstyle==JoinMiter) fprintf(f,"    PLINEJOIN=\"0\"\n");
			else if (lstyle->joinstyle==JoinRound) fprintf(f,"    PLINEJOIN=\"128\"\n");
			else if (lstyle->joinstyle==JoinBevel) fprintf(f,"    PLINEJOIN=\"64\"\n");

			double w=norm(transform_point(ctm,flatpoint(0,0))-transform_point(ctm,flatpoint(0,lstyle->width)));
			fprintf(f,"    PWIDTH=\"%.10g\"\n",w);

			fprintf(f,"    TransValueS=\"%.10g\" \n"  //(opt) Transparency value for stroke
				      "    PCOLOR2=\"%d,%d,%d\" \n",      //color name of stroke
							1.-lstyle->color.alpha/65535.,
							lstyle->color.red, lstyle->color.green, lstyle->color.blue);
					     
		} //if lstyle->hasStroke

		 //fill
		if (fstyle && fstyle->hasFill()) {
			fprintf(f,"    PCOLOR=\"%d,%d,%d\"\n"   //name of color in palette we dumped out before
					  "    TransValue=\"%.10g\"\n"  //transparency value
				  	  "    SHADE=\"100\" \n"        //shading for fill
				  	  "    fillRule=\"%d\" \n",     //0 for Non zero winding rule, 1 for Even-Odd winding rule
						fstyle->color.red, fstyle->color.green, fstyle->color.blue,
						1-fstyle->color.alpha/65535.,
						fstyle->fillrule==EvenOddRule ? 1 : 0);
		} else {
			fprintf(f,"    PCOLOR=\"None\"\n"
				  	  "    SHADE=\"100\" \n"        //shading for fill
				  	  "    fillRule=\"1\" \n"       //0 for Non zero winding rule, 1 for Even-Odd winding rule
					  "    TransValue=\"0\"\n");
		}

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
				  "    SCALETYPE=\"0\" \n"      //(opt) how image can scale,0=free, 1=bound to frame
				  "    PICART=\"1\" \n"         //1 if image should be shown
				  "    LOCALX=\"0\" \n"         //xpos of image in frame
				  "    LOCALY=\"0\" \n");       //ypos of image in frame
	} //if !mysteryatts

	fprintf(f,    "    LOCALSCX=\"%g\" \n"      //image scaling in x direction
				  "    LOCALSCY=\"%g\" \n"      //image scaling in y direction
				  "    PFILE=\"%s\" \n",	    //file of image
				localscx,localscy,pfile?pfile:"");

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
	if (mysteryatts) {
		 //*** WARNING! THis is a hack, not sure how or why, but seems to work in many cases...
		y-=height;
		if (fabs(rot)>90) y+=2*height;
	}
	fprintf(f,"\" \n"			
				  "    ROT=\"%g\" \n"         //rotation of object
				  "    XPOS=\"%g\" \n"        //x of object
				  "    YPOS=\"%g\" \n"        //y of object
				  "    WIDTH=\"%g\" \n"       //width of object
				  "    HEIGHT=\"%g\" \n",     //height of object
								 rot,x,y,width,height);

	 //close the tag
	fprintf(f,">\n"); //close PAGEOBJECT opening tag
	 //output PAGEOBJECT elements
	if (mysteryatts && content>=0) {
		AttributeToXMLFile(f,mysteryatts->attributes.e[content],6);
	}
	fprintf(f,"  </PAGEOBJECT>\n");  //end of PAGEOBJECT


	delete[] pocoor;
	psPopCtm();

	curobj++;
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

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
ObjectDef *ScribusImportFilter::GetObjectDef()
{
	ObjectDef *styledef;
	styledef=stylemanager.FindDef("ScribusImportConfig");
	if (styledef) return styledef; 

	styledef=makeObjectDef();
	makestr(styledef->name,"ScribusImportConfig");
	makestr(styledef->Name,_("Scribus Import Configuration"));
	makestr(styledef->description,_("Configuration to import a Scribus file."));
	styledef->newfunc=newScribusImportConfig;
	styledef->stylefunc=createScribusImportConfig;

	stylemanager.AddObjectDef(styledef,0);
	styledef->dec_count();

	return styledef;
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
 * \todo COLOR, master pages, ensure text sizes ok upon scaling, scale to fit existing pages
 */
int ScribusImportFilter::In(const char *file, Laxkit::anObject *context, ErrorLog &log)
{
	DBG cerr <<"-----Scribus import start-------"<<endl;

	ImportConfig *in=dynamic_cast<ImportConfig *>(context);
	if (!in) {
		log.AddMessage(_("Missing config!"),ERROR_Fail);
		return 1;
	}

	Document *doc=in->doc;

	Attribute *att=XMLFileToAttribute(NULL,file,NULL);
	if (!att) {
		log.AddMessage(_("Could not read file!"),ERROR_Fail);
		return 2;
	}

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
	int start,end; //page indices in Scribus file
	if (in->instart<0) start=0; else start=in->instart;
	if (in->inend<0 || in->inend>=numpages) end=numpages-1; 
		else end=in->inend;

	 //find first page number, for offset page numbering
	int firstpagenum=0;
	a=scribusdoc->find("FIRSTPAGENUM");
	if (a) IntAttribute(a->value,&firstpagenum);

	SomeData pagebounds[end-start+1]; //max/min are the bounds in the Scribus canvas space,
									 //and m() is optional whole page transform to fit doc pages

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
	 //"PAGE", "PAGEOBJECT", "Sections", and "COLOR" fields, and maybe "PageSets"
	 //all the others we can safely ignore, and let pass into iohints

	 //create the document if necessary
	if (!doc && !in->toobj) {
		Imposition *imp=new Singles; //*** this is not necessarily so! uses PageSets??
		paper->flags=((paper->flags)&~1)|(landscape?1:0);
		imp->SetPaperSize(paper);
		doc=new Document(imp,Untitled_name());//incs imp count
		imp->dec_count();//remove initial count
	}

	if (doc && docpagenum+(end-start)>=doc->pages.n) //create enough pages to hold the Scribus pages
		doc->NewPages(-1,(docpagenum+(end-start+1))-doc->pages.n);

	Group *group=in->toobj;
	Attribute *page,*object;
	char scratch[50];
	MysteryData *mdata=NULL;
	char *name, *value;
	PtrStack<Page> masterpages;
	RefPtrStack<SomeData> masterpagebounds;


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
			if (pagenum<start || pagenum>end) continue; //only store pages that'll really be imported *** but what about object bleeds??

			DoubleAttribute(page->find("PAGEXPOS")->value,&pagebounds[pagenum].minx);
			DoubleAttribute(page->find("PAGEYPOS")->value,&pagebounds[pagenum].miny);
			DoubleAttribute(page->find("PAGEWIDTH")->value,&pagebounds[pagenum].maxx);
			DoubleAttribute(page->find("PAGEHEIGHT")->value,&pagebounds[pagenum].maxy);
			pagebounds[pagenum].maxx+=pagebounds[pagenum].minx;
			pagebounds[pagenum].maxy+=pagebounds[pagenum].miny;

			a=page->find("MNAM"); //the name of the master page to use for this page
			if (a && !isblank(a->value)) {
				makestr(pagebounds[pagenum].nameid,a->value);
			}

			 //remaining stuff is iohint 
			if (scribushints) {
				a=page->duplicate();
				sprintf(scratch,"%d",pagenum);
				makestr(a->name,"scribusPageHint");
				makestr(a->value,scratch);
				scribushints->push(a,-1);
			}

			 //create extra transform if we need to scale to pages
			if (doc && in->scaletopage!=0) {
				PageStyle *pagestyle=doc->pages.e[docpagenum+(pagenum-start)]->pagestyle;
				double scrw=(pagebounds[pagenum].maxx-pagebounds[pagenum].minx)/72,
					   scrh=(pagebounds[pagenum].maxy-pagebounds[pagenum].miny)/72;
				double sx,sy; //scaling factors: laidout page/scribus page
				sx=pagestyle->w()/scrw;
				sy=pagestyle->h()/scrh;
				if (sx>1 && sy>1) {
					 //scribus page fits entirely within laidout page. We need to center, but
					 //scale only if scaletopage==2
					if (in->scaletopage!=2) { sx=sy=1; }
				} 
				 //apply scale
				if (sx!=1 && sy!=1) {
					if (sy<sx) sx=sy;
					pagebounds[pagenum].m(0,sx);
					pagebounds[pagenum].m(3,sx);
				}
				 //center when dimensions vary
				if (scrw!=pagestyle->w() && scrh!=pagestyle->h()) {
					pagebounds[pagenum].m(4,(pagestyle->w()-scrw*sx)/2);
					pagebounds[pagenum].m(5,(pagestyle->h()-scrh*sx)/2);
				}
			}
			continue;

		} else if (!strcmp(name,"MASTERPAGE")) {
			 //MASTERPAGE objects are just like PAGE, but they hold MASTEROBJECTS instead.
			 //Each PAGE has MNAM which is the name of the master page to apply to it.
			 //Master page objects appear to be applied underneath all actual page objects.
			 //MASTEROBJECTs have an OnMasterPage attribute which is the name of the MASTERPAGE it belongs to.

			Page *mpage=new Page;
			page=scribusdoc->attributes.e[c];
			a=page->find("NAM");
			makestr(mpage->label,a->value);

			masterpages.push(mpage,1);

			SomeData *pagebound=new SomeData;
			masterpagebounds.push(pagebound); pagebound->dec_count();
			DoubleAttribute(page->find("PAGEXPOS")->value,  &pagebound->minx);
			DoubleAttribute(page->find("PAGEYPOS")->value,  &pagebound->miny);
			DoubleAttribute(page->find("PAGEWIDTH")->value, &pagebound->maxx);
			DoubleAttribute(page->find("PAGEHEIGHT")->value,&pagebound->maxy);
			pagebound->maxx+=pagebound->minx;
			pagebound->maxy+=pagebound->miny;

			//ignore other MASTERPAGE attributes, they are mainly just hints
		}
	}

	 //now scan for everything other than PAGE
	int pageobjectcount=-1;
	int masterpageindex=-1;
	Attribute *tmp=NULL;
	PtrStack<PageRange> newranges;

	for (c=0; c<scribusdoc->attributes.n; c++) {
		name=scribusdoc->attributes.e[c]->name;
		value=scribusdoc->attributes.e[c]->value;

		if (!strcmp(name,"PAGE") || !strcmp(name,"MASTERPAGE")) {
			continue;

		} else if (!strcmp(name,"PAGEOBJECT") || !strcmp(name,"MASTEROBJECT")) {
			object=scribusdoc->attributes.e[c];
			pageobjectcount++; //***should this increment for masterobjects too?

			masterpageindex=-1;
			if (!strcmp(name,"MASTEROBJECT")) {
				tmp=object->find("OnMasterPage");
				for (masterpageindex=0; masterpageindex<masterpages.n; masterpageindex++) {
					if (!strcmp(tmp->value,masterpages.e[masterpageindex]->label)) break;
				}
				if (masterpageindex==masterpages.n) masterpageindex=-1; //master page not found!
			}

			 //figure out what page it is supposed to be on..
			 // ***need some way to compensate for bleeding!!!
			tmp=object->find("OwnPage");
			if (tmp) IntAttribute(tmp->value,&pagenum);
			if (masterpageindex==-1 && (pagenum<start || pagenum>end)) continue; //***what about when object bleeding!!
			if (masterpageindex==-1 && doc) {
				 //update group to point to the document page's group
				curdocpage=docpagenum+(pagenum-start);
				group=dynamic_cast<Group *>(doc->pages.e[curdocpage]->layers.e(0)); //pick layer 0 of the page
			} else if (masterpageindex>=0) {
				group=dynamic_cast<Group *>(masterpages.e[masterpageindex]->layers.e(0));
			}

			double x=0,y=0,rot=0,w=0,h=0;
			double matrix[6];
			DoubleAttribute(object->find("XPOS")->value  ,&x);//***this could be att->doubleValue("XPOS",&x) for safety
			DoubleAttribute(object->find("YPOS")->value  ,&y);
			DoubleAttribute(object->find("ROT")->value   ,&rot); //rotation is in degrees
			DoubleAttribute(object->find("WIDTH")->value ,&w);
			DoubleAttribute(object->find("HEIGHT")->value,&h);

			if (masterpageindex==-1) { //pagebounds are only for document pages
				x-=pagebounds[pagenum].minx;
				y-=pagebounds[pagenum].miny;
				y=(pagebounds[pagenum].maxy-pagebounds[pagenum].miny)-y; //pageheight-y, needed to flip y around
			} else {
				x-=masterpagebounds.e[masterpageindex]->minx;
				y-=masterpagebounds.e[masterpageindex]->miny;
				y=(masterpagebounds.e[masterpageindex]->maxy-masterpagebounds.e[masterpageindex]->miny)-y; //pageheight-y, needed to flip y around
			}

			 //find out what type of Scribus object this is
			int ptype=atoi(object->find("PTYPE")->value); //2=img, 4=text, 5=line, 6=polygon, 7=polyline, 8=text on path
			rot*=-M_PI/180;

			matrix[0]=cos(rot);
			matrix[1]=sin(rot);
			matrix[2]=sin(rot);
			matrix[3]=-cos(rot);
			matrix[4]=x/72;
			matrix[5]=y/72;

			if (ptype==2 && in->keepmystery!=2) {
				 //we found an image so convert it to native Laidout object

				Attribute *pfile=object->find("PFILE");
				ImageData *image=dynamic_cast<ImageData *>(newObject("ImageData"));
				char *fullfile=full_path_for_file(pfile->value,NULL);
				image->LoadImage(fullfile); //this will set maxx, maxy to dimensions of the image
				delete[] fullfile;
				image->m(matrix);
				image->m(0,image->m(0)*w/image->maxx/72.);
				image->m(1,image->m(1)*w/image->maxx/72.);
				image->m(2,image->m(2)*h/image->maxy/72.);
				image->m(3,image->m(3)*h/image->maxy/72.);
				image->Flip(0);
				if (masterpageindex==-1 && in->scaletopage!=0) { //*** might have to scale for master pages too!!
					 //apply extra page transform to fit document page
					double mt[6];
					//transform_mult(mt,pagebounds[pagenum].m(),image->m());
					transform_mult(mt,image->m(),pagebounds[pagenum].m());
					image->m(mt);
				}
				group->push(image);
				image->dec_count();

			//} else if (ptype==5 && in->keepmystery!=2) { //line
			//} else if (ptype==6 && in->keepmystery!=2) { //line
			//} else if (ptype==7 && in->keepmystery!=2) { //line
			} else if (scribushints) { 
				 //undealt with object, push as MysteryData if in->keepmystery

				mdata=new MysteryData("Scribus"); //note, this is untranslated "Scribus"
				mdata->nativeid=pageobjectcount;

				if (ptype==2) makestr(mdata->name,"Image");
				else if (ptype==4) makestr(mdata->name,"Text Frame");
				else if (ptype==5) makestr(mdata->name,"Line");
				else if (ptype==6) makestr(mdata->name,"Polygon");
				else if (ptype==7) makestr(mdata->name,"Polyline");
				else if (ptype==8) makestr(mdata->name,"Text on path");
				mdata->m(matrix);
				mdata->maxx=w/72;
				mdata->maxy=h/72;
				mdata->attributes=object->duplicate();
				//int i=-1;
				//if (mdata->attributes->find("PFILE",&i)) {
				//	mdata->attributes.remove(i);
				//}
				if (masterpageindex==-1 && in->scaletopage!=0) { //*** might have to scale for master pages too!!
					 //apply extra page transform to fit document page
					double mt[6];
					//transform_mult(mt,pagebounds[pagenum].m(),mdata->m());
					transform_mult(mt,mdata->m(),pagebounds[pagenum].m());
					mdata->m(mt);
				}

				 //--scour mdata->attributes for <var name="pgco|pgno"/>
				 //mdata->attributes:
				 //  content:
				 //    ITEXT
				 //      CH blah
				 //    var
				 //      name pgco
				 //    var
				 //      name pgno
				if (masterpageindex==-1) tmp=mdata->attributes->find("content:");
				else tmp=NULL; //don't convert for mp's yet
				if (tmp) {
					Attribute *sub;
					int num=-1;
					char scratch[50];
					for (int c=0; c<tmp->attributes.n; c++) {
						sub=tmp->attributes.e[c];
						num=-1;
						if (strcmp(sub->name,"var")) continue;
						if (!strcmp(sub->attributes.e[0]->value,"pgno")) {
							num=firstpagenum+pagenum+1;
						} else if (!strcmp(sub->attributes.e[0]->value,"pgco")) {
							num=numpages;
						}
						if (num<0) continue;

						makestr(sub->name,"ITEXT");
						makestr(sub->attributes.e[0]->name,"CH");
						sprintf(scratch,"%d",num);
						makestr(sub->attributes.e[0]->value,scratch);
									
					}
				}

				group->push(mdata);
				mdata->dec_count();

			}

		} else if (!strcmp(name,"Sections")) {
			 //<Sections>
			 //  <Section Number="0" Name="string" From="0" To="10" Type="..." Start="1" Reversed="0" Active="1"/>
			 //</Sections>
			 //  Type can be: Type_A_B_C, Type_a_b_c, Type_1_2_3, Type_I_II_III, Type_i_ii_iii, Type_None
			Attribute *sub=scribusdoc->attributes.e[c]->find("content:");
			if (sub) {
				int num=-1, from=-1, to=-1, type=0, start=1, reversed=0; //, active=1;
				char *Name=NULL;
				for (int c2=0; c2<sub->attributes.n; c2++) {
					name =sub->attributes.e[c2]->name;
					value=sub->attributes.e[c2]->value;

					if (!strcmp(name,"Section")) {
						for (int c3=0; c3<sub->attributes.e[c2]->attributes.n; c3++) {
							name =sub->attributes.e[c2]->attributes.e[c3]->name;
							value=sub->attributes.e[c2]->attributes.e[c3]->value;

							if (!strcmp(name,"Number")) {
								IntAttribute(value,&num);
							} else if (!strcmp(name,"Name")) {
								Name=value;
							} else if (!strcmp(name,"From")) {
								IntAttribute(value,&from);
							} else if (!strcmp(name,"To")) {
								IntAttribute(value,&to);
							} else if (!strcmp(name,"Type")) {
								if (!strcmp(value,"Type_A_B_C")) type=Numbers_abc;
								else if (!strcmp(value,"Type_a_b_c")) type=Numbers_ABC;
								else if (!strcmp(value,"Type_1_2_3")) type=Numbers_Arabic;
								else if (!strcmp(value,"Type_I_II_III")) type=Numbers_Roman_cap;
								else if (!strcmp(value,"Type_i_ii_iii")) type=Numbers_Roman;
								else if (!strcmp(value,"Type_None")) type=Numbers_None;
								else type=Numbers_Arabic;
							} else if (!strcmp(name,"Start")) {
								IntAttribute(value,&start);
							} else if (!strcmp(name,"Reversed")) {
								reversed=BooleanAttribute(value);
							} else if (!strcmp(name,"Active")) {
								//active=BooleanAttribute(value);
							}
						}
						newranges.push(new PageRange(Name,"#",type,from+docpagenum,to+docpagenum,start+docpagenum,reversed));
					}
				}
			}

		//} else if (!strcmp(scribusdoc->attributes.e[c]->name,"COLOR")) {
			 //this will be something like:
			 // NAME "White"
			 // CMYK "#00000000"  (or RGB "#000000")
			 // Spot "0"
			 // Register "0"
			//***** finish me!
//			Palette *palette=new Palette;
//			char *cname=NULL;
//			Color *color=NULL;
//			int isspot=0, isreg=0;
//			for (int c2=0; c2<scribusdoc->attributes.e[c]->attributes.n; c2++) {
//				name=scribusdoc->attributes.e[c]->attributes.e[c2]->name;
//				value=scribusdoc->attributes.e[c]->attributes.e[c2]->value;
//				
//				if (!strcmp(name,"NAME")) {
//					cname=value;
//				} else if (!strcmp(name,"CMYK")) {
//					color=newCMYKColor(value);
//				} else if (!strcmp(name,"RGB")) {
//					color=newRGBColor(value);
//				} else if (!strcmp(name,"Spot")) {
//					isspot=BooleanValue(value);
//				} else if (!strcmp(name,"Register")) {
//					isreg=BooleanValue(value);
//				}
//			}
//			if (color) {
//				if (cname) color->Name(cname);
//				palette->push(color,1);
//			}
//			//*** further down the line, must do something akin to: project->pushResource(RES_Palette, palette);
//			continue;

		} else if (scribushints) {
			 //push any other blocks into scribushints.. we can usually safely ignore them
			Attribute *more=new Attribute("docContent",NULL);
			more->push(scribusdoc->attributes.e[c]->duplicate(),-1);
			scribushints->push(more,-1);
			continue;
		}
	}

	 //Apply any master pages by duplicating new objects on the relevant pages
	if (masterpages.n) {
		 //pages in range [start,end] from the Scribus file get imported into
		 //the Laidout document in range [docpagenum, docpagenum+start-end]
		Page *master, *docpage;
		SomeData *obj, *newobj;
		Group *layer;
		MysteryData *mobj;
		for (int c=docpagenum; c<=docpagenum+end-start; c++) {
			 //find which master page to use
			if (!pagebounds[c-docpagenum].nameid) continue; //no master page for this page
			master=NULL;
			for (int i=0; i<masterpages.n; i++) {
				if (!strcmp(pagebounds[c-docpagenum].nameid,masterpages.e[i]->label)) {
					master=masterpages.e[i];
					break;
				}
			}
			if (!master) continue; //master page not found, uh oh!
		
			 //apply the objects at beginning of stack. Master page objects occur under all other objects.
			docpage=doc->pages.e[c];
			for (int c2=0; c2<master->layers.n(); c2++) {//for each layer on master page
			  layer=dynamic_cast<Group*>(master->layers.e(c2));//this is a layer
			  for (int c3=0; c3<layer->n(); c3++) {//for each object in c2 layer of master page
				obj=layer->e(c3);
				DBG cerr<<"scribus master page object: "<<obj->whattype()<<endl;

				newobj=obj->duplicate(NULL);
				if (!newobj) {
					DBG cerr<<" *** could not duplicate "<<obj->whattype()<<endl;
					continue;
				}
				dynamic_cast<Group *>(docpage->layers.e(0))->push(newobj);

				mobj=dynamic_cast<MysteryData*>(newobj);
				if (mobj && (!strcmp(mobj->name,"Text Frame") || !strcmp(mobj->name,"Text on path"))) {
					 //now convert variable text
					tmp=mobj->attributes->find("content:");
					if (tmp) {
						Attribute *sub;
						int num=-1;
						char scratch[50];
						for (int c4=0; c4<tmp->attributes.n; c4++) {
							sub=tmp->attributes.e[c4];
							num=-1;
							if (strcmp(sub->name,"var")) continue;
							if (!strcmp(sub->attributes.e[0]->value,"pgno")) {
								num=start+(c-docpagenum)+1;
							} else if (!strcmp(sub->attributes.e[0]->value,"pgco")) {
								num=numpages;
							}
							if (num<0) continue;

							makestr(sub->name,"ITEXT");
							makestr(sub->attributes.e[0]->name,"CH");
							sprintf(scratch,"%d",num);
							makestr(sub->attributes.e[0]->value,scratch);
						}
					}
				}
				newobj->dec_count();
			}
		  }
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
	
	 //Apply the ranges
	if (doc && newranges.n) {
		PageRange *r;
		for (int c=0; c<newranges.n; c++) {
			r=newranges.e[c];
			doc->ApplyPageRange(r->name,r->labeltype,r->labelbase,r->start,r->end,r->first,r->decreasing);
		}
	}

	 //if doc is new, push into the project
	if (doc && doc!=in->doc) {
		laidout->project->Push(doc);
		laidout->app->addwindow(newHeadWindow(doc));
		doc->dec_count();
	}
	
	DBG cerr <<"-----Scribus import end successfully-------"<<endl;
	delete att;
	return 0;

}



} // namespace Laidout

