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
// Copyright (C) 2010 by Tom Lechner
//

#include "singleseditor.h"
#include "../laidout.h"
#include "../language.h"

#include <lax/button.h>
#include <lax/messagebar.h>
#include <lax/sliderpopup.h>
#include <lax/numslider.h>
#include <lax/units.h>
#include <lax/interfaces/interfacemanager.h>


using namespace Laxkit;
using namespace LaxInterfaces;

#define DBG



namespace Laidout {


//--------------------------------- SinglesEditor ------------------------------------
/*! \class SinglesEditor
 * \brief Editor for Singles imposition objects.
 *
 * This is currently a god awful hack to get things off the ground.
 * Must be restructured to actually provide interactive trim and margin handles,
 * as well as spread definitions to arbitrarily arrange singles next to each other.
 * Page bleeding is all visual according to spreads.
 */



/*! If doc!=nullptr, then assume we are editing settings of that document.
 */
SinglesEditor::SinglesEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
							unsigned long nowner, const char *mes,
							Document *ndoc, Singles *simp, PaperStyle *paper, bool escapable)
		: RowFrame(parnt,nname,ntitle,
					ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ROWFRAME_VCENTER|ANXWIN_REMEMBER|(escapable ? ANXWIN_ESCAPABLE : 0),
					0,0,500,500,0, nullptr,nowner,mes,
					10)
{
	curorientation=0;

	doc=ndoc;
	if (doc) doc->inc_count();

	papertype=nullptr;
	if (paper) papertype=(PaperStyle*)paper->duplicate(); else papertype=nullptr;
	if (!papertype && doc && doc->imposition->paper && doc->imposition->paper->paperstyle)
		papertype=(PaperStyle*)doc->imposition->paper->paperstyle->duplicate();

	imp=nullptr;
	if (simp) imp=(Singles*)simp->duplicate();

	tilex = tiley = nullptr;
	gapx = gapy = nullptr;
	insetl = insetr = insett = insetb = nullptr;
	marginl = marginr = margint = marginb = nullptr;
	psizewindow = nullptr;
}

SinglesEditor::~SinglesEditor()
{
	if (doc) doc->dec_count();
	if (imp) imp->dec_count();
	delete papertype;
}

int SinglesEditor::preinit()
{
	anXWindow::preinit();
	if (win_w<=0) win_w=500;
	if (win_h<=0) win_h=600;
	return 0;
}

