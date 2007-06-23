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

#include "language.h"
#include "importimages.h"
#include "laidout.h"
#include "extras.h"
#include <lax/checkbox.h>
#include <lax/fileutils.h>
#include <lax/menubutton.h>

#include <lax/lists.cc>

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
	curitem=-1;

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
	tbut->tooltip(_("Jump to previous selected file"));
	AddWin(tbut,c+1);
	tbut=new TextButton(this,"next file",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			tbut,window,"nextfile",
			">",3,3);
	tbut->tooltip(_("Jump to next selected file"));
	AddWin(tbut,c+2);
	
	 //---------------------- insert preview line input
	c=findWindowIndex("path");
	AddWin(new MessageBar(this,"previewm",MB_MOVE, 0,0, 0,0, 0, "Preview: "), c);
	MenuButton *menub=new MenuButton(this,"previewlist",MENUBUTTON_DOWNARROW|MENUBUTTON_CLICK_CALLS_OWNER, 0,0,0,0,0,
									 linp,window,"previewlist",0,
									 NULL,1,
									 (const char *)NULL,"v");
	menub->tooltip(_("Select from possible automatic previews"));
	AddWin(menub,c+1);
	last=linp=new LineInput(this,"preview",
						LINP_FILE, 0,0,0,0,0, last,window,"preview",
						" ",NULL,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->win_style|=LINEEDIT_SEND_ANY_CHANGE;
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50, c+2);
	tbut=new TextButton(this,"generate preview",ANXWIN_CLICK_FOCUS, 0,0,0,0, 1, 
			NULL,window,"generate",
			_("Generate"),3,3);
	tbut->tooltip(_("Generate a preview for file at this location."));
	AddWin(tbut, tbut->win_w,0,50,50, linpheight,0,0,50, c+3);
	AddNull(c+4);
	
	
	 //---------------------- per image preview controls ---------------------------
//	   filename:_____________
//		previewfile:_________
//		[[Re]Generate Preview]
//		use temporary preview
//		description:_________

	 //---------------------- extra image layout controls ---------------------------
	str=numtostr(startpage,0);
	last=linp=new LineInput(this,"StartPage",0, 0,0,0,0,0, last,window,"startpage",
						_("Start Page:"),str,0,
						0,0,2,2,2,2);
	delete[] str; str=NULL;
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	
	str=numtostr(dpi,0);
	last=linp=new LineInput(this,"DPI",0, 0,0,0,0,0, last,window,"dpi",
						_("Default dpi:"),str,0,
						0,0,2,2,2,2);
	delete[] str; str=NULL;
//	virtual int AddWin(anXWindow *win,int npw,int nws,int nwg,int nhalign, int nph,int nhs,int nhg,int nvalign);
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	AddNull();
	
