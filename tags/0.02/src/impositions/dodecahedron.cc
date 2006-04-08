//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/*************** dodecahedron.cc ********************/


#include "dodecahedron.h"

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;


#include <iostream>
using namespace std;
#define DBG 


//-------------------------- Dodecahedron ---------------------------------------------


//! Define all the points in a Dodecahedron net.
Net *makeDodecahedronNet(double ww,double hh)
{
	Net *net=new Net();
	makestr(net->thenettype,"dodecahedron");
	double s,a,b,c,h,v,i,j,k,q,r,m,n,tau,l;
	s=10;
	tau=(sqrt(5.)+1)/2;
	a=s*cos(54/180.*3.1415926535);
	b=s*sin(72/180.*3.1415926535);
	c=s/2*(1+cos(36/180.*3.1415926535))/sin(36/180.*3.1415926535);
	h=c*tau*tau;
	//v=s*(3*tau*tau+cos(72/180.*3.1415926535));
	v=s/2*(7*tau+5);
	i=s/2/tau;
	j=s/2;
	k=s*tau/2;
	q=s*tau*tau/2;
	r=s*tau;
	m=s/2*(tau+2);
	n=s*tau*tau*tau/2;
	l=s*tau*tau*tau;

	// total height=v, total width=h
	
	 // make points
	net->np=38;
	net->points=new flatpoint[38];
	net->points[0]=flatpoint(c,l);
	net->points[1]=flatpoint(b,l-k);
	net->points[2]=flatpoint(c,l-r);
	net->points[3]=flatpoint(a,l-q);
	net->points[4]=flatpoint(0,l/2);
	net->points[5]=flatpoint(a,q);
	net->points[6]=flatpoint(c,r);
	net->points[7]=flatpoint(b,k);
	net->points[8]=flatpoint(c,0);
	net->points[9]=flatpoint(h-c,i);
	net->points[10]=flatpoint(h-c,q);
	net->points[11]=flatpoint(h-b,j);
	net->points[12]=flatpoint(h,k);
	net->points[13]=flatpoint(h,m);
	net->points[14]=flatpoint(h-b,l/2);
	net->points[15]=flatpoint(h,l-m);
	net->points[16]=flatpoint(h,l-k);
	net->points[17]=flatpoint(h-b,l-j);
	net->points[18]=flatpoint(h-c,l-q);
	net->points[19]=flatpoint(h-c,l-i);
	net->points[20]=flatpoint(h-b,v-l+k);
	net->points[21]=flatpoint(h-c,v-l+r);
	net->points[22]=flatpoint(h-a,v-l+q);
	net->points[23]=flatpoint(h,v-l/2);
	net->points[24]=flatpoint(h-a,v-q);
	net->points[25]=flatpoint(h-c,v-r);
	net->points[26]=flatpoint(h-b,v-k);
	net->points[27]=flatpoint(h-c,v);
	net->points[28]=flatpoint(c,v-i);
	net->points[29]=flatpoint(c,v-q);
	net->points[30]=flatpoint(b,v-j);
	net->points[31]=flatpoint(0,v-k);
	net->points[32]=flatpoint(0,v-m);
	net->points[33]=flatpoint(b,v-l/2);
	net->points[34]=flatpoint(0,v-l+m);
	net->points[35]=flatpoint(0,v-l+k);
	net->points[36]=flatpoint(b,v-l+j);
	net->points[37]=flatpoint(c,v-l+q);
	//for (int c=0; c<net->np; c++) cout <<"Dodecahedron point "<<c<<":"<<net->points[c].x<<','<<net->points[c].y<<endl;

	 // make lines
	net->nl=4;
	net->lines=new NetLine[net->nl];
	net->lines[0].Set(net->np,1);
	net->lines[1].Set("2 6 10 14 18 2");
	net->lines[2].Set("0 19");
	net->lines[3].Set("21 25 29 33 37 21");
	
	 // make months
	net->nf=12;
	net->faces=new NetFace[net->nf];

	net->faces[0].Set("29 33 37 21 25");
	net->faces[1].Set("0 19 20 21 37");
	net->faces[2].Set("34 35 36 37 33");
	net->faces[3].Set("30 31 32 33 29");
	net->faces[4].Set("26 27 28 29 25");
	net->faces[5].Set("22 23 24 25 21");
	net->faces[6].Set("10 14 18 2 6");
	net->faces[7].Set("3 4 5 6 2");
	net->faces[8].Set("7 8 9 10 6");
	net->faces[9].Set("11 12 13 14 10");
	net->faces[10].Set("15 16 17 18 14");
	net->faces[11].Set("19 0 1 2 18");
	
	net->FindBBox();

	 // Adjust net matrix so it fits in a rectangle with width and height ww,hh
	SomeData d;
	d.maxx=ww;
	d.maxy=hh;

	////DBG cout <<"******* dodechaedron plain: **********"<<endl;
	////DBG net->dump_out(stdout,0);

	
	net->FitToData(&d,ww*.05);
	net->ApplyTransform();

	//DBG cout <<"******* dodechaedron after: **********"<<endl;
	//DBG net->dump_out(stdout,0);

		
	return net;
}



