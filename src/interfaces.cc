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
// Copyright (C) 2005-2013 by Tom Lechner
//

#include "interfaces/aligninterface.h"
#include "interfaces/nupinterface.h"
#include "interfaces/pagerangeinterface.h"
#include "interfaces/objectindicator.h"

//#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/lists.cc>

#include "interfaces.h"
#include "laidout.h"
#include "dataobjects/limagepatch.h"

#include "dataobjects/datafactory.h"
#include "dataobjects/groupinterface.h"
#include "dataobjects/epsdata.h"
#include "dataobjects/mysterydata.h"
#include "dataobjects/limageinterface.h"
#include "dataobjects/lgradientdata.h"
#include "interfaces/paperinterface.h"
#include "interfaces/graphicalshell.h"
#include "interfaces/anchorinterface.h"


using namespace Laxkit;
using namespace LaxInterfaces;


#define DBG 



namespace Laidout {


//! Push any necessary PathOperator instances onto PathInterface::basepathops
void PushBuiltinPathops()
{
}


//! Get the built in interfaces. NOTE: Must be called after PushBuiltinPathops().
/*! The PathInterface requires that pathoppool be filled already.
 *
 * \todo combine with EpsInterface with ImageInterface somehow to make easily expandable..
 */
RefPtrStack<anInterface> *GetBuiltinInterfaces(RefPtrStack<anInterface> *existingpool) //existingpool=NULL
{
	if (!existingpool) { // create new pool if you are not appending to an existing one.
		existingpool=new RefPtrStack<anInterface>;
	}


	InitializeDataFactory();


	int id=1;
	anInterface *i;


	// *************** testing:
	//i=new CaptionInterface(id++,NULL);
	//existingpool->push(i);
	//i->dec_count();
	//
	i=new AnchorInterface(id++,NULL);
	existingpool->push(i);
	i->dec_count();
	// *************** end testing


	 //------Group
	i=new GroupInterface(id++,NULL);
	existingpool->push(i);
	i->dec_count();
	
	 //------Images
	LImageInterface *imagei=new LImageInterface(id++,NULL);
	imagei->style=1;
	existingpool->push(imagei);
	imagei->dec_count();
	
	 //------Gradients
	LGradientInterface *gi=new LGradientInterface(id++,NULL);
	gi->createv=flatpoint(1,0);
	gi->creater1=gi->creater2=1;
	existingpool->push(gi);
	gi->dec_count();
	
	 //------Image Patch
	LImagePatchInterface *ip=new LImagePatchInterface(id++,NULL);
	ip->style=IMGPATCHI_POPUP_INFO;
	ip->recurse=2;
	existingpool->push(ip);
	ip->dec_count();
	
	 //------Color Patch
	i=new LColorPatchInterface(id++,NULL);
	existingpool->push(i);
	i->dec_count();
	
	 //------Paths
	i=new PathInterface(id++,NULL);
	existingpool->push(i); //2nd null is pathop pool
	i->dec_count();

//	 //------MysteryData
//	MysteryInterface *mdata=new MysteryInterface(id++,NULL);
//	mdata->style=1;
//	existingpool->push(mdata);//*** combine with Image somehow?
//	mdata->dec_count();

	 //------Paper
	i=new PaperInterface(id++,NULL);
	existingpool->push(i);
	i->dec_count();


	//------------------------Overlays---------------

	 //------ObjectIndicator
	i=new ObjectIndicator(id++,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------PageRangeInterface
	i=new PageRangeInterface(id++,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------GraphicalShell
	i=new GraphicalShell(id++,NULL);
	existingpool->push(i);
	i->dec_count();


	//...
	//i=new Interface(*****);
	//existingpool->push(i);
	//i->dec_count();
	
	return existingpool;
}


} // namespace Laidout

