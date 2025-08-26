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

#include <lax/laxutils.h>
#include <lax/filedialog.h>

#include "../language.h"
#include "../core/utils.h"
#include "../filetypes/scribus.h"
#include "../ui/viewwindow.h"
#include "../ui/headwindow.h"
#include "impositioneditor.h"
#include "signatureinterface.h"
#include "singlesinterface.h"

#include "netimposition.h"
#include "netdialog.h"


#include <lax/debug.h>
using namespace std;

using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


//----------------------------- generic Imposition Editor creator -------------------------------

/*! Create the correct editor for type and/or imp. type overrides imp.
 * doc count gets incremented.
 */
Laxkit::anXWindow *newImpositionEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner, const char *mes,
						PaperStyle *papertype, const char *type, Imposition *imp, const char *imposearg,
						Document *doc)
{
	if (!type && !imp) return nullptr;

	anXWindow *win = nullptr;

	int dec = 0;
	if (!imp && type) {
		if      (!strcmp(type,"NetImposition"))       imp = new NetImposition;
		else if (!strcmp(type,"Singles"))             imp = new Singles;
		else if (!strcmp(type,"SignatureImposition")) imp = new SignatureImposition;
		if (imp) dec = 1;
	}

	win = new ImpositionEditor(parnt,nname,ntitle,
						nowner,mes,
						doc,imp,nullptr,
						imposearg
						);

	if (dec) imp->dec_count();
	return win;
}


//----------------------------- ImpositionEditor -------------------------------

#define WHICH_Signature 1
#define WHICH_Net       2
#define WHICH_Singles   3

/*! \class ImpositionEditor
 * \brief A Laxkit::ViewerWindow that gets filled with stuff appropriate for imposition editing.
 *
 * Currently, imposearg is "in=infile out=outfile prefer=booklet width=(paper width) height=(paper height).
 */


//! Make the window using project.
/*! Inc count of ndoc.
 */
