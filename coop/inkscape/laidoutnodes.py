#!/usr/bin/env python3

'''
laidoutnodes.py

Script to call Laidout to edit svg filters with nodes.
Tom Lechner, 2018

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


'''

__version__ = "0.1"


import sys
from subprocess import Popen, PIPE


class LaidoutNodes ():
     ####
     ####  If you have a different location of your laidout program change it here:
     ####
    #laidoutexecutable = "/usr/bin/laidout"
    #laidoutexecutable = "/usr/local/bin/laidout"
    #laidoutexecutable = "laidout-git"
    laidoutexecutable = "/home/tom/other/p/github/laidout/src/laidout"


    def __init__(self):
        #inkex.Effect.__init__(self)
        pass

    def validate_options(self):
        pass 

    def errormsg(msg):
        if isinstance(msg, unicode):
            sys.stderr.write(msg.encode("utf-8") + "\n")
        else:
            sys.stderr.write((unicode(msg, "utf-8", errors='replace') + "\n").encode("utf-8"))

    def DoLaidoutStuff(self):
        stream = sys.stdin

		 #Inkscape calls with a filename in argv
        args=sys.argv[1:]
        filename = None
        if len(args) > 0:
            filename = args[-1]
        if filename is not None:
            try:
                stream = open(filename, 'r')
            except IOError:
                errormsg(_("Unable to open object member file: %s") % self.svg_file)
                sys.exit()

        #data = stream.readlines()
        ##data = "".join(data)
        ###data = sys.stdin.read()
        #text_file = open("/home/tom/other/p/github/laidout/src/FROMINKSCAPE.txt", "w")
        #text_file.write("\n".join(sys.argv))
        #text_file.write(str(data)+str(len(data)))
        #text_file.write("EOF!")
        #text_file.close()

        #sys.stderr.write("stderr Done!")
        #sys.stdout.write('<svg><rect style="fill:#000000;fill-opacity:1;" x="0" y="0" width="100" height="100"></svg>')
        #sys.exit(1)

        #if os.path.exists(self.laidoutexecutable) == False:
        #    self.laidoutexecutable = "/usr/local/bin/laidout"


        thecommand = self.laidoutexecutable+"  -E --nodes-only 'format=svg, pipein, pipeout'" 


        try:
            #sys.stderr.write("Trying: "+thecommand)
            #os.system(thecommand)
            process = Popen(thecommand, shell=True, stdout = sys.stdout, stdin = stream)

            process.wait()
            #stdout_value, stderr_value = proc.communicate()

            return 0

        except:
            sys.stderr.write("An error occured trying to run the system command: "+thecommand)
            sys.exit(1)



    def effect(self):
        self.validate_options()
        parent = self.document.getroot()


        self.DoLaidoutStuff()



if __name__ == '__main__':   #pragma: no cover
    e = LaidoutNodes()

    sys.exit(e.DoLaidoutStuff())
