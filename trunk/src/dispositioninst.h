#ifndef DISPOSITIONINST_H
#define DISPOSITIONINST_H

#include <disposition.h>

//------------------------ Singles ---------------------------

class Singles : public Disposition
{
  public:
	double insetl,insetr,insett,insetb;
	int tilex,tiley; 
	Singles();
	virtual ~Singles() {}
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
};

//------------------------ Double Sided Singles ---------------------------

class DoubleSidedSingles : public Singles
{
 public:
	int isvertical,isleft;
	DoubleSidedSingles();
//	virtual PageStyle *GetPageStyle(int pagenum); // return the default page style for that page
	virtual StyleDef *makeStyleDef();
	virtual Page **CreatePages(PageStyle *pagestyle=NULL); // create necessary pages based on default pagestyle
	virtual Spread *PageLayout(int whichpage); 
	virtual Spread *PaperLayout(int whichpaper);
	virtual Style *duplicate(Style *s=NULL);
	virtual Laxkit::DoubleBBox *GoodWorkspaceSize(int page=1,Laxkit::DoubleBBox *bbox=NULL);
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

////------------------------ Booklet ---------------------------
//
class BookletDisposition : public DoubleSidedSingles
{
 public:
	double creep;  // booklet.5
	unsigned long covercolor; // booklet.13
	unsigned long bodycolor; // booklet.14
	BookletDisposition();
	virtual StyleDef *makeStyleDef();
	virtual Style *duplicate(Style *s=NULL);
	
//	virtual SomeData *GetPrinterMarks(int papernum=-1) { return NULL; } // return marks in paper coords
	virtual Spread *PaperLayout(int whichpaper);
	virtual Laxkit::DoubleBBox *GetDefaultPageSize(Laxkit::DoubleBBox *bbox=NULL);
	virtual int *PrintingPapers(int frompage,int topage);
	virtual int PaperFromPage(int pagenumber); // the paper number containing page pagenumber
	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
	virtual void setPage();

	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


////------------------------ Basic Book ---------------------------
//
//class BasicBook : public Disposition
//{
// public:
//	int numsections;
//	int paperspersection; // 0 does not mean 0. 0 means infinity (or MAX_INT).
//	int creeppersection; 
//	int insetl,insetr,insett,insetb;
//	int tilex,tiley;
//	unsigned long bodycolor;
//	
//	char specifycover;
//	int spinewidth;
//	unsigned long covercolor;
//	PaperStyle coverpaper;
//	PageStyle coverpage;
//	virtual Style *duplicate(Style *s=NULL);
//	
//	virtual int GetPagesNeeded(int npapers); // how many pages needed when you have n papers
//	virtual int GetPapersNeeded(int npages); // how many papers needed to contain n pages
//
//	virtual void dump_out(FILE *f,int indent);
//	virtual void dump_in_atts(LaxFiles::Attribute *att);
//};

////---------------------------------- CompositeDisposition ----------------------------
//****
//
//



#endif

