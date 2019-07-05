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
// Copyright (C) 2019 by Tom Lechner
//
//

#include <lax/messagebar.h>
#include <lax/sliderpopup.h>
#include <lax/lineinput.h>
#include <lax/checkbox.h>
#include <lax/colorbox.h>
#include <lax/utf8string.h>
#include <lax/language.h>

#include "valuewindow.h"


#define DBG
#include <iostream>
using namespace std;


using namespace Laxkit;
//using namespace LaxInterfaces;


namespace Laidout {

typedef int ValueGUIInitFunc(Value *value);
typedef int ValueGUIEventFunc(Value *value, Laxkit::EventData *ev, Laxkit::ErrorLog &log);

/*! \class ValueGUI
 * Class to aid in construction of gui editors tied to specific data types.
 */
class ValueGUI
{
  public:
	ValueGUIInitFunc initFunc;
	ValueGUIEventFunc eventFunc;

	char *vtype;
	int vtypei;
	int variation;

	ValueGUI(const char *valtype, int valtypei);
	virtual ~ValueGUI();
};


//----------------------------- ValueWindow -------------------------------

/*! \class ValueWindow
 */

ValueWindow::ValueWindow(Laxkit::anXWindow *prnt, const char *nname, const char *ntitle, unsigned long nowner, const char *mes, Value *nvalue)
  : ScrolledWindow(prnt, nname ? nname : (nvalue ? nvalue->Id() : nullptr), ntitle, SW_MOVE_WINDOW | SW_RIGHT /*| SW_BOTTOM*/,
		  0,0,600,600,0, nullptr,nowner,mes)
{
	rowframe = nullptr;
	value = nvalue;
	if (value) value->inc_count();

	initialized = false;

	InstallColors(THEME_Panel);
}

ValueWindow::~ValueWindow()
{
	if (value) value->dec_count();
}

int ValueWindow::init()
{
	if (!initialized) Initialize();
	return ScrolledWindow::init();
}


void ValueWindow::Initialize()
{
	Initialize(nullptr, value, value ? value->GetObjectDef() : nullptr, nullptr);
	rowframe->WrapToExtent();
	initialized = true;
}

void ValueWindow::Initialize(const char *prevpath, Value *val, ObjectDef *mainDef, const char *pathOverride)
{
	if (!val) val = value;
	if (!val) return;


	double th = win_themestyle->normal->textheight();
	double HMULT = 1.5;

	if (rowframe == nullptr) {
		const char *id = val->Id();
		if (!id) id = val->whattype();
		rowframe = new RowFrame(this, id, id, ROWFRAME_ROWS | ROWFRAME_STRETCH_IN_ROW | ROWFRAME_STRETCH_IN_COL,
				0,0,0,0,1, nullptr,0,nullptr, th*.25);
		//rowframe->flags |= BOX_WRAP_TO_EXTENT;
	}

	if (thewindow != rowframe) {
		UseThisWindow(rowframe);
	}

	anXWindow *last = nullptr;

	ObjectDef *def = val->GetObjectDef();
	if (!def) {
		DBG cerr << " *** WARNING! Missing object def for "<< val->Id()<<endl;
		//exit(1);
		return;
	}

	const char *fieldName = pathOverride;
	if (!fieldName && mainDef) fieldName = mainDef->Name;
	if (!fieldName) fieldName = "";

	const char *fieldTooltip = nullptr;
	if (mainDef) fieldTooltip = mainDef->description;
	if (!fieldTooltip) fieldTooltip = "";

	int type = val->type();
	Utf8String scratch;
	Utf8String mes;
	if (prevpath) mes.Sprintf("%s.%s", prevpath, def->name);
	else mes = def->name;
	Utf8String path = mes;

	bool do_kids = false;
	bool rearrangeable = false;
	bool deleteable = false;

	if (type == VALUE_Int) {
		//gui hint: slider or input, range

		IntValue *v = dynamic_cast<IntValue*>(val);
		scratch = v->i;
		LineInput *box;
		last = box = new LineInput(this, def->name,def->Name, LINP_INT,
							 0,0,0,0,0,
							 last,object_id, mes.c_str(),
							 fieldName, scratch.c_str());
		if (fieldTooltip) last->tooltip(fieldTooltip);
		rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);
		rowframe->AddNull();

	} else if (type == VALUE_Real || type == VALUE_Number) {
		//gui hint: range

		DoubleValue *v = dynamic_cast<DoubleValue*>(val);
		scratch = v->d;
		LineInput *box;
		last = box = new LineInput(this, def->name,def->Name, LINP_FLOAT,
							 0,0,0,0,0,
							 last,object_id, mes.c_str(),
							 fieldName, scratch.c_str());
		if (fieldTooltip) last->tooltip(fieldTooltip);
		rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);
		rowframe->AddNull();


	} else if (type == VALUE_Boolean) {
		BooleanValue *v = dynamic_cast<BooleanValue*>(val);
		CheckBox *check;
		last = check = new CheckBox(this, def->name, def->Name, CHECK_CIRCLE|CHECK_LEFT,
                             0,0,0,0,0,
                             last,object_id,mes.c_str(),
                             fieldName, th/2,th/2);
		if (fieldTooltip) last->tooltip(fieldTooltip);
        check->Checked(v->i);
		rowframe->AddWin(check,1, -1);
		rowframe->AddNull();

	} else if (type == VALUE_String) {
		StringValue *sv = dynamic_cast<StringValue*>(val);
		LineInput *box;
		last = box = new LineInput(this, def->name,def->Name, 0,
							 0,0,0,0,0,
							 last,object_id, mes.c_str(),
							 fieldName, sv->str);
		if (fieldTooltip) last->tooltip(fieldTooltip);
		rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);
		rowframe->AddNull();

	} else if (type == VALUE_Flatvector || type == VALUE_Spacevector || type == VALUE_Quaternion) {
		Quaternion sv;

		if (type == VALUE_Flatvector) {
			FlatvectorValue *v = dynamic_cast<FlatvectorValue*>(val);
			sv.set(v->v.x, v->v.y, 0,0);
		} else if (type == VALUE_Spacevector) {
			SpacevectorValue *v = dynamic_cast<SpacevectorValue*>(val);
			sv.set(v->v.x, v->v.y, v->v.z, 0);
		} else {
			QuaternionValue *v = dynamic_cast<QuaternionValue*>(val);
			sv = v->v;
		}

		MessageBar *bar = new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,1, fieldName);
		if (fieldTooltip) bar->tooltip(fieldTooltip);
		rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);

		LineInput *box;
		Utf8String path2 = path + ".x";
		scratch = sv.x;
		last = box = new LineInput(this, def->name,def->Name, 0,
							 0,0,0,0,0,
							 last,object_id, path2.c_str(),
							 _("x"), scratch.c_str());
		rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);

		path2 = path + ".y";
		scratch = sv.y;
		last = box = new LineInput(this, def->name,def->Name, 0,
							 0,0,0,0,0,
							 last,object_id, path2.c_str(),
							 _("y"), scratch.c_str());
		rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);

		if (type == VALUE_Spacevector || type == VALUE_Quaternion) {
			path2 = path + ".z";
			scratch = sv.z;
			last = box = new LineInput(this, def->name,def->Name, 0,
								 0,0,0,0,0,
								 last,object_id, path2.c_str(),
								 _("z"), scratch.c_str());
			rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);
		}

		if (type == VALUE_Quaternion) {
			path2 = path + ".w";
			scratch = sv.w;
			last = box = new LineInput(this, def->name,def->Name, 0,
								 0,0,0,0,0,
								 last,object_id, path2.c_str(),
								 _("w"), scratch.c_str());
			rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);
		}

		rowframe->AddNull();

	} else if (type == VALUE_File) {
		FileValue *v = dynamic_cast<FileValue*>(val);
		LineInput *box;
		last = box = new LineInput(this, def->name,def->Name, LINP_SEND_ANY | LINP_FILE,
							 0,0,0,0,0,
							 last,object_id, mes.c_str(),
							 fieldName, v->filename);
		if (fieldTooltip) last->tooltip(fieldTooltip);
		rowframe->AddWin(box,1, box->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);
		rowframe->AddNull();

	} else if (type == VALUE_Enum) { //} else if (type == VALUE_EnumVal) {
		EnumValue *ev = dynamic_cast<EnumValue*>(val);
		const char *nm=NULL, *Nm=NULL;

		MessageBar *bar = new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,1, fieldName);
		rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);

		SliderPopup *popup;
		last = popup = new SliderPopup(this,def->name,NULL,SLIDER_POP_ONLY, 0,0,0,0,0,
									   last,object_id, mes.c_str(),
									   nullptr,1
									   );
		if (fieldTooltip) last->tooltip(fieldTooltip);
		for (int c=0; c < ev->GetObjectDef()->getNumEnumFields(); c++) {
			ev->GetObjectDef()->getEnumInfo(c, &nm, &Nm);
			if (!Nm) Nm = nm;
			if (isblank(Nm)) continue;
			popup->AddItem(Nm,c);
		}
		rowframe->AddWin(popup,1, popup->win_w,0,10000,50,0, th*HMULT,0,0,50,0, -1);

		rowframe->AddNull();

	} else if (type == VALUE_Color) {
		ColorBox *colorbox;
		last = colorbox = new ColorBox(this,"colorbox",NULL,
								   //COLORBOX_ALLOW_NONE|COLORBOX_ALLOW_REGISTRATION|COLORBOX_ALLOW_KNOCKOUT,
								   0,
								   0,0,0,0,1, last,object_id,"curcolor",
								   LAX_COLOR_RGB,
								   .01,
								   1.,0.,0.,1.);
		if (fieldTooltip) last->tooltip(fieldTooltip);
		rowframe->AddWin(colorbox,1, 50,0,50,50,0, colorbox->win_h,0,50,50,0, -1);

	} else if (type == VALUE_Object) {
		ObjectValue *v = dynamic_cast<ObjectValue*>(val);

		MessageBar *bar = new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,1, fieldName); 
		if (fieldTooltip) bar->tooltip(fieldTooltip);
		rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);
		if (v) {
			bar = new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,1, v->Id());
			rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);
		}

		rowframe->AddNull();


	} else if (type == VALUE_Hash) {
		do_kids = true;
		// *** need extra for adding and removing elements
		
	} else if (type == VALUE_Fields) {
		do_kids = true;

	} else if (type == VALUE_Class) {
		do_kids = true;

	//} else if (type == VALUE_Array) {

	} else if (type == VALUE_Set) {
		SetValue *v = dynamic_cast<SetValue*>(val);

		MessageBar *bar = new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,0, fieldName); 
		if (fieldTooltip) bar->tooltip(fieldTooltip);
		rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, HMULT * bar->win_h,0,0,50,0, -1);
		rowframe->AddNull();

		Utf8String path2;
		ObjectDef *fdef;

		for (int c=0; c<v->values.n; c++) {
			Value *vv = v->values.e[c];
			fdef = vv->GetObjectDef();

			rowframe->AddHSpacer(th,0,0,0);
			//rowframe->AddWin(new MoveElementHandle(***));

			path2 = c;
			Initialize(prevpath, vv, fdef, path2.c_str());

			//bar = new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,1, vv->whattype());
			//rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);
			//rowframe->AddNull();
		}

		Button *tbut;
		path2 = "+";
		path2 += path;
		last = tbut = new Button(this,path2.c_str(),NULL,0, 0,0,0,0, 1,
            last, object_id, path2.c_str(),
            -1,
            " + ");



