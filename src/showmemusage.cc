//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
/**************** showrusage.cc **********/

#include <iostream>
#include <unistd.h>
#include <sys/resource.h>

void showrusage()
{
	struct rusage rself,rchildren;
	int c=getrusage(RUSAGE_SELF,&rself);
	cout <<"getrusage="<<c<<endl;
	cout <<"--rusageself.ru_ixrss: "<<rself.ru_ixrss<<endl;
	cout <<"--rusageself.ru_idrss: "<<rself.ru_idrss<<endl;
	cout <<"--rusageself.ru_isrss: "<<rself.ru_isrss<<endl;
	cout <<"--rusageself.ru_ixrss: "<<rself.ru_ixrss<<endl;
	cout <<"--rusageself.ru_ixrss: "<<rself.ru_ixrss<<endl;
	cout <<"--rusageself.ru_ixrss: "<<rself.ru_ixrss<<endl;
	cout <<"--rusageself.ru_ixrss: "<<rself.ru_ixrss<<endl;
	c=getrusage(RUSAGE_CHILDREN,&rchildren);
	cout <<"getrusage="<<c<<endl;
	cout <<"--rusagechildren.ru_ixrss: "<<rchildren.ru_ixrss<<endl;
	cout <<"--rusagechildren.ru_idrss: "<<rchildren.ru_idrss<<endl;
	cout <<"--rusagechildren.ru_isrss: "<<rchildren.ru_isrss<<endl;
	cout <<"--rusagechildren.ru_ixrss: "<<rchildren.ru_ixrss<<endl;
	cout <<"--rusagechildren.ru_ixrss: "<<rchildren.ru_ixrss<<endl;
	cout <<"--rusagechildren.ru_ixrss: "<<rchildren.ru_ixrss<<endl;
	cout <<"--rusagechildren.ru_ixrss: "<<rchildren.ru_ixrss<<endl;
}

void showmemusage()
{
	pid=getpid();
	char blah[100];
	sprintf(blah,"more /proc/%d/status",pid);
	system(blah);
}

