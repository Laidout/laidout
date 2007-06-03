#!/usr/bin/python2.4

"""
Using Inkscape, look in icons.svg and take all top items in base
layers of the document that have ids starting with capital letters,
and export the half inch area around them to TheId.png.

Someday I hope to have something to generate the icons internally to
Laidout, the icons should be able to be generated from svg-ish (or
laidout-ish vector specs fast enough to not be irritating maybe.
"""

import commands, sys
#from xml.sax import saxexts
import xml.sax

if (len(sys.argv)>1) : bitmapw=int(sys.argv[1])
else : bitmapw=24
print "Width: "+str(bitmapw)

dpi=int(90.0*bitmapw/45)

#get names
names=[]

depth=0
class SAXtracer:

    def __init__(self,objname):
        self.objname=objname
        self.met_name=""

    def __getattr__(self,name):
        self.met_name=name # UGLY! :)
        return self.trace

    def endElement(self, name):
        globals()["depth"]=globals()["depth"]-1
        #print "depth=",globals()["depth"]

    def startElement(self, name, attrs):
        globals()["depth"]=globals()["depth"]+1
        #print "depth=",globals()["depth"]
  
        if (globals()["depth"]!=3) : return 

        attr_str="\n"
        for attr in attrs:
            if (attr=="id" and attrs[attr][0]>='A' and attrs[attr][0]<='Z') : 
                names.append(attrs[attr])
                print attrs[attr]

    def trace(self,*rest):
        return

# --- Main prog

pf=xml.sax.saxexts.ParserFactory()
p=pf.make_parser("xml.sax.drivers.drv_xmlproc")
p.setDocumentHandler(SAXtracer("doc_handler"))
p.parse("icons.svg")

print
for name in names :
    print
    X=int(float(commands.getoutput("inkscape -I "+name+" -X icons.svg")))
    Y=int(float(commands.getoutput("inkscape -I "+name+" -Y icons.svg")))
    W=int(float(commands.getoutput("inkscape -I "+name+" -W icons.svg")))
    H=int(float(commands.getoutput("inkscape -I "+name+" -H icons.svg")))

    print str(X)+","+str(Y)+" "+str(W)+"x"+str(H)
    x1=int(X/45)*45
    y1=11*90-int(Y/45)*45-45
    x2=x1+45
    y2=y1+45

    command="inkscape -a "+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w "+ \
        str(bitmapw)+" -h "+str(bitmapw)+" -e "+name+".png icons.svg"
    print command
    #print commands.getoutput(command)
    #print commands.getstatus(command)
    print commands.getstatusoutput(command)
   