//	} else if (type == VALUE_Date) {
//		cerr << " *** MUST IMPLEMENT VALUE_Date for ValueWindow!!!"<<endl;
//	} else if (type == VALUE_Image) {
//		cerr << " *** MUST IMPLEMENT VALUE_Image for ValueWindow!!!"<<endl;
//	} else if (type == VALUE_Flags) {
//		cerr << " *** MUST IMPLEMENT VALUE_Flags for ValueWindow!!!"<<endl;
//	} else if (type == VALUE_Time) {
//		cerr << " *** MUST IMPLEMENT VALUE_Time for ValueWindow!!!"<<endl;
//	} else if (type == VALUE_Complex) {
//		cerr << " *** MUST IMPLEMENT VALUE_Complex for ValueWindow!!!"<<endl;
//	} else if (type == VALUE_Bytes) {
//		cerr << " *** MUST IMPLEMENT VALUE_Bytes for ValueWindow!!!"<<endl;
//	} else if (type == VALUE_Any) {

	} else {
		DBG cerr << " *** ValueWindow encountered unknown type!!! "<< type <<endl;

		scratch = "unhandled type ";
		scratch += def->name;
		MessageBar *bar = new MessageBar(this,"unhandled",NULL,MB_MOVE, 0,0,0,0,1, scratch.c_str());
		rowframe->AddWin(bar,1, bar->win_w,0,0,50,0, bar->win_h,0,0,50,0, -1);
	}

	if (do_kids) {
		const char *nm;
		//ValueTypes tp; *** should make sure it's data, not function that we are querying
		Utf8String path2;
		ObjectDef *fdef;

		for (int c=0; c<def->getNumFields(); c++) {
			fdef = def->getField(c);
			//def->getInfo(c, &nm);
			//if (!nm) continue;

			nm = fdef->name;
			Value *v = val->dereference(nm, -1);
			if (!v) continue;
			path2 = path + "." + nm;
			Initialize(path2.c_str(), v, fdef, nullptr);
		}
	}
}