int SinglesEditor::init()
{
	
	int textheight  = UIScale() * win_themestyle->normal->textheight(); //app->defaultlaxfont->textheight();
	int linpheight  = 1.5*textheight;
	anXWindow *last = nullptr;
	LineInput *linp = nullptr;

	UnitManager *units = GetUnitManager();


	 //--------------- PaperGroup ----------------
	
	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
    ResourceManager *rm = imanager->GetResourceManager();
    MenuInfo *papergroup_menu = rm->ResourceMenu("PaperGroup", false, nullptr, 0, 0);

    if (papergroup_menu) {
		AddWin(new MessageBar(this,"papergroup",nullptr,0, 0,0,0,0,0, _("Paper Group:")), 1,-1);
		SliderPopup *groups = new SliderPopup(this, "papergroups",nullptr,
				SLIDER_POP_ONLY, 0,0,0,0,0,
				last, object_id, "papergroup",
				papergroup_menu,1);
		AddWin(groups,1, groups->win_w,0,0,50,0, 1.25 * textheight,0,0,50,0, -1);
		AddNull();
    } else {
    	AddWin(new MessageBar(this,"papergroup",nullptr,0, 0,0,0,0,0, _("Paper Group: default")), 1,-1);
    }
	AddVSpacer(textheight,0,0,50);
	AddNull();


	 // -------------- Paper Size --------------------
	AddWin(new MessageBar(this,"defaultpage",nullptr,MB_MOVE, 0,0,0,0,0, _("Default paper size:")), 1,-1);
	AddNull();

	if (!papertype) papertype=(PaperStyle *)laidout->papersizes.e[0]->duplicate();

	int o = papertype->landscape();
	curorientation = o;

				// int npw,int nws,int nwg,int nhalign,int nhgap,
				// int nph,int nhs,int nhg,int nvalign,int nvgap,
	psizewindow = new PaperSizeWindow(this, "psizewindow", nullptr, 0, object_id, "papersize", 
										papertype, false, true, false, true);
	AddWin(psizewindow,1, -1);
	//AddWin(psizewindow,1, 5000,4900,0,50,0, 4*textheight,0,0,50,0, -1);
	AddNull();
	AddVSpacer(textheight,0,0,50);
	AddNull();


	//  // ------Tiling
	// last=tiley=new LineInput(this,"y tiling",nullptr,LINP_ONLEFT, 0,0,0,0, 0, 
	// 					last,object_id,"ytile",
	// 		            _("Tile y:"),"1", 0,
	// 		           100,0,1,1,3,3);
	// if (imp) tiley->SetText(imp->tiley);
	// tiley->tooltip("How many times to repeat a spread vertically on a paper.");
	// AddWin(tiley,1, tiley->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	// last=tilex=new LineInput(this,"x tiling",nullptr,LINP_ONLEFT, 0,0,0,0, 0, 
	// 					last,object_id,"xtile",
	// 		            _("Tile x:"),"1", 0,
	// 		           100,0,1,1,3,3);
	// if (imp) tilex->SetText(imp->tilex);
	// tilex->tooltip(_("How many times to repeat a spread horizontally on a paper."));
	// AddWin(tilex,1, tilex->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	// AddNull();

	// last=gapy=new LineInput(this,"y gap",nullptr,LINP_ONLEFT, 0,0,0,0, 0, 
	// 					last,object_id,"ygap",
	// 		            _("Gap y:"),"1", 0,
	// 		           100,0,1,1,3,3);
	// if (imp) gapy->SetText(units->Convert(imp->gapy,UNITS_Inches,laidout->prefs.default_units,nullptr));
	// gapy->tooltip("Gap between tiles vertically");
	// AddWin(gapy,1, gapy->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	// last=gapx=new LineInput(this,"x gap",nullptr,LINP_ONLEFT, 0,0,0,0, 0, 
	// 					last,object_id,"xgap",
	// 		            _("Gap x:"),"1", 0,
	// 		           100,0,1,1,3,3);
	// if (imp) gapx->SetText(units->Convert(imp->gapx,UNITS_Inches,laidout->prefs.default_units,nullptr));
	// gapx->tooltip("Gap between tiles horizontally");
	// AddWin(gapx,1, gapx->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	// AddNull();


	//  // -------  paper inset
	// //AddWin(new MessageBar(this,"Paper inset",MB_MOVE|MB_LEFT, 0,0,0,0,0, _("Paper inset:")));
	// //AddWin(nullptr, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak, makes left justify

	// last=linp=insett=new LineInput(this,"inset t",nullptr,LINP_ONLEFT,
	// 		            5,250,0,0, 0, 
	// 					last,object_id,"inset t",
	// 		            _("Inset Top:"),nullptr,0,
	// 		            0,0,3,0,3,3);
	// linp->tooltip(_("Amount to chop from paper before applying tiling"));
	// if (imp) linp->SetText(units->Convert(imp->insettop,UNITS_Inches,laidout->prefs.default_units,nullptr)); 
	// AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	//  // ******
	// //NumSlider *slider=new NumSlider(this,"inset t",nullptr, NumSlider::NO_MAXIMUM,
	// //		            0,0,0,linpheight, 0, 
	// //					last,object_id,"inset t2",
	// //					nullptr, 0,1000, units->Convert(imp->insettop,UNITS_Inches,laidout->prefs.default_units,nullptr), .05);
	// //last=slider;
	// //slider->tooltip(_("Amount to chop from paper before applying tiling"));
	// //AddWin(slider,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	// last=linp=insetb=new LineInput(this,"inset b",nullptr,LINP_ONLEFT,
	// 		            5,250,0,0, 0, 
	// 					last,object_id,"inset b",
	// 		            _("Inset Bottom:"),nullptr,0,
	// 		            0,0,3,0,3,3);
	// linp->tooltip(_("Amount to chop from paper before applying tiling"));
	// if (imp) linp->SetText(units->Convert(imp->insetbottom,UNITS_Inches,laidout->prefs.default_units,nullptr));
	// AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	// AddNull();
	
	// last=linp=insetl=new LineInput(this,"inset l",nullptr,LINP_ONLEFT,
	// 		            5,250,0,0, 0, 
	// 					last,object_id,"inset l",
	// 		            _("Inset Left:"),nullptr,0,
	// 		            0,0,3,0,3,3);
	// linp->tooltip(_("Amount to chop from paper before applying tiling"));
	// if (imp) linp->SetText(units->Convert(imp->insetleft,UNITS_Inches,laidout->prefs.default_units,nullptr));
	// AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	// last=linp=insetr=new LineInput(this,"inset r",nullptr,LINP_ONLEFT,
	// 		            5,250,0,0, 0, 
	// 					last,object_id,"inset r",
	// 		            _("Inset Right:"),nullptr,0,
	// 		            0,0,3,0,3,3);
	// linp->tooltip(_("Amount to chop from paper before applying tiling"));
	// if (imp) linp->SetText(units->Convert(imp->insetright,UNITS_Inches,laidout->prefs.default_units,nullptr));
	// AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	// AddNull();
	
	

	 //add thin spacer
	//AddSpacer(2000,2000,0,50, textheight*2/3,0,0,0);// forced vertical spacer
	//AddVSpacer(textheight*2/3,0,0,0);

	


	 // --------------------- page specific setup ------------------------------------------
	
	//***first page is page number: 	___1_

//		imp->insetl=marginl->GetDouble();
//		imp->insetr=marginr->GetDouble();
//		imp->insett=margint->GetDouble();
//		imp->insetb=marginb->GetDouble();
	 // ------------------ margins ------------------
	AddWin(new MessageBar(this,"page margins",nullptr,MB_MOVE, 0,0,0,0,0, _("Default page margins:")), 1,-1);
	//AddWin(nullptr,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//forced linebreak, makes left justify
	AddNull();

	last=linp=margint=new LineInput(this,"margin t",nullptr,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin t",
			            _("Top Margin"),nullptr,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->margintop,UNITS_Inches,laidout->prefs.default_units,nullptr));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last=linp=marginb=new LineInput(this,"margin b",nullptr,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin b",
			            _("Bottom Margin"),nullptr,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->marginbottom,UNITS_Inches,laidout->prefs.default_units,nullptr));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	last=linp=marginl=new LineInput(this,"margin l",nullptr,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin l",
			            _("Left Margin"),nullptr,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->marginleft,UNITS_Inches,laidout->prefs.default_units,nullptr));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last=linp=marginr=new LineInput(this,"margin r",nullptr,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin r",
			            _("Right Margin"),nullptr,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->marginright,UNITS_Inches,laidout->prefs.default_units,nullptr));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	

	AddVSpacer(textheight,0,0,50);
	AddNull();

	
	//------------------------------ final ok -------------------------------------------------------

