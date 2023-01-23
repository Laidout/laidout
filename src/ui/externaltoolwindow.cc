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
// Copyright (C) 2016 by Tom Lechner
//


#include <lax/messagebar.h>

#include "externaltoolwindow.h"
#include "metawindow.h"
#include "../laidout.h"
#include "../language.h"

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;


namespace Laidout {


//------------------------------ ExternalToolManagerWindow2 ----------------------------

// class ListArranger : public Laxkit::RowFrame
// {
// 	class ListItem
// 	{
// 	  public:
// 	  	DoubleBBox box;
// 	  	anObject *info;
// 	};

//   public:
//  	ListArranger(Laxkit::anXWindow *parnt);
// 	virtual ~ListArranger();
// 	virtual const char *whattype() { return "ListArranger"; }
// 	// virtual int preinit();
// 	virtual int init();
// 	virtual int Event(const Laxkit::EventData *data,const char *mes);

// 	virtual int send();

// 	virtual int DrawBox();
// };


// ListArranger::ListArranger(Laxkit::anXWindow *parnt)
// 		: anXWindow(parnt,"lister",nullptr,0,
// 					0,0,600,800,0, NULL,0,NULL,
// 					)
// {
	
// }


//------------------------------ ExternalToolManagerWindow ----------------------------

/*! \class ExternalToolManagerWindow
 *
 * Settings window for autosaving.
 */
ExternalToolManagerWindow::ExternalToolManagerWindow(Laxkit::anXWindow *parnt, unsigned long nowner, const char *mes)
		: RowFrame(parnt,"exttools",_("External tools"),
					ANXWIN_DOUBLEBUFFER|ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ANXWIN_REMEMBER|ANXWIN_CENTER|(parnt?ANXWIN_ESCAPABLE:0),
					0,0,600,800,0, NULL,nowner,mes,
					0)
{
	main_box = nullptr;
	hover_cat = -1;
	hover_item = -1;
	hover_action = -1;
	lb_action = -1;
	pad = padinset = win_themestyle->normal->textheight()/2;
}

enum ExtToolOptions {
	EXT_None = 0,
	EXT_Del_Tool,
	EXT_New_Tool,
	EXT_On_Tool,
	EXT_Del_Category,
	EXT_New_Category,
	EXT_On_Category,
	EXT_MAX
};

ExternalToolManagerWindow::~ExternalToolManagerWindow()
{
	
}

void ExternalToolManagerWindow::GetMainExtent()
{
	double h = 2*pad;
	double w = 0, ww;
	int th = win_themestyle->normal->textheight();
	Utf8String scratch;

	for (int c=0; c<laidout->prefs.external_tool_manager.external_categories.n; c++) {
		ExternalToolCategory *cat = laidout->prefs.external_tool_manager.external_categories.e[c];
		h += th;
	
		//name, desc
		if (!isblank(cat->description)) scratch.Sprintf("%s, %s", cat->Name, cat->description);
		else scratch = cat->Name;
		ww = win_themestyle->normal->Extent(scratch.c_str(),-1);
		if (ww > w) w = ww;

		//tools
		for (int c2=0; c2<cat->tools.n; c2++) {
			ExternalTool *tool = cat->tools.e[c2];

			if (!isblank(tool->description)) scratch.Sprintf("%s, %s", tool->name, tool->description);
			else scratch = tool->name;

			ww = win_themestyle->normal->Extent(scratch.c_str(),-1);
			if (ww > w) w = ww;
			h += th;
		}
		h += th/2; // pad after tool list per category
	}

	w += 2*pad + th;

	main_w = w;
	main_h = h;
	// pw(w);
	// ph(h);
}

/*! Return hover category. */
int ExternalToolManagerWindow::scan(int x, int y, int *hitem, int *haction)
{
	double yy = pad;
	int th = win_themestyle->normal->textheight();
	// Utf8String scratch;
	
	*hitem = -1;
	*haction = -1;

	for (int c=0; c<laidout->prefs.external_tool_manager.external_categories.n; c++) {
		ExternalToolCategory *cat = laidout->prefs.external_tool_manager.external_categories.e[c];

		if (y >= yy && y < yy+th) {
			if (x >= win_w - th - pad) *haction = EXT_Del_Category;
			else *haction = EXT_On_Category;
			return c;
		}

		yy += th;		

		//tools
		for (int c2=0; c2<cat->tools.n; c2++) {
			// ExternalTool *tool = cat->tools.e[c2];

			if (y >= yy && y < yy+th) {
				*hitem = c2;
				if (x >= win_w - th - pad) *haction = EXT_Del_Tool;
				else *haction = EXT_On_Tool;
				return c;
			}

			yy += th;
		}

		if (y >= yy && y < yy+th) {
			*haction = EXT_New_Tool;
			return c;
		}
		yy += th;

		yy += th/2; // pad after tool list per category
	}

	if (y >= yy && y < yy+th) {
		*haction = EXT_New_Category;
		return -1;
	}

	return -1;
}

int ExternalToolManagerWindow::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *mouse)
{
	if (hover_cat == -1 && hover_item == -1 && hover_action == -1) return 1;
	buttondown.down(mouse->id,LEFTBUTTON,x,y, hover_cat, hover_item);
	lb_action = hover_action;
	return 0;
}

