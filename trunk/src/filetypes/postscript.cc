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
// Copyright (C) 2004-2007 by Tom Lechner
//


#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include <lax/attributes.h>

#include "../language.h"
#include "../laidout.h"
#include "../headwindow.h"
#include "../impositions/impositioninst.h"
#include "postscript.h"
#include "../printing/psout.h"

#include <iostream>
#define DBG 

using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


//--------------------------------- install postscript filters

//! Tells the Laidout application that there's a new filter in town.
void installPostscriptFilters()
{
	 //--------export
	PsOutFilter *psout=new PsOutFilter;
	laidout->exportfilters.push(psout);
	
	EpsOutFilter *epsout=new EpsOutFilter;
	laidout->exportfilters.push(epsout);
	
	
	 //--------import
	//EpsInFilter *epsin=new EpsInFilter;
	//laidout->importfilters.push(epsin);
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
int PsOutFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	return psout(filename,context,error_ret);
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
int EpsOutFilter::Out(const char *filename, Laxkit::anObject *context, char **error_ret)
{
	return epsout(filename,context,error_ret);
}

//////------------------------------------- EpsInFilter -----------------------------------
///*! \class EpsInFilter 
// * \brief EPS input filter.
// *
// * would have config to import as image or break apart into components
// */
//
//
//const char *EpsInFilter::FileType(const char *first100bytes)
//{ ***
//}
//
////! Import an EPS file.
///*! If doc!=NULL, then import the pptout files to Document starting at page startpage.
// */
//int EpsInFilter::In(const char *file, Laxkit::anObject *context, char **error_ret)
//{***
//}



