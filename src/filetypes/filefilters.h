//
//
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2007-2009,2012 by Tom Lechner
//
#ifndef FILEFILTERS_H
#define FILEFILTERS_H

#include <lax/errorlog.h>
#include <lax/anxapp.h>
#include <lax/dump.h>
#include <lax/indexrange.h>

#include "../core/papersizes.h"
#include "../core/externaltools.h"
#include "../dataobjects/group.h"
#include "../calculator/values.h"
#include "../plugins/plugin.h"


namespace Laidout {



class Document;
class DocumentExportConfig;


//------------------------------------- FileFilter -----------------------------------

#define FILTER_MULTIPAGE  (1<<0)
#define FILTER_MANY_FILES (1<<1)
#define FILTER_DIR_BASED  (1<<2)

class FileFilter : public Laxkit::anObject
{
 public:
	unsigned int flags;
	PluginBase *plugin; //***which plugin, if any, the filter came from.

	FileFilter();
	virtual ~FileFilter();
	virtual const char *Author() = 0;
	virtual const char *FilterVersion() = 0;
	
	virtual const char *Format() = 0;
	virtual const char *DefaultExtension() = 0;
	virtual const char *Version() = 0;
	virtual const char *VersionName() = 0;
	virtual const char *FilterClass() = 0;
	virtual ObjectDef *GetObjectDef() = 0;
	virtual bool DirectoryBased();

	virtual Laxkit::anXWindow *ConfigDialog() { return NULL; }
};


//------------------------------------- ImportFilter -----------------------------------
class ImportFilter : public FileFilter
{
 public:
	virtual const char *whattype() { return "FileInputFilter"; }
	virtual const char *FileType(const char *first100bytes) = 0;
	virtual int In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents,int contentslen) = 0;
	virtual ObjectDef *makeObjectDef();
};


//------------------------------------- ExportFilter -----------------------------------
class ExportFilter : public FileFilter
{
 public:
	virtual const char *whattype() { return "FileOutputFilter"; }
	virtual int Out(const char *file, Laxkit::anObject *context,  Laxkit::ErrorLog &log) = 0;
	virtual int Verify(Laxkit::anObject *context) { return 1; } //= 0; //***preflight checker
	virtual ObjectDef *makeObjectDef();
	virtual DocumentExportConfig *CreateConfig(DocumentExportConfig *fromconfig);
};

//------------------------------- DocumentExportConfig ----------------------------------
enum CollectForOutValues {
	COLLECT_Dont_Collect,
	COLLECT_Without_log,
	COLLECT_With_Log
};

ObjectDef *makeExportConfigDef();
int createExportConfig(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, Laxkit::ErrorLog &log);

class DocumentExportConfig : public Value
{
 public:
	enum Targets { TARGET_Single = 0, TARGET_Multi = 1 }; //needs to stay in sync with objectdef enum
	int target; //TARGET_Single or TARGET_Multi
	bool send_to_command;
	bool del_after_command;
	ExternalTool *command;
	Laxkit::IndexRange range; //of spreads in doc
	int layout;
	enum EvenOdd { All,Even,Odd } evenodd;
	int batches;
	int reverse_order;
	int paperrotation;
	int rotate180; //0 or 1
	int curpaperrotation; //set automatically, not a user variable
	int collect_for_out;
	bool rasterize;
	bool textaspaths;
	Laxkit::DoubleBBox crop;

	Document *doc;
	Group *limbo;
	char *filename;
	char *tofiles;
	char *custom_command;
	PaperGroup *papergroup;

	ExportFilter *filter;

	DocumentExportConfig();
	DocumentExportConfig(DocumentExportConfig *config);
	DocumentExportConfig(Document *ndoc, Group *lmbo, const char *file, const char *to,
						 int l,int s,int e,PaperGroup *group);
	virtual ~DocumentExportConfig();

	virtual void CopySource(DocumentExportConfig *config);

	virtual ObjectDef* makeObjectDef();
	virtual Value* duplicate();

	virtual Value *dereference(const char *extstring, int len);
	virtual int assign(FieldExtPlace *ext,Value *v);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute * dump_out_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};

//------------------------------- export_document() ----------------------------------

int export_document(DocumentExportConfig *config, Laxkit::ErrorLog &log);

//------------------------------ ImportConfig ----------------------------
ObjectDef *makeImportConfigDef();
int createImportConfig(ValueHash *context, ValueHash *parameters,
					   Value **value_ret, Laxkit::ErrorLog &log);

class ImportConfig : public Value
{
 public:
	char *filename;
	int keepmystery;
	Laxkit::IndexRange range;
	int topage,spread,layout;
	int scaletopage;
	Document *doc;
	Group *toobj;
	ImportFilter *filter;
	double dpi;

	ImportConfig();
	ImportConfig(const char *file, double ndpi, int ins, int ine, int top, int spr, int lay,
				 Document *ndoc, Group *nobj);
	virtual ~ImportConfig();

	virtual ObjectDef* makeObjectDef();
	virtual Value* duplicate();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute * dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};

//------------------------------- import_document() ----------------------------------

int import_document(ImportConfig *config, Laxkit::ErrorLog &log, const char *filecontents,int contentslen);


} // namespace Laidout

#endif


