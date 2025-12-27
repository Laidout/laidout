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

#include "limagedata.h"
#include "../core/papersizes.h"


namespace Laidout {


//------------------------- PdfPageProxy -----------------------------


class PdfPageProxy : public LImageData
{
  public:
  	Laxkit::Utf8String pdf_file;
  	int preview_max_px = 256;
  	PaperStyle *paperstyle = nullptr;

  	PdfPageProxy();
  	PdfPageProxy(const char *file, int i, PaperStyle *style, int preview_size);
  	virtual ~PdfPageProxy();
  	virtual const char *whattype() { return "PdfPageProxy"; }
  	virtual Laxkit::LaxImage *GetPreview() { return previewimage; }
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

};


} // namespace Laidout
