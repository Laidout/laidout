//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/********************* document.cc *************************/

#include <lax/strmanip.h>
#include <lax/lists.cc>
#include "document.h"
#include "saveppt.h"
#include "printing/psout.h"
#include "version.h"
#include <lax/attributes.h>
#include "laidout.h"


using namespace LaxFiles;

#ifndef HIDEGARBAGE
#include <iostream>
using namespace std;
#define DBG 
#else
#define DBG //
#endif


//---------------------------- DocumentStyle ---------------------------------------

/*! \class DocumentStyle
 * \brief Style for Document objects, suprisingly enough.
 *
 * Keeps a local imposition object.
 */
//class DocumentStyle : public Style
//{
// public:
//	Imposition *imposition;
//	DocumentStyle(Imposition *imp);
//	virtual ~DocumentStyle();
//	virtual Style *duplicate(Style *s=NULL);
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

//! Constructor, copies Imposition pointer, does not duplicate imp.
/*! ***should sanity check the save? *** who should be deleting the imp?
 * currently it is deleted in destructor.
 */
DocumentStyle::DocumentStyle(Imposition *imp)
	: Style(NULL,NULL, ("somedocstyle"))//***
{
	imposition=imp;
}

/*! Recognizes 'imposition'. Discards all else.
 */
void DocumentStyle::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"imposition")) {
			if (imposition) delete imposition;
			 // figure out which kind of imposition it is..
			imposition=newImposition(value);
			if (imposition) imposition->dump_in_atts(att->attributes.e[c]);
		} else { 
			DBG cout <<"DocumentStyle dump_in:*** unknown attribute!!"<<endl;
		}
	}
}

//! Write out saveas, then call imposition->dump(f,indent+2).
void DocumentStyle::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (imposition) {
		fprintf(f,"%simposition %s\n",spc,imposition->Stylename());
		imposition->dump_out(f,indent+2);
	}
}

//! *** returns a new DocumentStyle(NULL,NULL)
/*! Should make "$save.2"?? and duplicate the imposition. Each doc must
 * have own instance of Imposition since there are doc specific settings in imp?
 */
Style *DocumentStyle::duplicate(Style *s)//s=NULL
{
	if (s==NULL) s=new DocumentStyle(NULL);
	if (!dynamic_cast<DocumentStyle *>(s)) return NULL;
	DocumentStyle *ds=dynamic_cast<DocumentStyle *>(s);
	if (!ds) return NULL;
	ds->imposition=(Imposition *)imposition->duplicate();
	//ds->saveas?? untitled2 untitled.3  ds->saveas=getUniqueUntitled() ds->saveas=GetUniqueFileName(based on saveas)
	return s;
}

//! Deletes saveas and imposition. *** must work out who should be holding what!!!
DocumentStyle::~DocumentStyle()
{
	if (imposition) delete imposition;
}

//---------------------------- Document ---------------------------------------

/*! \class Document
 * \brief Holds individual documents.
 *
 * A Document in Laidout is a collection of pages that all go onto the same
 * type of paper. Thus, a book with a larger cover, for instance, is most likely
 * two documents: the body pages, and the cover page, which together might
 * constitute a Project.
 *
 * \todo figure out whats up with notesandscripts
 * 
 * \todo Do a scribus out/in, and a passepartout in.
 */
/*! \var int Document::curpage
 * \brief The index into pages of the current page.
 */
//class Document : public ObjectContainer, public LaxFiles::DumpUtility
//{
// public:
//	DocumentStyle *docstyle;
//	char *saveas;
//	
//	Laxkit::PtrStack<Page> pages;
//	int curpage;
//	int numn;
//	char **notesorscripts;
//	clock_t modtime;
//
//	Document(const char *filename);
//	Document(DocumentStyle *stuff=NULL,const char *filename=NULL);
//	virtual ~Document();
//	virtual const char *Name();
//	virtual int Name(const char *nname);
//	virtual void clear();
//
//	virtual Page *Curpage();
//	virtual int NewPages(int starting,int n);
//	virtual int RemovePages(int start,int n);
//	
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//	virtual int Load(const char *file);
//	virtual int Save(LaidoutSaveFormat format=Save_Normal);
//	
//	virtual int n() { return pages.n; }
//	virtual Laxkit::anObject *object_e(int i) 
//		{ if (i>=0 && i<pages.n) return (anObject *)(pages.e[i]); return NULL; }
////	int SaveAs(char *newfile,int format=1); // format: binary, xml, pdata
////	int New();
////	int Print();
//};

//! Constructor from a file.
/*! Loads the file.
 */
Document::Document(const char *filename)
{ 
	saveas=NULL;
	makestr(saveas,filename);
	modtime=times(NULL);
	docstyle=NULL;
	numn=0;
	notesorscripts=NULL;
	curpage=-1;
	
	Load(filename);
}

