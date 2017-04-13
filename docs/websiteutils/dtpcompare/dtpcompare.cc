#include <lax/attributes.h>
#include <lax/strmanip.h>
#include <lax/lists.cc>
#include <iostream>
#include <sstream>
#include <sys/stat.h>


using namespace Laxkit;
using namespace LaxFiles;
using namespace std;


#define DBG

typedef const char cchar;

class Program 
{ 
 public:
	char *name;
	char *website;
	char *title;
	Program(cchar *n,cchar *w,cchar *t)
	{ 
		name=newstr(n);
		website=newstr(w);
		title=newstr(t); 
	}

};

int main(int argc, char **argv)
{


	Attribute comparison;
	comparison.dump_in("dtpcomparison.txt");

	
	int c=0;
	PtrStack<Program> programs;

	 // read in program info: name, website, title if any;
	c=0;
	Program *program;
	char *name,*url,*title;
	while (1) {
		if (strcasecmp(comparison.attributes.e[c]->name,"program")) break;

		name=url=title=NULL;
		name=comparison.attributes.e[c]->value;
		for (int c2=0; c2<comparison.attributes.e[c]->attributes.n; c2++) {
			if (!strcasecmp(comparison.attributes.e[c]->attributes.e[c2]->name,"title"))
				title=comparison.attributes.e[c]->attributes.e[c2]->value;
			else if (!strcasecmp(comparison.attributes.e[c]->attributes.e[c2]->name,"website"))
				url=comparison.attributes.e[c]->attributes.e[c2]->value;
		}

		programs.push(new Program(name,url,title));
		c++;
	}
	// c now points to starting section attribute
	



	//-------------------------begin file output------------------------------

	 //output header
	cout << 
"<!DOCTYPE html>\n"
"<html lang=\"en-us\">\n"
"<head>\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> \n"
"<meta charset=\"utf-8\">\n"
"\n"
"<meta name=\"description\" content=\"Laidout, desktop publishing software.\">\n"
"<meta name=\"keywords\" content=\"Cartoons, Prints, Drawings, Polyhedra, Polyhedron, Calendar, \n"
"Desktop, Publishing, Page, Layout, Multipage, Booklet, Books, Imposition, Linux, Software,\n"
"Tensor, Product, Patch, Laidout, Folded, Pamphlet, Help,\n"
"Dodecahedron, software, open, source, dtp, signature, editor\">\n"
"\n"
"<title>Laidout</title>\n"
"\n"
"<link rel=\"icon\" href=\"laidout-icon-16x16.png\" type=\"image/png\">\n"
"<link rel=\"stylesheet\" href=\"css/style.css\" type=\"text/css\">\n"

"\n"  //begin table specific style sheet
"<style type=\"text/css\">\n"
"  \n"
"  td.Yes {\n"
"    background: #00ff00;\n"
"    text-align: center;\n"
"  }\n"
"  \n"
"  td.No {\n"
"    background: #ffaaaa;\n"
"    text-align: center;\n"
"  }\n"
"  \n"
"  td.planned {\n"
"    background: #dddddd;\n"
"    text-align: center;\n"
"  }\n"
"  \n"
"  td.partial {\n"
"    background: #ddffdd;\n"
"    text-align: center;\n"
"  }\n"
"  \n"
"  td.plugin {\n"
"    background: #ddffff;\n"
"    text-align: center;\n"
"  }\n"
"  \n"
"  span.Yes {\n"
"    background: #00ff00;\n"
"  }\n"
"  \n"
"  span.No {\n"
"    background: #ffaaaa;\n"
"  }\n"
"  \n"
"  span.planned {\n"
"    background: #dddddd;\n"
"  }\n"
"  \n"
"  span.plugin {\n"
"    background: #ddffff;\n"
"  }\n"
"  \n"
"  span.partial {\n"
"    background: #ddffdd;\n"
"  }\n"
"  td.partial a:hover { background-color:aaddaa; }\n"
"  td.plugin  a:hover { background-color:aadddd; }\n"
"  \n"
"  #thetable {\n"
"    padding: 5;\n"
"  }\n"
"  #thetable {\n"
"  	 border:2px solid #bbb;\n"
"  	 padding:0px;\n"
"  	 border-collapse: collapse;\n"
"  }\n"
"  #thetable td {\n"
"    padding:2px;\n"
"    border-bottom: 1px solid #ddd;\n"
"	 border-left: 1px solid #ddd;\n"
"	 border-right: 0px solid #ccc;\n"
"	 border-top: 0px solid #ccc;\n"
"  } \n"
"  #thetable td.header {\n"
"    background:#ddd;\n"
"    font-weight:bold;\n"
"  }\n"
"  #thetable td.desc {\n"
"    padding-left:1em;\n"
"  }\n"
"  \n"
"  #thetable a {\n"
"    display:block;\n"
"  }\n"
"  #thetable td.desc > a {\n"
"    display: inline;\n"
"  }\n"
"  #thetable tr:hover {\n"
"    background-color: #f0f0f0;\n"
"  }\n"
"</style>\n"
"\n"
"\n"
"</head>\n"
"<body>\n"
"\n"
"<div class=\"whole\">\n"
"\n"
"  <div id=\"side\">\n"
"    <ul class='sidenav'>\n"
"	   <li><a class=\"feeds\" href=\"http://plus.google.com/u/0/b/117350226436132405223/117350226436132405223\"><img src=\"images/gplus.png\" alt=\"Google+\"/></a></li>\n"
"	   <li><a class=\"feeds\" href=\"rss.xml\"><img src=\"images/rss.png\" alt=\"RSS feed\"/></a></li>\n"
"	   &nbsp;&nbsp;\n"
"      <li><a href=\"dev.html\">Dev</a></li>\n"
"      <li><a href=\"index.html#download\">Download</a></li>\n"
"      <li><a href=\"links.html\">Links</a></li>\n"
"      <li><a href=\"faq.html\">FAQ</a></li>\n"
"      <li><a href=\"screenshots/\">Screenshots</a></li>\n"
"      <li><a href=\"index.html\">Home</a></li>\n"
"    </ul>\n"
"  \n"
"\n"
"  </div><!-- side -->\n"
"\n"
"\n"
"\n"
"  <div id=\"main\">\n"
"\n"
"\n" 
"<h1>DTP Features</h1>\n"
"\n"
"<p>\n"
"Here is a brief comparison between a few closed and open source vector graphics programs. It is a\n"
"list of features that are not so much a roadmap as things that any\n"
"self respecting desktop publishing software should have. You will note that not much\n"
"of this is currently implemented in Laidout.<br/>\n"
"<br/>\n"
"THIS TABLE IS FAR FROM COMPLETE! Please send me updates, or post on the <a href=\"https://lists.sourceforge.net/lists/listinfo/laidout-general\">Laidout mailing list</a>\n"
"if you see something that is not correct. You might also check out\n"
" <a href=\"http://wiki.scribus.net/index.php/Export/Import_capabilities_of_Scribus%2C_OpenOffice.org%2C_Inkscape%2C_GIMP%2C_and_Krita\">this comparison</a> \n"
" over on the Scribus site about import and export capabilities of Scribus, OpenOffice.org, Inkscape, GIMP, and Krita. Also \n"
" <a href=\"http://en.wikipedia.org/wiki/Comparison_of_vector_graphics_editors\">this page about vector graphics</a> editors in Wikipedia, \n"
"this <a href=\"http://en.wikipedia.org/wiki/Comparison_of_desktop_publishing_software\">dtp comparison on Wikipedia</a>, and also \n"
"<a href=\"http://en.wikipedia.org/wiki/List_of_desktop_publishing_software\">a plain list</a> of such software.<br/>\n"
"<br/>\n"
"Please note that I have no access to InDesign, Quark, or Illustrator, so the estimation of the capabilities of those programs might be totally wrong.\n"
"They are based mostly on browsing video tutorials for them.\n"
"</p>\n"
"\n"
"<p>* Hover the mouse over a block to see what version the feature first appears in, if known, plus other relevant notes.<br/>\n"
"* If a spot is blank, its status is either not known, or the devs might be vaguely thinking about implementing it some day.<br/>\n"
"<span class=\"partial\">\"partial\"</span> means the feature exists, but leaves much to be desired.<br/>\n"
"<span class=\"planned\">\"planned\"</span> means actual coding has begun on it, but it is not yet functional.<br/>\n"
"<span class=\"plugin\">\"plugin\"</span> means that you can do it in the program, but you need an extra plugin.</p>\n"
;  //end cout header


	 //create program table header
	stringstream programheader;
	for (int c=0; c<programs.n; c++) {
		program=programs.e[c];
		programheader << "  <td class=\"header\"> <a href=\"" << program->website << "\"  ";
		if (!isblank(program->title)) {
			programheader <<"title=\""<<program->title<<"\"";
		}
		programheader << "><font color=\"#000000\">"<<program->name<< "</font></a> </td> \n";
	}



	 //-------process sections


	char *sectionname=NULL;
	int notenumber=1;
	stringstream notestream;

	for (; c<comparison.attributes.n; c++) {
		if (!strcasecmp(comparison.attributes.e[c]->name,"section")) {
			 //start new section
			if (!sectionname) {
				 //first time, create the table
				sectionname=newstr(comparison.attributes.e[c]->value);
				cout <<"<table id=\"thetable\" border=\"1\" cellspacing=\"0\" "
						"bordercolorlight=\"#333300\" bordercolordark=\"#000066\">\n";
			} else {
				 //close off old section
				cout << "<tr><td colspan=\""<< programs.n+1  <<"\">&nbsp;<br/>&nbsp;</td></tr>\n\n";
				makestr(sectionname,comparison.attributes.e[c]->value);
			}
			 //new section
			cout <<	"<tr>\n"
					" <td align=\"center\" class=\"header\">"<<sectionname<<"</td>\n"
					<< programheader.str() <<"</tr>\n";
			notestream<<"<br/>\n";
		} else if (!strcasecmp(comparison.attributes.e[c]->name,"item")) {
			//continue section
			
			Attribute *item,*pinfo,*urlatt,*noteatt;
			item=comparison.attributes.e[c];

			cout <<"<tr><td class=\"desc\">"<<item->value<<"</td>\n";


			char *p,*tag;
			cchar *yesno,*yesnotext=NULL;
			for (int c2=0; c2<programs.n; c2++) {
				 //get yes or no per program in order
				p=programs.e[c2]->name;
				int c3;
				for (c3=0; c3<item->attributes.n; c3++) {
					if (!strcasecmp(p,item->attributes.e[c3]->name)) {
						pinfo=item->attributes.e[c3];
						tag=item->attributes.e[c3]->value;
						title=NULL;
						yesno=NULL;
						if (tag==NULL) {
							yesno=NULL;
						} else if (!strncasecmp(tag,"yes",3)) {
							yesno="Yes";
							title=tag+3;
						} else if (!strncasecmp(tag,"no",2)) {
							yesno="No";
							title=tag+2;
						} else if (!strncasecmp(tag,"planned",7)) {
							yesno="planned";
							title=tag+7;
						} else if (!strncasecmp(tag,"partial",7)) {
							yesno="partial";
							title=tag+7;
						} else if (!strncasecmp(tag,"plugin",6)) {
							yesno="plugin";
							title=tag+6;
						} else yesno=NULL;
						if (title) {
							if (*title==',' || *title==':') title++;
							while (isspace(*title)) title++;
						}

						//makestr(yesnotext,yesno);

						// //**** make text of yes or no be something other then class tag
						//while (isspace(title[0])) title++;
						//if (title[0]=='(');


						if (yesno) {
							cout <<"  <td class=\""<<yesno<<"\"  ";
							 //add title if any
							if (title && *title) cout <<"title=\""<<title<<"\" ";
							cout <<">";
							 //add url --OR-- link to note then ---Yes|No|Planned---
							urlatt=pinfo->find("url");
							noteatt=pinfo->find("note");
							if (!urlatt && !noteatt) cout << yesno;
							else if (urlatt && !noteatt) {
								 //add url link to yesno
								cout <<"<a href=\""<<urlatt->value<<"\">"<<yesno<<"</a>";
							} else {
								 //add note link to yesno
								cout <<"<a name=\"cite"<< notenumber
									 <<"\" href=\"#"<<notenumber<<"\">"<<yesno
									 <<"<sup><span style=\"font-size:70%\">"<<notenumber<<"</span></sup></a>";
								
								notestream <<"<a name=\""<<notenumber<<"\" title=\"Back\" href=\"#cite"<<notenumber<<"\">"
									<<"<strong>["<<notenumber<<"]</a> "
									<<sectionname<<": "<<item->value<<":<br/>\n&nbsp;&nbsp;"
									<<pinfo->name<<"</strong> -- "
									<<"<span class=\""<<yesno<<"\"> &nbsp;"<<yesno<<"&nbsp; </span><br/>"
									<<noteatt->value;
								if (urlatt) {
									notestream <<"<br/><br/><a href=\""<<urlatt->value<<"\">Further info here.</a>\n";
								} 
								notestream<<"<br/><br/>\n";

								notenumber++;
							}

							cout <<"</td>\n";
						} else {
							 // program listed, but is blank
							cout <<"<td class=\"blank\"  > &nbsp;  </td>\n";
						}
						break;
					}
				}
				if (c3==item->attributes.n) {
					 //no info found for program on this item, so draw a blank
					cout <<"<td class=\"blank\"  > &nbsp;  </td>\n";
				}
			} //end add box for each program
			cout <<"</tr>\n";
		}
		
	}
	cout <<"</table>\n";


	//----end process sections

	cout <<
"<!-- ------------------------ -->\n"
"<br><br>\n"
"<strong>Lower level functions and other neat ideas from other programs, though maybe less vital:</strong>\n"
"\n"
"<ul>\n"
"<li>Auto sync keyboard shortcuts with inkscape/gimp/whatever</li>\n"
"<li>Openclipart integration/scrapbook</li>\n"
"<li>cntl-+/- zooms around object, neat!</li>\n"
"<li>Color selector previews different shades of color with black and white text on it!!! Fab!</li>\n"
"<li>The scribus color wheel is very clever, including allowing preview for various kinds of color blindness!</li>\n"
"</ul>\n"
"<br>\n"
"\n";



	//----------output notes;
	cout <<"<hr/>\n <br/> <h2>Notes</h2>\n <p>\n"
		 << notestream.str()
		 <<"</p>\n<br/>\n";







	 //output date of last update
	struct stat st;
	stat("dtpcomparison.txt",&st);
	cout <<"<hr>\n<center><span style=\"font-style:italic; font-size:90%; \">Last updated "<<ctime(&st.st_mtime)<<"</font><br><br></center>\n";
	
	 //output footer
 	cout << "</div><!-- main -->\n"
		 << "</div><!-- whole -->\n"
		 << "</body>\n"
			"</html>";

}