int ExternalToolManagerWindow::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	if (!buttondown.any()) return 1;

	int lb_item, lb_cat;
	// int dragged = 
	buttondown.up(mouse->id, LEFTBUTTON, &lb_cat, &lb_item);
	int hitem = -1, haction = -1;
	int hcat = scan(x,y,&hitem,&haction);


	if (hcat == lb_cat && hitem == lb_item && haction == lb_action) {
		DBG cerr << "up, cat: "<<lb_cat<<", item: "<<lb_item<<", action: "<<lb_action<<endl;

		// do something!!
		if (haction == EXT_New_Tool) {
			ExternalTool *etool = new ExternalTool(nullptr, nullptr, laidout->prefs.external_tool_manager.external_categories.e[hcat]->id);
			ExternalToolWindow *newtool = new ExternalToolWindow(nullptr, etool, object_id, "newtool");
			etool->dec_count();
			app->rundialog(newtool);
			return 0;

		} else if (haction == EXT_On_Tool) {
			ExternalTool *t = dynamic_cast<ExternalTool*>(laidout->prefs.external_tool_manager.external_categories.e[hcat]->tools.e[hitem]->duplicate(nullptr));
			t->object_id = laidout->prefs.external_tool_manager.external_categories.e[hcat]->tools.e[hitem]->object_id;
			ExternalToolWindow *win = new ExternalToolWindow(nullptr, t, object_id, "updatetool");
			t->dec_count();
			app->rundialog(win);
			return 0;

		} else if (haction == EXT_Del_Tool) {
			laidout->prefs.external_tool_manager.external_categories.e[hcat]->tools.remove(hitem);
			laidout->prefs.external_tool_manager.Save();
			needtodraw = 1;
			return 0;

		} else if (haction == EXT_Del_Category) {
			if (laidout->prefs.external_tool_manager.external_categories.e[hcat]->is_user_category) {
				laidout->prefs.external_tool_manager.external_categories.remove(hcat);
				laidout->prefs.external_tool_manager.Save();
			}
			needtodraw = 1;
			return 0;

		} else if (haction == EXT_On_Category) {
			ExternalToolCategory *cat = laidout->prefs.external_tool_manager.external_categories.e[hcat];
			if (cat->is_user_category) {
				AttributeObject *att = new AttributeObject();
				att->object_id = cat->id;
				att->push(_("Name"), cat->Name);
				att->push(_("Description"), cat->description);
				MetaWindow *win = new MetaWindow(nullptr, "newcat", _("Edit category"), META_As_Is, object_id, "editcategory", att);
				att->dec_count();
				laidout->rundialog(win);
			}
			return 0;
			
		} else if (haction == EXT_New_Category) {
			// just need: 
			//   name
			//   description
			AttributeObject *att = new AttributeObject();
			att->push(_("Name"));
			att->push(_("Description"));
			MetaWindow *win = new MetaWindow(nullptr, "newcat", _("New category..."), 0, object_id, "newcategory", att);
			att->dec_count();
			laidout->rundialog(win);
			return 0;
		}
	}
	return 0;
}

int ExternalToolManagerWindow::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	// if (!buttondown.any()) {}
	
	//do this regardless of mouse pressed:
	int hitem = -1, haction = -1;
	int hcat = scan(x,y,&hitem,&haction);
	DBG cerr << "ext tool: cat: "<<hcat<<", item: "<<hitem<<", action: "<<haction<<endl;

	if (hcat != hover_cat || hitem != hover_item || haction != hover_action) {
		hover_cat = hcat;
		hover_item = hitem;
		hover_action = haction;
		needtodraw = 1;
	}

	return 0;
}
	
