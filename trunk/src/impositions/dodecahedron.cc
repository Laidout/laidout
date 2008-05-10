//
// $Id$
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/*************** dodecahedron.cc ********************/


#include "dodecahedron.h"
#include "poly.h"
#include <lax/strmanip.h>

using namespace Laxkit;
using namespace LaxInterfaces;

#include <iostream>
using namespace std;


#include <iostream>
using namespace std;
#define DBG 


//-------------------------- Dodecahedron ---------------------------------------------

Net *makeDodecahedronNet(double ww,double hh)
{
	Polyhedron *poly=new Polyhedron;

	double t=(sqrt(5)+1)/2;
	double tt=t*t;
	poly->AddPoint(spacevector(  0, -1, tt)); //0
	poly->AddPoint(spacevector(  0,  1, tt)); //1
	poly->AddPoint(spacevector(  0,  1,-tt)); //2
	poly->AddPoint(spacevector(  0, -1,-tt)); //3
	poly->AddPoint(spacevector( tt,  0, -1)); //4
	poly->AddPoint(spacevector( tt,  0,  1)); //5
	poly->AddPoint(spacevector(-tt,  0,  1)); //6
	poly->AddPoint(spacevector(-tt,  0, -1)); //7
	poly->AddPoint(spacevector( -1,-tt,  0)); //8
	poly->AddPoint(spacevector(  1,-tt,  0)); //9
	poly->AddPoint(spacevector( -1, tt,  0)); //10
	poly->AddPoint(spacevector(  1, tt,  0)); //11
	poly->AddPoint(spacevector(  t,  t,  t)); //12
	poly->AddPoint(spacevector( -t,  t,  t)); //13
	poly->AddPoint(spacevector( -t, -t,  t)); //14
	poly->AddPoint(spacevector(  t, -t,  t)); //15
	poly->AddPoint(spacevector(  t,  t, -t)); //16
	poly->AddPoint(spacevector( -t,  t, -t)); //17
	poly->AddPoint(spacevector( -t, -t, -t)); //18
	poly->AddPoint(spacevector(  t, -t, -t)); //19

	poly->AddFace("0 1 12 5 15");
	poly->AddFace("1 0 14 6 13");
	poly->AddFace("2 3 19 4 16");
	poly->AddFace("3 2 17 7 18");
	poly->AddFace("0 15 9 8 14");
	poly->AddFace("8 9 19 3 18");
	poly->AddFace("12 1 13 10 11");
	poly->AddFace("11 10 17 2 16");
	poly->AddFace("5 12 11 16 4");
	poly->AddFace("5 4 19 9 15");
	poly->AddFace("6 13 10 17 7");
	poly->AddFace("6 7 18 8 14");
	 
	Net *net=new Net;
	net->basenet=poly;
	net->TotalUnwrap();
	return net;
}

