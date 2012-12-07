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
// Copyright (C) 2004-2012 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../stylemanager.h"
#include "../headwindow.h"
#include "../impositions/singles.h"
#include "postscript.h"
#include "../printing/psout.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;



namespace Laidout {


//--------------------------------- install postscript filters

//! Tells the Laidout application that there's a new filter in town.
void installPostscriptFilters()
{
	 //--------export
	PsOutFilter *psout=new PsOutFilter;
	psout->GetStyleDef();
	laidout->PushExportFilter(psout);
	
	EpsOutFilter *epsout=new EpsOutFilter;
	epsout->GetStyleDef();
	laidout->PushExportFilter(epsout);
	
	
	 //--------import
	//EpsInFilter *epsin=new EpsInFilter;
	//laidout->importfilters.push(epsin);
}


//------------------------------------ PsExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Style *newPsExportConfig(StyleDef*)
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"Postscript"))
			d->filter=laidout->exportfilters.e[c];
	}
	return d;
}


//------------------------------------ EpsExportConfig ----------------------------------

//! For now, just returns a new DocumentExportConfig.
Style *newEpsExportConfig(StyleDef*)
{
	DocumentExportConfig *d=new DocumentExportConfig;
	for (int c=0; c<laidout->exportfilters.n; c++) {
		if (!strcmp(laidout->exportfilters.e[c]->Format(),"EPS"))
			d->filter=laidout->exportfilters.e[c];
	}
	return d;
}


//------------------------------- PsOutFilter --------------------------------------
/*! \class PsOutFilter
 * \brief Output filter for Postscript LL3 files.
 */

PsOutFilter::PsOutFilter()
{
	flags=FILTER_MULTIPAGE;
}

const char *PsOutFilter::VersionName()
{
	return _("Postscript LL3");
}

//! Save the document as a Postscript file.
int PsOutFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	return psout(filename,context,log);
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
StyleDef *PsOutFilter::GetStyleDef()
{
	StyleDef *styledef;
	styledef=stylemanager.FindDef("PsExportConfig");
	if (styledef) return styledef; 

	styledef=makeStyleDef();
	makestr(styledef->name,"PsExportConfig");
	makestr(styledef->Name,_("Postscript Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to a postscript file."));
	styledef->newfunc=newPsExportConfig;

	stylemanager.AddStyleDef(styledef);
	styledef->dec_count();

	return styledef;
}

//------------------------------- EpsOutFilter --------------------------------------
/*! \class EpsOutFilter
 * \brief Output filter for EPS 3.0 files.
 */

const char *EpsOutFilter::VersionName()
{
	return _("EPS 3.0");
}


//! Save the document as an EPS file.
/*!
 */
int EpsOutFilter::Out(const char *filename, Laxkit::anObject *context, ErrorLog &log)
{
	return epsout(filename,context,log);
}

//! Try to grab from stylemanager, and install a new one there if not found.
/*! The returned def need not be dec_counted.
 */
StyleDef *EpsOutFilter::GetStyleDef()
{
	StyleDef *styledef;
	styledef=stylemanager.FindDef("EpsExportConfig");
	if (styledef) return styledef; 

	styledef=makeStyleDef();
	makestr(styledef->name,"EpsExportConfig");
	makestr(styledef->Name,_("EPS Export Configuration"));
	makestr(styledef->description,_("Configuration to export a document to an EPS file."));
	styledef->newfunc=newEpsExportConfig;

	stylemanager.AddStyleDef(styledef);
	styledef->dec_count();

	return styledef;
}



} // namespace Laidout

