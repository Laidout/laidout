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
// Copyright (C) 2010-2013 by Tom Lechner
//


#include <lax/transformmath.h>
#include <lax/interfaces/pathinterface.h>

#include "spreadview.h"
#include "../laidout.h"
#include "../language.h"

//template implementation:
#include <lax/refptrstack.cc>


#define DBG
#include <iostream>
using namespace std;

using namespace Laxkit;
using namespace LaxFiles;
using namespace LaxInterfaces;


namespace Laidout {

//------------------------------- arrangetype --------------------------------

const char *arrangetypestring(int a)
{
	if (a==ArrangeAutoAlways)       return _("Auto arrange on each window size change");
	else if (a==ArrangeAutoTillMod) return _("Auto arrange until you move a spread");
	else if (a==Arrange1Row)        return _("Arrange in one row");
	else if (a==ArrangeRows)        return _("Arrange in rows at current scale");
	else if (a==Arrange1Column)     return _("Arrange in one column");
	else if (a==ArrangeGrid)        return _("Arrange in a grid based on screen proportions");
	else if (a==ArrangeCustom)      return _("You will do all arranging");
	return _("? error: bad arrange value");
}


//----------------------- LittleSpread --------------------------------------

/*! \class LittleSpread
 * \brief Data class for use in SpreadEditor
 */
/*! \var int LittleSpread::what
 * 0 == main thread\n
 * 1 == thread of pages\n
 * 2 == thread of spreads
 */
/*! \var LaxInterfaces::PathsData *LittleSpread::connection
 * \brief A path connecting this spread to the previous spread.
 */


LittleSpread::LittleSpread()
{
	deleteattachements=0;
	lasttouch=0;
	spread=NULL;
	connection=NULL;
	prev=next=NULL;
	hidden=false;
}

//! Transfers pointer, does not dupicate s or check it out.
LittleSpread::LittleSpread(Spread *s, LittleSpread *prv)
{
	deleteattachements=0;
	lasttouch=0;
	spread=s;
	connection=NULL;
	prev=prv;
	next=NULL;
	if (prev) {
		prev->next=this;
		mapConnection();
	}
	hidden=false;
}

//! Destructor, deletes connection and spread.
/*! Note that this will NOT prev or next, UNLESS deleteattachments!=0.
 * If deleteattachments!=0, then delete next and ignore prev.
 */
LittleSpread::~LittleSpread()
{
	if (connection) delete connection; //the path connecting to the next spread
	if (spread) delete spread;

	if (deleteattachements) {
		if (prev) prev->next=NULL; //just in case of wraparound lists
		if (next) delete next;
	}
}

//! Return if the point is in the littlespread.
/*! pin==1 means return if point is within the spread's path.
 * pin==2 means return the pagestack index of the spread->pagestack element
 * whose outline contains pp, plus 1. That plus 1 enables 0 to
 * still be used as meaning "not in".
 */
int LittleSpread::pointin(flatpoint pp,int pin)
{
	if (spread) {
		int c;
		double i[6];
		transform_invert(i,m());
		pp=transform_point(i,pp); //makes point in littlespread coords
		c=spread->path->pointin(pp,1);
		if (pin==1 || c==0) return c;
		for (c=0; c<spread->pagestack.n(); c++) {
			if (spread->pagestack.e[c]->outline->pointin(pp,1)) 
				return c+1;
		}
	}
	return 0;
}

//! Finds the bounds of the spread. Ignores connection.
void LittleSpread::FindBBox()
{
	maxx=minx-1;
	if (!spread) return;
	addtobounds(spread->path->m(),spread->path);
}

//! Either create or adjust the connection line to the previous spread.
void LittleSpread::mapConnection()
{
	if (!prev) {
		if (connection) delete connection; 
		connection=NULL;
		return;
	}

	if (!spread) return;
	
	if (connection) { delete connection; }
	connection=new PathsData;
	flatpoint min,max;
	double i[6];
	transform_mult(i,m(),spread->path->m());
	min=transform_point(i,      spread->minimum);
	transform_mult(i,prev->m(),prev->spread->path->m());
	max=transform_point(i,prev->spread->maximum);
	connection->append(min); //control on this spread
	connection->append(min+(max-min)*.33333333, POINT_TOPREV, NULL,1);//bez handle
	connection->append(min+(max-min)*.66666666, POINT_TONEXT, NULL,1);//bez handle
	connection->append(max); //control on previous spread
}


//----------------------- SpreadView --------------------------------------
/*! \class SpreadView
 * \brief Class to hold an arrangement of spreads for use in the SpreadEditor.
 */
/*! \var int SpreadView::doc_id
 * \brief object_id of the Document this view belongs to, or 0 for a temporary view.
 */
/*! \var int SpreadView::default_marker
 * \brief Default color/shape to draw page labels with. -1 means do pagenum%8.
 */
/*! \var int SpreadView::centerlabels
 * \brief Where page labels should be written.
 *
 * Uses LAX_CENTER, LAX_LEFT, LAX_RIGHT, LAX_BOTTOM, or LAX_TOP.
 */
/*! \var int SpreadView::arrangetype
 * \brief How arranging should be done.
 */
/*! \var int SpreadView::arrangestate
 * \brief The current state of the arrangement.
 */
/*! \var char SpreadView::drawthumbnails
 * \brief Whether to draw thumbnails for this view by default.
 */
/*! \var int *SpreadView::temppagemap
 * \brief How the document's pages have been rearranged.
 *
 * temppagemap elements say what doc->page index is temporarily in littlespread index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}.
 */


SpreadView::SpreadView(const char *newname)
  : viewname(NULL),
	doc_id(0),
	default_marker(0),
	centerlabels(LAX_BOTTOM),
	arrangetype (ArrangeAutoTillMod),
	arrangestate(ArrangeNeedsArranging),
	drawthumbnails(1),
	lastmodtime(0)
{
	makestr(viewname,newname);
	transform_identity(matrix);
}

SpreadView::~SpreadView()
{
	delete[] viewname;
}

//! Return whether there are hanging threads, or unapplied rearranged pages.
int SpreadView::Modified()
{
	if (threads.n) return 1;
	for (int c=0; c<temppagemap.n; c++) if (c!=temppagemap.e[c]) return 1;
	return 0;
}

//! Save.
/*! If what==-1, dump out a pseudocode mockup of file format.
 */
void SpreadView::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sname blah           #the name of the view\n",spc);
		fprintf(f,"%sdocument blah       #the filename or name of the document this is a view of\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #transform between screen and real space\n",spc);
		fprintf(f,"%spagemap   0 3 1 2   #how document pages map to view pages\n",spc);
		fprintf(f,"%scenterlabels bottom #center, bottom, left, top, right\n",spc);
		fprintf(f,"%sdrawthumbnails      #whether to use page thumbnails rather than a blank page outline\n",spc);
		fprintf(f,"%sarrangetype 0       #*** this is in flux, check back later\n",spc);
		return;
	}

	//if (doc->saveas) fprintf(f,"%sdocument %s\n",spc,doc->saveas);
	if (viewname) fprintf(f,"%sname %s\n",spc,viewname);

	if (centerlabels==LAX_CENTER) fprintf(f,"%scenterlabels center\n",spc);
	else if (centerlabels==LAX_TOP)    fprintf(f,"%scenterlabels top\n",spc);
	else if (centerlabels==LAX_BOTTOM) fprintf(f,"%scenterlabels bottom\n",spc);
	else if (centerlabels==LAX_LEFT)   fprintf(f,"%scenterlabels left\n",spc);
	else if (centerlabels==LAX_RIGHT)  fprintf(f,"%scenterlabels right\n",spc);
	else fprintf(f,"%scenterlabels %d\n",spc,centerlabels);
	fprintf(f,"%sdrawthumbnails %d\n",spc,drawthumbnails);
	fprintf(f,"%sarrangetype %d\n",spc,arrangetype);
	
	const double *m=matrix;
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);

	 //write out temppagemap
	if (Modified()) {
		fprintf(f,"%spagemap ",spc);
		for (int c=0; c<temppagemap.n; c++) fprintf(f,"%d ",temppagemap[c]);
		fprintf(f,"\n");
	}

	 //output main spread chain
	for (int c=0; c<spreads.n; c++) {
		fprintf(f,"%sspread\n",spc);
		fprintf(f,"%s  index %d\n",spc,spreads.e[c]->spread->pagestack.e[0]->index);
		m=spreads.e[c]->m();
		fprintf(f,"%s  matrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
				spc,m[0],m[1],m[2],m[3],m[4],m[5]);
	}

	 //output any limbo threads
	for (int c=0; c<threads.n; c++) {
		LittleSpread *s=threads.e[c];
		while (s) {
			fprintf(f,"%sthread\n",spc);
			fprintf(f,"%s  pageindex %d\n",spc,s->spread->pagestack.e[0]->index);
			fprintf(f,"%s    matrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
					 spc,s->m(0),s->m(1),s->m(2),s->m(3),s->m(4),s->m(5));
			s=s->next;
			if (s==threads.e[c]) break; //watching out for circular threads
		}
	}
}

