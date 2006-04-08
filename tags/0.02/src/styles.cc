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
/****** styles.cc ************/

#include <cctype>
#include <cstdlib>
#include <lax/strmanip.h>
//#include <value.h>
#include "styles.h"
#include <lax/lists.cc>
#include <cstdarg>

#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;
using namespace LaxFiles;



/*! \defgroup stylesandstyledefs Styles and StyleDefs
 * 
 * -------- Styles and StyleDefs -----------
 * 
 *  there are Style's and StyleDef's. StyleDef has a map of the names
 *  of the style and of its fields. Style has actual instances of the 
 *  various fields. Also, each Style can be based on some other Style. 
 *  
 *  For example you might have a character style \n
 *    c1 = italic/bookman/10pt/red/etc., basedon=NULL\n
 *  The StyleDef has field names (say) ItalicFlag/FaceName/Size/Color/etc.
 *  Then say you want a style c2 exactly like c1, but bold too, then the only 
 *  data actually stored in c2 is the BoldFlag, a pointer to c1, and a pointer
 *  to the StyleDef.
 *
 */



//NOTES:
// definetype polyhedron { v=set of vector, f=set of set of int, e=set of array[2]int }
// names: v={vector},f={dummy1},e={array[2]int},  dummy1=set of int 
//
// paragraph:
// 	first indent
// 	before lead
// 	after lead
// 	middle lead
// 	left indent
// 	right indent
//  

//----------------------------- FieldPlace -----------------------------
/*! \class FieldPlace
 * \brief Stack of field places. So "3.2.7" would translate into a stack with elements 3, 2, and 7.
 *
 * Any invalid place is signified by -1, so if you try to get the value of a place that does
 * not exist, -1 is returned.
 *
 * FieldMask is stack of these objects.
 */
//class FieldPlace : protected Laxkit::NumStack<int>
//{
// public:
//	virtual int n() const { return Laxkit::NumStack<int>::n; }
//	virtual int e(int i) const { if (i>=0 && i<Laxkit::NumStack<int>::n) 
//		return Laxkit::NumStack<int>::e[i];  return -1; }
//	virtual int e(int i,int val) { if (i>=0 && i<Laxkit::NumStack<int>::n) 
//		{ return Laxkit::NumStack<int>::e[i]=val; }  return -1; }
//	virtual int operator==(const FieldPlace &place) const;
//	virtual FieldPlace &operator=(const FieldPlace &place);
//	virtual ~FieldPlace() {}
//	virtual int push(int nd,int where=-3) { return Laxkit::NumStack<int>::push(nd,where); }
//	virtual int pop(int which=-3) { return Laxkit::NumStack<int>::pop(which); }
//	virtual void out(const char *str);
//};

