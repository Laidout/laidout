//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//

#include <lax/interfaces/imageinterface.h>
#include <lax/transformmath.h>
#include "imageutils.h"
#include <iostream>
using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;

/*! \ingroup misc
 */
LaxImage *_lload_image(const char *filename)
{*********maybe this should call a laidout->something()...

	if (laidout->preview_images==Preview_None) return _load_imlib_image(filename);
	char *tempfile;
	if (laidout->preview_images==Preview_Temporary) {
		 // from /a/b/blah.jpg, make ~/.laidout/.tmp/3adc4ep8yg.jpg
		 // and ***must remember to delete on exit!!
		tempfile=***********;
	} else if (laidout->preview_images==Preview_ProjectDir) {
		 // from /a/b/blah.jpg, make /path/to/project/.tmp/blah3adc4ep8yg.jpg
		tempfile=***********;
	else { //Preview_SameDir
		 // from /a/b/blah.jpg, make /a/b/.blah-preview.jpg
		tempfile=***********;
	}
	LaxImage *img=_load_imlib_image_with_preview(filename,******tempfile,
								max_preview_length,max_preview_length);
	if (tempfile) delete[] tempfile;
	return img;
	------------------
	return laidout->load_image(filename);
}


