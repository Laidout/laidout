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
// Copyright (C) 2004-2009 by Tom Lechner
//

#include <lax/strmanip.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/freedesktop.h>
#include <lax/interfaces/dumpcontext.h>

#include <lax/refptrstack.cc>

#include "document.h"
#include "filetypes/scribus.h"
#include "printing/psout.h"
#include "version.h"
#include "laidout.h"
#include "headwindow.h"
#include "utils.h"
#include "language.h"


using namespace LaxFiles;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 


//------------------------------- PageLableType enum --------------------------------
/*! \enum PageLabelType
 * \brief Enum for PageRange types.
 *
 * See also pageLabelTypeName().
 */

/*! \ingroup lmisc
 * \todo put me somewhere
 */
const char *pageLabelTypeName(PageLabelType t)
{
	switch (t) {
		case Numbers_Default:       return "default";
		case Numbers_Arabic:        return "arabic";
		case Numbers_Roman:         return "roman";
		case Numbers_Roman_cap:     return "roman_capitals";
		case Numbers_abc:           return "abc";
		case Numbers_ABC:           return "ABC";
		case Numbers_None:          return "none";
		default: return NULL;
	}
}


//---------------------------- PageRange ---------------------------------------
/*! \class PageRange
 * \brief Holds info about page labels.
 *
 * Labels are 1,2,3... or i,ii,iii...
 * 
 * \todo *** this might be better off as a doubly linked list
 */
/*! \var char *PageRange::name
 * \brief An id for this range.
 */
/*! \var char *PageRange::labelbase
 * \brief The template for creation of page labels.
 *
 * Any instance of '#' is replaced by the number. Multiple '#' like "##" makes
 * a zero padded number, padded on right, but only for Numbers_Arabic only.
 * 
 * "#" translates to "1", "12", etc.\n
 * "###" is "001", "012","123","1234", etc. \n
 * "A-#" is "A-1", "A-23", etc.
 */
/*! \var int PageRange::first
 * \brief The first number of the range. Page label indices start with this number and go to first+(end-start).
 */
/*! \var int PageRange::start
 * \brief The index in doc->pages that this page range starts at.
 */
/*! \var int PageRange::end
 * \brief The index in doc->pages that this page range ends at.
 *
 * end will always be greater than or equal to start.
 * This variable should be set by Document, which maintains a stack of PageRange 
 * objects. It is not written or read from a file. After a dump_in, its value is -1.
 */
/*! \var int PageRange::labeltype
 * \brief The style of letter, like 1,2,3 or i,ii,iii...
 *
 * Currently uses the enum PageLabelType, which covers arabic numerals, roman numerals, and letters.
 */
/*! \var int PageRange::decreasing
 * \brief Whether the page numbers should decrease from start to end, or decrease.
 */


void pagerangedump(Document *d)
{
	cerr <<"----------#ranges:"<<d->pageranges.n<<endl;
	for (int c=0; c<d->pageranges.n; c++) {
		cerr <<c<<"  start:"<<d->pageranges.e[c]->start<<"  end:"<<d->pageranges.e[c]->end<<endl;
	}
}

PageRange::PageRange()
	: color(65535,65535,65535,65535)
{
	name=NULL;
	first=1;
	start=0;
	end=-1;
	labelbase=NULL;
	labeltype=Numbers_Default;
	decreasing=0;
}

PageRange::PageRange(const char *nm, const char *base, int type, int s, int e, int f, int dec)
	: color(65535,65535,65535,65535)
{
	name=newstr(nm);
	labelbase=newstr(base);
	labeltype=type;
	start=s;
	end=e;
	first=f;
	decreasing=dec;
}

PageRange::~PageRange()
{
	if (labelbase) delete[] labelbase; 
	if (name) delete[] name;
}


