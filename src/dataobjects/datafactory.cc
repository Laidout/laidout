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
#include "lengraverfilldata.h"
#include "lcaptiondata.h"
#include "ltextonpath.h"
#include "lvoronoidata.h"

#include <lax/interfaces/interfacemanager.h>

#include <lax/lists.cc>

#include <iostream>
#define DBG
using namespace std;

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {



//---------------------------- Group --------------------------------

//! For somedatafactory.
Laxkit::anObject *createGroup(int p, Laxkit::anObject *refobj)
{
	return new DrawableObject();
}


//---------------------------- MysteryData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createMysteryData(int p, Laxkit::anObject *refobj)
{
	return new MysteryData();
}


//---------------------------- LImageData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLImageData(int p, Laxkit::anObject *refobj)
{
	return new LImageData();
}


//---------------------------- LPathsData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLPathsData(int p, Laxkit::anObject *refobj)
{
	return new LPathsData();
}


////---------------------------- LGradientData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLGradientData(int p, Laxkit::anObject *refobj)
{
	return new LGradientData();
}



//---------------------------- LImagePatchData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLImagePatchData(int p, Laxkit::anObject *refobj)
{
	return new LImagePatchData();
}


//---------------------------- LColorPatchData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLColorPatchData(int p, Laxkit::anObject *refobj)
{
	return new LColorPatchData();
}


//---------------------------- LSomeDataRef --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLSomeDataRef(int p, Laxkit::anObject *refobj)
{
	return new LSomeDataRef();
}

//---------------------------- LEngraverFillData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLEngraverFillData(int p, Laxkit::anObject *refobj)
{
	return new LEngraverFillData();
}


//---------------------------- LCaptionData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLCaptionData(int p, Laxkit::anObject *refobj)
{
	return new LCaptionData();
}


//---------------------------- LTextOnPath --------------------------------

//! For somedatafactory.
Laxkit::anObject *createLTextOnPath(int p, Laxkit::anObject *refobj)
{
	return new LTextOnPath();
}


//---------------------------- VoronoiData --------------------------------

//! For somedatafactory.
Laxkit::anObject *createVoronoiData(int p, Laxkit::anObject *refobj)
{
	return new LVoronoiData();
}


//---------------------------- SomeDataFactory Setup --------------------------


void InitializeDataFactory()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	ObjectFactory *lobjectfactory = imanager->GetObjectFactory();

	lobjectfactory->DefineNewObject(LAX_GROUPDATA,       "Group",           createGroup,            NULL, 0);
	lobjectfactory->DefineNewObject(LO_MYSTERYDATA,      "MysteryData",     createMysteryData,      NULL, 0);
	lobjectfactory->DefineNewObject(LAX_IMAGEDATA,       "ImageData",       createLImageData,       NULL, 0);
	lobjectfactory->DefineNewObject(LAX_PATHSDATA,       "PathsData",       createLPathsData,       NULL, 0);
	lobjectfactory->DefineNewObject(LAX_GRADIENTDATA,    "GradientData",    createLGradientData,    NULL, 0);
	lobjectfactory->DefineNewObject(LAX_IMAGEPATCHDATA,  "ImagePatchData",  createLImagePatchData,  NULL, 0);
	lobjectfactory->DefineNewObject(LAX_COLORPATCHDATA,  "ColorPatchData",  createLColorPatchData,  NULL, 0);
	lobjectfactory->DefineNewObject(LAX_SOMEDATAREF,     "SomeDataRef",     createLSomeDataRef,     NULL, 0);
	lobjectfactory->DefineNewObject(LAX_ENGRAVERFILLDATA,"EngraverFillData",createLEngraverFillData,NULL, 0);
	lobjectfactory->DefineNewObject(LAX_CAPTIONDATA,     "CaptionData",     createLCaptionData,     NULL, 0);
	lobjectfactory->DefineNewObject(LAX_TEXTONPATH,      "TextOnPath",      createLTextOnPath,      NULL, 0);

	 //experimental:
	lobjectfactory->DefineNewObject(LAX_VORONOIDATA,     "VoronoiData",     createVoronoiData,      NULL, 0);

	DBG lobjectfactory->dump_out(stderr,0);
}


} //namespace Laidout