//! Load.
void SpreadView::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	char *name,*value;
	double m[6];
	int n,s=0;

	spreads.flush();

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"name")) {
			makestr(viewname,value);

		} else if (!strcmp(name,"matrix")) {
			n=DoubleListAttribute(value,matrix,6);
			if (n!=6) transform_identity(matrix);

		} else if (!strcmp(name,"document")) {
			Document *newdoc=laidout->findDocument(value);
			if (newdoc) {
				//if (doc) doc->dec_count(); *** Document might be owner
				doc_id=newdoc->object_id;
				//if (doc) doc->inc_count();
			}

		} else if (!strcmp(name,"centerlabels")) {
			if (isblank(value)) centerlabels=LAX_CENTER;
			else if (!strcmp(value,"center")) centerlabels=LAX_CENTER;
			else if (!strcmp(value,"top"))    centerlabels=LAX_TOP;
			else if (!strcmp(value,"bottom")) centerlabels=LAX_BOTTOM;
			else if (!strcmp(value,"left"))   centerlabels=LAX_LEFT;
			else if (!strcmp(value,"right"))  centerlabels=LAX_RIGHT;
			else if (IntAttribute(value,&centerlabels)==0) centerlabels=LAX_CENTER;
			else centerlabels=LAX_CENTER;

		} else if (!strcmp(name,"drawthumbnails")) {
			drawthumbnails=BooleanAttribute(value);

		} else if (!strcmp(name,"arrangetype")) {
			IntAttribute(value,&arrangetype);

		} else if (!strcmp(name,"pagemap")) {
			temppagemap.flush();
			int *i=NULL;
			IntListAttribute(value,&i,&n);
			temppagemap.insertArray(i,n);

		} else if (!strcmp(name,"thread")) {
			 //read in chain of pages
			LittleSpread *s=NULL;
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"pageindex")) {
					if (!s) s=new LittleSpread();
					else { 
						s->next=new LittleSpread();
						s->next->prev=s;
						s=s->next;
					}
					IntAttribute(value,&s->iid);

					if (att->attributes.e[c]->attributes.e[c2]->attributes.n 
							&& !strcmp(att->attributes.e[c]->attributes.e[c2]->attributes.e[0]->name,"matrix")) {
						n=DoubleListAttribute(att->attributes.e[c]->attributes.e[c2]->attributes.e[0]->value,m,6);
						if (n==6) s->m(m);
					}
				}
			}
			if (s) threads.push(s);

		} else if (!strcmp(name,"spread")) {
			 //read in the next chunk of the main chain
			spreads.push(new LittleSpread(NULL, spreads.n?spreads.e[spreads.n-1]:NULL),1);
			s=spreads.n-1;
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;
				if (!strcmp(name,"matrix")) {
					n=DoubleListAttribute(value,m,6);
					if (n==6) spreads.e[s]->m(m);
				}
			}
		}
	}
	
	Update(laidout->findDocumentById(doc_id)); //must insert Spread objects into the LittleSpread place holders...
}