//! Constructor from a DocumentStyle
/*! Document takes the style. The calling code should not delete it.
 *
 * Here, the documents pages are created from the info in stuff (or the default
 * document style). Imposition::CreatePages() creates the pages, and they are
 * put in the pages stack.
 *
 * The filename is put in saveas, but the file is not loaded. Settings are
 * taken from stuff.
 */
Document::Document(DocumentStyle *stuff,const char *filename)//stuff=NULL
{ 
	modtime=times(NULL);
	numn=0;
	notesorscripts=NULL;
	curpage=-1;
	saveas=NULL;
	makestr(saveas,filename);
	
	docstyle=stuff;
	if (docstyle==NULL) {
		//*** need to create a new DocumentStyle
		//docstyle=Styles::newStyle("DocumentStyle"); //*** should grab default doc style?
		COUT("***need to implement get defualt document in Document constructor.."<<endl);
	}
	if (docstyle==NULL) {
		COUT("***need to implement document constructor.."<<endl);
	} else {
		 // create the pages
		if (docstyle->imposition) pages.e=docstyle->imposition->CreatePages();
		else { COUT("**** in new Document, docstyle has no imposition"<<endl);}
		if (pages.e) { // must manually count how many element in e, put that in n
			int c=0;
			while (pages.e[c]!=NULL) c++;
			pages.n=c;
			if (c) {
				pages.islocal=new char[c];
				for (c=0; c<pages.n; c++) pages.islocal[c]=1;
				curpage=0;
			}
		}
	}
}

Document::~Document()
{
	DBG cout <<" Document destructor.."<<endl;
	pages.flush();
	delete docstyle;
	if (saveas) delete[] saveas;
	//***delete notes
}

//! Remove everything from the document.
void Document::clear()
{
	pages.flush();
	if (docstyle) { delete docstyle; docstyle=NULL; }
	curpage=-1;

	//***delete notesandscripts
}

//! Add n new blank pages starting before page index starting, or at end if starting==-1.
/*! \todo **** this is rather broken, should migrate maintenance of margins and page width
 * and height to Imposition? The correct PageStyle is not being added here. And when pages
 * are inserted, the pagestyles of the following pages (that already existed) are possibly
 * out of sync with the correct pagestyles, since w(), h(), and margins are incorrect... must
 * update impositioninst.cc
 *
 * Returns number of pages added, or negative for error.
 */
int Document::NewPages(int starting,int np)
{
	if (np<=0) return 0;
	Page *p;
	if (starting<0) starting=pages.n;
	for (int c=0; c<np; c++) {
		p=new Page(NULL);
		pages.push(p,1,starting);
	}
	docstyle->imposition->NumPages(pages.n);
	docstyle->imposition->SyncPages(this,starting,-1);
	laidout->notifyDocTreeChanged();
	return np;
}

//! Remove pages [start,start+n-1].
/*! Return the number of pages removed, or negative for error.
 *
 * \todo *** this is slightly broken.. does not reorient pagestyles
 * properly.
 */
int Document::RemovePages(int start,int n)
{
	if (start>=pages.n) return -1;
	if (start+n>pages.n) n=pages.n-start;
	for (int c=0; c<n; c++) {
		pages.remove(start);
	}
	laidout->notifyDocTreeChanged();
	return n;
}

	
//! Return 0 if saved, return nonzero if not saved.
/*! format==Save_PPT does pptout(this) which is a Passepartout file.\n
 * format==Save_PS calls psout(this,NULL) which dumps out to 'output.ps'
 *
 * format==Save_Normal is the standard Laidout format.
 *
 * future formats would be: pdf, scribus?
 *  Save_EPS, Save_HTML, Save_SVG, Save_Scribus, .....
 * 
 * *** only checks for saveas existence, does no sanity checking on it...
 *
 * \todo 
 */
int Document::Save(LaidoutSaveFormat format)//format=Save_Normal
{
	if (format==Save_PPT) return pptout(this);
	if (format==Save_PS) return psout(this);
	if (format!=Save_Normal) {
		//anXApp::app->postmessage("That save format is not implemented.");
		DBG cout << "Format "<<format<<" is not implemented."<<endl;
		return 1;
	}
	
	FILE *f=NULL;
	if (!saveas || !strcmp(saveas,"")) {
		DBG cout <<"**** cannot save, saveas is null."<<endl;
		return 2;
	}
	f=fopen(saveas,"w");
	if (!f) {
		DBG cout <<"**** cannot save, file \""<<saveas<<"\" cannot be opened for writing."<<endl;
		return 3;
	}

	char *dir=get_current_dir_name();
	DBG cout <<"....Saving document to "<<saveas<<" in "<<dir<<endl;
	if (dir) free(dir);
//	f=stdout;//***
	fprintf(f,"#Laidout %s Document\n",LAIDOUT_VERSION);
	dump_out(f,0);
	fclose(f);
	return 0;
}

