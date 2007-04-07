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

***
*** NOTE This file is not active yet, it is to be used to manage preview images
***
*** TODO: must catch a sigkill, and remove any temp files...
***


class PreviewCacheNode 
{
 public:
	char *file;
	char *preview;
	int count;
	int deleteon0;
};

//! ******
void deleteLaidoutImage(LaxImage *img)
{
	laidout->wipeImage(img);
}

/*! \ingroup misc
 * Redefines the default Laxkit image loader to automatically use previews
 * above a certain size. This function just returns
 * laidout->load_image(filename).
 */
LaxImage *_lload_image(const char *filename)
{
	return laidout->load_image(filename);
}

//! Default image loader to automatically use previews above a certain size.
/*! If filename is NULL or is not an existing regular file, return NULL.
 *
 * Note that this is using Laxkit::_load_imlib_image_with_preview(). In the future
 * this will likely change to be able to swap out the image loader more easily.
 */
LaxImage *LaidoutApp::load_image(const char *filename)
{
	if (!filename || file_exists(filename,1,NULL)!=S_IFREG) return NULL;
	
	 // laidout needs to know where to store previews, and also whether newly
	 // created previews should be deleted when the program exits, maybe past
	 // a certain size......
	
	if (preview_images==Preview_None) return _load_imlib_image(filename);
	char *tempfile=NULL;
	if (preview_images==Preview_Temporary) {
		 // from /a/b/blah.jpg, make ~/.laidout/.tmp/3adc4ep8yg.jpg
		 // and ***must remember to delete on exit!!
		tempfile=new char[strlen(config_dir)+10];
		sprintf(tempfile,"%s/tmp/000000",config_dir);
		mkstemp(tempfile); **********
	} else if (preview_images==Preview_ProjectDir) {
		// // from /a/b/blah.jpg, make /path/to/project/.tmp/blah3adc4ep8yg.jpg
		//tempfile=***********;
		cout <<"*** imp previews in project dir"<<endl;
	else { 
		 //Preview_SameDir
		 // from /a/b/blah.jpg, make /a/b/.blah.jpg-laidout.jpg
		 // basename
		const char *file=lax_basename(filename);
		if (!file) return NULL;
		tempfile=new char[strlen(filename)+20];
		strncpy(tempfile,filename,file-filename);
		sprintf(tempfile+file-filename,".%s-laidout.jpg",file);
	}

	 // note that the imlib laximage stuff will only create jpg previews
	LaxImage *img=_load_imlib_image_with_preview(filename,tempfile,
								max_preview_length,max_preview_length,previews_while_running);
	if (tempfile) delete[] tempfile;
	if (img && preview_images==Preview_Temporary) img->delpreview=1;
	return img;
}

//! Return a temporary file name based on file in dir.
/*! format should have one "%s", which is where file goes.
char *make_temp_file(const char *dir,const char *format,const char *file)
{
	tmpfile mkstemp
}


