//
// $Id$
//	
// Laidout, for laying out
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


#include <cstdio>
#include <X11/Xlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "epsutils.h"
#include "lax/strmanip.h"

#include <iostream>
using namespace std;
#define DBG

using namespace Laxkit;


//! From the open file f, get the bounding box, and the preview, title and creation date (if present).
/*!
 * Turns preview to a new'd char[], with same depth and data as the preview if any. 
 * This assumes the EPS is really an EPSI, and the preview data structured according
 * to that specification. The preview data returned is packed by rows with no padding.
 *
 * title, date, and preview are also turned into new'd char[], or NULL if none present. The previous
 * contents of those variables are all ignored, so beware of memory hole potential here.
 *
 * Return -1 for not a readable EPS. -2 for error during some point in the reading.
 * 0 for success.
 *
 * \todo In future, must also be able to extract any resources that must go
 *   at the top of an including ps file.
 * \todo allow the return of an error string
 */
int scaninEPS(FILE *f, Laxkit::DoubleBBox *bbox, char **title, char **date, 
		char **preview, int *depth, int *width, int *height)
{

		//EPS file structure basics:
		//Required:
		// %!PS-Adobe-3.0 EPSF-3.0
		// %%BoundingBox: llx lly urx ury
		//Other eps 3.0 possible header comments:
		// %%EndComments <-- the preview comes right after this
		// %%Extensions
		// %%LanguageLevel
		// %%***** resource related stuff for fonts files, forms, patterns, procsets, "any other"
		//  %%DocumentNeededResources:
		//  %%DocumentNeededFonts:
		//recommended comments:
		// %%Title:
		// %%CreationDate:
		// %%Creator:

	if (!f) return -1;
	
	 // initialize the return vars
	*title=*date=*preview=NULL;
	
	int c;
	int languagelevel=3;
	char *line=NULL;
	size_t n=0;
	char error=0;

	
	c=getline(&line,&n,f); // c==0 is either error or eof
	if (!c) return -2; // corrupt file

	stripws(line,2);
	float psversion,epsversion;
	c=sscanf(line,"%%!PS-Adobe-%f EPSF-%f",&psversion,&epsversion);
	cout <<"sscanf="<<c<<",  ps:"<<psversion<<" eps:"<<epsversion<<endl;
	if (c!=2 || epsversion>psversion || psversion>3 || epsversion>3) {
		 // not a readable eps
		cout <<"***header line ps:"<<psversion<<" eps:"<<epsversion<<endl;
		if (line) free(line);
		return -3;
	}
	
	while (c=getline(&line,&n,f), c>0) { 
		if (!strncmp(line,"%%BoundingBox:",14)) {
			if (sscanf(line,"%%%%BoundingBox: %lf %lf %lf %lf",
						&bbox->minx,&bbox->miny,&bbox->maxx,&bbox->maxy)==4) {
				//***do not convert to inches, keep as ps units?
				//bbox->minx/=72.; //convert to inches
				//bbox->maxx/=72.;
				//bbox->miny/=72.;
				//bbox->maxy/=72.;
				if (width)  *width =int(bbox->maxx-bbox->minx);
				if (height) *height=int(bbox->maxy-bbox->miny);
			} else {
				error=-4;
				break;
				//***error, corrupt bounding box line
			}
			continue;
		} else if (!strncmp(line,"%%Title:",8)) {
			 //*** should really check that Title and other fields are only provided
			 //once? to check for malformed eps?
			makestr(*title,line+8);
			stripws(*title);
			if (!strlen(*title)) { delete[] *title; *title=NULL; }
			continue;
		} else if (!strncmp(line,"%%LangaugeLevel:",16)) {
			sscanf(line,"%%%%LangaugeLevel: %d",&languagelevel);
			continue;
		} else if (!strncmp(line,"%%CreationDate:",15)) {
			makestr(*date,line+15);
			stripws(*date);
			if (!strlen(*date)) { delete[] *date; *date=NULL; }
			continue;
		} else if (!strncmp(line,"%%BeginPreview:",15)) {
			 //%%BeginPreview width height depth lines
			 //
			 //scan in the EPSI formatted preview
			 //This occurs right after the %%EndComments, and before the prolog.
			 //Each scan line of the preview (width dots with given depth) must be given in
			 //segments that are multiples of 8 bits. If the end of a scan line falls short of
			 //that, then the source data is padded with 0 bits. Note that each hex char
			 //represents 4 bits.
			 //

			int nn,lines;
			nn=sscanf(line,"%%%%BeginPreview: %d %d %d %d", width, height, depth, &lines);
			if (nn!=4) {
				DBG cout <<"corrupt preview in EPS at %%BeginPreview line"<<endl;
				break;
			}

			DBG cout <<"w,h,d,l:" <<*width<<"x"<<*height<<"  d:"<<*depth<<" l:"<<lines<<endl;
			
		
			int plen=*width * *height * *depth/8+1; //expected number of bytes of preview data
			*preview=new char[plen];
			
			
			int lpos=0,           //index into line
				ppos=0,           //index into preview
				hlen=0,           //hex char length of each scan line of preview
				cpos=0,           //position in scan line by hex chars
				paddingbits;      //number of bits each scan line is padded
			unsigned int byte=0,  //buffer to hold incoming bits
						 bits=0;  //number of bits read in so far (reset after each 8)
			unsigned char ch;

			hlen=((*width * *depth-1)/8+1)*2; //num chars per line
			paddingbits=hlen*4 - *width * *depth;
			
			DBG int linessofar=0;
				
			 //now for each line, convert "%%ab35f12..." to binary data in preview
			 //by building a byte from bit data, and tacking the bit to preview.
			for ( ; lines && !error; lines--) {
				//DBG cout <<endl;

				c=getline(&line,&n,f);
				
				DBG linessofar++;
			
				if (c<=0) {
					error=-5;
					break; //*** premature end of data!
				}
				if (!strncmp(line,"%%EndPreview",12)) break;
				
				lpos=0;
				while (line[lpos]=='%') lpos++;
				if (lpos==0) continue; //***note this shouldn't happen maybe?
				while (isspace(line[lpos])) lpos++;

				DBG cout <<"begin scan line, bits="<<bits<<endl;
				
				nn=strlen(line);
				while (lpos<nn) { //loop till eol
					 // EPSI spec says to ignore all whitespace and other non-hex digit
					while (line[lpos] && !isxdigit(line[lpos])) lpos++;
					if (lpos==nn) continue;

					 //add to byte
					ch=line[lpos++]; 
					cpos++;
					byte<<=4;
					if (isdigit(ch)) byte|=ch-'0';
					else byte|=tolower(ch)-'a'+10;
					bits+=4;

					DBG cout <<ch<<" bits:"<<bits<<"  byte:"<<byte<<"  cpos:"<<cpos<<"  l:"<<linessofar<<endl;
					
					if (cpos==hlen && paddingbits) {
						 // full scanline read, possible padding to deal with
						 
						//DBG printf(" eoscanline, lsofar:%d\n",linessofar);
						DBG printf(" eoscanline, bits=%d\n",bits);
						
						byte>>=paddingbits;
						bits-=paddingbits;
						cpos=0;
						
					}

					if (bits>=(unsigned)8+paddingbits) {
						(*preview)[ppos++]=byte>>(bits-8);
						//DBG printf("%x",(*preview)[ppos-1]);
						DBG printf(".");
						
						if (bits-8) {
							bits-=8;
							byte=byte&((1<<bits)-1);
						} else { byte=0; bits=0; }
					}
					
					if (ppos>=plen) {
						cout <<" *** too much data in EPS preview!"<<endl;
						error=-3;
						break;
					}
				}
			}
			if (bits) {
				cout<<"*** add on final bits: todo"<<endl;
			}
			if (ppos<plen-1) {
				cout <<" *** less preview data than expected (ppos="<<ppos<<", plen="<<plen<<") in EPS..."<<endl;
			//} else if (error) {***
			}
		} else if (!strncmp(line,"%%BeginProlog",13)) { 
			break; // stop scanning at the beginning of prolog. Preview is just before prolog.
		} //else break;
		//} //***??else if (!strncmp(line,"%%Page:",7)) break;
		
	}

	if (line) free(line);
	return 0;
}

