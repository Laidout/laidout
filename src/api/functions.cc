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
// Copyright (C) 2009-2013 by Tom Lechner
//

#include "functions.h"

#include "reimpose.h"
#include "openandnew.h"
#include "importexport.h"
#include "buildicons.h"

#include "../core/stylemanager.h"
#include "../core/papersizes.h"
#include "../core/page.h"
#include "../dataobjects/bboxvalue.h"

#include "../language.h"
#include "../laidout.h"

namespace Laidout {


//! Initialize available functions in stylemanager.
/*! Return the number of functions added.
 */
int InitFunctions()
{
	stylemanager.AddObjectDef(makeReimposeObjectDef(),1);
	stylemanager.AddObjectDef(makeOpenObjectDef(),1);
	stylemanager.AddObjectDef(makeNewDocumentObjectDef(),1);
	stylemanager.AddObjectDef(makeImportObjectDef(),1);
	stylemanager.AddObjectDef(makeExportObjectDef(),1);
	stylemanager.AddObjectDef(makeBuildIconsDef(),1);
	
	return stylemanager.getNumFields();
}

int InitObjectDefinitions()
{
	PaperStyle paper;
	paper.GetObjectDef();

	PageStyle ps;
	ps.GetObjectDef();
	
	RectPageStyle rps(RECTPAGE_LRTB);
	rps.GetObjectDef();


	//stylemanager.AddObjectDef(makeAffineObjectDef(),1);
	stylemanager.AddObjectDef(makeBBoxObjectDef(),1);
	stylemanager.pushVariable("laidout",_("Laidout"),_("Main Laidout container"),NULL,0,laidout,0);
	

	return stylemanager.getNumFields();
}

} // namespace Laidout

