#!/usr/bin/python

import subprocess

formats=subprocess.check_output(["laidout", "-X"])

for format in formats:
    subprocess.call(["../src/laidout", "test-all-objects.laidout", "-e", "format="+format])