//! Convert EPSI preview data to an Imlib_Image.
/*! Return 0 for success, nonzero error.
 *
 * preview has width*height samples of depth bits, packing sequentially with no padding.
 */
Imlib_Image EpsPreviewToImlib(const char *preview, int width, int height, int depth)
{
	Imlib_Image imlibimage=imlib_create_image(width,height);
	imlib_context_set_image(imlibimage);
	imlib_image_set_format("png");
	DATA32 *buf=imlib_image_get_data();
	
	 // create an Imlib_Image based on the preview data.
	unsigned int pos=0,    //index to buf
			 	 ppos=0,   //index to preview
			 	 t,        //temporary variable
				 sample, ss,
				 d=0,      //dangling bits
				 nd=0,     //number of dangling bits
				 nbuf;     //size of buf
	nbuf=width*height;
	
	DBG int cc=0;
	do { //one iteration per sample
		sample=0;
		t=depth;

		 //insert dangling bits
		if (nd) {
			if (nd<t) while (t && nd) {
				sample|=d&(1<<(nd-1));
				nd--;
				t--;
			} else {
				while (t) {
					sample=(sample<<1)|((d&(1<<(nd-1)))>>(nd-1));
					nd--;
					t--;
				}
			}
		}

		 //insert next whole bytes
		while (t>=8) {
			sample=(sample<<8)|preview[ppos++];
			//DBG printf("%x",sample);
			t-=8;
		}

		 //insert final bits that do not form a whole byte in preview
		if (t) {
			d=preview[ppos++];
			//DBG printf("%x",d);
			nd=8-t;
			sample<<=t;
			while (t) {
				sample|=(d&(1<<(8-t)))>>nd;
				t--;
			}
		}

		 // make sample 8bit
		if (depth<8) {
			if ((sample&1)==0) sample<<=(8-depth);
			else {
				t=8-depth;
				while (t) {
					sample<<=1;
					sample|=1;
					t--;
				}
			}
		} else if (depth>8) sample>>=(depth-8);
		
		 //insert pixel
		ss=sample;
		//sample=ss|(ss<<8)|(ss<<16)|(ss<<24);
		//sample=ss|(ss<<8)|(ss<<16)|(255<<24);
		sample=(ss|(ss<<8)|(ss<<16))^~0; //swap black for white as per EPSI spec

		DBG printf("%x",(sample&0xffffff)?1:0);
		buf[pos++]=sample;

		DBG cc++;
		DBG if (cc==width) { printf("\n"); cc=0; }
	} while (pos<nbuf);

	imlib_image_put_back_data(buf);
	//imlib_save_image("epspreview.png");
	
	DBG cout <<endl;
	return imlibimage;
}

