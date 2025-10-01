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
// Copyright (C) 2022 by Tom Lechner
//

#include "singlesinterface.h"

#include "../language.h"

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {

SinglesInterface::SinglesInterface(LaxInterfaces::anInterface *nowner,int nid,Laxkit::Displayer *ndp)
  : ImpositionInterface(nowner, nid, ndp), PaperInterface(nid, ndp)
{
	showdecs = 1;
	default_outline_color.rgbf(1., 0., 0.);
	full_menu = false;

	allow_trim_edit = true;
	allow_margin_edit = true;
	edit_trim = true;
	edit_margins = true;
}

SinglesInterface::~SinglesInterface()
{
	if (document)  document ->dec_count();
	if (papertype) papertype->dec_count();
	if (singles)   singles  ->dec_count();
}

const char *SinglesInterface::Name()
{ return _("Singles"); }

anInterface *SinglesInterface::duplicateInterface(anInterface *dup)
{
	if (dup == nullptr) dup = new SinglesInterface(nullptr,id,nullptr);
	else if (!dynamic_cast<SinglesInterface *>(dup)) return nullptr;
	
	return anInterface::duplicateInterface(dup);
}

int SinglesInterface::InterfaceOn()
{
	showdecs = 1;
	needtodraw = 1;
	return 0;
}

/*! Set default paper size */
int SinglesInterface::SetTotalDimensions(double width, double height)
{
	PaperStyle *p = new PaperStyle("Custom",width,height,0,300,NULL);
	SetPaper(p);
	p->dec_count();
	return 0;
}

/*! Return an enclosing size. */
int SinglesInterface::GetDimensions(double &width, double &height)
{
	if (!singles || !papergroup || !papergroup->papers.n) {
		width = height = 0;

	} else {
		DoubleBBox box;
		for (int c = 0; c < papergroup->papers.n; c++) {
			box.addtobounds(papergroup->papers.e[c]->m(), papergroup->papers.e[c]);
		}
		width  = box.boxwidth();
		height = box.boxheight();
	}
	return 0;
}


/*! Set paper size of current paper. Installs duplicate of paper.
 */
int SinglesInterface::SetPaper(PaperStyle *paper)
{
	singles->SetPaperSize(paper);
	return 0;
}


/*! Use the given doc, but still use the current imposition, updated with number of pages from doc.
 */
int SinglesInterface::UseThisDocument(Document *doc)
{
	if (doc != document) {
		if (document) document->dec_count();
		document = doc;
		if (document) document->inc_count();
	}

	singles->NumPages(document->pages.n);
	AdjustBoxInsets();
	needtodraw = 1;
	return 0;
}


/*! Return 1 for cannot use it, else 0.
 * Otherwise, the imposition is duplicated. If imp->doc exists, use it for this->document.
 */
int SinglesInterface::UseThisImposition(Imposition *imp)
{
	Singles *s = dynamic_cast<Singles*>(imp);
	if (!s) return 1;

	if (singles) singles->dec_count();
	singles = (Singles*)imp->duplicateValue();
	
	if (singles->NumPages() == 0) {
		singles->NumPages(1);
	}

	if (singles->doc && singles->doc != document) {
		singles->doc->inc_count();
		if (document) document->dec_count();
		document = singles->doc;
	}

	if (document) singles->NumPages(document->pages.n);
	UseThis(singles->papergroup);
	ShowThisPaperSpread(0);
	AdjustBoxInsets();
	return 0;
}


int SinglesInterface::ShowThisPaperSpread(int index)
{
	if (index < 0) index = 0;
	if (index >= singles->NumPapers()) index = singles->NumPapers()-1;
	
	first_paper = (index / singles->PapersPerPageSpread()) * singles->PapersPerPageSpread();

    //remapHandles(0);
	needtodraw = 1;
	return 0;
}

void SinglesInterface::send()
{
	// not needed?
}

void SinglesInterface::Clear(SomeData *d)
{
	PaperInterface::Clear(d);
	// if (maybebox) { maybebox->dec_count(); maybebox=nullptr; }
	// if (curbox) { curbox->dec_count(); curbox=nullptr; }
	// curboxes.flush();
	// if (papergroup) { papergroup->dec_count(); papergroup=nullptr; }
}


} // namespace Laidout