//! For debugging, dumps to stdout.
void FieldPlace::out(const char *str)
{
	if (str) cout <<str<<": "; else cout <<"FieldPlace: ";
	for (int c=0; c<NumStack<int>::n; c++) cout <<NumStack<int>::e[c]<<", ";
	cout <<endl;
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
//class FieldMask : public PtrStack<FieldPlace>
//{
// protected:
//	int *chartoint(const char *ext,const char **next_ret);
// public:
//	FieldMask();
//	FieldMask(const char *ext);
//	FieldMask(const FieldMask &mask);
//	FieldMask &operator=(FieldMask &mask);
//	FieldMask &operator=(FieldPlace &place);
//	virtual int operator==(int what);
//	virtual int operator==(const FieldMask &mask);
//	virtual FieldMask &operator=(int f); // shorthand for single level fields
//	//virtual int push(FieldMask &f);
//	virtual int push(int where,int n, ...);
//	virtual int push(int n,int *list,int where=-1);
//	virtual int has(const char *ext,int *index=NULL); // does ext correspond to any in mask?
//	virtual int value(int f,int where);
//	virtual int value(int f,int where,int newval);
//};

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
			cout << "ext had invalid characters!"<<endl;
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


//-----------------------------------------------------

//-------------------------- StyleDefinition/StyleManager -------------------------------
//! A manager class to simplify addition of Styles and creation/deletion/duplication of Style instances.
class StyleDefinition
{
 public:
	char *name;
	Style *newStyleFunc();
	StyleDef *getStyleDef(int idtobe); // called usually just the first time
	int deleteStyle(Style *style);
};


// Generally StyleDefs and Styles would be stored in some StyleManager, as
// care must be taken when deleting or reassigning Styles or StyleDefs.
//
//class StyleManager
//{
// protected:
//	
// public:
//	int RegisterStyle(StyleDef *def,StyleCreateFunc func);
//	void deleteStyle(Style *style);
//	Style *newStyle(const char *stylename);
//	StyleDef *findStyleDef(const char *stylename);
//};
//


//------------------------------ StyleDef --------------------------------------------

/*! \class StyleDef
 * \ingroup stylesandstyledefs
 * \brief The definition of the elements of a Style.
 *
 * \todo *** haven't worked on this code in quite a while, perhaps it could be somehow
 * combined with the Laxkit::Attribute type of thing? but for that, still need to reason
 * out a consistent Attribute definition standard...
 * 
 * These things are used to autmatically create edit dialogs for any data. It also aids in
 * defining names and what they are for an interpreter. ***eventually include accepted functions, not
 * just variables??
 *
 * Currently, this stuff is an internationalization nightmare (english only right now).. would have to integrate with
 * gettext somehow (gotta read up on that!!!)
 * 
 *  example: 
 *  <pre>
 *   name        = spacevector    <-- name as it appears in an interpreter 
 *   Name        = Space Vector   <-- name as it appears as a dialog label
 *   tooltip     = spacevector    <-- tooltip 
 *   description = A three dimensional vector  <-- Longer description for some in-program help system
 *    3 subfields:
 *    x,y,z all real
 * 
 *   Might be displayed like this:
 *   Space Vector: x= _12__  y= __1__  z= ___0_
 * 
 *   The name would be used in some command line interpreter.
 *   The Name would be used as shown in an edit dialog.
 *   The description would be used in some sort of help system
 *  </pre>
 */
/*! \var int StyleDef::format
 * \brief What is the nature of *this.
 * 
 *   The format of the value of this Styledef. It can be any id that is defined in the value Manager***.
 *   These formats other than FIELDS imply that *this is a single unit. FIELDS implies that there are further
 *   subunits. It may be that the type of value it is has further subfields, but they lay outside the
 *   scope of StyleDef.
 *   <pre>
 *    STYLEDEF_INT      1
 *    STYLEDEF_REAL     2
 *    STYLEDEF_STRING   3
 *    STYLEDEF_FIELDS   4 <-- implies fields holds a stack of the subfields for *this
 *    STYLEDEF_BIT      5 <-- has 2 possible states
 *    STYLEDEF_3BIT     6 <-- has 3 possible states (-1,0,1 or yes,no,maybe for instance)
 *    STYLEDEF_ENUM     7 <-- implies that fields exists, and contains only ENUM_VALs
 *    STYLEDEF_ENUM_VAL 8 <-- is not actually a value of anything. The ENUM can be only one of its ENUM_VALs
 *    -STYLEDEF_VALUE- (starting at 100) <-- is a number type used in the built-in interpreter
 *    </pre>
 */
/*! \var char *StyleDef::extends
 *  \brief Name of which StyleDef this one extends.
 *  
 *  If there is an extends, the index of all the fields starts with 0, which
 *	is the very first field in the very first base styledef.
 */
/*! \var StyleDef *StyleDef::extendsdef
 *  \brief Which StyleDef this one extends.
 *  
 *	This is a pointer to a def in a StyleManager. ***StyleDef should look up extends in
 *	the stylemanager to get the appropriate reference.
 *  
 *  If there is an extends, the index of all the fields starts with 0, which
 *	is the very first field in the very first base styledef.
 */
//class StyleDef : public Laxkit::anObject
//{
// public:
//	char *extends;
//	StyleDef *extendsdef;
//	char *name; //name for interpreter
//	char *Name; // Name for dialog label
//	char *tooltip; // short description
//	char *description; // long description, this would be displayed on a help page, for instance.
//	
//	 // STYLEDEF_ORIGINAL
//	 // STYLEDEF_DUPLICATE
//	 // STYLEDEF_ORPHAN  =  is a representation of a composite style, not stored in any manager, and only 1 reference to it exists
//	 // STYLEDEF_CAPPED = cannot push/pop fields
//	 // STYLEDEF_READONLY = cannot modify parts of the styledef
//	unsigned int flags;
//
//	int format; // int,real,string,fields,...
//	PtrStack<StyleDef> *fields; //might be NULL, any fields are assumed to not be local to the stack.
//	
//	StyleDef();
//	StyleDef(const char *nextends,const char *nname,const char *nName,const char *ttip,
//			const char *ndesc,int fmt, Laxkit::PtrStack<StyleDef> *nfields=NULL,unsigned int fflags=STYLEDEF_CAPPED);
//	virtual ~StyleDef();
//	
//	 // helpers to locate fields by name, "blah.3.x"
//	virtual int getNumFields();
//	virtual int findfield(char *fname,char **next); // return index value of fname. assumed top level field
//	virtual int findDef(int index,StyleDef **def);
//	virtual int getInfo(int index,const char **nm=NULL,const char **Nm=NULL,const char **tt=NULL,const char **desc=NULL);
// 	//int *getfields(const char *extstr); // returns 0 terminated list of indices: "1.4.23+ -> { 1,4,23,0 }
//
//	 //-------- StyleDef creation helper functions ------
//	 // The following (push/pop/cap) are convenience functions 
//	 // to initially construct a styledef on the fly
//	virtual int push(const char *nname,const char *nName,const char *ttip,const char *ndesc,int fformat,unsigned int fflags);
//	virtual int push(const char *nname,const char *nName,const char *ttip,const char *ndesc,
//						int fformat,Laxkit::PtrStack<StyleDef> *nfields,unsigned int fflags);
//	virtual int push(StyleDef *newfield);
//	virtual int pop(int fieldindex);
//	 // cap prevents accidental further adding/removing fields to a styledef
//	 // that is being constructed
//	virtual void cap(int y=1) { if (y) flags|=STYLEDEF_CAPPED; else flags&=~STYLEDEF_CAPPED; }
//};


//! Constructor.
StyleDef::StyleDef(const char *nextends, //!< Which StyleDef does this one extend
			const char *nname, //!< The name that would be used in the interpreter
			const char *nName, //!< A basic title, most likely an input label
			const char *ttip, //!< Tooltip text
			const char *ndesc, //!< Long description, newlines ok.
			int fmt, //!< Format of this StyleDef
			Laxkit::PtrStack<StyleDef> *nfields, //!< StyleDef for the subfields or enum values.
			unsigned int fflags) // <- these 2 default=0
{
	extendsdef=NULL; //***** must look up extends to set extendsdef
	extends=name=Name=tooltip=description=NULL;
	makestr(extends,nextends);
	makestr(name,nname);
	makestr(Name,nName);
	makestr(tooltip,ttip);
	makestr(description,ndesc);
	fields=nfields;
	format=fmt;
	flags=fflags;
}

StyleDef::~StyleDef()
{
	if (extends) delete[] extends;
	if (name) delete[] name;
	if (Name) delete[] Name;
	if (tooltip) delete[] tooltip;
	if (description) delete[] description;
}

//! Push def without fields. If pushing this new field onto fields fails, return 1, else 0
int StyleDef::push(const char *nname,const char *nName,const char *ttip,const char *ndesc,int fformat,unsigned int fflags)
{
	StyleDef *newdef=new StyleDef(NULL,nname,nName,ttip,ndesc,fformat,NULL,fflags);
	if (push(newdef)) { delete newdef; return 1; }
	return 0;
}

//! Push def with fields. If pushing this new field onto fields fails, return 1, else 0
int StyleDef::push(const char *nname,const char *nName,const char *ttip,const char *ndesc,int fformat,
		Laxkit::PtrStack<StyleDef> *nfields,unsigned int fflags)
{
	StyleDef *newdef=new StyleDef(NULL,nname,nName,ttip,ndesc,fformat,nfields,fflags);
	if (push(newdef)) { delete newdef; return 1; }
	return 0;
}

//! Push newfield onto fields.
/*! Returns whatever PtrStack::push returns.
 */
int StyleDef::push(StyleDef *newfield)
{
	if (!newfield) return 1;
	if (!fields) fields=new PtrStack<StyleDef>;
	return fields->push(newfield);
}

//! Special care must be taken when popping to ensure Style/StyleDef pointer integrity!
/*! This function calls remove on fieldindex in fields.
 * Returns whatever PtrStack::remove returns.
 */
int StyleDef::pop(int fieldindex)
{
	if (!fields || !fields->n || fieldindex<0 || fieldindex>=fields->n) return 0;
	return fields->remove(fieldindex);
}


 //! Returns the number of upper most fields that this styledef contains.
 /*! If this styledef is an extension of another, then the number returned is
  * the total number of fields defined in *this plus all the upper fields in
  * the extended styledef(s). Each styledef simply adds fields->n if fields
  * exists, or 0 if fields does not exists and extendsdef exists, or 1 if neither fields 
  * nor extendsdef exist. 
  *
  * A special exception is when format==STYLEDEF_ENUM. In that case, fields would
  * contain the possible enum values, but the enum as a whole acts like a single
  * number, so 1 is added, rather than fields->n. If the enum is an extension of
  * some other enum, then this styledef adds 0 to the count.
  * 
  * Consider a StyleDef vector(x,y,z), Now say you define a StyleDef called zenith
  * which is a vector with x,y,z. You want the normal x,y,z definitions, but the
  * head you want to now be "zenith" rather than "vector", So you extend "vector",
  * and do not define any new fields. Viola!
  *
  */
int StyleDef::getNumFields()
{
	int n=0;

	if (format==STYLEDEF_ENUM) {
		if (!extendsdef) n++;
	} else {
		if (fields && fields->n) n+=fields->n; 
			else if (!extendsdef) n++;
		if (extendsdef) n+=extendsdef->getNumFields();
	}
	return n; 
}

//! Return const char pointers to the various texts.
/*! Given the index of a desired field, this looks up which StyleDef actually has the text
 * and sets the pointers to point to there. Nothing is done if the particular pointer is NULL.
 *
 * If index==-1, then the info from *this is provided, rather than from a fields stack.
 *
 * Returns 0 success, 1 error.
 */
int StyleDef::getInfo(int index,const char **nm,const char **Nm,const char **tt,const char **desc)
{
	StyleDef *def=NULL;
	index=findDef(index,&def);
	if (!def) return 1; // otherwise index should be a valid value in fields, or refer to this
	if (index==-1) {
		if (nm) *nm=def->name;
		if (Nm) *Nm=def->Name;
		if (tt) *tt=def->tooltip;
		if (desc) *desc=def->description;
		return 0;
	}
	if (nm) *nm=def->fields->e[index]->name;
	if (Nm) *Nm=def->fields->e[index]->Name;
	if (tt) *tt=def->fields->e[index]->tooltip;
	if (desc) *desc=def->fields->e[index]->description;
	return 0;
}

//! Find a valid StyleDef and return a valid index within it corresponding to the given overall index.
/*! Does not consider subfields, only top level fields.
 *
 * enum: i=0
 * 	extended enum: i=0, but use this
 * 	no fields, return error
 */ 	
int StyleDef::findDef(int index,StyleDef **def)
{
	if (index<0) { *def=NULL; return -1; }
	 // if enum, or this is singe unit (not extending anything): index must be 0
	if (index==0 && (format==STYLEDEF_ENUM || (!extendsdef && (!fields || !fields->n)))) {
		*def=this;
		return -1;
	}
	 // else there should be fields somewhere
	int n=0;
	if (extendsdef) {
		n=extendsdef->getNumFields();
		if (index<n) { // index lies in extendsdef somewhere
			return extendsdef->findDef(index,def);
		} 	
	}
	
	 // index puts it in *this.
	index-=n;
	 // index>=0 at this point implies that this has fields, assuming original index is valid
	if (!fields || !fields->n || index<0 || index>=fields->n)  { //error
		*def=NULL;
		return -1;
	}
	*def=this;
	return index;
}

//! Find the index of the field named fname. Doesn't look in subfields.
/*! Compares the chars in fname up to the first '.'. It assumes that fname
 *  does not contain whitespace or start with a '.'. 
 *  Updates next to point to char after the first '.', unless field is not found
 *  in which case next is set to fname, and -1 is returned.
 *  On success, the field index is returned (field index starting at 0). 
 *  If the field string is a number (ie "34.2...") the number is parsed
 *  and returned without bounds checking (though it must be non-negative). This 
 *  number passing is to accommodate sets with variable numbers of
 *  elements, where the number of elements is held in Style, not StyleDef.
 *
 *  If fname has something like "34ddfdsa.33" then -1 is returned and 
 *  next is set to fname changed. Field names must be all digits, or [a-zA-Z0-9-_] and
 *  start with a letter.
 */
int StyleDef::findfield(char *fname,char **next) // next=NULL
{
	int n;
	if (extendsdef) {
		n=extendsdef->findfield(fname,next);
		if (n>=0) return n;
	}
		
	char *nxt;
	 // make n the index of the first '.' or end of string
	n=strchr(fname,'.')-fname;
	if (n==0) n=strlen(fname);

	 // check for number first: "34.blah.blah"
	if (isdigit(fname[0])) {
		int nn=strtol(fname,&nxt,10); 
		if (nxt!=fname) {
			if (nxt-fname!=n) { if (next) *next=fname; return -1; } // was "34asdfasd" or some such bad input
			if (next) *next=fname + n + (fname[n]=='.'?1:0);
			if (nn>0) return nn;
		}
	} else if (fields) { // else check for field: "blah.3.blah..."
		for (int c=0; c<fields->n; c++) {
			if (!strncmp(fname,fields->e[c]->name,n) && fields->e[c]->name[n]=='\0') {
				if (next) *next=fname + n + (fname[n]=='.'?1:0);
				if (extendsdef) return extendsdef->getNumFields()+c;
				return c;
			}
		}
	}
	return -1;
}

////**** return FieldMask instead?
////! returns 0 terminated list of indices: "1.4.23" -> { 1,4,23,-1 }
///*! in form of new int[n+1], n=number of subfields
// *  or NULL on error. Index numbers are transferred without modification,
// *  while field names must correspond to a name in the StyleDef.
// *  *** perhaps this should be in Value to allow Value to interperet
// *  the passed in field names. Having separate getfields here is unnecessary?
// *  .. thinking of bitfields where there is a whole slew of names, but they
// *  all control the same (internal) int, plus the mechanism to return a mask
// *  of what changes when you set a value..
// * 
// *  end_ptr is changed to point to NULL on success, or the first invalid
// *  or unparsable field section.
// */  
//int *StyleDef::getfields(char *extstr,char **end_ptr)
//{***
//	int n=1;
//	char *str=extstr;
//	while (*str) { 
//		if (*str=='.') n++; 
//		str++;
//	}
//	int *list=new int[n+1];
//
//	***
//}


//! For debugging, print out the style def to stdout
void dumpstyledef(StyleDef *sd, int i)
{
	char ii[i+1];
	memset(ii,' ',i);
	ii[i]='\0';
	cout <<ii<<"StyleDef: "<<(sd->name?sd->name:"no interpreter name")<<", "<<(sd->Name?sd->Name:"no Name")<<endl;
	cout <<ii<<(sd->tooltip?sd->tooltip:"no tooltip")<<endl;
	cout <<ii<<(sd->description?sd->description:"No description")<<endl;
	cout <<ii<<(sd->extends?sd->extends:"no extends str");
	if (sd->extendsdef) dumpstyledef(sd->extendsdef,i+2);
	else cout <<ii<<"No extendsdef"<<endl;
	cout <<ii<<"flags:"<<sd->flags<<"  format:"<<sd->format<<endl;
	if (!sd->fields) cout <<ii<<"No fields"<<endl;
	else {
		cout <<ii<<"Fields:"<<endl;
		for (int c=0; c<sd->fields->n; c++) dumpstyledef(sd->fields->e[c],i+2);
	}
	cout <<ii<<"---done dumpstyledef"<<endl;
}


//--------------------------------------------------------------------------------


//------------------------- Style ------------------------------------


//------------------------------- FieldNode -----------
	
/*! \class FieldNode 
 * \ingroup stylesandstyledefs
 * \brief An internal stack node for Style::fields.
 * 
 * \todo *** this class is a little clunky, need to think more about a good system
 * for interacting with the values of Style objects. such a system must make 
 * interpreter integration a snap, be it my own interpreter or potential python/perl/whatever
 * add ons...
 * 
 * This class is used to wrap which fields a given Style instance has in the
 * form of a union with the typical formats unsigned int, enum (stored as int), int, 
 * bit (stored as int), 3bit (stored as int),
 * double, string, fields, Style *, Value * ***include Value yes or no, or other (void *).
 * Any "other" formats must be deciphered some other way.
 *
 * Extraneous FieldNodes such as are returned by Style should be deleted with
 * deleteFieldNode(), which checks to make sure that the node is not held by
 * something else before deleting. If it is held elsewhere, the pointer is simply
 * set to NULL, and nothing is deleted. If it is not held elsewhere, the FieldNode is
 * deleted. In that case, if format is fields, the value.fields are recursively deleted 
 * with deleteFieldNode(). value.str, v, and other are assumed to be held by something
 * else, and are never deleted.*****??have a check for that?
 */
/*! \var int FieldNode::index
 * \brief The index number of the value.
 *
 * ***Please note that for variable sized sets, the index might suddenly become
 * inaccurate, and so must be maintained whenever an element is popped or pushed
 * onto such a set.
 */
/*! \var int FieldNode::format
 * \brief The format of the value.
 *
 * This is the same format number as is in StyleDef::format.
 */
/*! \var char islocal
 * \brief Whether this field node is held somewhere else or stands alone.
 *
 * When a Style returns a FieldNode in response to a query of a field, there is not
 * a guarantee that it is a pointer held by that Style. It might have been a FieldNode
 * created by Style on the fly to hold the field data. If islocal is nonzero, it implies
 * that it is not held by anything, and must be delete'd by the calling code. The contents
 * of value.str, value.otherfields, and value.v are assumed to be held elsewhere and are
 * not deleted. Also if islocal is nonzero, each field value.fields 
 * (if fields is the format) is called with
 * deleteFieldNode(), which deletes or not depending on each field's islocal.
 */
/*! \var unsigned int FieldNode::flags
 * \brief Extra flags for this field, such as read-only.
 *
 * This holds any extra information that might be useful to the Style.
 */
//class FieldNode
//{
// public:
//	int index; // have an int for index, implies cindex=NULL
//	char *cindex; // have a string for index, implies index=-1 ***
//	int format;
//	unsigned int flags; *** thisislocal flag
//	union { 
//		unsigned int f;    // format=0, for (non-exclusive) flags***** must settle on defines for standard formats
//		int i;             // format=1,2 for enums, bit, 3bit, ints
//		double d;          // format=3
//		char *str;         // format=4
//		FieldNode *fields; // format=5
//		Style *style;      // format=6, this is useful for variable sized sets
//		Value *v;          // format=7
//		void *other;          // format=8 other
//	} value;
//	FieldNode() { value.d=0; } //*** does this really wipe out all?
//};
//--OR--
//class FieldNode
//{
// public:
//	unsigned int flags; *** thisislocal flag
//	int index; // have an int for index, this is consulted before cindex
//	char *cindex; // have a string for index, non-null cindex means can use this rather than index, which is consulted first
//	int format;
//	void *value;
//	PtrStack<FieldNode> *subfields;
//	FieldNode() { cindex=NULL; index=-1; flags=0; format=0; value=NULL; subfields=NULL; }
//};

//! If the FieldNode contains fields, then delete the fields.
/*! **** This sure as hell needs some further thinking... memory leak galore...
 */
void deleteFieldNode(FieldNode *fn)
{
	if (!fn) return;
	if (fn->flags&STYLEDEF_ORPHAN) { //***
		if (fn->format==STYLEDEF_FIELDS) {
			delete[] fn->value.fields;
//			--------------assumes FieldNode **fields, and fields is NULL terminated:
//			int c=0;
//			FieldNode *f=fn.value.fields[c];
//			while (f) {
//				deleteFieldNode(f);
//				f=value.fields[c++];
		}
		delete fn;
	}
	fn=NULL;
}

//------------------------------------- Style -------------------------------------------


/*! \class Style
 * \ingroup stylesandstyledefs
 * \brief Abstract base class for styles.
 * 
 *  Styles hold the actual values of a style definition found in a StyleDef.
 *  Many styles are specialized enough to have their own derived Style classes,
 *  for instance when changing one value forces the change in another, it is 
 *  just easier to have a separate class to deal with it. Styles that do not
 *  need such specific checking can be a GenericStyle instance.
 *
 *  Please note that Style itself does not check for pointer consistency when
 *  reassigning, deleting, and creating new styles. that has to be implemented
 *  somewhere else (StyleManager, for instance).
 * 
 *  Styles are kind of an uber-Value. They are not Value themselves because 
 *  the Value setup doesn't need all the fancy basedon stuff,
 *  and also doesn't need the FieldMask feedback, which says what fields are
 *  changed when a field is assigned a new value.
 *
 *  Any given Style might be a derived class, and stores the fields in
 *  any old way, so any code that deals with Styles
 *  should always use the wrapper functions to get and set values. 
 *
 *  See GenericStyle for a kind of all-purpose flat Style, that is, it does not
 *  have any fancy hardcoded shortcuts.
 *
 *  Finally, derived classes should remember to define their own dump_out(FILE*,int) and
 *  dump_out_atts(Attribute*), required by class LaxFiles::DumpUtility.
 *
 *
 * <pre>
 *  ****IMPORTANT there must be an easy setup that allows a redefined Style to 
 *  **** easily change
 *  ****values when other values are changed, such as for constrained aspect ratios..
 *  **** some default way to set/access fields that uses all the FieldNode/StyleDef mechanism. This would 
 *  		be beneficial for scripting interface
 * </pre>
 */
/*! \var char *Style::stylename
 * \brief The name of the style.
 * 
 * Note this is not necessarily a variable name as seen by an interpreter, 
 * but it is an instance name.
 * It would be included in a list of available styles, like "Bold Body", and the
 * StyleDef name/Name might be "charstyle"/"Character Style"
 */
/*! \var StyleDef *Style::styledef
 * \brief The StyleDef that gives information about what the Style's fields are.
 *
 *  This points to an original StyleDef that would be kept in a pool somewhere, and
 *  which outlasts the Style instances.
 */
/*! \var Style *Style::basedon
 * \brief The Style that this style is based on.
 *
 * Any fields that are not actually defined in this Style should be available in basedon,
 * or something basedon is based on.
 */
/*! \var FieldMask Style::fieldmask
 * \brief Mask of which values are defined in this Style, and would overwrite fields from basedon.
 *
 * Mask values refer only to the top level fields, not subfields.
 */
/*! \fn StyleDef *Style::makeStyleDef()
 * \brief Construct and return an instance of the relevant StyleDef.
 *
 * Default just returns NULL. This function should otherwise return a unique instance
 * of the relevant StyleDef. Normally, this would then be added to the StyleManager,
 * and other Styles of the same kind would have a reference to that one, not a unique
 * def of their own.
 */
/*! \fn const char *Style::Stylename()
 * \brief Return const pointer to stylename.
 */
/*! \fn int Style::Stylename(const char *nname)
 * \brief Change stylename. Return 1 for changed, 0 for not.
 */
//class Style : public Laxkit::anObject
//{
// protected:
//	char *stylename;
//	StyleDef *styledef;
//	Style *basedon;
//	FieldMask fieldmask; 
// public:
//	Style();
//	Style(StyleDef *sdef,Style *bsdon,const char *nstn);
//	virtual ~Style();
//	virtual StyleDef *makeStyleDef() { return NULL; }
//	virtual StyleDef *GetStyleDef(StyleDef **maketohere);
//	virtual const char *Stylename() { return stylename; }
//	virtual int Stylename(const char *nname) { makestr(stylename,nname); return 1; }
//	virtual int getNumFields();
//	virtual Style *duplicate(Style *s)=0;
//	
//	 // these return a mask of what changes when you set the specified value.
//	 // set must ask the styledef if it can really set that field, 
//	 // 	the StyleDef returns a mask of what else changes?????***
//	 // set must create the field in *this if it does not exist
//	 // get should indicate whether the found value is in *this or a basedon
//	//maybe:
////	virtual void *dereference(const char *extstr,int copy);
////	virtual int set(FieldMask *field,Value *val,FieldMask *mask_ret)=0; 
////	virtual int set(const char *ext,Value *val,FieldMask *mask_ret)=0;
//	
////	virtual FieldMask set(FieldMask *field,Value *val); 
////	virtual FieldMask set(Fieldmask *field,const char *val);
////	virtual FieldMask set(const char *ext, const char *val);
////	virtual FieldMask set(Fieldmask *field,int val);
////	virtual FieldMask set(const char *ext, int val);
////	virtual FieldMask set(Fieldmask *field,double val);
////	virtual FieldMask set(const char *ext, double val);
////	virtual Value *getvalue(FieldMask *field) { return NULL; }
////	virtual Value *getvalue(const cahr *ext) { return NULL; }
////	virtual int getint(FieldMask *field) { return 0; }
////	virtual int getint(const char *ext) { return 0; }
////	virtual double getdouble(FieldMask *field) { return 0; };
////	virtual double getdouble(const char *ext) { return 0; }
////	virtual char *getstring(FieldMask *field) { return NULL; }
////	virtual char *getstring(const char *ext) { return NULL; }
//};

//! Start up with blank slate. All null variables.
Style::Style()
{ 
	basedon=NULL; 
	styledef=NULL; 
	stylename=NULL; 
}

//! Set up with the given styledef, basedon and stylename.
Style::Style(StyleDef *sdef,Style *bsdon,const char *nstn)
{ 
	styledef=sdef; 
	basedon=bsdon; 
	stylename=NULL; 
	makestr(stylename,nstn); 
}

//! Destructor, currently just deletes stylename. styledef and based on assumed non-local.
/*! \todo *** should implement some reference counting thing for Style and StyleDef..
 */
Style::~Style()
{
	if (stylename) delete[] stylename;
}

/*! \fn Style *Style::duplicate(Style *s)
 * \brief Abstract function. Derived classes should return a copy of the style.
 * 
 * This is really in lieu of copy constructor, since one never always
 * knows what is down in the style.. duplicate() must create a whole copy,
 * not just the elements that subclasses know how to copy over.
 *
 * Usually, s could  be a subclass. In that case, the subclass would have called
 * the based class duplicate() to fill in what the subclass didn't copy over.
 * Otherwise, passing s==NULL means the style should create a new copy of itself.
 */
//Style *Style::duplicate(Style *s)//s=NULL
//{
//	//***
//	return NULL;
//}

//! Return the StyleDef, or create one.
/*! If maketohere is not NULL, then the Style should create a suitable StyleDef instance
 *  and make *maketohere point to it, and also make styledef point to it, but only if 
 *  styledef is not already pointing to something. Usually this shouldn't happen, because
 *  calling GetStyleDef with maketohere not NULL should only be called at program
 *  or plugin startup to initialize the 
 *  Style instances of a pool which have their own idiosyncratic StyleDefs.
 *
 *  Derived classes need only redefine makeStyleDef() to make this function work properly.
 *
 *  In either case, the value of styledef (not necessarily what is now in maketohere!) is returned.
 */
StyleDef *Style::GetStyleDef(StyleDef **maketohere)
{ 
	if (maketohere) {
		*maketohere=makeStyleDef();
		if (!styledef) styledef=*maketohere;
	}
	return styledef; 
}

//! Return the number of top fields in this Style.
/*! This would be redefined for Styles that are variable length sets
 *  StyleDef itself has no way of knowing how many fields there are
 *  and even what kind of fields they are in that case.
 *
 *  Returns styledef->getNumFields(). If styledef==NULL, then return -1;
 *  *** so what? how is this used in conjunction with StyleDef::getNumFields?
 */
int Style::getNumFields()
{ 
	if (styledef) return styledef->getNumFields(); 
	return -1;
}
	

//---------------------------------------------------------------------



