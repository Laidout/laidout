/************ genericstyle.cc **************/

#include "genericstyle.h"



/*! \class GenericStyle
 * \ingroup stylesandstyledefs
 * \brief A style that has no very special internals.
 *
 * A generic style cannot generate its own StyleDef. ***? maybe it could!
 *
 * *** TODO: Should later change this so that internal checking can be done
 * with scripts stored here, rather than hardcoded!!
 */
/*! \var PtrStack<FieldNode> Style::fields
 * \brief Holds the actual values of the style.
 */
//class GenericStyle : public Style
//{
// protected:
//	 Laxkit::PtrStack<FieldNode> fields; // this usually holds the actual values
// public:
//	GenericStyle() { basedon=NULL; styledef=NULL; stylename=NULL; }
//	GenericStyle(StyleDef *sdef,Style *bsdon,const char *nstn);
//	virtual ~GenericStyle();
//	virtual Style *duplicate(Style *s=NULL);
//
//	virtual void *dereference(const char *extstr,int copy);
//	virtual int set(FieldMask *field,Value *val,FieldMask *mask_ret)=0; 
//	virtual int set(const char *ext,Value *val,FieldMask *mask_ret)=0;
//	
//	virtual FieldMask set(FieldMask *field,Value *val); 
//	virtual FieldMask set(const char *ext, Value *val);
//	virtual FieldMask set(Fieldmask *field,const char *val);
//	virtual FieldMask set(const char *ext, const char *val);
//	virtual FieldMask set(Fieldmask *field,int val);
//	virtual FieldMask set(const char *ext, int val);
//	virtual FieldMask set(Fieldmask *field,double val);
//	virtual FieldMask set(const char *ext, double val);
//	virtual Value *getvalue(FieldMask *field);
//	virtual Value *getvalue(const cahr *ext);
//	virtual int getint(FieldMask *field);
//	virtual int getint(const char *ext);
//	virtual double getdouble(FieldMask *field);
//	virtual double getdouble(const char *ext);
//	virtual char *getstring(FieldMask *field);
//	virtual char *getstring(const char *ext);
//};

//! **** need to imp.
Style *GenericStyle::duplicate(Style *s=NULL)
{
	return s;
}




//*** must return some kind of error code if wrong format
	*** have a flag to make Value/Style delete the int*field list?
FieldMask GenericStyle::set(char *extstr,int i) 
{
	char *rextstr=NULL;
	FieldNode *fn=deref(extstr,rextstr);
	if (!fn) return 0;
	if (fn->format==***STYLEDEF_INT) {
		fn->value.i=i;
	} else if (fn->format==***STYLEDEF_REAL) {
		fn->value.d=i;
	} else if (fn->format==***STYLEDEF_STRING) {
		if (fn->value.str) delete[] fn->value.str;
		fn->value.str=numtostr(i);
	}
	return 0;
}

 // returns a pointer (not a copy) to the FieldNode containing the data, for use
 // in assigning and whatever else. extstr="blah.34.3.blah".
 // extstr must not start with a '.'. Fields
 // beginning with a letter are field names, and beginning with a number
 // are indices to the fields, starting at zero. 
 //
 // finalIndexDeref holds the first unparsable part of extstr (or NULL). Return value fnode
 // of deref is still the most relevant FieldNode (or NULL), and presumably the unused portion
 // of extstr would then be passed on to the data itself (like Value).
 //
 // There are 2 trees: the StyleDef, and the FieldNodes. Names in the extstr are 
 // converted to numbers by calling StyleDef::findfield, and the appropriate FieldNode
 // is then searched for based on whatever the style is based on.
 //
 // returns:
 //   0 on failure, 
 //   1 if found, and field is part of this
 //   2 if found, but field is part of a substyle????
 //  -1 if found, and field is in a parent Style
int GenericStyle::deref(char *extstr,FieldNode **fnode,Style **s,char **finalIndexDeref) //  fiD=NULL
{******
	int p=0,c=0,n=-1;
	 
	 // For each part of extstr considered, during the loop, extstr will point
	 // to the starting character of the name, and st will point to the character
	 // after the ending character of the name.
	char *st=extstr;
	*s=this; // cannot just use this because a value of a field is allowed
	         // to be another Style, and deref() should be able to handle that
	FieldNode *f=NULL;
	Style *ts;
	
	 // there is one interation per field in extstr
	while (*extstr) {
		n=(*s)->styledef->findfield(extstr,&st); // if st="blah.blah2", st becomes "blah2"
		if (n>=0 && n<(*s)->getNumFields()) { // n is a valid index
			 // search for the n index in the fieldnodes
			ts=*s;
			do {
				for (c=0; c<ts->fields.n; c++) {
					if (c==ts->fields.e[c]->index) break;
				}
				if (c!=ts->fields.n) { // found the FieldNode
					f=ts->fields.e[n];
				} else {
					
				}
				
			} until (!(*s)->basedon);
		} else { // n<0 or >numf
			 // no suitable index fits what is in the extstr, so it is assumed that
			 // the rest of extstr should be passed on and parsed by the value
			finalIndexDeref=extstr;
			return f;
		}
	}
	***if (finalIndexDeref) *finalIndexDeref=n;
	return f;
}

 // returns a copy, if it knows how to???
void *GenericStyle::dereference(const char *extstr,int copy)
{
	*** must check for set.0 to return anInt(v->n)???
	Value *v=deref(extstr,copy,NULL);
	*** must return style, not value?
	if (v) if (copy) return v.duplicate(); else return v;
	
}
