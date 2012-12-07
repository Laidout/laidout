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
	stylemanager.AddStyleDef(makeReimposeStyleDef(),1);
	stylemanager.AddStyleDef(makeOpenStyleDef(),1);
	stylemanager.AddStyleDef(makeNewDocumentStyleDef(),1);
	stylemanager.AddStyleDef(makeImportStyleDef(),1);
	stylemanager.AddStyleDef(makeExportStyleDef(),1);
	
	return stylemanager.functions.n;
}

int InitObjectDefinitions()
{
	stylemanager.AddStyleDef(makePaperStyleDef(),1);

	return stylemanager.styledefs.n;
}

} // namespace Laidout

