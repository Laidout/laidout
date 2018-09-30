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
// Copyright (C) 2004-2013 by Tom Lechner
//

#include <lax/strmanip.h>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/freedesktop.h>
#include <lax/interfaces/interfacemanager.h>

#include "document.h"
#include "filetypes/scribus.h"
#include "filetypes/svg.h"
#include "printing/psout.h"
#include "version.h"
#include "laidout.h"
#include "headwindow.h"
#include "utils.h"
#include "interfaces/nodeinterface.h"
#include "language.h"


//template implementation:
#include <lax/refptrstack.cc>


using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;
#define DBG 



namespace Laidout {



	// ***********TEMP!!!
int Document::inc_count()
{
	//DBG cerr <<"document "<<object_id<<" inc_count to "<<_count+1<<endl;
	return anObject::inc_count();
}

int Document::dec_count()
{
	//DBG cerr <<"document "<<object_id<<" dec_count to "<<_count-1<<endl;
	return anObject::dec_count();
}

	// ***********end TEMP!!!

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


ObjectDef *PageRangeObjectDef()
{
	ObjectDef *sd=new ObjectDef(NULL,"PageRange",_("Page Label for Range"),_("Page labels"),
			"class", NULL,NULL);

	//int ObjectDef::push(name,Name,ttip,ndesc,format,range,val,flags,newfunc);
	sd->newfunc=NULL; 

	sd->push("end",
			_("End"),
			_("The document page index at which this range ends."),
			"int",
			"0,context.doc.pages.n", //range
			"0",                    //default value
			0,
			NULL);

	sd->push("start",
			_("Start"),
			_("The document page index at which this range starts."),
			"int",
			"0,context.doc.pages.n", //range
			"0",                    //default value
			0,
			NULL);

	sd->push("first",
			_("First page number of the range"),
			_("First page number of the range"),
			"int", NULL,"0",
			0,
			NULL);

	sd->push("labelbase",
			_("Page label template"),
			_("Page label template"),
			"string", NULL,"0",
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
			"boolean", NULL,"1",
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
 *    will have to make full switch to Value.
 *
 * If what==-1, write out pseudocode mockup.
 */
void PageRange::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
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

void PageRange::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
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
	imposition=NULL;
	
	ErrorLog log;
	Load(filename,log);
}

//! Construct from an Imposition.
/*! Document increments count of imposition.
 *
 * Here, the documents pages are created from the imposition.
 * Imposition::CreatePages() creates the pages, and they are
 * put in the pages stack.
 *
 * The filename is put in saveas, but the file is NOT loaded or saved.
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
				for (c=0; c<pages.n; c++) pages.islocal[c]=LISTS_DELETE_Refcount;
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
	SyncPages(0,-1, false);
}

Document::~Document()
{
	//DBG cerr <<" Document destructor.."<<endl;
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

ObjectDef* Document::makeObjectDef()
{
	cout <<"*** implement Document styledef!!!"<<endl;
	return NULL; //*****
}

//! Duplicate a document.
/*! \todo *** unimplemented! 
 */
Value *Document::duplicate()
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
		pages.push(p,LISTS_DELETE_Refcount,starting);
		p->dec_count();
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
	SyncPages(starting,-1, true);

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
	PageRange *r = pageranges.e[whichrange];
	for (int c=r->start; c<=r->end; c++) {
		if (c >= pages.n) {
			cerr << " *** UpdateLabel(): page index from pagerange out of range for document! dev needs to track this down!!"<<endl;
			r->end = pages.n-1;
			break;
		}
		label=r->GetLabel(c);
		if (pages.e[c]->label) delete[] pages.e[c]->label;
		pages.e[c]->label=label;
	}
}

/*! Search page labels for the first page with that label.
 * If lookafter>=0 then start search on pages with indices after lookafter.
 *
 * If not found, -1 is returned.
 */
int Document::FindPageIndexFromLabel(const char *label, int lookafter)
{
	if (isblank(label)) return -1;
	if (lookafter<0) lookafter=-1;
	for (int c=lookafter+1; c<pages.n; c++) {
		if (isblank(pages.e[c]->label)) continue;
		if (!strcmp(label,pages.e[c]->label)) return c;
	}

	return -1;
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
 */
int Document::RemovePages(int start,int n)
{
	if (pages.n<=1) return -2;
	if (start<0 || start>=pages.n) return -1;
	if (start+n>pages.n) n=pages.n-start;
	for (int c=0; c<n; c++) {
		pages.remove(start);
	}
	imposition->NumPages(pages.n);
	SyncPages(start,-1, true);
	laidout->notifyDocTreeChanged(NULL,TreePagesDeleted, start,-1);
	return n;
}

/*! Use tname as the name of the template. This will try to use the name as the readable name as well as the file name.
 * If it can't use it as a filename, try to use some variant of the current filename instead.
 *
 * Tries to save to laidout->config_dir/templates/tname.
 *
 * If clobber, then always overwrite. If !clobber and the constructed template file exists already,
 * then return it as a new char[] in tfilename_attempt.
 *
 * Return 0 for success, or nonzero for error.
 */
int Document::SaveAsTemplate(const char *tname, const char *tfile,
							 int includelimbos,int includewindows,Laxkit::ErrorLog &log,
							 bool clobber, char **tfilename_attempt)
{
	char *templatefile=NULL;
	if (!tname && tfile) tname=lax_basename(tfile);
	
	if (tfile) {
		makestr(templatefile, tfile);

	} else {
		 //try to find a reasonable path and filename for the template
		char *templatedir = laidout->default_path_for_resource("templates");

		if (!file_exists(templatedir, 1, NULL)) {
			 //dir doesn't exist for some reason, create it!
			if (check_dirs(templatedir, true)!=-1) {
				delete[] templatedir;
				log.AddMessage(_("Could not access template directory."), ERROR_Fail);
				return 1;
			}
		}

		 //build template file path
		templatefile=newstr(templatedir);
		delete[] templatedir;

		appendstr(templatefile, "/");
		if (!strcmp(tname,"default") || !strcmp(tname,_("default"))) {
			appendstr(templatefile, "default");

		} else { 
			 //try to make filename based on tname
			char *tfile=newstr(tname);
			sanitize_filename(tfile,0);
			if (isblank(tfile)) {
				 //tname doesn't translate to a filename, try using saveas
				delete[] tfile;
				tfile=newstr(lax_basename(Saveas()));
			}

			if (isblank(tfile)) {
				 //still no luck with naming, fall back on:
				delete[] tfile;
				tfile=newstr("template");
			}

			appendstr(templatefile, tfile);
			delete[] tfile;
		}
	}

	simplify_path(templatefile,1);

	if (!clobber && file_exists(templatefile, 1, NULL)) {
		if (tfilename_attempt) *tfilename_attempt = newstr(templatefile);
		delete[] templatefile;
		return 3;
	}

	int error=0;
	char *oldname=this->name;
	this->name=newstr(tname);

	if (SaveACopy(templatefile, 1,1,log, true)==0) {
		//success!
	} else {
		//fail!
		if (!log.Total()) log.AddMessage(_("Problem saving. Not saved."), ERROR_Fail);
		error=2;
	}
	delete[] this->name;
	this->name=oldname;

	delete[] templatefile;
	return error;
}

/*! Return 0 for success or nonzero for error.
 */
int Document::SaveACopy(const char *filename, int includelimbos,int includewindows,ErrorLog &log, bool add_to_recent)
{
	if (isblank(filename)) {
		log.AddMessage(_("Need a file name to save to!"),ERROR_Fail);
		return 1;
	}

	char *oldname = newstr(Saveas());
	Saveas(filename);

	int error=0;
	if (Save(includelimbos, includewindows, log, add_to_recent)==0) {
		 //success!

	} else {
		 //failure!
		error=2;
	}

	Saveas(oldname);
	delete[] oldname;
	return error;
}
	
//! Return 0 if saved, return nonzero if not saved.
/*! Save as document file.
 *
 * If includelimbos, then also save laidout->project->papergroups,  
 * laidout->project->textobjects in addition to existing limbo objects.
 *
 * \todo *** only checks for saveas existence, does no sanity checking on it...
 * \todo  need to work out saving Specific project/no proj but many docs/single doc
 */
int Document::Save(int includelimbos,int includewindows,ErrorLog &log, bool add_to_recent)
{
	FILE *f=NULL;
	if (isblank(saveas)) {
		//DBG cerr <<"**** cannot save, saveas is null."<<endl;
		log.AddMessage(_("Need a file name to save to!"),ERROR_Fail);
		return 2;
	}
	f=fopen(saveas,"w");
	if (!f) {
		//DBG cerr <<"**** cannot save, file \""<<saveas<<"\" cannot be opened for writing."<<endl;
		log.AddMessage(_("File cannot be opened for writing"),ERROR_Fail);
		return 3;
	}

	setlocale(LC_ALL,"C");
	//DBG cerr <<"....Saving document to "<<saveas<<endl;
//	f=stdout;//***
	fprintf(f,"#Laidout %s Document\n",LAIDOUT_VERSION);
	
	char *dir=lax_dirname(laidout->project->filename,0);
	DumpContext context(dir,1, object_id);
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

	if (add_to_recent) touch_recently_used_xbel(saveas,"application/x-laidout-doc",
							"Laidout","laidout", //application
							"Laidout", //group
							true, //visited
							true, //modified
							NULL); //recent file
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
 * \todo window attributes are found when document is saved independent of a project.
 *   must have mechanism to pass those back to LaidoutApp? right now, that is in
 *   dump_in_atts(), and it shouldn't be there....
 */
int Document::Load(const char *file,ErrorLog &log)
{
	//DBG cerr <<"----Document::Load read file "<<(file?file:"**** AH! null file!")<<" into a new Document"<<endl;
	if (!file) return 0;
	
	FILE *f=open_laidout_file_to_read(file,"Document",&log);
	if (!f) {
		if (isScribusFile(file)) {
			int c=addScribusDocument(file,this); //0 success, 1 failure
			if (c==0) return 1;
		}

		if (isSvgFile(file)) {
			int c=addSvgDocument(file,this); //0 success, 1 failure
			if (c==0) return 1;
		}
		return 0;
	}
	
	 //so now, assume ok to load attribute styled Document file

	char *dir=lax_dirname(file,0);
	DumpContext context(dir,1, object_id);
	context.log = &log;
	if (dir) delete[] dir;

	clear();
	setlocale(LC_ALL,"C");
	dump_in(f,0,0,&context,NULL);
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
				for (c=0; c<pages.n; c++) pages.islocal[c]=LISTS_DELETE_Refcount;
				curpage=0;
			}
		}
	}
	imposition->NumPages(pages.n);
	SyncPages(0,-1, false);

