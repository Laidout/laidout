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
#ifndef IMPORTIMAGES_H
#define IMPORTIMAGES_H

#include <lax/filedialog.h>
#include <lax/imageinfo.h>
#include <lax/lists.h>
#include <lax/dump.h>
#include "document.h"


class ImportImagesDialog : public Laxkit::FileDialog
{
 protected:
	int curitem;
	Laxkit::PtrStack<Laxkit::ImageInfo> images;
	virtual void rebuildPreviewName();
	virtual void updateFileList();
	virtual Laxkit::ImageInfo *findImageInfo(const char *fullfile,int *i=NULL);
	virtual char *getPreviewFileName(const char *full);
 public:
	double dpi;
	int startpage;
	Document *doc;
	ImportImagesDialog(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			Window nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Document *ndoc,int startpg,double defdpi);
	virtual ~ImportImagesDialog();
	virtual const char *whattype() { return "ImportImagesDialog"; }
	virtual int init();
	virtual int ClientEvent(XClientMessageEvent *e,const char *mes);
	virtual int DataEvent(Laxkit::EventData *data,const char *mes);
	virtual int send();
	virtual void SetFile(const char *f,const char *pfile);

	virtual void dump_out(FILE *f,int indent,int what);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
};


#endif