// void ExternalToolManagerWindow::sync(int xx,int yy,int ww,int hh)
// {
// 	SquishyBox::sync(xx,yy,ww,hh);
// 	MoveResize(xx,yy,ww,hh);
// }

// int ExternalToolManagerWindow::preinit()
// {
// 	anXWindow::preinit();

// 	if (win_w<=0) win_w=400;
// 	if (win_h<=0) win_h=laidout->defaultlaxfont->textheight()*10;

// //	if (win_w<=0 || win_h<=0) {
// //		arrangeBoxes(1);
// //		win_w=w();
// //		win_h=h();
// //	} 
	
// 	return 0;
// }

int ExternalToolManagerWindow::init()
{
	int        th         = app->defaultlaxfont->textheight();
	int        linpheight = th * 1.3;
	
	Button *   tbut       = NULL;
	anXWindow *last       = NULL;
	
	//add empty box that we deal with manually:
	GetMainExtent();
	main_box = new SquishyBox(0, 0,main_w,main_w,0,10000,50,0, 0,main_h,main_h,0,100000,50,0);
	Push(main_box, 1);
	AddNull();


	//------------------------------ final ok -------------------------------------------------------

	if (!win_parent) {
		 //ok and cancel only when is toplevel...

		AddVSpacer(th/2,0,0,50,-1);
		AddNull();

		AddWin(NULL,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
		
		 // [ ok ]
		last=tbut=new Button(this,"ok",NULL,0,0,0,0,0,1, last,object_id,"Ok", BUTTON_OK,_("Done"));
		tbut->State(LAX_OFF);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

		//  // [ cancel ]
		// last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL,0,0,0,0,1, last,object_id,"Cancel");
		// AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

		AddWin(NULL,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
	}

	
	last->CloseControlLoop();
	Sync(1);
	return 0;
}

int ExternalToolManagerWindow::Event(const EventData *data,const char *mes)
{

	if (!strcmp(mes,"Ok")) {
		if (send()==0) return 0;
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;


	} else if (!strcmp(mes,"newtool")) { 
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		ExternalTool *tool = dynamic_cast<ExternalTool*>(e->object);
		ExternalToolCategory *cat = laidout->prefs.GetToolCategory(tool->category);
		cat->tools.push(tool);
		laidout->prefs.external_tool_manager.Save();
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes,"updatetool")) { 
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		ExternalTool *tool = dynamic_cast<ExternalTool*>(e->object);
		ExternalToolCategory *cat = laidout->prefs.GetToolCategory(tool->category);
		
		for (int c=0; c<cat->tools.n; c++) {
			if (cat->tools.e[c]->object_id == tool->object_id) {
				cat->tools.e[c]->SetFrom(tool);
				break;
			}
		}
		laidout->prefs.external_tool_manager.Save();
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes,"newcategory")) { 
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		AttributeObject *att = dynamic_cast<AttributeObject*>(e->object);

		ExternalToolCategory *cat = new ExternalToolCategory(ExternalToolCategory::GetNewUniqueId(),
															 att->findValue(_("Name")),
															 att->findValue(_("Name")),
															 att->findValue(_("Description")),
															 true);
		laidout->prefs.AddExternalCategory(cat);
		laidout->prefs.external_tool_manager.Save();
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes,"editcategory")) { 
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		AttributeObject *att = dynamic_cast<AttributeObject*>(e->object);

		ExternalToolCategory *cat = laidout->prefs.GetToolCategory(att->object_id);
		makestr(cat->Name, att->findValue(_("Name")));
		cat->Id(cat->Name);
		makestr(cat->description, att->findValue(_("Description")));
		laidout->prefs.external_tool_manager.Save();
		return 0;

	}

	return anXWindow::Event(data, mes);
}

/*! Validate inputs and send if valid.
 *
 * Return 1 for ok, 0 for invalid and don't destroy dialog yet.
 */
int ExternalToolManagerWindow::send()
{
	if (!win_owner) return 0;
	SimpleMessage *mes = new SimpleMessage();
	app->SendMessage(mes,win_owner,win_sendthis,object_id);

	return 1;
}


