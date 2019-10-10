import os.path
import logging
import numpy as np
import math
import time

""" General functions """
property_names = ['Line number', 'ID', 'Hash', 'User NSID',
                  'User nickname', 'Date taken', 'Date uploaded',
                  'Capture device',
                  'Title', 'Description', 'User tags', 'Machine tags',
                  'Longitude', 'Latitude', 'Coord. Accuracy', 'Page URL',
                  'Download URL', 'License name', 'License URL',
                  'Server ID', 'Farm ID', 'Secret', 'Secret original',
                  'Extension', 'Marker']
tag_property_names=['ID', 'autotags']


def display_images(imgs):
    from IPython.display import Image, display
    for im in imgs:
        display(Image(im))


""" Using multiple entries per thread """

def add_autotags_entity_batch(index, database, tag, results):

    def check_status(response):
        redo = False
        for ix, _ in enumerate(tag):
            try:
                results[index + ix] = response[0][ix]['AddEntity']["status"]
            except:
                results[index + ix] = -1
                redo = True
                break
        return redo

    all_queries = []
    for ix, row in enumerate(tag):
        query = {}
        add_entity = {"class": 'autotags', "properties": {"name": row}}
        query["AddEntity"] = add_entity
        all_queries.append(query)

    redo_flag = True
    cnt = 0
    while redo_flag is True and cnt < 5:
        res = database.query(all_queries)
        redo_flag = check_status(res)
        cnt += 1

def add_image_all_batch(index, batch_size, db,
                        start, end, row_data, results):

    rounds = math.ceil((end - start)/ batch_size)

    for i in range(rounds):

        start_r = start + batch_size * i
        if start_r < end:
            end_r   = min(start_r + batch_size , end)
            add_image_entity_batch(start_r, end_r, db,
                                   row_data.iloc[start_r:end_r, :], results)
            # results = add_image_entity_batch(start_r, end_r, db,
                                   # row_data.iloc[start_r:end_r, :], results)
    # return results

def add_image_entity_batch(start, end, database, row_data, results):
    from pandas import isna
    from urllib.parse import urlparse

    def check_status(response):
        redo = False
        for ix, b in enumerate(range(start, end)):
            try:
                results[b] = response[0][ix]['AddEntity']["status"]
            except:
                results[b] = -1
                redo = True
                # print(response)
                break
        return redo

    all_queries = []
    for ix, row in row_data.iterrows():
        # Set Metadata as properties
        props = {}
        for key in property_names:
            if not isna(row[key]):
                if key == 'ID':
                    props[key] = int(row[key])
                elif key == 'Latitude':
                    props[key] = float(row[key])
                elif key == 'Longitude':
                    props[key] = float(row[key])
                else:
                    props[key] = str(row[key])

        props["VD:imgPath"] = 'db/images/jpg' + urlparse(props['Download URL']).path
        props["format"] = "jpg"

        query = {}
        add_entity = {"class": "VD:IMG", "properties": props}
        query["AddEntity"] = add_entity
        all_queries.append(query)

    redo_flag = True
    flag_report = False
    cnt = 0
    while redo_flag is True and cnt < 20:
        res = database.query(all_queries)
        redo_flag = check_status(res)
        if redo_flag is True:
            # print("retrying: ", start, end)
            flag_report = True

        #     print("redoing: ", query)
        cnt += 1

    if flag_report is True:
        print("went through after n retries: ", cnt, start, end)

    if cnt == 20:
        print("--------------!!!!!!!!!!!!!!! batch not completed: ", start, end)
    # return results


def add_autotag_connection_all_batch(index, batch_size, db,
                                     start, end, row_data, results):

    rounds = math.ceil((end - start)/ batch_size)

    # print("rounds", rounds)
    for i in range(rounds):

        start_r = start + batch_size * i
        if start_r < end:
            end_r   = min(start_r + batch_size , end)
            add_autotag_connection_batch(start_r, db,
                                         row_data, start_r, end_r, results)

