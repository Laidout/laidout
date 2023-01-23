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


#include <lax/button.h>
#include <lax/laxutils.h>
#include <lax/utf8string.h>
#include <lax/sliderpopup.h>
#include <lax/interfaces/viewportwindow.h>

#include "findwindow.h"
#include "../language.h"
#include "../ui/viewwindow.h"
#include "../laidout.h"


//template implementation:
#include <lax/lists.cc>

	
#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//--------------------------------- FindWindow ------------------------------------
/*! \class FindWindow
 * \brief Window that lets you search for objects.
 */  

FindWindow::FindWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int x, int y, int w, int h, unsigned long owner, const char *msg)
	: DraggableFrame(parnt, nname, ntitle, nstyle | ANXWIN_REMEMBER, x,y,w,h,1)
		
{
	win_owner = owner;
	makestr(win_sendthis, msg);

	InstallColors(THEME_Panel);
	
	if (!win_parent) win_style |= ANXWIN_ESCAPABLE;

	how_many = nullptr;
	pattern = nullptr;
	founditems = nullptr;

	caseless = true;
	regex = false;
	where = FIND_InView;
	search_started = false;
	most_recent = -1;

	// continuous_update = false;
}

FindWindow::~FindWindow()
{
}

int FindWindow::Finalize()
{
	iterator.Clear();
	return anXWindow::Finalize();
}

int FindWindow::preinit()
{
	anXWindow::preinit();
	return 0;
}

bool MatchSelectable(Laxkit::anObject *obj, ObjectIterator::SearchPattern *pattern)
{
	ObjectContainer *oc = dynamic_cast<ObjectContainer*>(obj);
	if (!oc) return false;
	if (oc->object_flags() & OBJ_Unselectable) return false;
	return true;
}

bool MatchDrawableObject(Laxkit::anObject *obj, ObjectIterator::SearchPattern *pattern)
{
	DrawableObject *d = dynamic_cast<DrawableObject*>(obj);
	if (!d) return false;
	if (d->flags&SOMEDATA_UNSELECTABLE) return false;
	if (d->object_flags() & OBJ_Unselectable) return false;
	return true;
}

