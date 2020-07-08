//
//	
// Laidout, for laying out
// Copyright (C) 2004-2012 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//


#include <cups/cups.h>

#include <lax/sliderpopup.h>
#include <lax/simpleprint.h>
#include <lax/messagebar.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/laxutils.h>

#include "../version.h"
#include "../laidout.h"
#include "../language.h"
#include "../ui/metawindow.h"
#include "exportdialog.h"


#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;



namespace Laidout {


//---------------------------- ConfigEventData -------------------------
/*! \class ConfigEventData
 * \brief Class to transport a DocumentExportConfig.
 */


/*! Incs count on config.
 */
ConfigEventData::ConfigEventData(DocumentExportConfig *c)
{
	config=c;
	if (config) config->inc_count();
}

/*! Decs count on config.
 */
ConfigEventData::~ConfigEventData()
{
	if (config) config->dec_count();
}

//---------------------------- ExportDialog -------------------------
/*! \class ExportDialog
 * \brief Dialog to edit an export section. Sends a DocumentExportConfig object.
 *
 * \todo should allow alternate imposition, notably PosterImposition
 * \todo allow remember settings, and when popped up, bring up last one accessed..
 * \todo overwrite protection
 * \todo allow selecting alternate imposition (with preview?) as well as layout type..
 */


/*! Controls are added here, rather than waiting for init().
 */
ExportDialog::ExportDialog(unsigned long nstyle,unsigned long nowner,const char *nsend,
						   Document *doc,
						   Group *limbo,
						   PaperGroup *papergroup,
						   ExportFilter *nfilter,
						   const char *file, 
						   int layout, //!< Type of layout to export
						   int pmin, //!< The minimum of the range
						   int pmax, //!< The maximum of the range
						   int pcur) //!< The current element of the range
	: RowFrame(NULL,NULL,_("Export"),
			   (nstyle&ANXWIN_MASK)|ROWFRAME_ROWS|ROWFRAME_TOP|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
			   0,0,0,0,0,
			   NULL,nowner,nsend, 5)
{
	dialog_style = nstyle & ~ANXWIN_MASK;

	filter = nfilter;
	if (filter) {
		config=filter->CreateConfig(NULL);
		config->layout = layout;
		config->start  = pmin;
		config->end    = pmax;
		makestr(config->filename, file);

		if (doc!=config->doc) {
			if (config->doc) config->doc->dec_count();
			config->doc = doc;
			if (config->doc) config->doc->inc_count(); 
		}
		if (limbo!=config->limbo) {
			if (config->limbo) config->limbo->dec_count();
			config->limbo = limbo;
			if (config->limbo) config->limbo->inc_count(); 
		}
		if (papergroup!=config->papergroup) {
			if (config->papergroup) config->papergroup->dec_count();
			config->papergroup = papergroup;
			if (config->papergroup) config->papergroup->inc_count(); 
		}

	} else config=new DocumentExportConfig(doc,limbo,file,NULL,layout,pmin,pmax,papergroup);

	cur = pcur;

	fileedit = filesedit = printstart = printend = command = NULL;
	filecheck = filescheck = commandcheck = printall = printcurrent = printrange = NULL;

	batches     = NULL;
	batchnumber = NULL;

	everyspread = evenonly = oddonly = NULL;

	firstextra = -1;

	last_meta_dir = nullptr;
}

/*! Decs count of config.
 */
ExportDialog::~ExportDialog()
{
	if (config) config->dec_count();
	delete[] last_meta_dir;
}

//! Set win_w and win_h to sane values if necessary.
int ExportDialog::preinit()
{
	anXWindow::preinit();
	if (win_w==0) win_w=500;
	if (win_h==0) {
		int textheight = win_themestyle->normal->textheight();
		win_h=20*(textheight+7)+20;
	}

	if (!filter && laidout->exportfilters.n) {
		for (int c=0; c<laidout->exportfilters.n; c++) {
			if (!strcmp(laidout->exportfilters.e[c]->Format(),"Pdf")) {
				filter=laidout->exportfilters.e[c];
				break;
			}
		}
		if (!filter) filter=laidout->exportfilters.e[0];

		 //update config to new filter
		DocumentExportConfig *nconfig=filter->CreateConfig(config);
		if (config) config->dec_count();
		config=nconfig;
	}

	return 0;
}

/*! Append to att if att!=NULL, else return new att.
 */
Attribute *ExportDialog::dump_out_atts(Attribute *att,int what, LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	char scratch[100];

	sprintf(scratch,"%d",win_x);
	att->push("win_x",scratch);

	sprintf(scratch,"%d",win_y);
	att->push("win_y",scratch);

	sprintf(scratch,"%d",win_w);
	att->push("win_w",scratch);

	sprintf(scratch,"%d",win_h);
	att->push("win_h",scratch);

	Attribute *att2 = att->pushSubAtt("last_format", filter->VersionName());
	config->dump_out_atts(att2, what, context);

	if (last_meta_dir) att->push("last_meta_dir", last_meta_dir);

	if (previous_configs.n()) {
		Attribute *att2 = att->pushSubAtt("previous_configs");
		for (int c=0; c<previous_configs.n(); c++) {
			DocumentExportConfig *config = dynamic_cast<DocumentExportConfig*>(previous_configs.e(c));
			if (!config->filter) continue; //this shouldn't happen
			
			Attribute *att3 = att2->pushSubAtt("config", config->filter->VersionName());
			config->dump_out_atts(att3, what, context);
		}
	}

	return att;
}

/*! \todo  *** ensure that the dimensions read in are in part on screen...
 */
void ExportDialog::dump_in_atts(Attribute *att,int flag, LaxFiles::DumpContext *context)
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

		} else if (!strcmp(name,"last_format") && !isblank(value)) {
			ExportFilter *nfilter=NULL;
			for (int c2=0; c2<laidout->exportfilters.n; c2++) {
				if (!strcmp(value, laidout->exportfilters.e[c2]->VersionName())) {
					nfilter=laidout->exportfilters.e[c2];
					break;
				}
			}

			 //update config to new filter
			if (nfilter != filter) {
				filter = nfilter;
				DocumentExportConfig *nconfig = filter->CreateConfig(config);
				if (config) config->dec_count();
				config = nconfig;
			} 

			char *oldfile = newstr(config->filename);
			char *oldfiles = newstr(config->tofiles);
			config->dump_in_atts(att->attributes.e[c], flag, context);
			makestr(config->filename, oldfile);
			makestr(config->tofiles, oldfiles);
			delete[] oldfile;
			delete[] oldfiles;

		} else if (!strcmp(name,"previous_configs")) {
			for (int c2 = 0; c2 < att->attributes.e[c]->attributes.n; c2++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name, "config")) {
					ExportFilter *nfilter = nullptr;
					for (int cc2=0; cc2<laidout->exportfilters.n; cc2++) {
						if (!strcmp(value, laidout->exportfilters.e[cc2]->VersionName())) {
							nfilter = laidout->exportfilters.e[cc2];
							break;
						}
					}

					 //update config to new filter
					if (nfilter) {
						DocumentExportConfig *nconfig = nfilter->CreateConfig(nullptr);
						nconfig->dump_in_atts(att->attributes.e[c]->attributes.e[c2], flag, context);
						previous_configs.push(value, nconfig);
						nconfig->dec_count();
					}
				}
			}

		} else if (!strcmp(name,"last_meta_dir")) {
			makestr(last_meta_dir, value);
		}
	}
}

