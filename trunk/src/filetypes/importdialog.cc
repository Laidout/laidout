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

#include "importdialog.h"
#include "../language.h"
#include "../laidout.h"
#include "../extras.h"
#include "../utils.h"
#include <lax/checkbox.h>
#include <lax/fileutils.h>
#include <lax/menubutton.h>

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


/*! \class ImportFileDialog
 * \brief Dialog for importing many images all at once.
 */

/*! Only one of obj or doc should be defined. If obj is defined, then dump all the selected images
 * to obj within obj's bounds. Otherwise dump to doc with the full per page settings.
 *
 * \todo default dpi?
 */
ImportFileDialog::ImportFileDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			Window nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Group *obj,
			Document *ndoc,int startpg,double defdpi)
	: FileDialog(parnt,ntitle,
			(nstyle&0xffff)|FILES_PREVIEW|FILES_OPEN_MANY|ANXWIN_REMEMBER,
			xx,yy,ww,hh,brder,nowner,nsend,nfile,npath,nmask)
{
	startpage=startpg;
	toobj=obj;
	if (toobj) toobj->inc_count();
	doc=ndoc;
	if (doc) doc->inc_count();
	curitem=-1;

	dpi=defdpi;
	if (dpi<=0) {
		if (doc) dpi=doc->docstyle->imposition->paper->paperstyle->dpi;
		else dpi=300;//***
	}

	dialog_style|=FILES_PREVIEW;
}

ImportFileDialog::~ImportFileDialog()
{
	if (toobj) toobj->dec_count();
	if (doc)   doc->dec_count();
}

void ImportFileDialog::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	Attribute *att=dump_out_atts(NULL,0,context);
	att->dump_out(f,indent);
	delete att;
}

/*! Append to att if att!=NULL, else return new att.
 */
Attribute *ImportFileDialog::dump_out_atts(Attribute *att,int what,Laxkit::anObject *context)
{
	if (!att) att=new Attribute("ImportFileDialog",NULL);
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

//	 //------------export settings
//	int dpi      =dynamic_cast<LineInput *>(findWindow("DPI"))->GetLineEdit()->GetLong(NULL);
//	int perpage=-2; //force to 1 page
//	if (dynamic_cast<CheckBox *>(findWindow("perpageexactly"))->State()==LAX_ON)
//		perpage=dynamic_cast<LineInput *>(findWindow("NumPerPage"))->GetLineEdit()->GetLong(NULL);
//	else if (dynamic_cast<CheckBox *>(findWindow("perpagefit"))->State()==LAX_ON)
//		perpage=-1; //as will fit to page
//
//	 //dpi
//	sprintf(scratch,"%d",dpi);
//	att->push("dpi",scratch);
//
//	 //autopreview
//	att->push("autopreview",
//				(dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON)?"yes":"no");
//
//	 //perPage
//	if (perpage==-2) att->push("perPage","all");
//	else if (perpage==-1) att->push("perPage","fit");
//	else att->push("perPage",perpage,-1);
//
//	 //maxPreviewWidth 200
//	int w=dynamic_cast<LineInput *>(findWindow("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
//	att->push("maxPreviewWidth",w,-1);
//
//	 //defaultPreviewName any
//	LineInput *prevbase=dynamic_cast<LineInput *>(findWindow("PreviewBase"));
//	att->push("defaultPreviewName",prevbase->GetCText());

	return att;
}

/*! \todo  *** ensure that the dimensions read in are in part on screen...
 */