int FindWindow::init()
{
	//   ____pattern_____ .* Aa all (is regex, case sensitive)
	//   [next] [prev] [all]
	//   [ 5 found: (found object thumbnail list) ]
	//   search:
	//     name desc other-meta
	//     broken
	//     by type
	//     scriptable pre-flighty conditions
	//     current view only | in current selection | everywhere
	//   foreach: scriptable action
	// 

	RowFrame *rowframe = new RowFrame(this,"rf",nullptr,ROWFRAME_HORIZONTAL | ROWFRAME_LEFT,
					pad,pad,win_w-2*pad,win_h-2*pad, 1, nullptr,win_owner,win_sendthis,
					10);
	// if (win_w <= 0 || win_h <= 0) flags |= BOX_WRAP_TO_EXTENT; //WrapToExtent();

	int         textheight = win_themestyle->normal->textheight();
	int         linpheight = textheight + 12;
	Button *    tbut = nullptr;
	anXWindow * last = nullptr;

	rowframe->padinset = textheight / 2;


	 // -------------- pattern input --------------------

	last = pattern = new LineInput(this,"pattern",nullptr,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"pattern",
			            _("Find:"), nullptr,0,
			            100,0,1,1,3,3);
	pattern->tooltip(
			regex 
			  ? _("Pattern as a regular expressions.")
			  : _("Space separated list of patterns. Put a minus in front to ignore.")
			);
	pattern->GetLineEdit()->SetWinStyle(LINEEDIT_CLEAR_X, true);
	pattern->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_FOCUS_OFF, true);
	rowframe->AddWin(pattern,1, pattern->win_w+3*textheight,textheight,5000,50,0, linpheight,0,0,50,0, -1);


	 // -------------- regex
	last = tbut = new Button(this, "regex", nullptr, IBUT_FLAT | BUTTON_TOGGLE, 0,0,0,0,0, last,object_id,"regex",
						 0, //id
						 ".*", //label
						 nullptr,nullptr); //img filename, img
	tbut->tooltip(_("Toggle using regular expressions"));
	tbut->State(regex ? LAX_ON : LAX_OFF);
	rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	 // -------------- caseless
	last = tbut = new Button(this, "caseless", nullptr,IBUT_FLAT | BUTTON_TOGGLE, 0,0,0,0,0, last,object_id,"caseless",
						 0, //id
						 "Aa", //label
						 nullptr,nullptr); //img filename, img
	tbut->State(caseless ? LAX_OFF : LAX_ON);
	tbut->tooltip(_("Toggle caseless searching. Off is ignore case."));
	rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	 // ----- where: all, current view, selection
	SliderPopup *popup;
	last = popup = new SliderPopup(this,"where",nullptr,SLIDER_POP_ONLY, 0,0, 0,0, 1, last,object_id,"where");
	popup->AddItem(_("In view"),   FIND_InView);
	popup->AddItem(_("Anywhere"),  FIND_Anywhere);
	popup->AddItem(_("Selection"), FIND_InSelection);
	popup->Select(where);
	rowframe->AddWin(popup, 1, 200, 100, 50, 50, 0, linpheight, 0, 0, 50, 0, -1);

	rowframe->AddNull();
	
	
	 // -------------- Find prev
	last = tbut = new Button(this, "next", nullptr,0, 0,0,0,0,1, last,object_id,"next",
						 0, //id
						 _("Next"), //label
						 nullptr,nullptr); //img filename, img
	rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	last = tbut = new Button(this, "prev", nullptr,0, 0,0,0,0,1, last,object_id,"prev",
						 0, //id
						 _("Prev"), //label
						 nullptr,nullptr); //img filename, img
	rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last = tbut = new Button(this, "all", nullptr,0, 0,0,0,0,1, last,object_id,"findall",
						 0, //id
						 _("All"), //label
						 nullptr,nullptr); //img filename, img
	rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	rowframe->AddNull();


	//--------------- Found ---------------------------
	how_many = new MessageBar(this,"foundmsg",nullptr, 0, 0,0,0,0,0, _("Found (0)"));
	rowframe->AddWin(how_many,1, 40,0,5000,50,0, linpheight,0,0,50,0, -1);

	last = tbut = new Button(this,"togglelist",nullptr,0, 0,0,0,0,1, last,object_id,"togglelist",
						 0,
						 "=",
						 nullptr,nullptr);
	tbut->tooltip(_("Toggle list view"));
	rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	rowframe->AddNull();
	
	last = founditems = new IconSelector(this, "founditems", nullptr,
						BOXSEL_ONE_ONLY | BOXSEL_ROWS | BOXSEL_TOP | BOXSEL_HCENTER | BOXSEL_SPACE | BOXSEL_FLAT,
						0,0,0,0, 1,
						last,object_id,"founditems",
						0,textheight/4, textheight/2);
	founditems->display_style = BoxSelector::BOXES_Highlighted;
	// founditems->selection_style = BoxSelector::SEL_List_Select;
	// founditems->boxinset = textheight/2;
	founditems->labelstyle = LAX_ICON_ONLY;
	rowframe->AddWin(founditems,1, 100,0,5000,50,0, linpheight * 2,0,5000,50,0, -1);

	rowframe->AddNull();
	

	//------------------------------ final ok -------------------------------------------------------
	
	if (!win_parent) {
		 // [ Done ]   [ cancel ]
		last = tbut = new Button(this,"ok",nullptr,0, 0,0,0,0,1, last,object_id,"Ok",
							 BUTTON_OK,
							 _("Done"),
							 nullptr,nullptr);
		rowframe->AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		rowframe->AddWin(nullptr,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

//		last = tbut = new Button(this,"cancel",nullptr,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
//		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	}


	rowframe->WrapToExtent();
	last->CloseControlLoop();
	SetChild(rowframe);
	rowframe->Sync(1);
	rowframe->dec_count();
	return 0;
}

void FindWindow::SearchNext()
{
	last_search_type = LastSearchType::Next;
	anObject *obj = nullptr;
	FieldPlace place;
	if (!search_started) {
		InitiateSearch();
		obj = iterator.Start(&place);
	} else {
		iterator.StartFromCurrent(nullptr);
		obj = iterator.Next(&place);
	}

	selection.flush();
	if (obj) {
		selection.push(new FindResult(place, obj));
	}
	UpdateFound();
}

void FindWindow::SearchPrevious()
{
	last_search_type = LastSearchType::Previous;
	anObject *obj = nullptr;
	FieldPlace place;
	if (!search_started) {
		InitiateSearch();
		obj = iterator.End(&place);
	} else {
		iterator.StartFromCurrent(nullptr);
		obj = iterator.Previous(&place);
	}

	selection.flush();
	if (obj) {
		selection.push(new FindResult(place, obj));
	}
	UpdateFound();
}

void FindWindow::SearchAll()
{
	last_search_type = LastSearchType::All;
	anObject *obj = nullptr;
	FieldPlace place;
	InitiateSearch();
	obj = iterator.Start(&place);

	selection.flush();

	if (obj) {
		FieldPlace firstmatch = place;
		do {
			selection.push(new FindResult(place, obj));

			obj = iterator.Next(&place);
		} while (place != firstmatch);
	}

	UpdateFound();
}

void FindWindow::InitiateSearch()
{
	if (where == FIND_InView) {
		LaidoutViewport *vp = dynamic_cast<LaidoutViewport*>(win_parent);
		if (vp) iterator.SearchIn(vp);

	} else if (where == FIND_InSelection) {
		LaidoutViewport *vp = dynamic_cast<LaidoutViewport*>(win_parent);
		if (vp) iterator.SearchIn(vp->GetSelection());

	} else if (where == FIND_Anywhere) {
		iterator.SearchIn(laidout->project);
	}

	const char *pat = pattern->GetCText();
	iterator.Pattern(pat, regex, caseless, true, false, false);
	iterator.Pattern(MatchDrawableObject, true, false, true);

	search_started = true;
}

void FindWindow::UpdateFound()
{
	if (how_many) {
		Utf8String str;
		str.Sprintf(_("Found (%d):"), selection.n);
		how_many->SetText(str.c_str());
	}

	int dim = 50 * UIScale();
	founditems->Flush();
	for (int c=0; c<selection.n; c++) {
		Previewable *obj = dynamic_cast<Previewable*>(selection.e[c]->obj);
		LaxImage *preview = nullptr;
		if (obj) {
			LaxImage *img = obj->GetPreview();
			preview = GeneratePreview(img, dim,dim, true);
		}
		founditems->AddBox(selection.e[c]->obj->Id(), preview, c);
	}
	founditems->sync();
	sync();
}

/*! Return 1 for found, or 0 for not.
 */
int FindWindow::GetCurrent(FieldPlace &place_ret, Laxkit::anObject *&obj_ret)
{
	if (selection.n == 0) return 0;
	int i = most_recent;
	if (i < 0 || i >= selection.n) i = 0;

	place_ret = selection.e[i]->where;
	obj_ret = selection.e[i]->obj;
	return 1;
}

int FindWindow::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"FindWindow got Event: "<<(mes?mes:"(unknown)")<<endl;


	const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(data);

	if (!strcmp(mes,"pattern")) {
		if (s->info1 == 1) { //enter was pressed
			if (last_search_type == LastSearchType::Next) SearchNext();
			else if (last_search_type == LastSearchType::Previous) SearchPrevious();
			else if (last_search_type == LastSearchType::All) SearchAll();
		} else if (s->info1 == 0) { //contents changed
			const char *pat = pattern->GetCText();
			iterator.Pattern(pat, regex, caseless, true, false, false);
			iterator.Pattern(MatchDrawableObject, true, false, true);
		}
		return 0;

	} else if (!strcmp(mes,"where")) {
		where = (FindThings)s->info1;
		selection.flush();
		UpdateFound();
		return 0;

	} else if (!strcmp(mes,"prev")) {
		SearchPrevious();
		return 0;

	} else if (!strcmp(mes,"next")) {
		SearchNext();
		return 0;

	} else if (!strcmp(mes,"findall")) {
		SearchAll();
		return 0;

	} else if (!strcmp(mes,"regex")) {
		int l = s->info1;
		regex = (l == LAX_ON);
		return 0;

	} else if (!strcmp(mes,"caseless")) {
		int l = s->info1;
		caseless = (l != LAX_ON);
		return 0;

	} else if (!strcmp(mes,"founditems")) {
		send(s->info1);
		most_recent = s->info1;
		return 0;

	} else if (!strcmp(mes,"togglelist")) {
		founditems->DisplayAsList(founditems->DisplayType() == 0);
		return 0;

	} else if (!strcmp(mes,"Ok")) {
		send(-1);
		if (win_parent) app->destroywindow(win_parent); // *** assuming parent is a headwindow? wut's going on here
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;
	}

	return DraggableFrame::Event(data,mes);
}


void FindWindow::send(int info)
{
	DBG cerr << "Send in FindWindow"<<endl;

	if (!win_owner || isblank(win_sendthis)) return;

	SimpleMessage *msg = new SimpleMessage();
	msg->info1 = info;
	app->SendMessage(msg, win_owner, win_sendthis, object_id);
}

int FindWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch == LAX_Esc) { // *** why aren't keys propagating???
		app->unmapwindow(this);
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

/*! Force keyboard focus to be the pattern lineedit.
 */
void FindWindow::SetFocus()
{
	app->setfocus(pattern->GetLineEdit());
}

} // namespace Laidout

