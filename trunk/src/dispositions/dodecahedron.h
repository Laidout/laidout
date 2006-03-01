#ifndef DODECAHEDRON_H
#define DODECAHEDRON_H

#include "disposition.h"

class Dodecahedron : public Disposition
{
  public:
	double insetl,insetr,insett,insetb;
	int pagesoncorners;
	Dodecahedron();
	virtual ~Dodecahedron() {}
	virtual StyleDef *makeStyleDef();
	
//	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
	virtual Laxkit::SomeData *GetPage(int pagenum,int local); // return outline of page in paper coords
	virtual Spread *GetLittleSpread(int whichpage); 
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual Style *duplicate(Style *s=NULL);
	virtual int SetPaperSize(PaperType *npaper);
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);

	virtual void DefineNet();
	virtual void FitToData(Laxkit::SomeData *data,double margin);
};




#endif


