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
// Copyright (c) 2008-2009 Tom Lechner
//

#include "importdialog.h"
#include "../language.h"
#include "../laidout.h"
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
 * \brief Dialog for importing a single, primarily vector based file.
 *
 * The file will be broken down into Laidout elements as possible, via import_document().
 */

/*! Only one of obj or doc should be defined. If obj is defined, then dump all the selected images
 * to obj within obj's bounds. Otherwise dump to doc with the full per page settings.
 *
 * \todo default dpi?
 */
ImportFileDialog::ImportFileDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			unsigned long nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Group *obj,
			Document *ndoc,int startpg,double defdpi)
	: FileDialog(parnt,nname,ntitle,
			(nstyle&0xffff)|ANXWIN_REMEMBER,
			xx,yy,ww,hh,brder,nowner,nsend,
			FILES_PREVIEW|FILES_OPEN_MANY,
			nfile,npath,nmask)
{
	if (defdpi<=0) {
		if (ndoc) defdpi=ndoc->imposition->paper->paperstyle->dpi; //***<-- crash hazard!
		else defdpi=300;//***
	}

	config=new ImportConfig(nfile, defdpi, startpg, startpg, -1, -1, -1, ndoc, obj);
	config->keepmystery=1;
	//config->inend=3;//****TEMP FOR TESTING!!

	dialog_style|=FILES_PREVIEW;
}

ImportFileDialog::~ImportFileDialog()
{
	if (config) config->dec_count();
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
//	int dpi      =dynamic_cast<LineInput *>(findChildWindowByName("DPI"))->GetLineEdit()->GetLong(NULL);
//	int perpage=-2; //force to 1 page
//	if (dynamic_cast<CheckBox *>(findChildWindowByName("perpageexactly"))->State()==LAX_ON)
//		perpage=dynamic_cast<LineInput *>(findChildWindowByName("NumPerPage"))->GetLineEdit()->GetLong(NULL);
//	else if (dynamic_cast<CheckBox *>(findChildWindowByName("perpagefit"))->State()==LAX_ON)
//		perpage=-1; //as will fit to page
//
//	 //dpi
//	sprintf(scratch,"%d",dpi);
//	att->push("dpi",scratch);
//
//	 //autopreview
//	att->push("autopreview",
//				(dynamic_cast<CheckBox *>(findChildWindowByName("autopreview"))->State()==LAX_ON)?"yes":"no");
//
//	 //perPage
//	if (perpage==-2) att->push("perPage","all");
//	else if (perpage==-1) att->push("perPage","fit");
//	else att->push("perPage",perpage,-1);
//
//	 //maxPreviewWidth 200
//	int w=dynamic_cast<LineInput *>(findChildWindowByName("PreviewWidth"))->GetLineEdit()->GetLong(NULL);
//
//	ImportConfig *config=new ImportConfig();
//
//	att->push("maxPreviewWidth",w,-1);
//
//	 //defaultPreviewName any
//	LineInput *prevbase=dynamic_cast<LineInput *>(findChildWindowByName("PreviewBase"));
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

/*! 
 *  \todo maybe add "import pages as objects" option?
 */
int ImportFileDialog::init() 
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
	char *str=NULL;
	
	anXWindow *last=NULL;
	LineInput *linp=NULL;
	CheckBox *check=NULL;
	//TextButton *tbut=NULL;


	
	
	last=NULL;

	 //--------------------- File Info message bar
	fileinfo=new MessageBar(this,"perpage",NULL,MB_MOVE, 0,0, 0,0, 0, _("No file selected"));
	AddWin(fileinfo,1, 200,100,1000,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	AddWin(NULL,0, 200,100,3000,50,0, linpheight/2,0,0,50,0, -1);
	AddNull();

	 //---------------------- import from file ---------------------------

	last=importpagerange=new LineInput(this,"importpagerange",NULL,0, 0,0,0,0,0, 
						last,object_id,"importpagerange",
						_("Import page range:"),_("all"),0,
						0,0,2,2,2,2);
	importpagerange->tooltip(_("The range of pages of the file to import, numbered from 0.\n"
							   "Such as \"3-9\". Currently, only one range per import"));
	AddWin(importpagerange,1, importpagerange->win_w,0,1000,50,0, importpagerange->win_h,0,0,50,0, -1);
	AddNull();
	

	 //---------------------- import to document or object ---------------------------
	str=numtostr(config->topage,0);
	last=linp=new LineInput(this,"StartIndex",NULL,0, 0,0,0,0,0, 
						last,object_id,"startindex",
						_("Start Index:"),str,0,
						0,0,2,2,2,2);
	delete[] str; str=NULL;
	linp->tooltip(_("The document page to start importing into"));
	AddWin(linp,1, 200,100,1000,50,0, linp->win_h,0,0,50,0, -1);
	AddNull();

	
	 //---------------------- MysteryData --------------------------------
	last=check=new CheckBox(this,"ignoremystery",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"ignoremystery", _("Ignore mystery data"),5,5);
	check->tooltip(_("Ignore anything Laidout doesn't understand"));
	check->State(config->keepmystery==0?LAX_ON:LAX_OFF);
	AddWin(check,1, check->win_w,0,3000,50,0, linpheight,0,0,50,0, -1);
	AddNull();

	last=check=new CheckBox(this,"keepmostmystery",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"keepmostmystery", _("Keep mystery data as necessary"),5,5);
	check->tooltip(_("Use mystery data for things Laidout cannot convert"));
	check->State(config->keepmystery==1?LAX_ON:LAX_OFF);
	AddWin(check,1, check->win_w,1,3000,50,0, linpheight,0,0,50,0, -1);
	AddNull();

	last=check=new CheckBox(this,"keepallmystery",NULL,CHECK_LEFT, 0,0,0,0,1, 
						last,object_id,"keepallmystery", _("All objects are mystery data"),5,5);
	check->tooltip(_("Do not convert any object to native Laidout objects"));
	check->State(config->keepmystery==2?LAX_ON:LAX_OFF);
	AddWin(check,1, check->win_w,0,3000,50,0, linpheight,0,0,50,0, -1);
	AddNull();



	 //-------------------- change Ok to Import
	OkButton(_("Import"), NULL); 


	Sync(1);
	
	return 0;
}

int ImportFileDialog::Event(const EventData *e,const char *mes)
{//***
	if (!strcmp(mes,"new file") || !strcmp(mes,"files")) {
		 //when a new file name is typed or selected
		FileDialog::Event(e,mes);

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
			makestr(config->filename,filename);
			//if (config->filter) config->filter->dec_count();
			config->filter=filter;
			//config->filter->inc_count();
			ok->Grayed(0);
		} else {
			file->GetLineEdit()->Valid(0);
			fileinfo->SetText(_("File cannot be imported."));
			ok->Grayed(1);
		}
		delete[] filename;
		return 0;

	} else if (!strcmp(mes,"startindex")) {
		 //the starting page of the document to import to
		return 0;

	} else if (!strcmp(mes,"ignoremystery")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		CheckBox *none=dynamic_cast<CheckBox *>(findChildWindowByName("ignoremystery"));
		CheckBox *all=dynamic_cast<CheckBox *>(findChildWindowByName("keepallmystery"));
		CheckBox *most=dynamic_cast<CheckBox *>(findChildWindowByName("keepmostmystery"));
		config->keepmystery=0;
		if (s->info1==LAX_ON) {
			most->State(LAX_OFF);
			all->State(LAX_OFF);
		} else {
			none->State(LAX_ON);
		}
		return 0;

	} else if (!strcmp(mes,"keepmostmystery")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		CheckBox *all=dynamic_cast<CheckBox *>(findChildWindowByName("keepallmystery"));
		CheckBox *most=dynamic_cast<CheckBox *>(findChildWindowByName("keepmostmystery"));
		CheckBox *none=dynamic_cast<CheckBox *>(findChildWindowByName("ignoremystery"));
		config->keepmystery=1;
		if (s->info1==LAX_ON) {
			none->State(LAX_OFF);
			all->State(LAX_OFF);
		} else {
			most->State(LAX_ON);
		}
		return 0;

	} else if (!strcmp(mes,"keepallmystery")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		CheckBox *none=dynamic_cast<CheckBox *>(findChildWindowByName("ignoremystery"));
		CheckBox *most=dynamic_cast<CheckBox *>(findChildWindowByName("keepmostmystery"));
		CheckBox *all=dynamic_cast<CheckBox *>(findChildWindowByName("keepallmystery"));
		config->keepmystery=2;
		if (s->info1==LAX_ON) {
			most->State(LAX_OFF);
			none->State(LAX_OFF);
		} else {
			all->State(LAX_ON);
		}
		return 0;

	}

	return FileDialog::Event(e,mes);
}

