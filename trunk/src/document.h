#ifndef DOCUMENT_H
#define DOCUMENT_H

class Document;
#include "styles.h"
#include "disposition.h"

//------------------------- DocumentStyle ------------------------------------

class DocumentStyle : public Style
{
 public:
	Disposition *disposition;
	DocumentStyle(Disposition *disp);
	virtual ~DocumentStyle();
	virtual Style *duplicate(Style *s=NULL);
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};


//------------------------- Document ------------------------------------

enum  LaidoutSaveFormat {
	Save_Normal,
	Save_PPT,
	Save_PS,
	Save_EPS,
	Save_PDF,
	Save_HTML,
	Save_SVG,
	Save_Scribus,
};

class Document : public ObjectContainer, public LaxFiles::DumpUtility
{
 public:
	DocumentStyle *docstyle;
	char *saveas;
	
	Laxkit::PtrStack<Page> pages;
	int curpage;
	int numn;
	char **notesorscripts;
	clock_t modtime;

	Document(const char *filename);
	Document(DocumentStyle *stuff=NULL,const char *filename=NULL);
	virtual ~Document();
	virtual const char *Name();
	virtual int Name(const char *nname);
	virtual Page *Curpage();

	virtual void clear();
	virtual int NewPages(int starting,int n);
	
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual int Load(const char *file);
	virtual int Save(LaidoutSaveFormat format=Save_Normal);
	
	virtual int n() { return pages.n; }
	virtual Laxkit::anObject *object_e(int i) 
		{ if (i>=0 && i<pages.n) return (anObject *)(pages.e[i]); return NULL; }
//	int SaveAs(char *newfile,int format=1); // format: binary, xml, pdata
//	int New();
//	int Print();
};


//------------------------- Project ------------------------------------

class Project
{
 public:
//	StyleManager styles;
	Laxkit::PtrStack<Document> docs;
	Page scratchboard;
	Laxkit::PtrStack<char> project_notes;

	Project();
	virtual ~Project();
};

#endif

