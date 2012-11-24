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
// Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef LAIDOUT_H
#define LAIDOUT_H

#include <lax/anxapp.h>
#include <lax/iconmanager.h>
#include <lax/errorlog.h>

#include "papersizes.h"
#include "document.h"
#include "project.h"
#include "interfaces.h"
#include "calculator/calculator.h"
#include "impositions/imposition.h"
#include "filetypes/filefilters.h"

const char *LaidoutVersion();

//------------------------------------ TreeChangeEvent ----------------------------------
enum TreeChangeType {
		TreeDocGone,
		TreePagesAdded,
		TreePagesDeleted,
		TreePagesMoved,
		TreeObjectRepositioned,
		TreeObjectReorder,
		TreeObjectDiffPage,
		TreeObjectDeleted,
		TreeObjectAdded
	};


class TreeChangeEvent : public Laxkit::EventData
{
 public:
	Laxkit::anObject *changer;
	TreeChangeType changetype;
	union {
		Document *doc;
		Page *page;
		Laxkit::anObject *obj;
	} obj;
	int start,end;
	TreeChangeEvent() : changer(NULL),start(0),end(0) {}
	TreeChangeEvent(const TreeChangeEvent &te);
};

enum GlobalPrefsNotify {
	PrefsDefaultUnits,
	PrefsDisplayDetails
};

//------------------------------------ LaidoutApp ----------------------------------

enum RunModeType {
		RUNMODE_Normal,
		RUNMODE_Commands,
		RUNMODE_Shell,
		RUNMODE_Impose_Only
	};

class LaidoutApp : public Laxkit::anXApp
{
 protected:
	void dumpOutResources();
 public:
	RunModeType runmode;

	char *config_dir;
	Project *project;
	Document *curdoc;
	Laxkit::anXWindow *lastview;

	LaidoutCalculator *calculator;

	 //global prefs
	int default_units;
	char *unitname;
	int pagedropshadow;
	char *splash_image_file;


	unsigned long curcolor;
	
	char preview_transient;
	int preview_over_this_size;
	Laxkit::PtrStack<char>preview_file_bases;
	int max_preview_length, max_preview_width, max_preview_height;

	char *ghostscript_binary;

	char *default_template;
	char *defaultpaper;
	char *palette_dir;
	char *icon_dir;
	char *temp_dir;

	Laxkit::IconManager icons;
	
//	Laxkit::PtrStack<Style> stylestack:
//	Laxkit::PtrStack<FontThing> fontstack;
	Laxkit::RefPtrStack<LaxInterfaces::anInterface> interfacepool;
	Laxkit::PtrStack<ImpositionResource> impositionpool;
	Laxkit::PtrStack<PaperStyle> papersizes;
	Laxkit::PtrStack<ExportFilter> exportfilters;
	Laxkit::PtrStack<ImportFilter>  importfilters;
	
	LaidoutApp();
	virtual ~LaidoutApp();
	virtual int init(int argc,char **argv);
	virtual void setupdefaultcolors();
	StyleDef *makeStyleDef();
	void parseargs(int argc,char **argv);
	int readinLaidoutDefaults();
	int createlaidoutrc();
	int isTopWindow(Laxkit::anXWindow *win);
	int numTopWindows() { return topwindows.n; }

	int dump_out_file_format(const char *file, int nooverwrite);
	int dump_out_shortcuts(FILE *f, int indent, int how);
	void InitializeShortcuts();
	int DumpWindows(FILE *f,int indent,Document *doc);
	int IsProject();

	 //commands
	Document *findDocumentById(unsigned long id);
	Document *findDocument(const char *saveas);
	int Load(const char *filename, ErrorLog &log);
	Document *LoadTemplate(const char *filename, ErrorLog &log);
	int NewDocument(Imposition *imposition, const char *filename);
	int NewDocument(const char *spec);
	int NewProject(Project *proj, ErrorLog &log);
	void PushExportFilter(ExportFilter *filter);
	void PushImportFilter(ImportFilter *filter);

	 //data manipulation peacekeeper
	void notifyDocTreeChanged(Laxkit::anXWindow *callfrom,TreeChangeType change,int s,int e);
	void notifyPrefsChanged(Laxkit::anXWindow *callfrom,int what);

	 //resource and external executable management
	char *full_path_for_resource(const char *name,const char *dir=NULL);
	char *default_path_for_resource(const char *resource);
	const char *binary(const char *what);
};

// if included in laidout.cc, then don't include "extern" when defining *laidout
// *** is that really necessary??
#ifndef LAIDOUT_CC
extern
#endif
LaidoutApp *laidout;


#endif