void ExternalToolManagerWindow::Refresh()
{
	if (needtodraw == 0) return;
	RowFrame::Refresh();
	needtodraw = 0;

	Displayer *dp = MakeCurrent();
	dp->ClearWindow();
	dp->font(win_themestyle->normal);

	double th = win_themestyle->normal->textheight();
	double y = pad;
	double pad = th/2;
	Utf8String scratch;

	for (int c=0; c<laidout->prefs.external_tool_manager.external_categories.n; c++) {
		ExternalToolCategory *cat = laidout->prefs.external_tool_manager.external_categories.e[c];
		// Name, description   [x]
		//   tool1
		//   new tool

		// double h = 2*th + th * cat->tools.n;

		//highlight category name
		if (hover_cat == c && hover_item == -1 && cat->is_user_category
			&& (hover_action == EXT_On_Category || hover_action == EXT_Del_Category))
			dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .8));
		else dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .9));
		dp->drawrectangle(pad,y, win_w-2*pad,th, 1);

		//cat name, desc
		if (!isblank(cat->description)) scratch.Sprintf("%s, %s", cat->Name, cat->description);
		else scratch = cat->Name;

		dp->NewFG(win_themestyle->fg);
		dp->textout(pad+pad,y, scratch.c_str(),-1, LAX_LEFT | LAX_TOP);
		
		//x button
		if (cat->is_user_category) {
			if (hover_cat == c && hover_action == EXT_Del_Category) {
				dp->NewFG(coloravg(win_themestyle->bg, rgbcolorf(1.,0.,0.)));
				dp->drawrectangle(win_w - pad - 2*th, y, 2*th, th, 1);
			}
			dp->NewFG(win_themestyle->fg);
			dp->textout(win_w - th - pad, y, "x",1, LAX_LEFT|LAX_TOP);
		}
		y += th;

		//tools
		dp->NewFG(win_themestyle->fg);
		for (int c2=0; c2<cat->tools.n; c2++) {
			ExternalTool *tool = cat->tools.e[c2];

			if (hover_cat == c && hover_item == c2 && (hover_action == EXT_On_Tool || hover_action == EXT_Del_Tool)) {
				dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .8));
				dp->drawrectangle(pad,y, win_w - pad - 2*th, th, 1);
				dp->NewFG(win_themestyle->fg);
			}

			//name, desc
			if (!isblank(tool->description)) scratch.Sprintf("%s, %s", tool->name, tool->description);
			else scratch = tool->name;

			dp->textout(pad + 2*th,y, scratch.c_str(),-1, LAX_LEFT | LAX_TOP);

			//x button
			if (hover_cat == c && hover_item == c2 && hover_action == EXT_Del_Tool) {
				dp->NewFG(coloravg(win_themestyle->bg, rgbcolorf(1.,0.,0.)));
				dp->drawrectangle(win_w - pad - 2*th, y, 2*th, th, 1);
			}
			dp->NewFG(win_themestyle->fg);
			dp->textout(win_w - th - pad, y, "x",1, LAX_LEFT|LAX_TOP);

			y += th;
		}

		 //new tool
		if (hover_cat == c && hover_item == -1 && hover_action == EXT_New_Tool) {
			dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .8));
			dp->drawrectangle(pad,y, win_w - pad - 2*th, th, 1);
			dp->NewFG(win_themestyle->fg);
		} else dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .5));
		dp->textout(pad + 2*th, y, _("New tool..."),-1, LAX_LEFT | LAX_TOP);
		y += th + th/2;
	}

	//new category...
	if (hover_cat == -1 && hover_item == -1 && hover_action == EXT_New_Category) {
		dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .8));
		dp->drawrectangle(pad,y, win_w - pad - 2*th, th, 1);
		dp->NewFG(win_themestyle->fg);
	} else dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .5));
	dp->textout(pad, y, _("New category..."),-1, LAX_LEFT | LAX_TOP);

	SwapBuffers();
}


//------------------------------ ExternalToolWindow ----------------------------

/*! \class ExternalToolWindow
 *
 * Edit settings for a single external tool.
 */

ExternalToolWindow::ExternalToolWindow(Laxkit::anXWindow *parnt, ExternalTool *etool, unsigned long nowner, const char *mes)
		: RowFrame(parnt,"newtool",_("New tool..."),
					ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ANXWIN_REMEMBER|ANXWIN_CENTER|(parnt?ANXWIN_ESCAPABLE:0),
					0,0,0,0,0, NULL,nowner,mes,
					laidout->defaultlaxfont->textheight()/3)
{
	name = commandid = path = parameters = description = website = nullptr;
	tool = etool;
	if (tool) {
		tool->inc_count();

		ExternalToolCategory *cat = nullptr;
		for (int c=0; c<laidout->prefs.external_tool_manager.external_categories.n; c++) {
			if (tool->category == laidout->prefs.external_tool_manager.external_categories.e[c]->id) {
				cat = laidout->prefs.external_tool_manager.external_categories.e[c];
				Utf8String str;
				str.Sprintf(_("New %s..."), cat->Name);
				WindowTitle(str.c_str());
				break;
			}
		}
	}
}

