//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/**** laidout/src/interfaces.cc *****/


#include "interfaces.h"
#include "laidout.h"
#include <lax/lists.cc>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/bezpathoperator.h>
#include <lax/interfaces/rectinterface.h>


using namespace Laxkit;
using namespace LaxInterfaces;

//! Push any necessary PathOperator instances onto PathInterface::basepathops
void PushBuiltinPathops()
{
	PathInterface::basepathops.push(new BezpathOperator(NULL,1,NULL),1);
}

//! Get the built in interfaces. NOTE: Must be called after GetBuiltinPathops().
/*! The PathInterface requires that pathoppool be filled already.
 */
PtrStack<anInterface> *GetBuiltinInterfaces(PtrStack<anInterface> *existingpool) //existingpool=NULL
{
	if (!existingpool) { // create new pool if you are not appending to an existing one.
		existingpool=new PtrStack<anInterface>;
	}

	existingpool->push(new ImageInterface(2,NULL),1);
	existingpool->push(new PathInterface(1,NULL,NULL),1); //2nd null is pathop pool
	GradientInterface *gi=new GradientInterface(3,NULL);
	gi->createv=flatpoint(1,0);
	gi->creater1=gi->creater2=1;
	existingpool->push(gi,1);
	existingpool->push(new ColorPatchInterface(4,NULL),1);
	existingpool->push(new RectInterface(5,NULL),1);
	//existingpool->push(new Interface(*****),1);
	//existingpool->push(new Interface(*****),1);
	
	return existingpool;
}
