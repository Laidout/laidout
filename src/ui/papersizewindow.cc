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
// Copyright (C) 2020 by Tom Lechner
//


#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/tabframe.h>
#include <lax/button.h>
#include <lax/colorbox.h>
#include <lax/units.h>

#include "papersizewindow.h"
#include "../language.h"
#include "../core/utils.h"
	
#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;


namespace Laidout {


//--------------------------------- PaperSizeWindow ------------------------------------
/*! \class PaperSizeWindow
 * \brief Class to let users chose a paper size or create a new one.
 */  

/*! If mod_in_place, incs count. Otherwise makes a duplicate, and count is not inc'd.
 */
PaperSizeWindow::PaperSizeWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							unsigned long owner, const char *msg,
							PaperStyle *paper, bool mod_in_place, bool edit_dpi, bool edit_color, bool send_every_change)
		: RowFrame(parnt,nname,ntitle,nstyle | ROWFRAME_HORIZONTAL | ROWFRAME_LEFT | ANXWIN_REMEMBER,
					0,0,600,300,0, nullptr,owner,msg,
					10)
{
	modify_in_place = mod_in_place;

	InstallColors(THEME_Panel);

	papernames = nullptr;
	paperx = papery = nullptr;
	papersizes = nullptr; //ends up being just a shadow of laidout->papersizes
	cur_units = laidout->prefs.default_units;
	custom_index = -1;
	with_dpi = edit_dpi;
	with_color = edit_color;
	send_on_change = send_every_change;

	if (!modify_in_place && paper) papertype = dynamic_cast<PaperStyle*>(paper->duplicate());
	else {
		papertype = paper;
		if (papertype) papertype->inc_count();
	}
	curorientation = 0;
	if (papertype) curorientation = papertype->landscape();
	
	if (!win_parent) win_style |= ANXWIN_ESCAPABLE;
}

PaperSizeWindow::~PaperSizeWindow()
{
	if (papertype) papertype->dec_count();
}

int PaperSizeWindow::preinit()
{
	anXWindow::preinit();
	// if (win_w <= 0 || win_h <= 0) flags |= BOX_WRAP_TO_EXTENT; //WrapToExtent();
	flags |= BOX_WRAP_TO_EXTENT;
	// if (win_w <= 0) win_w = 500;
	// if (win_h <= 0) win_h = 600;
	return 0;
}


