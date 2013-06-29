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
// Copyright (C) 2009-2013 by Tom Lechner
//

#include "functions.h"

#include "../stylemanager.h"
#include "reimpose.h"
#include "openandnew.h"
#include "importexport.h"

#include "../papersizes.h"
#include "../page.h"

#include "../language.h"
#include "../laidout.h"

namespace Laidout {


//! Initialize available functions in stylemanager.
/*! Return the number of functions added.
 */
int InitFunctions()
{
	stylemanager.AddObjectDef(makeReimposeStyleDef(),1);
	stylemanager.AddObjectDef(makeOpenStyleDef(),1);
	stylemanager.AddObjectDef(makeNewDocumentStyleDef(),1);
	stylemanager.AddObjectDef(makeImportStyleDef(),1);
	stylemanager.AddObjectDef(makeExportStyleDef(),1);
	
	return stylemanager.getNumFields();
}

int InitObjectDefinitions()
{
	stylemanager.AddObjectDef(makePaperObjectDef(),1);

	PageStyle *ps=new PageStyle;
	StyleDef *sd=ps->makeStyleDef();
	stylemanager.AddObjectDef(sd,1);
	delete ps;
	ps=new RectPageStyle(RECTPAGE_LRTB);
	sd=ps->makeStyleDef();
	stylemanager.AddObjectDef(sd,1);
	delete ps;


	stylemanager.AddObjectDef(makeAffineObjectDef(),1);
	stylemanager.AddObjectDef(makeBBoxObjectDef(),1);
	stylemanager.pushVariable("laidout",_("Laidout"),_("Main Laidout container"),NULL,0,laidout,0);
	

	return stylemanager.getNumFields();
}

} // namespace Laidout

