import urllib
import time
import os
import time
import re

dataset = "/mnt/nvme1/metadata/original/yfcc100m_dataset"

filename_set = 'yfcc100m_ids_10M'
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
        id = tokens[1]  # Image ID
        f_out.write(id + "\n")

        if counter % 10000000 == 0:
		break

        counter = counter + 1