//! Return the reverse map from temppagemap[].
/*! Returns index of temppagemap.
 *
 * temppagemap elements say what doc->page index is temporarily stored in
 * littlespread->spread->pagestack->index. For instance,
 * if temppagemap=={0,1,2,3,4}, after swapping 4 and 1, temppagemap=={0,4,2,3,1}. Then say
 * you swap the fourth and fifth pages as you see them, then now temppagemap=={0,4,2,1,3}.
 * 
 * So reversemap(4) would return 1 (the index of "4" in temppagemap), 
 * reversemap(1) returns 3, reversemap(3) returns 4,  reversemap(2)==2, reversemap(0)==0.
 *
 * Return -1 on not found.
 */
int SpreadView::reversemap(int i)
{
	for (int c=0; c<temppagemap.n; c++) 
		if (temppagemap.e[c]==i) return c;
	return -1;
}

//! Given a view page index, return what document page index it corresponds to.
/*! Say you have 10 pages, and you've swapped page indices 2 and 9, but all the
 * other pages are unmoved.
 * Then map(2) would return 9, map(9)==2, map(3)==3, etc.
 */
int SpreadView::map(int i)
{
	if (i<0 || i>=temppagemap.n) return -1;
	return temppagemap.e[i];
}

//! Remove a page from existing in a thread.
/*! If thread>=0, then assume it is in that thread.
 *
 * Please note this will NOT update the main spreads thread so that the page will appear
 * back in it somewhere.
 *
 * Return 0 for page index not found in a thread.
 * Return 1 for page removed, and thread still has pages in it.
 * Return 2 for page removed, and thread was empty, so was deleted.
 */
