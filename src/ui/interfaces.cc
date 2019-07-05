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
// Copyright (C) 2005-2016 by Tom Lechner
//

#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/characterinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/engraverfillinterface.h>
#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/textonpathinterface.h>
#include <lax/interfaces/delauneyinterface.h>

#include "../nodes/nodeinterface.h"
#include "../interfaces/objectfilterinterface.h"

//experimental:
#include <lax/interfaces/perspectiveinterface.h>
#include <lax/interfaces/textstreaminterface.h>
#include <lax/interfaces/ellipseinterface.h>
#include "../interfaces/anchorinterface.h"
#include "../interfaces/animationinterface.h"


#include "interfaces.h"
#include "../laidout.h"
#include "../dataobjects/limagepatch.h"

#include "../dataobjects/datafactory.h"
#include "../dataobjects/groupinterface.h"
#include "../dataobjects/mysterydata.h"
#include "../dataobjects/limageinterface.h"
#include "../dataobjects/lgradientdata.h"
#include "../dataobjects/lpathsdata.h"

#include "../interfaces/aligninterface.h"
#include "../interfaces/nupinterface.h"
#include "../interfaces/pagerangeinterface.h"
#include "../interfaces/objectindicator.h" 
#include "../interfaces/paperinterface.h"
#include "../interfaces/graphicalshell.h"
#include "../interfaces/cloneinterface.h"
#include "../interfaces/pagemarkerinterface.h"


//template implementation:
#include <lax/lists.cc>


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
 */
RefPtrStack<anInterface> *GetBuiltinInterfaces(RefPtrStack<anInterface> *existingpool) //existingpool=NULL
{
	if (!existingpool) { // create new pool if you are not appending to an existing one.
		existingpool=new RefPtrStack<anInterface>;
	}


	InitializeDataFactory();


	int id=1;
	anInterface *i;

	//-----------------new fangled InterfaceManager tools-----------------
	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	ResourceManager *tools = imanager->GetTools();


	// ******* TODo: remove existingpool stuff in favor of interfacemanager->tools

	//-----------------end new fangled InterfaceManager tools-----------------


	if (laidout->experimental) {
		// *************** testing:

		 //------Animation
		i=new AnimationInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------TextStream
		i=new TextStreamInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Perspective
		i=new PerspectiveInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Ellipse
		i=new EllipseInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Anchor
		i=new AnchorInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		// *************** end testing
	}


	 //------Group
	i=new GroupInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();
	
	 //------Images
	LImageInterface *imagei=new LImageInterface(id++,NULL);
	tools->AddResource("tools", imagei, NULL, imagei->whattype(), imagei->Name(), NULL,NULL,NULL);
	imagei->style=1;
	existingpool->push(imagei);
	imagei->dec_count();
	
	 //------Gradients
	LGradientInterface *gi=new LGradientInterface(id++,NULL);
	tools->AddResource("tools", gi, NULL, gi->whattype(), gi->Name(), NULL,NULL,NULL);
	gi->createv=flatpoint(1,0);
	gi->creater1=gi->creater2=1;
	existingpool->push(gi);
	gi->dec_count();
	
	 //------Paths
	i=new LPathInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i); //2nd null is pathop pool
	i->dec_count();

	 //-----Caption
	i=new CaptionInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();
		
	 //------TextOnPath
	i=new TextOnPathInterface(NULL,id++);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------Color Patch
	i=new LColorPatchInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();
	
	 //------Image Patch
	LImagePatchInterface *ip=new LImagePatchInterface(id++,NULL);
	tools->AddResource("tools", ip, NULL, ip->whattype(), ip->Name(), NULL,NULL,NULL);
	ip->style=IMGPATCHI_POPUP_INFO;
	ip->recurse=2;
	existingpool->push(ip);
	ip->dec_count();
	
	 //-----Engraver
	i=new EngraverFillInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	i->InitializeResources();
	existingpool->push(i);
	i->dec_count();

	 //------Clone tiler
	i=new CloneInterface(NULL,id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------Freehand
	i=new FreehandInterface(NULL,id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------Paper
	i=new PaperInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------Nodes
	i=new NodeInterface(NULL,id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->InitializeResources();
	i->dec_count();

	 //------ObjectFilters
	i=new ObjectFilterInterface(NULL,id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------Delauney
	i=new DelauneyInterface(NULL,id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();


	//------------------------Overlays---------------

	 //------PageMarkerInterface
	i=new PageMarkerInterface(NULL,id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------ObjectIndicator
	i=new ObjectIndicator(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------PageRangeInterface
	i=new PageRangeInterface(id++,NULL,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------GraphicalShell
	i=new GraphicalShell(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	
	return existingpool;
}


} // namespace Laidout

