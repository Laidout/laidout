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
// Copyright (C) 2004-2007 by Tom Lechner
//

#include <lax/strmanip.h>
#include <lax/lists.cc>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/freedesktop.h>

#include "document.h"
#include "filetypes/ppt.h"
#include "printing/psout.h"
#include "version.h"
#include "laidout.h"
#include "headwindow.h"
#include "utils.h"
#include "language.h"


using namespace LaxFiles;

#include <iostream>
using namespace std;
#define DBG 


//---------------------------- DocumentStyle ---------------------------------------

/*! \class DocumentStyle
 * \brief Style for Document objects, suprisingly enough.
 *
 * Should have enough info to create a specific kind of Document.
 * Currently this just means keeping an Imposition object.
 *
 * \todo ***In the future, perhaps it might also have things like whether the default page
 *   advance direction is to the right or left. have to think about interaction between
 *   Document/PageRange/Imposition more.....
 */


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
void DocumentStyle::dump_in_atts(LaxFiles::Attribute *att,int flag)
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
			if (imposition) imposition->dump_in_atts(att->attributes.e[c],flag);
		} else { 
			DBG cerr <<"DocumentStyle dump_in:*** unknown attribute!!"<<endl;
		}
	}
}

/*! Writes like:
 * <pre>
 *  Imposition Singles
 *    ...
 * </pre>
 *
 * Calls imposition->dump(f,indent+2,0).
 *
 * If what==-1, write out pseudocode mockup.
 */
void DocumentStyle::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s#A document style has only 1 of the following impositions attached to it.\n",spc);
		fprintf(f,"%s#These are all the impositions currently installed:\n",spc);
		for (int c=0; c<laidout->impositionpool.n; c++) {
			fprintf(f,"\n%simposition %s\n",spc,laidout->impositionpool.e[c]->Stylename());
			laidout->impositionpool.e[c]->dump_out(f,indent+2,-1);
		}
		return;
	}
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
 * Labels are 1,2,3... or i,ii,iii...
 * 
 * In the future, might implement which imposition should lay it out, allowing more than
 * one imposition to work on same doc, might still make a CompositeImposition...
 *
 * \todo *** this might be better off as a doubly linked list
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
 *
 * This variable should be set by Document, which maintains a stack of PageRange 
 * objects. It is not written or read from a file. After a dump_in, its value is -1.
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


StyleDef *PageRangeStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,"PageRange","Page Label for Range","Page labels","Page labels",
			Element_Fields, NULL,NULL);

	//int StyleDef::push(name,Name,ttip,ndesc,format,range,val,flags,newfunc);
	sd->newfunc=NULL; 
	sd->push("startindex",
			"Start",
			"The page index at which this range starts.",
			"The page index at which this range starts.",
			Element_Int, "0,context.doc.pages.n","0",
			0,
			NULL);
	sd->push("offset",
			"Index offset",
			"Offset to add to the index before making an actual label",
			"Offset to add to the index before making an actual label",
			Element_Int, NULL,"0",
			0,
			NULL);
	sd->push("labelbase",
			"Page label template",
			"Page label template",
			"Page label template",
			Element_String, NULL,"0",
			0,
			NULL);
	StyleDef *e=new StyleDef(NULL,
			"labeltype",
			"Number style",
			"Number style",
			"Number style",
			Element_Enum, NULL,"Arabic",
			NULL,0,NULL);
	e->push("arabic", "Arabic", "Arabic: 1,2,3...", "Arabic: 1,2,3...", 
			Element_EnumVal, NULL,NULL,0,NULL);
	e->push("roman", "Lower case roman", "Roman: i,ii,iii...", "Roman: i,ii,iii...", 
			Element_EnumVal, NULL,NULL,0,NULL);
	e->push("roman_cap", "Upper case roman", "Roman: I,II,III...", "Roman: I,II,III...", 
			Element_EnumVal, NULL,NULL,0,NULL);
	e->push("abc", "Letter numbering", "a,b,c,..,aa,ab,...", "a,b,c,..,aa,ab,...", 
			Element_EnumVal, NULL,NULL,0,NULL);
	sd->push(e);
	sd->push("reverse",
			"Reverse order",
			"Whether the range goes 1,2,3.. or ..3,2,1.",
			"Whether the range goes 1,2,3.. or ..3,2,1.",
			Element_Boolean, NULL,"1",
			0,
			NULL);
	
	return sd;
}