int SpreadView::RemoveFromThread(int pageindex, int thread)
{
	//find thread the page index is in, if any
	LittleSpread *s=NULL;
	int first=1;
	int c;
	for (c=thread>=0?thread:0; c<threads.n; c++) {
		if (c==thread && !first) continue; //alread searched this thread
		s=threads.e[c];
		while (s) {
			if (s->spread->pagestack.e[0]->index==pageindex) break;
			s=s->next;
		}
		if (s) break; //spread found
		if (c==thread && first) first=0; 
	}
	if (!s) return 0;
	thread=c;

	//so now we found pageindex in s, part of thread c
	if (s->prev==NULL && s->next==NULL) {
		//page was only element of the thread, so totally remove it
		threads.remove(thread);
		return 2;
	}

	//detach spread from thread
	LittleSpread *ss=s;
	s=s->next;
	if (s) s->prev=ss->prev;
	if (ss->prev) ss->prev->next=s;
	ss->next=s->prev=NULL;
	if (ss==threads.e[thread]) {
		//offending spread is at head of thread
		threads.e[thread]=s; //if ss was head, then it had no prev, thus s is new head
	}
	delete ss;
	return 1;
}

//! Remove a page from the main thread, and put it into thread.
/*! If thread<0, then create a new thread. Otherwise, try to put it at
 * index threadplace in the thread. If threadplace out of bounds, put it at end
 * of the thread.
 *
 * Return 0 for success, nonzero for error.
 */
int SpreadView::MoveToThread(int pageindex,int thread, int threadplace)
{
	//***
	return 1;
}

//***********more thought required about thread maintenance
////! Make pageindex be active in the main spreads thread.
///*! This is usually called when the pageindex has just been removed from a thread
// * and needs to be reinserted into main thread.
// */
//int SpreadView::reactivatePage(int pageindex)
//{***
//	int actualindex=reversemap(pageindex);
//
//	LittleSpread spr=NULL;
//	int psi;
//	for (int c=0; c<spreads.n; c++) {
//		spr=spread.e[c]->spread;
//		psi=spr->PagestackIndex(pageindex)
//		if (psi>=0) break;
//	}
//	spr->pagestack.e[psi]->index=pageindex;********
//}

//! Make sure temppagemap is indeed a one to one map.
/*! Return 0 for map was already valid.
 * Return 2 for map is empty.
 * Return 1 for map had inconsistencies, and has been corrected.
 *
 * \todo this is unfinished! It should only be necessary after reading in
 *   a view in order to validate it. If views are created and modified only with a spread editor,
 *   this should not be necessary.
 */
int SpreadView::validateTemppagemap()
{
	return 1;
//	if (!temppagemap.n) return 2;
//
//	int errors=0;
//	int n=temppagemap.n;
//	int map[n]; //list for each doc->page
//	for (int c=0; c<n; c++) {
//		map[c]=reversemap(c);
//	}
//	for (int c=0; c<n; c++) {
//		if (map[c]<0) *** found hole!!
//	}
//
//	----------------
//	for (int c=0; c<n; c++) {
//		if (temppagemap[c]<0 || temppagemap>=n) {
//			 //index out of range
//			errors++;
//			temppagemap[c]=-1;
//		} else {
//			if (***page index already in map) { skip, errors++ }
//			map[temppagemap.e[c]]=c;
//		}
//	}
//
//	for (int c=0; c<n; c++) {
//		if (map[c]<0)
//	}
}

