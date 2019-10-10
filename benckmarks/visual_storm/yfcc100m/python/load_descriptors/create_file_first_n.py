# This script will create a file of the first N
# feature vectors, acording to the order in the original dataset file

# We need this because the feature vectors are not in the same order.

import descriptors_io as d_io
import numpy as np
import time

# Returns index of x in arr if present, else -1
def binarySearch (arr, l, r, x):

    # Check base case
    if r >= l:

        mid = l + int((r - l)/2)

        # If element is present at the middle itself
        if arr[mid] == x:
            return mid

        # If element is smaller than mid, then it
        # can only be present in left subarray
        elif arr[mid] > x:
            return binarySearch(arr, l, mid-1, x)

        # Else the element can only be present
        # in right subarray
        else:
            return binarySearch(arr, mid + 1, r, x)

    else:
        # Element is not present in the array
        return -1

def read_and_write_to_file(filename):

    data = np.loadtxt(filename, dtype='long')

    print("Sorting...")
    data.sort()

    print("Saving to file...")
    np.save(filename + ".npy", data)

def main():

    # read_and_write_to_file("yfcc100m_ids_1M.txt")
    # return 0

    first_ids = "yfcc100m_ids_1M.txt.npy"
    data = np.load(first_ids)

    # This is a test of a know results for the 10M case to make sure the
    # import went well

    if (len(data) == int(10e6)):
        print("Imported Correctly?",
              bool(1757364 == binarySearch(data, 0, len(data), 2297552664)))
    else:
        print("No check for this data size...")

    prefix = "/mnt/nvme0/YFCC100M_hybridCNN_gmean_fc6_"
    d_reader = d_io.descriptors_reader(prefix)

    output = "/mnt/nvme1/features_10M/YFCC100M_hybridCNN_gmean_fc6_first_1M_"
    d_writer = d_io.descriptors_writer(output)

    total = int(100e6)
    batch_size = 1000

    inserted = 0

    total_found = 0

    start = time.time()

    while inserted < total:

        ids, desc = d_reader.get_next_n(batch_size)

        for idx in range(len(ids)):

            if ( -1 != binarySearch(data, 0, len(data)-1, ids[idx]) ):
                d_writer.write( int(ids[idx]), desc[idx])
                total_found += 1

        inserted += batch_size

        # Print the percentage of elements that are part of the first 10m
        # This should be statiscally around 10%
        if (0 == inserted % (int(1e6))):
            end = time.time()
            print(100 * total_found/inserted, "Took", end - start, "s" )
            start = time.time()

    # Read from file, this should read the files as needed, given a number
    # of elements, it should open the different files as needed.

    # Loop over the total.


if __name__ == "__main__":

    main()
