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
// Copyright (C) 2025 by Tom Lechner
//

#include <lax/utf8string.h>
#include <lax/iconmanager.h>
#include <lax/language.h>

#include "impositionselectwindow.h"
#include "../api/buildicons.h"
#include "../configured.h"
#include "../laidout.h"

#include <lax/debug.h>


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


ImpositionSelectWindow::ImpositionSelectWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle, unsigned long owner, const char *msg)
  : InterfaceWindow(parnt, nname, ntitle, nstyle,
						0,0,0,0, 2,
						nullptr, owner, msg, //Laxkit::anXWindow *prev, unsigned long nowner, const char *nsend,
						nullptr, false)
{
	ginterface = new GridSelectInterface(nullptr, -1, nullptr, owner, msg);
	ginterface->select_type = LAX_ONE_ONLY;
	SetInterface(ginterface);

	InstallDefaultList();
	ColorsFromTheme();
}

void ImpositionSelectWindow::ColorsFromTheme()
{

	ginterface->color_normal = win_themestyle->fg.Lerp(win_themestyle->bg, .9);
	ginterface->color_selected = win_themestyle->fg.Lerp(win_themestyle->bg, .5);
}

//todo: coordinate with newdoc
#define IMP_NEW_SINGLES    10000
#define IMP_NEW_SIGNATURE  10001
#define IMP_NEW_NET        10002
#define IMP_FROM_FILE      10003
#define IMP_CURRENT        10004

/*! Parse the icons from impositions.svg.
 * Return true for things parsed, else false.
 */
bool ImpositionSelectWindow::InstallDefaultList()
{
	IconManager *icons = IconManager::GetDefault();
	const char *path = icons->FindPathForFile("impositions.svg");
	if (!path) {
		DBGE("Can't find impositions.svg!!")
		return false;
	}

	Utf8String icon_path = path;
	icon_path.Append("/impositions.svg");
	StringValue files(icon_path.c_str());
	IntValue grid_cell_size(1);
    IntValue output_px_size(icon_size);
	SetValue list;
	
	Value *ret = nullptr;
	ValueHash *context = nullptr;
	ValueHash params;
	params.push("files", &files);
	params.push("grid_cell_size", &grid_cell_size); // page units
	params.push("output_px_size", &output_px_size); // pixels wide of icon blocks
	params.push("output_list", &list);

	ErrorLog log;
	BuildIconsFunction(context, &params, &ret, log);

	MenuInfo *menu = new MenuInfo;
	ImageData *data = dynamic_cast<ImageData*>(list.FindID("NewSingles"));
	menu->AddItem(_("New Singles"), IMP_NEW_SINGLES, 0, data ? data->image : nullptr);
	if (data) data->image->inc_count();

	data = dynamic_cast<ImageData*>(list.FindID("NewSignature"));
	menu->AddItem(_("New Signature"), IMP_NEW_SIGNATURE, 0, data ? data->image : nullptr);
	if (data) data->image->inc_count();

	data = dynamic_cast<ImageData*>(list.FindID("NewNet"));
	menu->AddItem(_("New Net"), IMP_NEW_NET, 0, data ? data->image : nullptr);
	if (data) data->image->inc_count();

	data = dynamic_cast<ImageData*>(list.FindID("FromFile"));
	menu->AddItem(_("From file..."), IMP_FROM_FILE, 0, data ? data->image : nullptr);
	if (data) data->image->inc_count();

	for (int c = 0; c < laidout->impositionpool.n; c++) {
		data = dynamic_cast<ImageData*>(list.FindID(laidout->impositionpool.e[c]->icon_key));
		menu->AddItem(laidout->impositionpool.e[c]->name, c, 0, data ? data->image : nullptr);
		if (data) data->image->inc_count();
	}

	ginterface->UseThisMenu(menu);
	menu->dec_count();

	return true;
}

ImpositionSelectWindow::~ImpositionSelectWindow()
{

}

bool ImpositionSelectWindow::Select(int id)
{
	return ginterface->Select(id);
}

int ImpositionSelectWindow::init()
{
	

	return InterfaceWindow::init();
}


int ImpositionSelectWindow::Event(const Laxkit::EventData *data,const char *mes)
{
	return InterfaceWindow::Event(data, mes);
}


} // namespace Laidout