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
// Copyright (C) 2013 by Tom Lechner
//

#include <lax/imagedialog.h>
#include "limageinterface.h"
#include "../language.h"
#include "../core/stylemanager.h"
#include "../calculator/shortcuttodef.h"


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
	ImageInfo *inf = new ImageInfo(data->filename, data->previewfile, nullptr, data->description,0, data->index);
	curwindow->app->rundialog(new ImageDialog(NULL,"Image Properties",_("Image Properties"),
					ANXWIN_REMEMBER,
					0,0,400,400,0,
					NULL,object_id,"image properties",
					IMGD_NO_TITLE,
					inf));
	inf->dec_count();
}


LaxInterfaces::anInterface *LImageInterface::duplicateInterface(LaxInterfaces::anInterface *dup)
{
	if (dup==NULL) dup=dynamic_cast<anInterface *>(new LImageInterface(id,NULL));
	else if (!dynamic_cast<LImageInterface *>(dup)) return NULL;

	return ImageInterface::duplicateInterface(dup);
}


//! Returns this, but count is incremented.
Value *LImageInterface::duplicateValue()
{
    this->inc_count();
    return this;
}


ObjectDef *LImageInterface::makeObjectDef()
{

	ObjectDef *sd=stylemanager.FindDef("ImageInterface");
    if (sd) {
        sd->inc_count();
        return sd;
    }

	sd=new ObjectDef(NULL,"ImageInterface",
            _("Image Interface"),
            _("Image Interface"),
            "class",
            NULL,NULL);

	if (!sc) sc=GetShortcuts();
	ShortcutsToObjectDef(sc, sd);

	stylemanager.AddObjectDef(sd,0);
	return sd;
}


///*!
// * Return
// *  0 for success, value optionally returned.
// * -1 for no value returned due to incompatible parameters, which aids in function overloading.
// *  1 for parameters ok, but there was somehow an error, so no value returned.
// */
//int LImageInterface::Evaluate(const char *func,int len, ValueHash *context, ValueHash *parameters, CalcSettings *settings,
//	                     Value **value_ret, ErrorLog *log)
//{
//	return 1;
//}

/*! *** for now, don't allow assignments
 *
 * If ext==NULL, then assign v to replace what exists in this.
 * Otherwise assign v to the value at the end of the extension.
 *  
 * Return 1 for success.
 *  2 for success, but other contents changed too.
 *  0 for total fail, as when v is wrong type.
 *  -1 for bad extension.
 */
int LImageInterface::assign(FieldExtPlace *ext,Value *v)
{
	 //assignments not allowed
	return 0;
}

Value *LImageInterface::dereference(const char *extstring, int len)
{
	return NULL;
}

void LImageInterface::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	ImageInterface::dump_out(f,indent,what,context);
}

void LImageInterface::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	ImageInterface::dump_in_atts(att,flag,context);
}

Laxkit::Attribute *LImageInterface::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	return att;
}


} //namespace Laidout

