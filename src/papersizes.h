#ifndef PAPERSIZES_H
#define PAPERSIZES_H


#define LAX_LISTS_H_ONLY
#include <lax/lists.h>
#undef LAX_LISTS_H_ONLY

#include "styles.h"

class PaperType : public Style
{
 public:
	char *name;
	double width,height;
	int dpi;
	unsigned int flags; //1=landscape !(&1)=portrait
	PaperType(const char *nname,double ww,double hh,unsigned int f,int ndpi);
	virtual double w() { if (flags&1) return height; else return width; }
	virtual double h() { if (flags&1) return width; else return height; }
	virtual Style *duplicate(Style *s=NULL);

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

Laxkit::PtrStack<PaperType> *GetBuiltinPaperSizes(Laxkit::PtrStack<PaperType> *papers);


#endif