def add_autotag_connection_batch(index, database, row_data, start, end, results):
    import pandas as pd

    def check_status(response, indices):
        redo = False
        for rix, n in enumerate(indices):
            tmp = response[n[0] : n[1]]
            try:
                status = 0
                for i in range(len(tmp)):
                    cmd = list(tmp[0][i].items())[0][0]
                    if tmp[0][i][cmd]["status"] != 0:
                        results[index + rix] = -1
                        break
                results[index + rix] = 0
            except:
                results[index + rix] = -1
                redo = True
                break
        return redo

    t0 = time.time()

    data = row_data.iloc[start:end, :]
    # print("start_r ", start, "end ", end)

    run_index = []
    all_queries = []
    num_queries = 0

    pre = time.time() - t0

    t0 = time.time()

    for rix, row in data.iterrows():
        # Find Tag
        if not pd.isna(row['autotags']):
            ind = [None, None]
            # Find Image
            parentref = (100 * rix) % 19000
            parentref +=1  #Max ref 20000
            query = {}
            findImage = {}
            findImage["_ref"] = parentref
            findImage['constraints'] = {'ID': ["==", int(row['ID'])]}
            findImage['results'] = {"blob": False}
            query["FindImage"] = findImage
            all_queries.append(query)
            ind[0] = num_queries
            num_queries +=1

            current_tags = row['autotags'].split(',')
            for ix, t in enumerate(current_tags):
                val = t.split(':')
                this_ref = parentref + ix + 1

                # Find Tag
                query = {}
                find_entity = {}
                find_entity["_ref"] = this_ref
                find_entity["class"] = 'autotags'
                find_entity["constraints"] = {'name': ["==", val[0]]}
                query["FindEntity"] = find_entity
                all_queries.append(query)
                num_queries +=1

                # Add Connection
                query = {}
                add_connection = {}
                add_connection["class"] = 'tag'
                add_connection["ref1"] = parentref
                add_connection["ref2"] = this_ref
                add_connection["properties"] = {"tag_name": val[0],
                                                "tag_prob": float(val[1]),
                                                "MetaDataID": int(row['ID'])}
                query["AddConnection"] = add_connection
                all_queries.append(query)
                ind[1] = num_queries
                num_queries +=1
            run_index.append(ind)

    make_query = time.time() - t0

    t0 = time.time()

    redo_flag = True
    cnt = 0
    while redo_flag is True and cnt < 40:
        res = database.query(all_queries)
        redo_flag = check_status(res, run_index)
        cnt += 1

    run_query = time.time() - t0

    total_time = pre + make_query + run_query

    # print("pre:", pre / total_time, "make:", make_query/total_time, "run:", run_query/total_time)

""" Using single entry per thread """

def add_autotags_entity(index, database, tag, results):
    query = {}
    add_entity = {"class": 'autotags', "properties": {"name": tag}}
    query["AddEntity"] = add_entity
    res = database.query([query])
    # print('res[0][0]', res[0][0])
    results[index] = res[0][0]['AddEntity']["status"]


def add_image_entity(index, database, row_data, results):
    from pandas import isna
    from urllib.parse import urlparse

    # Set Metadata as properties
    props = {}
    for key in property_names:
        if not isna(row_data[key]):
            # props[key] = str(row_data[key])
            props[key] = int(row_data[key]) if key == 'ID' else str(row_data[key])
    props["VD:imgPath"] = 'db/images/jpg' + urlparse(props['Download URL']).path
    props["format"] = "jpg"

    query = {}
    add_entity = {"class": "VD:IMG", "properties": props}

    query["AddEntity"] = add_entity
    res = database.query([query])
    results[index] = res[0][0]['AddEntity']["status"]


def add_autotag_connection(index, database, row_data, results):
    import pandas as pd
    all_queries = []
    success_counter = 0

    # Find Image
    parentref = 10
    query = {}
    find_image = {
        "_ref": parentref,
        'constraints': {
            'ID': ["==", int(row_data['ID'])]
        }
    }
    query["FindImage"] = find_image
    all_queries.append(query)

    # Find Tag
    if not pd.isna(row_data['autotags']):
        current_tags = row_data['autotags'].split(',')
        for ix, t in enumerate(current_tags):
            val = t.split(':')
            this_ref = parentref + ix + 1

            # Find Tag
            query = {}
            find_entity = {
                "class": 'autotags',
                "_ref": this_ref,
                "constraints": {
                    'name': ["==", val[0]]
                    }
                }

            query["FindEntity"] = find_entity
            all_queries.append(query)

            # Add Connection
            query = {}
            add_connection = {
                    "class": 'tag',
                    "ref1": parentref,
                    "ref2": this_ref,
                    "properties": {
                        "tag_name": val[0],
                        'tag_prob': float(val[1]),
                        "MetaDataID": int(row_data['ID'])
                        }
                    }

            query["AddConnection"] = add_connection
            all_queries.append(query)
    try:
        res = database.query(all_queries)
        for i in range(len(res)):
            cmd = list(res[i][0].items())[0][0]
            assert res[i][0][cmd]["status"] == 0, 0
            # success_counter +=1
        results[index] = 0
        # results[index] = success_counter

    except:
        results[index] = -1

