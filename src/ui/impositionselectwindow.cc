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

#include "impositionselectwindow.h"
#include "../api/buildicons.h"
#include "../configured.h"

#include <lax/debug.h>


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


ImpositionSelectWindow::ImpositionSelectWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle)
  : InterfaceWindow(parnt, nname, ntitle, nstyle,
						0,0,0,0, 2,
						nullptr, 0, nullptr, //Laxkit::anXWindow *prev, unsigned long nowner, const char *nsend,
						nullptr, false)
{
	ginterface = new GridSelectInterface(nullptr, -1, nullptr);
	SetInterface(ginterface);

	InstallDefaultList();
	ColorsFromTheme();
}

void ImpositionSelectWindow::ColorsFromTheme()
{

	ginterface->color_normal = win_themestyle->fg.Lerp(win_themestyle->bg, .9);
	ginterface->color_selected = win_themestyle->fg.Lerp(win_themestyle->bg, .5);
}

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
	int status = BuildIconsFunction(context, &params, &ret, log);

	MenuInfo *menu = new MenuInfo;
	if (status == 0) {
		for (int c = 0; c < list.n(); c++) {
			ImageData *data = dynamic_cast<ImageData*>(list.e(c));
			LaxImage *img = data->image;
			menu->AddItem(data->Id(), 0, 0, img);
			img->inc_count(); //todo: should probably make the AddItem inc or absorb but that's too much work at the moment
		}
	}
	ginterface->UseThisMenu(menu);
	menu->dec_count();

	return true;
}

ImpositionSelectWindow::~ImpositionSelectWindow()
{

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