#from cycler import cycler
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import csv
import os
import re

color = ['#000099', '#3333ff', 'g',
         '#ff6600', '#cc0000',
         'purple', 'pink', '#000000']

patterns = [ "/" ,"+" , "x", "o", "O", ".", "*",  "\\" , "|" , "-" ]

def isfloat(value):
  try:
    float(value)
    return True
  except ValueError:
    return False

input_root = "../results/"
filename = "faiss_sizes.log"

line_counter = 0
with open(input_root + filename) as f:
    data = []
    for line in f:

        line = line.replace('\n','')

        if line_counter == 0:
            title = line
            line_counter =+ 1
            continue

        line = line.split(',') # to deal with blank

        if line:            # lines (ie skip them)
            #line = [float(i) for i in line]
            data.append(line)

labels = []
for i in range(len(data)-1):
    labels.append(data[i+1][0])

print(labels)

graphs = []
for i in range(len(data[0])-1):
    graphs.append(data[0][i+1].rstrip())

print(graphs)

val = []

for i in range(len(data)-1):
    new = []
    for j in range(len(data[0])-1):
        new.append(data[i+1][j+1])
    val.append(new)

for i in range(len(val)):
    val[i] = [float(j) for j in val[i]]
    # print val[i]

lines = np.array(val).transpose()
yy = lines

fig, ax0 = plt.subplots(nrows=1)
fig.set_size_inches(12, 3)

bar_width = 0.15
n_groups = yy.shape[0]
index = np.arange(n_groups)
opacity = 0.7

error_config = {'ecolor': '0.3'}

for i in range(yy.shape[1]):

    color_bar = color[i]

    plt.bar(index + i*bar_width, yy[:,i], bar_width,
            alpha=opacity,
            color=color_bar,
            # hatch=patterns[i],
            # yerr=yy[:,1+2*i+1],
            # error_kw=error_config,
            label=labels[i])

ax0.set_title(title)
# ax0.set_yscale('log')

plt.ylabel('Index File Size (GB)', fontsize=12)
ax0.set_xticks(index + bar_width*2.5)
ax0.set_xticklabels(graphs, fontsize=10)
ax0.tick_params(axis='y', labelsize=10)

plt.legend(ncol=1, shadow=True, fancybox=True, fontsize=9.5, loc="best")
# plt.tight_layout()

# pp = PdfPages('test.pdf')

# plt.savefig('histoExePlots/histo_' + filename + '.pdf', format='pdf')
# Write to pdf

newpath = 'plots/'
if not os.path.exists(newpath):
    os.makedirs(newpath)

file_format = 'pdf' # 'png'

out_filename = newpath + filename + '.' + file_format
plt.savefig(out_filename, format=file_format)

os.system("open " + out_filename)

