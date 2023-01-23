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
	ValueHash previous_configs;
	
	int firstextra;
	int overwriteok;
	unsigned long dialog_style;
	int tofile;
	int cur, max;
	char *last_meta_dir;
	Laxkit::LineEdit *fileedit,*filesedit,*printspreadrange,*command;
	Laxkit::CheckBox *filecheck,*filescheck;
	Laxkit::CheckBox *commandcheck, *delaftercommand;
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
	virtual void SetCommandToggle();
	virtual void paperRotation(int rotation);
	virtual void changeRangeTarget(int t);
	virtual void configBounds();
	virtual void overwriteCheck();
	virtual int send();

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

	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag, Laxkit::DumpContext *context);
};


} // namespace Laidout

#endif