StyleDef *PageRangeStyleDef()
{
	StyleDef *sd=new StyleDef(NULL,"PageRange",_("Page Label for Range"),_("Page labels"),
			Element_Fields, NULL,NULL);

	//int StyleDef::push(name,Name,ttip,ndesc,format,range,val,flags,newfunc);
	sd->newfunc=NULL; 

	sd->push("end",
			_("End"),
			_("The document page index at which this range ends."),
			Element_Int,
			"0,context.doc.pages.n", //range
			"0",                    //default value
			0,
			NULL);

	sd->push("start",
			_("Start"),
			_("The document page index at which this range starts."),
			Element_Int,
			"0,context.doc.pages.n", //range
			"0",                    //default value
			0,
			NULL);

	sd->push("first",
			_("First page number of the range"),
			_("First page number of the range"),
			Element_Int, NULL,"0",
			0,
			NULL);

	sd->push("labelbase",
			_("Page label template"),
			_("Page label template"),
			Element_String, NULL,"0",
			0,
			NULL);

	sd->pushEnum("labeltype",_("Page number style"),_("Page number style"),"Arabic",NULL,NULL,
				  "arabic", _("Arabic"), _("Arabic: 1,2,3..."),
				  "roman", _("Lower case roman numerals"), _("Roman: i,ii,iii..."),
				  "roman_cap", _("Upper case roman numerals"), _("Roman: I,II,III..."),
				  "abc", _("Letter numbering"), _("a,b,c,..,aa,ab,..."),
				  "ABC", _("Capital letter numbering"), _("A,B,C,..,AA,AB,..."),
				  "none", _("Don't show any labels for this range"), _("Don't show any labels for this range"),
				  NULL);

	sd->push("reverse",
			_("Reverse order"),
			_("Whether the range goes 1,2,3.. or ..3,2,1."),
			Element_Boolean, NULL,"1",
			0,
			NULL);

	return sd;
}

//! Return a label that correctly corresponds to labelbase and page number i.
/*! This just returns GetLabel(i,first,labeltype).
 */
char *PageRange::GetLabel(int i)
{
	return GetLabel(i,first,Numbers_Default);
}

//! Return a label that correctly corresponds to labelbase and page number i, with overrideable type and first.
/*! i is index in doc->pages, so number used is (i-start+offset).
 * If index is not in the range, then NULL is returned, else a new'd char[].
 *
 * If labeltype is Numbers_None, then "" is always returned, whatever the base is.
 * If labelbase is NULL or "", then "" is returned.
 *
 * If alttype==Numbers_Default, then use the labeltype for this range.
 *
 * \todo maybe have decreasing_to_first and decreasing_from_first
 */
char *PageRange::GetLabel(int i,int altfirst,int alttype)
{
	if (i<start || (end>=0 && i>end)) return NULL; //end>=0 exception is to smooth out temp ranges in PageRangeInterface
	if (!labelbase || *labelbase=='\0' || labeltype==Numbers_None) return newstr("");
	if (altfirst<0) altfirst=first;

	int type=alttype;
	if (type==Numbers_Default) type=labeltype;
	char *label=NULL,*lb,*n;
	
	//if (decreasing) i=end-(i-start)+altfirst; //<-- this is for altfirst==lowest in range
	//else i=i-start+altfirst;
	if (decreasing) i=altfirst-(i-start); //<--this is for altfirst is start of range, maybe not lowest
	else i=altfirst+(i-start);
	
	int len;
	lb=make_labelbase_for_printf(labelbase,&len);
	if (type==Numbers_Roman) n=roman_numeral(i,0);
	else if (type==Numbers_Roman_cap) n=roman_numeral(i,1);
	else if (type==Numbers_abc) n=letter_numeral(i,0);
	else if (type==Numbers_ABC) n=letter_numeral(i,1);
	else n=numtostr(i);

	int lenn=strlen(n);
	if (lenn<len && type==Numbers_Arabic){ //need to pad when using decimal
		char *tt=new char[len-lenn+1];
		memset(tt,'0',len-lenn);
		tt[len-lenn]='\0';
		prependstr(n,tt);
		delete[] tt;
		lenn=len;
	}

	label=new char[strlen(lb)+lenn+1];
	sprintf(label,lb,n);
	delete[] n;
	delete[] lb;
	return label;
}

/*! \todo make labeltype be the enum names.. this ultimately means PageRange
 *    will have to make full switch to Style.
 *
 * If what==-1, write out pseudocode mockup.
 *
 * \todo *** finish what==-1
 */
