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
// Copyright (C) 2012 by Tom Lechner
//

#include "datafactory.h"
#include "drawableobject.h"
#include "mysterydata.h"
#include "limagedata.h"
#include "lpathsdata.h"
#include "lgradientdata.h"
#include "limagepatch.h"
#include "lsomedataref.h"

#include <lax/lists.cc>

#include <iostream>
#define DBG
using namespace std;

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {



//---------------------------- Group --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createGroup(LaxInterfaces::SomeData *refobj)
{
	return new DrawableObject(refobj);
}


//---------------------------- MysteryData --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createMysteryData(LaxInterfaces::SomeData *refobj)
{
	return new MysteryData();
}


//---------------------------- LImageData --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createLImageData(LaxInterfaces::SomeData *refobj)
{
	return new LImageData();
}


//---------------------------- LPathsData --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createLPathsData(LaxInterfaces::SomeData *refobj)
{
	return new LPathsData();
}


////---------------------------- LGradientData --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createLGradientData(LaxInterfaces::SomeData *refobj)
{
	return new LGradientData();
}



//---------------------------- LImagePatchData --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createLImagePatchData(LaxInterfaces::SomeData *refobj)
{
	return new LImagePatchData();
}


//---------------------------- LColorPatchData --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createLColorPatchData(LaxInterfaces::SomeData *refobj)
{
	return new LColorPatchData();
}


//---------------------------- LSomeDataRef --------------------------------

//! For somedatafactory.
LaxInterfaces::SomeData *createLSomeDataRef(LaxInterfaces::SomeData *refobj)
{
	return new LSomeDataRef();
}



//---------------------------- SomeDataFactory Setup --------------------------

SomeDataFactory lobjectfactory;

void InitializeDataFactory()
{
	LaxInterfaces::somedatafactory=&lobjectfactory;

	lobjectfactory.DefineNewObject(LO_GROUP,          "Group",         createGroup,NULL);
	lobjectfactory.DefineNewObject(LO_MYSTERYDATA,    "MysteryData",   createMysteryData,NULL);
	lobjectfactory.DefineNewObject(LAX_IMAGEDATA,     "ImageData",     createLImageData,NULL);
	lobjectfactory.DefineNewObject(LAX_PATHSDATA,     "PathsData",     createLPathsData,NULL);
	lobjectfactory.DefineNewObject(LAX_GRADIENTDATA,  "GradientData",  createLGradientData,NULL);
	lobjectfactory.DefineNewObject(LAX_IMAGEPATCHDATA,"ImagePatchData",createLImagePatchData,NULL);
	lobjectfactory.DefineNewObject(LAX_COLORPATCHDATA,"ColorPatchData",createLColorPatchData,NULL);
	lobjectfactory.DefineNewObject(LAX_COLORPATCHDATA,"ColorPatchData",createLColorPatchData,NULL);
	lobjectfactory.DefineNewObject(LAX_SOMEDATAREF,   "SomeDataRef",   createLSomeDataRef,NULL);
}


} //namespace Laidout

