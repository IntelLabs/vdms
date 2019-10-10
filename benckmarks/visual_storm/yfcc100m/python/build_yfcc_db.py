import pandas as pd
import numpy as np
import time
import math
import logging
import argparse
from threading import Thread

import multiprocessing

import util
import vdms
import sys

# process_first_n = 100000

log_name = sys.argv[0].split('/')[-1].replace('.py','.log')
connection_batch_limit = 100  #10

def get_args():
    parserobj = argparse.ArgumentParser()
    # parserobj.add_argument('-process_first_n', type=int, default=None,
                           # help='Process only the first N lines of image/connection data [default: None-> all data]')
    parserobj.add_argument('-num_threads', type=int, default=100,
                           help='Number of threads to use [default: 100]')
    parserobj.add_argument('-batch_size', type=int, default=100,
                           help='Number of entries per thread for autotags and images [default: 100; connections: max {}]'.format(connection_batch_limit))
    parserobj.add_argument('-data_file', type=str,
                           default='yfcc100m_dataset_1M',
                           help='YFCC metadata [default: ' +
                                './yfcc100m_dataset_1M')
    parserobj.add_argument('-tag_list', type=str,
                           default='../yfcc_parse_labels/autotag_list.txt',
                           help='List of expected tags [default: ../yfcc_parse_labels/autotag_list.txt]')
    parserobj.add_argument('-add_tags', default=False, action="store_true",
                           help='Whether or not insert tags')
    parserobj.add_argument('-tag_file', type=str,
                           default='yfcc100m_autotags_1M',
                           help='YFCC file of autotags [default: ' +
                                './yfcc100m_autotags_1M]')

    params = parserobj.parse_args()


    print(params.add_tags)
    return params

def get_data(params):
    all_data = pd.read_csv(params.data_file, sep='\t', names=util.property_names)  # , nrows=params.process_first_n)
    tags = pd.read_csv(params.tag_file, sep='\t', names=['ID', 'autotags'])
    all_data = pd.merge(all_data, tags, on='ID')
    return all_data, [line.strip() for line in open(params.tag_list, 'r')]


def get_db_list(num):
    dbs = []
    for i in range(0, num):
        db = vdms.vdms()
        db.connect("localhost")
        dbs.append(db)
    return dbs


def disconnect_db_list(dbs):
    for db in dbs:
        db.disconnect()


def make_logger(name, log_file, level=logging.INFO):
    logger = logging.getLogger(name)
    # format = logging.Formatter(logging.BASIC_FORMAT)
    format = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
    handler = logging.FileHandler(log_file, mode='w')
    handler.setFormatter(format)

    streamhandler = logging.StreamHandler()
    streamhandler.setFormatter(format)

    logger.setLevel(level)
    logger.addHandler(handler)
    logger.addHandler(streamhandler)

    return logger


def get_parameters(params, all_data, num_entries_per_thread=None):
    if num_entries_per_thread is None:
        num_entries_per_thread = params.batch_size

    num_entries = len(all_data)
    # if params.process_first_n is not None and params.process_first_n < num_entries:
        # num_entries = params.process_first_n

    blocks = int(np.ceil(num_entries / (params.num_threads * num_entries_per_thread)))
    return num_entries_per_thread, num_entries, blocks, [None] * num_entries


def process_tag_entities(params, dbs, all_data):
    batch, num_lines, blocks, results = get_parameters(params, all_data)

    for block in range(0, blocks):
        thread_arr = []

        for i in range(0, params.num_threads):
            idx = (params.num_threads * batch) * block + i * batch

            if idx < num_lines:
                thread_add = Thread(target=util.add_autotags_entity_batch, args=(idx, dbs[i], all_data[idx:min([idx+batch, num_lines])], results))
                thread_add.start()
                thread_arr.append(thread_add)
            else:
                break

        idx = (params.num_threads * batch) * block
        error_counter = 0
        for th in thread_arr:
            th.join()
            error_counter += results[idx:min([idx+batch, num_lines])].count(-1)
            idx += batch

    return error_counter


def process_image_entities(params, dbs, all_data):
    batch, num_lines, blocks, results = get_parameters(params, all_data)

    # num_lines = process_first_n

    thread_arr = []

    per_thread = int(num_lines / params.num_threads)

    for i in range(0, params.num_threads):
        idx = i * per_thread

        if idx < num_lines:
            # print('block: {}\tthread: {}\tstart index: {}\tend index:{}'.format(block, i, idx, min([idx+batch, num_lines])))
            start = idx
            end = min([idx+per_thread, num_lines])
            # results = util.add_image_all_batch(idx, batch, dbs[i], start, end, all_data, results)
            thread_add = Thread(target=util.add_image_all_batch, args=(idx, batch, dbs[i], start, end, all_data, results))
            thread_add.start()
            thread_arr.append(thread_add)
        else:
            break

    idx = 0
    error_counter = 0
    for th in thread_arr:
        th.join()
        error_counter += results[idx:min([idx+batch, num_lines])].count(-1)
        idx += batch

    return error_counter

def process_connections(params, dbs, all_data):
    batch, num_lines, blocks, results = get_parameters(params, all_data,
        num_entries_per_thread=min([connection_batch_limit, params.batch_size]))

    # num_lines = process_first_n

    thread_arr = []

    batch = 50
    params.num_threads = 2

    per_thread = int(num_lines / params.num_threads)

    # print("n lines: ", num_lines, "batch: ", batch, "per_thread: ", per_thread)

    for i in range(0, params.num_threads):
        idx = i * per_thread

        if idx < num_lines:

            start = idx
            end = min([idx+per_thread, num_lines])
            thread_add = Thread(target=util.add_autotag_connection_all_batch,
                                args=(idx, batch, dbs[i], start, end,
                                      all_data, results) )
            thread_add.start()
            thread_arr.append(thread_add)
        else:
            break

    for th in thread_arr:
        th.join()

    error_counter = results.count(-1)
    return error_counter

def main(in_args):
    main_logger.info('[!] Building database')
    start = time.time()

    # Get data
    data, all_tags = get_data(in_args)
    e_time = time.time() - start
    main_logger.info('\t[!] [get_data] Total elapsed time: {:0.4f} secs ({:0.4f} mins)'.format(e_time, e_time / 60.))

    db_list = get_db_list(in_args.num_threads)

    func_list = []
    func_data = []
    if (in_args.add_tags == True):
        func_list.append('process_tag_entities')
        func_data.append(all_tags)

    func_list.append('process_image_entities')
    func_list.append('process_connections')

    func_data.append(data)
    func_data.append(data)
    # func_list = ['process_tag_entities', 'process_image_entities']
    # func_data = [all_tags, data]
    # func_list = ['process_connections']
    # func_data = [ data]
    for fn, fd in zip(func_list, func_data):
        start_t = time.time()
        tag_num_errors = eval('{}(in_args, db_list, fd)'.format(fn))
        e_time = time.time() - start_t
        main_logger.info('\t[!] [{}] Total Errors: {}\tTotal elapsed time: {:0.4f} secs ({:0.4f} mins)'.format(fn, tag_num_errors, e_time, e_time / 60.))

    disconnect_db_list(db_list)
    e_time = time.time() - start
    main_logger.info('[!] Total elapsed time: {:0.4f} secs ({:0.4f} mins)'.format(e_time, e_time / 60.))


if __name__ == "__main__":
    main_logger = make_logger('main_logger', log_name)

    args = get_args()
    main(args)