void ImportFileDialog::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
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
int ImportFileDialog::init() 
{
	dialog_style|=FILES_PREVIEW;
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
	
	anXWindow *last=NULL;
	LineInput *linp=NULL;
	CheckBox *check=NULL;
	TextButton *tbut=NULL;


	
	
	last=NULL;

	 //--------------------- File Info message bar
	fileinfo=new MessageBar(this,"perpage",MB_MOVE, 0,0, 0,0, 0, _("No file selected"));
	AddWin(fileinfo, 200,100,1000,50, linpheight,0,0,50);
	AddNull();
	AddWin(NULL, 200,100,3000,50, linpheight/2,0,0,50);
	AddNull();

	 //---------------------- import from file ---------------------------

	last=importpagerange=new LineInput(this,"importpagerange",0, 0,0,0,0,0, 
						last,window,"importpagerange",
						_("Import page range:"),_("all"),0,
						0,0,2,2,2,2);
	importpagerange->tooltip(_("The pages of the file to import"));
	AddWin(importpagerange, importpagerange->win_w,0,1000,50, importpagerange->win_h,0,0,50);
	AddNull();
	

	 //---------------------- import to document or object ---------------------------
	str=numtostr(startpage,0);
	last=linp=new LineInput(this,"StartIndex",0, 0,0,0,0,0, 
						last,window,"startindex",
						_("Start Index:"),str,0,
						0,0,2,2,2,2);
	delete[] str; str=NULL;
	linp->tooltip(_("The document page to start importing into"));
	AddWin(linp,200,100,1000,50, linp->win_h,0,0,50);
	AddNull();

	
	last=check=new CheckBox(this,"keepmystery",ANXWIN_CLICK_FOCUS|CHECK_LEFT, 0,0,0,0,1, 
						last,window,"keepmystery", _("Preserve mystery data"),5,5);
	check->tooltip(_("Try to preserve data that Laidout does not understand"));
	check->State(LAX_ON);
	AddWin(check, check->win_w,0,3000,50, linpheight,0,0,50);
	AddNull();




	 //-------------------- change Ok to Import
	tbut=dynamic_cast<TextButton *>(findWindow("fd-Ok"));
	if (tbut) tbut->SetName(_("Import"));


	Sync(1);
	
	return 0;
}

int ImportFileDialog::DataEvent(EventData *data,const char *mes)
{//***
	return 1;
}

int ImportFileDialog::ClientEvent(XClientMessageEvent *e,const char *mes)
{//***
	if (!strcmp(mes,"new file") || !strcmp(mes,"files")) {
		 //when a new file name is typed or selected
		FileDialog::ClientEvent(e,mes);

		if (!file) return 0;
		char *filename=fullFilePath(NULL);
		if (isblank(filename)) {
			file->GetLineEdit()->Valid(1);
			delete[] filename;
			return 0;
		}

		 //check file type
		int c;
		FILE *f=fopen(filename,"r");
		if (!f) return 0;
		char first100[200];
		c=fread(first100,1,200,f);
		fclose(f);

		const char *filtertype=NULL;
		ImportFilter *filter=NULL;
		for (c=0; c<laidout->importfilters.n; c++) {
			filtertype=laidout->importfilters.e[c]->FileType(first100);
			if (filtertype) {
				filter=laidout->importfilters.e[c];
				break;
			}
		}
		if (filtertype) {
			file->GetLineEdit()->Valid(1);
			fileinfo->SetText(filename);
		} else {
			file->GetLineEdit()->Valid(0);
			fileinfo->SetText(_("File cannot be imported."));
		}
		delete[] filename;
		return 0;
	}

	if (!FileDialog::ClientEvent(e,mes)) return 0;
	
	return 1;
}

//! Set the file and preview fields, changing directory if need be.
/*! \todo should probably redefine FileDialog::SetFile() also to check against
 * 		images?
 */
void ImportFileDialog::SetFile(const char *f,const char *pfile)
{
	FileDialog::SetFile(f); //sets file and path
	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
	preview->SetText(pfile);
}

////! Change the Preview input to reflect a new file name.
//void ImportFileDialog::rebuildPreviewName()
//{//***
//	DBG cerr <<"ImportFileDialog::rebuildPreviewName()"<<endl;
//
//	//int ifauto=dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON;
//	//const char *f=linp->GetCText();
//	
//	 // find file in list 
//	char *full=fullFilePath(NULL);
//	ImageInfo *info=findImageInfo(full);
//	LineInput *preview= dynamic_cast<LineInput *>(findWindow("preview"));
//	DBG if (!preview) { cerr <<"*********rebuildPreviewName ERROR!!"<<endl; exit(1); }
//	
//	char *prev=NULL;
//	if (info) prev=newstr(info->previewfile);
//	if (!prev) prev=getPreviewFileName(full);
//	if (!prev) prev=newstr("");
//	preview->SetText(prev);
//	delete[] full;
//	delete[] prev;
//	DBG cerr <<"...done with ImportFileDialog::rebuildPreviewName()"<<endl;
//}

//! Return the Laxkit::ImageInfo in images with fullfile as the path.
/*! If i!=NULL, then set *i=(index of the imageinfo), -1 if not found.
 */
Laxkit::ImageInfo *ImportFileDialog::findImageInfo(const char *fullfile,int *i)
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

