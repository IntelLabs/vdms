import urllib
import time
import os
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import time 
import pylab 

filename = "../urls.txt"

_cached_stamp = 0

while True:
    stamp = os.stat(filename).st_mtime
    if stamp != _cached_stamp:
        _cached_stamp = stamp
        # File has changed, so do something...

        with open(filename) as f:
            lines = f.readlines()


        counter = 0;
        os.system("rm images/*")
        for line in lines:
            print line
            # if "video" not in line
            try: 
        		name = "images/" + str(counter) + ".jpg"
           		urllib.urlretrieve(line, name)
        		#img=mpimg.imread(name)
        		#imgplot = plt.imshow(img)
        		#time.sleep(2)
            except:
            	print "ERRRO FOR " + line

            stamp = os.stat(filename).st_mtime
            if stamp != _cached_stamp:
                break

            counter = counter + 1

        time.sleep(2)