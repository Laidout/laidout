//
//	
// Laidout, for laying out
// Copyright (C) 2004-2006 by Tom Lechner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
/*********************** psfilters.cc *********************/

/*! \file psfilters.cc
 * Define various encoding filters for use in postscript.
 *
 * Includes:\n
 * Ascii85 encoding
 *
 * \todo Ultimately, perhaps include RunlengthEncode and also make them
 * operate with something resembling streams rather than complete
 * buffers?
 */




#include "psfilters.h"


namespace Laidout {


//--------------------------- Ascii85 encoding --------------------------------

/*! \ingroup postscript
 * Translate in to the Ascii85 encoding, which translates groups
 * of 4 8-bit bytes into 5 ascii characters, from '!'==33 to 'u'==117.
 * Returns the number of bytes written to f.
 *
 * When the 5 new chars are all 0, then 'z' is put, rather than '!!!!!'.
 * If not enough bytes are provided, it is padded with 0.
 *
 * <tt> a1*256^3 + a2*256^2 + a3*256 + a4 = b1*85^4 + b2*85^3 + b3*85^2 + b4*85 + b5</tt>
 *
 * If puteod!=0, then after writing the data, put an extra "~>\\n" at the end.
 *
 * linewidth is rounded to the nearest multiple of 5 above or equal to it.
 */
int Ascii85_out(FILE *f,unsigned char *in,int len,int puteod,int linewidth,int *curwidth)
{
	if (!f) return -1;
	linewidth=((linewidth-1)/5+1)*5;

	unsigned int i;
	unsigned char b1,b2,b3,b4,b5;
	int c=0,w,n=0;
	if (curwidth) w=*curwidth; else w=0;
	while (c<len) {
		 // find the 4-byte chunk
		i=(unsigned char)(in[c++])*256; //first byte
		if (c<len) i|=(in[c++]); //second byte or 0
		i*=256;
		if (c<len) i|=(in[c++]); //third byte or 0
		i*=256;
		if (c<len) i|=(in[c++]); //fourth byte or 0
		
		if (i==0) {
			fprintf(f,"z");
			w++;
			n++;
			if (w>=linewidth) { fprintf(f,"\n"); n++; w=0; }
		} else {
			 // find the 5-number translation
			b1 = i/52200625; // 85^4
			i%= 52200625;
			b2 = i / 614125;   // 85^3
			i %= 614125;
			b3 = i / 7225;     // 85^2
			i %= 7225;
			b4 = i / 85;       // 85^1
			b5 = i % 85;
			
			fputc(33+b1,f);
			w++; if (w>=linewidth) { fprintf(f,"\n"); n++; w=0; }
			
			fputc(33+b2,f);
			w++; if (w>=linewidth) { fprintf(f,"\n"); n++; w=0; }
			
			fputc(33+b3,f);
			w++; if (w>=linewidth) { fprintf(f,"\n"); n++; w=0; }
			
			fputc(33+b4,f);
			w++; if (w>=linewidth) { fprintf(f,"\n"); n++; w=0; }
			
			fputc(33+b5,f);
			w++; if (w>=linewidth) { fprintf(f,"\n"); n++; w=0; }
			
			n+=5;
		}
		if (c>=len && puteod) {
			if (2+w>linewidth) fprintf(f,"\n");
			fprintf(f,"~>\n");
			n+=3;
			w=0;
		} 
	}
	if (curwidth) *curwidth=w;
	return n;
}


//! Take in[4] and make out[5]. Returns the number of chars to use, 1 for 'z' or 5 other ascii chars.
/*! \ingroup postscript
 * \todo *** this is untested
 */
int Ascii85_chars(unsigned char *in,unsigned char *out)
{
	unsigned long i;
	i=(((((in[0]<<8) | in[1])<<8) | in[2])<<8) | in[3];
	
	if (i==0) {
		out[0]='z';
		out[1]=out[2]=out[3]=out[4]=0;
		return 1;
	}

	 // find the 5-number translation
	out[0]=33+ i/52200625; // 85^4
	i%= 52200625;
	
	out[1]=33+ i/614125;  // 85^3
	i %= 614125;
	
	out[2]=33+ i / 7225; // 85^2
	i %= 7225;
	
	out[3]=33+ i / 85;  // 85^1
	
	out[4]=33+ i % 85; // 85^0     
	
	return 5;
}


} // namespace Laidout