//! Sync up with a Document, so as to not point to missing pages.
/*! Return 1 if things have changed, else 0.
 *
 * Each thread is a string of imposition->SingleLayout() spreads, but the
 * main thread is a string of imposition->LittleSpread() spreads.
 */
int SpreadView::Update(Document *doc)
{
	if (!doc || !doc->imposition) return 0;

	LittleSpread *s=NULL;

	 //completely initialize from scratch:
	if (spreads.n==0) {
		if (threads.n) threads.flush();

		while (temppagemap.n>doc->pages.n) temppagemap.pop();
		while (temppagemap.n<doc->pages.n) temppagemap.push(0);
		for (int c=0; c<doc->pages.n; c++) {
			temppagemap.e[c]=c;
		}
		Spread *s;
		LittleSpread *ls;
		for (int c=0; c<doc->imposition->NumSpreads(LITTLESPREADLAYOUT); c++) {
			s=doc->imposition->GetLittleSpread(c);
			ls=new LittleSpread(s,(spreads.n?spreads.e[spreads.n-1]:NULL)); //takes pointer, not dups or checkout
			ls->FindBBox();
			spreads.push(ls);
		}
		return 1;
	} //else must fix existing littlespreads


	 //Adjust threads.
	 //threads are strings of single pages (for now), so just detach and delete out of range one,
	 //and renew spreads of ones in range
	Spread *spr;
	LittleSpread *ss;
	for (int c=0; c<threads.n; c++) {
		s=threads.e[c];
		while (s) {
			//for (int c2=0; c2<s->spread->pagestack.n; c2++) {//*** someday have random spreads in threads
				int c2=0;
				if (s->spread->pagestack.e[c2]->index>=doc->pages.n) {
					 //spread out of range, remove this spread!
					if (s==threads.e[c]) { //offending spread is at head of first spread
						if (!s->next) { //only one spread in thread
							threads.remove(c);
							c--;
							s=NULL;
							break;
						}
						 //else detach s from threads.c
						ss=s;
						s=s->next;
						if (s) s->prev=ss->prev;
						if (ss->prev) ss->prev->next=s;
						ss->next=s->prev=NULL;
						delete ss;
					}
					break;
				}
				 //renew singles spread
				spr=doc->imposition->SingleLayout(s->spread->pagestack.e[c2]->index);
				delete s->spread;
				s->spread=spr;
				s=s->next; break; 
			//} //loop over s->spread->pagestack, assuming only singles in threads for now...
		}
	}


	 //go through main spreads thread, refreshing each spread

	 // correct temppagemap to have correct number of elements
	while (temppagemap.n>doc->pages.n) temppagemap.pop();
	while (temppagemap.n<doc->pages.n) temppagemap.push(0);
	for (int c=0; c<temppagemap.n; c++) temppagemap.e[c]=c;//***resetting page map

	LittleSpread *littlespread;
	Spread *freshspread;
	int c;
	double x=0,y=0;
	for (c=0; c<doc->imposition->NumSpreads(LITTLESPREADLAYOUT); c++) {
		freshspread=doc->imposition->GetLittleSpread(c);
		
		 // try to preserve previous spread placement
		if (c<spreads.n) {
			x=spreads.e[c]->m(4);
			y=spreads.e[c]->m(5);		
		} else if (c>=0) {
			x+=(spreads.e[c-1]->maxx-spreads.e[c-1]->minx)*1.2;
		}

		 // set up and install replacement spread
		littlespread=new LittleSpread(freshspread,c>0?spreads.e[c-1]:NULL); //takes pointer, not dups or checkout
		littlespread->m(4,x);
		littlespread->m(5,y);
		littlespread->FindBBox();
		if (c<spreads.n) {
			delete spreads.e[c];
			spreads.e[c]=littlespread;
		} else {
			spreads.push(littlespread);
		}
		if (c>0) spreads.e[c]->mapConnection();
	}
	while (c!=spreads.n) spreads.remove(c); //remove excess spreads

	 // blank out any pages that are in threads
	Spread *spread;
	for (c=0; c<spreads.n; c++) {
		spread=spreads.e[c]->spread;
		for (int c2=0; c2<spread->pagestack.n(); c2++) {
			if (SpreadOfPage(spread->pagestack.e[c2]->index,NULL,NULL,NULL,1)) { 
				//page is in a thread
				spread->pagestack.e[c2]->index=-1;
				spread->pagestack.e[c2]->page=NULL;
			}
		}
	}

	return 1;
}

