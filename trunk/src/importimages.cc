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

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


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

	 //---------------------- insert preview line input
	linp=dynamic_cast<LineInput *>(findWindow("file"));
	linp->GetLineEdit()->win_style|=LINEEDIT_SEND_ANY_CHANGE;
	c=findWindowIndex("path");
	last=linp=new LineInput(this,"preview",LINP_FILE, 0,0,0,0,0, last,window,"preview",
						"Preview:",NULL,0,
						0,0,2,2,2,2);
	//makestr(linp->GetLineEdit()->qualifier,****);
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50, c);
	tbut=new TextButton(this,"generate preview",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			NULL,window,"generate",
			"Generate",3,3);
	tbut->tooltip("Generate a preview for file at this location.");
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50, c+1);
	AddNull(c+2);
	
	
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
						"Default name for previews:",laidout->preview_file_base,0,
						0,0,2,2,2,2);
	linp->tooltip("For file.jpg,\n"
				  "* gets replaced with the original file name (\"file.jpg\"), and\n"
				  "% gets replaced with the file name without the final suffix (\"file\")\n");
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
	} else if (!strcmp(mes,"perpageexactlyn")) {
	} else if (!strcmp(mes,"dpi")) {
	} else if (!strcmp(mes,"startpage")) {
	//} else if (!strcmp(mes,"endpage")) {
	} else if (!strcmp(mes,"autopreview")) {
		//***should gray and ungray the previews for over size
	} else if (!strcmp(mes,"mintopreview")) {
	} else if (!strcmp(mes,"previewbase")) {
	} else if (!strcmp(mes,"new file")) { //sent by the file input on any change
		rebuildPreviewName();
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

//! Instead of sending the file name(s) to owner, call dump_images directly.
/*! Returns 0 if nothing done. Else nonzero.
 *  
 *  \todo on success, should probably send a status message to whoever called up
 *    this dialog? right now it only knows doc, not a specific location
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
		if (!strcmp(blah,"")) { //***** should sanity check better! whitespace not allowed!
			delete[] blah;
			deletestrs(imagefiles,1);
			deletestrs(previewfiles,1);
			return 0;
		}
		imagefiles[0]=blah;
	}

	 //generate standard preview files to search for
	LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
	for (int c=0; c<n; c++) {
		previewfiles[c]=previewFileName(imagefiles[c],prevbase->GetCText());
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