ExternalToolWindow::~ExternalToolWindow()
{
	if (tool) tool->dec_count();
}

int ExternalToolWindow::preinit()
{
	anXWindow::preinit();

	if (win_w<=0) win_w=400;
	if (win_h<=0) win_h=laidout->defaultlaxfont->textheight()*10;

//	if (win_w<=0 || win_h<=0) {
//		arrangeBoxes(1);
//		win_w=w();
//		win_h=h();
//	} 
	
	return 0;
}

int ExternalToolWindow::init()
{
	int        th         = app->defaultlaxfont->textheight();
	int        linpheight = th * 1.3;
	int        pad        = th / 3;
	int        lpad       = th / 4;
	Button *   tbut       = NULL;
	anXWindow *last       = NULL;
	// LineInput *linp=NULL;
	// CheckBox *check = NULL;
	// char      scratch[200];

	//   new tool:
	//     Name        _____
	//     CommandID   _______
	//     path        _______[...]
	//     Parameters  _________________
	//     description _______
	//     website     _______
	//     
	//  new category:
	//    Name
	//    Description
	//    id (automatically assigned)


	// last=check=new CheckBox(this,"autosave",NULL,CHECK_LEFT, 0,0,0,0,0, 
	// 								last,object_id,"autosave", _("Autosave"), pad,pad);
	// check->State(laidout->prefs.autosave ? LAX_ON : LAX_OFF);
	// check->tooltip(_("Whether to autosave or not."));
	// AddWin(check,1, check->win_w,0,30000,50,0, check->win_h,0,0,50,0, -1);
	// AddNull();

	if (!tool) tool = new ExternalTool(nullptr, nullptr, ExternalToolCategory::PrintCommand);


	 //-------------  Name
	last = name = new LineInput(this,"name",NULL,LINP_ONLEFT|LINP_SEND_ANY, 0,0,0,0, 0, 
						last,object_id,"name",
			            _("Name"), tool->name,0,
			            0,0, pad,pad, lpad,lpad);
	last->tooltip(_("Human readable name of tool"));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	AddNull();


	 //-------------  CommandID
	last = commandid = new LineInput(this,"command_id",NULL,LINP_ONLEFT|LINP_SEND_ANY, 0,0,0,0, 0, 
						last,object_id,"command_id",
			            _("Command id"), tool->command_name,0,
			            0,0, pad,pad, lpad,lpad);
	last->tooltip(_("Unique id of the tool, independent of languages"));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	last=tbut=new Button(this,"findpath",NULL,0,0,0,0,0,1, last,object_id,"findpath", 0,_("Find path"));
	tbut->tooltip(_("Search PATH for this command"));
	AddWin(tbut,1, tbut->win_w,0,0,50,0, linpheight,0,0,50,0, -1);	
	AddNull();


	//     path        _______[...]
	last = path = new LineInput(this,"path",NULL,LINP_ONLEFT | LINP_FILE|LINP_SEND_ANY, 0,0,0,0, 0, 
						last,object_id,"path",
			            _("Path"), tool->binary_path,0,
			            0,0, pad,pad, lpad,lpad);
	last->tooltip(_("Full file path of command"));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	AddNull();


	//     Parameters  _________________
	last = parameters = new LineInput(this,"parameters",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"parameters",
			            _("Parameters"), tool->parameters, 0,
			            0,0, pad,pad, lpad,lpad);
	//last->tooltip(_("%f = filename\n%b = basename without extension\n%e = extension\n# = autosave number"));
	last->tooltip(_("Optional parameters. Default is to list files.\nUse %f or {file} for file.\n{dirname} for directory of file\n{basename} for file basename\n{files} for list of all arguments"));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	AddNull();


	//     description _______
	last = description = new LineInput(this,"description",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"description",
			            _("Description"), tool->description,0,
			            0,0, pad,pad, lpad,lpad);
	last->tooltip(_("Optional tooltip for command."));
	AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	AddNull();


	// //     website     _______
	// last = website = new LineInput(this,"website",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
	// 					last,object_id,"website",
	// 		            _("website"), tool->website,0,
	// 		            0,0, pad,pad, lpad,lpad);
	// last->tooltip(_("Website (if any) for further information."));
	// AddWin(last,1, 300,0,10000,50,0, last->win_h,0,0,50,0, -1);
	// AddNull();


	//------------------------------ final ok -------------------------------------------------------

	if (!win_parent) {
		 //ok and cancel only when is toplevel...

		AddVSpacer(th/2,0,0,50,-1);
		AddNull();

		AddWin(NULL,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
		
		 // [ ok ]
		last=tbut=new Button(this,"ok",NULL,0,0,0,0,0,1, last,object_id,"Ok", BUTTON_OK);
		tbut->State(LAX_OFF);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels

		 // [ cancel ]
		last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL,0,0,0,0,1, last,object_id,"Cancel");
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

		AddWin(NULL,0, 0,0,3000,50,0, 20,0,0,50,0, -1);
	}


	
	last->CloseControlLoop();
	Sync(1);
	ValidityCheck();
	return 0;
}