PageRange::PageRange(const char *newbase,int ltype)
{
	name=NULL;
	impositiongroup=0;
	start=offset=0;
	end=-1;
	labelbase=newstr(newbase);
	labeltype=ltype;
	decreasing=0;
}

PageRange::~PageRange()
{
	if (labelbase) delete[] labelbase; 
	if (name) delete[] name;
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
		prependstr(n,d);
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

//! Return a label that correctly corresponds to labelbase and page number i.
/*! i is index in doc->pages, so number used is (i-start+offset).
 * If index is not in the range, then NULL is returned, else a new'd char[].
 */
char *PageRange::GetLabel(int i)
{
	if (i<start || i>end) return NULL;
	if (!labelbase || *labelbase=='\0') return newstr("");

	char *label=NULL,*lb,*n;
	
	if (decreasing) i=end-(i-start)+offset;
	else i=i-start+offset;
	
	int len;
	lb=make_labelbase_for_printf(labelbase,&len);
	if (labeltype==Numbers_Roman) n=roman_numeral(i,0);
	else if (labeltype==Numbers_Roman_cap) n=roman_numeral(i,1);
	else if (labeltype==Numbers_abc) letter_numeral(i,0);
	else if (labeltype==Numbers_ABC) letter_numeral(i,1);
	else n=numtostr(i+1);
		
	label=new char[strlen(lb)+strlen(n)+1];
	sprintf(label,lb,n);
	delete[] n;
	delete[] lb;
	return label;
}

/*! \ingroup misc
 * \todo put me somewhere
 */
const char *labeltypename(PageLabelType t)
{
	switch (t) {
		case Numbers_Default:       return "default";
		case Numbers_Arabic:        return "arabic";
		case Numbers_Roman:         return "roman";
		case Numbers_Roman_cap:     return "roman_capitals";
		case Numbers_abc:           return "abc";
		case Numbers_ABC:           return "ABC";
		default: return NULL;
	}
}

/*! \todo make labeltype be the enum names.. this ultimately means PageRange
 *    will have to make full switch to Style.
 *
 * If what==-1, write out pseudocode mockup.
 *
 * \todo *** finish what==-1
 */
void PageRange::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sname Blah          #optional name of the range\n",spc);
		fprintf(f,"%simpositiongroup 0  #(unimplemented)\n",spc);
		fprintf(f,"%sstart 0            #the starting page index for the range\n",spc);
		fprintf(f,"%soffset 2           #amount to add to the index of each page\n",spc);
		fprintf(f,"%s#labelbase is a template used to construct labels:\n",spc);
		fprintf(f,"%slabelbase \"#\"      #(default) would make normal numbers: 0,1,2,3,...\n",spc);
		fprintf(f,"%s#labelbase \"###\"   #would make labels with 0 padding: 000,001,...100,...999,1000,...\n",spc);
		fprintf(f,"%s#labelbase \"A-#\"   #would make: A-0,A-1,A-2,A-3,...\n",spc);
		fprintf(f,"%sdecreasing         #make the range count down rather than up\n",spc);
		fprintf(f,"%slabeltype default  #this can instead be one of the following:\n",spc);
		fprintf(f,"%s                   # arabic              ->  1,2,3,...\n",spc);
		fprintf(f,"%s                   # roman               ->  i,ii,iii,iv,...\n",spc);
		fprintf(f,"%s                   # roman_capitals      ->  I,II,III,...\n",spc);
		fprintf(f,"%s                   # abc                 ->  a,b,c,...\n",spc);
		fprintf(f,"%s                   # ABC                 ->  A,B,C,...\n",spc);
		return;
	}

	//int impositiongroup;
	//int start,end,offset;
	//char *labelbase;
	//int labeltype;
	
	if (name) fprintf(f,"%sname %s\n",spc,name);
	fprintf(f,"%simpositiongroup %d\n",spc,impositiongroup);
	fprintf(f,"%sstart %d\n",spc,start);
	fprintf(f,"%soffset %d\n",spc,offset);
	fprintf(f,"%slabeltype %s\n",spc,labeltypename((PageLabelType)labeltype));
	fprintf(f,"%slabelbase ",spc);
	dump_out_escaped(f,labelbase,-1);
	fprintf(f,"\n");
}

/*! \todo ultimately PageRange will have to make full switch to Style.
 */