//	AddWin(nullptr,0, 2000,1990,0,50,0, 20,0,0,50,0, -1);
//	
//	 // [ ok ]   [ cancel ]
//	AddHSpacer(0,0,1000,50);
//	Button *tbut;
//	last=tbut=new Button(this,"ok",nullptr,0, 0,0,0,0,1, last,object_id,"Ok",
//						 BUTTON_OK,
//						 nullptr,
//						 nullptr,nullptr);
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddWin(nullptr,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels
//
//	last=tbut=new Button(this,"cancel",nullptr,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddHSpacer(0,0,1000,50);


	
	last->CloseControlLoop();
	Sync(1);
	return 0;
}


int SinglesEditor::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

	if (!strcmp(mes,"papersize")) { 
		const SimpleMessage *s = dynamic_cast<const SimpleMessage *>(data);
		PaperStyle *paper = dynamic_cast<PaperStyle*>(s->object);
		if (!paper) return 0;
		delete papertype;
		papertype = dynamic_cast<PaperStyle*>(paper->duplicate());
		UpdatePaper(1);
		return 0;
	
	} else if (!strcmp(mes,"inset l")) {
	} else if (!strcmp(mes,"inset r")) {
	} else if (!strcmp(mes,"inset t")) {
	} else if (!strcmp(mes,"inset b")) {
	} else if (!strcmp(mes,"y tiling")) {
	} else if (!strcmp(mes,"x tiling")) {
	} else if (!strcmp(mes,"ygap")) {
	} else if (!strcmp(mes,"xgap")) {
	} else if (!strcmp(mes,"margit t")) {
	} else if (!strcmp(mes,"margit b")) {
	} else if (!strcmp(mes,"margit l")) {
	} else if (!strcmp(mes,"margit r")) {

	} else if (!strcmp(mes,"Ok")) {
		send();
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"Cancel")) {
		if (win_parent) app->destroywindow(win_parent);
		else app->destroywindow(this);
		return 0;
	}

	return anXWindow::Event(data,mes);
}

