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
// Copyright (C) 2013 by Tom Lechner
//

#include <lax/laxutils.h>
#include <lax/menubutton.h>
#include <lax/filedialog.h>

#include "../language.h"
#include "../utils.h"
#include "../filetypes/scribus.h"
#include "../viewwindow.h"
#include "../headwindow.h"
#include "impositioneditor.h"
#include "signatureinterface.h"
//#include "polyptych/src/poly.h"

#include "netimposition.h"
#include "netdialog.h"
#include "singleseditor.h"

// DBG !!!!!
#include <lax/displayer-cairo.h>

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {

//----------------------------- generic Imposition Editor creator -------------------------------

/*! Create the correct editor for type and/or imp. type overrides imp.
 */
Laxkit::anXWindow *newImpositionEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
						unsigned long nowner, const char *mes,
						PaperStyle *papertype, const char *type, Imposition *imp, const char *imposearg,
						Document *doc)
{
	if (!type && !imp) return NULL;

	anXWindow *win=NULL;

	int dec=0;
	if (!imp && type) {
		if (!strcmp(type,"NetImposition")) imp=new NetImposition;
		else if (!strcmp(type,"Singles")) imp=new Singles;
		else if (!strcmp(type,"SignatureImposition")) imp=new SignatureImposition;
		if (imp) dec=1;
	}

	ImpositionInterface *iface=NULL;
	if (imp) iface=imp->Interface();
	win=new ImpositionEditor(parnt,nname,ntitle,
						nowner,mes,
						doc,imp,NULL,
						imposearg,
						iface);

	if (dec) imp->dec_count();
	return win;
}





//----------------------------- ImpositionEditor -------------------------------

#define WHICH_Signature 1
#define WHICH_Net       2
#define WHICH_Singles   3