//---------------------------------
////! Define all the points in a Dodecahedron net.
//Net *makeDodecahedronNet(double ww,double hh)
//{
//	Net *net=new Net();
//	makestr(net->thenettype,"dodecahedron");
//	double s,a,b,c,h,v,i,j,k,q,r,m,n,tau,l;
//	s=10;
//	tau=(sqrt(5.)+1)/2;
//	a=s*cos(54/180.*3.1415926535);
//	b=s*sin(72/180.*3.1415926535);
//	c=s/2*(1+cos(36/180.*3.1415926535))/sin(36/180.*3.1415926535);
//	h=c*tau*tau;
//	//v=s*(3*tau*tau+cos(72/180.*3.1415926535));
//	v=s/2*(7*tau+5);
//	i=s/2/tau;
//	j=s/2;
//	k=s*tau/2;
//	q=s*tau*tau/2;
//	r=s*tau;
//	m=s/2*(tau+2);
//	n=s*tau*tau*tau/2;
//	l=s*tau*tau*tau;
//
//	// total height=v, total width=h
//	
//	 // make points
//	net->np=38;
//	net->points=new flatpoint[38];
//	net->pointmap=new int[38];
//	net->points[0]=flatpoint(c,l);         net->pointmap[ 0]=0;
//	net->points[1]=flatpoint(b,l-k);       net->pointmap[ 1]=1;
//	net->points[2]=flatpoint(c,l-r);       net->pointmap[ 2]=2;
//	net->points[3]=flatpoint(a,l-q);       net->pointmap[ 3]=1;
//	net->points[4]=flatpoint(0,l/2);       net->pointmap[ 4]=3;
//	net->points[5]=flatpoint(a,q);         net->pointmap[ 5]=4;
//	net->points[6]=flatpoint(c,r);         net->pointmap[ 6]=5;
//	net->points[7]=flatpoint(b,k);         net->pointmap[ 7]=4;
//	net->points[8]=flatpoint(c,0);         net->pointmap[ 8]=6;
//	net->points[9]=flatpoint(h-c,i);       net->pointmap[ 9]=7;
//	net->points[10]=flatpoint(h-c,q);      net->pointmap[10]=8;
//	net->points[11]=flatpoint(h-b,j);      net->pointmap[11]=7;
//	net->points[12]=flatpoint(h,k);        net->pointmap[12]=9;
//	net->points[13]=flatpoint(h,m);        net->pointmap[13]=10;
//	net->points[14]=flatpoint(h-b,l/2);    net->pointmap[14]=11;
//	net->points[15]=flatpoint(h,l-m);      net->pointmap[15]=10;
//	net->points[16]=flatpoint(h,l-k);      net->pointmap[16]=12;
//	net->points[17]=flatpoint(h-b,l-j);    net->pointmap[17]=13;
//	net->points[18]=flatpoint(h-c,l-q);    net->pointmap[18]=14;
//	net->points[19]=flatpoint(h-c,l-i);    net->pointmap[19]=13;
//	net->points[20]=flatpoint(h-b,v-l+k);  net->pointmap[20]=12;
//	net->points[21]=flatpoint(h-c,v-l+r);  net->pointmap[21]=15;
//	net->points[22]=flatpoint(h-a,v-l+q);  net->pointmap[22]=12;
//	net->points[23]=flatpoint(h,v-l/2);    net->pointmap[23]=10;
//	net->points[24]=flatpoint(h-a,v-q);    net->pointmap[24]=9;
//	net->points[25]=flatpoint(h-c,v-r);    net->pointmap[25]=16;
//	net->points[26]=flatpoint(h-b,v-k);    net->pointmap[26]=9;
//	net->points[27]=flatpoint(h-c,v);      net->pointmap[27]=7;
//	net->points[28]=flatpoint(c,v-i);      net->pointmap[28]=6;
//	net->points[29]=flatpoint(c,v-q);      net->pointmap[29]=17;
//	net->points[30]=flatpoint(b,v-j);      net->pointmap[30]=6;
//	net->points[31]=flatpoint(0,v-k);      net->pointmap[31]=4;
//	net->points[32]=flatpoint(0,v-m);      net->pointmap[32]=3;
//	net->points[33]=flatpoint(b,v-l/2);    net->pointmap[33]=18;
//	net->points[34]=flatpoint(0,v-l+m);    net->pointmap[34]=3;
//	net->points[35]=flatpoint(0,v-l+k);    net->pointmap[35]=1;
//	net->points[36]=flatpoint(b,v-l+j);    net->pointmap[36]=0;
//	net->points[37]=flatpoint(c,v-l+q);    net->pointmap[37]=19;
//	//for (int c=0; c<net->np; c++) cout <<"Dodecahedron point "<<c<<":"<<net->points[c].x<<','<<net->points[c].y<<endl;
//
//	 // make lines
//	net->nl=4;
//	net->lines=new NetLine[net->nl];
//	net->lines[0].Set(net->np,1);
//	net->lines[1].Set("2 6 10 14 18 2");
//	net->lines[2].Set("0 19");
//	net->lines[3].Set("21 25 29 33 37 21");
//	
//	 // make months
//	net->nf=12;
//	net->faces=new NetFace[net->nf];
//
//	net->faces[0].Set("29 33 37 21 25");
//	net->faces[1].Set("0 19 20 21 37");
//	net->faces[2].Set("34 35 36 37 33");
//	net->faces[3].Set("30 31 32 33 29");
//	net->faces[4].Set("26 27 28 29 25");
//	net->faces[5].Set("22 23 24 25 21");
//	net->faces[6].Set("10 14 18 2 6");
//	net->faces[7].Set("3 4 5 6 2");
//	net->faces[8].Set("7 8 9 10 6");
//	net->faces[9].Set("11 12 13 14 10");
//	net->faces[10].Set("15 16 17 18 14");
//	net->faces[11].Set("19 0 1 2 18");
//	
//	net->FindBBox();
//
//	 // Adjust net matrix so it fits in a rectangle with width and height ww,hh
//	SomeData d;
//	d.maxx=ww;
//	d.maxy=hh;
//
//	//DBG cerr <<"******* dodechaedron plain: **********"<<endl;
//	//DBG net->dump_out(stderr,0,0);
//
//	
//	net->FitToData(&d,ww*.05);
//	net->ApplyTransform();
//
//	//DBG cerr <<"******* dodechaedron after: **********"<<endl;
//	//DBG net->dump_out(stderr,0,0);
//
//		
//	return net;
//}



