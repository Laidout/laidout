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
// Copyright (c) 2007 Tom Lechner
//

#include "importimages.h"
#include "laidout.h"
#include "extras.h"
#include <lax/checkbox.h>
#include <lax/fileutils.h>
#include <lax/menubutton.h>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


/*! \class ImportImagesDialog
 * \brief Dialog for importing many images all at once.
 */

// 	FileDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//			int xx,int yy,int ww,int hh,int brder, 
//			Window nowner,const char *nsend,
//			const char *nfile=NULL,const char *npath=NULL,const char *nmask=NULL);
ImportImagesDialog::ImportImagesDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			Window nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Document *ndoc,int startpg,double defdpi)
	: FileDialog(parnt,ntitle,(nstyle&0xffff)|FILES_PREVIEW|FILES_OPEN_MANY,xx,yy,ww,hh,brder,nowner,nsend,nfile,npath,nmask)
{
	dpi=defdpi;
	startpage=startpg;
	doc=ndoc;

	dialog_style|=FILES_PREVIEW;
	win_h=win_w=700;
}

/*! \todo preview base should be a list, not a single base, so should laidout->preview_base
 */
int ImportImagesDialog::init() 
{
	dialog_style=(dialog_style&~(FILES_PREVIEW|FILES_PREVIEW2))|FILES_PREVIEW;
	FileDialog::init();
	AddNull();
	AddWin(NULL, 2000,1990,0,50, 15,0,0,50);

	 // set up the extra windows....
	  
//LineInput::LineInput(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
//			int xx,int yy,int ww,int hh,unsigned int bordr,
//			anXWindow *prev,Window nowner,const char *nsend,
//			const char *newlabel,const char *newtext,unsigned int ntstyle,
//			int nlew,int nleh,int npadx,int npady,int npadlx,int npadly) // all after and inc newtext==0
	
	int textheight=app->defaultfont->max_bounds.ascent+app->defaultfont->max_bounds.descent;
	int linpheight=textheight+4;
	char *str=NULL;
	int c;
	
	anXWindow *last=NULL;
	LineInput *linp=NULL;
	MessageBar *mesbar=NULL;
	CheckBox *check=NULL;
	TextButton *tbut=NULL;

	 //---------------------- to insert list, next to main dir listing ---------------------------
	// menuinfo that allows rearranging, and has other info about what page image will be on...
	//--- for future! ---


	 //---------------------- add prev/next file buttons next to file
	linp=dynamic_cast<LineInput *>(findWindow("file"));
	linp->SetLabel(" ");
	linp->GetLineEdit()->win_style|=LINEEDIT_SEND_ANY_CHANGE;
	c=findWindowIndex("file");
	AddWin(new MessageBar(this,"file",MB_MOVE, 0,0, 0,0, 0, "File? "), c);
	tbut=new TextButton(this,"prev file",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			linp,window,"prevfile",
			"<",3,3);
	tbut->tooltip("Jump to previous selected file");
	AddWin(tbut,c+1);
	tbut=new TextButton(this,"next file",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			tbut,window,"nextfile",
			">",3,3);
	tbut->tooltip("Jump to next selected file");
	AddWin(tbut,c+2);
	
	 //---------------------- insert preview line input
	c=findWindowIndex("path");
	AddWin(new MessageBar(this,"previewm",MB_MOVE, 0,0, 0,0, 0, "Preview: "), c);
	MenuButton *menub=new MenuButton(this,"previewlist",MENUBUTTON_DOWNARROW|MENUBUTTON_CLICK_CALLS_OWNER, 0,0,0,0,0,
									 linp,window,"previewlist",0,
									 NULL,1,
									 (const char *)NULL,"v");
	menub->tooltip("Select from possible automatic previews");
	AddWin(menub,c+1);
	last=linp=new LineInput(this,"preview",LINP_FILE, 0,0,0,0,0, last,window,"preview",
						" ",NULL,0,
						0,0,2,2,2,2);
	//makestr(linp->GetLineEdit()->qualifier,****);
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50, c+2);
	tbut=new TextButton(this,"generate preview",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			NULL,window,"generate",
			"Generate",3,3);
	tbut->tooltip("Generate a preview for file at this location.");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50, c+3);
	AddNull(c+4);
	
	
	 //---------------------- per image preview controls ---------------------------
