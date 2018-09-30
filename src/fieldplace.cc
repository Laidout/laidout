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

//#include <cctype>
#include <cstdlib>
#include <cstdarg>
#include "fieldplace.h"

#include <lax/strmanip.h>
#include <lax/lists.cc>

using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG


using namespace Laxkit;

namespace Laidout {


//----------------------------- FieldExtPlace -----------------------------
/*! \class FieldExtPlace
 * \brief Stack of field places. So "blah.2.7" would translate into a stack with elements blah, 2, and 7,
 * 
 * You may push strings or integers. Integers are assumed to be indices, so <0 means index not used,
 * and you are supposed to use the string instead.
 */

FieldExtPlace::ExtNode::ExtNode(const char *str, int i)
{
	ext=newstr(str);
	exti=i;
}


FieldExtPlace::ExtNode::~ExtNode()
{
	if (ext) delete[] ext;
}

//! Parse a simple string. No function calls, must be plain strings and numbers. No whitespace.
FieldExtPlace::FieldExtPlace(const char *str, int len)
{
	Set(str,len,NULL);
}

//! Append extensions. Returns number of fields added.
int FieldExtPlace::Set(const char *str, int len, const char **next)
{
	if (len<0) len=strlen(str);

	int nf=0;
	unsigned int n=0; //length of string to check in str
	char *ss;

	while (len>0 && str && *str) {
		while (isalnum(str[n]) || str[n]=='_') n++;
		if (n==0) break;

		 // check for number first: "34.blah.blah"
		if (isdigit(str[0])) {
			char *nxt=NULL;
			int nn=strtol(str,&nxt,10); 
			if (nxt-str==n) {
				 //found valid number
				if (nn>=0) {
					push(nn); nf++;
					str+=n;
					len-=n;
					if (*str=='.') str++;
					continue;
				}
			}
		}

		ss=newnstr(str,n);
		push(ss); nf++;
		delete[] ss;

		str+=n;
		len-=n;
		if (*str=='.') str++;
	}

	if (next) *next=str;
	return nf;
}

FieldExtPlace::FieldExtPlace(const FieldExtPlace &place)
{
	int i;
	char *str;
	for (int c=0; c<place.n(); c++) {
		str=place.e(c,&i);
		if (str) push(str);
		else push(i);
	}
}
	
//! Assignment operator. Flushes, and copies over place's stuff.
FieldExtPlace &FieldExtPlace::operator=(const FieldExtPlace &place)
{
	flush();
	int i;
	char *str;
	for (int c=0; c<place.n(); c++) {
		str=place.e(c,&i);
		if (str) push(str);
		else push(i);
	}
	return *this;
}

//! Return whether place specifies the same location as this.
int FieldExtPlace::operator==(const FieldExtPlace &place) const
{
	if (n()!=place.n()) return 0;
	int i;
	char *str;
	for (int c=0; c<ext.n; c++) {
		str=place.e(c, &i);
		if (i!=ext.e[c]->exti || strcmp(ext.e[c]->ext,str)) return 0;
	}
	return 1;
}

//! Return the string and integer of element i (if ei!=NULL).
char *FieldExtPlace::e(int i, int *ei) const
{
	if (i<0 && i>=ext.n) { if (ei) *ei=-1; return NULL; }
	if (ei) *ei=ext.e[i]->exti;
	return ext.e[i]->ext;
}

//! Set field i. Returns 0 for success, or -1 for out of range.
/*! Return for success, or -1 for out of bounds.
 */
int FieldExtPlace::e(int i,const char *val, int ei)
{
	if (i<0 || i>=ext.n) return -1;

	makestr(ext.e[i]->ext,val);
	ext.e[i]->exti=ei;
	return 0;
}

int FieldExtPlace::push(const char *nd,int where)
{
	return ext.push(new ExtNode(nd,-1), 1, where);
}

int FieldExtPlace::push(int i,int where)
{
	return ext.push(new ExtNode(NULL,i), 1, where);
}

/*! Remember the returned string must be delete[]'d.
 */
char *FieldExtPlace::pop(int *i, int which)
{
	ExtNode *node=ext.pop(which);
	if (!node) { if (i) *i=-1; return NULL; }

	if (i) *i=node->exti;
	char *str=node->ext;
	node->ext=NULL;
	return str;
}

//! Remove element which. Return 0 for succes, or -1 for which out of bounds. A negative which means remove the top element.
int FieldExtPlace::remove(int which)
{
	if (which<0) which=ext.n;
	if (which>=ext.n) return -1;
	ext.remove(which);
	return 0;
}

//! For debugging, dumps to stdout.
void FieldExtPlace::out(const char *str)
{
	//DBG if (str) cerr <<str<<": "; else cerr <<"FieldExtPlace: ";
	//DBG for (int c=0; c<ext.n; c++) {
	//DBG 	if (ext.e[c]->ext) cerr <<ext.e[c]->ext;
	//DBG 	else cerr <<ext.e[c]->exti;
	//DBG 	cerr<<".";
	//DBG }
	//DBG cerr <<endl;
}


//----------------------------- FieldPlace -----------------------------
/*! \class FieldPlace
 * \brief Stack of field places. So "3.2.7" would translate into a stack with elements 3, 2, and 7.
 *
 * Any invalid place is signified by -1, so if you try to get the value of a place that does
 * not exist, -1 is returned.
 *
 * FieldMask is stack of these objects.
 */
/*! \fn const int *FieldPlace::list()
 * \brief Return a const int list of the place's elements. There are n() of them.
 */


FieldPlace::FieldPlace(const FieldPlace &place)
{
	if (place.NumStack<int>::e) {
		NumStack<int>::n=place.NumStack<int>::n;
		max=NumStack<int>::n+delta;
		NumStack<int>::e=new int[max];
		memcpy(NumStack<int>::e,place.NumStack<int>::e,NumStack<int>::n*sizeof(int));
	}
}
	
//! For debugging, dumps to stdout.
void FieldPlace::out(const char *str)
{
	//DBG if (str) cerr <<str<<": "; else cerr <<"FieldPlace: ";
	//DBG for (int c=0; c<NumStack<int>::n; c++) cerr <<NumStack<int>::e[c]<<", ";
	//DBG cerr <<endl;
}

//! Return whether place specifies the same location as this.
int FieldPlace::operator==(const FieldPlace &place) const
{
	if (n()!=place.n()) return 0;
	for (int c=0; c<NumStack<int>::n; c++) if (e(c)!=place.e(c)) return 0;
	return 1;
}

//! Assignment operator. Flushes, and copies over place's stuff.
FieldPlace &FieldPlace::operator=(const FieldPlace &place)
{
	flush();
	if (place.NumStack<int>::e) {
		NumStack<int>::n=place.NumStack<int>::n;
		max=NumStack<int>::n+delta;
		NumStack<int>::e=new int[max];
		memcpy(NumStack<int>::e,place.NumStack<int>::e,NumStack<int>::n*sizeof(int));
	}
	return *this;
}

//----------------------------- FieldMask -----------------------------

/*! \class FieldMask
 * \ingroup stylesandstyledefs
 * \brief This class holds a list of field specifications for Styles.
 *
 * Each element is a FieldPlace, each of which holds a different
 * field specification, with indices starting at 0. For instance an extension
 * "4.5.6" would translate into an element {4,5,6,-1}.
 *
 * The main motivation for a whole class to specify fields is to make it easy to keep
 * track of what fields and subfields change when a particular field is modified. 
 * Certain dialogs of Styles need that kind of feedback.
 *
 * Please note that the default Laxkit::PtrStack::push function is not redefined here. When
 * pushing a FieldPlace, the pointer is transfered (as opposed to copying the contents).
 */


//! Constructor with all numbers, comma separated field specs, ext is like: "34.234.6.7,3.4"
/*! No whitespace allowed. Adds fields until no more in ext, or until the current field spec
 * is somehow invalid. If it is invalid, the previous valid ones remain pushed, but no new ones
 * are added.
 */
FieldMask::FieldMask(const char *ext)
	: PtrStack<FieldPlace>(2)
{
	const char *start=ext,*end=ext;
	if (!start) return;
	FieldPlace *fp;
	int *fe,c;
	while (*start) {
		fp=new FieldPlace;
		fe=chartoint(start,&end);
		if (*end!=',' && *end!='\0') {
			//DBG cerr << "ext had invalid characters!"<<endl;
			if (fe) delete[] fe;
			if (fp) delete fp;
			return;
		}
		for (c=0; fe[c]!=-1; c++) fp->push(fe[c]);
		PtrStack<FieldPlace>::push(fp,1);
		if (*end==',') start=end+1; else start=end;
	}
}

//! Do nothing default constructor.
FieldMask::FieldMask() : PtrStack<FieldPlace>(2)  {}

//! Copy constructor.
/*! This duplicates the code in operator=(). ***Is it ok to just remove the const?
 * so to have *this=mask;
 */
FieldMask::FieldMask(const FieldMask &mask)
	: PtrStack<FieldPlace>(2)
{
	if (!mask.n) return;
	max=n=mask.n;
	e=new FieldPlace*[mask.n];
	islocal=new char[mask.n];
	for (int c=0; c<n; c++) {
		islocal[c]=1;
		e[c]=new FieldPlace;
		*e[c]=*mask.e[c];
	}
}

//! Equal operator, makes whole copy of the stack
/*! islocal is always {1,1,1...}.
 * Makes copies of everything in mask.e
 */
FieldMask &FieldMask::operator=(FieldMask &mask)
{
	flush();
	max=n=mask.n;
	e=new FieldPlace*[mask.n];
	islocal=new char[mask.n];
	for (int c=0; c<n; c++) {
		islocal[c]=1;
		e[c]=new FieldPlace;
		*e[c]=*mask.e[c];
	}
	return mask;
}

//! Equal operator, makes whole copy of the FieldPlace.
FieldMask &FieldMask::operator=(FieldPlace &place)
{
	flush();
	max=n=1;
	e=new FieldPlace*[1];

	islocal=new char[1];
	islocal[0]=1;
	e[0]=new FieldPlace;
	*e[0]=place;
	return *this;
}


//! Shortcut to assign a single level field to *this.
/*! This flushes the mask, and adds {f,-1} as the first element.
 */
FieldMask &FieldMask::operator=(int f) 
{
	flush();
	push(-1,1,f);
	return *this;
}

//! Check for equality with another field mask.
/*! Currently, the order and values have to correspond exactly.
 */
int FieldMask::operator==(const FieldMask &mask)
{
	if (mask.n!=n) return 0;
	for (int c=0; c<n; c++) {
		if (!(e[c]==mask.e[c])) return 0;
	}
	return 1;
}

//! Shortcut to check for a single, top field, such as stack with only {5,-1}, what==5 returns 1.
/*! For checking to see if there are ANY fields defined, use "!fieldmask" or fieldmask.howmany()
 */
int FieldMask::operator==(int what) 
{
	if (n!=1) return 0;
	return e[0]->e(0)==what && e[0]->n()==1;
}

//! Check for existence of fields defined in fieldmask.
/*! This just returns fieldmask.howmany()
 */
int operator!(FieldMask &fieldmask)
{
	return fieldmask.howmany(); 
}

//! This sets the value of stacked field f, element where of f.
/*! If such an element does not exist, -1 is returned, else val is returned.
 */
int FieldMask::value(int f,int where,int val)
{
	if (!n || f<0 || f>=n || where<0 || where>=e[f]->n()) return -1;
	return e[f]->e(where,val);
}

//! This returns the value of stacked field f, element i of f.
/*!  That is, say the stack is { 2,4,6,8,-1}, {1,7,-1}, then
 * value(0,0)==2, value(0,3)==8, value(1,1)==7. (f and i both start at 0).
 * If such an element does not exist, -1 is returned.
 */
int FieldMask::value(int f,int where)
{
	if (!n || f<0 || f>=n || where<0 || where>=e[f]->n()) return -1;
	return e[f]->e(where);
}

//! Returns whether ext corresponds to any in mask
/*! ext must be all numbers like: "3.54.56.2"
 * 
 *  Returns:\n
 *   0=no\n
 *   1=yes, exact\n
 *   2=yes, partial (subset),   such as ext=4.5, and 4.5.6 is in the mask\n
 *   3=yes, partial (superset), such as ext=4.5.6, and 4.5 is in the mask\n
 *
 * 	If howdeepismatch is not NULL, then the number of matched fields is put in
 * 	it. Say ext="4.5.6", an exact match puts 3. If a superset match occurs, say "4.5",
 * 	then 2 is set. If a subset match occurs, say "4.5.6.7", then 3 is set.
 * 	If there is an error with ext, then howdeepismatch is not altered, and 0 is returned.
 *
 *  In the fieldmask, there might be duplicates, and a combination of 
 *  subset/superset/exact matches. Search the whole fieldmask for an exact match. 
 *  If no exact match found, return the nearest
 *  superset, else return the nearest subset.
 */
int FieldMask::has(const char *ext,int *howdeepismatch) //index=NULL
{
	if (!ext || !n) return 0;
	const char *start=ext,*end=ext;
	int numinext,       // number of indices in ext
		subnear=1000,   // how close is a subset
		supernear=1000, // how close is a superset
		extissubset=-1, // index in e[] of subset match
		extissuper=-1;  // index in e[] of superset match
	int //ii=0, // the index in e
		//i,   // an individual number extracted from ext
		y=0, // y is how deep the match goes so far
		ee=0; // the stacked field spec to check against
		
	int *exti=chartoint(start,&end);
	if (!exti || *end) return 0; // there was a problem with ext
	numinext=0;
	while (exti[numinext]!=-1) numinext++;

	 // check exti against stuff in e[]
	for (ee=0; ee<n; ee++) {
		y=0;
		 // find extent of match
		while (e[ee]->e(y)==exti[y] && y<e[ee]->n() && exti[y]!=-1) y++;
		if (y==0) continue; // no match yet
		if (e[ee]->e(y)==exti[y] && exti[y]==-1) { // exact match
			if (howdeepismatch) *howdeepismatch=numinext;
			return 1;
		}
		if (exti[y]==-1) { // is subset
			while (y<e[ee]->n()) y++;
			if (y-numinext<subnear) {
				subnear=y-numinext;
				extissubset=ee;
			}
		} else if (y<e[ee]->n()) { // is superset
			if (numinext-y < supernear) {
				supernear=numinext-y;
				extissuper=ee;
			}
		} else continue; // is incompatible match, such as "1.2.3" and "1.2.4"
	}
	if (extissuper>=0) { // match is superset
		if (howdeepismatch) *howdeepismatch=numinext-supernear;
		return 3;
	}
	if (extissubset>=0) { // match is subset
		if (howdeepismatch) *howdeepismatch=numinext;
		return 2;
	}
	if (howdeepismatch) *howdeepismatch=0;
	return 0; // else no match
}

//! Convert a "1.2.3" to an int[]={1,2,3,-1}. Single spec, not comma separated specs.
/*! ext is checked to ensure that it has a sequence:  "digit* [ '.' digit* ]*".
 * If not, then NULL is returned. 
 *
 * If next_ret is not NULL, then it is set to the first char that 
 * doesn't meet the above sequence.
 * Note that "1.2.3abcd" is ok, {1,2,3,-1} is
 * returned, and next_ret is set to "abcd".
 *
 * *** this doesn't have to be FieldMask function
 *
 * *** could make this chartoint(cchar *ext, int**list, int *bufsize, cchar **next_ret)
 * which reallocates list if necessary.
 */
int *FieldMask::chartoint(const char *ext,const char **next_ret)
{
	const char *start=ext,*end=ext,*realend;
	if (!start) return NULL;
	
	 // make sure ext is:   digit* [ '.' digit* ]* '\0'
	int numinext=0;
	while (*start) {
		while (isdigit(*end)) end++;
		if (end==ext || end-start==0) { // bad ext, or no number when one was expected
			if (next_ret) *next_ret=ext; 
			return NULL; 
		}
		numinext++;
		if (*end!='.') { // there was a number not followed by '.'
			if (next_ret) *next_ret=end;
			realend=end;
			break;
		}
		start=end;
		if (*start) start++; // start must have been '.'
	}
	 // now [ext,realend) must have form "1.2.3"
	start=ext;
	int *fe=new int[numinext+1],
		i,c=0;
	while (start!=realend) {
		char *theend;
		i=strtol(start,&theend, 10);
		fe[c++]=i;
		start+=theend-start;
		if (*start=='.') start++; // otherwise start must be realend
	}
	fe[c]=-1;
	return fe;
}

//! Push a field with ne elements at position where (-1==at end) in overall mask. Returns 0=success, 1=error
/*! Shortcut for pushing single fields. Creates {a1,a2,...,-1} and pushes it.
 *
 * Returns whatever PtrStack::push returns.
 */
int FieldMask::push(int where,int ne, ...)
{
	va_list ap;
	va_start(ap,ne);
	int c;
	FieldPlace *fp=new FieldPlace;
	for (c=0; c<ne; c++) { fp->push(va_arg(ap,int)); }
	va_end(ap);
	return PtrStack<FieldPlace>::push(fp,1,where);
}

//! Push a list of integers onto stack. Copies the list, does not use original.
/*! Where is which field to put it in. The list is a whole field, that is it
 * occupies one place in FieldMask::e.
 *
 * The list has either n values, or if n==0, then the list must be terminated
 * with a -1.
 *
 * Warning: Do not get this push(int,int*,int) mixed up with the push(int*,char,int) inherited
 * from PtrStack!
 *
 * Returns the number of elements on the stack (or whatever Laxkit::PtrStack::push() returns!).
 */
int FieldMask::push(int n,int *list,int where)//where=-1
{
	if (n==0) while (list[n]!=-1) n++;
	FieldPlace *fp=new FieldPlace;
	int c;
	for (c=0; c<n; c++) fp->push(list[c]);
	return PtrStack<FieldPlace>::push(fp,1,where);
}


} // namespace Laidout