//! Have Ghostscript make epsfile into a smaller png previewfile.
/*! If error!=NULL, then put a new'd char[] with the error message,
 * or NULL if there was no error. The previous contents of error are ignored.
 *
 * Return 0 for success or non-zero for error.
 */
int WriteEpsPreviewAsPng(const char *fullgspath,
						 const char *epsfile, int epsw, int epsh,
						 const char *previewfile, int maxw, int maxh,
						 char **error_ret)
{
	if (error_ret) *error_ret=NULL;
	
	if (!fullgspath || !epsfile || !previewfile) return 1;
	
	 //figure out decent preview size. If maxw and/or maxh are greater than 0, then
	 //generate preview via:
 	 //* gs -dNOPAUSE -sDEVICE=pngalpha -sOutputFile=temp234234234.png
	 //     -dBATCH -r(resolution) whatever.eps
	double dpi,t;
	dpi=maxw ? maxw*72./epsw : 200;
	t  =maxh*72./epsh;
	if (maxh && t && t<dpi) dpi=t;
	
	char *arglist[10], str1[20], str2[300];
	arglist[0]="gs";
	arglist[1]="-dNOPAUSE";
	arglist[2]="-dBATCH";
	arglist[3]="-dEPSCrop";
	arglist[4]="-sDEVICE=pngalpha";

	sprintf(str1,"-r%f",dpi);
	arglist[5]=str1;

	sprintf(str2,"-sOutputFile=%s",previewfile);
	arglist[6]=str2;

	arglist[7]=const_cast<char *>(epsfile);
	arglist[8]=NULL;
		 
	DBG cout <<"Creating preview via:    "<<fullgspath<<' ';
	DBG for (int c=1; c<8; c++) cout <<arglist[c]<<' ';
	DBG cout <<endl;
	
	pid_t child=fork();
	if (child==0) { // is child
		execv(fullgspath,arglist);
		cout <<"*** error in exec!"<<endl;
		if (error_ret) *error_ret=newstr("Error trying to run Ghostscript.");
		exit(1);
	} 
	int status;
	waitpid(child,&status,0);
	if (!WIFEXITED(status)) {
		DBG cout <<"*** error in child process, not returned normally!"<<endl;
		if (error_ret) *error_ret=newstr("Ghostscript interrupted from making preview.");
	} else if (WEXITSTATUS(status)!=0) {
		DBG cout <<"*** ghostscript returned error while trying to make preview"<<endl;
		if (error_ret) *error_ret=newstr("Ghostscript had error while making preview.");
	}

	if (*error_ret) return -3;
	return 0;
}