//! Arrange the spreads in some sort of order.
/*! Default is have 1 page take up about an inch of screen space.
 * The units stay in spread units, but the viewport is scaled so that a respectable
 * number of spreads fit on screen.
 *
 * how==-1 means use default arrangetype. 
 * Otherwise, see ArrangeTypes.
 *
 * This is called on shift-'a' keypress, a firsttime refresh, and curwindow resize.
 * 
 * \todo the auto grid arranging could be smarter
 */
void SpreadView::ArrangeSpreads(Displayer *dp,int how)//how==-1
{
	if (!spreads.n) return;

	if (arrangetype==arrangestate) return;
	if (arrangetype==ArrangeCustom) {
		if (arrangestate!=ArrangeNeedsArranging) { arrangestate=ArrangeCustom; return; }
	}

	 // figure out how we should be laying out spreads
	 // return if already arranged according to arrangetype
	int towhat=arrangetype;
	double winw,winh;
	winw=dp->Maxx-dp->Minx;
	winh=dp->Maxy-dp->Miny;
	if (towhat==ArrangeAutoAlways || towhat==ArrangeAutoTillMod) {
		 // could be row or column
		if (winw>3*winh) { //row
			if (arrangestate==ArrangeTempRow || arrangestate==Arrange1Row) {
				arrangestate=ArrangeTempRow;
				return;
			}
			towhat=Arrange1Row;
			arrangestate=ArrangeTempRow;

		} else if (winh>3*winw) { //column
			if (arrangestate==ArrangeTempColumn || arrangestate==Arrange1Column) {
				arrangestate=ArrangeTempColumn;
				return;
			}
			towhat=Arrange1Column;
			arrangestate=ArrangeTempColumn;

		} else { //autogrid
			if (arrangestate==ArrangeTempGrid || arrangestate==ArrangeGrid) {
				arrangestate=ArrangeTempGrid;
				return;
			}
			towhat=ArrangeGrid;
			arrangestate=ArrangeTempGrid;
		}

	} else if (towhat==ArrangeCustom) {
		towhat=ArrangeGrid;
		arrangestate=ArrangeCustom;
	} else arrangestate=arrangetype;
	
	 // find the bounding box for all spreads
	DoubleBBox bbox;
	double gap;
	int perrow;
	for (int c=0; c<spreads.n; c++) {
		bbox.addtobounds(spreads.e[c]);
	}
	gap=(bbox.maxx-bbox.minx)*.2;
	
	 // figure out how many spreads per row
	if (towhat==ArrangeGrid) {
		double r=sqrt(spreads.n*winh/winw*(bbox.maxx-bbox.minx+gap)/(bbox.maxy-bbox.miny+gap));
		perrow=(int)(spreads.n/r);
		if (perrow==0) perrow=1;
		//perrow=int(sqrt((double)spreads.n)+1); //put within a square

	} else if (towhat==Arrange1Column) {
		perrow=1;

	} else if (towhat==ArrangeRows) {
		perrow=(dp->Maxx-dp->Minx)/(bbox.maxx-bbox.minx)/dp->Getmag();
		if (perrow<1) perrow=1;

	} else   if (towhat==Arrange1Row) perrow=1000000;
	
	
	double x,y,w,h; //temporary variables for spread dimensions
	double X,Y,W,H; //final dimensions of spread arrangement
	x=y=w=h=X=Y=W=H=0;

	 // Find how many pixels make 1 inch, basescaling=inches/pixel
	 // this is difficult, as X does not return reliable values
	//double scaling;
	//double basescaling=double(XDisplayWidthMM(anXApp::app->dpy,0))/25.4/XDisplayWidth(anXApp::app->dpy,0);
	
	 // Position the LittleSpreads...
	DoubleBBox bb;
	double rh=0,rw=0;

	for (int c=0; c<spreads.n; c++) {
		w=spreads.e[c]->maxx-spreads.e[c]->minx;
		h=spreads.e[c]->maxy-spreads.e[c]->miny;
		//if (c==0) { scaling=1./(basescaling*w); }
		spreads.e[c]->origin(flatpoint(x-spreads.e[c]->minx,y-spreads.e[c]->miny));
		x+=w+gap;
		rw+=w+gap;
		if (h>rh) rh=h;
		if (c==spreads.n-1 || (c+1)%perrow==0) { // start a new row.
			H+=rh;
			x=0;
			y-=rh+gap;
			if (rw>W) W=rw; 
			rw=rh=0;
		}
	}
	if (W==0) W=rw;
	if (H==0) H=rh;
	
	 //sync up the connecting lines
	for (int c=1; c<spreads.n; c++) {
		spreads.e[c]->mapConnection();
	}
	
	 //add a pad around the spreads, and set the displayer to have these work space bounds...
	X-=W*.1;
	W+=.2*W;
	Y-=H*.1;
	H+=.2*H;

	minx=X;
	miny=Y;
	maxx=X+W;
	maxy=Y+H;

	DBG cerr <<"spreadview bounds: p1:"<<X<<','<<Y<<" p2:"<<X+W<<','<<Y+H<<endl;
}