//	str=numtostr(end,0);
//	last=linp=new LineInput(this,"EndPage",0, 0,0,0,0,0, last,window,"endpage",
//						_("End Page:"),str,0,
//						0,0,2,2,2,2);
//	delete[] str; str=NULL;
//	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
//	AddNull();

	
	 //------------------------------ Images Per Page
	//Images Per Page ____, or "as many as will fit", or "all on 1 page"
		
	mesbar=new MessageBar(this,"perpage",MB_MOVE, 0,0, 0,0, 0, _("Images Per Page:"));
	AddWin(mesbar, mesbar->win_w,0,0,50, mesbar->win_h,0,0,50);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"perpageexactly",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpageexactly", _("Exactly this many:"),5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, check->win_h,0,0,50);
	last=linp=new LineInput(this,"NumPerPage",0, 0,0,0,0,0, last,window,"perpageexactlyn",
						NULL,"1",0,
						textheight*10,textheight+4,2,2,2,2);
	AddWin(linp);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"perpagefit",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpagefit", _("As many as will fit per page"),5,5);
	check->State(LAX_ON);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	AddNull();
	
	last=check=new CheckBox(this,"perpageall",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"perpageall", _("All on one page"),5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	AddWin(NULL, 1000,1000,0,50, 0,0,0,50);
	
	 //------------------------ preview options ----------------------
	last=linp=new LineInput(this,"PreviewBase",0, 0,0,0,0,0, last,window,"previewbase",
						_("Default name for previews:"),
						_("any"),0,
						//laidout->preview_file_bases.n?laidout->preview_file_bases.e[0]:".laidout.%.jpg",0,
						0,0,2,2,2,2);
	linp->tooltip(_("For file.jpg,\n"
				  "* gets replaced with the original file name (\"file.jpg\"), and\n"
				  "% gets replaced with the file name without the final suffix (\"file\")\n"));
	AddWin(linp);
	MenuInfo *menu=new MenuInfo("Preview Bases");
	for (c=0; c<laidout->preview_file_bases.n; c++) {
		menu->AddItem(laidout->preview_file_bases.e[c],c);
	}
	menub=new MenuButton(this,"PreviewBase",MENUBUTTON_DOWNARROW, 0,0,0,0,0,
									 last,window,"previewbasemenu",0,
									 menu,1,
									 (const char *)NULL,"v");
	menub->tooltip(_("Select from the available preview bases"));
	AddWin(menub);
	AddNull();
	
	last=linp=new LineInput(this,"PreviewWidth",0, 0,0,0,0,0, last,window,"previewwidth",
						_("Default max width for new previews:"),NULL,0,
						0,0,2,2,2,2);
	linp->GetLineEdit()->SetText(laidout->max_preview_length);
	linp->tooltip(_("Any newly generated previews must fit\nin a square this many pixels wide"));
	AddWin(linp);
	AddNull();
	 
	last=check=new CheckBox(this,"autopreview",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"autopreview", _("Make previews for files larger than"),5,5);
	check->State(LAX_OFF);
	AddWin(check, check->win_w,0,0,50, linpheight,0,0,50);
	
	 //----------------- file size to maybe generate previews for 
	last=linp=new LineInput(this,"MinSize",0, 0,0,0,0,0, last,window,"mintopreview",
						" ",NULL,0,
						textheight*6,0,2,2,2,2);
	linp->GetLineEdit()->SetText(laidout->preview_over_this_size);
	linp->tooltip(_("Previews will be automatically generated \n"
			      "for files over this size, and then only \n"
				  "whenever the generated preview name doesn't\n"
				  "exist already"));
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
		char *full=fullFilePath(NULL);
		ImageInfo *info=findImageInfo(full);
		delete[] full;
		if (info) makestr(info->previewfile,strs->strs[0]+2);
		
		delete data;
		return 0;
	}
	return 1;
}

int ImportImagesDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{
	if (!strcmp(mes,"files")) {
		FileDialog::ClientEvent(e,mes);
		updateFileList();
		rebuildPreviewName();
		return 0;
	} else if (!strcmp(mes,"preview")) {
		 //something's been typed in the preview edit
		LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
		ImageInfo *info=findImageInfo(file->GetCText());
		if (!info) return 0;
		makestr(info->previewfile,preview->GetCText());
		return 0;
	} else if (!strcmp(mes,"new file")) {
		FileDialog::ClientEvent(e,mes);
		char *full=fullFilePath(NULL);
		if (file_exists(full,1,NULL)) updateFileList();
		delete[] full;
		rebuildPreviewName();
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
		//nothing to do
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
		 // must generate what is in preview for file
		cout <<"*** imp ImportImageDialog -> generate!!"<<endl;
	} else if (!strcmp(mes,"new file")) { //sent by the file input on any change
		rebuildPreviewName();
		return 0;
	} else if (!strcmp(mes,"nextfile") || !strcmp(mes,"prevfile")) {
			
		int *which=filelist->WhichSelected(LAX_ON);
		if (!images.n || !which) return 0;
		int c,i;
		if (curitem<1 || curitem>which[0]) {
			c=filelist->Curitem();
			for (i=1; i<=which[0]; i++) 
				if (which[i]==c) break;
			if (i<=which[0]) curitem=i;
			else curitem=1;
		}
		if (!strcmp(mes,"prevfile")) {
			curitem--;
			if (curitem<1) curitem=which[0];
		} else {
			curitem++;
			if (curitem>which[0]) curitem=1;
		}

		 //set current file/path/previewer to c
		const MenuItem *m=filelist->Item(which[curitem]);
		char *full=fullFilePath(m->name);
		ImageInfo *info=findImageInfo(full,&c);
		delete[] full;
		if (info) {
			SetFile(info->file,info->previewfile);
			previewer->Preview(info->file);
		}
		delete[] which;
		return 0;
	}

	if (!FileDialog::ClientEvent(e,mes)) return 0;
	
	return 1;
}

