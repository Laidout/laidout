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
// Copyright (c) 2007-2010 Tom Lechner
//
#ifndef IMPORTIMAGES_H
#define IMPORTIMAGES_H

#include <lax/filedialog.h>
#include <lax/imageinfo.h>
#include <lax/lists.h>
#include <lax/dump.h>

#include "importimage.h"
#include "document.h"



namespace Laidout {


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
	ImportImageSettings *settings;
	Document *doc;
	Group *toobj;
	Laxkit::MenuSelector *reviewlist;

	ImportImagesDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			unsigned long nowner,const char *nsend,
			const char *nfile,const char *npath,const char *nmask,
			Group *obj,
			Document *ndoc,int startpg,double defdpi);
	virtual ~ImportImagesDialog();
	virtual const char *whattype() { return "ImportImagesDialog"; }
	virtual int init();
	virtual int Event(const Laxkit::EventData *data,const char *mes);
	virtual int send(int id);
	virtual void SetFile(const char *f,const char *pfile);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


} // namespace Laidout

#endif


