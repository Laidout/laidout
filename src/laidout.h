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
// Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef LAIDOUT_H
#define LAIDOUT_H

#include <lax/anxapp.h>
#include <lax/iconmanager.h>
#include <lax/errorlog.h>
#include <lax/resources.h>

#include "laidoutprefs.h"
#include "papersizes.h"
#include "document.h"
#include "project.h"
#include "interfaces.h"
#include "calculator/calculator.h"
#include "impositions/imposition.h"
#include "filetypes/filefilters.h"
#include "calculator/values.h"
#include "plugins/plugin.h"


namespace Laidout {

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
		TreeObjectAdded,
		TreeMAX
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
	PrefsDisplayDetails,
	PrefsJustAutosaved
};

//------------------------------------ LaidoutApp ----------------------------------

enum RunModeType {
		RUNMODE_Normal,
		RUNMODE_Commands,
		RUNMODE_Shell,
		RUNMODE_Quit,
		RUNMODE_Impose_Only
	};

class LaidoutApp : public Laxkit::anXApp, public Value, public Laxkit::EventReceiver
{
 protected:
	Laxkit::ErrorLog generallog;

	void dumpOutResources();

	int autosave_timerid;
	virtual int  Idle(int tid, double delta);
	virtual int Autosave();

 public:
	RunModeType runmode;

	char *config_dir;
	Project *project;
	Document *curdoc;
	Laxkit::anXWindow *lastview;

	 //global prefs
	int experimental;
	LaidoutPreferences prefs;

	char *icon_dir;
	Laxkit::IconManager *icons;

	unsigned long curcolor;
	
	char preview_transient;
	int preview_over_this_size;
	Laxkit::PtrStack<char>preview_file_bases;
	int max_preview_length, max_preview_width, max_preview_height;

	char *ghostscript_binary;
	
	Laxkit::ResourceManager resources;

//	Laxkit::PtrStack<Style> stylestack:
//	Laxkit::PtrStack<FontThing> fontstack;
	Laxkit::RefPtrStack<LaxInterfaces::anInterface> interfacepool;
	Laxkit::PtrStack<ImpositionResource> impositionpool;
	Laxkit::PtrStack<ExportFilter> exportfilters;
	Laxkit::PtrStack<ImportFilter>  importfilters;

	Laxkit::RefPtrStack<PluginBase> plugins;
	int AddPlugin(const char *path);
	int RemovePlugin(const char *name);
	int InitializePlugins();

	LaidoutCalculator *calculator;
	Laxkit::RefPtrStack<Interpreter> interpreters;

	int InitInterpreters();
	int AddInterpreter(Interpreter *i, bool absorb_count);
	int RemoveInterpreter(Interpreter *i);
	Interpreter *FindInterpreter(const char *name);

	Laxkit::PtrStack<PaperStyle> papersizes;
	PaperStyle *defaultpaper; //could be a custom, so need to have extra field here
	PaperStyle *GetDefaultPaper();

	LaidoutApp();
	virtual ~LaidoutApp();
	virtual int close();
	virtual const char *whattype() { return "LaidoutApp"; }
	virtual int init(int argc,char **argv);
	virtual void setupdefaultcolors();
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

	 //for Value:
	ObjectDef *makeObjectDef();
	Value *duplicate();

	 //commands
	Document *findDocumentById(unsigned long id);
	Document *findDocument(const char *saveas);
	int Load(const char *filename, Laxkit::ErrorLog &log);
	Document *LoadTemplate(const char *filename, Laxkit::ErrorLog &log);
	int NewDocument(Imposition *imposition, const char *filename);
	int NewDocument(const char *spec);
	int NewProject(Project *proj, Laxkit::ErrorLog &log);
	void PushExportFilter(ExportFilter *filter);
	ExportFilter *FindExportFilter(const char *name, bool exact_only);
	void PushImportFilter(ImportFilter *filter);
	void NotifyGeneralErrors(Laxkit::ErrorLog *log);

	virtual void UpdateAutosave();

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


} // namespace Laidout

#endif


