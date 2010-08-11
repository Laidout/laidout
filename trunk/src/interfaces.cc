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
// Copyright (C) 2005-2010 by Tom Lechner
//


#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/lists.cc>

#include "interfaces.h"
#include "laidout.h"
#include "dataobjects/limagepatch.h"

#include "dataobjects/groupinterface.h"
#include "dataobjects/epsdata.h"
#include "dataobjects/mysterydata.h"
#include "interfaces/paperinterface.h"

using namespace Laxkit;
using namespace LaxInterfaces;

//! Push any necessary PathOperator instances onto PathInterface::basepathops
void PushBuiltinPathops()
{
}

////---------------------
///*! \class LImageInterface
// * \brief add on a little custom behavior.
// * \todo *** move this somewhere more appropriate
// */
//class LImageInterface : public LaxInterfaces::ImageInterface
//{
// protected:
//	virtual void runImageDialog();
// public:
//	LImageInterface(int nid,Displayer *ndp);
//};
//
//LImageInterface::LImageInterface(int nid,Displayer *ndp) : ImageInterface(nid,ndp)
//{
//	style=1;
//}
//
///*! Redefine to blot out title from the dialog.
// */
//void LImageInterface::runImageDialog()
//{
//	 //after Laxkit event system is rewritten, this will be very different:
//	ImageInfo *inf=new ImageInfo(data->filename,data->previewfile,NULL,data->desc,0);
//	curwindow->app->rundialog(new ImageDialog(NULL,"imagedialog for imageinterface",
//					ANXWIN_DELETEABLE|IMGD_NO_TITLE,
//					0,0,400,400,0,
//					NULL,curwindow->window,"image properties",
//					inf));
//	inf->dec_count();
//}
////---------------------


//! Get the built in interfaces. NOTE: Must be called after PushBuiltinPathops().
/*! The PathInterface requires that pathoppool be filled already.
 *
 * \todo combine with EpsInterface with ImageInterface somehow to make easily expandable..
 */
PtrStack<anInterface> *GetBuiltinInterfaces(PtrStack<anInterface> *existingpool) //existingpool=NULL
{
	if (!existingpool) { // create new pool if you are not appending to an existing one.
		existingpool=new PtrStack<anInterface>;
	}

	int id=1;


	 //------Group
	existingpool->push(new GroupInterface(id++,NULL),1);
	
	 //------Images
	ImageInterface *imagei=new ImageInterface(id++,NULL);
	imagei->style=1;
	existingpool->push(imagei,1);
	
	 //------Gradients
	GradientInterface *gi=new GradientInterface(id++,NULL);
	gi->createv=flatpoint(1,0);
	gi->creater1=gi->creater2=1;
	existingpool->push(gi,1);
	
	 //------Image Patch
	LImagePatchInterface *ip=new LImagePatchInterface(id++,NULL);
	ip->style=IMGPATCHI_POPUP_INFO;
	ip->recurse=2;
	existingpool->push(ip,1);
	
	 //------Color Patch
	existingpool->push(new LColorPatchInterface(id++,NULL),1);
	
	 //------Paths
	existingpool->push(new PathInterface(id++,NULL),1); //2nd null is pathop pool
	
	 //------EPS
	EpsInterface *eps=new EpsInterface(id++,NULL);
	eps->style=1;
	existingpool->push(eps,1);//*** combine with Image somehow?

//	 //------MysteryData
//	MysteryInterface *mdata=new MysteryInterface(id++,NULL);
//	mdata->style=1;
//	existingpool->push(mdata,1);//*** combine with Image somehow?

	 //------Paper
	existingpool->push(new PaperInterface(id++,NULL),1);



	//...
	//existingpool->push(new Interface(*****),1);
	
	return existingpool;
}
