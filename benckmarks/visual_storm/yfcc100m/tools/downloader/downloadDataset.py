import urllib
import time
import os
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import time
import pylab
import re

dataset = "/home/luisremi/data/yfcc100m/yfcc100m_dataset"

f=open(dataset, 'r')
for line in f:
    tokens = line.strip().split('\t')
    if tokens[24] == '0': # photo indicator
        url = tokens[16]  # url of the photo
        #print url

        img_name = re.sub(r'.*staticflickr.com/(.*)', r'\1', url)
        img_name = img_name.replace("/", "-")
        print img_name

        flag = False
        counter_failed = 0

        while (flag == False and counter_failed < 10):
            try:
                name = "images/" + img_name
                #urllib.urlretrieve(tokens[16], name)
                os.system("wget " + url)
                #img=mpimg.imread(name)
                #imgplot = plt.imshow(img)
                #time.sleep(2)
                flag = True
            except:
                print "ERRRO FOR " + url
                counter_failed = counter_failed + 1
                print counter_failed

    #time.sleep(1)

