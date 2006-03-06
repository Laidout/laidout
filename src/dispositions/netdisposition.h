#ifndef NETDISPOSITION_H
#define NETDISPOSITION_H

#include "../disposition.h"
#include "nets.h"

class NetDisposition : public Disposition
{
 public:
	Net *net;
	int netisbuiltin;

	NetDisposition(Net *newnet=NULL);
	virtual ~NetDisposition();
	virtual Style *duplicate(Style *s=NULL);
	
	virtual int SetPaperSize(PaperType *npaper); // set paperstyle, and compute page size
	
	//virtual Laxkit::SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle

	virtual Laxkit::SomeData *GetPage(int pagenum,int local); // return outline of page in page coords

	virtual Spread *GetLittleSpread(int whichpage); 
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual int *PrintingPapers(int frompage,int topage);

//	virtual int NumPapers(int npapers);
//	virtual int NumPages(int npages);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	
	virtual int SetNet(const char *nettype);
	virtual int SetNet(Net *newnet);
	virtual void setPage();
};

#endif