/*! \class ImpositionEditor
 * \brief A Laxkit::ViewerWindow that gets filled with stuff appropriate for signature editing.
 *
 * This creates the window with a SignatureInterface.
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
						const char *imposearg,
						ImpositionInterface *interface)
	: ViewerWindow(parnt,nname,ntitle,
				   ANXWIN_REMEMBER
					|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_NO_SCROLLERS|VIEWPORT_NO_RULERS, 
					0,0,500,500, 0, NULL)
{
	SetOwner(nowner,mes);
	doc=ndoc;

	rescale_pages=1;
	singleseditor=NULL;
	neteditor=NULL;
	tool=NULL;
	whichactive=WHICH_Signature;

	if (!viewport) {
		viewport=new ViewportWindow(this,"imposition-editor-viewport","imposition-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL);
		app->reparent(viewport,this);
		viewport->dec_count();
	}

	win_colors->bg=rgbcolor(200,200,200);
	viewport->dp->NewBG(200,200,200);

	//DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(viewport->dp);
	//DBG if (ddp->GetCairo()) cerr <<" ImpositionEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	needtodraw=1;
	tool=interface;

	firstimp=imposition;
	if (imposition) {
		firstimp->inc_count();

		if (tool) tool->UseThisImposition(imposition);
		else tool=imposition->Interface();
		if (tool && ndoc) tool->UseThisDocument(ndoc);
	}
	if (!tool) {
		tool=new SignatureInterface(NULL,1,viewport->dp, NULL,p,ndoc);
	}
	tool->GetShortcuts();
	if (imposearg) tool->ShowSplash(1);


	AddTool(tool,1,1); // local, and select it
	//DBG if (ddp->GetCairo()) cerr <<" ImpositionEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;
	

	if (imposition && dynamic_cast<NetImposition*>(imposition)) {
		if (!neteditor) {
			neteditor=new NetDialog(this,"Net",_("Net"),
									object_id,"newnet",NULL,
									dynamic_cast<NetImposition*>(imposition));
			app->addwindow(neteditor,0,1);
		}
		whichactive=WHICH_Net;

	} else if (imposition && dynamic_cast<Singles*>(imposition)) {
		if (!singleseditor) {
			singleseditor=new SinglesEditor(this,"Singles",_("Singles"),
									object_id,"newsingles",
								    doc,
								    dynamic_cast<Singles*>(imposition),
								    NULL); //paper
			app->addwindow(singleseditor,0,1);
		}
		whichactive=WHICH_Singles;
	}



	 //**** this is a hack! Should instead be parsed into an export config with extra fields for additional
	 // 		imposing
	imposeout=NULL;
	imposeformat=NULL;
	double ww=-1, hh=-1;

	if (imposearg) {
		//need to load a new document, which may be a non-laidout document.
		//If non-laidout, then create new singles, and import
		//add extra field for impose out

		//DBG const char *in="",*out="";

		Attribute att;
		NameValueToAttribute(&att,imposearg,'=',',');

		const char *prefer=NULL;
		const char *name,*value;

		for (int c=0; c<att.attributes.n; c++) {
			name =att.attributes.e[c]->name;
			value=att.attributes.e[c]->value;
			if (!strcmp(name,"in")) {
				//DBG in=value;
				int docindex=laidout->project->docs.n;

				if (isScribusFile(value)) {
					if (addScribusDocument(value)==0) {
						//yikes! crash magnet!
						tool->SetPaper(laidout->project->docs.e[0]->doc->imposition->papergroup->papers.e[0]->box->paperstyle);
						if (laidout->project->docs.n>docindex) {
							tool->UseThisDocument(laidout->project->docs.e[docindex]->doc);
						}
					}

				} else if (isPdfFile(value,NULL)) {
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
				//DBG out=value;

			} else if (!strcmp(name,"prefer")) {
				prefer=value;

			} else if (!strcmp(name,"width")) {
				DoubleAttribute(value,&ww,NULL);

			} else if (!strcmp(name,"height")) {
				DoubleAttribute(value,&hh,NULL);
			}
		}

		if (ww>0 && hh>0) tool->SetTotalDimensions(ww,hh);

		if (prefer) {
			int c;
			for (c=0; c<laidout->impositionpool.n; c++) {
				if (!strcasecmp(laidout->impositionpool.e[c]->name,prefer)) break;
			}
			if (c<laidout->impositionpool.n) {
				Imposition *imp=laidout->impositionpool.e[c]->Create();
				SignatureImposition *simp=dynamic_cast<SignatureImposition*>(imp);
				if (simp) {
					PaperStyle *p;
					p=(PaperStyle*)laidout->project->docs.e[0]->doc->imposition->papergroup->papers.e[0]->box->paperstyle->duplicate();
					//simp->SetPaper(paper);
					simp->SetPaperFromFinalSize(p->w(),p->h());
					tool->UseThisImposition(simp);
					simp->dec_count();
					p->dec_count();
				} else {
					delete imp;
				}
			}
		}

		//DBG cerr <<"Impose only from "<<in<<" to "<<out<<endl;
	}

	//DBG if (ddp->GetCairo()) cerr <<" ImpositionEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

}

ImpositionEditor::~ImpositionEditor()
{ 
	if (firstimp) firstimp->dec_count();
	if (imposeout) delete[] imposeout;
	if (imposeformat) delete[] imposeformat;
}

//! Passes off to SignatureInterface::dump_out().
void ImpositionEditor::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	// *** ((SignatureInterface *)curtool)->dump_out(f,indent,what,context);
	anXWindow::dump_out(f,indent,what,context);
}

//! Passes off to SignatureInterface::dump_in_atts().
void ImpositionEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	// *** ((SignatureInterface *)curtool)->dump_in_atts(att,flag,context);
	anXWindow::dump_in_atts(att,flag,context);
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

	anXWindow *last=NULL;
	Button *tbut;

	int textheight=app->defaultlaxfont->textheight();

	//AddNull();

	MenuInfo *menu=new MenuInfo();
	menu->AddSep(_("Change to:"));
	for (int c=0; c<laidout->impositionpool.n; c++) {
		menu->AddItem(laidout->impositionpool.e[c]->name,c);
	}

	 // *** these need to be all the imposition base creation types
	menu->AddSep();
	menu->AddItem(_("NEW Singles..."),   IMP_NEW_SINGLES);
	menu->AddItem(_("NEW Signature..."), IMP_NEW_SIGNATURE);
	menu->AddItem(_("NEW Net..."),       IMP_NEW_NET);
	menu->AddItem(_("From file..."),     IMP_FROM_FILE);

	MenuButton *mbut=new MenuButton(this,"imp",NULL,MENUBUTTON_DOWNARROW|MENUBUTTON_LEFT|MENUBUTTON_ICON_ONLY,
									0,0,textheight,textheight,1,last,object_id,"newimp",-1,menu,1);
	mbut->pad=textheight/4;
	last=mbut;
	AddWin(mbut,1, mbut->win_w,0,50,50,0, mbut->win_w,0,0,50,0, wholelist.n-1);





	last=tbut=new Button(this,"ok",NULL, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"cancel",NULL, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

//	if (imposeout) {
//		AddNull();
//		LineInput *linp;
//		last=linp=new LineInput(NULL,"imp",_("Impose..."),0,
//									  0,0,0,0,0,
//									  last,object_id,"out",
//									  _("Out:"),imposeout,0,
//									  0,0, 5,3, 5,3);
//		linp->tooltip(_("The file to output the imposed file to"));
//		//AddWin(linp,1, 50,0,2000,50,0, 50,0,50,50,0, -1);
//		AddWin(linp,1, 50,0,2000,50,0, linp->win_h,0,0,0,0, -1);
//	}




	Sync(1);	

	double w=1,h=1;
	tool->GetDimensions(w,h);
	viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
	viewport->postmessage(" ");

	if (whichactive!=WHICH_Signature) {
		ChangeImposition(firstimp);
	}

	//DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(viewport->dp);
	//DBG if (ddp->GetCairo()) cerr <<" ImpositionEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	return 0;
}

//! Send the current imposition to win_owner.
void ImpositionEditor::send()
{
	Imposition *imp=NULL;

	if (whichactive==WHICH_Signature) {
		if (tool) imp=(Imposition*)(tool->GetImposition());
		//if (tool) imp=(Imposition*)(tool->GetImposition()->duplicate());
	} else if (whichactive==WHICH_Net) {
		if (neteditor) imp=dynamic_cast<NetDialog*>(neteditor)->getNetImposition();
	} else if (whichactive==WHICH_Singles) {
		if (singleseditor) imp=dynamic_cast<SinglesEditor*>(singleseditor)->GetImposition();
	}

	if (!imp) return;

	RefCountedEventData *data=new RefCountedEventData(imp);
	data->info1=rescale_pages;

	if (imposeout) {
		//for impose-only mode
		//if imposeformat==scribus, continue...
		Document *doc=laidout->project->docs.e[0]->doc;
		doc->ReImpose(imp,rescale_pages);
		exportImposedScribus(doc,imposeout);
	}

	//imp->dec_count();

	app->SendMessage(data, win_owner, win_sendthis, object_id);
}

#define SETTINGS_Rescale  1

int ImpositionEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	//DBG cerr <<"ImpositionEditor got message: "<<(mes?mes:"?")<<endl;

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
		//DBG cerr <<"---newimp---"<<endl;

		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);

		 //when new imposition type selected from popup menu
		Imposition *newimp=NULL;
		if (s->info2==IMP_NEW_SIGNATURE) {
			SignatureImposition *simp=new SignatureImposition;
			simp->AddStack(0,0,NULL); //sigimp starts with null signatures
			newimp=simp;

		} else if (s->info2==IMP_NEW_NET) {
			newimp=new NetImposition;

		} else if (s->info2==IMP_FROM_FILE) {
			app->rundialog(new FileDialog(NULL,NULL,_("Imposition from file"),
					ANXWIN_REMEMBER, 0,0, 0,0,0,
					object_id, "impfile",
					FILES_OPEN_ONE
					));
			return 0;

		} else if (s->info2<0 || s->info2>=laidout->impositionpool.n) {
			return 0;

		} else { 
			//DBG cerr <<"--- new imp from menu: "<<laidout->impositionpool.e[s->info2]->name<<endl;
			newimp=laidout->impositionpool.e[s->info2]->Create();
		} 

		if (newimp) {
			ChangeImposition(newimp);
			newimp->dec_count();
		}

		return 0;

	} else if (!strcmp(mes,"impfile")) {
		 //comes after a file select dialog for imposition file, from "From File..." imposition select
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || isblank(s->str)) return 1;
		Imposition *imp=impositionFromFile(s->str);
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


	if (dynamic_cast<SignatureImposition*>(newimp)) {
		whichactive=WHICH_Signature;

		if (tool && !strcmp(tool->whattype(),"SignatureInterface")) {
			tool->UseThisImposition(newimp);
			//return 0;
		}

		 //else activate viewport, assign new SignatureInterface
		WinFrameBox *box=dynamic_cast<WinFrameBox*>(wholelist.e[0]);
		if (box->win()!=viewport) {
			app->mapwindow(viewport);
			if (singleseditor) app->unmapwindow(singleseditor);
			if (neteditor) app->unmapwindow(neteditor);

			box->NewWindow(viewport);
			Sync(1);
		}

		return 0;


	} else if (dynamic_cast<Singles*>(newimp)) {
		whichactive=WHICH_Singles;

		if (tool && !strcmp(tool->whattype(),"SinglesInterface")) {
			 // -- if it ever gets implemented!!
			tool->UseThisImposition(newimp);
			return 0;
		}

		if (!singleseditor) {
			singleseditor=new SinglesEditor(this,"Singles",_("Singles"),
									object_id,"newsingles",
								    doc,
								    dynamic_cast<Singles*>(newimp),
								    NULL); //paper
			app->addwindow(singleseditor,0,0);
		} else {
			dynamic_cast<ImpositionWindow*>(singleseditor)->UseThisImposition(newimp);
		}

		WinFrameBox *box=dynamic_cast<WinFrameBox*>(wholelist.e[0]);
		if (box->win()!=singleseditor) {
			app->unmapwindow(viewport);
			app->mapwindow(singleseditor);
			if (neteditor) app->unmapwindow(neteditor);

			box->NewWindow(singleseditor);
			Sync(1);
		}

		return 0;


	} else if (dynamic_cast<NetImposition*>(newimp)) {
		whichactive=WHICH_Net;

		if (tool && !strcmp(tool->whattype(),"NetInterface")) {
			 // -- if it ever gets implemented!!
			tool->UseThisImposition(newimp);
			return 0;
		}

		if (!neteditor) {
			neteditor=new NetDialog(this,"Net",_("Net"),
									object_id,"newnet",NULL,
									dynamic_cast<NetImposition*>(newimp));
			app->addwindow(neteditor,0,0);
		} else {
			dynamic_cast<ImpositionWindow*>(neteditor)->UseThisImposition(newimp);
		}

		WinFrameBox *box=dynamic_cast<WinFrameBox*>(wholelist.e[0]);
		if (box->win()!=neteditor) {
			app->unmapwindow(viewport);
			if (singleseditor) app->unmapwindow(singleseditor);
			app->mapwindow(neteditor);

			box->NewWindow(neteditor);
			Sync(1);
		}
	}

	return 1;
}


//! Update imposition settings based on a changed imposition file
Imposition *ImpositionEditor::impositionFromFile(const char *file)
{
	//DBG cerr<<"----------attempting to impositionFromFile()-------"<<endl;

	
	if (laidout_file_type(file,NULL,NULL,NULL,"Imposition",NULL)==0) {
		Attribute att;
		att.dump_in(file);

		const char *type=att.findValue("type");
		Imposition *imp=newImpositionByType(type);
		if (!imp) {
			//DBG cerr<<"   impositionFromFile() -> unknown imposition type "<<(type?type:"unknown type")<<"..."<<endl;
			return NULL;
		}
		imp->dump_in_atts(&att,0,NULL);
		return imp;
	}


	 //check if it's a polyhedron file, autogenerate a net from it...
	Polyptych::Polyhedron *poly=new Polyptych::Polyhedron();
	if (poly->dumpInFile(file,NULL)==0) {
		Polyptych::Net *net=new Polyptych::Net;
		net->basenet=poly;
		net->TotalUnwrap();
		NetImposition *nimp;
		nimp=new NetImposition();
		nimp->SetNet(net);

		//DBG cerr<<"   installed polyhedron file..."<<endl;
		return nimp;

	} else {
		//DBG cerr <<"...file does not appear to be a polyhedron file: "<<(file?file:"")<<endl;
		delete poly;
	}


	//DBG cerr<<"   impositionFromFile() FAILED..."<<endl;
	return NULL;
}


int ImpositionEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
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

