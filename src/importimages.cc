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
// Copyright (c) 2007-2010 Tom Lechner
//

#include "language.h"
#include "importimages.h"
#include "importimage.h"
#include "laidout.h"
#include "utils.h"

#include <lax/checkbox.h>
#include <lax/fileutils.h>
#include <lax/menubutton.h>
#include <lax/tabframe.h>
#include <lax/sliderpopup.h>

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {



/*! \class ImportImagesDialog
 * \brief Dialog for importing many images all at once.
 */

/*! Only one of obj or doc should be defined. If obj is defined, then dump all the selected images
 * to obj within obj's bounds. Otherwise dump to doc with the full per page settings.
 *
 * \todo default dpi?
 */
ImportImagesDialog::ImportImagesDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			unsigned long  nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Group *obj,
			Document *ndoc,int startpg,double defdpi)
	: FileDialog(parnt,nname,ntitle,
			(nstyle&0xffff)|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
			xx,yy,ww,hh,brder,
			nowner,nsend,
			FILES_PREVIEW|FILES_OPEN_MANY,
			nfile,npath,nmask)
{
	toobj=obj;
	if (toobj) toobj->inc_count();
	doc=ndoc;
	if (doc) doc->inc_count();
	curitem=-1;

	reviewlist=NULL;

	settings=new ImportImageSettings;
	settings->startpage=startpg;

	double dpi=defdpi;
	if (dpi<=0) {
		if (doc && doc->imposition && doc->imposition->GetDefaultPaper())
			dpi=doc->imposition->GetDefaultPaper()->dpi;
		else dpi=300;//***
	}
	settings->defaultdpi=dpi;

	dialog_style|=FILES_PREVIEW;
}

ImportImagesDialog::~ImportImagesDialog()
{
	if (toobj) toobj->dec_count();
	if (doc)   doc->dec_count();

	if (settings) settings->dec_count();
}

void ImportImagesDialog::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute *att=dump_out_atts(NULL,0,context);
	att->dump_out(f,indent);
	delete att;
}

/*! Append to att if att!=NULL, else return new att.
 */
Attribute *ImportImagesDialog::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute("ImportImagesDialog",NULL);
	char scratch[100];

	 //window settings
	sprintf(scratch,"%d",win_x);
	att->push("win_x",scratch);

	sprintf(scratch,"%d",win_y);
	att->push("win_y",scratch);

	sprintf(scratch,"%d",win_w);
	att->push("win_w",scratch);

	sprintf(scratch,"%d",win_h);
	att->push("win_h",scratch);

	 //------------export settings
	double dpi      =dynamic_cast<LineInput *>(findWindow("DPI"))->GetLineEdit()->GetDouble(NULL);

	int perpage=-2; //force to 1 page
	if (dynamic_cast<CheckBox *>(findWindow("perpageexactly"))->State()==LAX_ON)
		perpage=dynamic_cast<LineInput *>(findWindow("NumPerPage"))->GetLineEdit()->GetLong(NULL);
	else if (dynamic_cast<CheckBox *>(findWindow("perpagefit"))->State()==LAX_ON)
		perpage=-1; //as will fit to page

	 //dpi
	sprintf(scratch,"%.10g",dpi);
	att->push("dpi",scratch);

	 //autopreview
	att->push("autopreview",
				(dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON)?"yes":"no");

	 //perPage
	if (perpage==-2) att->push("perPage","all");
	else if (perpage==-1) att->push("perPage","fit");
	else att->push("perPage",perpage,-1);

	 //maxPreviewWidth 200
	int w=dynamic_cast<LineInput *>(findWindow("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
	att->push("maxPreviewWidth",w,-1);

	 //defaultPreviewName any
	LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
	att->push("defaultPreviewName",prevbase->GetCText());

	return att;
}

/*! \todo  *** ensure that the dimensions read in are in part on screen...
 */
void ImportImagesDialog::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"win_x")) {
			IntAttribute(value,&win_x);
		} else if (!strcmp(name,"win_y")) {
			IntAttribute(value,&win_y);
		} else if (!strcmp(name,"win_w")) {
			IntAttribute(value,&win_w);
		} else if (!strcmp(name,"win_h")) {
			IntAttribute(value,&win_h);
		}
	}
}

/*! \todo preview base should be a list, not a single base, so should laidout->preview_base
 */
