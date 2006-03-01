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
