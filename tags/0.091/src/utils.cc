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
// Copyright (C) 2007 by Tom Lechner
//

#include "language.h"
#include "utils.h"
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/laximages.h>
#include <lax/misc.h>

#include <cctype>
#include <cstdlib>

 //for freedesktop thumbnail md5 names:
#include <openssl/evp.h>
#include <openssl/md5.h>


#include <iostream>
#define DBG

using namespace LaxFiles;
using namespace Laxkit;
using namespace std;



//! Return a roughly unique id. Uniqueness is not guaranteed!
/*! Say base=="blah" then something like "blah12" will be returned.
 *
 * This currently uses Laxkit::getUniqueNumber(), and simply appends it to base.
 * Please note that if you load in something, either a laidout document or a resource,
 * it is possible to have name collision.
 */
char *make_id(const char *base)
{
	if (!base) base="id";
	char *str=new char[strlen(base)+30];
	sprintf(str,"%s%ld",base,getUniqueNumber());
	return str;
}

//! Return a pointer to a char[] akin to "untitled 1", "untitled 2", etc.
/*! The number is based on a static int stored in the function.
 *
 * The name here is stored in a static buffer, so it should be used quickly,
 * before some other call increments the count.
 */
const char *Untitled_name()
{
	static int count=1;
	static char *name=new char[strlen(_("untitled"))+10];
	sprintf(name,_("untitled %d"),count);
	count++;
	return name;
}

	
//! Simplify opening any sort of file for writing, does basic error checking, such as for existence, and writability.
/*! \ingroup misc
 *
 * If nooverwrite, then do not overwrite existing files.
 *
 * Returns the opened file, or NULL if file cannot be opened for writing,
 * and error_ret gets set to a proper error message if *error_ret was NULL,
 * or appended to error_ret if *error_ret!=NULL. Beware of this!! If you start
 * with a blank error, then remember to set it to NULL before calling this function!
 *
 * This assumes that the normal locale is in effect.
 */
FILE *open_file_for_writing(const char *file,int nooverwrite,char **error_ret)
{
	int exists=file_exists(file,1,NULL);
	if (exists && exists!=S_IFREG) {
		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot write to %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}
	
	if (exists && nooverwrite) {
		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot overwrite %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}

	FILE *f=fopen(file,"w");

	if (!f) {
		//DBG cerr <<"**** cannot load, "<<(file?file:"(nofile)")<<" cannot be opened for writing."<<endl;

		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot write to %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}

	return f;
}
	
//! Simplify opening any sort of file for reading, does basic error checking, such as for existence, and readability.
/*! \ingroup misc
 *
 * Returns the opened file, or NULL if file cannot be opened for reading,
 * and error_ret gets set to a proper error message if *error_ret was NULL,
 * or appended to error_ret if *error_ret!=NULL. Beware of this!! If you start
 * with a blank error, then remember to set it to NULL before calling this function!
 *
 * This assumes that the normal locale is in effect.
 */