ImpositionEditor::ImpositionEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner, const char *mes,
						Document *ndoc,
						Imposition *imposition,
						PaperStyle *p,
						const char *imposearg
						)
	: ViewerWindow(parnt,nname,ntitle,
				   ANXWIN_REMEMBER
					| VIEWPORT_RIGHT_HANDED | VIEWPORT_BACK_BUFFER | VIEWPORT_NO_RULERS,  //| VIEWPORT_NO_SCROLLERS
					0,0,500,500, 0, nullptr)
{
	SetOwner(nowner, mes);
	doc = ndoc;
	if (doc) doc->inc_count();

	whichactive = 0;
	firstimp = imposition;
	if (!firstimp && doc) firstimp = ndoc->imposition;
	if (firstimp) firstimp->inc_count();


	tool            = nullptr;
	tool_signatures = nullptr;
	tool_net        = nullptr;
	tool_singles    = nullptr;
	neteditor       = nullptr;
	rescale_pages   = 1;

	if (!viewport) {
		viewport = new ViewportWindow(this,"imposition-editor-viewport","imposition-editor-viewport",
									ANXWIN_HOVER_FOCUS | VIEWPORT_RIGHT_HANDED | VIEWPORT_BACK_BUFFER | VIEWPORT_ROTATABLE,
									0,0,0,0,0,nullptr);
		viewport->win_themestyle->bg.rgbf(.9,.9,.9);
		app->reparent(viewport,this);
		viewport->dec_count();
	}

	WindowStyle *style = win_themestyle->duplicate();
	InstallColors(style);
	win_themestyle->bg.rgbf(200/255.,200/255.,200/255.);
	viewport->dp->NewBG(200,200,200);
	needtodraw = 1;


	// **** this is a hack! Should instead be parsed into an export config with extra fields for additional imposing
	imposeout    = nullptr;
	imposeformat = nullptr;
	double ww = -1, hh = -1;

	if (imposearg) {
		//need to load a new document, which may be a non-laidout document.
		//If non-laidout, then create new singles, and import
		//add extra field for impose out

		DBG const char *in = "", *out = "";

		show_splash = true;

		Attribute att;
		NameValueToAttribute(&att,imposearg,'=',',');

		const char *prefer = nullptr;
		const char *name, *value;
		int docindex = laidout->project->docs.n;

		for (int c = 0; c < att.attributes.n; c++) {
			name  = att.attributes.e[c]->name;
			value = att.attributes.e[c]->value;
			if (!strcmp(name,"in")) {
				DBG in = value;

				if (isScribusFile(value)) {
					if (addScribusDocument(value) == 0) {
						//yikes! crash magnet!
					}

				} else if (isPdfFile(value,nullptr)) {
					//  pdf: if you know number of pages, create singles with that many pages and page sizes,
					//     export temp podofo plan, call podofoimpose?
					//     or figure out how to remap pdf pages.. should just be matter of 
					//       adjusting root page streams with extra transforms, calling old page streams
					//       ..and adding extra markings
					//       updating pdf object catalog
					//       use ghostscript to generate pdf page preview images, store at ??? (dir of file.pdf)/.file.pdf.page01.png???
				}
				//else if (isLaidoutDocumentFile) {
				//	...load normally
				//

			} else if (!strcmp(name,"out")) {
				makestr(imposeout,value);
				DBG out = value;

			} else if (!strcmp(name,"prefer")) {
				prefer = value;

			} else if (!strcmp(name,"width")) {
				DoubleAttribute(value,&ww,nullptr);

			} else if (!strcmp(name,"height")) {
				DoubleAttribute(value,&hh,nullptr);
			}
		}

		if (prefer) {
			int c;
			for (c = 0; c < laidout->impositionpool.n; c++) {
				if (!strcasecmp(laidout->impositionpool.e[c]->name, prefer)) break;
			}
			if (c < laidout->impositionpool.n) {
				if (firstimp) firstimp->dec_count();
				firstimp = laidout->impositionpool.e[c]->Create();
			}
		}

		if (laidout->project->docs.n > docindex) { // was successful doc load in
			if (doc) doc->dec_count();
			doc = laidout->project->docs.e[docindex]->doc;
		}
		if (ww > 0 && hh > 0) tool->SetTotalDimensions(ww,hh);

		DBG cerr <<"Impose only from "<<in<<" to "<<out<<endl;
	}

	if (firstimp) {
		if      (dynamic_cast<NetImposition*>(firstimp))       whichactive = WHICH_Net;
		else if (dynamic_cast<Singles*>(firstimp))             whichactive = WHICH_Singles;
		else if (dynamic_cast<SignatureImposition*>(firstimp)) whichactive = WHICH_Signature;
	}

	if (whichactive == 0) whichactive = WHICH_Signature;
}

ImpositionEditor::~ImpositionEditor()
{ 
	if (imposeout) delete[] imposeout;
	if (imposeformat) delete[] imposeformat;

	if (doc)             doc            ->dec_count();
	if (firstimp)        firstimp       ->dec_count();
	if (tool)            tool           ->dec_count();
	if (tool_singles)    tool_singles   ->dec_count();
	if (tool_signatures) tool_signatures->dec_count();
	if (tool_net)        tool_net       ->dec_count();
}

//! Passes off to SignatureInterface::dump_out().
void ImpositionEditor::dump_out(FILE *f, int indent, int what, Laxkit::DumpContext *context)
{
    Attribute att;
	dump_out_atts(&att,0,context);
    att.dump_out(f,indent);
}

Laxkit::Attribute *ImpositionEditor::dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context)
{
	att = anXWindow::dump_out_atts(att, what, context);
	if (!att) att = new Attribute();
	const char *which = nullptr;
	if      (whichactive == WHICH_Signature) which = "Signature";
	else if (whichactive == WHICH_Singles)   which = "Singles";
	else if (whichactive == WHICH_Net)       which = "Net";
	att->push("which", which);
	return att;
}