//! Set the file and preview fields, changing directory if need be.
/*! \todo should probably redefine FileDialog::SetFile() also, and check against
 * 		images?
 */
void ImportImagesDialog::SetFile(const char *f,const char *pfile)
{
	FileDialog::SetFile(f);
	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
	preview->SetText(pfile);
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
	//int ifauto=dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON;
	//const char *f=linp->GetCText();
	
	 // find file in list 
	char *full=fullFilePath(NULL);
	ImageInfo *info=findImageInfo(full);
	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
	
	char *prev=NULL;
	if (info) {
		prev=newstr(info->previewfile);
	} 
	if (!prev) prev=getPreviewFileName(full);
	if (!prev) prev=newstr("");
	preview->SetText(prev);
	delete[] full;
	delete[] prev;
}

//! Create new ImageInfo nodes for any selected files not in the list.
/*! \todo this adds nodes to images. should put them in sorted to speed up checking?
 */
void ImportImagesDialog::updateFileList()
{
	curitem=-1;
	int *which=filelist->WhichSelected(LAX_ON);
	if (!which) return;
	const MenuItem *item;
	ImageInfo *info;
	char *full;
	for (int c=0; c<which[0]; c++) {
		item=filelist->Item(which[c+1]);
		
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
	delete[] which;
}
	
//! Convert things like "24kb" and "3M" to kb.
/*! "never" gets translated to INT_MAX. "34" becomes 34 kilobytes
 *
 * Return 0 for success or nonzero for unknown.
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

//! Return the Laxkit::ImageInfo in images with fullfile as the path.
Laxkit::ImageInfo *ImportImagesDialog::findImageInfo(const char *fullfile,int *i)
{
	int c;
	char *full;
	for (c=0; c<images.n; c++) {
		full=fullFilePath(fullfile);
		if (!strcmp(full,images.e[c]->file)) {
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
 */
int ImportImagesDialog::send()
{
	//if (!owner) return 0;
	
	DBG cerr <<"====Generating file names for import images..."<<endl;
	
	int *which=filelist->WhichSelected(LAX_ON);
	int n=0;
	char **imagefiles=NULL, **previewfiles=NULL;
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
		char *blah=path->GetText();
		if (blah[strlen(blah)]!='/') appendstr(blah,"/");
		appendstr(blah,file->GetCText());
		if (isblank(blah)) { //******** need better sanity checking here
			delete[] blah;
			return 0;
		}
		
		n=1;
		imagefiles  =new char*[n];
		previewfiles=new char*[n];
		imagefiles[0]=blah;
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
					DBG cerr <<"-=-=-=--=-==-==-=-==-- Generate preview at: "<<previewfiles[c]<<endl;
					si=dynamic_cast<LineInput *>(findWindow("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
					if (si<10) si=128;
					if (generate_preview_image(imagefiles[c],previewfiles[c],"jpg",si,si,1)) {
						DBG cerr <<"              ***generate preview failed....."<<endl;
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
		for (c=0; c<laidout->preview_file_bases.n; c++) {
			prev=previewFileName(full,laidout->preview_file_bases.e[c]);
			if (file_exists(prev,1,NULL)==S_IFREG) {
				break;
			}
		}
		if (c==laidout->preview_file_bases.n) {
			if (laidout->preview_file_bases.n) prev=previewFileName(full,laidout->preview_file_bases.e[0]);
			else prev=previewFileName(full,".laidout-*.jpg");
		}
	} else prev=previewFileName(full,prevbase->GetCText());

	return prev;
}

