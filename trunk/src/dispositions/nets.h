#ifndef NETS_H
#define NETS_H

#include <lax/interfaces/somedata.h>
#include <lax/displayer.h>

#define pi 3.14159265358979323846

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
};

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
	const NetLine &operator=(const NetLine &line);
};

class Net : public Laxkit::SomeData
{
 public:
	char *thenettype;
	int np; 
	flatpoint *points;
	int *pointmap; // which thing (possibly 3-d points) corresponding point maps to
	int nl;
	NetLine *lines;
	int nf;
	NetFace *faces;
	Net();
	virtual ~Net();
	virtual void clear();
	virtual const char *whatshape() { return thenettype; }
	virtual int Draw(cairo_t *cairo,Laxkit::Displayer *dp,int month,int year);
	virtual void DrawMonth(cairo_t *cairo,Laxkit::Displayer *dp,int month,int year,Laxkit::SomeData *monthbox);
	virtual void PrintPS(std::ofstream &ps,Laxkit::SomeData *paper);
	virtual void PrintSVG(std::ostream &svg,Laxkit::SomeData *paper,int month=1,int year=2006);
	virtual void FindBBox();
	virtual void FitToData(Laxkit::SomeData *data,double margin);
	virtual void Center();
	virtual SomeData *GetMonthBox(int which);
	virtual void SVGMonth(ostream &svg,int month,int year,SomeData *monthbox);
	virtual const char *whattype() { return thenettype; }
	virtual void dump_out(FILE *f,int indent);
	virtual void  dump_in(FILE *f,int indent);
	virtual void pushface(int *f,int n);
	virtual void pushpoint(flatpoint pp);
	virtual int pointinface(flatpoint pp);
	virtual void rotateface(int f,int endonly=0);
};



#endif

