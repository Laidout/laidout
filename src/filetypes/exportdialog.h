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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <lax/lineedit.h>
#include <lax/checkbox.h>
#include <lax/messagebox.h>
#include <lax/dump.h>

#include "filefilters.h"


#define EXPORT_COMMAND (1<<16)


namespace Laidout {

	
//---------------------------- ConfigEventData -------------------------
class ConfigEventData : public Laxkit::EventData
{
 public:
	DocumentExportConfig *config;
	ConfigEventData(DocumentExportConfig *c);
	virtual ~ConfigEventData();
};

//---------------------------- ExportDialog -------------------------
class ExportDialog : public Laxkit::RowFrame
{
 protected:
	DocumentExportConfig *config;
	
	int firstextra;
	int overwriteok;
	unsigned long dialog_style;
	int tofile;
	int cur, max, min;
	char *last_meta_dir;
	Laxkit::LineEdit *fileedit,*filesedit,*printstart,*printend,*command;
	Laxkit::CheckBox *filecheck,*filescheck,*commandcheck;
	Laxkit::CheckBox *printall,*printcurrent,*printrange;
	Laxkit::CheckBox *everyspread, *evenonly, *oddonly;
	Laxkit::CheckBox *batches;
	Laxkit::CheckBox *reverse;
	Laxkit::CheckBox *rotatealternate;
	Laxkit::CheckBox *rotate0, *rotate90, *rotate180, *rotate270;
	Laxkit::LineEdit *batchnumber;
	Laxkit::CheckBox *textaspaths;
	ExportFilter *filter;

	virtual void changeToEvenOdd(DocumentExportConfig::EvenOdd t);
	virtual void changeTofile(int t);
	virtual void paperRotation(int rotation);
	virtual void changeRangeTarget(int t);
	virtual void configBounds();
	virtual void overwriteCheck();
	virtual int send();

	virtual void start(int s);
	virtual int  start();
	virtual void end(int e);
	virtual int  end();
	virtual void findMinMax();
	virtual int updateExt();
	virtual void updateEdits();

 public:
	ExportDialog(unsigned long nstyle,unsigned long nowner,const char *nsend,
				 Document *doc, 
				 Group *limbo,
				 PaperGroup *group,
				 ExportFilter *nfilter, const char *file, int layout,
				 int pmin, int pmax, int pcur);
	virtual ~ExportDialog();
	virtual const char *whattype() { return "ExportDialog"; }
	virtual int preinit();
	virtual int init();
	virtual int CharInput(unsigned int ch, unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *data,const char *mes);

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what, LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag, LaxFiles::DumpContext *context);
};


} // namespace Laidout

#endif

