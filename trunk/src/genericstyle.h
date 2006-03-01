#ifndef GENERICSTYLE_H
#define GENERICSTYLE_H

#include "styles.h"

class GenericStyle : public Style
{
 protected:
	 Laxkit::PtrStack<FieldNode> fields; // this usually holds the actual values
 public:
	GenericStyle() { basedon=NULL; styledef=NULL; stylename=NULL; }
	GenericStyle(StyleDef *sdef,Style *bsdon,const char *nstn);
	virtual ~GenericStyle();
	virtual Style *duplicate(Style *s=NULL);

	virtual void *dereference(const char *extstr,int copy);
	virtual int set(FieldMask *field,Value *val,FieldMask *mask_ret)=0; 
	virtual int set(const char *ext,Value *val,FieldMask *mask_ret)=0;
	
	virtual FieldMask set(FieldMask *field,Value *val); 
	virtual FieldMask set(const char *ext, Value *val);
	virtual FieldMask set(Fieldmask *field,const char *val);
	virtual FieldMask set(const char *ext, const char *val);
	virtual FieldMask set(Fieldmask *field,int val);
	virtual FieldMask set(const char *ext, int val);
	virtual FieldMask set(Fieldmask *field,double val);
	virtual FieldMask set(const char *ext, double val);
	virtual Value *getvalue(FieldMask *field);
	virtual Value *getvalue(const cahr *ext);
	virtual int getint(FieldMask *field);
	virtual int getint(const char *ext);
	virtual double getdouble(FieldMask *field);
	virtual double getdouble(const char *ext);
	virtual char *getstring(FieldMask *field);
	virtual char *getstring(const char *ext);
};

#endif

