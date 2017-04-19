#ifndef UNWRAPINTERFACE_H
#define UNWRAPINTERFACE_H

#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/viewerwindow.h>
#include <lax/lists.h>

#include "polyptych/src/poly.h"
#include "polyptych/src/nets.h"


namespace Laidout {


//--------------------------- UnwrapInterface ---------------------------
class UnwrapInterface : public LaxInterfaces::anInterface
{
  protected:
	int mode;
	flatpoint leftp;
	int lx,ly;
	int max_preview_x, max_preview_y;
	int newseed;
	NetFace *seedface;
	AbstractNet *abstractnet;
	Net *net;
	int firsttime;

	NetFace *findNewSeed();
	//virtual void runImageDialog();

  public:
	int showdecs;
	char showfile,showimages;
	UnwrapInterface(Polyhedron *npoly,Net *nnet,const char *nfilebase,int nid,Laxkit::Displayer *ndp);
	virtual ~UnwrapInterface();
	virtual anInterface *duplicate(anInterface *dup=NULL);
	virtual void deletedata();

	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual void drawNetFace(NetFace *pgon);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	//virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual const char *whattype() { return "UnwrapInterface"; }
	virtual const char *whatdatatype() { return "none"; }
	virtual LaxInterfaces::SomeData *Curdata() { return NULL; }
	virtual void Clear(LaxInterfaces::SomeData *d);
};


} //namespace Laidout

#endif

