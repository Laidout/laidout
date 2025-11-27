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
// Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/multilineedit.h>
#include <lax/messagebar.h>
#include <lax/menubutton.h>
#include <lax/popupmenu.h>
#include <lax/filedialog.h>
#include <lax/messagebox.h>
#include <lax/interfaces/interfacemanager.h>

#include "plaintextwindow.h"
#include "../language.h"
#include "../laidout.h"

#include <lax/debug.h>


using namespace Laxkit;
using namespace LaxInterfaces;


namespace Laidout {


#define TEXT_Select_Temp       -1
#define TEXT_Add_New           -2
#define TEXT_Delete_Current    -3
#define TEXT_Save_With_Project -4
#define TEXT_Save_Internally   -5
#define TEXT_Save_In_File      -6
#define TEXT_Duplicate         -7


//------------------------------ PlainTextWindow -------------------------------
/*! \class PlainTextWindow
 * \brief Editor for plain text
 *
 * <pre>
 *  [  the edit box       ]
 *  [owner/filename][apply][name [v]] <-- click name to retrieve other plain text objects
 *  [run (internally)][run with... (externally)]
 * </pre>
 */


/*! Increments the count of newtext.
 */
PlainTextWindow::PlainTextWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
 		int xx,int yy,int ww,int hh,int brder,
		PlainText *newtext)
	: RowFrame(parnt,nname,ntitle,ANXWIN_REMEMBER|ROWFRAME_ROWS|nstyle, xx,yy,ww,hh,brder, NULL,None,NULL)
{
	textobj=newtext;
	if (textobj) textobj->inc_count();
	else {
		textobj=new PlainText();
	}
}

PlainTextWindow::~PlainTextWindow()
{
	if (textobj) textobj->dec_count();
}

//! Update window controls to reflect current state of textobj.
void PlainTextWindow::updateControls()
{
	LineInput *inp = dynamic_cast<LineInput *>(findChildWindowByName("nameinput"));
	if (inp) {
		const char *str = (textobj 
							? (textobj->IsTemporary() ? _("(temporary)") : textobj->Id())
							: NULL);
		inp->SetText(str);
	}
}

//! Make obj->name be a new name not found in the project.
void PlainTextWindow::uniqueName(PlainText *obj)
{
	if (obj) PlainText::uniqueName(obj);
	//if (isblank(obj->Id())) obj->Id(_("Text object"));

	//InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	//ResourceManager *rm = imanager->GetResourceManager();
	//ResourceType *objs = rm->FindType("PlainText");
	//if (!objs) return; //no PlainText resources yet, so name definitely unique
	//objs->MakeNameUnique(obj->object_idstr);
}