int PaperSizeWindow::init()
{
      // [ Paper/Resource name  v ]   [ load from file ] [ save to file ] [ save globally ]
	  // 
	  // [ + new resource from current ]
	  // [portrait/landscape]
      // __width__  __height__
      // [Preferred units, or auto from paper]
      // [color]
      // [default dpi]
      // [other properties]

	int         textheight = win_themestyle->normal->textheight();
	int         linpheight = textheight + 12;
	Button *    tbut = nullptr;
	anXWindow * last = nullptr;
	LineInput * linp;


	 // -------------- Paper Size --------------------
	
	papersizes = &laidout->papersizes;

	if (!papertype) papertype = dynamic_cast<PaperStyle*>(laidout->GetDefaultPaper()->duplicate());

	char blah[100],blah2[100];
	bool o = papertype->landscape();
	curorientation = o;

	 // -----Paper Size X
	UnitManager *units=GetUnitManager();
	sprintf(blah,"%.10g", units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,nullptr));
	sprintf(blah2,"%.10g",units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,nullptr));
	last = paperx = new LineInput(this,"paper x",nullptr,LINP_ONLEFT/*|LINP_FLOAT*/, 0,0,0,0, 0, 
						last,object_id,"paper x",
			            _("Paper Size  w:"),(o&1?blah2:blah),0,
			            100,0,1,1,3,3);
	paperx->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_OFF, true);
	AddWin(paperx,1, paperx->win_w,0,0,50,0, linpheight,0,0,50,0, -1);

	
	 // -----Paper Size Y
	last = papery = new LineInput(this,"paper y",nullptr,LINP_ONLEFT/*|LINP_FLOAT*/, 0,0,0,0, 0, 
						last,object_id,"paper y",
			            _("h:"),(o ? blah : blah2),0,
			           100,0,1,1,3,3);
	papery->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_OFF, true);
	AddWin(papery,1, papery->win_w,0,0,50,0, linpheight,0,0,50,0, -1);


	 // -----Default Units
    SliderPopup *popup;
	last = popup = new SliderPopup(this,"units",nullptr,SLIDER_POP_ONLY, 0,0, 0,0, 1, last,object_id,"units");
	char *tmp;
	int c2 = 0;
	int uniti=-1,tid;
	units->UnitInfo(laidout->prefs.unitname,&uniti,nullptr,nullptr,nullptr,nullptr,nullptr);
	for (int c = 0; c < units->NumberOfUnits(); c++) {
		units->UnitInfoIndex(c, &tid, nullptr, nullptr, nullptr, &tmp, nullptr);
		if (uniti == tid) c2 = c;
		popup->AddItem(tmp, c);
	}
	if (c2 >= 0) popup->Select(c2);
	AddWin(popup, 1, 200, 100, 50, 50, 0, linpheight, 0, 0, 50, 0, -1);
	AddNull();

	
	 // -----Paper Name
	last = papernames = new SliderPopup(this,"paperName",nullptr,SLIDER_POP_ONLY, 0,0, 0,0, 1, last,object_id,"paper name");
	for (int c=0; c<papersizes->n; c++) {
		if (!strcmp(papersizes->e[c]->name, papertype->name)) c2 = c;
		if (!strcasecmp(papersizes->e[c]->name, "custom")) custom_index = c;
		papernames->AddItem(papersizes->e[c]->name,c);
	}
	papernames->Select(c2);
	AddWin(papernames,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	
	 // -----Paper Orientation
	last = orientation = popup = new SliderPopup(this,"paperOrientation",nullptr,0, 0,0, 0,0, 1, last,object_id,"orientation");
	popup->AddItem(_("Portrait"),0);
	popup->AddItem(_("Landscape"),1);
	popup->Select(o&1?1:0);
	AddWin(popup,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();


	if (with_dpi) {
		double d = papertype->dpi;
		last = linp = new LineInput(this,"dpi",nullptr,LINP_ONLEFT, 5,250,0,0, 0, 
							last,object_id,"dpi",
				            _("Default dpi:"),nullptr,0,
				            0,0,1,1,3,3);
		linp->SetText(d);
		AddWin(linp,1, linp->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddNull();
	}
	

// ******* uncomment when implemented!!
//
//	 // ------- color mode:		black and white, grayscale, rgb, cmyk, other
//	last=popup=new SliderPopup(this,"colormode",0, 0,0, 0,0, 1, popup,object_id,"colormode");
//	popup->AddItem(_("RGB"),0);
//	popup->AddItem(_("CMYK"),1);
//	popup->AddItem(_("Grayscale"),1);
//	popup->Select(0);
//	AddWin(popup, 200,100,50,50,0, linpheight,0,0,50,0);
//	AddWin(nullptr, 2000,2000,0,50,0, 0,0,0,0,0);// forced linebreak

	if (with_color) {
		AddWin(new MessageBar(this,"colormes",nullptr, MB_MOVE, 0,0,0,0,0, _("Paper Color:")), 1, -1);
		ColorBox *cbox;
		last = cbox = new ColorBox(this,"paper color",nullptr,0, 0,0,0,0, 1,
								   last,object_id,"paper color", LAX_COLOR_RGB,1./255, 1.,1.,1.);
		AddWin(cbox,1, 40,0,50,50,0, linpheight,0,0,50,0, -1);
		AddNull();
	}

	

	//------------------------------ final ok -------------------------------------------------------
	
	if (!win_parent) {
		 // [ ok ]   [ cancel ]
		last = tbut = new Button(this,"ok",nullptr,0, 0,0,0,0,1, last,object_id,"Ok",
							 BUTTON_OK,
							 nullptr,
							 nullptr,nullptr);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddWin(nullptr,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

		last = tbut = new Button(this,"cancel",nullptr,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	}


	
	WrapToExtent();
	last->CloseControlLoop();
	Sync(1);
	return 0;
}

/*! Ignores trailing chars.
 */
bool ParseAspectRatio(const char *str, double *d1_ret, double *d2_ret)
{
	if (!strstr(str, ":")) return false;

	char *endptr = nullptr;
	double d1 = strtod(str, &endptr);
	if (endptr == str) return false;

	while (isspace(*endptr)) endptr++;
	if (*endptr != ':') return false;

	endptr++;
	str = endptr;
	double d2 = strtod(str, &endptr);
	if (str == endptr) return false;

	if (d1 <= 0 || d2 <= 0)	return false;
	*d1_ret = d1;
	*d2_ret = d2;
	return true;
}

void PaperSizeWindow::UpdatePaperName()
{
	int o_ret = 0, i_ret = -1;
	PaperStyle *pp = GetNamedPaper(papertype->w(), papertype->h(), &o_ret, 0, &i_ret, 1e-8);
	if (pp) {
		papernames->Select(i_ret);
		makestr(papertype->name, pp->name);
		if (o_ret < 0) {
			if (papertype->landscape() == pp->landscape()) {
				papertype->landscape(!papertype->landscape());
				double d = papertype->width;
				papertype->width = papertype->height;
				papertype->height = d;
				orientation->Select(papertype->landscape() ? 1 : 0); //need to flip orientation
				curorientation = papertype->landscape();
			}
		}
	} else {
		papernames->Select(custom_index);
		makestr(papertype->name,_("Custom"));
	}
}

/*! Incs count of paper.
 * Return 0 for success, or 1 for error, like paper == null.
 */
int PaperSizeWindow::UsePaper(PaperStyle *paper, bool mod_in_place)
{
	if (!paper) return 1;
	modify_in_place = mod_in_place;
	if (papertype) papertype->dec_count();
	if (!modify_in_place) papertype = dynamic_cast<PaperStyle*>(paper->duplicate());
	else {
		papertype = paper;
		if (papertype) papertype->inc_count();
	}

	orientation->Select(papertype->landscape());

	char num[30];
	UnitManager *units=GetUnitManager();
	numtostr(num,30, units->Convert(papertype->w(),UNITS_Inches,cur_units,nullptr),0);
	paperx->SetText(num);
	numtostr(num,30, units->Convert(papertype->h(),UNITS_Inches,cur_units,nullptr),0);
	papery->SetText(num);

	UpdatePaperName();

	return 0;
}

int PaperSizeWindow::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;


	if (!strcmp(mes,"paper name")) { 
		 // new paper selected from the popup, so must find the x/y and set x/y appropriately
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		int i = s->info1;
		DBG cerr <<"new paper size:"<<i<<endl;
		if (i < 0 || i >= papersizes->n) return 0;

		papertype->dec_count();
		papertype = (PaperStyle *)papersizes->e[i]->duplicate();
		if (!strcmp(papertype->name,"custom")) return 0;
		papertype->landscape(curorientation);
		char num[30];
		UnitManager *units=GetUnitManager();
		numtostr(num,30, units->Convert(papertype->w(),UNITS_Inches,cur_units,nullptr),0);
		paperx->SetText(num);
		numtostr(num,30, units->Convert(papertype->h(),UNITS_Inches,cur_units,nullptr),0);
		papery->SetText(num);

		if (send_on_change) send(false);

		return 0;

	} else if (!strcmp(mes,"orientation")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int l = s->info1;
		DBG cerr <<"New orientation:"<<l<<endl;
		if (l != curorientation) {
			char *txt  = paperx->GetText();
			char *txt2 = papery->GetText();
			paperx->SetText(txt2);
			papery->SetText(txt);
			delete[] txt;
			delete[] txt2;
			curorientation = (l ? 1 : 0);
			papertype->landscape(curorientation);
			if (send_on_change) send(false);
		}
		return 0;

	} else if (!strcmp(mes,"paper x")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (!s || !s->str) return 0;

		double d1,d2;
		UnitManager *units=GetUnitManager();
		if (ParseAspectRatio(s->str, &d1, &d2)) {
			//we have an aspect ratio, so adjust accordingly
			double y = papery->GetDouble();
			double x = y * d1 / d2;
			paperx->SetText(x);
			x = units->Convert(x, cur_units, UNITS_Inches, nullptr);
			papertype->w(x);
		} else {
			d1 = strtod(s->str, nullptr);
			if (d1 > 0) papertype->w(units->Convert(d1, cur_units, UNITS_Inches, nullptr));
			else paperx->SetText(units->Convert(papertype->w(), UNITS_Inches, cur_units, nullptr));
		}

		// if not currently custom, try to match to a known paper portrait or landscape?
		UpdatePaperName();
		if (send_on_change) send(false);
		return 0;

	} else if (!strcmp(mes,"paper y")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (!s || !s->str) return 0;

		double d1,d2;
		UnitManager *units=GetUnitManager();
		if (ParseAspectRatio(s->str, &d1, &d2)) {
			//we have an aspect ratio, so adjust accordingly
			double x = paperx->GetDouble();
			double y = x * d2 / d1;
			papery->SetText(y);
			y = units->Convert(y, cur_units, UNITS_Inches, nullptr);
			papertype->h(y);
		} else {
			d1 = strtod(s->str, nullptr);
			if (d1 > 0) papertype->h(units->Convert(d1, cur_units, UNITS_Inches, nullptr));
			else papery->SetText(units->Convert(papertype->h(), UNITS_Inches, cur_units, nullptr));
		}

		// if not currently custom, try to match to a known paper portrait or landscape?
		UpdatePaperName();
		if (send_on_change) send(false);
		return 0;

	} else if (!strcmp(mes,"dpi")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (!s || !s->str) return 0;
		double dpi = strtod(s->str, nullptr);
		if (dpi > 0) {
			papertype->dpi = dpi;
			DBG cerr << "new paper dpi: "<<dpi<<endl;
			if (send_on_change) send(false);
		}
		return 0;

	} else if (!strcmp(mes,"units")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		UnitManager *units = GetUnitManager();
		int   i = s->info1;
		int   id;
		char *name;
		units->UnitInfoIndex(i, &id, nullptr, nullptr, nullptr, &name, nullptr);
		paperx->SetText(units->Convert(paperx->GetDouble(), cur_units, id, nullptr));
		papery->SetText(units->Convert(papery->GetDouble(), cur_units, id, nullptr));
		cur_units = id;
		//laidout->prefs.default_units = id;
		//makestr(laidout->prefs.unitname, name);
		makestr(papertype->defaultunits, name);
		if (send_on_change) send(false);
		return 0;

	} else if (!strcmp(mes,"Ok")) {
		send(true);
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;
	}
	return 1;
}


void PaperSizeWindow::send(bool also_delete)
{
	DBG cerr << "Send paper:"<<endl;
	DBG papertype->dump_out(stderr, 2, 0, nullptr);

	if (!win_owner || isblank(win_sendthis)) return;

	SimpleMessage *msg = new SimpleMessage(papertype);
	app->SendMessage(msg, win_owner, win_sendthis, object_id);
}


} // namespace Laidout