//! Load a document, completely replacing what's here already.
/*! This only clears the current variables when the file can be loaded (but
 * not necessarily read correctly).
 *
 * ***This should maybe be a standalone? or at least have a standalone
 * implemented somewhere?
 *
 * Return 0 for not loaded, positive for loaded.
 *
 * \todo *** for file, check that it is in fact a Laidout file! If it can
 * be interpreted as another importable file, then loading should be delegated
 * to some appropriate function....
 */
int Document::Load(const char *file)
{
	//*** need to create a new DocumentStyle from what's in the file..
	COUT("----Document::Load read file "<<(file?file:"**** AH! null file!")<<" into a new Document"<<endl);
	FILE *f=fopen(file,"r");
	//*** make sure it is a laidout document!!
	if (!f) {
		DBG cout <<"**** cannot load, "<<(file?file:"(nofile)")<<" cannot be opened for reading."<<endl;
		return 0;
	}
	clear();
	dump_in(f,0,NULL);
	fclose(f);
	
	makestr(saveas,file);
	if (!docstyle) docstyle=new DocumentStyle(NULL);
	if (!docstyle->imposition) docstyle->imposition=newImposition("Singles");
	if (pages.n==0) {
		pages.e=docstyle->imposition->CreatePages();
		if (pages.e) { // must manually count how many element in e, put that in n
			int c=0;
			while (pages.e[c]!=NULL) c++;
			pages.n=c;
			if (c) {
				pages.islocal=new char[c];
				for (c=0; c<pages.n; c++) pages.islocal[c]=1;
				curpage=0;
			}
		}
	}
	docstyle->imposition->NumPages(pages.n);
	docstyle->imposition->SyncPages(this,0,-1);
	DBG cout <<"------ Done reading "<<file<<endl<<endl;
	return 1;
}

//! Low level reading in a document.
/*! Please note that this deletes docstyle if nonnull.
 * If docstyle required special treatment, it should have been dealt with
 * previous to coming here.
 *
 * Recognizes 'docstyle' and 'page'. Discards all else. ***perhaps the notesandscripts
 * should be all the excess attributes?
 */
void Document::dump_in_atts(LaxFiles::Attribute *att)
{
	if (!att) return;
	Page *page;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"saveas")) {
			makestr(saveas,value);
		} else if (!strcmp(name,"docstyle")) {
			if (docstyle) delete docstyle;
			docstyle=new DocumentStyle(NULL);
			docstyle->dump_in_atts(att->attributes.e[c]);
		} else if (!strcmp(name,"page")) {
			PageStyle *ps=NULL;
			if (docstyle && docstyle->imposition) ps=docstyle->imposition->GetPageStyle(pages.n);
			page=new Page();
			page->layers.flush();
			page->dump_in_atts(att->attributes.e[c]);
			pages.push(page,1);
		}
	}
}

//! Dumps docstyle, pages, and notes(well not notes yet..).
void Document::dump_out(FILE *f,int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	 //*** shouldn't have this? it is just the filename, file knows that already
	if (saveas) fprintf(f,"%ssaveas %s\n",spc,saveas);
	 // dump docstyle
	if (docstyle) {
		fprintf(f,"%sdocstyle\n",spc);
		docstyle->dump_out(f,indent+2);
	}
	 // dump objects
	for (int c=0; c<pages.n; c++) {
		fprintf(f,"%spage %d\n",spc,c);
		pages.e[c]->dump_out(f,indent+2);
	}
	 // dump notes/meta data
	//***
}

//! Rename the saveas part of the document. Return 1 for success, 0 for fail.
int Document::Name(const char *nname)
{
	if (!nname || !nname[0]) return 0;
	makestr(saveas,nname);
	return 1;
}

//! Returns saveas if it exists, else "untitled"
const char *Document::Name()
{
	const char *nm=saveas;
	if (!nm || nm && nm[0]=='\0') nm="untitled";
	return nm;
}

//! Return the internal Page object corresponding to curpage.
/*! if curpage<0 then select the first page.
 */
Page *Document::Curpage()
{
	if (!docstyle || pages.n==0) return NULL;
	if (curpage==-1) curpage=0;
	return pages.e[curpage];
}

//---------------------------- Project ---------------------------------------
/*! \class Project
 * \brief Class holding several documents, as well as various other settings.
 *
 * When laidout is opened and a new document is started, everything goes into
 * the default Project. The project maintains the scratchboard, any project notes,
 * the documents of the project, default directories, and other little tidbits
 * the user might care to associate with the project.
 *
 * Also, the Project maintains its own StyleManager***??????
 */
//class Project
//{
// public:
////	StyleManager styles;
//	Laxkit::PtrStack<Document> docs;
//	Page scratchboard;
//	Laxkit::PtrStack<char> project_notes;
//
//	Project();
//	virtual ~Project();
//};

Project::Project() : project_notes(1)
{}

Project::~Project()
{
	docs.flush();
}
