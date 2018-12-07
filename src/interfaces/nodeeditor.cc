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
// Copyright (C) 2018 by Tom Lechner
//

#include <lax/laxutils.h>
#include <lax/menubutton.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>

#include "../language.h"
#include "../utils.h"
#include "../viewwindow.h"
#include "../headwindow.h"
#include "../settingswindow.h"


#include "nodeeditor.h"



// DBG !!!!!
#include <lax/displayer-cairo.h>


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


namespace Laidout {


//----------------------------- Stand alone node editor -------------------------------


/*! Create the correct editor for type and/or imp. type overrides imp.
 *
 * editor_arg: "pipeout, pipein, pass_nodes_only"
 */
Laxkit::anXWindow *newNodeEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,
								 unsigned long nowner, const char *mes,
								 NodeGroup *nnodes,int absorb,
								 const char *editor_arg
								)
{
	anXWindow *win=NULL;

	win = new NodeEditor(parnt,nname,ntitle, nowner,mes,
						 nnodes,absorb,
						 editor_arg,
						 NULL
						);
	return win;
}



//----------------------------- NodeEditor


/*! \class NodeEditor
 * A stand alone Laxkit::ViewerWindow that gets filled with stuff appropriate for node editing.
 * Basically a bare bones window with a NodeInterface.
 *
 */


//! Make the window using project.
/*! Inc count of ndoc.
 */
NodeEditor::NodeEditor(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle, unsigned long nowner, const char *mes,
						NodeGroup *nnodes,int absorb,
						const char *editor_arg,
						NodeInterface *interface)
	: ViewerWindow(parnt,nname,ntitle,
				   ANXWIN_REMEMBER
					|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_NO_SCROLLERS|VIEWPORT_NO_RULERS, 
					0,0,500,500, 0, NULL)
{
	SetOwner(nowner,mes);

	tool=NULL;

	if (!viewport) {
		viewport=new ViewportWindow(this,"node-editor-viewport","node-editor-viewport",
									ANXWIN_HOVER_FOCUS|VIEWPORT_RIGHT_HANDED|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
									0,0,0,0,0,NULL);
		app->reparent(viewport,this);
		viewport->dec_count();
	}

	WindowStyle *style = win_themestyle->duplicate();
	InstallColors(style);
    win_themestyle->bg.rgbf(200/255.,200/255.,200/255.);
	viewport->dp->NewBG(200,200,200);


	DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(viewport->dp);
	DBG if (ddp->GetCairo()) cerr <<" NodeEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;


	needtodraw=1;
	tool = interface;

	if (!tool) {
		tool = new NodeInterface(NULL,1,viewport->dp);
		if (nnodes) {
			tool->UseThis(nnodes);
			if (absorb) nnodes->dec_count();
		}
		
	}
	tool->GetShortcuts();
	//if (editor_arg) tool->ShowSplash(1);


	AddTool(tool,1,1); // local, and select it
	DBG if (ddp->GetCairo()) cerr <<" NodeEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;





	pipeout = false;
	fileout = NULL;
	outformat = NULL;
	char *indata = NULL;
	int in_n = 0;


	DBG cerr <<"parse editor_arg in NodeEditor()..."<<endl;
	if (editor_arg) {
		//need to load a new document, which may be a non-laidout document.
		//If non-laidout, then create new singles, and import
		//add extra field for impose out

		const char *filein=NULL;

		Attribute att;
		NameValueToAttribute(&att,editor_arg,'=',',');

		const char *name,*value;

		//DBG ofstream of;
		//DBG of.open("PIPEDIN.svg");
		//DBG char buffer[1000];
		//DBG of << getcwd(buffer,1000) <<endl;

		for (int c=0; c<att.attributes.n; c++) {
			name =att.attributes.e[c]->name;
			value=att.attributes.e[c]->value;

			if (!strcmp(name,"in")) {
				filein = value;

			} else if (!strcmp(name,"out")) {
				makestr(fileout,value);

			} else if (!strcmp(name,"format")) {
				makestr(outformat, value);

			} else if (!strcmp(name,"pipein")) {
				if (indata) { delete[] indata; indata = NULL; in_n = 0; }

				//DBG of << "piping..."<<endl;

				indata = pipe_in_whole_file(stdin, &in_n);
				//indata = read_in_whole_file("svg-filter-test.svg", &in_n, 0);
				
				//DBG app->addwindow(new MessageBar(NULL, "mes","mes",0, 100,100,200,50,0, indata));

				//DBG of << "---------Pipedin for nodes: "<<endl;
				//DBG of << (indata? indata : "") <<endl;
				//DBG of << "---------end Pipedin for nodes "<<endl;

			} else if (!strcmp(name,"pipeout")) {
				pipeout = true;
			}
		}

		//DBG of.close();

		if (indata) {
			tool->LoadNodes(indata, false, in_n, true);
			delete[] indata;

		} else if (filein) {
			tool->LoadNodes(filein, false, 0, true);
		}


		DBG cerr <<"Nodes-only from "<<(filein ? filein : "data:\n")<<(indata ? indata : "")<<" to "<< (outformat ? outformat : "?") <<endl;
	}

	DBG if (ddp->GetCairo()) cerr <<" NodeEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

}