//! Passes off to SignatureInterface::dump_in_atts().
void ImpositionEditor::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);
	const char *name;
	const char *value;
	for (int c = 0; c < att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;
		if (!strcmp(name, "which")) {
			if      (strEquals(value, "Signature")) whichactive = WHICH_Signature;
			else if (strEquals(value, "Singles"))   whichactive = WHICH_Singles;
			else if (strEquals(value, "Net"))       whichactive = WHICH_Net;
		}
	}
}


#define IMP_NEW_SINGLES    10000
#define IMP_NEW_SIGNATURE  10001
#define IMP_NEW_NET        10002
#define IMP_FROM_FILE      10003
#define IMP_CURRENT        10004

/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int ImpositionEditor::init()
{
	ViewerWindow::init();
	viewport->dp->NewBG(200,200,200);

	if (!whichactive) whichactive = WHICH_Signature;
	// if (!tool) {
	// 	whichactive = WHICH_Signature;
	// 	tool = tool_signatures = new SignatureInterface(nullptr,1,viewport->dp, nullptr,p,ndoc);
	// 	tool->inc_count();
	// }

	// tool->GetShortcuts();
	// if (firstimp) tool->UseThisImposition(firstimp);
	// if (doc) {
	// 	tool->UseThisDocument(ndoc);
	// 	tool->SetPaper(doc->imposition->papergroup->papers.e[0]->box->paperstyle);
	// }
	
	// if (tool) AddTool(tool,1,0); // select it, absorb


	anXWindow *last = nullptr;
	Button *tbut;

	int textheight = UIScale() * app->defaultlaxfont->textheight();

	//AddNull();

	MenuInfo *menu = new MenuInfo();
	menu->AddSep(_("Change to"));
	for (int c=0; c<laidout->impositionpool.n; c++) {
		menu->AddItem(laidout->impositionpool.e[c]->name,c);
	}

	 // *** these need to be all the imposition base creation types
	menu->AddSep();
	menu->AddItem(_("NEW Singles..."),   IMP_NEW_SINGLES);
	menu->AddItem(_("NEW Signature..."), IMP_NEW_SIGNATURE);
	menu->AddItem(_("NEW Net..."),       IMP_NEW_NET);
	menu->AddItem(_("From file..."),     IMP_FROM_FILE);

	type_mbut = new MenuButton(this,"imp",nullptr,MENUBUTTON_DOWNARROW|MENUBUTTON_LEFT|MENUBUTTON_TEXT_ICON,
									0,0,textheight,textheight,1,last,object_id,"newimp",-1,menu,1, _("Impose"));
	type_mbut->pad = textheight/4;
	last = type_mbut;
	AddWin(type_mbut,1, type_mbut->win_w,0,50,50,0, type_mbut->win_w,0,0,50,0, wholelist.n-1);





	last=tbut=new Button(this,"ok",nullptr, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"cancel",nullptr, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

//	if (imposeout) {
//		AddNull();
//		LineInput *linp;
//		last=linp=new LineInput(nullptr,"imp",_("Impose..."),0,
//									  0,0,0,0,0,
//									  last,object_id,"out",
//									  _("Out:"),imposeout,0,
//									  0,0, 5,3, 5,3);
//		linp->tooltip(_("The file to output the imposed file to"));
//		//AddWin(linp,1, 50,0,2000,50,0, 50,0,50,50,0, -1);
//		AddWin(linp,1, 50,0,2000,50,0, linp->win_h,0,0,0,0, -1);
//	}




	Sync(1);	


	ChangeImposition(firstimp);
	
	double w = 1,h = 1;
	tool->GetDimensions(w,h);
	viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
	PostMessage(" ");

	if (show_splash) {
		// add a window that goes away with a click
		DBGE("*** IMPLEMENT ME!!!!")
	}

	return 0;
}

void ImpositionEditor::SetMenuButton(const char *text)
{
	type_mbut->Label(text);
	type_mbut->WrapToExtent(1);
	SquishyBox *box = findBox(type_mbut);
	box->pw(type_mbut->win_w);
	Sync();
}

//! Send the current imposition to win_owner.
void ImpositionEditor::send()
{
	Imposition *imp = nullptr;

	if (whichactive == WHICH_Signature) {
		if (tool_signatures) imp = (Imposition*)(tool_signatures->GetImposition());
		//if (tool) imp=(Imposition*)(tool->GetImposition()->duplicate());
		
	} else if (whichactive == WHICH_Net) {
		if (neteditor) imp = dynamic_cast<NetDialog*>(neteditor)->getNetImposition();

	} else if (whichactive == WHICH_Singles) {
		if (tool_singles) imp = (Imposition*)(tool_singles->GetImposition());
		// if (singleseditor) imp = dynamic_cast<SinglesEditor*>(singleseditor)->GetImposition();
	}

	if (!imp) return;

	RefCountedEventData *data = new RefCountedEventData(imp);
	data->info1 = rescale_pages;

	if (imposeout) {
		//for impose-only mode
		//if imposeformat==scribus, continue...
		Document *doc = laidout->project->docs.e[0]->doc;
		doc->ReImpose(imp,rescale_pages);
		exportImposedScribus(doc,imposeout);
	}

	//imp->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);
}

#define SETTINGS_Rescale  1

int ImpositionEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"ImpositionEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"ok")) {
		send();
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (!strcmp("cancel",mes)) {
		EventData *e=new EventData(LAX_onCancel);
		app->SendMessage(e, win_owner, win_sendthis, object_id);

		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (!strcmp("settings",mes)) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		if (s->info1==SETTINGS_Rescale) {
			rescale_pages=s->info2;
		}
		return 0;

	} else if (!strcmp("newimp",mes)) {
		DBG cerr <<"---newimp---"<<endl;

		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		 //when new imposition type selected from popup menu
		Imposition *newimp = nullptr;
		if (s->info2 == IMP_NEW_SINGLES) {


		} else if (s->info2 == IMP_NEW_SIGNATURE) {
			SignatureImposition *simp=new SignatureImposition;
			simp->AddStack(0,0,nullptr); //sigimp starts with null signatures
			newimp = simp;
			SetMenuButton(_("Signature"));

		} else if (s->info2 == IMP_NEW_NET) {
			newimp = new NetImposition;
			SetMenuButton(_("Net"));

		} else if (s->info2==IMP_FROM_FILE) {
			app->rundialog(new FileDialog(nullptr,nullptr,_("Imposition from file"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "impfile",
					FILES_OPEN_ONE
					));
			return 0;

		} else if (s->info2<0 || s->info2>=laidout->impositionpool.n) {
			return 0;

		} else { 
			DBG cerr <<"--- new imp from menu: "<<laidout->impositionpool.e[s->info2]->name<<endl;
			newimp = laidout->impositionpool.e[s->info2]->Create();
		} 

		if (newimp) {
			ChangeImposition(newimp);
			newimp->dec_count();
		}

		return 0;

	} else if (!strcmp(mes,"impfile")) {
		 //comes after a file select dialog for imposition file, from "From File..." imposition select
		const StrEventData *s = dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;
		Imposition *imp = impositionFromFile(s->str);
		ChangeImposition(imp);
		imp->dec_count();
		return 0;
	}

	return 1;
}

/*! Return 0 success, 1 failure, not changed.
 */
int ImpositionEditor::ChangeImposition(Imposition *newimp)
{
	//if necessary replace the viewport/tool with a separate window

	PostMessage("");

	if (dynamic_cast<SignatureImposition*>(newimp)) {
		whichactive = WHICH_Signature;

		if (!tool_signatures) {
			tool_signatures = new SignatureInterface(nullptr,1,viewport->dp, nullptr,nullptr /*paperstyle*/,doc);
		}
		if (tool != tool_signatures) {
			if (tool) tool->dec_count();
			tool = tool_signatures;
			tool->inc_count();
		}
		tool->UseThisImposition(newimp);
		AddTool(tool, 1, 0);

		// else activate viewport, assign new SignatureInterface
		WinFrameBox *box = dynamic_cast<WinFrameBox*>(wholelist.e[0]);
		if (box->win() != viewport) {
			app->mapwindow(viewport);
			// if (singleseditor) app->unmapwindow(singleseditor);
			if (neteditor) app->unmapwindow(neteditor);

			box->NewWindow(viewport);
			Sync(1);
		}
		if (tool_singles) RemoveTool(tool_singles->id);
		// if (tool_net) RemoveTool(tool_net->id);

		SetMenuButton(_("Signature"));
		return 0;

	} else if (dynamic_cast<Singles*>(newimp)) {
		whichactive = WHICH_Singles;

		if (!tool_singles) {
			tool_singles = new SinglesInterface(nullptr,1,viewport->dp);
		}
		if (tool != tool_singles) {
			if (tool) tool->dec_count();
			tool = tool_singles;
			tool->inc_count();
		}
		tool->UseThisImposition(newimp);
		AddTool(tool, 1, 0);

		WinFrameBox *box=dynamic_cast<WinFrameBox*>(wholelist.e[0]);
		if (box->win() != viewport) {
			app->mapwindow(viewport);
			if (neteditor) app->unmapwindow(neteditor);

			box->NewWindow(viewport);
			Sync(1);
		}
		if (tool_signatures) RemoveTool(tool_signatures->id);
		// if (tool_net) RemoveTool(tool_net->id);

		SetMenuButton(_("Singles"));
		return 0;


	} else if (dynamic_cast<NetImposition*>(newimp)) {
		whichactive = WHICH_Net;

		if (tool && !strcmp(tool->whattype(),"NetInterface")) {
			// -- if it ever gets implemented!!
			tool->UseThisImposition(newimp);
			return 0;
		}

		if (!neteditor) {
			neteditor = new NetDialog(this,"Net",_("Net"),
									object_id,"newnet",nullptr,
									dynamic_cast<NetImposition*>(newimp));
			app->addwindow(neteditor,0,0);
		} else {
			dynamic_cast<ImpositionWindow*>(neteditor)->UseThisImposition(newimp);
		}

		WinFrameBox *box = dynamic_cast<WinFrameBox*>(wholelist.e[0]);
		if (box->win() != neteditor) {
			app->unmapwindow(viewport);
			// if (singleseditor) app->unmapwindow(singleseditor);
			app->mapwindow(neteditor);

			box->NewWindow(neteditor);
			Sync(1);
		}

		SetMenuButton(_("Net"));
	}

	return 1;
}


//! Update imposition settings based on a changed imposition file
Imposition *ImpositionEditor::impositionFromFile(const char *file)
{
	DBG cerr<<"----------attempting to impositionFromFile()-------"<<endl;

	
	if (laidout_file_type(file,nullptr,nullptr,nullptr,"Imposition",nullptr)==0) {
		Attribute att;
		att.dump_in(file);

		const char *type=att.findValue("type");
		Imposition *imp=newImpositionByType(type);
		if (!imp) {
			DBG cerr<<"   impositionFromFile() -> unknown imposition type "<<(type?type:"unknown type")<<"..."<<endl;
			PostMessage(_("Could not open file."));
			return nullptr;
		}
		imp->dump_in_atts(&att,0,nullptr);
		return imp;
	}


	 //check if it's a polyhedron file, autogenerate a net from it...
	Polyptych::Polyhedron *poly = new Polyptych::Polyhedron();
	if (poly->dumpInFile(file,nullptr) == 0) {
		Polyptych::Net *net = new Polyptych::Net;
		net->basenet = poly;
		net->TotalUnwrap();
		NetImposition *nimp = new NetImposition();
		nimp->SetNet(net);

		DBG cerr<<"   installed polyhedron file..."<<endl;
		return nimp;

	} else {
		DBG cerr <<"...file does not appear to be a polyhedron file: "<<(file?file:"")<<endl;
		delete poly;
	}


	DBG cerr<<"   impositionFromFile() FAILED..."<<endl;
	return nullptr;
}


int ImpositionEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch == LAX_Esc) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

//	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
//		app->addwindow(new HelpWindow());
//		return 0;
	}
	return 1;
}


} // namespace Laidout