//	***filename:_____________
//		previewfile:_________
//		[[Re]Generate Preview]
//		use temporary preview
//		description:_________

	 //---------------------- extra image layout controls ---------------------------
	str=numtostr(startpage,0);
	last=linp=new LineInput(this,"StartPage",0, 0,0,0,0,0, last,window,"startpage",
						"Start Page:",str,0,
						0,0,2,2,2,2);
	delete[] str; str=NULL;
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	
	str=numtostr(dpi,0);
	last=linp=new LineInput(this,"DPI",0, 0,0,0,0,0, last,window,"dpi",
						"Default dpi:",str,0,
						0,0,2,2,2,2);
	delete[] str; str=NULL;
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	AddNull();
	
//	str=numtostr(end,0);
//	last=linp=new LineInput(this,"EndPage",0, 0,0,0,0,0, last,window,"endpage",
//						"End Page:",str,0,
//						0,0,2,2,2,2);
//	delete[] str; str=NULL;
//	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
//	AddNull();

	
	 //------------------------------ Images Per Page
	//Images Per Page ____, or "as many as will fit", or "all on 1 page"
		
	mesbar=new MessageBar(this,"perpage",MB_MOVE, 0,0, 0,0, 0, "Images Per Page:");
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"perpageexactly",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpageexactly", "Exactly this many:",5,5);
	check->State(LAX_ON);
	AddWin(check, check->win_w,0,0,50, check->win_h,0,0,50);
	last=linp=new LineInput(this,"NumPerPage",0, 0,0,0,0,0, last,window,"perpageexactlyn",
						NULL,"1",0,
						textheight*10,textheight+4,2,2,2,2);
	AddWin(linp);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"perpagefit",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpagefit", "As many as will fit per page",5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"perpageall",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpageall", "All on one page",5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	
	 //------------------------ preview options ----------------------
	last=linp=new LineInput(this,"PreviewBase",0, 0,0,0,0,0, last,window,"previewbase",
						"Default name for previews:",
						laidout->preview_file_bases.n?laidout->preview_file_bases.e[0]:".laidout.%.jpg",0,
						0,0,2,2,2,2);
	linp->tooltip("For file.jpg,\n"
				  "* gets replaced with the original file name (\"file.jpg\"), and\n"
				  "% gets replaced with the file name without the final suffix (\"file\")\n");
	AddWin(linp);
	MenuInfo *menu=new MenuInfo("Preview Bases");
	for (c=0; c<laidout->preview_file_bases.n; c++) {
		menu->AddItem(laidout->preview_file_bases.e[c],c);
	}
	menub=new MenuButton(this,"PreviewBase",MENUBUTTON_DOWNARROW, 0,0,0,0,0,
									 last,window,"previewbasemenu",0,
									 menu,1,
									 (const char *)NULL,"v");
	menub->tooltip("Select from the available preview bases");
	AddWin(menub);
	AddNull();
	
	last=linp=new LineInput(this,"PreviewWidth",0, 0,0,0,0,0, last,window,"previewwidth",
						"Default max width for new previews:",NULL,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->SetText(laidout->max_preview_length);
	linp->tooltip("Any newly generated previews must fit\nin a square this many pixels wide");
	AddWin(linp);
	AddNull();
	 
	last=check=new CheckBox(this,"autopreview",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"autopreview", "Make previews for files larger than",5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	
	 //----------------- file size to maybe generate previews for 
	last=linp=new LineInput(this,"MinSize",0, 0,0,0,0,0, last,window,"mintopreview",
						" ",NULL,0,
						textheight*6,0,2,2,2,2);
	linp->GetLineEdit()->SetText(laidout->preview_over_this_size);
	linp->tooltip("Previews will be automatically generated \n"
			      "for files over this size, and then only \n"
				  "whenever the generated preview name doesn't\n"
				  "exist already");
	AddWin(linp);
	mesbar=new MessageBar(this,"kb",MB_MOVE, 0,0, 0,0, 0, "kB");
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	AddNull();
	 
	 //-------------------- change Ok to Import
	tbut=dynamic_cast<TextButton *>(findWindow("fd-Ok"));
	if (tbut) tbut->SetName("Import");


	Sync(1);
	
	return 0;
}

int ImportImagesDialog::DataEvent(EventData *data,const char *mes)
{
	if (!strcmp(mes,"usethispreview")) {
		StrsEventData *strs=dynamic_cast<StrsEventData *>(data);
		if (!strs) return 1;

		LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
		preview->SetText(strs->strs[0]+2);
		
		delete data;
		return 0;
	}
	return 1;
}

int ImportImagesDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"perpageexactly") || !strcmp(mes,"perpagefit") || !strcmp(mes,"perpageall")) {
		DBG cout <<"*************** !!!!!"<<mes<<endl;
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
	} else if (!strcmp(mes,"dpi")) {
	} else if (!strcmp(mes,"startpage")) {
	//} else if (!strcmp(mes,"endpage")) {
	} else if (!strcmp(mes,"autopreview")) {
		//***should gray and ungray the previews for over size
	} else if (!strcmp(mes,"mintopreview")) {
	} else if (!strcmp(mes,"previewbase")) {
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
		MenuSelector *popup=new MenuSelector(NULL,menu->title, 
						ANXWIN_BARE|ANXWIN_HOVER_FOCUS,
						0,0,0,0, 1, 
						NULL,window,"usethispreview", 
						MENUSEL_LEFT
						 | MENUSEL_ZERO_OR_ONE|MENUSEL_CURSSELECTS
						 | MENUSEL_FOLLOW_MOUSE|MENUSEL_SEND_ON_UP
						 | MENUSEL_GRAB_ON_MAP|MENUSEL_OUT_CLICK_DESTROYS
						 | MENUSEL_CLICK_UP_DESTROYS|MENUSEL_DESTROY_ON_FOCUS_OFF
					 	 | MENUSEL_SEND_STRINGS,
						menu,1);
		popup->pad=3;
		popup->Select(0);
	//	popup->SetFirst(curitem,x,y); 
		popup->WrapToMouse(None);
		app->rundialog(popup);
		if (popup->window) app->setfocus(popup);
		else { app->destroywindow(popup); popup=NULL; }
		return 0;	
	} else if (!strcmp(mes,"previewbasemenu")) {
		if (e->data.l[0]>=0 && e->data.l[0]<laidout->preview_file_bases.n) {
			LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
			prevbase->SetText(laidout->preview_file_bases.e[e->data.l[0]]);
		}
	} else if (!strcmp(mes,"generate")) {
		 //**** must generate what is in preview for file
	} else if (!strcmp(mes,"new file")) { //sent by the file input on any change
		rebuildPreviewName();
		return 0;
	} else if (!strcmp(mes,"nextfile")) {
		cout <<"*** imp nexfile"<<endl;
	} else if (!strcmp(mes,"prevfile")) {
		cout <<"*** imp prevfile"<<endl;
	}

	if (!FileDialog::ClientEvent(e,mes)) return 0;
	
	return 1;
}