void PageRange::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"name")) {
			makestr(this->name,value);
		} else if (!strcmp(name,"impositiongroup")) {
			IntAttribute(value,&impositiongroup);
		} else if (!strcmp(name,"start")) {
			IntAttribute(value,&start);
		} else if (!strcmp(name,"end")) {
			IntAttribute(value,&end);
		} else if (!strcmp(name,"offset")) {
			IntAttribute(value,&offset);
		} else if (!strcmp(name,"labeltype")) {
			if (!strcmp(value,"arabic")) labeltype=Numbers_Arabic;
			else if (!strcmp(value,"roman")) labeltype=Numbers_Roman;
			else if (!strcmp(value,"roman_capitals")) labeltype=Numbers_Roman_cap;
			else if (!strcmp(value,"abc")) labeltype=Numbers_abc;
			else if (!strcmp(value,"ABC")) labeltype=Numbers_ABC;
			else labeltype=Numbers_Default;
		} else if (!strcmp(name,"decreasing")) {
			decreasing=BooleanAttribute(value);
		} else if (!strcmp(name,"increasing")) {
			decreasing=!BooleanAttribute(value);
		} else if (!strcmp(name,"labelbase")) {
			makestr(labelbase,value);
		}
	}
	if (labelbase==NULL || strlen(labelbase)==0) labelbase=newstr("#");
	end=-1;
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


//! Constructor from a file.
/*! Loads the file.
 */
