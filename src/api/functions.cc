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
// Copyright (C) 2009 by Tom Lechner
//

#include "functions.h"

#include "../stylemanager.h"
#include "reimpose.h"
#include "openandnew.h"
#include "importexport.h"

#include "../papersizes.h"


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
	stylemanager.AddObjectDef(makePaperStyleDef(),1);

	return stylemanager.getNumFields();
}

} // namespace Laidout

