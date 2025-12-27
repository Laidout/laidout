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
// Copyright (C) 2007 by Tom Lechner
//

#include "../laidout.h"

#include "ppt.h"
#include "postscript.h"
#include "svg.h"
#include "image.h"
#include "image-gs.h"
#include "scribus.h"
#include "pdf.h"
#include "htmlgallery.h"
#include "podofoimposeplan.h"
#include "pdfpodofo.h"
#include "laidoutimport.h"
#include "pageatlas.h"


namespace Laidout {


//! Just call each installWhatever() for the filters defined in other files.
void installFilters()
{
	installPdfFilter();
	installSvgFilter();
	installScribusFilter();
	installLaidoutFilter();
	installImageFilter();
	installImageGsFilter();
	InstallHtmlFilter();
	installPodofoPlanFilter();
	installPodofoFilters();
	installPptFilter();
	installPostscriptFilters();
	installPageAtlasFilter();
}


} // namespace Laidout

