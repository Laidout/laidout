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
// Copyright (C) 2013 by Tom Lechner
//

#include <lax/imagedialog.h>
#include "limageinterface.h"
#include "../language.h"


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//------------------------------- LImageDialog --------------------------------
/*! \class LImageDialog
 */

class LImageDialog : public Laxkit::ImageDialog
{
  public:
	LImageDialog(anXWindow *parnt, unsigned long nowner, ImageInfo *inf);
};


LImageDialog::LImageDialog(anXWindow *parnt, unsigned long nowner, ImageInfo *inf)
  : ImageDialog(parnt,_("Image Properties"),_("Image Properties"), ANXWIN_REMEMBER,
				0,0,400,400,0,NULL, nowner,"image properties",
				IMGD_NO_TITLE,
				inf)
{
}



//------------------------------- LImageInterface --------------------------------
/*! \class LImageInterface
 * \brief add on a little custom behavior.
 */


LImageInterface::LImageInterface(int nid,Laxkit::Displayer *ndp)
  : ImageInterface(nid,ndp)
{
	style=1;
}

/*! Redefine to blot out title from the dialog.
 */
void LImageInterface::runImageDialog()
{
	 //after Laxkit event system is rewritten, this will be very different:
	ImageInfo *inf=new ImageInfo(data->filename,data->previewfile,NULL,data->description,0);
	curwindow->app->rundialog(new ImageDialog(NULL,"Image Properties","Image Properties",
					ANXWIN_REMEMBER,
					0,0,400,400,0,
					NULL,object_id,"image properties",
					IMGD_NO_TITLE,
					inf));
	inf->dec_count();
}


} //namespace Laidout