//void ValueWindow::AddIntValue(RowFrame *rowframe, IntValue *val)
//{
//}

void ValueWindow::syncWindows()
{
	if (rowframe != nullptr && rowframe->win_w < win_w - scrollwidth) {
		rowframe->pw(win_w - scrollwidth);
	}
	ScrolledWindow::syncWindows();
}

int ValueWindow::Event(const EventData *data,const char *mes)
{
	if (!strcmp(mes, "pan change")) return ScrolledWindow::Event(data,mes);

	if (!isalnum(mes[0])) {
		return ScrolledWindow::Event(data,mes);
	}

	if (!value) return ScrolledWindow::Event(data,mes);
	Value *val = value;

	int type = value->type();

    const SimpleMessage *e = dynamic_cast<const SimpleMessage *>(data);
	if (e) {

		if (type == VALUE_Int) {
			IntValue *v = dynamic_cast<IntValue*>(val);
			char *endptr=NULL;
			int i = strtol(e->str, &endptr, 10);
			v->i = i;

		} else if (type == VALUE_Real || type == VALUE_Number) {
			DoubleValue *v = dynamic_cast<DoubleValue*>(val);
			char *endptr=NULL;
			double d = strtod(e->str, &endptr);
			v->d = d;

		} else if (type == VALUE_Boolean) {
			BooleanValue *v = dynamic_cast<BooleanValue*>(val);
			v->i = (e->info1 == LAX_ON);

		} else if (type == VALUE_String) {
			StringValue *v = dynamic_cast<StringValue*>(val);
			v->Set(e->str,-1);

		} else if (type == VALUE_Flatvector || type == VALUE_Spacevector || type == VALUE_Quaternion) {
			Quaternion v;

			char *endptr=NULL;
			double d = strtod(e->str, &endptr);
			char which = mes[strlen(mes)-1];

			if (type == VALUE_Flatvector) {
				FlatvectorValue *v = dynamic_cast<FlatvectorValue*>(val);
				if (which == 'x') v->v.x = d;
				else if (which == 'x') v->v.y = d;
			} else if (type == VALUE_Spacevector) {
				SpacevectorValue *v = dynamic_cast<SpacevectorValue*>(val);
				if (which == 'x') v->v.x = d;
				else if (which == 'y') v->v.y = d;
				else if (which == 'x') v->v.z = d;
			} else {
				QuaternionValue *v = dynamic_cast<QuaternionValue*>(val);
				if (which == 'x') v->v.x = d;
				else if (which == 'y') v->v.y = d;
				else if (which == 'x') v->v.z = d;
				else v->v.w = d;
			}


		} else if (type == VALUE_File) { //ignore? VALUE_FileSave and VALUE_FileLoad
			FileValue *v = dynamic_cast<FileValue*>(value);
			v->Set(e->str);

		} else if (type == VALUE_Object) {
			ObjectValue *v = dynamic_cast<ObjectValue*>(val);
			v->SetObject(const_cast<anObject*>(e->object), false);
	
		} else if (type == VALUE_Enum) {
		//} else if (type == VALUE_EnumVal) {
			EnumValue *ev = dynamic_cast<EnumValue*>(val);
			ev->value = e->info1;
		}

		return 0;

	}
	
	if (type == VALUE_Color) {
		ColorValue *v = dynamic_cast<ColorValue*>(val);
		const SimpleColorEventData *ce = dynamic_cast<const SimpleColorEventData *>(data);
        if (!ce) return ScrolledWindow::Event(data,mes);

		//if (ce->colorsystem = LAX_COLOR_RGB) v->color...
		v->color.Set(ce->colorsystem, ce->Valuef(0), ce->Valuef(1), ce->Valuef(2), ce->Valuef(3), ce->Valuef(4));

		return 0;
	} 
	
//	} else if (type == VALUE_Date) {
//	} else if (type == VALUE_Image) {
//	} else if (type == VALUE_Flags) {
//	} else if (type == VALUE_Time) {
//	} else if (type == VALUE_Complex) {
//	} else if (type == VALUE_Bytes) {
//
//	} else if (type == VALUE_Hash) {
//		do_kids = true;
//		// *** need extra for adding and removing elements
//		
//	} else if (type == VALUE_Fields) {
//		do_kids = true;
//	} else if (type == VALUE_Array) {
//		do_kids = true;
//	} else if (type == VALUE_Set) {
//		do_kids = true;
//
//	} else if (type == VALUE_Any) {
//	}

	return ScrolledWindow::Event(data,mes);
}

void ValueWindow::Send()
{
	SimpleMessage *message = new SimpleMessage(value);
	app->SendMessage(message, win_owner, win_sendthis, object_id);
}


void ValueWindow::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{ //***
	anXWindow::dump_out(f,indent,what,context);

//	char spc[indent+1]; memset(spc,' ',indent); spc[indent]=' ';
//
//	if (what==-1) {
//		fprintf(f,"%sAField value #comment\n",spc);
//		return;
//	}
//						
//	fprintf(f,"%sAField %d\n",spc, value);
//
//	-------- OR:  piggy back on dump_out_atts() ------------
//
//	LaxFiles::Attribute att;
//	dump_out_atts(&att,what,savecontext);
//	att.dump_out(f,indent);
}

LaxFiles::Attribute *ValueWindow::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{ //***
	return anXWindow::dump_out_atts(att,what,savecontext);

//	if (what==-1) {
//		if (!att) att = new Attribute;
//
//		return att;
//	}
//
//	if (!att) att = new Attribute;
//
//	return att;
}

void ValueWindow::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ //***
	return anXWindow::dump_in_atts(att,flag,context);

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
}


} // namespace Laidout