int PlainTextWindow::Event(const Laxkit::EventData *data,const char *mes)
{
	//DBG cerr <<"plaintext message: "<<(mes?mes:"(some event)")<<endl;

	if (!strcmp(mes,"openPopup")) {
		// returned from file open dialog
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || !s->str) return 1;

		 //remove old text object, and install new one
		PlainText *newobj = new PlainText();
		newobj->Id("PlainText");
		if (newobj->LoadFromFile(s->str)==0) {
			newobj->texttype = TEXT_Plain;
			uniqueName(newobj);
			UseThis(newobj);

			InterfaceManager *imanager = InterfaceManager::GetDefault(true);
			ResourceManager *rm = imanager->GetResourceManager();
			rm->AddResource("PlainText", newobj, nullptr, newobj->Id(), nullptr, nullptr, s->str, nullptr);
		} // else failed to load, do not replace text object
		newobj->dec_count(); //remove excess count

		return 0;

	} else if (!strcmp(mes,"saveAsPopup")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s || !s->str) return 1;
	
		makestr(textobj->filename,s->str);
		textobj->SaveText();// ***if save error, should notify something

		if (isblank(textobj->Id())) uniqueName(textobj);

		return 0;

	} else if (!strcmp(mes,"open")) {
		FileDialog *fd=new FileDialog(NULL,NULL,_("Open text file..."),
					ANXWIN_REMEMBER|FILES_FILES_ONLY|FILES_OPEN_ONE|FILES_PREVIEW,
					0,0,0,0,0, object_id,"openPopup",
					0,NULL);
		fd->OkButton(_("Open as text"),NULL);
		app->rundialog(fd);
		return 0;

	} else if (!strcmp(mes,"whichtext")) { 
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		int i=s->info2;
		if (i<0) {
			if (i==TEXT_Select_Temp) {
				 //now you can't really select this any more
				return 0;

			} else if (i==TEXT_Save_With_Project) {
				 //Change the save location to save with project.
				 //If it is file text, then the project will save the file location, not the contents
				if (dynamic_cast<Resource*>(textobj->ResourceOwner())) return 0; //already a resource

				if (textobj->IsTemporary()) textobj->texttype = TEXT_Plain;
				if (isblank(textobj->Id())) uniqueName(textobj);

				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				ResourceManager *rm = imanager->GetResourceManager();
				rm->AddResource("PlainText", textobj, nullptr, textobj->Id(), nullptr, nullptr, nullptr, nullptr);
				updateControls();
				return 0;

			} else if (i==TEXT_Save_Internally) {
				 //if was saving as a file, remove the filename and make it save with the project/document
				if (!textobj->filename) return 0;
				makestr(textobj->filename,NULL);
				updateControls();
				return 0;

			} else if (i==TEXT_Save_In_File) {
				 //Change the save location to save in a file.
				if (textobj->filename) return 0;
				syncText(0);
				callSaveAs();
				return 0;

			} else if (i==TEXT_Duplicate) {
				if (!textobj) return 0;
				PlainText *obj = dynamic_cast<PlainText*>(textobj->duplicate());
				
				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				ResourceManager *rm = imanager->GetResourceManager();
				rm->AddResource("PlainText", obj, nullptr, obj->Id(), nullptr, nullptr, nullptr, nullptr);

				UseThis(obj);
				obj->dec_count();
				return 0;

			} else if (i == TEXT_Add_New) {
				// Create a new blank text object, push onto project, 
				// and make it the current one in editor
				PlainText *obj = new PlainText();
				obj->texttype = TEXT_Plain;

				// figure out a unique name
				uniqueName(obj);

				// push object onto project
				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				ResourceManager *rm = imanager->GetResourceManager();
				rm->AddResource("PlainText", obj, nullptr, obj->Id(), nullptr, nullptr, nullptr, nullptr);
				UseThis(obj);
				obj->dec_count();
				return 0;

			} else if (i==TEXT_Delete_Current) {
				 //if in project, remove from project
				if (!textobj) return 0;
				if (textobj->IsTemporary()) return 0;

				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				ResourceManager *rm = imanager->GetResourceManager();
				rm->RemoveResource(textobj, "PlainText");

				UseThis(nullptr);
				return 0;

			}
		} else {
			InterfaceManager *imanager = InterfaceManager::GetDefault(true);
			ResourceManager *rm = imanager->GetResourceManager();
			Resource *res = rm->FindResourceFromRID(i, "PlainText");
			if (res) {
				PlainText *obj = dynamic_cast<PlainText*>(res->GetObject());
				if (obj) UseThis(obj);
				else {
					DBGW("can't get PlainText resource object");
				}
			}
		}
		return 0;

	} else if (!strcmp(mes,"whichtextbutton")) { 
		MenuInfo *menu;
		menu = new MenuInfo("Text Objects");

		 //---add textobject list, info2 in return event is Resource object_id
		InterfaceManager *imanager = InterfaceManager::GetDefault(true);
		ResourceManager *rm = imanager->GetResourceManager();
		menu->AddSep(_("Project texts"));
		rm->ResourceMenu("PlainText", false, menu, 0, 0, textobj);

		int currentobj=-1;
		int isprojects=0;
		if (textobj && rm->FindResource(textobj, "PlainText")) isprojects = 1;
		
		// //try to center on current object
		// MenuItem *mitem = nullptr;
		// if (textobj) mitem = menu->findid(textobj->object_id);
		// **** if nested, straight index doesn't mean much


		 //------- Content save location
		menu->AddSep(_("Content save location"));
		int isinternal=-1;
		if (textobj) { if (textobj->Filename()) isinternal=0; else isinternal=1; } 

		 //temporary indicator only if text IS temporary..
		//if (!isprojects) menu->AddItem(_("(Temporary)"),TEXT_Select_Temp,LAX_ISTOGGLE|LAX_OFF|(isprojects?0:LAX_CHECKED));
		if (!isprojects) menu->AddToggleItem(_("Save with project"),TEXT_Save_With_Project, 0,false);
		if (isinternal)  menu->AddToggleItem(_("Save in text file..."),   TEXT_Save_In_File,0,false);
		else {
			char *str = new char[strlen(_("Save in file: "))+strlen(textobj->filename)+5];
			sprintf(str,"%s %s",_("Save in file: "), textobj->filename);
			menu->AddToggleItem(str,   TEXT_Save_In_File, 0, true);
			menu->AddToggleItem(_("Save within project file"), TEXT_Save_Internally, 0, false);
		}

		if (currentobj>=0) menu->menuitems.e[currentobj]->state |= LAX_CHECKED;

		 //-----further text object operations
		//if (laidout->interpreters.n) {
		//  int interpreterid=(textobject&&textobject->texttype==TEXT_Script?textobjet->textsubtype:-1);
		//	menu->AddItem(_("Assign interpreter"), TEXT_Assign_Interpreter);
		//	menu->SubMenu(_("Interpreters"));
		//	menu->AddItem(_("None, not a script"),4999,LAX_ISTOGGLE|LAX_OFF|(interpreterid<0?LAX_CHECKED:0));
		//	for (int c=0; c<laidout->interpreters.n)
		//		menu->AddItem(laidout->interpreter.e[c]->Name(),5000+c,
		//			LAX_ISTOGGLE|LAX_OFF|(interpreterid==laidout->interpreters.e[c]->id?LAX_CHECKED:0));
		//	menu->EndSubMenu();
		//}
		menu->AddSep();
		menu->AddItem(_("New text object"), TEXT_Add_New);
		if (isprojects) {
			menu->AddItem(_("Duplicate current"), TEXT_Duplicate);
			menu->AddItem(_("Remove current"), TEXT_Delete_Current);
		}


		 //create the actual popup menu...
		PopupMenu *popup=new PopupMenu(NULL,_("Documents"), 0,
                        0,0,0,0, 1,
                        object_id,"whichtext",
                        0, //mouse to position near?
                        menu,1, NULL,
                        TREESEL_LEFT
						|TREESEL_LIVE_SEARCH
						|TREESEL_SUB_ON_RIGHT
						|TREESEL_ZERO_OR_ONE
					);

		popup->Select(0);
		popup->WrapToMouse(0,NULL);
		app->rundialog(popup);
		return 0;

	} else if (!strcmp(mes,"save")) { 
		//if (isblank(textobj->filename)) { syncText(0); callSaveAs(); }
		//else syncText(1);
		syncText(1);
		return 0; 

	} else if (!strcmp(mes,"apply")) { 
		syncText(0);
		return 0; 

	} else if (!strcmp(mes,"run")) { 
		//if (!textobj) return 0;
		const char *input=NULL;
		MultiLineEdit *edit=dynamic_cast<MultiLineEdit *>(findChildWindowByName("plain-text-edit"));
		if (edit) {
			input=edit->GetCText();
		}
		if (!input) return 0;
		char *output = laidout->calculator->In(input, NULL);
		DBG if (!output) std::cerr  << "script in: "<<input<<std::endl<< "script out: (none)" <<std::endl;
		if (output) {
			DBG std::cerr << "script in: "<<input<<std::endl<< "script out" << output<<std::endl;
			//prependstr(output,":\n");
			//prependstr(output,_("Script output"));
			MessageBox *mbox=new MessageBox(NULL,NULL,_("Script output"),ANXWIN_CENTER|MB_LEFT, 0,0,0,0,0,
										NULL,0,NULL, _("Script output"));

			MultiLineEdit *outedit = new MultiLineEdit(this,"out-text",NULL,0, 0,0,0,0,1, NULL,0,NULL,
							  0,output);
			double th = app->defaultlaxfont->textheight();
			mbox->AddWin(outedit,1, 200,0,2000, 50,50, 1.5*th,0,1000,50,50, -1);
			mbox->AddNull();

			mbox->AddButton(BUTTON_OK);
			mbox->AddButton(_("Dammit!"),0);
			app->rundialog(mbox);
			delete[] output;
		}
		return 0;

	} else if (!strcmp(mes,"nameinput")) { 
		const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(data);
		DBG std::cerr<<"plaintextwindow nameinput update "<<s->info1<<std::endl;

		if (!textobj) return 0;

		int i = s->info1;
		if (i == 0 || i==1 || i==3) { //focus left or enter pressed for nameinput
			LineInput *inp = dynamic_cast<LineInput *>(findChildWindowByName("nameinput"));
			if (!inp) return 0;
			const char *txt = inp->GetCText();
			textobj->Id(txt);
			Resource *r = dynamic_cast<Resource*>(textobj->ResourceOwner());
			if (r && !isblank(txt)) makestr(r->name, txt);
		}
	}
	return 1;
}