//! For an already arranged view, set bounds properly.
void SpreadView::FindBBox()
{
	ClearBBox();

	for (int c=0; c<spreads.n; c++) {
		addtobounds(spreads.e[c]->m(),spreads.e[c]);
	}

	for (int c=0; c<threads.n; c++) {
		addtobounds(threads.e[c]->m(),threads.e[c]);
	}
}

//! Return the spread that contains the given document page index, and what thread it is in.
/*! Main thread is 0, otherwise it is 1+(index of thread in thread stack).
 *
 * Also optionally return the spread number in the thread, and the spread's page stack index
 * containing the page.
 */
LittleSpread *SpreadView::SpreadOfPage(int page, int *thread, int *spreadi, int *psi, int skipmain)
{
	LittleSpread *s;
	for (int c=0; c<threads.n; c++) {
		s=threads.e[c];
		int i=0, psindex;
		while (s) {
			psindex=s->spread->PagestackIndex(page);
			if (psindex>=0) {
				if (thread) *thread=c+1;
				if (spreadi) *spreadi=i;
				if (psi) *psi=psindex;
				return s;
			}
			s=s->next;
			i++;
		}
	}
	if (!skipmain) {
		int c=0, psindex;
		for (c=0; c<spreads.n; c++) {
			psindex=spreads.e[c]->spread->PagestackIndex(page);
			if (psindex>=0) {
				if (thread) *thread=0;
				if (spreadi) *spreadi=c;
				if (psi) *psi=psindex;
				return spreads.e[c];
			}
		}
	}
	if (thread) *thread=-1;
	if (spreadi) *spreadi=-1;
	if (psi) *psi=-1;

	return NULL;
}

//! Find the spread under point (x,y), return it, or NULL.
/*! If page and thread!=NULL, then also find the index in spread->pagestack
 * of the page clicked down on, and
 * the thread number, where 0 means the main thread, and any other positive number
 * is the index of the thread in threads stack plus 1.
 */
LittleSpread *SpreadView::findSpread(int x,int y, int *pagestacki, int *thread)
{
	flatpoint p(x,y);

	 //first check in main thread
	int pg=-1,c;
	for (c=spreads.n-1; c>=0; c--) { //search reverse because that's how they are displayed
		pg=spreads.e[c]->pointin(p,2);
		if (pg) {
			*pagestacki=pg-1;
			*thread=0;
			return spreads.e[c];
		}
	}

	 //check other threads
	LittleSpread *spr;
	for (c=threads.n-1; c>=0; c--) {
		spr=threads.e[c];
		while (spr) {
			pg=spr->pointin(p,2);
			if (pg) {
				*pagestacki=pg-1;
				*thread=c;
				return spreads.e[c];
			}
			spr=spr->next;
		}
	}

	*pagestacki=-1;
	*thread=-1;
	return NULL;
}

//! Swap positions (temporarily) of temppagemap indices previouspos and newpos.
/*! This does not change actual order in the document, only how it appears in the view.
 * The actual document pages are map(previouspos) and map(newpos).
 *
 * Return 0 for success, or nonzero for error and not swapped.
 */
