//
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
// Copyright (C) 2022 by Tom Lechner
//
//

#include "masterpages.h"


namespace Laidout {

//----------------------------- MasterPage -------------------------------

/*! \class MasterPage
 */

MasterPage::MasterPage()
{
}

MasterPage::~MasterPage()
{
}

void MasterPage::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
}

void MasterPage::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
}




//----------------------------- Document MasterPage stuff-------------------------------


Spread *Document::MasterPageSpread()
{
	//             Left Page      Right Page     Any
	//             ---------      ---------     ----
	// Set Name      [   ]           < + >       < + >
	//   < + >       < + >           < + >       < + >


	for (int c=0; c<pagetypes.n; c++) {
		*** one CaptionData per page type as header
	}

	for (int c=0; c<masterpagesets.n; c++) {
		*** CaptionData with set name

		for (int c=0; c<pagetypes.n; c++) {
			*** one CaptionData per page type as header
		}
	}
}

//...for each Page in Document:
	PtrStack<MasterPageInfo> masterstack;

class MasterPageInfo
{
  public:
	bool visible;
	MasterPageSet *master_set; //if !=NULL, update master_page when page type changes for this page
	MasterPage *master_page; //if NULL, needs to be set from appropriate one for master_set

	Group *objects; //could just be refs, but could be dynamic objects only based on original master page objects

	//if master_set==NULL and master_page==NULL, then this info object is a standin for actual page data
};


} // namespace Laxkit

