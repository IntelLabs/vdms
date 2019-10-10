# This files assumes that the images are already inserted,
# with the ID property set as a int, matching the descriptors ID.

import vdms

import descriptors_io as d_io

descriptor_set_name = "hybridNet"

def insert_descriptor_set(db):

    iDS = { "AddDescriptorSet" : {

            "name": descriptor_set_name,
            "dimensions": 4096,
            "engine": "FaissIVFFlat",
            "metric": "L2"
        }
    }

    allqueries = []
    allqueries.append(iDS)

    db.query(allqueries)

    print(db.get_last_response_str())

# This method is paralelizable, can be called multiple times
def insert_descriptors(batch_size, ids, descriptors, db):

    tx_batch = 10

    rounds = int(len(ids) / tx_batch)

    # print("rounds:", rounds)
    # print("n_ids:", len(ids))
    # print("n_descriptors:", len(descriptors))

    error_counter_fi = 0
    error_counter_ad = 0
    total_errors = 0

    for round_n in range(rounds):

        start = round_n * tx_batch
        end   = min(start + tx_batch, len(ids))

        ref_counter = 1

        all_queries = []
        blobs = []

        for i in range(start,end):

            # print("inderting desc id:", ids[i])

            fI = { "FindImage": {
                    "_ref": ref_counter,
                    "constraints": {
                        "ID": ["==", ids[i]]
                    },
                    "unique": True,
                    "results": {
                        "blob": False # Avoid returning the image blobe
                    }
                }
            }

            aD = { "AddDescriptor": {

                    "set": descriptor_set_name,

                    "link": {
                        "ref": ref_counter
                    }
                }
            }

            all_queries.append(fI)
            all_queries.append(aD)

            blobs.append(descriptors[i])

            ref_counter += 1

        responses, blob = db.query(all_queries, [blobs])

        if (len(responses) < tx_batch * 2):
            # print("Responses sizee error")
            total_errors += tx_batch
            continue

        for i in range(int(len(responses)/2)):
            if (responses[2*i]["FindImage"]["status"] != 0):
                error_counter_fi += 1
            if (responses[2*i+1]["AddDescriptor"]["status"] != 0):
                error_counter_ad += 1

    if (error_counter_fi > 0 or error_counter_ad > 0):
        print("fi_errors:", error_counter_fi)
        print("ad_errors:", error_counter_ad)

    return total_errors

def main():

    db = vdms.vdms()
    db.connect("localhost")

    insert_descriptor_set(db)

    prefix = "/mnt/nvme1/features_1M/YFCC100M_hybridCNN_gmean_fc6_first_1M_"
    d_reader = d_io.descriptors_reader(prefix)

    total = int(1e6)
    batch_size = 1000

    inserted = 0
    total_errors = 0

    while inserted < total:

        ids, desc = d_reader.get_next_n(batch_size)

        total_errors += insert_descriptors(batch_size, ids, desc, db)

        inserted += batch_size

        print("Elements Inserted: ", inserted, " - ",
              100 * inserted / total, "%")

        print("total_errors:", total_errors,
              "Percentage:", 100 * total_errors/inserted, "%")

    # Read from file, this should read the files as needed, given a number
    # of elements, it should open the different files as needed.

    # Loop over the total.


if __name__ == "__main__":

    main()