Document::Document(const char *filename)
{ 
	name=NULL;
	saveas=NULL;
	makestr(saveas,filename);
	modtime=times(NULL);
	docstyle=NULL;
	curpage=-1;
	
	Load(filename,NULL);
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
	name=NULL;
	
	docstyle=stuff;
	if (docstyle==NULL) {
		//*** need to create a new DocumentStyle
		//docstyle=Styles::newStyle("DocumentStyle"); //*** should grab default doc style?
		//DBG cerr <<"***need to implement get default document in Document constructor.."<<endl;
		//
		//this is used for code that manually builds a Document, so no special treatment necessary
	} else {
		 // create the pages
		if (docstyle->imposition) pages.e=docstyle->imposition->CreatePages();
		else { 
			DBG cerr << "**** in new Document, docstyle has no imposition"<<endl;
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
	
	//****this exists here for debugging purposes, would be preemptible later?
	//****perhaps this could be the default page range type?
	//PageRange *range=new PageRange();
	//range->start=0;
	//range->end=pages.n-1;
	//range->offset=5;
	//pageranges.push(range);
	SyncPages(0,-1);
}

Document::~Document()
{
	DBG cerr <<" Document destructor.."<<endl;
	pages.flush();
	pageranges.flush();
	delete docstyle;
	if (saveas) delete[] saveas;
	if (name) delete[] name;
}

//! Remove everything from the document.
void Document::clear()
{
	pages.flush();
	if (docstyle) { delete docstyle; docstyle=NULL; }
	if (saveas) { delete[] saveas; saveas=NULL; }
	if (name) { delete[] name; name=NULL; }
	curpage=-1;
}

//! Return a spread for type and index of that type.
/*! In the future, different impositions might be used for different page ranges.
 * For now, still the much simpler docstyle->imposition is used.
 */
Spread *Document::GetLayout(int type, int index)
{
	if (!docstyle || !docstyle->imposition) return NULL;
	return docstyle->imposition->Layout(type,index);
}
	
//! Add n new blank pages starting before page index starting, or at end if starting==-1.
/*! \todo **** this is rather broken, should migrate maintenance of margins and page width
 * and height to Imposition? The correct PageStyle is not being added here. And when pages
 * are inserted, the pagestyles of the following pages (that already existed) are possibly
 * out of sync with the correct pagestyles, since w(), h(), and margins are incorrect... must
 * update impositioninst.cc
 *
 * Returns number of pages added, or negative for error.
 *
 * \todo *** figure out how to handle upkeep of page range and labels
 * \todo *** if np<0 insert before index starting, else after
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
	if (pageranges.n) pageranges.e[pageranges.n-1]->end=pages.n-1;
	docstyle->imposition->NumPages(pages.n);
	SyncPages(starting,-1);
	laidout->notifyDocTreeChanged(NULL,TreePagesAdded, starting,-1);
	return np;
}

//! Remove pages [start,start+n-1].
/*! Return the number of pages removed, or negative for error.
 *
 * Return -1 for start out of range, -2 for not enough pages to allow deleting.
 * 
 * \todo *** figure out how to handle upkeep of page range and labels
 * \todo *** this is slightly broken.. does not reorient pagestyles
 *   properly.
 */
int Document::RemovePages(int start,int n)
{
	if (pages.n<=1) return -2;
	if (start>=pages.n) return -1;
	if (start+n>pages.n) n=pages.n-start;
	for (int c=0; c<n; c++) {
		DBG cerr << "---page id:"<<pages.e[start]->object_id<<"... "<<endl;
		pages.remove(start);
		DBG cerr << "---  Done removing page "<<start+c<<endl;
	}
	docstyle->imposition->NumPages(pages.n);
	SyncPages(start,-1);
	laidout->notifyDocTreeChanged(NULL,TreePagesDeleted, start,-1);
	return n;
}

	
//! Return 0 if saved, return nonzero if not saved.
/*! Save as document file. 
 *
 * \todo *** only checks for saveas existence, does no sanity checking on it...
 * \todo  need to work out saving Specific project/no proj but many docs/single doc
 * \todo implement error message return
 */
int Document::Save(int includelimbos,int includewindows,char **error_ret)
{
	if (error_ret) *error_ret=NULL;
	FILE *f=NULL;
	if (isblank(saveas)) {
		DBG cerr <<"**** cannot save, saveas is null."<<endl;
		if (error_ret) makestr(*error_ret,_("Need a file name to save to!"));
		return 2;
	}
	f=fopen(saveas,"w");
	if (!f) {
		DBG cerr <<"**** cannot save, file \""<<saveas<<"\" cannot be opened for writing."<<endl;
		if (error_ret) makestr(*error_ret,_("File cannot be opened for writing"));
		return 3;
	}

	setlocale(LC_ALL,"C");
	DBG cerr <<"....Saving document to "<<saveas<<endl;
//	f=stdout;//***
	fprintf(f,"#Laidout %s Document\n",LAIDOUT_VERSION);
	
	dump_out(f,0,0);
	Group *g,*gg;
	if (includelimbos) {
		g=&laidout->project->limbos;
		for (int c=0; c<g->n(); c++) {
			gg=dynamic_cast<Group *>(g->e(c));
			fprintf(f,"limbo %s\n",(gg->id?gg->id:""));
			//fprintf(f,"%s  object %s\n",spc,limbos.e(c)->whattype());
			gg->dump_out(f,2,0);
		}
	}
	if (includewindows) laidout->DumpWindows(f,0,this);
	
	fclose(f);
	setlocale(LC_ALL,"");
	touch_recently_used(saveas,"application/x-laidout-doc","Laidout",NULL);
	return 0;
}

//! Load a document, completely replacing what's here already.
/*! This only clears the current variables when the file can be loaded (but
 * not necessarily read correctly).
 *
 * Return 0 for not loaded, positive for loaded. Note that no new window is created
 * here unless there is a window attribute in the file. (should probably separate
 * window creation from base Document class).
 *
 * \todo *** for file, check that it is in fact a Laidout file! Maybe there should
 *   be a similar function as a standalone so if the file can
 *   be interpreted as another importable file, then loading should be delegated
 *   to the appropriate function....
 * \todo window attributes are found when document is saved independent of a project.
 *   must have mechanism to pass those back to LaidoutApp? right now, that is in
 *   dump_in_atts(), and it shouldn't be there....
 */
int Document::Load(const char *file,char **error_ret)
{
	//*** need to create a new DocumentStyle from what's in the file..
	DBG cerr <<"----Document::Load read file "<<(file?file:"**** AH! null file!")<<" into a new Document"<<endl;
	
	FILE *f=open_laidout_file_to_read(file,"Document",error_ret);
	if (!f) return 0;
	
	clear();
	setlocale(LC_ALL,"C");
	dump_in(f,0,0,NULL);
	fclose(f);
	setlocale(LC_ALL,"");
	
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
	SyncPages(0,-1);

	if (!strstr(file,".laidout") && !strstr(file,"/templates/")) //***bit of a hack to not touch templates
		touch_recently_used(file,"application/x-laidout-doc","Laidout",NULL);

	DBG cerr <<"------ Done reading "<<file<<endl<<endl;
	return 1;
}

//! Make sure each page has the correct PageStyle and page label.
/*! Calls Imposition::SyncPageStyles() then figures out the
 * labels.
 *
 * Returns the number of pages adjusted.
 */
int Document::SyncPages(int start,int n)
{
	if (start>=pages.n) return 0;
	if (n<0) n=pages.n;
	if (start+n>pages.n) n=pages.n-start;
	
	if (docstyle && docstyle->imposition)
		docstyle->imposition->SyncPageStyles(this,start,n);
	
	char *label;
	int range=0;
	for (int c=start; c<start+n; c++) {
		if (pageranges.n==0) {
			label=new char[12];
			sprintf(label,"%d",c);
		} else {
			while (range<pageranges.n && c>pageranges.e[range]->end) range++;
			label=pageranges.e[range]->GetLabel(c);
		}
		if (pages.e[c]->label) delete[] pages.e[c]->label;
		pages.e[c]->label=label;
		DBG cerr <<"=============page["<<c<<"] label="<<label<<endl;
	}
	return n;
}

//! Low level reading in a document.
/*! Please note that this deletes docstyle if nonnull.
 * If docstyle required special treatment, it should have been dealt with
 * previous to coming here.
 *
 * Recognizes 'docstyle', 'page', and 'pagerange'. Discards all else. 
 * Takes special care to make sure that pagerange->end is set correctly for
 * each range.
 *
 * \todo *** should remove the HeadWindow thing? LaidoutApp should be
 *   handling all the window business. Always nice to separate the windows
 *   from the doc structure.
 */
void Document::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	if (!att) return;
	Page *page;
	char *nme,*value;
	int c;
	for (c=0; c<att->attributes.n; c++) {
		nme= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(nme,"name")) {
			makestr(name,value);
		} else if (!strcmp(nme,"saveas")) {
			makestr(saveas,value);//*** make sure saveas is abs path
		} else if (!strcmp(nme,"docstyle")) {
			if (docstyle) delete docstyle;
			docstyle=new DocumentStyle(NULL);
			docstyle->dump_in_atts(att->attributes.e[c],flag);
		} else if (!strcmp(nme,"pagerange")) {
			PageRange *pr=new PageRange;
			pr->dump_in_atts(att->attributes.e[c],flag);
			pageranges.push(pr,1);
		} else if (!strcmp(nme,"page")) {
			PageStyle *ps=NULL;
			if (docstyle && docstyle->imposition) ps=docstyle->imposition->GetPageStyle(pages.n,0);
			page=new Page(ps,0);
			ps->dec_count();
			page->layers.flush();
			page->dump_in_atts(att->attributes.e[c],flag);
			pages.push(page,1);
		} else if (!strcmp(nme,"limbo")) {
			Group *g=new Group;  //count=1
			g->dump_in_atts(att->attributes.e[c],flag);
			if (isblank(g->id) && !isblank(value)) makestr(g->id,value);
			laidout->project->limbos.push(g,0); // incs count
			g->dec_count();   //remove extra first count
		}
	}
	
	 //validate pagerange ends==-1 to scale properly
	for (c=0; c<pageranges.n; c++) {
		if (c<pageranges.n-1) pageranges.e[c]->end=pageranges.e[c+1]->start-1;
		else pageranges.e[c]->end=pages.n-1;
	}
	
	//****** finish this:
	if (docstyle) {
		if (docstyle->imposition) docstyle->imposition->NumPages(pages.n);
		else {
			cout <<"**** no docstyle->imposition in Document::dump_in_atts\n";
		}
	} else {
		cout <<"**** no docstyle in Document::dump_in_atts\n";
	}
	
	 // make sure pages all have proper labels and pagestyles
	SyncPages(0,-1);
	
	 // search for windows to create after reading in all pages
	HeadWindow *head;
	for (int c=0; c<att->attributes.n; c++) {
		nme= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(nme,"window")) {
			head=static_cast<HeadWindow *>(newHeadWindow(att->attributes.e[c]));
			if (head) laidout->addwindow(head);
		}
	}

}

