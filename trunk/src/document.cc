//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
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
#include <lax/fileutils.h>
#include "laidout.h"
#include "headwindow.h"


using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 


//---------------------------- DocumentStyle ---------------------------------------

/*! \class DocumentStyle
 * \brief Style for Document objects, suprisingly enough.
 *
 * Should have enough info to create a specific kind of Document...
 * ----------
 * Keeps a local imposition object.
 */
//class DocumentStyle : public Style
//{
// public:
//	Imposition *imposition;
//	DocumentStyle(Imposition *imp);
//	virtual ~DocumentStyle();
//	virtual Style *duplicate(Style *s=NULL);
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

//! Constructor, copies Imposition pointer, does not duplicate imposition.
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

//! Call imposition->dump(f,indent+2,0).
void DocumentStyle::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (imposition) {
		fprintf(f,"%simposition %s\n",spc,imposition->Stylename());
		imposition->dump_out(f,indent+2,0);
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


//---------------------------- PageRange ---------------------------------------
/*! \class PageRange
 * \brief Holds info about page labels.
 *
 * In the future, might implement which imposition should lay it out, allowing more than
 * one imposition to work on same doc, might still make a CompositeImposition...
 */
/*! \var char *PageRange::labelbase
 * \brief The template for creation of page labels.
 *
 * Any instance of '#' is replaced by the number. Multiple '#' like "##" makes
 * a zero padded number, padded on right, but only for Numbers_Arabic and Numbers_Arabic_dec only.
 * 
 * "#" translates to "1", "12", etc.\n
 * "###" is "001", "012","123","1234", etc. \n
 * "A-#" is "A-1", "A-23", etc.
 */
/*! \var int PageRange::offset
 * \brief Page label indices start with this number an go to offset+(end-start).
 */
/*! \var int PageRange::impositiongroup
 * \brief ***sticking to 1 imposition per doc for the time being..this var is ignored
 *
 * The object_id of the imposition that handles this range of pages.
 *
 * Multiple non-continuous page ranges can be processed by the same imposition. The idea
 * is to allow easy [sic] insertion of foldouts and such, which would be processed by
 * a seperate imposition on a different paper size, then later inserted into the other
 * printed material.
 */
/*! \var int PageRange::start
 * \brief The index in doc->pages that this page range starts at.
 */
/*! \var int PageRange::end
 * \brief The index in doc->pages that this page range ends at.
 */
/*! \var int PageRange::labeltype
 * \brief The style of letter, like 1,2,3 or i,ii,iii...
 *
 * Currently uses the enum PageLabelType:
 * <pre>
 *  Numbers_Default	
 *  Numbers_Arabic        1,2,3...
 *  Numbers_Arabic_dec    9,8,7...
 *  Numbers_Roman         i,ii,iii,iv,v,...
 *  Numbers_Roman_dec     v,iv,iii,...
 *  Numbers_Roman_cap     I,II,III,IV,V,...
 *  Numbers_Roman_cap_dec V,IV,III,...
 *  Numbers_abc           a,b,c,...
 *  Numbers_ABC           A,B,C,...
 * </pre>
 */
//class PageRange
//{
// public:
//	int impositiongroup;
//	int start,end,offset;
//	char *labelbase;
//	int labeltype;
//	PageRange(const char *newbase="#",int ltype=Numbers_Default);
//	~PageRange() { if (labelbase) delete[] labelbase; }
//	char *PageRange::GetLabel(int i);
//};

PageRange::PageRange(const char *newbase,int ltype)
{
	impositiongroup=0;
	start=offset=0;
	end=-1;
	labelbase=newstr(newbase);
	labeltype=ltype;
}

//! Convert things like "A-###" to "A-%s" and puts the number of '#' chars in len.
/*! \ingroup misc
 * Assumes there is only one block of '#' chars.
 * 
 * NULL -> NULL, len==0\n
 * "" -> "", len==0\n
 * "blah" -> "blah", len==0\n
 * "#" -> "%s", len==1\n
 * "###" -> "%s", len==3\n
 *
 * Returns a new char[].
 */
char *make_labelbase_for_printf(const char *f,int *len)
{
	if (!f) {
		if (len) *len=0;
		return NULL;
	}
	char *newf;
	const char *p;
	int n=0;
	p=strchr(f,'#');
	if (!p) {
		if (len) *len=0;
		return newstr(f);
	}
	
	while (*p=='#') { p++; n++; }
	newf=new char[strlen(f)-n+6];
	if (n>20) n=20;
	if (p-f-n) strncpy(newf,f,p-f-n);
	if (n) sprintf(newf+(p-f)-n,"%%s");

	if (*p) strcat(newf,p);
	if (len) *len=n;

	return newf;
}

//! Turn 26 into "z" or 27 into "za", etc. Optionally capitalize.
/*! \ingroup misc
 *  Returns a new'd char[].
 */
char *letter_numeral(int i,char cap)
{
	char *n=NULL;

	char d[2];
	d[1]='\0';
	while (i>0) { 
		d[0]=i%26+'a'; 
		appendstr(n,d,1); //prepends
		i/=26; 
	}

	if (cap) for (unsigned int c=0; c<strlen(n); c++) n[c]+='A'-'a';
	return n;
}

//! Make a roman numeral from i, optionally capitalized.
/*! \ingroup misc
 *
 * This makes numbers using ascii i,v,x,l,c,d,m, or I,V,X,L,C,D,M, not the unicode roman numerals U+2150-U+218F.
 * Also, M is the largest chunk of number dealt with, so 5000 translates to "mmmmm" for instance.
 */
char *roman_numeral(int i,char cap)
{
	char *n=NULL;

	while (i>=1000) { appendstr(n,"m"); i-=1000; }
	if (i>=900) { appendstr(n,"cm"); i-=900; }
	if (i>=500) { appendstr(n,"d"); i-=500; }
	if (i>=400) { appendstr(n,"cd"); i-=400; }
	while (i>=100) { appendstr(n,"c"); i-=100; }
	if (i>=90) { appendstr(n,"xc"); i-=90; }
	if (i>=50) { appendstr(n,"l"); i-=50; }
	if (i>=40) { appendstr(n,"xl"); i-=40; }
	while (i>=10) { appendstr(n,"x"); i-=10; }
	if (i>=9) { appendstr(n,"ix"); i-=9; }
	if (i>=5) { appendstr(n,"v"); i-=5; }
	if (i>=4) { appendstr(n,"iv"); i-=4; }
	while (i>=1) { appendstr(n,"i"); i--; }

	if (cap) for (unsigned int c=0; c<strlen(n); c++) n[c]+='A'-'a';
	return n;
}

//! Make label correctly correspond to labelbase and pagenumber.
/*! i is index in doc->pages, so number used is (i-start+offset).
 * If index is not in the range, then NULL is returned, else a new'd char[].
 */
char *PageRange::GetLabel(int i)
{
	if (i<start || i>end) return NULL;
	if (!labelbase || *labelbase=='\0') return newstr("");

	char *label=NULL,*lb,*n;
	
	if (labeltype==Numbers_Arabic_dec || labeltype==Numbers_Roman_dec || labeltype==Numbers_Roman_cap_dec)
		i=end-(start-i)+offset;
	else i=start-i+offset;
	
	int len;
	lb=make_labelbase_for_printf(labelbase,&len);
	if (labeltype==Numbers_Roman_dec || labeltype==Numbers_Roman) n=roman_numeral(i,0);
	else if (labeltype==Numbers_Roman_cap_dec || labeltype==Numbers_Roman_cap) n=roman_numeral(i,1);
	else if (labeltype==Numbers_abc) letter_numeral(i,0);
	else if (labeltype==Numbers_ABC) letter_numeral(i,1);
	else n=numtostr(i+1);
		
	label=new char[strlen(lb)+strlen(n)+1];
	sprintf(label,lb,n);
	delete[] n;
	delete[] lb;
	return label;
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
 * \todo Do a scribus out/in, and a passepartout in.
 */
/*! \var int Document::curpage
 * \brief The index into pages of the current page.
 */
/*! \var char Document::saveas
 * \brief The full absolute path to the document.
 */
//class Document : public ObjectContainer, public LaxFiles::DumpUtility
//{
// public:
//	DocumentStyle *docstyle;
//	char *saveas;
//	
//	Laxkit::PtrStack<Page> pages;
//	int curpage;
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
//	virtual void dump_out(FILE *f,int indent,int what);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//	virtual int Load(const char *file);
//	virtual int Save(LaidoutSaveFormat format=Save_Normal);
//
//	virtual Spread *GetLayout(int type, int index);
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
	curpage=-1;
	saveas=newstr(filename);
	
	docstyle=stuff;
	if (docstyle==NULL) {
		//*** need to create a new DocumentStyle
		//docstyle=Styles::newStyle("DocumentStyle"); //*** should grab default doc style?
		DBG cout <<"***need to implement get default document in Document constructor.."<<endl;
	}
	if (docstyle==NULL) {
		DBG cout <<"***need to implement document constructor.."<<endl;
	} else {
		 // create the pages
		if (docstyle->imposition) pages.e=docstyle->imposition->CreatePages();
		else { 
			DBG cout << "**** in new Document, docstyle has no imposition"<<endl;
		}
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
}

//! Remove everything from the document.
void Document::clear()
{
	pages.flush();
	if (docstyle) { delete docstyle; docstyle=NULL; }
	curpage=-1;
}

//! Return a spread for type and index of that type.
/*! In the future, different impositions might be used for different page ranges.
 * For now, still the much simpler docstyle->imposition is used.
 */
Spread *Document::GetLayout(int type, int index)
{
	if (!docstyle) return NULL;
	if (type==SINGLELAYOUT)       return docstyle->imposition->SingleLayout(index);
	if (type==PAGELAYOUT)         return docstyle->imposition->PageLayout(index);
	if (type==PAPERLAYOUT)        return docstyle->imposition->PaperLayout(index);
	if (type==LITTLESPREADLAYOUT) return docstyle->imposition->GetLittleSpread(index);
	return NULL;
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
	laidout->notifyDocTreeChanged(NULL,TreePagesAdded, starting,-1);
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
		DBG cout << "---page id:"<<pages.e[start]->object_id<<"... "<<endl;
		pages.remove(start);
		DBG cout << "---  Done removing page "<<start+c<<endl;
	}
	laidout->notifyDocTreeChanged(NULL,TreePagesDeleted, start,-1);
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
 * \todo  need to work out saving Specific project/no proj but many docs/single doc
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

	DBG cout <<"....Saving document to "<<saveas<<endl;
//	f=stdout;//***
	fprintf(f,"#Laidout %s Document\n",LAIDOUT_VERSION);
	
	dump_out(f,0,0);
	laidout->DumpWindows(f,0,this);
	
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
	DBG cout <<"----Document::Load read file "<<(file?file:"**** AH! null file!")<<" into a new Document"<<endl;
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
	if (saveas[0]!='/') full_path_for_file(saveas); 

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
 * Recognizes 'docstyle' and 'page'. Discards all else. 
 *
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
			makestr(saveas,value);//*** make sure saveas is abs path
		} else if (!strcmp(name,"docstyle")) {
			if (docstyle) delete docstyle;
			docstyle=new DocumentStyle(NULL);
			docstyle->dump_in_atts(att->attributes.e[c]);
		} else if (!strcmp(name,"page")) {
			PageStyle *ps=NULL;
			if (docstyle && docstyle->imposition) ps=docstyle->imposition->GetPageStyle(pages.n,0);
			page=new Page(ps,0);
			page->layers.flush();
			page->dump_in_atts(att->attributes.e[c]);
			pages.push(page,1);
		}
	}
	 // search for windows to create after reading in all pages
	HeadWindow *head;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"window")) {
			head=static_cast<HeadWindow *>(newHeadWindow(att->attributes.e[c]));
			if (head) laidout->addwindow(head);
		}
	}
}

