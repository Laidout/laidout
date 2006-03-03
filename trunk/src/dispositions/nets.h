#ifndef NETS_H
#define NETS_H

#include <lax/interfaces/somedata.h>
#include <lax/displayer.h>

//----------------------------------- NetLine -----------------------------------
class NetLine
{
 public:
	char isclosed;
	int np;
	int *points;
	char lsislocal;
	LineStyle *linestyle;
	NetLine() { linestyle=NULL; lsislocal=0; isclosed=0; np=0; points=NULL; }
	virtual ~NetLine() { if (lsislocal) delete linestyle; else linestyle->dec_count(); ???? }
	const NetLine &operator=(const NetLine &line);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

//----------------------------------- NetFace -----------------------------------
class NetFace
{
 public:
	int np;
	int *points, *facelink;
	double *m;
	int aligno, alignx;
	int faceclass;
	NetFace() { m=NULL; aligno=alignx=-1; faceclass=-1; np=0; points=NULL; }
	virtual ~NetFace() { if (m) delete[] m; }
	const NetFace &operator=(const NetFace &face);
	
	virtual void dump_out(FILE *f,int indent, int pfirst=0);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
};

//----------------------------------- Net -----------------------------------
class Net : public Laxkit::SomeData
{
 public:
	char *thenettype;
	int np,tabs; 
	flatpoint *points;
	int *pointmap; // which thing (possibly 3-d points) corresponding point maps to
	int nl;
	NetLine *lines;
	int nf;
	NetFace *faces;
	Net();
	virtual ~Net();
	virtual void clear();
	virtual Net *duplicate();
	virtual const char *whatshape() { return thenettype; }
	virtual int Draw(cairo_t *cairo,Laxkit::Displayer *dp,int month,int year);
	virtual void DrawMonth(cairo_t *cairo,Laxkit::Displayer *dp,int month,int year,Laxkit::SomeData *monthbox);
	virtual void FindBBox();
	virtual void FitToData(Laxkit::SomeData *data,double margin);
	virtual void Center();
	virtual const char *whattype() { return thenettype; }
	virtual void dump_out(FILE *f,int indent);
	virtual void dump_in_atts(LaxFiles::Attribute *att);
	virtual int pointinface(flatpoint pp);
	virtual int rotateface(int f,int alignxonly=0);
	virtual void pushline(NetLine &l,int where=-1);
	virtual void pushface(NetFace &f);
	virtual void pushpoint(flatpoint pp,int pmap=-1);

	//--perhaps for future:
	//virtual void PrintSVG(std::ostream &svg,Laxkit::SomeData *paper,int month=1,int year=2006);
	//virtual void PrintPS(std::ofstream &ps,Laxkit::SomeData *paper);
};



#endif