//! Change the Preview input to reflect a new file name.
/*! \todo ****this should check a list of file<->preview, which also says whether
 *     the preview name is still the default, or was custom. custom is kept, but default
 *     is changed according to whatever previewbase is set to...
 *  \todo this should ultimately use a list of preview bases/types, to make either
 *     the freedesktop type previews in ~/.thumbnails, or do what it does now, simply
 *     transform the preview name based on file..
 */
void ImportImagesDialog::rebuildPreviewName()
{
	//linp=dynamic_cast<LineInput *>(findWindow("file"));
	//const char *f=linp->GetCText();
	
	char *full=fullFilePath(NULL);
	LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
	char *prev=previewFileName(full,prevbase->GetCText());
	preview->SetText(prev);
	delete[] full;
	delete[] prev;
}

//! Convert things like "24kb" and "3M" to kb.
/*! "never" gets translated to INT_MAX. "34" becomes 34 kilobytes
 *
 * Return 0 for success or nonzero for unknown.
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
	long l=strtol(str,&e,10);
	if (e==str) {
		delete[] str;
		return 1;
	}
	while (isspace(*e)) e++;
	if (*e) {
		if (!strcasecmp(e,"m")) l*=1024;
		else if (!strcasecmp(e,"g")) l*=1024*1024;
		else if (!strcasecmp(e,"kb")) ;
		else {
			delete[] str;
			return 2;
		}
	}
	delete[] str;
	if (ll) *ll=l;
	return 0;
}

//! Instead of sending the file name(s) to owner, call dump_images directly.
/*! Returns 0 if nothing done. Else nonzero.
 *  
 *  \todo on success, should probably send a status message to whoever called up
 *    this dialog? right now it only knows doc, not a specific location
 *  \todo implement preview base as list of possible preview bases and search
 *    mechanism, that is, by file, or by freedesktop thumbnail spec, kphotoalbum thumbs, etc
 */