int SpreadView::SwapPages(int previouspos, int newpos)
{
	int thread1,thread2;
	LittleSpread *s1,*s2;
	int ps1, ps2;
	int page1=map(previouspos);
	int page2=map(newpos);
	if (page1<0 || page2<0) return 3;

	s1=SpreadOfPage(page1,&thread1,NULL,NULL,0);
	s2=SpreadOfPage(page2,&thread2,NULL,NULL,0);
	if (!s1 || !s2) return 1;

	ps1=s1->spread->PagestackIndex(page1);
	ps2=s2->spread->PagestackIndex(page2);
	if (ps1<0 || ps2<0) return 2;

	 // swap index map
	int t=temppagemap[previouspos];
	temppagemap[previouspos]=temppagemap[newpos];
	temppagemap[newpos]=t;

	 //swap page
	Page *tpage=s1->spread->pagestack.e[ps1]->page;
	s1->spread->pagestack.e[ps1]->page=s2->spread->pagestack.e[ps2]->page;
	s2->spread->pagestack.e[ps2]->page=tpage;

	 //swap index
	t=s1->spread->pagestack.e[ps1]->index;
	s1->spread->pagestack.e[ps1]->index=s2->spread->pagestack.e[ps2]->index;
	s2->spread->pagestack.e[ps2]->index=t;

	 //if transferring outside of main thread, then must remap to use single page outline only
	if (thread1==0 && thread2>0) {
		//***
		//Spread *newspread=doc->imposition->SingleLayout(s->spread->pagestack.e[c2]->index);
		//delete s->spread;
		//s->spread=spr;
		cerr <<" *** must remap singles thread for swapping from main to thread"<<endl;
	}

	return 0;
}

//! Apply the current mapping.
/*! This fails if there are any threads.
 *
 * Return 0 for success or nonzero for error.
 */
int SpreadView::ApplyChanges()
{
	if (!doc_id || threads.n) return 1;

	Document *doc=laidout->findDocumentById(doc_id);
	if (!doc) return 1;

	int n;
	char *newlocal,*oldlocal;
	Page **newpages,**oldpages=doc->pages.extractArrays(&oldlocal,&n);
	
	newpages=new Page*[n];
	newlocal=new char[n];
	
	int pg;
	//LittleSpread *s1;
	//int thread=-1, spreadi=-1, psi=-1;
	for (int c=0; c<n; c++) {
		pg=temppagemap[c]; //doc page index
		DBG cerr <<" --move page "<<pg<<" to page "<<c<<endl;
		
		 //map Document pages
		newpages[c]=oldpages[pg];
		newlocal[c]=oldlocal[pg];

		 //map links in spreads
//		if (temppagemap[c]!=c) {
//			SwapPages(reversemap(c),reversemap(temppagemap[c]));
//			s1=SpreadOfPage(pg,&thread,&spreadi,&psi,0);
//			s1->spread->pagestack.e[psi]->index=c;
//			temppagemap[c]=c;
//		}
//		temppagemap[c]=c;
	}

	for (int c=0; c<spreads.n; c++) {
		for (int c2=0; c2<spreads.e[c]->spread->pagestack.n(); c2++) {
			pg = spreads.e[c]->spread->pagestack.e[c2]->index;
			if (pg < 0) continue;
			int newpg = reversemap(pg);
			if (newpg >=0 && newpg < doc->pages.n) doc->pages.e[newpg]->thumbmodtime = 0;
			spreads.e[c]->spread->pagestack.e[c2]->index = newpg;
		}
	}

	for (int c=0; c<n; c++) temppagemap[c] = c;

	doc->pages.insertArrays(newpages,newlocal,n);
	delete[] oldlocal;
	delete[] oldpages;

	doc->SyncPages(0,-1, true);
	return 0;
}

void SpreadView::Reset()
{
	int c,c2;
	for (c=0; c<temppagemap.n; c++) {
		if (temppagemap[c]==c) continue;
		for (c2=c+1; c2<temppagemap.n; c2++)
			if (temppagemap[c2]==c) break;
		if (c2==temppagemap.n) continue;
		SwapPages(c,c2);
	}
}


} // namespace Laidout

