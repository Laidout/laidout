#!/usr/bin/python

import subprocess

formats = subprocess.check_output(["laidout", "-X"]).splitlines();

for format in formats:
	#print("do something with: "+format);
    subprocess.call(["../src/laidout", "test-all-objects.laidout", "-e", "filter='"+format+"'"])
