import urllib
import time
import os
import time
import re

dataset = "/data/yfcc100m/original/yfcc100m_dataset"

filename_set = 'yfcc100m_'
filename_reg = '_'

counter = 1
set_counter = 0
reg_counter = 0

f=open(dataset, 'r')



filename = filename_set + str(set_counter) + filename_reg + str(reg_counter)
f_out=open(filename, 'w')
for line in f:
    tokens = line.strip().split('\t')
    if tokens[24] == '0': # photo indicator
        url = tokens[16]  # url of the photo
        f_out.write(url + "\n")

        if counter % 8400000 == 0:
            reg_counter = reg_counter + 1
            if reg_counter > 3:
                reg_counter = 0
                set_counter = set_counter + 1
            filename = filename_set + str(set_counter) + filename_reg + str(reg_counter)
            f_out=open(filename, 'w')

	counter = counter + 1
