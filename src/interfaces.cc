//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/**** laidout/src/interfaces.cc *****/


#include "interfaces.h"
#include "laidout.h"
#include <lax/lists.cc>
#include <lax/interfaces/imageinterface.h>
#include "dataobjects/limagepatch.h"
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/bezpathoperator.h>
#include "dataobjects/groupinterface.h"
#include "dataobjects/epsdata.h"


using namespace Laxkit;
using namespace LaxInterfaces;

//! Push any necessary PathOperator instances onto PathInterface::basepathops
void PushBuiltinPathops()
{
	PathInterface::basepathops.push(new BezpathOperator(NULL,1,NULL),1);
}

//! Get the built in interfaces. NOTE: Must be called after GetBuiltinPathops().
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

	ImageInterface *imagei=new ImageInterface(id++,NULL);
	imagei->style=1;
	existingpool->push(imagei,1);
	
	EpsInterface *eps=new EpsInterface(id++,NULL);
	eps->style=1;
	existingpool->push(eps,1);//*** combine with Image somehow?

	LImagePatchInterface *ip=new LImagePatchInterface(id++,NULL);
	ip->recurse=2;
	existingpool->push(ip,1);
	
	existingpool->push(new PathInterface(id++,NULL,NULL),1); //2nd null is pathop pool
	
	GradientInterface *gi=new GradientInterface(id++,NULL);
	gi->createv=flatpoint(1,0);
	gi->creater1=gi->creater2=1;
	existingpool->push(gi,1);
	
	existingpool->push(new ColorPatchInterface(id++,NULL),1);
	
	existingpool->push(new GroupInterface(id++,NULL),1);
	
	//...
	//existingpool->push(new Interface(*****),1);
	
	return existingpool;
}
