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
// Copyright (C) 2025 by Tom Lechner
//

#include "pdfpageproxy.h"


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//------------------------- PdfPageProxy -----------------------------


PdfPageProxy::PdfPageProxy()
{}

PdfPageProxy::PdfPageProxy(const char *file, int i, PaperStyle *style, int preview_size)
 : LImageData()
{
	// index = i;
	paperstyle = style;
	if (paperstyle) paperstyle->inc_count();
	if (preview_size > 0) preview_max_px = preview_size;

	if (file) {
		pdf_file = file;
		LoadPreviewed(file, i, preview_max_px, nullptr, false); //sets maxx,miny to pixel size of full image
	}

	if (paperstyle && maxx > minx && maxy > miny) {
		double w = paperstyle->w();
		double h = paperstyle->h();
		double scale  = w/maxx;
		double scaley = h/maxy;
		if (scaley < scale) scale = scaley;
		xaxis(scale*xaxis().normalized());
		yaxis(scale*yaxis().normalized());
	}
}

PdfPageProxy::~PdfPageProxy()
{
	if (paperstyle) paperstyle->dec_count();
}

Laxkit::Attribute *PdfPageProxy::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (what == -1) {
		if (!att) att = new Attribute();
		att->push("pdf_file", "path_to_pdf_file.pdf");
		att->push("preview_max_px", "256", "Render previews to fit in a box this wide and tall");
		att->push("paperstyle", nullptr, "Further information from the original pdf page");
		return LImageData::dump_out_atts(att, what, context);
	}

	if (!att) att = new Attribute();
	att->push("pdf_file", pdf_file.c_str());
	att->push("preview_max_px", preview_max_px);
	if (paperstyle) {
		Attribute *att2 = att->pushSubAtt("paperstyle");
		paperstyle->dump_out_atts(att2, what, context);
	}
	
	return LImageData::dump_out_atts(att, what, context);
}

void PdfPageProxy::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	for (int c=0; c<att->attributes.n; c++) {
		const char *name  = att->attributes.e[c]->name;
		const char *value = att->attributes.e[c]->value;

		if (!strcmp(name, "pdf_file")) {
			pdf_file = value;

		} else if (!strcmp(name, "preview_max_px")) {
			int i = -1;
			IntAttribute(value, &i, nullptr);
			if (i > 0) preview_max_px = i;

		} else if (!strcmp(name, "paperstyle")) {
			PaperStyle *style = new PaperStyle();
			style->dump_in_atts(att->attributes.e[c], flag, context);
			if (paperstyle) paperstyle->dec_count();
			paperstyle = style;
		}
	}

	LImageData::dump_in_atts(att, flag, context);
}

} // namespace Laidout