int ImportImagesDialog::send()
{
	//if (!owner) return 0;
	
	DBG cout <<"====Generating file names for import images..."<<endl;
	
	int *which=filelist->WhichSelected(LAX_ON);
	int n=0;
	char **imagefiles, **previewfiles;
	if (which!=NULL) {
		n=which[0];
		imagefiles  =new char*[n];
		previewfiles=new char*[n];
		
		const MenuItem *item;
		for (int c=0; c<which[0]; c++) {
			imagefiles[c]=previewfiles[c]=NULL;
			item=filelist->Item(which[c+1]);
			if (item) {
				imagefiles[c]=path->GetText();
				if (imagefiles[c][strlen(imagefiles[c])-1]!='/') appendstr(imagefiles[c],"/");
				appendstr(imagefiles[c],item->name);
			}
		}
		delete[] which;
	} else { 
		 //nothing currently selected in the item list...
		 // so just use the single file in file input
		n=1;
		imagefiles  =new char*[n];
		previewfiles=new char*[n];
		
		char *blah=path->GetText();
		if (blah[strlen(blah)]!='/') appendstr(blah,"/");
		appendstr(blah,file->GetCText());
		if (isblank(blah)) { //******** need better sanity checking here
			delete[] blah;
			deletestrs(imagefiles,1);
			deletestrs(previewfiles,1);
			return 0;
		}
		imagefiles[0]=blah;
	}

	 //generate standard preview files to search for
	LineInput *templi,*prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
	for (int c=0; c<n; c++) {
		previewfiles[c]=previewFileName(imagefiles[c],prevbase->GetCText());
		if (!file_exists(previewfiles[c],1,NULL)) {
			if (dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON) {

				long si,
					 s=file_size(imagefiles[c],1,NULL);
				templi=dynamic_cast<LineInput *>(findWindow("MinSize"));			
				str_to_byte_size(templi->GetCText(), &si);
				if (s>si) {
					DBG cout <<"-=-=-=--=-==-==-=-==-- Generate preview at: "<<previewfiles[c]<<endl;
					si=dynamic_cast<LineInput *>(findWindow("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
					if (si<10) si=128;
					if (generate_preview_image(imagefiles[c],previewfiles[c],si,si,1)) {
						DBG cout <<"              ***generate preview failed....."<<endl;
					}
				}
			} else {
				delete[] previewfiles[c];
				previewfiles[c]=NULL;
			}
		}
	}
	startpage=dynamic_cast<LineInput *>(findWindow("StartPage"))->GetLineEdit()->GetLong(NULL);
	dpi      =dynamic_cast<LineInput *>(findWindow("DPI"))->GetLineEdit()->GetLong(NULL);
	
	int perpage=-2; //force to 1 page
	if (dynamic_cast<CheckBox *>(findWindow("perpageexactly"))->State()==LAX_ON)
		perpage=dynamic_cast<LineInput *>(findWindow("NumPerPage"))->GetLineEdit()->GetLong(NULL);
	else if (dynamic_cast<CheckBox *>(findWindow("perpagefit"))->State()==LAX_ON)
		perpage=-1; //as will fit to page
		
	dumpInImages(doc,startpage,(const char **)imagefiles,(const char **)previewfiles,n,perpage,(int)dpi);
	deletestrs(imagefiles,n);
	deletestrs(previewfiles,n);
			
	return 1;
}

