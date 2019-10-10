#from cycler import cycler
import numpy as np
import matplotlib
matplotlib.use('pdf')
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import csv
import os
import re
import argparse
from pathlib import Path

def isfloat(value):
  try:
    float(value)
    return True
  except ValueError:
    return False

def instr2bool(in_value):
    if in_value.lower() in ['true', 't']:
        return True
    else:
        return False

obj = argparse.ArgumentParser()
obj.add_argument('-infile', type=lambda s: Path(s), default="perf_results/perf_results.log",
                     help='File containing plot data')
obj.add_argument('-outfile', type=lambda s: Path(s), default="perf_results/plots/results_plot_error.pdf",
                     help='PDF path for file containing plots')
obj.add_argument('-log', type=instr2bool, default=True, const=True, nargs='?',
                     help='Use log scale for Tx/sec')

params = obj.parse_args()

plotfilename = str(params.outfile)

newpath = str(params.outfile.parents[0])
if not os.path.exists(newpath):
    os.makedirs(newpath)

color = ['red', 'blue', 'orange', 'red', 'blue', 'orange']
linestyles = ['-', '-', '-', '--', '--', '--',]
markers = ['o', 'o', 'o', '*', '*', '*']
plot = "all"
line_counter = 0

with open(str(params.infile)) as f:
    data = []
    for line in f:
        line = line.replace('\n','')

        if line_counter == 0:
            title = line
            line_counter =+ 1
            continue

        line = line.split(',') # to deal with blank

        if line:            # lines (ie skip them)
            # line = [float(i) for i in line]
            data.append(line)

print(title)

query_name = []
for i in range(len(data)-1):
    query_name.append(data[i+1][0])

xlabels = []
for i in range(len(data[0])-1):
    xlabels.append(data[0][i+1])

val = []

for i in range(len(data)-1):
    new = []
    for j in range(len(data[1])-1):
        new.append(float(data[i+1][j+1]))
    val.append(new)

for i in range(len(val)):
    val[i] = [float(j) for j in val[i]]

val = np.array(val)

print(data[0][1:])

columns_Tx = [i for i in range(0,len(val[0]),4)]
tx_sec = val[:, columns_Tx]
tx_sec = tx_sec.transpose()

print(tx_sec)

columns_Tx_std = [i for i in range(1,len(val[0]),4)]
tx_sec_std = val[:, columns_Tx_std]
tx_sec_std = tx_sec_std.transpose()

print(tx_sec_std)

columns_imgs = [i for i in range(2,len(val[0]),4)]
imgs_sec = val[:, columns_imgs]
imgs_sec = imgs_sec.transpose()

print(imgs_sec)

columns_imgs_std = [i for i in range(3,len(val[0]),4)]
imgs_sec_std = val[:, columns_imgs_std]
imgs_sec_std = imgs_sec_std.transpose()

print(imgs_sec_std)

tick_labels = data[0][1:]
x_pos = [1e5, 5e5, 1e6, 5e6]

# fig = plt.figure(figsize=(8,10))

"""
Plot Tx/sec
"""
# ax0 = plt.subplot(2,1,1)

fig = plt.figure()
plt.rc('lines', linewidth=1)
ax0 = plt.subplot()

for i in range(0,len(tx_sec[0,:])):
    ax0.errorbar(x_pos, tx_sec[:,i],
                 yerr=tx_sec_std[:,i],
                 label = query_name[i],
                 color=color[i], linestyle=linestyles[i], marker=markers[i])

if params.log:
    ax0.set_yscale('log')
    ax0.set_xscale('log')

# xticks = list(range(len(x_pos)))
# ax0.set_xticklabels(x_pos, fontsize=14)
plt.xticks(x_pos, tick_labels)

# ax0.set_title(title)
plt.xlabel('Number of Images', fontsize=12)
plt.ylabel('Tx/sec', fontsize=12)

plt.legend(loc="best", ncol=1, shadow=True, fancybox=True)

plt.savefig(plotfilename + "_metadata.pdf", format="pdf", bbox_inches='tight')

"""
Plot Images/sec
"""
# ax0 = plt.subplot(2,1,2)

fig = plt.figure()
plt.rc('lines', linewidth=1)
ax0 = plt.subplot()

for i in range(0,len(imgs_sec[0,:])):
    ax0.errorbar(x_pos, imgs_sec[:,i],
                 yerr=imgs_sec_std[:,i],
                 label = query_name[i],
                 color=color[i], linestyle=linestyles[i], marker=markers[i])

if params.log:
    ax0.set_yscale('log')
    ax0.set_xscale('log')

# xticks = list(range(len(xlabels)))
# plt.xticks(xticks)
# ax0.set_xticklabels(xlabels, fontsize=14)
plt.xticks(x_pos, tick_labels)

# ax0.set_title(title)
plt.xlabel('Number of Images', fontsize=12)
plt.ylabel('images/sec', fontsize=12)

plt.savefig(plotfilename + "_images.pdf", format="pdf", bbox_inches='tight')

"""
Write to pdf
"""
# plt.savefig(plotfilename, format="pdf")
