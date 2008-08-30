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
#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <lax/filedialog.h>
#include <lax/imageinfo.h>
#include <lax/lists.h>
#include <lax/dump.h>
#include "document.h"


class ImportFileDialog : public Laxkit::FileDialog
{
 protected:
	int curitem;
	Laxkit::PtrStack<Laxkit::ImageInfo> images;
	virtual Laxkit::ImageInfo *findImageInfo(const char *fullfile,int *i=NULL);
	//void rebuildPreviewName();
	//virtual char *getPreviewFileName(const char *full);
 public:
	double dpi;
	int startpage;
	Document *doc;
	Group *toobj;
	Laxkit::LineInput *importpagerange;
	Laxkit::MessageBar *fileinfo;
	ImportFileDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			Window nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Group *obj,
			Document *ndoc,int startpg,double defdpi);
	virtual ~ImportFileDialog();
	virtual const char *whattype() { return "ImportFileDialog"; }
	virtual int init();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);
	virtual int send(int id);
	virtual void SetFile(const char *f,const char *pfile);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


#endif


