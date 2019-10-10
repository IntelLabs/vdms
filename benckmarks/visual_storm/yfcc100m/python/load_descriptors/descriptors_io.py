import os
import struct
import numpy

class descriptors_reader(object):

    def __init__(self, prefix):

        self.file = None
        self.prefix = prefix
        self.file_counter = 0

        self.dimensions = 4096

        self.desc_counter = 0

        self.read_next_file()

    def get_next_n(self, n):

        ids = []
        descriptors = []

        i = 0

        while i < n:

            id_bin = self.file.read(8)
            if len(id_bin) != 8 :
                self.read_next_file()
                continue

            id_long = struct.unpack('@l', id_bin)[0]

            descriptor = self.file.read(4 * self.dimensions)

            ids.append(id_long)
            descriptors.append(descriptor)

            i += 1

        return ids, descriptors

    def read_next_file(self):

        filename = self.prefix + str(self.file_counter) + ".bin"

        print("Reading", filename, "...")

        self.file = open(filename, 'rb')
        self.file_counter += 1

class descriptors_writer(object):

    def __init__(self, prefix):

        self.file = None
        self.prefix = prefix
        self.file_counter = 0

        self.desc_counter = 0
        self.per_file = int(1e6)

        self.open_next_file()

    # This write descriptors 1 by 1
    def write(self, d_id, desc):

        i = 0

        id_pack = struct.pack('@l', d_id)
        self.file.write(id_pack)
        self.file.write(desc)

        self.desc_counter += 1

        if (self.desc_counter == self.per_file):
            self.open_next_file()

    def open_next_file(self):

        filename = self.prefix + str(self.file_counter) + ".bin"

        print("Opening", filename, "...")

        self.file = open(filename, 'wb')
        self.file_counter += 1
        self.desc_counter = 0