int ImportImagesDialog::init() 
{
	dialog_style|=FILES_PREVIEW;
	FileDialog::init();
	AddNull();
	AddWin(NULL,0, 2000,1990,0,50,0, 15,0,0,50,0, -1);

	 // set up the extra windows....
	  
//LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
//			int xx,int yy,int ww,int hh,unsigned int bordr,
//			anXWindow *prev,Window nowner,const char *nsend,
//			const char *newlabel,const char *newtext,unsigned int ntstyle,
//			int nlew,int nleh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0
	
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+4;
	char str[50];
	int c;
	
	anXWindow *last=NULL;
	LineInput *linp=NULL;
	MessageBar *mesbar=NULL;
	CheckBox *check=NULL;
	Button *tbut=NULL;


// ************** tabframe stuff NOT WORKING!!!
	 //---------------------- to insert list, next to main dir listing ---------------------------
	// menuinfo that allows rearranging, and has other info about what page image will be on...
	//--- for future! ---
	 //---------------------- create choose/review  ---------------------------
	 //*** need [Add] on choose tab, and [remove] on review tab
//	c=findWindowIndex("files"); //the original file MenuSelector
//	TabFrame *tabframe=new TabFrame(this,"choose",NULL,BOXSEL_LEFT|BOXSEL_TOP|BOXSEL_ONE_ONLY|BOXSEL_ROWS,
//										50,50,300,200,0, 
//										NULL, object_id, "choose");
//	tabframe->pw(100);
//	tabframe->wg(1000);
//	tabframe->ph(30);
//	tabframe->hg(1000);
////	reviewlist=new MenuSelector(this,"reviewlist",0, 0,0,0,0,1,
////			NULL,window,"reviewlist",
////			MENUSEL_SEND_ON_UP|MENUSEL_CURSSELECTS|MENUSEL_TEXTCOLORS|
////			MENUSEL_LEFT|MENUSEL_SUB_ON_LEFT|MENUSEL_SUB_FOLDER, NULL,0);
////	//***populate reviewlist as needed...
//
//	MenuSelector *oldfilelist=filelist;
//	filelist=new MenuSelector(this,"files",NULL,0, 0,0,0,0,1,
//			last,object_id,"files",
//			MENUSEL_SEND_ON_UP|MENUSEL_CURSSELECTS|MENUSEL_TEXTCOLORS|
//			MENUSEL_LEFT|MENUSEL_SUB_ON_LEFT|MENUSEL_SUB_FOLDER, &files,0);
//	filelist->tooltip(_("Choose from these files.\nRight-click drag scrolls"));
//
//	tabframe->AddWin(filelist,1, _("Choose"),NULL,0);
////	tabframe->AddWin(reviewlist,1, _("Review"),NULL);
//	//tabframe->SelectN(0);
//	wholelist.remove(c);
//	app->destroywindow(oldfilelist);
//	//-------------------
//	//***workaround for broken Laxkit::SquishyBox derived window insertion
//	WinFrameBox *wfb=new WinFrameBox();
//	wfb->win=tabframe;
//	memcpy(wfb->m, tabframe->m, 14*sizeof(int));//***beware sizeof induced segfaults!!
//	AddWin(wfb,1,c);
//	//-------------------
//	//AddWin(tabframe,c);
//	//-------------------


	 //---------------------- add prev/next file buttons next to file
	linp=dynamic_cast<LineInput *>(findWindow("file"));
	linp->SetLabel(" ");
	linp->GetLineEdit()->win_style|=LINEEDIT_SEND_ANY_CHANGE;
	c=findWindowIndex("file");
	AddWin(new MessageBar(this,"file",NULL,MB_MOVE, 0,0, 0,0, 0, "File? "), 1,c);
	tbut=new Button(this,"prev file",NULL,0, 0,0,0,0, 1, 
					linp,object_id,"prevfile",
					0,"<",NULL,NULL,3,3);
	tbut->tooltip(_("Jump to previous selected file"));
	AddWin(tbut,1, c+1);
	tbut=new Button(this,"next file",NULL,0, 0,0,0,0, 1, 
					tbut,object_id,"nextfile",
					0,
					">",NULL,NULL,3,3);
	tbut->tooltip(_("Jump to next selected file"));
	AddWin(tbut,1, c+2);
	
	 //---------------------- insert preview line input
	c=findWindowIndex("path");
	AddWin(new MessageBar(this,"previewm",NULL,MB_MOVE, 0,0, 0,0, 0, "Preview: "),1, c);
	MenuButton *menub;
	last=menub=new MenuButton(this,"previewlist",NULL,MENUBUTTON_DOWNARROW|MENUBUTTON_CLICK_CALLS_OWNER, 0,0,0,0,0,
							  linp,object_id,"previewlist",0,
							  NULL,1,
							  "v",NULL,NULL);
	menub->tooltip(_("Select from possible automatic previews"));
	AddWin(menub,1, c+1);
	last=linp=new LineInput(this,"preview",NULL,
						LINP_FILE, 0,0,0,0,0, 
						last,object_id,"preview",
						" ",NULL,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->win_style|=LINEEDIT_SEND_ANY_CHANGE;
	AddWin(linp,1, 200,100,1000,50,0, linp->win_h,0,0,50,0, c+2);
	last=tbut=new Button(this,"generate preview",NULL,0, 0,0,0,0, 1, 
			last,object_id,"generate",0,
			_("Generate"),NULL,NULL,3,3);
	tbut->tooltip(_("Generate a preview for file at this location."));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, c+3);
	AddNull(c+4);
	

	
	 //---------------------- per image preview controls ---------------------------
//	   filename:_____________
//		previewfile:_________
//		[[Re]Generate Preview]
//		use temporary preview
//		description:_________

	 //---------------------- extra image layout controls ---------------------------
	 //start page __0__   alignx___ aligny___
	last=NULL;
	sprintf(str,"%d",settings->startpage);
	last=linp=new LineInput(this,"StartPage",NULL,0, 0,0,0,0,0, last,object_id,"startpage",
						_("Start Page:"),str,0,
						0,0,2,2,2,2);
	linp->tooltip(_("The starting document page index to drop images onto. 0 is the first page."));
	AddWin(linp,1, linp->win_w+linpheight*8,100,1000,50,0, linp->win_h,0,0,50,0, -1);

	int currentpagetype=0;
	if (doc) {
		AddWin(NULL,0, linpheight*6,0,0,50,0, linpheight,0,0,50,0, -1);
		if (doc->imposition->NumPageTypes()>1) {
			 //only add this for more than one type
			mesbar=new MessageBar(this,"thispage",NULL,MB_MOVE, 0,0, 0,0, 0, _("Align on page:"));
			AddWin(mesbar,1, mesbar->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	
			SliderPopup *pagetypes;
			last=pagetypes=new SliderPopup(this,"pageTypes",NULL,0, 0,0, 0,0, 1, last,object_id,"pagetype");
			for (int c=0; c<doc->imposition->NumPageTypes(); c++) {
				pagetypes->AddItem(doc->imposition->PageTypeName(c),c);
			}
			pagetypes->tooltip(_("Alignment for this page type"));
			AddWin(pagetypes,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
		}

		 //install default alignment settings per page type
		for (int c=settings->alignment.n; c<doc->imposition->NumPageTypes(); c++) {
			settings->alignment.push(flatpoint(50,50));
		}
	}

	 //-----align x and y for each page type, this shows the settings for currentpagetype
	if (settings->alignment.n) sprintf(str,"%.10g",settings->alignment.e[currentpagetype].x);
	else sprintf(str,"center"); 
	last=linp=new LineInput(this,"alignx",NULL,0, 0,0,0,0,0, last,object_id,"alignx",
						_("Align X:"),str,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->setWinStyle(LINEEDIT_SEND_ANY_CHANGE,1);
	linp->tooltip(_("0 means align left, 50 center, 100 right, etc."));
	AddWin(linp,1, 200,100,1000,50,0, linp->win_h,0,0,50,0, -1);

	if (settings->alignment.n) sprintf(str,"%.10g",settings->alignment.e[currentpagetype].y);
	else sprintf(str,"center"); 
	last=linp=new LineInput(this,"aligny",NULL,0, 0,0,0,0,0, last,object_id,"aligny",
						_("Align Y:"),str,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->setWinStyle(LINEEDIT_SEND_ANY_CHANGE,1);
	linp->tooltip(_("0 means align top, 50 center, 100 bottom, etc."));
	AddWin(linp,1, 200,100,1000,50,0, linp->win_h,0,0,50,0, -1);
	AddNull();


	 //----dpi
	sprintf(str,"%.10g",settings->defaultdpi);
	last=linp=new LineInput(this,"DPI",NULL,0, 0,0,0,0,0, last,object_id,"dpi",
						_("Default dpi:"),str,0,
						0,0,2,2,2,2);
	AddWin(linp,1, linp->win_w+linpheight*8,10,100,50,0, linp->win_h,0,0,50,0, -1);
	
	last=check=new CheckBox(this,"scaleup",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"scaleup", _("Scale up"),5,5);
	if (settings->scaleup) check->State(LAX_ON); else check->State(LAX_OFF);
	check->tooltip(_("If necessary, scale up images to just fit within the target area"));
	AddWin(check,1,  check->win_w,0,0,50,0, check->win_h,0,0,50,0, -1);

	last=check=new CheckBox(this,"scaledown",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"scaledown", _("Scale down"),5,5);
	if (settings->scaledown) check->State(LAX_ON); else check->State(LAX_OFF);
	check->tooltip(_("If necessary, scale down images to just fit within the target area"));
	AddWin(check,1, check->win_w,0,0,50,0, check->win_h,0,0,50,0, -1);

	AddWin(NULL,0, 3000,2990,0,50,0, 0,0,0,50,0, -1);
	
	
	 //------------------------------ Images Per Page
	//Images Per Page ____, or "as many as will fit", or "all on 1 page"
		
	mesbar=new MessageBar(this,"perpage",NULL,MB_MOVE, 0,0, 0,0, 0, _("Images Per Page:"));
	AddWin(mesbar,1, mesbar->win_w,0,0,50,0, mesbar->win_h,0,0,50,0, -1);
	AddWin(NULL,0, 1000,1000,0,50,0, 0,0,0,50,0, -1);
	AddNull();
	
	AddSpacer(linpheight,0,0,50, linpheight,0,0,50);
	last=check=new CheckBox(this,"perpageexactly",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"perpageexactly", _("Exactly this many:"),5,5);
	if (settings->perpage>=0) {
		check->State(LAX_ON); 
		sprintf(str,"%d",settings->perpage);
	} else {
		check->State(LAX_OFF);
		sprintf(str,"1");
	}
	AddWin(check,1, check->win_w,1,0,50,0, check->win_h,0,0,50,0, -1);

	sprintf(str,"%d",settings->perpage>0?settings->perpage:1);
	last=linp=new LineInput(this,"NumPerPage",NULL,0, 0,0,0,0,0, last,object_id,"perpageexactlyn",
						NULL,str,0,
						textheight*10,textheight+4,2,2,2,2);
	linp->GetLineEdit()->setWinStyle(LINEEDIT_SEND_FOCUS_ON,1);
	AddWin(linp,1, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,50,0, -1);
	AddNull();

	AddSpacer(linpheight,0,0,50, linpheight,0,0,50);
	last=check=new CheckBox(this,"perpagefit",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"perpagefit", _("As many as will fit per page"),5,5);
	if (settings->perpage==-1) check->State(LAX_ON); else check->State(LAX_OFF);
	AddWin(check,1, check->win_w,0,0,50,0, check->win_h,0,0,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,50,0, -1);
	AddNull();

	AddSpacer(linpheight,0,0,50, linpheight,0,0,50);
	last=check=new CheckBox(this,"perpageall",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"perpageall", _("All on one page"),5,5);
	if (settings->perpage==-2) check->State(LAX_ON); else check->State(LAX_OFF);
	AddWin(check,1, check->win_w,1,0,50,0, check->win_h,0,0,50,0, -1);
	AddWin(NULL,0, 2001,2000,0,50,0, 0,0,0,50,0, -1);


	 //------------------------ preview options ----------------------
	last=linp=new LineInput(this,"PreviewBase",NULL,0, 0,0,0,0,0, last,object_id,"previewbase",
						_("Default name for previews:"),
						_("any"),0,
						//laidout->preview_file_bases.n?laidout->preview_file_bases.e[0]:".laidout.%.jpg",0,
						0,0,2,2,2,2);
	linp->tooltip(_("For file.jpg,\n"
				  "* gets replaced with the original file name (\"file.jpg\"), and\n"
				  "% gets replaced with the file name without the final suffix (\"file\")\n"));
	AddWin(linp,1, -1);
	MenuInfo *menu=new MenuInfo("Preview Bases");
	for (c=0; c<laidout->preview_file_bases.n; c++) {
		menu->AddItem(laidout->preview_file_bases.e[c],c);
	}
	menub=new MenuButton(this,"PreviewBase",NULL,MENUBUTTON_DOWNARROW, 0,0,0,0,0,
									 last,object_id,"previewbasemenu",0,
									 menu,1,
									 "v");
	menub->tooltip(_("Select from the available preview bases"));
	AddWin(menub,1, -1);
	AddWin(NULL,0, 3000,3000,0,50,0, 0,0,0,50,0, -1);//force left justify
	
	last=linp=new LineInput(this,"PreviewWidth",NULL,0, 0,0,0,0,0, last,object_id,"previewwidth",
						_("Default max width for new previews:"),NULL,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->SetText(laidout->max_preview_length);
	linp->tooltip(_("Any newly generated previews must fit\nin a square this many pixels wide"));
	AddWin(linp,1, -1);
	AddWin(NULL,0, 3000,3000,0,50,0, 0,0,0,50,0, -1);//force left justify
	 
	last=check=new CheckBox(this,"autopreview",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"autopreview", _("Make previews for files larger than"),5,5);
	check->State(LAX_OFF);
	AddWin(check,1, check->win_w,0,0,50,0, linpheight,0,0,50,0, -1);
	
	 //----------------- file size to maybe generate previews for 
	last=linp=new LineInput(this,"MinSize",NULL,0, 0,0,0,0,0, last,object_id,"mintopreview",
						" ",NULL,0,
						textheight*6,0,2,2,2,2);
	linp->GetLineEdit()->SetText(laidout->preview_over_this_size);
	linp->tooltip(_("Previews will be automatically generated \n"
			      "for files over this size, and then only \n"
				  "whenever the generated preview name doesn't\n"
				  "exist already"));
	AddWin(linp,1, -1);
	mesbar=new MessageBar(this,"kb",NULL,MB_MOVE, 0,0, 0,0, 0, "kB");
	AddWin(mesbar,1,  mesbar->win_w,0,0,50,0, mesbar->win_h,0,0,50,0, -1);
	AddWin(NULL,0, 3000,3000,0,50,0, 0,0,0,50,0, -1);//force left justify
	 

	 //-------------------- change Ok to Import
	tbut=dynamic_cast<Button *>(findWindow("fd-Ok"));
	if (tbut) tbut->Label(_("Import"));


	Sync(1);
	
	return 0;
}

int ImportImagesDialog::Event(const Laxkit::EventData *data,const char *mes)
{
	if (!strcmp(mes,"usethispreview")) {
		const StrsEventData *strs=dynamic_cast<const StrsEventData *>(data);
		if (!strs || !strs->n) return 1;

		LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
		preview->SetText(strs->strs[0]+2);
		char *full=fullFilePath(NULL);
		ImageInfo *info=findImageInfo(full);
		delete[] full;
		if (info) makestr(info->previewfile,strs->strs[0]+2);
		
		return 0;

	} else if (!strcmp(mes,"files")) {
		FileDialog::Event(data,mes);
		//DBG cerr <<"back in ImportImagesDialog files message..."<<endl;
		updateFileList();
		rebuildPreviewName();
		return 0;

	} else if (!strcmp(mes,"scaleup")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (!s) return 0;
		settings->scaleup=(s->info1==LAX_ON);
		return 0;

	} else if (!strcmp(mes,"scaledown")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		if (!s) return 0;
		settings->scaledown=(s->info1==LAX_ON);
		return 0;

	} else if (!strcmp(mes,"preview")) {
		 //something's been typed in the preview edit
		LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
		ImageInfo *info=findImageInfo(file->GetCText());
		if (!info) return 0;
		makestr(info->previewfile,preview->GetCText());
		return 0;

	} else if (!strcmp(mes,"new file")) {
//		FileDialog::ClientEvent(e,mes);
		char *full=fullFilePath(NULL);
		if (file_exists(full,1,NULL)) updateFileList();
		delete[] full;
		rebuildPreviewName();
		return 0;

	} else if (!strcmp(mes,"pagetype")) {
		SliderPopup *p=dynamic_cast<SliderPopup *>(findWindow("pageTypes"));
		LineInput *x=dynamic_cast<LineInput *>(findWindow("alignx"));
		LineInput *y=dynamic_cast<LineInput *>(findWindow("aligny"));

		int current=p->GetCurrentItemIndex();
		if (settings->alignment.e[current].x==0) x->SetText(_("left"));
		else if (settings->alignment.e[current].x==50) x->SetText(_("center"));
		else if (settings->alignment.e[current].x==100) x->SetText(_("right"));
		else x->SetText(settings->alignment.e[current].x);

		if (settings->alignment.e[current].y==0) y->SetText(_("top"));
		else if (settings->alignment.e[current].y==50) y->SetText(_("center"));
		else if (settings->alignment.e[current].y==100) y->SetText(_("bottom"));
		else y->SetText(settings->alignment.e[current].y);

		return 0;
		

	} else if (!strcmp(mes,"alignx")) {
		LineInput *i=dynamic_cast<LineInput *>(findWindow("alignx"));
		double a=50;
		if (!strcasecmp(i->GetCText(),_("left"))) a=0;
		else if (!strcasecmp(i->GetCText(),_("center"))) a=50;
		else if (!strcasecmp(i->GetCText(),_("right"))) a=100;
		else a=i->GetDouble();

		int current=0;
		SliderPopup *p=dynamic_cast<SliderPopup *>(findWindow("pageTypes"));
		if (p) current=p->GetCurrentItemIndex();
		settings->alignment.e[current].x=a;

		return 0;

	} else if (!strcmp(mes,"aligny")) {
		LineInput *i=dynamic_cast<LineInput *>(findWindow("aligny"));
		double a=50;
		if (!strcasecmp(i->GetCText(),_("top"))) a=0;
		else if (!strcasecmp(i->GetCText(),_("center"))) a=50;
		else if (!strcasecmp(i->GetCText(),_("bottom"))) a=100;
		else a=i->GetDouble();

		int current=0;
		SliderPopup *p=dynamic_cast<SliderPopup *>(findWindow("pageTypes"));
		if (p) current=p->GetCurrentItemIndex();
		settings->alignment.e[current].y=a;

		return 0;

	} else if (!strcmp(mes,"perpageexactly") || !strcmp(mes,"perpagefit") || !strcmp(mes,"perpageall")) {
		int c;
		if (!strcmp(mes,"perpageexactly")) c=0;
		else if (!strcmp(mes,"perpagefit")) c=1;
		else c=2;
		CheckBox *check;
		
		check=dynamic_cast<CheckBox *>(findWindow("perpageexactly"));
		if (c==0) check->State(LAX_ON); else check->State(LAX_OFF);
		
		check=dynamic_cast<CheckBox *>(findWindow("perpagefit"));
		if (c==1) check->State(LAX_ON); else check->State(LAX_OFF);
		
		check=dynamic_cast<CheckBox *>(findWindow("perpageall"));
		if (c==2) check->State(LAX_ON); else check->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"perpageexactlyn")) {
		 //just make sure perpageexactly check box is checked
		CheckBox *check;
		check=dynamic_cast<CheckBox *>(findWindow("perpageexactly"));
		check->State(LAX_ON);
		
		check=dynamic_cast<CheckBox *>(findWindow("perpagefit"));
		check->State(LAX_OFF);
		
		check=dynamic_cast<CheckBox *>(findWindow("perpageall"));
		check->State(LAX_OFF);

		return 0;

	} else if (!strcmp(mes,"dpi")) {
		//nothing to do

	} else if (!strcmp(mes,"startpage")) {
		//nothing to do

	//} else if (!strcmp(mes,"endpage")) {
		//nothing to do
	} else if (!strcmp(mes,"autopreview")) {
		cout <<" ***should gray and ungray the previews for over size"<<endl;

	} else if (!strcmp(mes,"mintopreview")) {
		cout <<" ***imp: mintopreview"<<endl;

	} else if (!strcmp(mes,"previewbase")) {
		cout <<" ***imp: previewbase"<<endl;

	} else if (!strcmp(mes,"previewlist")) {

		 // build and launch possible preview files menu
		if (isblank(file->GetCText())) return 0;
				
		MenuInfo *menu=new MenuInfo("Possible Preview Files");
		char *str,*full;
		LaxImage *img;
		full=fullFilePath(NULL);
		for (int c=0; c<laidout->preview_file_bases.n; c++) {
			str=previewFileName(full,laidout->preview_file_bases.e[c]);
			img=load_image(str);
			if (img) {
				prependstr(str,"* ");
				img->dec_count();
			} else prependstr(str,"  ");
			menu->AddItem(str,c);
		}
		MenuSelector *popup=new MenuSelector(NULL,NULL,menu->title, 
						ANXWIN_BARE|ANXWIN_HOVER_FOCUS,
						0,0,0,0, 1, 
						NULL,object_id,"usethispreview", 
						MENUSEL_LEFT
						 | MENUSEL_ZERO_OR_ONE|MENUSEL_CURSSELECTS
						 | MENUSEL_FOLLOW_MOUSE|MENUSEL_SEND_ON_UP
						 | MENUSEL_GRAB_ON_MAP|MENUSEL_OUT_CLICK_DESTROYS
						 | MENUSEL_CLICK_UP_DESTROYS|MENUSEL_DESTROY_ON_FOCUS_OFF
					 	 | MENUSEL_SEND_STRINGS,
						menu,1);
		popup->pad=3;
		popup->Select(0);
		popup->WrapToMouse(None);
		app->rundialog(popup);
		if (popup->object_id) app->setfocus(popup);
		else { app->destroywindow(popup); popup=NULL; }
		return 0;	

	} else if (!strcmp(mes,"previewbasemenu")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		int i=s->info1;
		if (i>=0 && i<laidout->preview_file_bases.n) {
			LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
			prevbase->SetText(laidout->preview_file_bases.e[i]);
		}
		return 0;

	} else if (!strcmp(mes,"generate")) {
		 // must generate what is in preview for file
		cout <<"*** imp ImportImageDialog -> generate!!"<<endl;
		return 0;

	} else if (!strcmp(mes,"nextfile") || !strcmp(mes,"prevfile")) {
			
		if (!images.n || !filelist->NumSelected()) return 0;
		
		if (curitem<0 || curitem>=filelist->NumSelected())
			curitem=filelist->NumSelected()-1;

		if (!strcmp(mes,"prevfile")) {
			curitem--;
			if (curitem<0) curitem=filelist->NumSelected()-1;
		} else {
			curitem++;
			if (curitem>filelist->NumSelected()-1) curitem=0;
		}

		 //set current file/path/previewer to c
		const MenuItem *m=filelist->GetSelected(curitem);
		char *full=fullFilePath(m->name);
		int c;
		ImageInfo *info=findImageInfo(full,&c);
		delete[] full;
		if (info) {
			SetFile(info->filename,info->previewfile);
			previewer->Preview(info->filename);
		}
		return 0;
	}

	return FileDialog::Event(data,mes);
}

//! Set the file and preview fields, changing directory if need be.
/*! \todo should probably redefine FileDialog::SetFile() also to check against
 * 		images?
 */
void ImportImagesDialog::SetFile(const char *f,const char *pfile)
{
	FileDialog::SetFile(f); //sets file and path
	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
	preview->SetText(pfile);
}

//! Change the Preview input to reflect a new file name.
void ImportImagesDialog::rebuildPreviewName()
{//***
	//DBG cerr <<"ImportImagesDialog::rebuildPreviewName()"<<endl;

	//int ifauto=dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON;
	//const char *f=linp->GetCText();
	
	 // find file in list 
	char *full=fullFilePath(NULL);
	ImageInfo *info=findImageInfo(full);
	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
	//DBG if (!preview) { cerr <<"*********rebuildPreviewName ERROR!!"<<endl; exit(1); }
	
	char *prev=NULL;
	if (info) prev=newstr(info->previewfile);
	if (!prev) prev=getPreviewFileName(full);
	if (!prev) prev=newstr("");
	preview->SetText(prev);
	delete[] full;
	delete[] prev;
	//DBG cerr <<"...done with ImportImagesDialog::rebuildPreviewName()"<<endl;
}

//! Create new ImageInfo nodes for any selected files not in the list.
/*! \todo this adds nodes to images. should put them in sorted to speed up checking?
 */
void ImportImagesDialog::updateFileList()
{//***
	//DBG cerr <<"ImportImagesDialog::updateFileList()"<<endl;
	curitem=-1;

	const MenuItem *item=NULL;
	ImageInfo *info=NULL;
	char *full=NULL;

	for (int c=0; c<filelist->NumSelected(); c++) {
		item=filelist->GetSelected(c);
		
		 // find file in list 
		full=fullFilePath(item->name);
		if (file_exists(full,1,NULL)==S_IFREG) {
			info=findImageInfo(full);
			if (!info) {
				 // add node
				char *prev=getPreviewFileName(full);
				images.push(new ImageInfo(full,prev,NULL,NULL,0));
				delete[] prev;
			} 
		}

		delete[] full;	
	}
	//DBG cerr <<"... done ImportImagesDialog::updateFileList()"<<endl;
}
	
//! Convert things like "24kb" and "3M" to kb.
/*! "never" gets translated to INT_MAX. "34" becomes 34 kilobytes
 *
 * Return 0 for success or nonzero for error in parsing. If there is 
 * an error, ll gets 0.
 *
 * Really this is pretty simple check. Looks for "number order" where
 * order is some text that begins with 'm' for megabytes, 'g' for gigabytes
 * or 'k' for kilobytes
 *
 * \todo this could be a Laxkit attribute reader
 */
int str_to_byte_size(const char *s, long *ll)
{
	char *str=newstr(s);
	stripws(str);
	if (!strcasecmp(str,"never")) {
		delete[] str;
		if (ll) *ll=INT_MAX;
		return 0;
	}
	char *e;
	long l=strtol(str,&e,10);//supposedly, e will never be NULL
	if (e==str) {
		delete[] str;
		if (ll) *ll=0;
		return 1;
	}
	while (isspace(*e)) e++;
	if (*e) {
		if (*e=='m' || *e=='M') l*=1024;
		else if (*e=='g' || *e=='G') l*=1024*1024;
		else if (*e=='k' || *e=='K') ; //kb is default
		else {
			 //unknown units
			delete[] str;
			if (ll) *ll=0;
			return 2;
		}
	}
	delete[] str;
	if (ll) *ll=l;
	return 0;
}

//! Return the Laxkit::ImageInfo in images with fullfile as the path.
/*! If i!=NULL, then set *i=(index of the imageinfo), -1 if not found.
 */
Laxkit::ImageInfo *ImportImagesDialog::findImageInfo(const char *fullfile,int *i)
{
	int c;
	char *full;
	for (c=0; c<images.n; c++) {
		full=fullFilePath(fullfile);
		if (!strcmp(full,images.e[c]->filename)) {
			delete[] full;
			if (i) *i=c;
			return images.e[c];
		}
		delete[] full;
	}
	if (i) *i=-1;
	return NULL;
}

//! Instead of sending the file name(s) to owner, call dump_images directly.
/*! Returns 0 if nothing done. Else nonzero.
 *  
 *  \todo on success, should probably send a status message to whoever called up
 *    this dialog? right now it only knows doc, not a specific location
 *  \todo implement preview base as list of possible preview bases and search
 *    mechanism, that is, by file, or by freedesktop thumbnail spec, kphotoalbum thumbs, etc
 *  \todo implement toobj
 *  \todo this prescreens files for reading images.. might be better to insert broken images
 *    rather than that. the dumpin functions do screening all over again.. would be nice if
 *    it was an option
 */
int ImportImagesDialog::send(int id)
{
	//if (!owner) return 0;
	
	//DBG cerr <<"====Generating file names for import images..."<<endl;
	
	int n=0;
	char **imagefiles=NULL, **previewfiles=NULL;

	if (filelist->NumSelected()>0) {
		n=0;
		imagefiles  =new char*[filelist->NumSelected()];
		previewfiles=new char*[filelist->NumSelected()];
		
		const MenuItem *item;
		for (int c=0; c<filelist->NumSelected(); c++) {
			imagefiles[n]=previewfiles[n]=NULL;
			item=filelist->GetSelected(c);

			if (item) {
				imagefiles[n]=path->GetText();
				if (imagefiles[n][strlen(imagefiles[n])-1]!='/') appendstr(imagefiles[n],"/");
				appendstr(imagefiles[n],item->name);
				if (is_bitmap_image(imagefiles[n])) {
					n++;
				} else {
					delete[] imagefiles[n];
				}
			}
		}
		if (!n) {
			delete[] imagefiles;   imagefiles=NULL;
			delete[] previewfiles; previewfiles=NULL;
		}

	} else { 
		 //nothing currently selected in the item list...
		 // so just use the single file in file input
		char *blah=path->GetText();
		if (blah[strlen(blah)]!='/') appendstr(blah,"/");
		appendstr(blah,file->GetCText());
		if (isblank(blah)) {
			delete[] blah;
		} else {
			if (is_bitmap_image(blah)) {
				n=1;
				imagefiles  =new char*[n];
				previewfiles=new char*[n];
				imagefiles[0]=blah;
				previewfiles[0]=NULL;
			} else {
				delete[] blah; blah=NULL;
				n=0;
			}
		}

	}
	if (!n) {
		cout <<"***need to implement importimagedialog send message to viewport or message saying no go."<<endl;
		return 0;
	}

	 //generate standard preview files to search for
	LineInput *templi;
	ImageInfo *info;
	for (int c=0; c<n; c++) {
		if (!imagefiles[c]) continue;
		if (file_exists(imagefiles[c],1,NULL)!=S_IFREG) {
			delete[] imagefiles[c];
			imagefiles[c]=NULL;
			continue;
		}
		info=findImageInfo(imagefiles[c]);
		if (info) previewfiles[c]=newstr(info->previewfile);
			else  previewfiles[c]=getPreviewFileName(imagefiles[c]);
		if (!file_exists(previewfiles[c],1,NULL)) {
			if (dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON) {

				long si,
					 s=file_size(imagefiles[c],1,NULL);
				templi=dynamic_cast<LineInput *>(findWindow("MinSize"));			
				str_to_byte_size(templi->GetCText(), &si);
				if (s>si) {
					//DBG cerr <<"-=-=-=--=-==-==-=-==-- Generate preview at: "<<previewfiles[c]<<endl;
					si=dynamic_cast<LineInput *>(findWindow("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
					if (si<10) si=128;
					if (generate_preview_image(imagefiles[c],previewfiles[c],"jpg",si,si,1)) {
						//DBG cerr <<"              ***generate preview failed....."<<endl;
					}
				}
			} else {
				delete[] previewfiles[c];
				previewfiles[c]=NULL;
			}
		}
	}
	settings->startpage =dynamic_cast<LineInput *>(findWindow("StartPage"))->GetLineEdit()->GetLong(NULL);
	settings->defaultdpi=dynamic_cast<LineInput *>(findWindow("DPI"))->GetLineEdit()->GetDouble(NULL);
	
	int perpage=-2; //force to 1 page
	if (dynamic_cast<CheckBox *>(findWindow("perpageexactly"))->State()==LAX_ON)
		perpage=dynamic_cast<LineInput *>(findWindow("NumPerPage"))->GetLineEdit()->GetLong(NULL);
	else if (dynamic_cast<CheckBox *>(findWindow("perpagefit"))->State()==LAX_ON)
		perpage=-1; //as will fit to page
	if (perpage==0 || perpage<-2) perpage=-1;
	settings->perpage=perpage;
		
	dumpInImages(settings, doc, (const char **)imagefiles,(const char **)previewfiles,n);
	deletestrs(imagefiles,n);
	deletestrs(previewfiles,n);

	SimpleMessage *mes=new SimpleMessage(NULL, n,0,0,0, win_sendthis);
	app->SendMessage(mes,win_owner,win_sendthis,object_id);

	return 1;
}

//! Based on what is in the preview file base selector/edit, return a file name.
/*! If the text of PreviewBase edit is "any", than select the first preview file that exists,
 * based on the names in laidout->preview_base_names. If none exist, then use the first in 
 * laidout->preview_base_names. If there are no templates there, use ".ladiout-*.jpg".
 *
 * f is the full file path.
 *
 * \todo *** this should be moved to somewhere outside of the dialog, as it is generally useful.
 * \todo *** this checks only existence of regular file at location, not whether it can actually be
 *   read in as an image.
 */
char *ImportImagesDialog::getPreviewFileName(const char *full)
{
	LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
	const char *prevtext=prevbase->GetCText();
	char *prev=NULL;

	if (!strcasecmp(prevtext,_("any"))) {
		int c;
		if (!laidout->preview_file_bases.n) prev=previewFileName(full,".laidout-*.jpg");
		else {
			for (c=0; c<laidout->preview_file_bases.n; c++) {
				prev=previewFileName(full,laidout->preview_file_bases.e[c]);
				if (file_exists(prev,1,NULL)==S_IFREG) {
					break;
				}
				delete[] prev; prev=NULL;
			}
			if (c==laidout->preview_file_bases.n) 
				prev=previewFileName(full,laidout->preview_file_bases.e[0]);
		}
	} else prev=previewFileName(full,prevtext);

	return prev;
}


} // namespace Laidout

