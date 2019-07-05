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
// Copyright (c) 2007 Tom Lechner
//
#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <lax/filedialog.h>
#include <lax/imageinfo.h>
#include <lax/lists.h>
#include <lax/dump.h>

#include "filefilters.h"
#include "../core/document.h"



namespace Laidout {


class ImportFileDialog : public Laxkit::FileDialog
{
 protected:
 public:
	ImportConfig *config;


	Laxkit::LineInput *importpagerange;
	Laxkit::MessageBar *fileinfo;

	ImportFileDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			unsigned long nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Group *obj,
			Document *ndoc,int startpg, int spreadi, int layout, 
			double defdpi);
	virtual ~ImportFileDialog();
	virtual const char *whattype() { return "ImportFileDialog"; }
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int send(int id);
	//virtual void SetFile(const char *f,const char *pfile);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} // namespace Laidout

#endif