void SinglesEditor::send()
{
	Singles *imposition=imp; 
	imp=nullptr; //disable so as to not delete it in ~SinglesEditor()
	
	int npgs=(doc?doc->pages.n:1);
	int xtile=1, ytile=1;
	if (tilex) xtile=atoi(tilex->GetCText());
	if (tiley) ytile=atoi(tiley->GetCText());

	UnitManager *units = GetUnitManager();
	double xgap=0, ygap=0;
	if (gapx) xgap=units->Convert(atof(gapx->GetCText()),laidout->prefs.default_units,UNITS_Inches,nullptr);
	if (gapy) ygap=units->Convert(atof(gapy->GetCText()),laidout->prefs.default_units,UNITS_Inches,nullptr);

	if (npgs<=0) npgs=1;

	if (xtile<=0) xtile=1;
	if (ytile<=0) ytile=1;

	imposition->tilex = xtile;
	imposition->tiley = ytile;
	imposition->gapx = xgap;
	imposition->gapy = ygap;
	// imposition->insetleft  =units->Convert(insetl->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	// imposition->insetright =units->Convert(insetr->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	// imposition->insettop   =units->Convert(insett->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	// imposition->insetbottom=units->Convert(insetb->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	imposition->SetDefaultMargins(
					units->Convert(marginl->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr),
					units->Convert(marginr->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr),
					units->Convert(margint->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr),
					units->Convert(marginb->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr)
				);

	imposition->NumPages(npgs);
	imposition->SetPaperSize(papertype);

	RefCountedEventData *data=new RefCountedEventData(imposition);
	imposition->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);
}

Imposition *SinglesEditor::GetImposition()
{
	int npgs=(doc?doc->pages.n:1);
	int xtile=1, ytile=1;
	if (tilex) xtile=atoi(tilex->GetCText());
	if (tiley) ytile=atoi(tiley->GetCText());

	UnitManager *units = GetUnitManager();
	double xgap=0, ygap=0;
	if (gapx) xgap=units->Convert(atof(gapx->GetCText()),laidout->prefs.default_units,UNITS_Inches,nullptr);
	if (gapy) ygap=units->Convert(atof(gapy->GetCText()),laidout->prefs.default_units,UNITS_Inches,nullptr);

	if (npgs<=0) npgs=1;

	if (xtile<=0) xtile=1;
	if (ytile<=0) ytile=1;

	imp->tilex=xtile;
	imp->tiley=ytile;
	imp->gapx=xgap;
	imp->gapy=ygap;
	// imp->insetleft  =units->Convert(insetl->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	// imp->insetright =units->Convert(insetr->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	// imp->insettop   =units->Convert(insett->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	// imp->insetbottom=units->Convert(insetb->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr);
	imp->SetDefaultMargins(
					units->Convert(marginl->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr),
					units->Convert(marginr->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr),
					units->Convert(margint->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr),
					units->Convert(marginb->GetDouble(),laidout->prefs.default_units,UNITS_Inches,nullptr)
				);

	imp->NumPages(npgs);
	imp->SetPaperSize(papertype);

	return imp;
}

//! Make sure the dialog and the imposition have sync'd up paper types.
/*! If dialogtoimp!=0, then update imp with the dialog's paper, otherwise
 * update the dialog to reflect the imposition paper.
 */
void SinglesEditor::UpdatePaper(int dialogtoimp)
{
	if (dialogtoimp) {
		if (imp) imp->SetPaperSize(papertype);
		return;
	}

	if (!imp) return;

	if (imp->papergroup 
			&& imp->papergroup->papers.n
			&& imp->papergroup->papers.e[0]->box
			&& imp->papergroup->papers.e[0]->box->paperstyle) {

		if (papertype) delete papertype;
		papertype = (PaperStyle*)imp->papergroup->papers.e[0]->box->paperstyle->duplicate();
		psizewindow->UsePaper(papertype, false);
	}
}

const char *SinglesEditor::ImpositionType()
{ return "Singles"; }

/*! Note, changes document, NOT the imposition.
 */
int SinglesEditor::UseThisDocument(Document *ndoc)
{
	if (doc!=ndoc) {
		if (doc) doc->dec_count();
		doc=ndoc;
		if (doc) doc->inc_count();
	}
	return 0;
}

/*! Return 0 for success, 1 for not a Singles imposition.
 */
int SinglesEditor::UseThisImposition(Imposition *nimp)
{
	if (!dynamic_cast<Singles*>(nimp)) return 1;

	if (imp!=nimp) {
		if (imp) imp->dec_count();
		imp=dynamic_cast<Singles*>(nimp);
		if (imp) imp->inc_count();
	}
	UpdatePaper(0);
	return 0;

}

void SinglesEditor::ShowSplash(int yes)
{ }

} // namespace Laidout