int ExternalToolWindow::Event(const EventData *data,const char *mes)
{

	if (!strcmp(mes,"Ok")) {
		if (send()==0) return 0;
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;


	} else if (!strcmp(mes,"findpath")) { 
		const char *cmd = commandid->GetCText();
		makestr(tool->command_name, cmd);
		if (tool->FindPath()) {
			path->SetText(tool->binary_path);
			ValidityCheck();
		}
		return 0;

	} else if (!strcmp(mes,"name")) {
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		makestr(tool->name, e->str);
		ValidityCheck();
		return 0;

	} else if (!strcmp(mes,"command_id")) {
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		makestr(tool->command_name, e->str);
		ValidityCheck();
		return 0;
	
	} else if (!strcmp(mes,"path")) {
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		makestr(tool->binary_path, e->str);
		tool->Verify();
		ValidityCheck();
		return 0;

	} else if (!strcmp(mes,"parameters")) {
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		// anything goes
		makestr(tool->parameters, e->str);
		return 0;
		
	} else if (!strcmp(mes,"description")) {
		const SimpleMessage *e=dynamic_cast<const SimpleMessage *>(data);
		// anything goes
		makestr(tool->description, e->str);
		return 0;

	}

	return anXWindow::Event(data, mes);
}

/*! Make sure name and command_id are unique for category. Update grayed state of "ok" button. */
bool ExternalToolWindow::ValidityCheck()
{
	ExternalToolCategory *cat = nullptr;
	for (int c=0; c<laidout->prefs.external_tool_manager.external_categories.n; c++) {
		if (tool->category == laidout->prefs.external_tool_manager.external_categories.e[c]->id) {
			cat = laidout->prefs.external_tool_manager.external_categories.e[c];
			break;
		}
	}

	bool valid = tool->Valid(); //checks path only
	
	bool name_exists = false;
	bool command_exists = false;
	for (int c=0; c<cat->tools.n; c++) {
		if (cat->tools.e[c] == tool || cat->tools.e[c]->object_id == tool->object_id) continue; //don't check uniqueness on ourself

		if (strEquals(tool->name, cat->tools.e[c]->name)) name_exists = true;
		if (strEquals(tool->command_name, cat->tools.e[c]->command_name)) command_exists = true;
	}

	name->GetLineEdit()->Valid(!name_exists);
	commandid->GetLineEdit()->Valid(!command_exists);

	valid = (valid && !name_exists && !command_exists && !isblank(tool->name) && !isblank(tool->command_name));

	anXWindow *win = findChildWindowByName("ok");
	if (win) win->Grayed(!valid);
	return valid;
}

/*! Validate inputs and send if valid.
 *
 * Return 1 for ok, 0 for invalid and don't destroy dialog yet.
 */
int ExternalToolWindow::send()
{
	if (!tool || !win_owner) return 0;

	makestr(tool->name, name->GetCText());
	makestr(tool->command_name, commandid->GetCText());
	makestr(tool->binary_path, path->GetCText());
	makestr(tool->description, description->GetCText());
	makestr(tool->parameters, parameters->GetCText());
	if (website) makestr(tool->doc_website, website->GetCText());

	SimpleMessage *mes = new SimpleMessage(tool);
	app->SendMessage(mes,win_owner,win_sendthis,object_id);

	return 1;
}


} //namespace Laidout


