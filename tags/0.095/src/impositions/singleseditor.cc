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
// Copyright (C) 2010 by Tom Lechner
//

#include "singleseditor.h"
#include "../laidout.h"
#include "../language.h"

#include <lax/button.h>
#include <lax/messagebar.h>
#include <lax/sliderpopup.h>
#include <lax/units.h>


using namespace Laxkit;

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



/*! If doc!=NULL, then assume we are editing settings of that document.
 */
SinglesEditor::SinglesEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
							unsigned long nowner, const char *mes,
							Document *ndoc, Singles *simp, PaperStyle *paper)
		: RowFrame(parnt,nname,ntitle,ROWFRAME_HORIZONTAL|ROWFRAME_LEFT|ROWFRAME_VCENTER|ANXWIN_REMEMBER|ANXWIN_ESCAPABLE,
					0,0,500,500,0, NULL,nowner,mes,
					10)
{
	curorientation=0;

	doc=ndoc;
	if (doc) doc->inc_count();

	papertype=NULL;
	if (paper) papertype=(PaperStyle*)paper->duplicate(); else papertype=NULL;
	if (!papertype && doc && doc->imposition->paper && doc->imposition->paper->paperstyle)
		papertype=(PaperStyle*)doc->imposition->paper->paperstyle->duplicate();

	imp=NULL;
	if (simp) imp=(Singles*)simp->duplicate();

	tilex=tiley=NULL;
	gapx=gapy=NULL;
	insetl=insetr=insett=insetb=NULL;
	marginl=marginr=margint=marginb=NULL;
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
	
	int textheight=app->defaultlaxfont->textheight();
	int linpheight=textheight+12;
	anXWindow *last=NULL;
	LineInput *linp;


	 // -------------- Paper Size --------------------
	
	if (!papertype) papertype=(PaperStyle *)laidout->papersizes.e[0]->duplicate();

	char blah[100],blah2[100];
	int o=papertype->landscape();
	curorientation=o;

	 // -----Paper Size X
	SimpleUnit *units=GetUnitManager();
	sprintf(blah,"%.10g", units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,NULL));
	sprintf(blah2,"%.10g",units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,NULL));

	last=paperx=new LineInput(this,"paper x",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"paper x",
			            _("Paper Size  x:"),(o&1?blah2:blah),0,
			            100,0,1,1,3,3);
	AddWin(paperx,1, paperx->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	
	 // -----Paper Size Y
	last=papery=new LineInput(this,"paper y",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"paper y",
			            _("y:"),(o&1?blah:blah2),0,
			           100,0,1,1,3,3);
	AddWin(papery,1, papery->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak

	 // -----Paper Name
    SliderPopup *popup;
	last=popup=new SliderPopup(this,"paperName",NULL,0, 0,0, 0,0, 1, last,object_id,"paper name");
	int c2=-1;
	for (int c=0; c<laidout->papersizes.n; c++) {
		if (!strcmp(laidout->papersizes.e[c]->name,papertype->name)) c2=c;
		popup->AddItem(laidout->papersizes.e[c]->name,c);
	}
	popup->Select(c2);
	AddWin(popup,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	
	 // -----Paper Orientation
	last=popup=new SliderPopup(this,"paperOrientation",NULL,0, 0,0, 0,0, 1, last,object_id,"orientation");
	popup->AddItem(_("Portrait"),0);
	popup->AddItem(_("Landscape"),1);
	popup->Select(o&1?1:0);
	AddWin(popup,1, 200,100,50,50,0, linpheight,0,0,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);// forced linebreak

	 //add thin spacer
	AddSpacer(2000,2000,0,50, textheight*2/3,0,0,0);// forced vertical spacer


	 // ------Tiling
	last=tiley=new LineInput(this,"y tiling",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"ytile",
			            _("Tile y:"),"1", 0,
			           100,0,1,1,3,3);
	if (imp) tiley->SetText(imp->tiley);
	tiley->tooltip("How many times to repeat a spread vertically on a paper.");
	AddWin(tiley,1, tiley->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	last=tilex=new LineInput(this,"x tiling",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"xtile",
			            _("Tile x:"),"1", 0,
			           100,0,1,1,3,3);
	if (imp) tilex->SetText(imp->tilex);
	tilex->tooltip(_("How many times to repeat a spread horizontally on a paper."));
	AddWin(tilex,1, tilex->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();

	last=gapy=new LineInput(this,"y gap",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"ygap",
			            _("Gap y:"),"1", 0,
			           100,0,1,1,3,3);
	if (imp) gapy->SetText(units->Convert(imp->gapy,UNITS_Inches,laidout->prefs.default_units,NULL));
	gapy->tooltip("Gap between tiles vertically");
	AddWin(gapy,1, gapy->win_w,0,50,50,0, linpheight,0,0,50,0, -1);

	last=gapx=new LineInput(this,"x gap",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"xgap",
			            _("Gap x:"),"1", 0,
			           100,0,1,1,3,3);
	if (imp) gapx->SetText(units->Convert(imp->gapx,UNITS_Inches,laidout->prefs.default_units,NULL));
	gapx->tooltip("Gap between tiles horizontally");
	AddWin(gapx,1, gapx->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();


	 // -------  paper inset
	//AddWin(new MessageBar(this,"Paper inset",MB_MOVE|MB_LEFT, 0,0,0,0,0, _("Paper inset:")));
	//AddWin(NULL, 2000,2000,0,50,0, 0,0,0,0);//forced linebreak, makes left justify

	last=linp=insett=new LineInput(this,"inset t",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"inset t",
			            _("Inset Top:"),NULL,0,
			            0,0,3,0,3,3);
	linp->tooltip(_("Amount to chop from paper before applying tiling"));
	if (imp) linp->SetText(units->Convert(imp->insettop,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last=linp=insetb=new LineInput(this,"inset b",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"inset b",
			            _("Inset Bottom:"),NULL,0,
			            0,0,3,0,3,3);
	linp->tooltip(_("Amount to chop from paper before applying tiling"));
	if (imp) linp->SetText(units->Convert(imp->insetbottom,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	last=linp=insetl=new LineInput(this,"inset l",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"inset l",
			            _("Inset Left:"),NULL,0,
			            0,0,3,0,3,3);
	linp->tooltip(_("Amount to chop from paper before applying tiling"));
	if (imp) linp->SetText(units->Convert(imp->insetleft,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last=linp=insetr=new LineInput(this,"inset r",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"inset r",
			            _("Inset Right:"),NULL,0,
			            0,0,3,0,3,3);
	linp->tooltip(_("Amount to chop from paper before applying tiling"));
	if (imp) linp->SetText(units->Convert(imp->insetright,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	

	 //add thin spacer
	AddSpacer(2000,2000,0,50, textheight*2/3,0,0,0);// forced vertical spacer

	


	 // --------------------- page specific setup ------------------------------------------
	
	//***first page is page number: 	___1_

//		imp->insetl=marginl->GetDouble();
//		imp->insetr=marginr->GetDouble();
//		imp->insett=margint->GetDouble();
//		imp->insetb=marginb->GetDouble();
	 // ------------------ margins ------------------
	AddWin(new MessageBar(this,"page margins",NULL,MB_MOVE, 0,0,0,0,0, _("Default page margins:")), 1,-1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//forced linebreak, makes left justify

	last=linp=margint=new LineInput(this,"margin t",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin t",
			            _("Top Margin"),NULL,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->margintop,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last=linp=marginb=new LineInput(this,"margin b",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin b",
			            _("Bottom Margin"),NULL,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->marginbottom,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	
	last=linp=marginl=new LineInput(this,"margin l",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin l",
			            _("Left Margin"),NULL,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->marginleft,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	
	last=linp=marginr=new LineInput(this,"margin r",NULL,LINP_ONLEFT,
			            5,250,0,0, 0, 
						last,object_id,"margin r",
			            _("Right Margin"),NULL,0,
			            0,0,3,0,3,3);
	if (imp) linp->SetText(units->Convert(imp->marginright,UNITS_Inches,laidout->prefs.default_units,NULL));
	AddWin(linp,1, 150,0,50,50,0, linpheight,0,0,50,0, -1);
	AddNull();
	




	
	//------------------------------ final ok -------------------------------------------------------

//	AddWin(NULL,0, 2000,1990,0,50,0, 20,0,0,50,0, -1);
//	
//	 // [ ok ]   [ cancel ]
//	AddHSpacer(0,0,1000,50);
//	Button *tbut;
//	last=tbut=new Button(this,"ok",NULL,0, 0,0,0,0,1, last,object_id,"Ok",
//						 BUTTON_OK,
//						 NULL,
//						 NULL,NULL);
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels
//
//	last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"Cancel");
//	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
//	AddHSpacer(0,0,1000,50);


	
	last->CloseControlLoop();
	Sync(1);
	return 0;
}


int SinglesEditor::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"newdocmessage: "<<(mes?mes:"(unknown)")<<endl;

	if (!strcmp(mes,"paper name")) { 
		 // new paper selected from the popup, so must find the x/y and set x/y appropriately
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		int i=s->info1;
		DBG cerr <<"new paper size:"<<i<<endl;
		if (i<0 || i>=laidout->papersizes.n) return 0;
		delete papertype;
		papertype=(PaperStyle *)laidout->papersizes.e[i]->duplicate();
		if (!strcmp(papertype->name,"custom")) return 0;
		papertype->landscape(curorientation);

		char num[30];
		SimpleUnit *units=GetUnitManager();
		numtostr(num,30,units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		paperx->SetText(num);
		numtostr(num,30,units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		papery->SetText(num);
		UpdatePaper(1);
		//*** would also reset page size x/y based on Layout Scheme, and if page is Default
		return 0;

	} else if (!strcmp(mes,"orientation")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int l=s->info1;
		DBG cerr <<"New orientation:"<<l<<endl;
		if (l!=curorientation) {
			char *txt=paperx->GetText(),
				*txt2=papery->GetText();
			paperx->SetText(txt2);
			papery->SetText(txt);
			delete[] txt;
			delete[] txt2;
			curorientation=(l?1:0);
			papertype->landscape(curorientation);
			UpdatePaper(1);
		}
		return 0;

	} else if (!strcmp(mes,"paper x")) {
		//***should switch to custom paper maybe
		makestr(papertype->name,_("Custom"));

	} else if (!strcmp(mes,"paper y")) {
		//***should switch to custom paper maybe
		makestr(papertype->name,_("Custom"));

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
	imp=NULL; //disable so as to not delete it in ~SinglesEditor()
	
	int npgs=(doc?doc->pages.n:1);
	int xtile=1, ytile=1;
	if (tilex) xtile=atoi(tilex->GetCText());
	if (tiley) ytile=atoi(tiley->GetCText());

	SimpleUnit *units=GetUnitManager();
	double xgap=0, ygap=0;
	if (gapx) xgap=units->Convert(atof(gapx->GetCText()),laidout->prefs.default_units,UNITS_Inches,NULL);
	if (gapy) ygap=units->Convert(atof(gapy->GetCText()),laidout->prefs.default_units,UNITS_Inches,NULL);

	if (npgs<=0) npgs=1;

	if (xtile<=0) xtile=1;
	if (ytile<=0) ytile=1;

	imposition->tilex=xtile;
	imposition->tiley=ytile;
	imposition->gapx=xgap;
	imposition->gapy=ygap;
	imposition->insetleft  =units->Convert(insetl->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL);
	imposition->insetright =units->Convert(insetr->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL);
	imposition->insettop   =units->Convert(insett->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL);
	imposition->insetbottom=units->Convert(insetb->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL);
	imposition->SetDefaultMargins(
					units->Convert(marginl->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL),
					units->Convert(marginr->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL),
					units->Convert(margint->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL),
					units->Convert(marginb->GetDouble(),laidout->prefs.default_units,UNITS_Inches,NULL)
				);

	imposition->NumPages(npgs);
	imposition->SetPaperSize(papertype);

	RefCountedEventData *data=new RefCountedEventData(imposition);
	imposition->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);
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
		papertype=(PaperStyle*)imp->papergroup->papers.e[0]->box->paperstyle->duplicate();

		 //update orientation
		SliderPopup *o=NULL;
		curorientation=papertype->landscape();
		o=dynamic_cast<SliderPopup*>(findChildWindowByName("paperOrientation"));
		o->Select(curorientation?1:0);

		 //update name
		o=dynamic_cast<SliderPopup*>(findChildWindowByName("paperName"));
		for (int c=0; c<laidout->papersizes.n; c++) {
			if (!strcasecmp(laidout->papersizes.e[c]->name,papertype->name)) {
				o->Select(c);
				break;
			}
		}

		 //update dimensions
		char num[30];
		SimpleUnit *units=GetUnitManager();
		numtostr(num,30,units->Convert(papertype->w(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		paperx->SetText(num);
		numtostr(num,30,units->Convert(papertype->h(),UNITS_Inches,laidout->prefs.default_units,NULL),0);
		papery->SetText(num);
	}
}

const char *SinglesEditor::ImpositionType()
{ return "Singles"; }

Imposition *SinglesEditor::GetImposition()
{ return imp; }

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