	laidout->project->ClarifyRefs(log);
	//DBG cerr<<" *** Document::Load should probably have a load context storing refs that need to be sorted, to save time loading..."<<endl;

	if (!(strstr(file,"/laidout/") && strstr(file,"/templates/"))) {
		//***bit of a hack to not make templates show up as recent files
		//   ..file appears to be in a laidout templates config dir.. pretty poor test though!!!

		touch_recently_used_xbel(saveas,"application/x-laidout-doc",
								"Laidout","laidout", //application
								"Laidout", //group
								true, //visited
								false, //modified
								NULL); //recent file
	} else {
		 //we probably opened a template, so zap the save as
		makestr(saveas, NULL);
	}

	//DBG cerr <<"------ Done reading "<<file<<endl<<endl;
	return 1;
}

//! Make sure each page has the correct PageStyle and page label.
/*! Calls Imposition::SyncPageStyles() then figures out the
 * labels.
 *
 * If shift_within_margins, then apply a move transform from old margin area to new margin area.
 *
 * Returns the number of pages adjusted.
 */
int Document::SyncPages(int start,int n, bool shift_within_margins)
{
	if (start>=pages.n) return 0;
	if (n<0) n=pages.n;
	if (start+n>pages.n) n=pages.n-start;
	
	if (imposition) imposition->SyncPageStyles(this,start,n, shift_within_margins);
	
	 //update page number labels
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
		pages.e[c]->Touch();
		//DBG cerr <<"=============page["<<c<<"] label="<<label<<endl;
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
 * If scale_page_contents_to_fit, enlarge or shrink page contents to
 * occupy new page size without exceeding bounds of the new page.
 *
 * If newimp==(existing imposition), then it is assumed an editor has changed
 * something in that imposition instance, and pages merely need to be synced
 * (and possibly resized).
 *
 * \todo when master pages are implemented, will need to ensure they are scaled properly
 * \todo rescaling assumes rectangular pages, but maybe it shouldn't.
 */
int Document::ReImpose(Imposition *newimp,int scale_page_contents_to_fit)
{
	if (!newimp) return 1;

	if (newimp!=imposition) {
		if (imposition) imposition->dec_count();
		imposition=newimp;
		imposition->inc_count();
	}


	if (scale_page_contents_to_fit) {
		 //first grab old page sizes for reference
		flatpoint old[pages.n]; //w+h
		flatpoint  dd[pages.n]; //origin
		for (int c=0; c<pages.n; c++) {
			//dd[c].x=pages.e[c]->pagestyle->minx();
			//dd[c].y=pages.e[c]->pagestyle->miny();
			//old[c].x=pages.e[c]->pagestyle->w();
			//old[c].y=pages.e[c]->pagestyle->h();
			dd[c].x=pages.e[c]->pagestyle->margin->minx;
			dd[c].y=pages.e[c]->pagestyle->margin->miny;
			old[c].x=pages.e[c]->pagestyle->margin->maxx-dd[c].x;
			old[c].y=pages.e[c]->pagestyle->margin->maxy-dd[c].y;
		}
	
		 //then install new page styles
		imposition->NumPages(pages.n);
		SyncPages(0,-1, false);

		 //now update page contents
		SomeData *o;
		DrawableObject *g;
		flatpoint offset;
		double scaling;
		double oldx,oldy, oldw,oldh;
		double neww,newh, newx,newy;

		for (int c=0; c<pages.n; c++) {
		  oldx=dd[c].x;
		  oldy=dd[c].y;
		  oldw=old[c].x;
		  oldh=old[c].y;

		  //newx=pages.e[c]->pagestyle->minx();
		  //newy=pages.e[c]->pagestyle->miny();
		  //neww=pages.e[c]->pagestyle->w();
		  //newh=pages.e[c]->pagestyle->h();
		  newx=pages.e[c]->pagestyle->margin->minx;
		  newy=pages.e[c]->pagestyle->margin->miny;
		  neww=pages.e[c]->pagestyle->margin->maxx-newx;
		  newh=pages.e[c]->pagestyle->margin->maxy-newy;

		  if (newx==oldx && newy==oldy && neww==oldw && newh==oldh)
			  continue; //no need to introduce unnecessary rounding errors

		  //-----------------
		  if (neww/newh > oldw/oldh) {
			  scaling=newh/oldh;
		  } else {
			  scaling=neww/oldw;
		  }
		  offset=flatpoint((newx+neww/2)-(oldx+oldw/2)*scaling, (newy+newh/2)-(oldy+oldh/2)*scaling);
//		  -----------------
//		  if (neww/newh > oldw/oldh) {
//			  scaling=newh/oldh;
//			  offset=flatpoint((neww-scaling*oldw)/2, 0);
//		  } else {
//			  scaling=neww/oldw;
//			  offset=flatpoint(0, (newh-scaling*oldh)/2);
//		  }
//		  offset+=flatpoint(newx-dd[c].x*scaling, newy-dd[c].y*scaling);
//		  -----------------

		   //step through each object on each layer on each page
		  for (int c2=0; c2<pages.e[c]->layers.n(); c2++) {
			g=dynamic_cast<DrawableObject*>(pages.e[c]->layers.e(c2));
			if (g) for (int c3=0; c3<g->n(); c3++) {
			  o=g->e(c3);
			  o->Scale(scaling);
			  o->origin(o->origin()+offset);
			}
		  }
		}

	} else {
		 //simple case, no page content resizing
		imposition->NumPages(pages.n);
		SyncPages(0,-1, true);
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
void Document::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
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

		} else if (!strcmp(nme,"resources")) {
			ResourceManager *resources=InterfaceManager::GetDefault(true)->GetResourceManager();
			resources->dump_in_atts(att->attributes.e[c],0,context);

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
			page=new Page(ps);
			if (ps) ps->dec_count();
			page->layers.flush();
			page->dump_in_atts(att->attributes.e[c],flag,context);
			pages.push(page,LISTS_DELETE_Refcount);
			page->dec_count();

		} else if (!strcmp(nme,"limbo")) {
			Group *g=new Group;  //count=1
			g->dump_in_atts(att->attributes.e[c],flag,context);
			g->obj_flags|=OBJ_Unselectable|OBJ_Zone;
			g->selectable = false;
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
		//DBG cerr <<"**** no imposition in Document::dump_in_atts\n";
	}
	
	 // make sure pages all have proper labels and pagestyles
	SyncPages(0,-1, false);
	
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
void Document::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		 //name
		fprintf(f,"%sname   Some name for the doc  #any random name you care to give the document\n",spc);
		fprintf(f,"%ssaveas /path/to/filename.doc  #The path previously saved as, which\n",spc);
		fprintf(f,"%s                              #is currently ignored when reading in again.\n",spc);

		 //resources
		fprintf(f,"%sresources                     #a list of resource objects used in the document, grouped by type\n",spc);
		fprintf(f,"%s  type Nodes\n",spc);
		fprintf(f,"%s    name Nodes\n",spc);
		fprintf(f,"%s    resource\n",spc);
		fprintf(f,"%s      name SomeNodeResourceName\n",spc);
		//fprintf(f,"%s      favorite 0 #whether this resource is a favorite\n",spc);
		fprintf(f,"%s      object NodeGroup\n",spc);
		NodeGroup node;
		node.dump_out(f, indent+8, -1, context);
		fprintf(f,"%s\n", spc);
		
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


	 //dump out resources
	ResourceManager *resources=InterfaceManager::GetDefault(true)->GetResourceManager();
	fprintf(f,"%sresources\n", spc);
	resources->dump_out(f,indent+2,0,context);


	 // dump imposition
	if (imposition) {
		fprintf(f,"%simposition %s\n",spc,imposition->whattype());
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

} // namespace Laidout

