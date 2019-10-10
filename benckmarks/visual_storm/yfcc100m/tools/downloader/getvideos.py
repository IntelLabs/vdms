import urllib
import time
import os
import time
import re

dataset = "/mnt/nvme1/metadata/original/yfcc100m_dataset"

filename_set = 'yfcc100m_videos_dataset'
filename_reg = '_'

counter = 1
set_counter = 0
reg_counter = 0

f=open(dataset, 'r')

filename = filename_set + str(set_counter) + filename_reg + str(reg_counter)
f_out=open(filename, 'w')
for line in f:
    tokens = line.strip().split('\t')
    if tokens[24] == '1': # video indicator
        # id = tokens[1]  # Image ID
        # f_out.write(line + "\n")
        f_out.write(line)

        counter = counter + 1






