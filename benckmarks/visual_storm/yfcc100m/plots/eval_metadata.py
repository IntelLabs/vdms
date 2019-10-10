#from cycler import cycler
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import csv
import os
import re

def isfloat(value):
  try:
    float(value)
    return True
  except ValueError:
    return False

color = ['red', 'blue', 'orange', 'g', 'brown', 'black']
linestyles = ['-', '--', '-.', ':', '-', '--',]

plot = "all" # "ops"

input_root = "../results/"
filename = "results.log"
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
            # line = [float(i) for i in line]
            data.append(line)

print(title)

indexes_name = []
for i in range(len(data)-1):
    indexes_name.append(data[i+1][0])

# print(indexes_name)

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

columns_used = [0,2,4,6]
yy = val[:, columns_used]
yy = yy.transpose()

columns_acc = [1,3,5,7]
accuracy = val[:, columns_acc]
accuracy = accuracy.transpose()

# print(yy)

fig = plt.figure(figsize=(8,10))
plt.rc('lines', linewidth=1)

# Plot Execution Time

ax0 = plt.subplot(2,1,1)

for i in range(0,len(yy[0,:])):
    ax0.plot(yy[:,i], label = indexes_name[i],
                color=color[i], linestyle=linestyles[i], marker='o')

xticks = list(range(len(xlabels)))
plt.xticks(xticks)
ax0.set_xticklabels(xlabels, fontsize=14)

ax0.set_yscale('log')
ax0.set_title(title)
plt.xlabel('# of feature vectors', fontsize=12)
plt.ylabel('Time (ms)', fontsize=12)

plt.legend(loc="best", ncol=1, shadow=True, fancybox=True)

# Plot Accuracy

ax0 = plt.subplot(2,1,2)

for i in range(0,len(yy[0,:])):
    ax0.plot(accuracy[:,i], label = indexes_name[i],
             color=color[i], linestyle=linestyles[i], marker='o')

xticks = list(range(len(xlabels)))
plt.xticks(xticks)
ax0.set_xticklabels(xlabels, fontsize=14)

plt.xlabel('# of feature vectors', fontsize=12)
plt.ylabel('Accuracy (%)', fontsize=12)
plt.ylim([0,105])

# Write to pdf

newpath = 'plots/'
if not os.path.exists(newpath):
    os.makedirs(newpath)

file_format = 'pdf' # 'png'

out_filename = newpath + filename + '.' + file_format
plt.savefig(out_filename, format=file_format)

os.system("open " + out_filename)