void PlainTextWindow::callSaveAs()
{
	app->rundialog(new FileDialog(NULL,"Save As...",_("Save As..."),
				ANXWIN_REMEMBER,
				0,0,0,0,0, object_id,"saveAsPopup",
				FILES_FILES_ONLY|FILES_SAVE_AS,
				textobj->filename));
}

//! Syncronize the text object with the text in the editor.
/*! This should update any objects that depend on the current\n"
 *  text object, if any.
 *
 *  If filetoo, then also save to the file, if the text object has a FileRef.
 *
 *  Return 0 for success, nonzero for error.
 */
int PlainTextWindow::syncText(int filetoo)
{
	if (!textobj) return 1;
	MultiLineEdit *edit=dynamic_cast<MultiLineEdit *>(findChildWindowByName("plain-text-edit"));
	if (!edit) return 1;

	textobj->SetText(edit->GetCText());
	if (filetoo) if (textobj->SaveText()!=0) callSaveAs();
	updateControls();
	return 0;
}

int PlainTextWindow::init()
{
	anXWindow *last=NULL;

	MultiLineEdit *editbox;
	last = editbox = new MultiLineEdit(this,"plain-text-edit",NULL,0, 0,0,0,0,1, NULL,object_id,"ptedit",
							  0, textobj ? textobj->thetext : NULL);
    LaxFont *font = app->fontmanager->MakeFontFromStr("mono",getUniqueNumber());
    editbox->UseThisFont(font);
	
	AddWin(editbox,1, 100,95,2000,50,0, 100,95,20000,50,0, -1);
	AddNull();


	//-----------textobject name edit
	LineInput *nameinput=NULL;
	const char *str=(textobj?(textobj->IsTemporary() ? _("(temporary)"):textobj->Id()):NULL);
	last=nameinput=new LineInput(this,"nameinput",NULL,
						LINP_SEND_ANY, 0,0,0,0,0, 
						last,object_id,"nameinput",
						_("Name:"),str,0);
	nameinput->GetLineEdit()->win_style |= LINEEDIT_SEND_FOCUS_OFF;
	AddWin(nameinput,1, 200,100,1000,50,0, nameinput->win_h,0,0,50,0, -1);


	 //------select text object
	Button *ibut=NULL;
	last=ibut=new Button(this,"whichtext",NULL,0, 0,0,0,0,1, NULL,object_id,"whichtextbutton",-1,
						 "v");
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);


	 //--------open
	last=ibut=new Button(this,"open",NULL,IBUT_ICON_ONLY|IBUT_FLAT, 0,0,0,0,0, last,object_id,"open",-1,
						 _("Open"),NULL,laidout->icons->GetIcon("Open"));
	ibut->tooltip(_("Open a file from disk"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	 //--------save
	last=ibut=new Button(this,"save",NULL,IBUT_ICON_ONLY|IBUT_FLAT, 0,0,0,0,0, last,object_id,"save",-1,
						 _("Save"),NULL,laidout->icons->GetIcon("Save"));
	ibut->tooltip(_("Save the current text"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	 //--------apply
	last=ibut=new Button(this,"apply",NULL,IBUT_ICON_ONLY|IBUT_FLAT, 0,0,0,0,0, last,object_id,"apply",-1,
						 _("Apply"),NULL,laidout->icons->GetIcon("ApplyText"));
	ibut->tooltip(_("Syncronize the text object with the text in the editor\n"
				    "This should update any objects that depend on the current\n"
					"text object, if any"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	 //--------run
	last=ibut=new Button(this,"Run",NULL,IBUT_ICON_ONLY|IBUT_FLAT, 0,0,0,0,0, last,object_id,"run",-1,
						 _("Run"),NULL,laidout->icons->GetIcon("Run"));
	ibut->tooltip(_("Run this text as a script"));
	AddWin(ibut,1, ibut->win_w,0,50,50,0, ibut->win_h,0,50,50,0, -1);

	Sync(1);
	return 0;
}

/*! Return 0 for success, nonzero for error.
 *
 * Increments the count of txt.
 */
int PlainTextWindow::UseThis(PlainText *txt)
{
	if (txt==textobj) return 0;
	if (textobj) textobj->dec_count();
	textobj=txt;
	if (textobj) textobj->inc_count();
	if (!textobj) textobj=new PlainText();

	MultiLineEdit *edit = dynamic_cast<MultiLineEdit *>(findChildWindowByName("plain-text-edit"));
	if (edit) {
		edit->SetText(textobj->GetText());
	}

	updateControls();
	
	return 0;
}

void PlainTextWindow::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
    anXWindow::dump_out(f,indent,what,context);
}

Laxkit::Attribute *PlainTextWindow::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (textobj) {
		if (!att) att = new Attribute();
		att->push("textobject", textobj->Id());
	}
    return anXWindow::dump_out_atts(att,what,context);
}

void PlainTextWindow::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
    anXWindow::dump_in_atts(att,flag,context);

	char *nme,*value;

    for (int c=0; c<att->attributes.n; c++)  {
        nme  =att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(nme,"textobject") && value) {
        	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
			ResourceManager *rm = imanager->GetResourceManager();
			PlainText *tobj = dynamic_cast<PlainText*>(rm->FindResource(value, "PlainText"));

			if (tobj) UseThis(tobj);

        } else if (!strcmp(nme,"filename")) {
            if (isblank(value)) continue;
			PlainText *tobj=new PlainText;
			if (tobj->LoadFromFile(value)!=0) {
				tobj->dec_count();
				tobj=NULL;
			}
			if (tobj) {
				UseThis(tobj);
				tobj->dec_count();
			}

        } else if (!strcmp(nme,"show_line_numbers")) {
			MultiLineEdit *edit = dynamic_cast<MultiLineEdit*>(findChildWindowByName("plain-text-edit"));
			if (edit) {
				edit->SetStyle(TEXT_LINE_NUMBERS, BooleanAttribute(value));
			}
        }
    }
}


} //namespace Laidout

