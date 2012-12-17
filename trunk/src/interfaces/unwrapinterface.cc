/*** unwrapinterface.cc ***/

#include <iostream>

#include "unwrapinterface.h"
#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/lists.cc>
#include "nets.h"



//
//
// **** NOTES
//
// - be able to position net on a piece of paper...
//
// - type number to jump to new seed face, or to face set down already
// - cursor left/right to trip through laid down faces
// - left click on no face, initiate potential drop new seed
//      -> left click again to actually drop face
//      -> left/right arrows to change seed face to any other available face
// - left click on face to pick up an actual face, or make actual a potential face
// - unwrap all for currently selected face
// - unwrap all remaining
// - undo last action
// - save net to disk
// - save unwrapping instructions
//
//
//



using namespace std;
using namespace Laxkit;
using namespace LaxInterfaces;

#define DBG 


namespace Laidout {



//--------------------------- UnwrapInterface ---------------------------
/*! \class UnwrapInterface
 * \brief Interface to unwrap an AbstractNet into a 2-d net.
 *
 */

UnwrapInterface::UnwrapInterface(Polyhedron *nabstractnet,Net *nnet,const char *nfilebase,int nid,Displayer *ndp)
	: anInterface(nid,ndp)
{ ***
	buttondown=0;
	showdecs=1;
	showfile=0;
	showimages=1;
	mode=0;
	firsttime=1;

	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
	
	needtodraw=1;

	abstractnet=nabstractnet->duplicate();
	net=nnet;
	if (!net) net=new Net;
	eles net->inc_count();
	net->Basenet(abstractnet);

	if (!net->faces.n) seedface=findNewSeed();
	else seedface=NULL;

	filebase=newstr(nfilebase);
	images=NULL;
	if (filebase && *filebase && abstractnet->faces.n) {
		char filename[600];
		images=new Imlib_Image[abstractnet->faces.n];
		for (int c=0; c<abstractnet->faces.n; c++) {
			sprintf(filename,"%s%03d.png",filebase,c);
			images[c]=imlib_load_image(filename);
			if (!images[c]) {
				cerr <<"Warning: failed to load face image from "<<filename<<endl;
			}
		}
	}
	DBG cerr <<"Done reading in images..."<<endl;
}

//! Return the first face not already in the net. It can be a potential face.
NetFace *UnwrapInterface::findNewSeed()
{ ***
	int s=0,c;
	if (net->faces.n) for (s=0; s<abstractnet->faces.n; s++) {
		for (c=0; c<net->faces.n; c++) {
			if (s==net->faces.e[c]->original && net->faces.e[c]->tag!=FACE_Actual)
				break;
		}
		if (c<net->faces.n) break;
	}
	if (s==abstractnet->faces.n) return NULL;

	return net->basenet->GetFace(s);
}


UnwrapInterface::~UnwrapInterface()
{ *** 	
	deletedata();
	DBG cerr <<"----in UnwrapInterface destructor"<<endl;
}

//! Return new UnwrapInterface.
/*! If dup!=NULL and it cannot be cast to UnwrapInterface, then return NULL.
 *
 * \todo dup max_preview dims, and make it one dim, not x and y?
 */
anInterface *UnwrapInterface::duplicate(anInterface *dup)
{ ***
	if (dup==NULL) dup=new UnwrapInterface(abstractnet,net,filebase,id,NULL);
	else if (!dynamic_cast<UnwrapInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}

//! Sets showdecs=1, and needtodraw=1.
int UnwrapInterface::InterfaceOn()
{ ***
	DBG cerr <<"imageinterfaceOn()"<<endl;
	showdecs=1;
	needtodraw=1;
	mode=0;
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int UnwrapInterface::InterfaceOff()
{ ***
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	DBG cerr <<"imageinterfaceOff()"<<endl;
	return 0;
}

void UnwrapInterface::Clear(SomeData *d)
{ ***
}

//! Draw ndata, but remember that data should still be the resident data.
int UnwrapInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{ ***
	return 1;
}

//! Transform to pgon space and draw it's outline, and optionally numbers in it.
void UnwrapInterface::drawNetFace(NetFace *pgon)
{ ***
	if (pgon->matrix) dp->PushAndNewTransform(pgon->matrix);
	

	 //draw face outline
	char blah[20];
	XPoint points[pgon->edges.n+1];
	int c;
	XPoint center,t;
	center.x=center.y=0;

	flatpoint x,y,o;
	DoubleBBox bbox;
	flatpoint ul,ll,ur, p;

	o=pgon->edges.e[0]->points->p();
	x=pgon->edges.e[1]->points->p() - o;
	y=pgon->edges.e[pgon->edges.n-1]->points->p() - o;
	y=(y|=x);

	for (c=0; c<pgon->edges.n; c++) {
		p=pgon->edges.e[c]->points->p();
		bbox.addtobounds(p);
		p=dp->realtoscreen(p);
		points[c].x=(int)p.x;
		points[c].y=(int)p.y;
		center.x+=points[c].x;
		center.y+=points[c].y;
	}
	center.x/=pgon->edges.n;
	center.y/=pgon->edges.n;
	points[c]=points[0];
	

	 //make potential faces dashed
	if (pgon->tag!=FACE_Actual) {
		dp->NewFG(255,128,128);
		dp->LineAttributes(1,LineOnOffDash,CapRound,JoinRound);
	} else {
		dp->NewFG(255,255,255);
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
	}

	XDrawLines(dp->GetDpy(),dp->GetWindow(),dp->GetGC(),points,pgon->edges.n+1,CoordModeOrigin);

	 //reset to solid line
	dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
	

	 // draw vertex labels
	if (showdecs&2) {
		dp->NewFG(0,255,0);
		for (c=0; c<pgon->edges.n; c++) {
			t.x=(center.x+4*points[c].x)/5;
			t.y=(center.y+4*points[c].y)/5;
			sprintf(blah,"%d",pgon->edges.e[c]->id);
			textout(dp->GetWindow(),blah,strlen(blah),t.x,t.y,LAX_CENTER);
			//XDrawString(app->dpy,window,app->gc(),t.x,t.y,blah,strlen(blah));
		}
	}


	 //draw face number
	if (showdecs&1) {
		dp->NewFG(255,255,255);
		sprintf(blah,"%d",pgon->original);
		textout(dp->GetWindow(),blah,strlen(blah),center.x,center.y,LAX_CENTER);
		//XDrawString(app->dpy,window,app->gc(),center.x,center.y,blah,strlen(blah));
	}

//	if (showdecs&4) { // show face axes, origin is center, x is p1->p2
//		flatpoint x,y,cnt,l;
//		cnt=dp->screentoreal(center.x,center.y);
//		x=pgon->p[1]-pgon->p[0];
//		x=x/2;
//		y=transpose(x);
//		dp->NewFG(255,0,0);
//		dp->drawrline(cnt,cnt+x);
//		dp->NewFG(0,255,0);
//		dp->drawrline(cnt,cnt+y);
//	}
	
	
	if (pgon->matrix) dp->PopAxes();
}

////! Draw a little circle around a face number, n for net and p for potential. (skip if -2)
//void UnwrapInterface::ShowFace(int n,int p)
//{***
//	XSetFunction(app->dpy,app->gc(),GXxor);
//	XSetForeground(app->dpy,app->gc(),~0);
//	NetFace *pg;
//	if (n>=0 && n<net->faces.en) {
//		pg=net->faces.e[n];
//		int c;
//		flatpoint center;
//		for (c=0; c<pg->edges.n; c++) {
//			center+=dp->realtoscreen(transform_point(pg->matrix,pg->edges.e[c]->points->p()));
//		}
//		center/=c;
//		XDrawArc(app->dpy,window,app->gc(), (int)center.x-15, (int)center.y-15,2*15,2*15, 0, 23040);
//	}
//	if (p>=0 && p<potential.n) {
//		pg=potential.e[p];
//		int c;
//		flatpoint center;
//		for (c=0; c<pg->pn; c++) {
//			center+=dp->realtoscreen(transform_point(pg->m(),pg->p[c]));
//		}
//		center/=c;
//		XDrawArc(app->dpy,window,app->gc(), (int)center.x-15, (int)center.y-15,2*15,2*15, 0, 23040);
//	}
//	XSetFunction(app->dpy,app->gc(),GXcopy);
//}

/*! Note that this calls  imlib_context_set_drawable(dp->GetWindow()).
 *
 * Returns 1 if no data, -1 if thing was offscreen, or 0 if thing drawn.
 *
 */
int UnwrapInterface::Refresh()
{ ***
	if (!dp || !needtodraw) return 0;
	needtodraw=0;
	if (!net) return 1;
	if (firsttime) {
		firsttime=0;
	}

//	 // draw lines
//	for (int c=0; c<net->lines.n; c++) {
//		*****
//	}

	 // draw faces
	for (int c=0; c<net->faces.n; c++) {
		drawNetFace(net->faces.e[c]);
	}

	if (seedface) {
		dp->NewFG(255,0,0);
		drawNetFace(seedface);
		dp->NewFG(255,255,255);
	}

	//DBG cerr<<"..Done drawing UnwrapInterface"<<endl;
	return 0;
}

void UnwrapInterface::deletedata()
{ ***
	//if (data) data->dec_count();
	//data=NULL;
}

/*! Left click on no face, initiate potential drop with new seed.
 *  Press left/right arrows to change seed face to any other available face.
 *  Left click again to actually drop face. Potential face follows mouse.
 *
 * Left click on a leaf face to pick up an actual face.
 * Left click on a potential face to make it an actual one.
 * 
 * Shift click on actual or potential to unwrap all on that face?
 */
int UnwrapInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ ***
	DBG cerr << "  in unwrapinterface lbd..";
	buttondown.down(d->id,LEFTBUTTON, x,y);

	//***Find which face clicked down in if any

//	if (state&ControlMask && state&ShiftMask && data) { // +^lb move around wildpoint
//		return 0;
//	} else if (state&ControlMask && data) { // ^lb focus or end angle
//		return 0;
//	} else if (state&ShiftMask && data) { // +lb start angle
//		return 0;
//	} else { // straight click
//		return 0;
//	}

	needtodraw=1;
	return 0;
	DBG cerr <<"..unwrapinterfacelbd done   ";
}

//! If data, then call viewport->ObjectMove(data).
int UnwrapInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{ ***
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (seedface) {
		net->Anchor(seedface->original);
		int c;
		for (c=0; c<net->faces.n; c++) if (net->faces.e[c]->original==seedface->original) break;
		//**** move face to be under mouse...
		//
		delete seedface;
		seedface=NULL;
		needtodraw=1;
		return 0;
	}

	flatpoint p=dp->screentoreal(x,y);
	int neti=net->pointinface(p);
	cout <<"point in face: "<<(neti<0?-1:net->faces.e[neti]->original)<<endl;

	if (neti<0) return 0;

	NetFace *netf=net->faces.e[neti];
	if (netf->tag==FACE_Actual) {
		 //clicked on an actual face. if is leaf, then pick up
		int e=net->PickUp(neti,-1);
		if (e!=0) {
			viewport->postmessage("Cannot pick up that face!");
		} else viewport->postmessage("Face picked up.");
		needtodraw=1;
		return 0;
	} else if (netf->tag==FACE_Potential) {
		 //clicked on potential face, so drop it down
		net->Drop(neti);
		//netf->tag=FACE_Actual;
		//net->clearPotentials(netf->original);
		//net->addPotentialsToFace(net->faces.findindex(netf));
		//net->validateNet();
		needtodraw=1;
		return 0;
	}
	

	return 0;
}

int UnwrapInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{ ***
	int mx,my;
	buttondown.move(mouse->id, x,y, &mx,&my);

	flatpoint p=dp->screentoreal(x,y);
	int neti=net->pointinface(p);
	cout <<"point in face: "<<(neti<0?-1:net->faces.e[neti]->original)<<endl;
	char mes[50];
	sprintf(mes,"%d",(neti<0?-1:net->faces.e[neti]->original));
	viewport->postmessage(mes);

	if (!buttondown.any() || !net) return 0;


	
//	 // If mode!=1, then do normal rotate and scale
//	flatpoint d; // amount to shift the image origin.
//	flatpoint p=screentoreal(x,y),    //real point where mouse moved to
//				//real point where mouse clicked:
//				//should be the same as screentoreal(lx,ly)
//			  lp=data->origin() + leftp.x*data->xaxis() + leftp.y*data->yaxis(); 
//	flatpoint oo,
//			  o; // the point in image coords the mouse is over
//	oo=(p-data->origin()); // real vector from data->origin() to mouse move to 
//	o.x=(oo*data->xaxis())/(data->xaxis()*data->xaxis());
//	o.y=(oo*data->yaxis())/(data->yaxis()*data->yaxis()); // o is in data coords now
//	
//	//DBG cerr <<"x,y="<<x<<','<<y<<"  p="<<p.x<<","<<p.y<<"  o="<<o.x<<','<<o.y;
//
//	if (!(buttondown&1) !dp) return 1;
//	if (x==mx && y==my) return 0;
//	if (state&ControlMask && state&ShiftMask) { //rotate
//		double angle=x-mx;
//		data->xaxis(rotate(data->xaxis(),angle,1));
//		data->yaxis(rotate(data->yaxis(),angle,1));
//		d=lp-(data->origin()+data->xaxis()*leftp.x+data->yaxis()*leftp.y);
//		data->origin(data->origin()+d);
//	} else if (state&ControlMask) { // scale
//		if (x>mx) {
//			if (data->xaxis()*data->xaxis()<dp->upperbound*dp->upperbound) {
//				data->xaxis(data->xaxis()*1.05);
//				data->yaxis(data->yaxis()*1.05);
//			}
//		} else if (x<mx) {
//			if (data->xaxis()*data->xaxis()>dp->lowerbound*dp->lowerbound) {
//				data->xaxis(data->xaxis()/1.05);
//				data->yaxis(data->yaxis()/1.05);
//			}
//		}
//		oo=data->origin() + leftp.x*data->xaxis() + leftp.y*data->yaxis(); // where the point clicked down on is now
//		//DBG cerr <<"  oo="<<oo.x<<','<<oo.y<<endl;
//		d=lp-oo;
//		data->origin(data->origin()+d);
//	} else { //translate
//		d=screentoreal(x,y)-screentoreal(mx,my);
//		data->origin(data->origin()+d);
//	}
//	//DBG cerr <<"  d="<<d.x<<','<<d.y<<endl;

	needtodraw|=2;
	return 0;
}

int UnwrapInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ *** return 1; }

int UnwrapInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{ *** return 1; }


/*! <pre>
 *   'a'          unwrap all
 *   left/right   select next/previous face
 *   space        pick up or drop selected face
 *   's'          save net to disk
 *   'l'          load net from disk
 *   'q'          quit
 *  </pre>
 */
int UnwrapInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{ ***
//	if (ch==LAX_Shift && (state:LAX_STATE_MASK)==0) { // shift
//	if (ch==LAX_Control && (state:LAX_STATE_MASK)==0) { // cntl
//	if (ch==LAX_Del && (state:LAX_STATE_MASK)==0) // delete
//	if (ch==LAX_Bksp && (state:LAX_STATE_MASK)==0) { // backspace
//	if (ch==LAX_Left && (state:LAX_STATE_MASK)==0) { // left //***unmodified from before rect
//	if (ch==LAX_Right && (state:LAX_STATE_MASK)==0) { // right
//	if (ch==LAX_Up && (state:LAX_STATE_MASK)==0) // up
//	if (ch==LAX_Down && (state:LAX_STATE_MASK)==0) // down
//			break;
//	if (ch==' ' && (state:LAX_STATE_MASK)==0)

	if (ch=='i' && (state&LAX_STATE_MASK)==0) {
		showimages=!showimages;
		needtodraw=1;
		return 0;

	} else if (ch=='f' && (state&LAX_STATE_MASK)==0) {
		showfile++;
		if (showfile==3) showfile=0;
		needtodraw=1;
		DBG cerr <<"UnwrapInterface showfile: "<<(int)showfile<<endl;
		return 0;

	} else if (ch=='D' && (state&LAX_STATE_MASK)==0) {
		net->dump_out(stdout,0,0,NULL);
		return 0;

	} else if (ch=='d' && (state&LAX_STATE_MASK)==0) {
		if (--showdecs<0) showdecs=3;
		needtodraw=1;
		return 0;

	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
		net->TotalUnwrap();
		if (seedface) { delete seedface; seedface=NULL; }
		needtodraw=1;
		return 0;

	} else if (ch=='s' && (state&LAX_STATE_MASK)==0) {
		FILE *f=fopen("saved.net","w");
		net->dump_out(f,0,0,NULL);
		fclose(f);
		viewport->postmessage("Net saved");
		return 0;

	} else if (ch=='q') {
		app->quit();
		return 0;
	}
	return 1; 
}

int UnwrapInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{ *** 
	return 1; 
}

} //namespace Laidout