FILE *open_file_for_reading(const char *file,char **error_ret)
{
	if (file_exists(file,1,NULL)!=S_IFREG) {
		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot read the file %s. Wrong type."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}
	
	FILE *f=fopen(file,"r");

	if (!f) {
		//DBG cerr <<"**** cannot load, "<<(file?file:"(nofile)")<<" cannot be opened for reading."<<endl;

		if (error_ret) {
			char scratch[strlen(file)+60];//****this 60 is likely to cause problems!!
			sprintf(scratch, _("Cannot read file %s."), file);
			appendstr(*error_ret,scratch);
		}
		return NULL;
	}

	return f;
}
	

//! Simplify opening Laidout specific files, does basic error checking, such as for existence, and readability.
/*! \ingroup misc
 *
 * what is the type of file, for instance "Project" or "Document".	
 * Returns the opened file, or NULL if file cannot be opened for reading,
 * and error_ret gets set to a proper error message if *error_ret was NULL,
 * or appended to error_ret if *error_ret!=NULL. Beware of this!! If you start
 * with a blank error, then remember to set it to NULL before calling this function!
 *
 * This assumes that the normal locale is in effect.
 *
 * The file pointer will point to the very beginning of the file on return.
 */
FILE *open_laidout_file_to_read(const char *file,const char *what,char **error_ret)
{
	FILE *f=open_file_for_reading(file,error_ret);
	if (!f) return NULL;

	 // make sure it is a laidout file!!
	char first100[100];
	int n=fread(first100,1,100,f);
	first100[n-1]='\0';
	int err=1;
	if (!strncmp(first100,"#Laidout ",9)) {
		char *version=first100+9;
		int c=9,c2=0,c3;
		while (c<n && isspace(*version) && *version!='\n') { version++; c++; }
		while (c<n && !isspace(version[c2])) { c2++; c++; }
		 //now the laidout version of the file is in version[0..c2)
		c3=c2;
		while (c<n && isspace(version[c3])) { c3++; c++; }
		if (!strncmp(version+c3,what,strlen(what)) && isspace(version[c3+strlen(what)])) err=0;
	}
	if (err) {
		if (error_ret) {
			*error_ret=new char[strlen(file)+strlen(what)+100];//****this definite 100 might cause problems!!
			sprintf(*error_ret, _("%s does not appear to be a Laidout %s file."), file, what);
		}
		fclose(f);
		return NULL;
	}
	rewind(f);
	return f;
}

//! Retrieve name and description from a resource file.
/*! This partially reads from a file for "name" and "description" attributes.
 *
 * Return 0 for name and description not found.
 * Return 1 for only name found.
 * Return 2 for only description found.
 * Return 3 for both name and description found.
 *
 * If a name or description attribute is found, then return a new char[] with
 * the corresponding value.
 */
int resource_name_and_desc(FILE *f,char **name, char **desc)
{
	if (!f) return 0;
	if (!name && !desc) return 0;
	Attribute att, *patt=NULL;

	int ret=0;
	while (ret!=3 && !feof(f)) {
		att.dump_in(f,0,&patt);
		if (!patt) break; //no more in file
		if (name && !(ret&1) && !strcmp(patt->name,"name")) {
			*name=newstr(patt->value);
			ret|=1;

		} else if (desc && !(ret&2) && !strcmp(patt->name,"description")) {
			*desc=newstr(patt->value);
			ret|=2;
		}
		skip_to_next_attribute(f,0);
	}
	return ret;
}


//! Create a preview file name based on a name template and the absolute path in file.
/*! \ingroup misc
 *
 * An initial "~/" will expand to the user's $HOME environment variable.
 * 
 * From something like "%-s.png", transform a file like "/blah/2/file.tiff"
 * to "/blah/2/file-s.png". The template can be an absolute path, or relative.
 * If not in same dir as the image, then it is ok to have a relative path from there,
 * such as "../thumbs/%-s.png". If the template is "/tmp/thumbs/%-s.png", then that
 * absolute path is used. A template like "*.jpg" uses the whole filename, so
 * file.tiff would become "file.tiff.jpg". 
 *
 * Also, nametemplate may have '@', which will be replaced by the name adhering to the  
 * freedesktop.org thumbnail management specification, which calls for having 128x128 or smaller previews
 * in ~/.thumbnails/normal/@, and up to 256x256 in ~/.thumbnails/large/@.
 *   
 * There should be only one of '%', '@' or '*'. Any such characters after the first one
 * will be replaced by a '-' character. If there are none of those characters, then assume
 * nametemplate is just a prefix to tack onto file, thus "path/to/file.jpg" with a template of
 * "blah." will return "path/to/blah.file.jpg". In this case if there are further '/' chars 
 * in nametemplate, they are converted to '-' chars, so just be sure to include a proper wildcard.
 *
 * Note that this does not check the filesystem for existence or not of the generated preview
 * name. Those duties lie elsewhere.
 *
 * \todo *** maybe should do check to make sure file is an absolute path...
 * \todo **** should probably keep relative templates as relative files...
 */
char *previewFileName(const char *file, const char *nametemplate)
{
	if (!file || !nametemplate) return NULL;
	
	const char *b=lax_basename(file);
	if (!b) return NULL;

	char *path=NULL;
	char *bname=NULL;

	 //fix up nametemplate
	char *tmplate=new char[strlen(nametemplate)+5];
	strcpy(tmplate,nametemplate);
	
	 //set bname to the thing to be placed in the template wildcard
	 //and replace the wildcard in tmplate with "%s" to be used in 
	 //later sprintf
	char *tmp=NULL;
	//------------**** wtf is wrong with strpbrk!!!! OMFG!!!
	//tmp=strpbrk(tmplate,"%*@");
	//-----------
	tmp=strchr(tmplate,'%');
	if (!tmp) tmp=strchr(tmplate,'*');
	if (!tmp) tmp=strchr(tmplate,'@');
	//------------
	if (tmp) { //found a wildcard	
		char c=*tmp        ;//the wildcard
		int pos=tmp-tmplate;
		replace(tmplate,"%s",tmp-tmplate,tmp-tmplate,NULL);//tmplate different afterwards!!
		char *tmp2=tmplate+pos+1;

		 //remove extraneous wildcard chars
		//------
		//while (tmp=strpbrk(tmp+1,"%*@"),tmp) *tmp='-';
		//------
		do {
			tmp=strchr(tmp2,'%');
			if (!tmp) tmp=strchr(tmp2,'*');
			if (!tmp) tmp=strchr(tmp2,'@');
			if (tmp) *tmp='-';
			tmp2=tmp+1;
		} while (tmp);
		//------------
		
		if (c=='@') {
			 //bname gets something like "83ab3492fa02f3bcd23829eaf2837243.png"
			//*******note if file is not an absolute path, this will crash
			char *str=file_to_uri(file);
			char *h;
			unsigned char md[17];
			bname=new char[40];
			
			MD5((unsigned char *)str, strlen(str), md);
			h=bname;
			for (int c2=0; c2<16; c2++) {
				sprintf(h,"%02x",(int)md[c2]);
				h+=2;
			}
			strcat(bname,".png");
			delete[] str;
		} else if (c=='%') { //chop suffix in bname
			bname=newstr(b);
			tmp=strrchr(bname,'.');
			if (tmp && tmp!=bname) *tmp='\0';
		} else { //'*'
			bname=newstr(b);
		}
	} else { //no "%*@" found
		//***is this even rational: 
		//  if template was "/a/b/c" then make it "-a-b-c%s", 
		//  a file "/d/e/f/blah.jpg" becomes "/d/e/f/-a-b-cblah.jpg"
		while (tmp=strchr(tmplate,'/'),tmp) *tmp='-';//removes template path identifiers
		bname=newstr(b);
		appendstr(tmplate,"%s");
	}
	
	if (tmplate[0]=='~' && tmplate[1]=='/') expand_home_inplace(tmplate);
	if (tmplate[0]!='/') path=lax_dirname(file,1); else path=NULL;
		
	char *previewname=new char[strlen(bname)+strlen(tmplate)];
	sprintf(previewname,tmplate,bname);
	if (path) {
		prependstr(previewname,path);
		delete[] path;
	}
	delete[] bname;
	delete[] tmplate;
	simplify_path(previewname,1);
	return previewname;
}

//------------------------------ File identification functions -------------------------------

////! Find out the version and type of Laidout file this is, if any.
//int laidout_file_type(const char *file, char **version, char **typ)

//! Check if the file is a Laidout type typ, with a version [minversion,maxversion].
/*! \ingroup misc
 * If typ==NULL, then 0 is always returned as long as the file starts out something 
 * like "#Laidout 0.08". If actual_type!=NULL, then *actual_type=new char[] with the
 * actual Laidout type of the file, such as "Project" or "Document".
 *
 * Return 0 if the file is a Laidout file and the type is the correct type.
 *
 * \todo implement the version check, and watch out for locale snafus
 */
int laidout_file_type(const char *file, 
					  const char *minversion, const char *maxversion,
					  char **actual_version,
					  const char *typ, char **actual_type)
{
	if (file_exists(file,1,NULL)!=S_IFREG) return 1;

	FILE *f=fopen(file,"r");
	if (!f) return 2;
	
	char first100[100];
	int n=fread(first100,1,100,f);
	first100[n-1]='\0';
	int err=1;
	if (!strncmp(first100,"#Laidout ",9)) {
		char *version=first100+9;
		int c=9,c2=0,c3;
		while (c<n && isspace(*version) && *version!='\n') { version++; c++; }
		while (c<n && !isspace(version[c2])) { c2++; c++; }
		if (c2 && actual_version) *actual_version=newnstr(version,c2);
		
		 //now the laidout version of the file is in version[0..c2)
		//*** check: check for a min, max, OR min AND max
		//if (minversion && maxversion) ***=laidout_version_check(version, minversion, maxversion);

		c3=c2;
		while (c<n && isspace(version[c3])) { c3++; c++; }
		if (!typ || (!strncmp(version+c3,typ,strlen(typ)) && isspace(version[c3+strlen(typ)]))) err=0;
		if (actual_type) {
			c2=c3;
			while (c<n && !isspace(version[c2])) { c2++; c++; }
			*actual_type=newnstr(version+c3,c2-c3);
		}
	}

	fclose(f);
	return err;
}

/*! \ingroup misc
 * Versions should be something like 0.05.1, or 0.05.custom.
 * This function merely checks the first part of the version string that can be converted
 * to a floating point number. If the version number cannot be so parsed, then 0 will be returned.
 *
 * Otherwise, 1 will be returned if version within bounds, else 0.
 *
 * \todo currently this assumes the version can be converted to a real number (such as 0.05 above),
 *   so any custom version should conform to that. 0.05.1, 0.05.custom, and 0.05.2 will convert to the same
 *   value, so watch out! 
 */
int laidout_version_check(const char *version, const char *minversion, const char *maxversion)
{
	float v,min,max;
	char *end;
	
	v=strtof(version,&end);
	if (end==version) return 0;

	min=strtof(minversion,&end);
	if (end==minversion) return 0;

	max=strtof(maxversion,&end);
	if (end==maxversion) return 0;

	return v>=min && v<=max;
}

//! Return string identifying the type of file.
/*! \ingroup misc
 *
 * Currently, this checks for EPS, OFF, and Laidout files.
 *
 * EPS files return "EPS", with version1==postscript version, version2=eps version.
 *
 * OFF files return "OFF", and the versions are NULL.
 *
 * Laidout files return "Laidout" with version1 the Laidout version, 
 * and version2 the specific type of Laidout file, such as "Project",
 * "Document", "Impostion", etc.
 *
 * Returns NULL for file not specifically identified.
 *
 * Please note that this is definitely not related to mime types. This function is used
 * mainly to identify the types of files that Laidout can readily load.
 */
const char *IdentifyFile(const char *file, char **version1, char **version2)
{
	float v1,v2;
	if (isEpsFile(file,&v1,&v2)) {
		if (version1) *version1=numtostr(v1);
		if (version2) *version2=numtostr(v2);
		return "EPS";
	}
	if (isOffFile(file)) return "OFF";
	if (laidout_file_type(file, NULL, NULL, version1, NULL, version2)) return "Laidout";
	return NULL;
}

//! Return whether file is a Scribus document.
int isScribusFile(const char *file)
{
	FILE *f=fopen(file,"r");
	if (!f) return 0;

	char first100[101];
	int c=fread(first100,1,100,f);
	first100[c]='\0';
	c=-1;

	 //check for the various OFF starts
	 //should have something like <SCRIBUSUTF8NEW Version="..."
	const char *p=strstr(first100,"SCRIBUSUTF8NEW");
	int foundscribus=0;
	if (p) { //extract version...
		foundscribus=1;
		//p=strstr(first100,"Version");
		//if (p) {
		//	while (*p && *p!="\"") p++;
		//	if (*p) {
		//		const char *v=p;
		//		while (*p && *p!="\"") p++;
		//	}
		//}
	}

	fclose(f);
	return foundscribus;
}

//! Return whether file is an OFF file or not.
/*! The OFF file format is originally from geomview.org.
 *
 * \todo this could check a little more accurately
 */
int isOffFile(const char *file)
{
	FILE *f=fopen(file,"r");
	if (!f) return 0;

	char first100[101];
	int c=fread(first100,1,100,f);
	first100[c]='\0';
	c=-1;

	 //check for the various OFF starts
	 //***todo: check for start like this: [ST][C][N][4][n]OFF, see poly.cc for some code..
	int p=0,
		foundoff=0;
	while (p<100 && isalpha(first100[p])) p++;
	if (p) p--;
	if (p>=2 && p<10) {
		if (first100[p-2]=='O' && first100[p-1]=='F' && first100[p]=='F') {
			//***this could be more thorough...
			foundoff=1;
		}
	}
	fclose(f);
	return foundoff;
}

/*! \ingroup misc
 *
 * Return 1 for yes it is or 0 for no it isn't. If psversion or epsversion are
 * not NULL, then return the respective versions.
 */
int isEpsFile(const char *file,float *psversion, float *epsversion)
{
	FILE *f=fopen(file,"r");
	if (f) {
		int n=0;
		char data[51];
		data[0]='\0';
		n=fread(data,1,50,f);
		fclose(f);
		if (n) {
			data[n]='\0';
			 //---check if is EPS
			if (!strncmp(data,"%!PS-Adobe-",11)) { // possible EPS
				float ps,eps;
				n=sscanf(data,"%%!PS-Adobe-%f EPSF-%f",&ps,&eps);
				if (n==2) {
					//DBG cerr <<"--found EPS, ps:"<<ps<<", eps:"<<eps<<endl;

					if (psversion)  *psversion =ps;
					if (epsversion) *epsversion=eps;
					return 1;
				}
			}
		}
	}
	return 0;
}

//! Return whether it is an EPS (returns 2) , or can be opened by Imlib2 (returns 1).
/*! \ingroup misc
 *
 * \todo this should probably be removed in favor of a more robust ImportImageFilter system
 */
int is_bitmap_image(const char *file)
{
	if (isEpsFile(file,NULL,NULL)) return 2;

	Laxkit::LaxImage *img=NULL;
	img=Laxkit::load_image(file);
	if (!img) return 0;
	
	delete img;
	return 1;
}


