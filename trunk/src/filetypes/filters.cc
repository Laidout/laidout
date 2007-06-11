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
// Copyright (C) 2007 by Tom Lechner
//

#include "../laidout.h"

#include "ppt.h"
//#include "svg.h"
//#include "scribus.h"
//#include "pdf.h"
//#include "ps.h"

void installFilters()
{
	installPptFilter();
	//installSvgFilter();
	//installScribusFilter();
	//installPdfFilter();
	//installPsFilter();
	//installEpsFilter();
}