//! Based on config->layout, set min and max accordingly.
void ExportDialog::findMinMax()
{
	min=0;
	if (config->doc) max=config->doc->imposition->NumSpreads(config->layout)-1;
		else max=0;
}

//! Make sure the kid windows have this as owner.
int ExportDialog::init() 
{
	findMinMax();

	anXWindow *last=NULL;
	Button *tbut=NULL;
	int c;
	double linpheight = 1.2 * win_themestyle->normal->textheight();
	double textheight = win_themestyle->normal->textheight();
	double CHECKGAP = textheight/4;

	padinset = linpheight/2;

	if (!config->filename) {
		config->filename=newstr("output");
		appendstr(config->filename,".");
		appendstr(config->filename,filter->DefaultExtension());
	} else {
		 //make sure template has extension
		if (!strrchr(config->filename,'.')) appendstr(config->filename,".huh");
	}

	if (!config->tofiles) {
		makestr(config->tofiles,config->filename);
		char *p=strrchr(config->tofiles,'.'),
			 *s=strrchr(config->tofiles,'/');
		if (p && p>s && p!=config->tofiles) *p='\0';
		appendstr(config->tofiles,"##.");
		appendstr(config->tofiles,filter->DefaultExtension());
	}
	
	

	//------------------------Destination----------
	 
	 //--------- format
	int c2=-1;
	SliderPopup *format;
	last=format=new SliderPopup(this, "format",NULL,SLIDER_LEFT|SLIDER_POP_ONLY, 0,0,0,0,1, last, object_id, "format",NULL,0);
	for (c=0; c<laidout->exportfilters.n; c++) {
		format->AddItem(laidout->exportfilters.e[c]->VersionName(),c);
		if (filter==laidout->exportfilters.e[c]) c2=c;
	}
	if (c2>=0) format->Select(c2);
	format->WrapToExtent();
	format->tooltip(_("The file format to export into"));
	AddWin(format,1, format->win_w,0,50,50,0, 2*textheight,0,0,50,0, -1);
	AddNull();

	 //--------- to command
	if (dialog_style&EXPORT_COMMAND) {
		last=commandcheck=new CheckBox(this,"command-check",NULL,CHECK_CIRCLE|CHECK_LEFT, 
							 0,0,0,0,0, 
							 last,object_id,"command-check",
							 _("By Command: "), CHECKGAP,5);
		commandcheck->State(LAX_ON);
		commandcheck->tooltip(_("Run this command on a single exported file"));
		AddWin(commandcheck,1,-1);

		last=command=new LineEdit(this,"command",NULL,LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF, 
							 0,0,100,20, 1,
							 last,object_id,"command",
							 "lp",0);
		command->padx=5;
		command->tooltip(_("Run this command on a single exported file"));
		AddWin(command,1, command->win_w,0,1000,50,0, linpheight,0,0,50,0, -1);
		AddNull();
	}
	
	 //--------- to file
	last=filecheck=new CheckBox(this,"tofile-check",NULL,CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,object_id,"tofile-check",
						 filter->DirectoryBased() ? _("To Dir: ") : _("To File: "), CHECKGAP,5);
	filecheck->State(LAX_ON);
	filecheck->tooltip(_("Export to this path"));
	AddWin(filecheck,1,-1);

//	 ***** have: [!] _filename_   <-- meaning file exists, tooltip to say what it means
//		         [O]              <-- meaning ok to overwrite
//				 [ ]              <-- meaning does not exist, ok to write to
	last=fileedit=new LineEdit(this,"tofile",NULL,
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_ANY_CHANGE, 
						 0,0,100,linpheight, 1,
						 last,object_id,"tofile",
						 config->filename,0);
	fileedit->padx=5;
	fileedit->tooltip(_("Export to this file"));
	fileedit->SetCurpos(-1);
	AddWin(fileedit,1, fileedit->win_w,0,1000,50,0, linpheight,0,0,50,0, -1);
	last=tbut=new Button(this,"filesaveas",NULL,0,
						0,0,0,0, 1, 
						last,object_id,"filesaveas",
						0,"...",NULL,NULL,3,3);
	tbut->tooltip(_("Browse for a new location"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	 //--------- to files
//	 ***** have: [!] _filename_   <-- meaning file exists, tooltip to say what files in range will be overwritten
	last=filescheck=new CheckBox(this,"tofiles",NULL,CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,object_id,"tofiles-check",
						 filter->DirectoryBased() ? _("To Dirs: ") : _("To Files: "), CHECKGAP,5);
	filescheck->State(LAX_OFF);
	filescheck->tooltip(_("Export to these paths. A '#' is replaced with\n"
						  "the spread index. A \"###\" for an index like 3\n"
						  "will get replaced with \"003\"."));
	AddWin(filescheck,1,-1);

	last=filesedit=new LineEdit(this,"tofiles",NULL,
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF|LINEEDIT_SEND_ANY_CHANGE, 
						 0,0,100,20, 1,
						 last,object_id,"tofiles",
						 config->tofiles,0);
	filesedit->padx=5;
	filesedit->tooltip(_("Export to these paths. A '#' is replaced with\n"
						  "the spread index. A \"###\" for an index like 3\n"
						  "will get replaced with \"003\"."));
	fileedit->SetCurpos(-1);
	AddWin(filesedit,1, filesedit->win_w,0,1000,50,0, filesedit->win_h,0,0,50,0, -1);
	 // a "..." to pop file dialog:
	last=tbut=new Button(this,"filessaveas",NULL,0, 0,0,0,0, 1, 
						last,object_id,"filessaveas",
						0,"...",NULL,NULL,3,3);
	tbut->tooltip(_("Browse for a new location"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	

	 //--- add a vertical spacer
	AddWin(NULL,0, 0,0,9999,50,0, 12,0,0,50,0, -1);
	AddNull();


	 //------------------------What kind of layout----------
	if (config->doc) {
		//****doc->imposition->Layouts()
		SliderPopup *layouts;
		last=layouts=new SliderPopup(this, "layouts",NULL,SLIDER_POP_ONLY, 0,0,0,0,1, last, object_id, "layout",NULL,0);
	//	for (c=0; c<config->doc->imposition->NumLayouts(); c++) {
	//		layouts->AddItem(config->doc->imposition->LayoutName(c),c);
	//		if (filter==laidout->exportfilters.e[c]) c2=c;
	//	}
		layouts->AddItem(config->doc->imposition->LayoutName(SINGLELAYOUT),SINGLELAYOUT);
		layouts->AddItem(config->doc->imposition->LayoutName(PAGELAYOUT),  PAGELAYOUT);
		layouts->AddItem(config->doc->imposition->LayoutName(PAPERLAYOUT), PAPERLAYOUT);
		layouts->Select(config->layout);
		layouts->WrapToExtent();
		layouts->tooltip(_("The type of spreads to export"));
		AddWin(layouts,1, layouts->win_w,0,50,50,0, layouts->win_h,0,0,50,0, -1);
		AddNull();
	}


	 //------------------------Range----------

	 //-------------[ ] All
	 //             [ ] Current
	 //             [ ] From _____ to ______  <-- need to know their ranges!!
	 //             ...todo: and use labels for pages   __"A1"_-_"Z8"_  __0 - 10, 15-2, 22__   _all_

	last = printall = new CheckBox(this,"ps-printall",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"ps-printall",
						 _("Export All"), CHECKGAP,5);
	printall->State(LAX_ON);
	//AddWin(printall,1, win_w,0,2000,0,0, printall->win_h,0,0,50,0, -1);
	AddWin(printall,1, -1);
	AddNull();

	last = printcurrent = new CheckBox(this,"ps-printcurrent",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"ps-printcurrent",
						 _("Export Current"), CHECKGAP,5);
	printcurrent->State(LAX_OFF);
	//AddWin(printcurrent,1, win_w,0,2000,0,0, linpheight,0,0,50,0, -1);
	AddWin(printcurrent,1, -1);
	AddNull();

	last=printrange=new CheckBox(this,"ps-printrange",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"ps-printrange",
						 _("Export From:"), CHECKGAP,5);
	printrange->State(LAX_OFF);
	AddWin(printrange,1,-1);

	char blah[15];
	sprintf(blah,"%d",config->start);
	last=printstart=new LineEdit(this,"start",NULL,
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF, 
						 0,0,50,20, 1,
						 last,object_id,"start",
						 blah,0);
	printstart->padx=5;
	printstart->tooltip(_("The starting index"));
	AddWin(printstart,1, printstart->win_w,0,1000,50,0, printstart->win_h,0,0,50,0, -1);
		
	AddWin(new MessageBar(this,"end",NULL,0, 0,0,0,0,0, _("To:")),1,-1);
	sprintf(blah,"%d",config->end);
	last=printend=new LineEdit(this,"end",NULL,LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_FOCUS_OFF, 
						 0,0,50,20, 1,
						 last,object_id,"end",
						 blah,0);
	printend->padx=5;
	printend->tooltip(_("The ending index"));
	AddWin(printend,1, printend->win_w,0,1000,50,0, printend->win_h,0,0,50,0, -1);
	AddNull();

	//printend->tooltip(_("Range of pages to use.\nRange is indices starting from 0, or quote for page labels."));

	AddWin(NULL,0, 0,0,9999,50,0, 12,0,0,50,0, -1);
	AddNull();


	//----------------------------- Selected range options
	//----export batches
	last=batches=new CheckBox(this,"batches",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"batches",
						 _("Export in batches"), CHECKGAP,5);
	batches->State(config->batches>1 ? LAX_ON : LAX_OFF);
	batches->tooltip(_("Export this many spreads at once, continue for whole range"));
	AddWin(batches,1,-1);
	AddHSpacer(textheight,0,0,50);

	sprintf(blah,"%d",config->batches>0?config->batches:1);
	last=batchnumber=new LineEdit(this,"batchnumber",NULL,
						 LINEEDIT_SEND_FOCUS_ON|LINEEDIT_SEND_ANY_CHANGE|LINEEDIT_INT, 
						 0,0,50,20, 1,
						 last,object_id,"batchnumber",
						 blah,0);
	batchnumber->padx=5;
	batchnumber->tooltip(_("The number of spreads in one batch"));
	AddWin(batchnumber,1, batchnumber->win_w,0,1000,50,0, batchnumber->win_h,0,0,50,0, -1);
	AddNull();


	//----reverse order
	last=reverse=new CheckBox(this,"reverse",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"reverse",
						 _("Reverse order"), CHECKGAP,5);
	reverse->State(config->reverse_order ? LAX_ON : LAX_OFF);
	AddWin(reverse,1, reverse->win_w,0,1000,50,0, reverse->win_h,0,0,50,0, -1);
	AddNull();


	AddWin(NULL,0, 0,0,9999,50,0, 12,0,0,50,0, -1);
	AddNull();


	//----export all/even/odd
	AddWin(new MessageBar(this,"end",NULL,0, 0,0,0,linpheight,0, _("Which: ")),1,-1);

	last=everyspread=new CheckBox(this,"everyspread",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"everyspread",
						 _("All"), CHECKGAP,5);
	everyspread->State(config->evenodd==DocumentExportConfig::All ? LAX_ON : LAX_OFF);
	everyspread->tooltip(_("Export each spread"));
	AddWin(everyspread,1,-1);

	AddHSpacer(textheight,textheight,textheight,50);
	last=evenonly=new CheckBox(this,"evenonly",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"evenonly",
						 _("Even only"), CHECKGAP,5);
	evenonly->State(config->evenodd==DocumentExportConfig::Even ? LAX_ON : LAX_OFF);
	evenonly->tooltip(_("Export only spreads with even indices"));
	AddWin(evenonly,1,-1);

	AddHSpacer(textheight,textheight,textheight,50);
	last=oddonly=new CheckBox(this,"oddonly",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"oddonly",
						 _("Odd only"), CHECKGAP,5);
	oddonly->State(config->evenodd==DocumentExportConfig::Odd ? LAX_ON : LAX_OFF);
	oddonly->tooltip(_("Export only spreads with odd indices"));
	AddWin(oddonly,1,-1);
	AddHSpacer(10*textheight,10*textheight,10*textheight,50);
	AddNull();


	 //rotation
	AddWin(new MessageBar(this,"end",NULL,0, 0,0,0,0,0, _("Paper rotation: ")),1,-1);

	last=rotate0=new CheckBox(this,"rotate0",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"rotate0",
						 _("0"), CHECKGAP,5);
	rotate0->State(config->paperrotation==0 ? LAX_ON : LAX_OFF);
	rotate0->tooltip(_("No extra paper rotation"));
	AddWin(rotate0,1,-1);

	last=rotate90=new CheckBox(this,"rotate90",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"rotate90",
						 _("90"), CHECKGAP,5);
	rotate90->State(config->paperrotation==90 ? LAX_ON : LAX_OFF);
	rotate90->tooltip(_("Rotate each paper by 90 degrees"));
	AddWin(rotate90,1,-1);

	last = rotate180 = new CheckBox(this,"rotate180",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"rotate180",
						 _("180"), CHECKGAP,5);
	rotate180->State(config->paperrotation==180 ? LAX_ON : LAX_OFF);
	rotate180->tooltip(_("Rotate each paper by 180 degrees"));
	AddWin(rotate180,1,-1);

	last=rotate270=new CheckBox(this,"rotate270",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"rotate270",
						 _("270"), CHECKGAP,5);
	rotate270->State(config->paperrotation==270 ? LAX_ON : LAX_OFF);
	rotate270->tooltip(_("Rotate each paper by 270 degrees"));
	AddWin(rotate270,1,-1);
	AddNull();

	 //rotate alternate 180
	last=rotatealternate=new CheckBox(this,"rotatealternate",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"rotatealternate",
						 _("Alternate 180"), CHECKGAP,5);
	rotatealternate->tooltip(_("Rotate every other spread by 180 degrees"));
	rotatealternate->State(config->rotate180 ? LAX_ON : LAX_OFF);
	AddWin(rotatealternate,1, rotatealternate->win_w,0,1000,50,0, rotatealternate->win_h,0,0,50,0, -1);
	AddNull();

	 //textaspaths
	last=textaspaths=new CheckBox(this,"textaspaths",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"textaspaths",
						 _("Text as paths"), CHECKGAP,5);
	textaspaths->State(config->textaspaths ? LAX_ON : LAX_OFF);
	textaspaths->tooltip(_("Export all text as paths"));
	AddWin(textaspaths,1,-1);
	AddNull();


	//-------------------------- Extra settings per export type ------------------------------------
	AddWin(NULL,0, 0,0,9999,50,0, 12,0,0,50,0, -1);
	AddNull();

	firstextra=wholelist.n;


	//-------------------------- Final OK ------------------------------------

	AddVSpacer(textheight,0,0,50, -1);
	AddNull();
	AddWin(NULL,0, 0,0,1000,50,0, 0,0,0,50,0, -1);
	last=tbut=new Button(this,"export",NULL,0, 0,0,0,0, 1, 
						last,object_id,"export",
						0,_("Export"),NULL,NULL,3,3);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);
	last=tbut=new Button(this,"cancel",NULL,0, 0,0,0,0, 1, 
						last,object_id,"cancel",
						0,_("Cancel"),NULL,NULL,3,3);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,0,50,0, -1);
	AddWin(NULL,0, 0,0,1000,50,0, 0,0,0,50,0, -1);
	last->CloseControlLoop();
	
	updateExt();

	//win_h=0;
	//m[1]=m[7]=BOX_SHOULD_WRAP;
	Sync(1);
	updateEdits();
	fileedit->SetCurpos(-1); //these seem to need the window to be inited already, so do this here
	filesedit->SetCurpos(-1);
	//Resize(m[0],m[6]);

	overwriteCheck();
	return 0;
}

//! Make sure the range is valid, and re-set the edits if necessary.
/*! Also sets the values in config based on the values in the edits.
 */
void ExportDialog::configBounds()
{
	int a=start(),
		b=end();

	if (a<min) a=min;
	else if (a>max) a=max;
	if (b<min) b=min;
	else if (b>max) b=max;

	config->start=a;
	config->end  =b;

	start(a);
	end(b);
}

//! Set the start of the range in config and edit based on index s.
/*! \todo *** just does index right now!! must use page labels
 */
void ExportDialog::start(int s)
{
	printstart->SetText(s);
}

//! Set the end of the range in config and edit based on index s.
/*! \todo *** just does index right now!! must use page labels
 */
void ExportDialog::end(int e)
{
	printend->SetText(e);
}

//! Return the start of the range as an index, not a name.
/*! \todo *** just does index right now!! must use page labels
 */
int ExportDialog::start()
{
	int e;
	int a=printstart->GetLong(&e);
	if (e) return 0;
	return a;
}

//! Return the end of the range as an index, not a name.
/*! \todo *** just does index right now!! must use page labels
 */
int ExportDialog::end()
{
	int e;
	int a=printend->GetLong(&e);
	if (e) return 0;
	return a;
}

//! Update extensions in file edits.
int ExportDialog::updateExt()
{
	 //do file
	char *s = fileedit->GetText();
	char *p = strrchr(s,'.'),
		 *b = strrchr(s,'/');
	if (p && ((b && p > b) || !b)) *p = '\0'; //removes extension
	if (!filter->DirectoryBased()) {
		appendstr(s,".");
		appendstr(s,filter->DefaultExtension());
	}
	fileedit->SetText(s);
	makestr(config->filename,s);
	delete[] s;

	 //do files
	s = filesedit->GetText();
	p = strrchr(s,'.');
	b = strrchr(s,'/');
	if (p && ((b && p > b) || !b)) *p = '\0'; //removes extension
	if (!filter->DirectoryBased()) {
		appendstr(s,".");
		appendstr(s,filter->DefaultExtension());
	}
	filesedit->SetText(s);
	makestr(config->tofiles,s);
	delete[] s;

	//if (filter->DirectoryBased()) {
	//	fileedit->SetType();
	//}
	fileedit->SetCurpos(-1); //these seem to need the window to be inited already, so do this here
	filesedit->SetCurpos(-1);

	return 0;
}

/*! Make sure the available edit controls correspond to config.
 */
void ExportDialog::updateEdits()
{
	WinFrameBox *box;
	const char *str;

	double linpheight = 2 * win_themestyle->normal->textheight();
	//double textheight = win_themestyle->normal->textheight();

	//first the easy part, labels on edits that exist for all filters:
	filecheck ->Label(filter->DirectoryBased() ? _("To Dir: ") : _("To File: "));
	filescheck->Label(filter->DirectoryBased() ? _("To Dirs: ") : _("To Files: "));


	//now modify panel to have the correct custom edits
	for (int c=wholelist.n-1; c>=0; c--) {
		box=dynamic_cast<WinFrameBox*>(wholelist.e[c]);
		if (!box || !box->win()) continue;
		str = box->win()->win_name;
		if (strstr(str,"extra-")) {
			Pop(c);
		}
	}

	ObjectDef *def = config->GetObjectDef();
	if (strcmp(def->name,"ExportConfig")) {
		 //only do this section for non-default export configs.
		 // *** Note this will add any fields returned by def->getFieldOfThis(),
		 //which is not quite accurate for defs that have more inheritance than just 
		 //straight from ExportConfig
		 //

		char scratch[200];
		anXWindow *last=NULL;
		int i=firstextra;
		for (int c=0; c<def->getNumFieldsOfThis(); c++) {
			ObjectDef *fd=def->getFieldOfThis(c);

			if (fd->format==VALUE_Boolean) {

				sprintf(scratch,"extra-%s",fd->name);
				CheckBox *box;
				last=box=new CheckBox(this,scratch,NULL,CHECK_CIRCLE|CHECK_LEFT, 
									 0,0,0,0,0, 
									 last,object_id,scratch,
									 fd->Name, win_themestyle->normal->textheight()/5,5);

				if (!isblank(fd->description)) box->tooltip(fd->description);
				Value *v=config->dereference(fd->name,strlen(fd->name));
				//if (config->findBoolean(fd->name)) box->State(LAX_ON);
				if (dynamic_cast<BooleanValue*>(v)->i) box->State(LAX_ON);
				v->dec_count();

				AddWin(box,1,i++); 
				AddNull(i++);

			} else if (fd->format == VALUE_String || fd->format == VALUE_File) { 
				sprintf(scratch,"extra-%s",fd->name);
				LineInput *box;
				last = box = new LineInput(this,scratch,NULL, LINP_SEND_ANY | (fd->format == VALUE_File ? LINP_FILE : 0), 
									 0,0,0,0,0, 
									 last,object_id,scratch,
									 fd->Name, NULL);

				if (!isblank(fd->description)) box->tooltip(fd->description);
				Value *v=config->dereference(fd->name,strlen(fd->name));
				if (fd->format == VALUE_File) {
					FileValue *str = dynamic_cast<FileValue*>(v);
					if (str) box->SetText(str->filename);
				} else {
					StringValue *str=dynamic_cast<StringValue*>(v);
					if (str) box->SetText(str->str);
				}
				v->dec_count();

				AddWin(box,1, box->win_w,0,10000,50,0, linpheight,0,0,50,0, i++); 
				//AddWin(box,1,i++); 
				AddNull(i++);

			} else if (fd->format==VALUE_Int || fd->format==VALUE_Real) { 
				sprintf(scratch,"extra-%s",fd->name);
				LineInput *box;
				last=box=new LineInput(this,scratch,NULL,
									(fd->format==VALUE_Int ? LINP_INT : LINP_FLOAT) | LINP_SEND_ANY, 
									 0,0,0,0,0, 
									 last,object_id,scratch,
									 fd->Name, NULL);

				if (!isblank(fd->description)) box->tooltip(fd->description);
				Value *v=config->dereference(fd->name,strlen(fd->name));
				IntValue *ii=dynamic_cast<IntValue*>(v);
				if (ii) box->SetText((int)ii->i);
				else {
					DoubleValue *ii=dynamic_cast<DoubleValue*>(v);
					if (ii) box->SetText(ii->d);
				}
				v->dec_count();

				AddWin(box,1,i++); 
				AddNull(i++);

			} else if (fd->format == VALUE_Object) {
				Value *v=config->dereference(fd->name,strlen(fd->name));
				ObjectValue *ov = dynamic_cast<ObjectValue*>(v);
				AttributeObject *ao = dynamic_cast<AttributeObject*>(ov->object);
				if (ao) { //assume this is a meta object
					//label [edit]
					AddWin(new MessageBar(this,"extra-meta","meta",0, 0,0,0,0,0, fd->Name),1, i++);
					Button *tbut;
					sprintf(scratch,"=extra-%s",fd->name);
					tbut = new Button(this, scratch,"metaedit",0,
										0,0,0,0, 1, 
										last,object_id,scratch,
										0,_("Edit meta"),NULL,NULL,3,3);
					AddWin(tbut,1,i++);

					sprintf(scratch,"+extra-%s",fd->name);
					tbut = new Button(this, scratch,"metaload",0,
										0,0,0,0, 1, 
										last,object_id,scratch,
										0,_("Load meta"),NULL,NULL,3,3);
					AddWin(tbut,1,i++);

					sprintf(scratch,"-extra-%s",fd->name);
					tbut = new Button(this, scratch,"metasave",0,
										0,0,0,0, 1, 
										last,object_id,scratch,
										0,_("Save meta"),NULL,NULL,3,3);
					AddWin(tbut,1,i++);
					AddNull(i++);
				}

			} else {
				DBG cerr << "*** warning! uncaught field type "<<element_TypeNames(fd->format)<<"in an export config!"<<endl;
			}
		}
	}

	Sync(1);
}

//! Keep the controls straight.
/*! 
 * \todo ability to use page/layout names like iii, instead of index numbers
 */
int ExportDialog::Event(const EventData *ee,const char *mes)
{
	const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(ee);

	if (!strncmp(mes,"extra-",6)) {
		 //for events outside the default DocumentExportConfig
		const char *field = mes+6;
		ObjectDef *def = config->GetObjectDef();
		ObjectDef *fd = def->FindDef(field,strlen(field),0);
		if (!fd) return 0;

		const SimpleMessage *eee=dynamic_cast<const SimpleMessage*>(ee);
		FieldExtPlace ff(field);
		if (fd->format==VALUE_Boolean) {
			BooleanValue v(eee->info1==LAX_ON ? true : false);
			config->assign(&ff, &v);

		} else if (fd->format==VALUE_String) {
			StringValue v(eee->str);
			config->assign(&ff, &v);

		} else if (fd->format==VALUE_File) {
			FileValue v(eee->str);
			config->assign(&ff, &v);

		} else if (fd->format==VALUE_Int || fd->format==VALUE_Real) {
			DoubleValue v(strtod(eee->str,NULL));
			config->assign(&ff, &v);
		}

		return 0;

	} else if (!strcmp(mes,"textaspaths")) {
		if (!e) return 1;
		int s=reverse->State();
		if (s==LAX_ON) config->textaspaths=1;
		else config->textaspaths=0;
		return 0;

	} else if (!strcmp(mes,"get new file")) {
		if (!e) return 1;
		fileedit->SetText(e->str);
		return 0;

	} else if (!strcmp(mes,"get new files")) {
		if (!e) return 1;
		filesedit->SetText(e->str);
		return 0;

	} else if (!strcmp(mes,"format")) {
		filter=laidout->exportfilters.e[e->info1];
		DocumentExportConfig *nconfig=filter->CreateConfig(config);
		config->dec_count();
		config=nconfig;

		updateEdits();
		findMinMax();
		configBounds();
		updateExt();
		overwriteCheck();
		return 0;

	} else if (!strcmp(mes,"layout")) {
		config->layout=e->info1;
		findMinMax();
		configBounds();
		return 0;

	} else if (!strcmp(mes,"command-check")) {
		changeTofile(DocumentExportConfig::TARGET_Command);
		return 0;

	} else if (!strcmp(mes,"tofile-check")) {
		changeTofile(DocumentExportConfig::TARGET_Single);
		return 0;

	} else if (!strcmp(mes,"tofiles-check")) {
		changeTofile(DocumentExportConfig::TARGET_Multi);
		return 0;

	} else if (!strcmp(mes,"everyspread")) {
		changeToEvenOdd(DocumentExportConfig::All);
		return 0;

	} else if (!strcmp(mes,"evenonly")) {
		changeToEvenOdd(DocumentExportConfig::Even);
		return 0;

	} else if (!strcmp(mes,"oddonly")) {
		changeToEvenOdd(DocumentExportConfig::Odd);
		return 0;

	} else if (!strcmp(mes,"batches")) {
		 //toggle using batches
		int s=batches->State();
		if (s==LAX_ON) {
			//now on, need to force using files, not file
			changeTofile(DocumentExportConfig::TARGET_Multi);
		}
		return 0;

	} else if (!strcmp(mes,"batchnumber")) {
		 //turn on or off batches as necessary
		int n=batchnumber->GetLong(NULL);
		DBG cerr << " **** batchnumber: "<<n<<endl;
		if (n<=0) {
			DBG cerr <<" *** setting to 0"<<endl;
			batches->State(LAX_OFF);
			batchnumber->SetText(0);
			config->batches=0;
		} else {
			batches->State(LAX_ON);
			config->batches=n;
			changeTofile(DocumentExportConfig::TARGET_Multi);
		}
		return 0;

	} else if (!strcmp(mes,"rotatealternate")) {
		int s = rotatealternate->State();
		if (s == LAX_ON) config->rotate180 = 1;
		else config->rotate180 = 0;
		return 0;

	} else if (!strcmp(mes,"rotate0")) {
		paperRotation(0);
		return 0;

	} else if (!strcmp(mes,"rotate90")) {
		paperRotation(90);
		return 0;

	} else if (!strcmp(mes,"rotate180")) {
		paperRotation(180);
		return 0;

	} else if (!strcmp(mes,"rotate270")) {
		paperRotation(270);
		return 0;

	} else if (!strcmp(mes,"reverse")) {
		int s=reverse->State();
		if (s==LAX_ON) config->reverse_order=1;
		else config->reverse_order=0;
		return 0;

	} else if (!strcmp(mes,"ps-printall")) {
		changeRangeTarget(1);
		start(0);
		end(max);
		configBounds();
		return 0;

	} else if (!strcmp(mes,"ps-printcurrent")) {
		changeRangeTarget(2);
		start(cur);
		end(cur);
		configBounds();
		return 0;

	} else if (!strcmp(mes,"ps-printrange")) {
		changeRangeTarget(3);
		return 0;

	} else if (!strcmp(mes,"start")) {
		DBG cerr <<"start data: "<<e->info1<<endl;
		if (e->info1==2) {
			changeRangeTarget(3);
		} else {
			configBounds();
		}
		return 0;

	} else if (!strcmp(mes,"end")) {
		DBG cerr <<"end data: "<<e->info1<<endl;
		if (e->info1==2) {
			 //focus on
			changeRangeTarget(3);
		} else {
			configBounds();
		}
		return 0;

	} else if (!strcmp(mes,"tofiles")) {
		if (e->info1==1) {
			 //1 means enter was pressed
			send();
			return 0;
		} else if (e->info1==2) {
			 //focus on
			changeTofile(DocumentExportConfig::TARGET_Multi);
			return 0;
		} else if (e->info1==3) {
			 //focus off
			makestr(config->tofiles,filesedit->GetCText());
			return 0;
		} else if (e->info1==0) {
			 //text was modified
			makestr(config->tofiles,filesedit->GetCText());
			overwriteCheck();
			return 0;
		}

	} else if (!strcmp(mes,"tofile")) {
		if (e->info1==1) {
			 //1 means enter was pressed
			send();
			return 0;
		} else if (e->info1==2) {
			 //focus on
			changeTofile(DocumentExportConfig::TARGET_Single);
			return 0;
		} else if (e->info1==3) {
			 //focus off
			makestr(config->filename,fileedit->GetCText());
			return 0;
		} else if (e->info1==0) {
			 //text was modified
			makestr(config->filename,fileedit->GetCText());
			overwriteCheck();
			return 0;
		}

	} else if (!strcmp(mes,"command")) {
		if (e->info1==1) { 
			send();
			return 0; 
		} else if (e->info1==2) {
			 //focus on
			changeTofile(DocumentExportConfig::TARGET_Command);
			return 0;
		} else if (e->info1==0) {
			 //text was modified
			makestr(config->command, command->GetCText());
		}
		return 0;

	} else if (!strcmp(mes,"cancel")) {
		app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"export")) {
		send();
		return 0;

	} else if (!strcmp(mes,"filesaveas")) {
		FileDialog *fd=new FileDialog(NULL,"get new file",NULL,ANXWIN_REMEMBER,
									  0,0,0,0,0,object_id,"get new file",
									  FILES_OPEN_ONE,
									  fileedit->GetCText());
		fd->OkButton(_("Select"),NULL);
		app->rundialog(fd);
		return 0;

	} else if (!strcmp(mes,"filessaveas")) {
		app->rundialog(new FileDialog(NULL,"get new file",NULL,ANXWIN_REMEMBER,
									  0,0,0,0,0,object_id,"get new files",
									  FILES_OPEN_ONE,
									  filesedit->GetCText()));
		return 0;

	} else if (mes[0] == '=') {
		//edit meta
		Value *v = config->dereference(mes+7, strlen(mes+7));
		ObjectDef *fd = v->GetObjectDef();
		if (fd) {
			ObjectValue *ov = dynamic_cast<ObjectValue*>(v);
			if (ov) {
				AttributeObject *ao = dynamic_cast<AttributeObject*>(ov->object);
				if (ao) app->rundialog(new MetaWindow(NULL,"meta",_("Filter Meta"),0, object_id,"docMeta", ao));
			}
		}
		return 0;

	} else if (mes[0] == '+') { //+extra-*
		//load meta: get a file name
		char scratch[strlen(mes)+2];
		sprintf(scratch, "<%s", mes+1);
		FileDialog *fd = new FileDialog(NULL,"LoadMeta",_("Load meta..."),ANXWIN_REMEMBER,
						  0,0,0,0,0,object_id,scratch,
						  FILES_OPEN_ONE,
						  last_meta_dir);
		app->rundialog(fd);
		return 0;

	} else if (mes[0] == '-') { //-extra-*
		//save meta: get a file name
		char scratch[strlen(mes)+2];
		sprintf(scratch, ">%s", mes+1);
		FileDialog *fd = new FileDialog(NULL,"SaveMeta",_("Save meta..."),ANXWIN_REMEMBER,
						  0,0,0,0,0,object_id,scratch,
						  FILES_SAVE | FILES_ASK_TO_OVERWRITE,
						  last_meta_dir);
		app->rundialog(fd);
		return 0;

	} else if (mes[0] == '>') { //>extra-*
		//save meta now
		Value *v = config->dereference(mes+7, strlen(mes+7));
		if (!v) return 0;
		ObjectValue *ov = dynamic_cast<ObjectValue*>(v);
		if (!ov) return 0;
		AttributeObject *ao = dynamic_cast<AttributeObject*>(ov->object);
		if (!ao) return 0;

		const char *file = e->str;
		if (isblank(file)) return 0;
		FILE *f = fopen(file, "w");
		if (!f) {
			DBG cerr << "Could not open file for saving meta"<<endl;
			return 0;
		}
		fprintf(f, "#Laidout %s Metadata\n", LAIDOUT_VERSION);
		ao->dump_out(f, 0);
		fclose(f);
		return 0;

	} else if (mes[0] == '<') { //<extra-*
		//load meta now
		const char *file = e->str;
		if (isblank(file)) return 0;
		FILE *f = fopen(file, "r");
		if (!f) {
			return 0;
		}
		AttributeObject *ao = new AttributeObject;
		ao->dump_in(f, 0);
		fclose(f);
		ObjectValue *ov = new ObjectValue(ao);
		ao->dec_count();
		FieldExtPlace ff(mes+7);
		config->assign(&ff, ov);
		ov->dec_count();
		return 0;
	}

	return 0;
}

void ExportDialog::overwriteCheck()
{
	DBG cerr <<"-----overwrite check "<<endl;

	int valid,err;
	unsigned long color=rgbcolor(255,255,255);

	int isdir = filter->DirectoryBased();

	if (filecheck->State() == LAX_ON) {
		 //check file
		if (isblank(fileedit->GetCText())) valid = -1;
		else valid = file_exists(fileedit->GetCText(),1,&err);

		if (valid) {
			if ((isdir && valid != S_IFDIR) || (!isdir && valid != S_IFREG)) { // exists, but is not proper type
				if (valid!=1) color=rgbcolor(255,100,100);
				fileedit->tooltip(_("Cannot overwrite this!"));
				findChildWindowByName("export")->Grayed(1);
			} else { // was proper type
				color=rgbcolor(255,255,0);
				fileedit->tooltip(_("WARNING: This file will be overwritten on export!"));
				findChildWindowByName("export")->Grayed(0);
			}
		} else {
			fileedit->tooltip(_("Export to this file"));
			findChildWindowByName("export")->Grayed(0);
		}
		fileedit->Valid(!valid,color);

	} else if (filescheck->State() == LAX_ON) {
		if (isblank(filesedit->GetCText())) {
			filesedit->tooltip(_("Cannot write to nothing!"));
			findChildWindowByName("export")->Grayed(1);
			filesedit->Valid(0,color);
			return;
		}

		char *filebase;
		int e=0,w=0; //errors and warnings
		filebase=LaxFiles::make_filename_base(config->tofiles);
		char file[strlen(filebase)+10]; //*** someone could trick this with more than "##" blocks maybe!! check!
		for (int c=config->start; c<=config->end; c++) {
			sprintf(file,filebase,c);

			valid = file_exists(file,1,&err);
			if (valid) {
				if ((isdir && valid != S_IFDIR) || (!isdir && valid != S_IFREG)) { // exists, but is wrong type
					e++;
				} else { // was proper type
					w++;
				}
			}
		}
		if (e) {
			color=rgbcolor(255,100,100);
			filesedit->tooltip(_("Some files cannot be overwritten!"));
			findChildWindowByName("export")->Grayed(1);
		} else if (w) {
			color=rgbcolor(255,255,0);
			filesedit->tooltip(_("Warning: Some files will be overwritten!"));
			findChildWindowByName("export")->Grayed(0);
		} else {
			 //note: this must be same tip as in init().
			filesedit->tooltip(_("Export to these files. A '#' is replaced with\n"
					  "the spread index. A \"###\" for an index like 3\n"
					  "will get replaced with \"003\"."));
			findChildWindowByName("export")->Grayed(0);
		}
		filesedit->Valid(!valid,color);
	}
}

/*! Update the paper rotation checkboxes to only have rotation checked.
 */
void ExportDialog::paperRotation(int rotation)
{
	config->paperrotation=rotation;
	rotate0  ->State(rotation==0   ? LAX_ON : LAX_OFF);
	rotate90 ->State(rotation==90  ? LAX_ON : LAX_OFF);
	rotate180->State(rotation==180 ? LAX_ON : LAX_OFF);
	rotate270->State(rotation==270 ? LAX_ON : LAX_OFF);
}

//! This allows special things to happen when a different printing target is selected.
/*! Updates the target checkboxes.
 *
 * Uses DocumentExportConfig::TARGET_*
 *
 * \todo maybe check against range, and whether the filter supports multipage
 */
void ExportDialog::changeTofile(int t)
{
	tofile = t;
	config->target = t;

	
	if (t == DocumentExportConfig::TARGET_Single) batches->State(LAX_OFF);
	filecheck-> State(tofile == DocumentExportConfig::TARGET_Single ? LAX_ON : LAX_OFF);
	filescheck->State(tofile == DocumentExportConfig::TARGET_Multi ? LAX_ON : LAX_OFF);
	if (commandcheck) commandcheck->State(tofile == DocumentExportConfig::TARGET_Command ? LAX_ON : LAX_OFF);

	overwriteCheck();
}

void ExportDialog::changeToEvenOdd(DocumentExportConfig::EvenOdd t)
{
	config->evenodd=t;

	everyspread->State(t==DocumentExportConfig::All ? LAX_ON : LAX_OFF);
	evenonly->State(t==DocumentExportConfig::Even ? LAX_ON : LAX_OFF);
	oddonly->State(t==DocumentExportConfig::Odd ? LAX_ON : LAX_OFF);
}

//! This allows special things to happen when a different range target is selected.
/*! Updates the range checkboxes.
 *
 * 1=all,
 * 2=current.
 * 3=from __ to __
 *
 * \todo update range indicator when current or all selected
 */
void ExportDialog::changeRangeTarget(int t)
{
	printrange->State(t==3?LAX_ON:LAX_OFF);
	printcurrent->State(t==2?LAX_ON:LAX_OFF);
	printall->State(t==1?LAX_ON:LAX_OFF);
}

//! Send the config object to owner.
/*! Note that a new config object is sent, and the receiving code should delete it.
 */
int ExportDialog::send()
{
	if (findChildWindowByName("export")->Grayed()) return 0;

	//a little sanity checking...
	if (printall->State() == LAX_ON) {
		config->start = 0;
		config->end = max;
	}

	config->filter = filter;
	DocumentExportConfig *nconf = dynamic_cast<DocumentExportConfig*>(config->duplicate());
	previous_configs.push(config->filter->VersionName(), nconf);
	nconf->dec_count();

	ConfigEventData *data = new ConfigEventData(config);
	app->SendMessage(data, win_owner, win_sendthis, object_id);
	app->destroywindow(this);
	return 1;
}

//! Character input.
/*! ESC  cancel exporting
 */
int ExportDialog::CharInput(unsigned int ch, unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		app->destroywindow(this);
		return 0;
	}
	return 1;
}


} // namespace Laidout