NodeEditor::~NodeEditor()
{ 
}

void NodeEditor::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	anXWindow::dump_out(f,indent,what,context);
}

void NodeEditor::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);
}


/*! Removes rulers and adds Apply, Reset, and Update Thumbs.
 */
int NodeEditor::init()
{
	ViewerWindow::init();
	viewport->dp->NewBG(200,200,200);

	anXWindow *last=NULL;
	Button *tbut;

	//int textheight = app->defaultlaxfont->textheight();

	//AddNull();



	last=tbut=new Button(this,"ok",NULL, 0, 0,0,0,0,1, last,object_id,"ok",0,_("Ok"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

	last=tbut=new Button(this,"cancel",NULL, 0, 0,0,0,0,1, last,object_id,"cancel",0,_("Cancel"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, tbut->win_h,0,50,50,0, -1);

//	if (fileout) {
//		AddNull();
//		LineInput *linp;
//		last=linp=new LineInput(NULL,"imp",_("Out..."),0,
//									  0,0,0,0,0,
//									  last,object_id,"out",
//									  _("Out:"),fileout,0,
//									  0,0, 5,3, 5,3);
//		linp->tooltip(_("What to output to on exit"));
//		//AddWin(linp,1, 50,0,2000,50,0, 50,0,50,50,0, -1);
//		AddWin(linp,1, 50,0,2000,50,0, linp->win_h,0,0,0,0, -1);
//	}




	Sync(1);	

	double w=1,h=1;
	viewport->dp->Center(-w*.15,w*1.15, -h*.15,h*1.15);
	viewport->postmessage(" ");


	DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(viewport->dp);
	DBG if (ddp->GetCairo()) cerr <<" NodeEditor initialized, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	return 0;
}

//! Send the current imposition to win_owner.
void NodeEditor::send()
{
	if (fileout) {

	} else if (pipeout) {
		NodeGroup *nodes = tool->GetCurrent();
		
		if (!outformat || !strcasecmp(outformat, "default") || !strcasecmp(outformat, "laidout")) {
			if (nodes) nodes->dump_out(stdout, 0, 0, NULL);

		} else {
			tool->ExportNodes(NULL, outformat);
		}
	}

	//if (win_owner) {
	//	data = ***the nodes;
	//	app->SendMessage(data, win_owner, win_sendthis, object_id);
	//}
}


int NodeEditor::Event(const Laxkit::EventData *data,const char *mes)
{
	DBG cerr <<"NodeEditor got message: "<<(mes?mes:"?")<<endl;

	if (!strcmp(mes,"ok")) {
		send();
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (!strcmp("cancel",mes)) {
		EventData *e = new EventData(LAX_onCancel);
		app->SendMessage(e, win_owner, win_sendthis, object_id);

		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

//	} else if (!strcmp("settings",mes)) {
//		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
//		if (s->info1 == SETTINGS_Rescale) {
//			rescale_pages=s->info2;
//		}
//		return 0;


	}

	return 1;
}



int NodeEditor::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Esc && (win_style & ANXWIN_ESCAPABLE)) {
		if (win_parent) ((HeadWindow *)win_parent)->WindowGone(this);
		app->destroywindow(this);
		return 0;

	} else if (ch==LAX_F1 && (state&LAX_STATE_MASK)==0) {
        app->addwindow(newSettingsWindow("keys", CurrentTool() ? CurrentTool()->whattype() : "NodeInterface"));

		return 0;
	}

	return 1;
}


} // namespace Laidout