//! Call import_file() with proper config.
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
int ImportFileDialog::send(int id)
{
	return 1;
	//if (!owner) return 0;
	
//	DBG cerr <<"====Generating file names for import images..."<<endl;
//	
//	int *which=filelist->WhichSelected(LAX_ON);
//	int n=0;
//	char **imagefiles=NULL, **previewfiles=NULL;
//	if (which!=NULL) {
//		n=0;
//		imagefiles  =new char*[which[0]];
//		previewfiles=new char*[which[0]];
//		
//		const MenuItem *item;
//		for (int c=0; c<which[0]; c++) {
//			imagefiles[n]=previewfiles[n]=NULL;
//			item=filelist->Item(which[c+1]);
//			if (item) {
//				imagefiles[n]=path->GetText();
//				if (imagefiles[n][strlen(imagefiles[n])-1]!='/') appendstr(imagefiles[n],"/");
//				appendstr(imagefiles[n],item->name);
//				if (is_bitmap_image(imagefiles[n])) {
//					n++;
//				} else {
//					delete[] imagefiles[n];
//				}
//			}
//		}
//		if (!n) {
//			delete[] imagefiles;   imagefiles=NULL;
//			delete[] previewfiles; previewfiles=NULL;
//		}
//		delete[] which;
//	} else { 
//		 //nothing currently selected in the item list...
//		 // so just use the single file in file input
//		char *blah=path->GetText();
//		if (blah[strlen(blah)]!='/') appendstr(blah,"/");
//		appendstr(blah,file->GetCText());
//		if (isblank(blah)) {
//			delete[] blah;
//		} else {
//			if (is_bitmap_image(blah)) {
//				n=1;
//				imagefiles  =new char*[n];
//				previewfiles=new char*[n];
//				imagefiles[0]=blah;
//				previewfiles[0]=NULL;
//			} else {
//				delete[] blah; blah=NULL;
//				n=0;
//			}
//		}
//
//	}
//	if (!n) {
//		cout <<"***need to implement importimagedialog send message to viewport or message saying no go."<<endl;
//		return 0;
//	}
//
//	 //generate standard preview files to search for
//	LineInput *templi;
//	ImageInfo *info;
//	for (int c=0; c<n; c++) {
//		if (!imagefiles[c]) continue;
//		if (file_exists(imagefiles[c],1,NULL)!=S_IFREG) {
//			delete[] imagefiles[c];
//			imagefiles[c]=NULL;
//			continue;
//		}
//		info=findImageInfo(imagefiles[c]);
//		if (info) previewfiles[c]=newstr(info->previewfile);
//			else  previewfiles[c]=getPreviewFileName(imagefiles[c]);
//		if (!file_exists(previewfiles[c],1,NULL)) {
//			if (dynamic_cast<CheckBox *>(findWindow("autopreview"))->State()==LAX_ON) {
//
//				long si,
//					 s=file_size(imagefiles[c],1,NULL);
//				templi=dynamic_cast<LineInput *>(findWindow("MinSize"));			
//				str_to_byte_size(templi->GetCText(), &si);
//				if (s>si) {
//					DBG cerr <<"-=-=-=--=-==-==-=-==-- Generate preview at: "<<previewfiles[c]<<endl;
//					si=dynamic_cast<LineInput *>(findWindow("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
//					if (si<10) si=128;
//					if (generate_preview_image(imagefiles[c],previewfiles[c],"jpg",si,si,1)) {
//						DBG cerr <<"              ***generate preview failed....."<<endl;
//					}
//				}
//			} else {
//				delete[] previewfiles[c];
//				previewfiles[c]=NULL;
//			}
//		}
//	}
//	startpage=dynamic_cast<LineInput *>(findWindow("StartPage"))->GetLineEdit()->GetLong(NULL);
//	dpi      =dynamic_cast<LineInput *>(findWindow("DPI"))->GetLineEdit()->GetLong(NULL);
//	
//	int perpage=-2; //force to 1 page
//	if (dynamic_cast<CheckBox *>(findWindow("perpageexactly"))->State()==LAX_ON)
//		perpage=dynamic_cast<LineInput *>(findWindow("NumPerPage"))->GetLineEdit()->GetLong(NULL);
//	else if (dynamic_cast<CheckBox *>(findWindow("perpagefit"))->State()==LAX_ON)
//		perpage=-1; //as will fit to page
//		
//	dumpInImages(doc,startpage,(const char **)imagefiles,(const char **)previewfiles,n,perpage,(int)dpi);
//	deletestrs(imagefiles,n);
//	deletestrs(previewfiles,n);
//			
//	return 1;
}

