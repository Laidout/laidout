//
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
// Copyright (C) 2018 by Tom Lechner
//
//

#include <lax/button.h>
#include <lax/lineinput.h>
#include <lax/utf8string.h>
#include <lax/language.h>

#include "metawindow.h"



#include <iostream>
#define DBG
using namespace std;

using namespace Laxkit;


namespace Laidout {

//----------------------------- MetaWindow -------------------------------

/*! \class MetaWindow
 */

/*! nMeta is inc_counted.
 */
MetaWindow::MetaWindow(anXWindow *prnt, const char *nname,const char *ntitle,unsigned long nstyle, unsigned long nowner, const char *msg,
					  Laxkit::AttributeObject *nMeta)
  : RowFrame(prnt, nname, ntitle, ROWFRAME_ROWS | ROWFRAME_STRETCH_IN_COL | ROWFRAME_LEFT | ANXWIN_REMEMBER | ANXWIN_ESCAPABLE, 0,0,600,500,0, NULL,nowner,msg)
{
	dialog_style = nstyle & ~ANXWIN_MASK;

	addpoint = 0;
	meta = nMeta;
	if (meta) meta->inc_count();
}

MetaWindow::~MetaWindow()
{
	if (meta) meta->dec_count();
}

int MetaWindow::init()
{
	double th = UIScale() * win_themestyle->normal->textheight();
	anXWindow *last = nullptr;
	Button *button = nullptr;
	padinset = th/2;

	LineInput *linp = nullptr;
	double max_label_width = 0;

	if (meta) {
		Attribute *att;
		Utf8String scratch;

		for (int c = 0; c < meta->attributes.n; c++) {
			att = meta->Att(c);
			double w = UIScale() * win_themestyle->normal->Extent(att->name,-1);
			if (w > max_label_width) max_label_width = w;
		}

		for (int c = 0; c < meta->attributes.n; c++) {
			att = meta->Att(c);

			scratch.Sprintf(".%s", att->name);
			last = linp = new LineInput(this, scratch.c_str(), att->name, LINP_SEND_ANY,
			        0,0,0,0,0,
			        last, object_id, scratch.c_str(),
			        att->name, att->value);
			linp->LabelWidth(max_label_width);
			//last->tooltip(sd->tooltip);
			AddWin(linp,1, linp->win_w,0,10000,50,0, linp->win_h,0,0,50,0, -1);

			if (!(dialog_style & META_As_Is)) {
				scratch.Sprintf("x%s", att->name);
				button = new Button(this, "X", "X", 0,
	                        0,0,0,0,1,
	                        last, object_id, scratch.c_str(),
	                        -1,
	                        "X"
						);
				AddWin(button,1, button->win_w,0,0,50,0, button->win_h,0,0,50,0, -1);
			}

			AddNull();
		}
	}

	addpoint = wholelist.n;

	if (!(dialog_style & META_As_Is)) {
		last = linp = new LineInput(this, "add", "add", 0,
				0,0,0,0,0,
				last, object_id, "add",
				_("New variable:"), nullptr);
		AddWin(linp,1, -1);
		AddNull();
	}

	//add Done button
	AddVSpacer(th,0,0, 0);
	AddNull();
	AddHSpacer(0,0,10000, 0);
	button = new Button(this, "Done", _("Done"), 0,
                        0,0,0,0,1,
                        last, object_id, "Done",
                        -1,
                        _("Done")
					);
	AddWin(button,1, button->win_w,0,2*button->win_w,50,0, 2*th,0,0,50,0, -1);
	AddHSpacer(0,0,10000, 0);

	Sync(1);
	return 0;
}

void MetaWindow::AddVariable(const char *name, const char *value, bool syncToo)
{
	Utf8String scratch;
	scratch.Sprintf(".%s", name);
	//anXWindow *last = ???;
	LineInput *linp = new LineInput(this, scratch.c_str(), name, LINP_SEND_ANY,
	        0,0,0,0,0,
	        nullptr, object_id, scratch.c_str(),
	        name, value);
	//last->tooltip(sd->tooltip);
	AddWin(linp,1, linp->win_w,0,10000,50,0, linp->win_h,0,0,50,0, addpoint++);

	scratch.Sprintf("x%s", name);
	Button *button = new Button(this, "X", "X", 0,
                0,0,0,0,1,
                nullptr, object_id, scratch.c_str(),
                -1,
                "X"
			);
	AddWin(button,1, button->win_w,0,0,50,0, button->win_h,0,0,50,0, addpoint++);

	AddNull(addpoint++);

	if (syncToo) Sync(1);
}

void MetaWindow::Send()
{
	DBG cerr << "MetaWindow::Send() "<<(win_sendthis ? win_sendthis : "null message")<<endl;

	SimpleMessage *ievent = new SimpleMessage(meta);
    app->SendMessage(ievent,win_owner,win_sendthis,object_id);
}

int MetaWindow::Event(const EventData *e,const char *mes)
{
    if (!strcmp(mes,"Done")) {
		Send();
		app->destroywindow(this);
		return 0;

    } else if (mes[0]=='.') { // is hopefully an extension
		const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(e);
		if (!s) return 1;

		const char *name = mes+1;
		Attribute *att = meta->find(name);
		if (att) makestr(att->value, s->str);

		return 0;

    } else if (mes[0]=='x') { // is hopefully an extension
		const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(e);
		if (!s) return 1;

		const char *name = mes+1;
		int i = -1;
		Attribute *att = meta->find(name, &i);
		if (att) {
			//delete att and remove the row
			meta->remove(i);
			char scratch[strlen(name)+2];
			sprintf(scratch, ".%s", name);
			int i = findWindowIndex(scratch);
			if (i >= 0) {
				Remove(i); //the lineinput
				Remove(i); //the X
				Remove(i); //the null
				addpoint -= 3;
			}
		}

		return 0;

	} else if (!strcmp(mes,"add")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(e);
		if (!s) return 1;

		if (!meta) meta = new AttributeObject();

		Attribute *att = meta->find(s->str);
		if (att) return 0; //already exists!!
		stripws(s->str);
		meta->push(s->str,nullptr);

		AddVariable(s->str, nullptr, true);

		return 0;
    }

    return 1;
}


//Laxkit::Attribute *MetaWindow::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
//{ ***
//	if (what==-1) {
//		if (!att) att = new Attribute;
//
//		return att;
//	}
//
//	if (!att) att = new Attribute;
//
//	return att;
//}
//
//void MetaWindow::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
//{ ***
//	char *name;
//    char *value;
//
//    for (int c=0; c<att->attributes.n; c++) {
//        name= att->attributes.e[c]->name;
//        value=att->attributes.e[c]->value;
//
//        if (!strcmp(name,"AField")) {
//			*** //do stuff
//		}
//	} 
//}


} // namespace Laidout