////! Set the file and preview fields, changing directory if need be.
///*! \todo should probably redefine FileDialog::SetFile() also to check against
// * 		images?
// */
//void ImportFileDialog::SetFile(const char *f,const char *pfile)
//{
//	FileDialog::SetFile(f); //sets file and path
//	LineInput *preview= dynamic_cast<LineInput *>(findChildWindowByName("preview"));
//	preview->SetText(pfile);
//}

//! Call import_file() with proper config.
/*! Returns 0 if nothing done. Else nonzero.
 *  
 *  \todo *** there should really be a global error log that users can peruse
 *     if necessary
 */
int ImportFileDialog::send(int id)
{
	const char *range=importpagerange->GetCText();
	if (!strcasecmp(range,_("all"))) {
		config->instart=0;
		config->inend=-1;
	} else {
		char *tmp;
		int s,e;
		s=strtol(range,&tmp,10);
		if (tmp==range) {
			s=0;
			e=-1;
		} else {
			range=tmp;
			e=strtol(range,&tmp,10);
			if (tmp==range) {
				e=s;
			}
		}
		config->instart=s;
		config->inend=e;
	}



	ErrorLog log;
	int err=import_document(config,log);

	if (log.Total()) {
		dumperrorlog("ImportFile error return:",log);
	}

	if (err>0) {
		 //send back error;
		if (win_owner) app->SendMessage(new StrEventData(_("Error importing file."),0,0,0,0),
										  win_owner,"statusMessage",object_id);
	}
	if (err<=0) {
		if (win_owner) app->SendMessage(new StrEventData(_("File imported."),0,0,0,0),
										  win_owner,"statusMessage",object_id);

	}

	return 1;
}