//! Dumps docstyle, pages, pageranges.
/*! If what==-1, write out pseudocode mockup.
 */
void Document::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sname   Some name for the doc  #any random name you care to give the document\n",spc);
		fprintf(f,"%ssaveas /path/to/filename.doc  #The path previously saved as, which\n",spc);
		fprintf(f,"%s                              #is currently ignored when reading in again.\n",spc);
		
		fprintf(f,"\n%s#The document style:\n",spc);
		fprintf(f,"%sdocstyle\n",spc);
		if (docstyle) docstyle->dump_out(f,indent+2,-1);
		else {
			DocumentStyle d(NULL);
			d.dump_out(f,indent+2,-1);
		}
	
		fprintf(f,"\n\n%s#The page labels for a document are defined within zero or more page ranges.\n",spc);
		fprintf(f,"%s#If no page range blocks are given, then the default numbering 0,1,2... is assumed.\n",spc);
		fprintf(f,"%spagerange\n",spc);
		if (pageranges.n) pageranges.e[0]->dump_out(f,indent+2,-1);
		else {
			PageRange pr;
			pr.dump_out(f,indent+2,-1);
		}
		
		fprintf(f,"\n\n%s#The pages of a document are currently just a collection\n",spc);
		fprintf(f,"%s#of layers containing drawing objects.\n",spc);
		fprintf(f,"%s#The order of the pages in the document is the same as the order listed in the file,\n",spc);
		fprintf(f,"%s#regardless of any label after \"Page\".\n",spc);
		fprintf(f,"%spage\n",spc);
		
		if (pages.n) pages.e[0]->dump_out(f,indent+2,-1);
		else {
			Page p;
			p.dump_out(f,indent+2,-1);
		}
		
		return;
	}

	if (name) fprintf(f,"%sname %s\n",spc,name);
	 //*** shouldn't have this? it is just the filename, file knows that already
	if (saveas) fprintf(f,"%ssaveas %s\n",spc,saveas);
	 // dump docstyle
	if (docstyle) {
		fprintf(f,"%sdocstyle\n",spc);
		docstyle->dump_out(f,indent+2,0);
	}

	 // PageRanges
	int c;
	for (c=0; c<pageranges.n; c++) {
		fprintf(f,"%spagerange\n",spc);
		pageranges.e[c]->dump_out(f,indent+2,0);
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
int Document::Saveas(const char *n)
{
	if (isblank(n)) return 0;
	makestr(saveas,n);
	if (saveas[0]!='/') full_path_for_file(saveas); 
	return 1;
}

//! Returns saveas.
const char *Document::Saveas()
{ return saveas; }

//! Return a new string for something like "Untitled 3".
char *Untitled()
{
	static int untitled=0;
	untitled++;

	if (untitled==1) return newstr(_("Untitled"));

	char *str=new char[strlen(_("Untitled"))+10];
	sprintf(str,"%s %d",_("Untitled"),untitled);
	return str;
}

//! Rename the document. Return 1 for success, 0 for fail.
/*! Can't really fail.. is ok to rename to NULL or "". If that is the case,
 * name gets something like "Untitled 3".
 */
int Document::Name(const char *nname)
{
	if (isblank(nname)) {
		if (name) delete[] name;
		name=Untitled();
	}
	makestr(name,nname);
	return 1;
}

//! Returns basename part of saveas if it exists, else "untitled"
/*! If name is blank, then return "basename saveas  (dirname saveas)". 
 * Otherwise return name. If withsaveas!=0, then return something
 * like "name (saveas)"
 *
 * WARNING: this uses a static variable to store the returned value.. Someday
 * this may change, but for now, as long as laidout is single threaded, beware!
 *
 * \todo when modifications implemented, then also return some indication that
 *   the document is in a modified state.
 */
const char *Document::Name(int withsaveas)
{
	static char *nme=NULL;
	if (nme) { delete[] nme; nme=NULL; }

	if (!withsaveas) return name;

	char *n=NULL,*extra=NULL;
	if (isblank(name)) {
		if (!isblank(saveas)) {
			n=newstr(lax_basename(saveas));
			extra=lax_dirname(saveas,0);
		}
	} else {
		n=newstr(name);
		if (!isblank(saveas)) extra=newstr(saveas);
	}

	nme=new char[(n?strlen(n):3) + (extra?strlen(extra):strlen(_("new"))) + 4];
	sprintf(nme,"%s (%s)", n?n:"???", extra?extra:_("new"));
	delete[] n;
	delete[] extra;
	return nme;
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

//! Put the -1 terminated list of items at whatlevel in their own group.
int Document::GroupItems(FieldPlace whatlevel, int *items)
{
	cout <<"*** imp Document::GroupItems!!"<<endl;
	//locate group at whatlevel, then g->GroupObjs(n,items);
	return 0;
}

//! Replace the Group object found at which with all its contents.
int Document::UnGroup(FieldPlace which)
{
	cout <<"*** imp Document::UnGroup!!"<<endl;
	//locate object which and if it is a group, do ungroup with parent group
	return 0;
}