//! Dumps docstyle, pages.
void Document::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	 //*** shouldn't have this? it is just the filename, file knows that already
	if (saveas) fprintf(f,"%ssaveas %s\n",spc,saveas);
	 // dump docstyle
	if (docstyle) {
		fprintf(f,"%sdocstyle\n",spc);
		docstyle->dump_out(f,indent+2,0);
	}
	 // dump objects
	for (int c=0; c<pages.n; c++) {
		fprintf(f,"%spage %d\n",spc,c);
		pages.e[c]->dump_out(f,indent+2,0);
	}
	 // dump notes/meta data
	//***
}

//! Rename the saveas part of the document. Return 1 for success, 0 for fail.
int Document::Name(const char *nname)
{
	if (!nname || !nname[0]) return 0;
	makestr(saveas,nname);
	if (saveas[0]!='/') full_path_for_file(saveas); 
	return 1;
}

//! Returns basename part of saveas if it exists, else "untitled"
/*! Returns a pointer to a part of saveas, so calling code should immediately
 * copy the returned string, lest saveas be reallocated, and the pointer point
 * to trouble.
 */
const char *Document::Name()
{
	const char *nm=saveas;
	if (!nm || nm && nm[0]=='\0') return "untitled";
	char *bn=strrchr(saveas,'/');
	if (bn) return bn+1;
	return saveas;
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