void PageRange::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sname Blah          #optional name of the range\n",spc);
		fprintf(f,"%sstart 0            #the starting page index for the range\n",spc);
		fprintf(f,"%sfirst 2            #amount to add to the index of each page\n",spc);
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
		fprintf(f,"%s                   # none                ->  (do not show anything for the label)\n",spc);
		return;
	}

	//int start,end,offset;
	//char *labelbase;
	//int labeltype;
	
	if (name) fprintf(f,"%sname %s\n",spc,name);
	fprintf(f,"%sstart %d\n",spc,start);
	fprintf(f,"%sfirst %d\n",spc,first);
	fprintf(f,"%slabeltype %s\n",spc,pageLabelTypeName((PageLabelType)labeltype));
	if (decreasing) fprintf(f,"%sdecreasing",spc);

	fprintf(f,"%slabelbase ",spc);
	dump_out_escaped(f,labelbase,-1);
	fprintf(f,"\n");
}

/*! \todo ultimately PageRange will have to make full switch to Style.
 */
void PageRange::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"name")) {
			makestr(this->name,value);

		} else if (!strcmp(name,"start")) {
			IntAttribute(value,&start);

		} else if (!strcmp(name,"end")) {
			IntAttribute(value,&end);

		} else if (!strcmp(name,"first")) {
			IntAttribute(value,&first);

		} else if (!strcmp(name,"labeltype")) {
			if (!strcmp(value,"arabic")) labeltype=Numbers_Arabic;
			else if (!strcmp(value,"roman")) labeltype=Numbers_Roman;
			else if (!strcmp(value,"roman_capitals")) labeltype=Numbers_Roman_cap;
			else if (!strcmp(value,"abc")) labeltype=Numbers_abc;
			else if (!strcmp(value,"ABC")) labeltype=Numbers_ABC;
			else if (!strcmp(value,"none")) labeltype=Numbers_None;
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
/*! \var LaxFiles::Attribute Document::metadata
 * \brief Extra descriptive metadata attached to the document.
 */
/*! \var LaxFiles::Attribute Document::iohints
 * \brief Extra hints attached through an import of a document.
 *
 * This stuff is installed by importers, and helps exporters export data with a
 * minimum of fuss.
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
	curpage=-1;
	
	Load(filename,NULL);
}

//! Construct from an Imposition.
/*! Document increments count of imposition.
 *
 * Here, the documents pages are created from the imposition.
 * Imposition::CreatePages() creates the pages, and they are
 * put in the pages stack.
 *
 * The filename is put in saveas, but the file is not loaded or saved.
 */
Document::Document(Imposition *imp,const char *filename)//stuff=NULL
{ 
	modtime=times(NULL);
	curpage=-1;
	saveas=newstr(filename);
	name=NULL;
	
	imposition=imp;
	if (imposition) imposition->inc_count();

	if (imposition==NULL) {
		//null imposition is used for code that manually builds a Document, so no special treatment necessary
	} else {
		 // create the pages
		pages.e=imposition->CreatePages(-1);
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
	////DBG cerr <<" Document destructor.."<<endl;
	pages.flush();
	pageranges.flush();
	if (saveas) delete[] saveas;
	if (name) delete[] name;
	if (imposition) imposition->dec_count();
}

//! Remove everything from the document.
void Document::clear()
{
	pages.flush();
	if (imposition) { imposition->dec_count(); imposition=NULL; }
	if (saveas) { delete[] saveas; saveas=NULL; }
	if (name) { delete[] name; name=NULL; }
	curpage=-1;
}

StyleDef* Document::makeStyleDef()
{
	cout <<"*** implement Document styledef!!!"<<endl;
	return NULL; //*****
}

//! Duplicate a document.
/*! \todo *** unimplemented! 
 */
Style *Document::duplicate(Style *s)//s=NULL
{
	cout <<"*** implement Document::duplicate()!!!"<<endl;
	return NULL;
	//if (s==NULL) s=new Document(NULL);
	//if (!dynamic_cast<Document *>(s)) return NULL;
	//Document *ds=dynamic_cast<Document *>(s);
	//if (!ds) return NULL;
	//ds->imposition=(Imposition *)imposition->duplicate();
	//return s;
}

//! Return a spread for type and index of that type.
/*! In the future, different impositions might be used for different page ranges.
 * For now, still the much simpler imposition is used.
 */
Spread *Document::GetLayout(int type, int index)
{
	if (!imposition) return NULL;
	return imposition->Layout(type,index);
}
	
//! Add n new blank pages starting before page index starting, or at end if starting==-1.
/*! Returns number of pages added, or negative for error.
 *
 * \todo *** figure out how to handle upkeep of page range and labels
 * \todo *** if np<0 insert before index starting, else after
 */
int Document::NewPages(int starting,int np)
{
	if (!imposition) return -1;

	if (np<=0) return 0;
	Page *p;

	 //create the pages
	if (starting<0) starting=pages.n;
	for (int c=0; c<np; c++) {
		p=new Page(NULL);
		pages.push(p,1,starting);
	}

	 //adjust pageranges if necessary
	if (pageranges.n) {
		int c;
		for (c=0; c<pageranges.n; c++)
			if (starting>=pageranges.e[c]->start && starting<=pageranges.e[c]->end) break;
		if (c==pageranges.n) c=pageranges.n-1; //if adding to end, extend final pagerange

		for (int c2=c; c2<pageranges.n; c2++) {
			if (c2!=c) pageranges.e[c2]->start+=np;
			pageranges.e[c2]->end+=np;
		}
	}

	 //sync up with the imposition
	imposition->NumPages(pages.n);
	SyncPages(starting,-1);

	laidout->notifyDocTreeChanged(NULL,TreePagesAdded, starting,-1);
	return np;
}

//! Update page labels, usually after a change to the given range, or all if whichrange<0.
void Document::UpdateLabels(int whichrange)
{
	if (whichrange<0 || whichrange>=pageranges.n) {
		for (int c=0; c<pageranges.n; c++) UpdateLabels(c);
		return;
	}

	char *label;
	PageRange *r=pageranges.e[whichrange];
	for (int c=r->start; c<=r->end; c++) {
		label=r->GetLabel(c);
		if (pages.e[c]->label) delete[] pages.e[c]->label;
		pages.e[c]->label=label;
	}
}

/*! Return 0 for range applied ok. 1 for improper range, not applied.
 *
 * If end<0, then make the range go until the end of the document.
 * If end is otherwise less than start, then it is an error, and 1 is returned.
 */
int Document::ApplyPageRange(const char *name, int type, const char *base, int start, int end, int first, int dec)
{
	if (start<0 || start>=pages.n) return 1;
	if (end<0) end=pages.n-1;
	if (end<start) return 1;
	if (end>=pages.n) end=pages.n-1;
	
	PageRange *newrange=new PageRange(name,base,type,start,end,first,dec);
	if (!pageranges.n) {
		 //no previous ranges, so just add the fresh one!
		pageranges.push(newrange,1);

	} else {
		 //there are existing page ranges, we need to overwrite the relevant portions.
		int c=-1;
		if (start<pageranges.e[0]->start) {
			 //new range goes in front of earliest range, but might extend into a later range
			pageranges.push(newrange,1,0);
			c=1; //range to deal with next

		} else if (start>pageranges.e[pageranges.n-1]->end) {
			 //range goes cleanly at the end, no need to cut up anything
			pageranges.push(newrange,1,-1);
			c=-1; //no range to deal with next

		} else {
			 //range will cut into an existing range maybe
			for (c=0; c<pageranges.n; c++) {
				if (start>=pageranges.e[c]->start && start<=pageranges.e[c]->end) break;
			}

			 //the new range cuts into range c.

			if (start==pageranges.e[c]->start) {
				 //new range starts at beginning of range c
				pageranges.push(newrange,1,c);
				c++;

				if (end==pageranges.e[c]->end) {
					 //range completely replaces old
					pageranges.remove(c);
					c=-1;
				} else if (end<pageranges.e[c]->end) {
					 //new replaces only beginning part of old
					//c=c; <--check range c down below
				} else {
					 //new totally covers old, and extends into further ranges
					pageranges.remove(c);
					//c=c; <--check range c down below
				}

			} else {
				 //new range starts somewhere inside range c, but not at beginning
				pageranges.push(newrange,1,c+1);
				int oldend=pageranges.e[c]->end;
				pageranges.e[c]->end=start-1;

				if (end>oldend) {
					 //new range ends in a later range
					c+=2;
				} else if (end==oldend) {
					 //new range covers the rest of range c
					c=-1;
				} else {
					 //new range ends within range c, so we need to add another range after newrange
					char *newname=newstr(pageranges.e[c]->name);
					appendstr(newname,"(2)");
					int first;
					if (pageranges.e[c]->decreasing) first=pageranges.e[c]->first-(end-pageranges.e[c]->start+1);
					else pageranges.e[c]->first=pageranges.e[c]->first+(end-pageranges.e[c]->start+1);
					pageranges.push(new PageRange(newname,
												  pageranges.e[c]->labelbase,
												  pageranges.e[c]->labeltype,
												  end+1,                 //start
												  oldend,               //end
												  first,               //first
												  pageranges.e[c]->decreasing),1,c+2);
					delete[] newname;
					c=-1;
				}

			}

			if (c>=0) {
				 //the end of the new range cuts into some range >=c
				int cut=1;
				while (c<pageranges.n && end>=pageranges.e[c]->end) {
					if (end==pageranges.e[c]->end) cut=0; else cut=1; //don't trim numbers if covers whole range
					pageranges.remove(c);
				}

				 //now c either is pageranges.n, or it is the range that the new range ends in

				if (c<pageranges.n) {
					if (cut) {
						if (pageranges.e[c]->decreasing) pageranges.e[c]->first-=end-start+1;
						else pageranges.e[c]->first+=end-start+1;
					}
					pageranges.e[c]->start=end+1;
				}
			}
		}
	}

	 //now remap page labels
	char *label;
	for (int c=start; c<=end; c++) {
		label=newrange->GetLabel(c);
		if (pages.e[c]->label) delete[] pages.e[c]->label;
		pages.e[c]->label=label;
	}

	return 0;
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
		////DBG cerr << "---page id:"<<pages.e[start]->object_id<<"... "<<endl;
		pages.remove(start);
		////DBG cerr << "---  Done removing page "<<start+c<<endl;
	}
	imposition->NumPages(pages.n);
	SyncPages(start,-1);
	laidout->notifyDocTreeChanged(NULL,TreePagesDeleted, start,-1);
	return n;
}

	
//! Return 0 if saved, return nonzero if not saved.
/*! Save as document file.
 *
 * If includelimbos, then also save laidout->project->papergroups, and 
 * laidout->project->textobjects too.
 *
 * \todo *** only checks for saveas existence, does no sanity checking on it...
 * \todo  need to work out saving Specific project/no proj but many docs/single doc
 * \todo implement error message return
 * \todo *** implement dump context
 */
int Document::Save(int includelimbos,int includewindows,char **error_ret)
{
	if (error_ret) *error_ret=NULL;
	FILE *f=NULL;
	if (isblank(saveas)) {
		////DBG cerr <<"**** cannot save, saveas is null."<<endl;
		if (error_ret) makestr(*error_ret,_("Need a file name to save to!"));
		return 2;
	}
	f=fopen(saveas,"w");
	if (!f) {
		////DBG cerr <<"**** cannot save, file \""<<saveas<<"\" cannot be opened for writing."<<endl;
		if (error_ret) makestr(*error_ret,_("File cannot be opened for writing"));
		return 3;
	}

	setlocale(LC_ALL,"C");
	////DBG cerr <<"....Saving document to "<<saveas<<endl;
//	f=stdout;//***
	fprintf(f,"#Laidout %s Document\n",LAIDOUT_VERSION);
	
	char *dir=lax_dirname(laidout->project->filename,0);
	DumpContext context(dir,1);
	if (dir) delete[] dir;
	dump_out(f,0,0,&context);

	Group *g,*gg;
	if (includelimbos) {
		g=&laidout->project->limbos;
		for (int c=0; c<g->n(); c++) {
			gg=dynamic_cast<Group *>(g->e(c));
			fprintf(f,"limbo %s\n",(gg->id?gg->id:""));
			//fprintf(f,"%s  object %s\n",spc,limbos.e(c)->whattype());
			gg->dump_out(f,2,0,NULL);
		}

		if (laidout->project->papergroups.n) {
			PaperGroup *pg;
			for (int c=0; c<laidout->project->papergroups.n; c++) {
				pg=laidout->project->papergroups.e[c];
				fprintf(f,"papergroup %s\n",(pg->name?pg->name:(pg->Name?pg->Name:"")));
				pg->dump_out(f,2,0,NULL);
			}
		}

		if (laidout->project->textobjects.n) {
			PlainText *t;
			for (int c=0; c<laidout->project->textobjects.n; c++) {
				t=laidout->project->textobjects.e[c];
				fprintf(f,"textobject %s\n",(t->name?t->name:""));
				t->dump_out(f,2,0,NULL);
			}
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
 * \todo *** implement dump context
 */
int Document::Load(const char *file,char **error_ret)
{
	////DBG cerr <<"----Document::Load read file "<<(file?file:"**** AH! null file!")<<" into a new Document"<<endl;
	
	FILE *f=open_laidout_file_to_read(file,"Document",error_ret);
	if (!f) {
		if (!isScribusFile(file)) return 0;
		int c=addScribusDocument(file,this); //0 success, 1 failure
		if (c==0) return 1;
		return 0;
	}
	
	 //so now, assume ok to load attribute styled Document file

	clear();
	setlocale(LC_ALL,"C");
	dump_in(f,0,0,NULL,NULL);
	fclose(f);
	setlocale(LC_ALL,"");
	
	makestr(saveas,file);
	if (saveas[0]!='/') convert_to_full_path(saveas,NULL);

	if (!imposition) imposition=newImpositionByType("Singles");
	if (pages.n==0) {
		pages.e=imposition->CreatePages(-1);
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
	imposition->NumPages(pages.n);
	SyncPages(0,-1);

	if (!strstr(file,".laidout") && !strstr(file,"/templates/")) //***bit of a hack to not touch templates
		touch_recently_used(file,"application/x-laidout-doc","Laidout",NULL);

	////DBG cerr <<"------ Done reading "<<file<<endl<<endl;
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
	
	if (imposition) imposition->SyncPageStyles(this,start,n);
	
	char *label;
	for (int c=start; c<start+n; c++) {
		if (pageranges.n==0 || c<pageranges.e[0]->start || c>pageranges.e[pageranges.n-1]->end) {
			label=new char[12];
			sprintf(label,"%d",c);
		} else {
			int range=0;
			while (range<pageranges.n && c>pageranges.e[range]->end) range++;
			label=pageranges.e[range]->GetLabel(c);
		}
		if (pages.e[c]->label) delete[] pages.e[c]->label;
		pages.e[c]->label=label;
		////DBG cerr <<"=============page["<<c<<"] label="<<label<<endl;
	}
	return n;
}

//! Remove the old imposition, and establish the new imposition.
/*! Return 0 for success, nonzero error.
 *
 * The new imposition will have its count incremented.
 * All the pages in the document will have their page styles updated by the
 * new imposition.
 *
 * \todo when master pages are implemented
 */
int Document::ReImpose(Imposition *newimp,int scale_page_contents_to_fit)
{
	if (!newimp) return 1;

	if (imposition) imposition->dec_count();
	imposition=newimp;
	imposition->inc_count();

	imposition->NumPages(pages.n);
	SyncPages(0,-1);

	if (scale_page_contents_to_fit) {
		cout << " *** need to implement scale page contents to fit Document::ReImpose()!!"<<endl;
	}

	laidout->notifyDocTreeChanged(NULL,TreePagesMoved, 0,-1);
	return 0;
}

//! Low level reading in a document.
/*! 
 * Takes special care to make sure that pagerange->end is set correctly for
 * each range.
 *
 * \todo *** should remove the HeadWindow thing? LaidoutApp should be
 *   handling all the window business. Always nice to separate the windows
 *   from the doc structure.
 */
void Document::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
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

		} else if (!strcmp(nme,"imposition")) {
			if (imposition) imposition->dec_count();
			 // figure out which kind of imposition it is..
			imposition=newImpositionByType(value);
			if (imposition) imposition->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(nme,"pagerange")) {
			PageRange *pr=new PageRange;
			pr->dump_in_atts(att->attributes.e[c],flag,context);
			pageranges.push(pr,1);

		} else if (!strcmp(nme,"page")) {
			PageStyle *ps=NULL;
			//***necessary?:if (imposition) ps=imposition->GetPageStyle(pages.n,0);?
			page=new Page(ps,0);
			if (ps) ps->dec_count();
			page->layers.flush();
			page->dump_in_atts(att->attributes.e[c],flag,context);
			pages.push(page,1);

		} else if (!strcmp(nme,"limbo")) {
			Group *g=new Group;  //count=1
			g->dump_in_atts(att->attributes.e[c],flag,context);
			g->obj_flags|=OBJ_Unselectable|OBJ_Zone;
			if (isblank(g->id) && !isblank(value)) makestr(g->id,value);
			laidout->project->limbos.push(g); // incs count
			g->dec_count();   //remove extra first count

		} else if (!strcmp(nme,"papergroup")) {
			PaperGroup *pg=new PaperGroup;
			pg->dump_in_atts(att->attributes.e[c],flag,context);
			if (isblank(pg->Name) && !isblank(value)) makestr(pg->Name,value);
			laidout->project->papergroups.push(pg);
			pg->dec_count();

		} else if (!strcmp(nme,"textobject")) {
			PlainText *t=new PlainText;  //count=1
			if (!isblank(value)) makestr(t->name,value);
			t->dump_in_atts(att->attributes.e[c],flag,context);
			laidout->project->textobjects.push(t); //incs count
			if (t->texttype==TEXT_Temporary) t->texttype=TEXT_Note;
			t->dec_count();   //remove extra first count

		} else if (!strcmp(nme,"iohints")) {
			if (iohints.attributes.n) iohints.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				iohints.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		} else if (!strcmp(nme,"metadata")) {
			if (metadata.attributes.n) metadata.clear();
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) 
				metadata.push(att->attributes.e[c]->attributes.e[c2]->duplicate(),-1);

		} else if (!strcmp(nme,"view")) {
			SpreadView *v=new SpreadView;
			v->dump_in_atts(att->attributes.e[c],flag,context);
			v->doc_id=object_id;
			spreadviews.push(v);
		}
	}
	
	 //validate pagerange ends==-1 to scale properly
	int laststart=-1;
	for (c=0; c<pageranges.n; c++) {
		if (pageranges.e[c]->start<=laststart) {
			 //invalid start index! supposed to be greater than the last start
			pageranges.remove(c);
			c--;
			continue;
		}
		if (c<pageranges.n-1) pageranges.e[c]->end=pageranges.e[c+1]->start-1;
		else pageranges.e[c]->end=pages.n-1;

		laststart=pageranges.e[c]->start;
	}

	//****** finish this:
	if (imposition) imposition->NumPages(pages.n);
	else {
		////DBG cerr <<"**** no imposition in Document::dump_in_atts\n";
	}
	
	 // make sure pages all have proper labels and pagestyles
	SyncPages(0,-1);
	
	 // search for windows to create after reading in all pages
	 // only if not in project mode
	if (laidout->runmode==RUNMODE_Normal && !laidout->donotusex && isblank(laidout->project->filename)) {
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
}

//! Dumps imposition, pages, pageranges, plus various project attributes if not in project mode.
/*! If what==-1, write out pseudocode mockup.
 */
void Document::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		 //name
		fprintf(f,"%sname   Some name for the doc  #any random name you care to give the document\n",spc);
		fprintf(f,"%ssaveas /path/to/filename.doc  #The path previously saved as, which\n",spc);
		fprintf(f,"%s                              #is currently ignored when reading in again.\n",spc);
		
		 //imposition
		fprintf(f,"%s#A document has only 1 imposition. It can be one of any imposition resources\n",spc);
		fprintf(f,"%s#available, or built from scratch from one of the base imposition types..\n",spc);
		fprintf(f,"%s#These are all the imposition resources currently available:\n",spc);
		for (int c=0; c<laidout->impositionpool.n; c++) {
			fprintf(f,"%simposition %s\n",spc,laidout->impositionpool.e[c]->name);
			//laidout->impositionpool.e[c]->dump_out(f,indent+2,-1,NULL);
			// *** need to figure out which actual styledefs are accessed, and output formats of those
			// base imposition classes, not just resource names!!!
		}
		fprintf(f,"\n%s#These are all the base imposition types currently available:\n",spc);
		dumpOutImpositionTypes(f,indent);
	
		 //page labels
		fprintf(f,"\n\n%s#The page labels for a document are defined within zero or more page ranges.\n",spc);
		fprintf(f,"%s#If no page range blocks are given, then the default numbering 0,1,2... is assumed.\n",spc);
		fprintf(f,"%spagerange\n",spc);
		if (pageranges.n) pageranges.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			PageRange pr;
			pr.dump_out(f,indent+2,-1,NULL);
		}
		
		 //pages and objects
		fprintf(f,"\n\n%s#The pages of a document are currently just a collection\n",spc);
		fprintf(f,"%s#of layers containing drawing objects.\n",spc);
		fprintf(f,"%s#The order of the pages in the document is the same as the order listed in the file,\n",spc);
		fprintf(f,"%s#regardless of any label after \"Page\".\n",spc);
		fprintf(f,"%spage\n",spc);
		
		if (pages.n) pages.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			Page p;
			p.dump_out(f,indent+2,-1,NULL);
		}

		 //views
		fprintf(f,"\n\n%sview     #There can be 0 or more spread editor views\n",spc);
		if (spreadviews.n) spreadviews.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			SpreadView v;
			v.dump_out(f,indent+2,-1,NULL);
		}

		 //iohints
		fprintf(f,"\n\n%siohints    #When files are imported, sometimes data that is not recognized by laidout\n"
				      "%s  ....     #can still be remembered in case you export to the same format. iohints\n"
				      "%s  ....     #contains the document level data of that kind.\n",spc,spc,spc);
		
		 //metedata
		fprintf(f,"\n%smetadata   #Whatever general metadata is attached to the document.\n"
				    "%s  ....     #This might be author, copyright, etc.\n",spc,spc);

		return;
	}

	if (name) fprintf(f,"%sname %s\n",spc,name);
	if (saveas) fprintf(f,"%ssaveas %s\n",spc,saveas);

	 // dump imposition
	if (imposition) {
		fprintf(f,"%simposition %s\n",spc,imposition->Stylename());
		imposition->dump_out(f,indent+2,0,context);
	}

	 // PageRanges
	int c;
	for (c=0; c<pageranges.n; c++) {
		fprintf(f,"%spagerange\n",spc);
		pageranges.e[c]->dump_out(f,indent+2,0,context);
	}
	
	 // dump objects
	for (int c=0; c<pages.n; c++) {
		fprintf(f,"%spage %d\n",spc,c);
		pages.e[c]->dump_out(f,indent+2,0,context);
	}

	 // dump views
	if (spreadviews.n) {
		for (int c=0; c<spreadviews.n; c++) {
			fprintf(f,"%sview #%d\n",spc,c);
			spreadviews.e[c]->dump_out(f,indent+2,0,context);
		}
	}

	 // dump notes/meta data
	if (metadata.attributes.n) {
		fprintf(f,"%smetadata\n",spc);
		metadata.dump_out(f,indent+2);
	}
	
	 // dump iohints if any
	if (iohints.attributes.n) {
		fprintf(f,"%siohints\n",spc);
		iohints.dump_out(f,indent+2);
	}
}

//! Rename the saveas part of the document. Return 1 for success, 0 for fail.
int Document::Saveas(const char *n)
{
	if (isblank(n)) return 0;
	makestr(saveas,n);
	if (saveas[1]!='/') convert_to_full_path(saveas,NULL);
	return 1;
}

//! Returns saveas.
const char *Document::Saveas()
{ return saveas; }


//! Rename the document. Return 1 for success, 0 for fail.
/*! Can't really fail.. is ok to rename to NULL or "". If that is the case,
 * name gets something like "Untitled 3".
 */
int Document::Name(const char *nname)
{
	if (isblank(nname)) {
		if (name) delete[] name;
		name=newstr(Untitled_name());
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
	if (pages.n==0) return NULL;
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

