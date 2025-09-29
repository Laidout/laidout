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
#include <lax/interfaces/delaunayinterface.h>
#include <lax/interfaces/ellipseinterface.h>
#include <lax/interfaces/roundedrectinterface.h>

#include "../nodes/nodeinterface.h"
#include "../interfaces/objectfilterinterface.h"

//experimental:
#include <lax/interfaces/perspectiveinterface.h>
#include <lax/interfaces/pressuremapinterface.h>
#include <lax/interfaces/beznetinterface.h>
#include "../text/streaminterface.h"
#include "../interfaces/anchorinterface.h"
#include "../interfaces/animationinterface.h"
#include "../interfaces/pathintersectionsinterface.h"
#include "../dataobjects/lsimplepath.h"
#include "../interfaces/partitioninterface.h"


#include "interfaces.h"
#include "../laidout.h"
#include "../dataobjects/limagepatch.h"

#include "../dataobjects/datafactory.h"
#include "../dataobjects/groupinterface.h"
#include "../dataobjects/mysterydata.h"
#include "../dataobjects/limageinterface.h"
#include "../dataobjects/limagedata.h"
#include "../dataobjects/lgradientdata.h"
#include "../dataobjects/lpathsdata.h"
#include "../dataobjects/lcaptiondata.h"
#include "../dataobjects/ltextonpath.h"
#include "../dataobjects/lengraverfilldata.h"
#include "../dataobjects/lvoronoidata.h"

#include "../dataobjects/imagevalue.h"

#include "../interfaces/aligninterface.h"
#include "../interfaces/nupinterface.h"
#include "../interfaces/pagerangeinterface.h"
#include "../interfaces/objectindicator.h" 
#include "../interfaces/paperinterface.h"
#include "../interfaces/graphicalshell.h"
#include "../interfaces/cloneinterface.h"
#include "../interfaces/pagemarkerinterface.h"


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


	if (laidout->prefs.experimental) {
		// *************** testing:

		 // pathintersections
		i=new PathIntersectionsInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Animation (overlay)
		i=new AnimationInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------StreamInterface
		i = new StreamInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Perspective
		i=new PerspectiveInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Anchor
		i=new AnchorInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		 //------Pressure Map
		i = new PressureMapInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		//------BezNetInterface
		i = new BezNetInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		//----SimplePathInterface
		i = new SimplePathInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		//------BezNetInterface
		i = new BezNetInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		//------PartitionInterface
		i = new PartitionInterface(NULL,id++,NULL);
		tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
		existingpool->push(i);
		i->dec_count();

		// *************** end experimental
	}



	 //------Group
	Group group;
	group.GetObjectDef();
	i=new GroupInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();
	
	 //------Images
	ImageValue iv;
	iv.GetObjectDef();
	LImageData lid;
	lid.GetObjectDef();
	LImageInterface *imagei=new LImageInterface(id++,NULL);
	tools->AddResource("tools", imagei, NULL, imagei->whattype(), imagei->Name(), NULL,NULL,NULL);
	imagei->style=1;
	existingpool->push(imagei);
	imagei->dec_count();
	
	 //------Gradients
	GradientValue gv;  gv.GetObjectDef();
	LGradientData lgd; lgd.GetObjectDef();
	LGradientInterface *gi = new LGradientInterface(id++,NULL);
	tools->AddResource("tools", gi, NULL, gi->whattype(), gi->Name(), NULL,NULL,NULL);
	gi->createv=flatpoint(1,0);
	gi->creater1=gi->creater2=1;
	existingpool->push(gi);
	gi->dec_count();
	
	 //------Paths
	LPathsData pdata;
	pdata.GetObjectDef();
	i=new LPathInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	i->InitializeResources();
	existingpool->push(i);
	i->dec_count();

	//------Ellipse
	EllipseInterface *ei = new EllipseInterface(NULL,id++,NULL);
	ei->linestyle.width = .1;
	i = ei;
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	//------Rounded Rectangle
	RoundedRectInterface *rri = new RoundedRectInterface(NULL,id++,NULL);
	//rri->linestyle.width = .1;
	i = rri;
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //-----Caption
	LCaptionData caption;
	caption.GetObjectDef();
	i=new CaptionInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();
		
	 //------TextOnPath
	LTextOnPath tonpath;
	tonpath.GetObjectDef();
	i=new TextOnPathInterface(NULL,id++);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();

	 //------Color Patch
	LColorPatchData cpatch;
	cpatch.GetObjectDef();
	i=new LColorPatchInterface(id++,NULL);
	tools->AddResource("tools", i, NULL, i->whattype(), i->Name(), NULL,NULL,NULL);
	existingpool->push(i);
	i->dec_count();
	
	 //------Image Patch
	LImagePatchData ipatch;
	ipatch.GetObjectDef();
	LImagePatchInterface *ip=new LImagePatchInterface(id++,NULL);
	tools->AddResource("tools", ip, NULL, ip->whattype(), ip->Name(), NULL,NULL,NULL);
	ip->style=IMGPATCHI_POPUP_INFO;
	ip->recurse=2;
	existingpool->push(ip);
	ip->dec_count();
	
	 //-----Engraver
	LEngraverFillData engdata;
	engdata.GetObjectDef();
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

	 //------Delaunay
	LVoronoiData voronoi;
	voronoi.GetObjectDef();
	i=new DelaunayInterface(NULL,id++,NULL);
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

