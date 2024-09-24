#
# The MIT License
#
# @copyright Copyright (c) 2017 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import TestCommand
import numpy as np


class TestDescriptors(TestCommand.TestCommand):
    def add_descriptor(
        self,
        command_str: str,
        setname: str,
        label: str = None,
        ref: int = None,
        props: dict = None,
        link: dict = None,
        k_neighbors: int = None,
        constraints: dict = None,
        results: dict = None,
    ):

        descriptor: dict = {"set": setname}

        if "Add" in command_str and label is not None:
            descriptor["label"] = label

        if ref is not None:
            descriptor["_ref"] = ref

        if props is not None:
            descriptor["properties"] = props

        if "Add" in command_str and link is not None:
            descriptor["link"] = link

        if "Find" in command_str and k_neighbors is not None:
            descriptor["k_neighbors"] = int(k_neighbors)

        if "Find" in command_str and constraints is not None:
            descriptor["constraints"] = constraints

        if "Find" in command_str and results is not None:
            descriptor["results"] = results

        query = {command_str: descriptor}
        return query

    def addSet(self, name, dim, metric="L2", engine="FaissFlat"):
        db = self.create_connection()
        all_queries = self.create_descriptor_set(name, dim, metric, engine)
        response, img_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)
        self.disconnect(db)

    def create_descriptors(self, set_name, dims, total, labels=True):
        all_queries = []
        descriptor_blob = []

        class_counter = -1
        for i in range(0, total):
            if (i % 4) == 0:
                class_counter += 1

            x = np.ones(dims)
            x[2] = 2.34 + i * 20
            x = x.astype("float32")
            descriptor_blob.append(x.tobytes())

            props = {}
            props["myid"] = i + 200

            if labels:
                label = "class" + str(class_counter)
            else:
                label = None

            query = self.add_descriptor("AddDescriptor", set_name, label, props=props)

            all_queries.append(query)

        return all_queries, descriptor_blob

    def addSet_and_Insert(
        self, set_name, dims, total, metric="L2", engine="FaissFlat", labels=True
    ):
        db = self.create_connection()
        all_queries = self.create_descriptor_set(set_name, dims, metric, engine)
        response, img_array = db.query(all_queries)

        # Check AddDescriptorSet success
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        all_queries, descriptor_blob = self.create_descriptors(
            set_name, dims, total, labels
        )
        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0, total):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

        self.disconnect(db)

    def test_findDescriptorSet(self):
        db = self.create_connection()
        name = "testFindDescriptorSet"
        dim = 128
        engine = "FaissFlat"
        metric = "L2"

        self.addSet(name, dim, metric, engine)

        all_queries = []
        storeIndex = True
        descriptor_set = {}
        descriptor_set["set"] = name
        descriptor_set["storeIndex"] = storeIndex
        query = {}
        query["FindDescriptorSet"] = descriptor_set
        all_queries.append(query)

        # Execute the query
        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindDescriptorSet"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptorSet"]["returned"], 1)
        self.assertEqual(
            response[0]["FindDescriptorSet"]["entities"][0]["VD:engine"], engine
        )
        self.assertEqual(
            response[0]["FindDescriptorSet"]["entities"][0]["VD:dimensions"], dim
        )
        self.assertEqual(
            response[0]["FindDescriptorSet"]["entities"][0]["VD:name"], name
        )

    @TestCommand.TestCommand.shouldSkipRemotePythonTest()
    def test_addSetAndDescriptors(self):
        engines = ["FaissFlat", "FaissIVFFlat", "Flinng", "TileDBDense", "TileDBSparse"]
        metrics = ["L2"]
        dimensions = [128]
        total = 2
        for eng in engines:
            for metric in metrics:
                for dim in dimensions:
                    self.addSet_and_Insert(
                        f"features_{dim}d-{metric}-{eng}",
                        dim,
                        total,
                        metric,
                        eng,
                        labels=True,
                    )

    def test_addSetAndDescriptorsDimMismatch(self):
        db = self.create_connection()

        # Add Set
        set_name = "features_64d_dim_mismatched"
        dims = 64
        all_queries = self.create_descriptor_set(set_name, dims)

        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        # Add Descriptors
        descriptor_blob = []
        x = np.zeros(dims // 2)
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        all_queries = []
        query = self.add_descriptor("AddDescriptor", set_name)
        all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(response[0]["info"], "Blob Dimensions Mismatch")

        self.disconnect(db)

    def test_AddSetAndWrongBatchSize(self):

        db = self.create_connection()

        # Create and verify descriptor set
        trans_list = []
        trans_dict = {}
        desc_set = {}
        desc_set["engine"] = "FaissFlat"
        desc_set["metric"] = "L2"
        desc_set["name"] = "wrongbatchsize"
        desc_set["dimensions"] = 128
        trans_dict["AddDescriptorSet"] = desc_set

        trans_list.append(trans_dict)

        response, img_array = db.query(trans_list)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        # Create and add a batch of feature vectors
        trans = []
        blobs = []
        nr_dims = 128
        batch_size = 10
        desc_blob = []
        x = np.zeros(nr_dims * batch_size)
        x = x.astype("float32")
        desc_blob.append(x.tobytes())

        properties_list = []
        for x in range(batch_size + 3):
            props = {"batchprop": x}
            properties_list.append(props)

        descriptor = {}
        descriptor["set"] = "wrongbatchsize"
        descriptor["batch_properties"] = properties_list
        query = {}
        query["AddDescriptor"] = descriptor
        trans.append(query)
        blobs.append(desc_blob)

        response, img_array = db.query(trans, blobs)
        self.assertEqual(response[0]["info"], "FV Input Length Mismatch")
        self.assertEqual(response[0]["status"], -1)

        self.disconnect(db)

    def test_AddSetAndInsertBatch(self):

        db = self.create_connection()

        # Create and verify descriptor set
        trans_list = []
        trans_dict = {}
        desc_set = {}
        desc_set["engine"] = "FaissFlat"
        desc_set["metric"] = "L2"
        desc_set["name"] = "rightbatchsize"
        desc_set["dimensions"] = 128
        trans_dict["AddDescriptorSet"] = desc_set

        trans_list.append(trans_dict)

        response, img_array = db.query(trans_list)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        # Create and add a batch of feature vectors
        trans = []
        blobs = []
        nr_dims = 128
        batch_size = 10
        desc_blob = []
        x = np.zeros(nr_dims * batch_size)
        x = x.astype("float32")
        desc_blob.append(x.tobytes())

        properties_list = []
        for x in range(batch_size):
            props = {"batchprop": x}
            properties_list.append(props)

        descriptor = {}
        descriptor["set"] = "rightbatchsize"
        descriptor["batch_properties"] = properties_list
        query = {}
        query["AddDescriptor"] = descriptor
        trans.append(query)
        blobs.append(desc_blob)

        response, img_array = db.query(trans, blobs)
        self.assertEqual(response[0]["AddDescriptor"]["status"], 0)

        # now try to get those same descriptors back
        desc_find = {}
        desc_find["set"] = "rightbatchsize"
        desc_find["results"] = {"list": ["batchprop"]}

        query = {}
        query["FindDescriptor"] = desc_find

        trans = []
        blobs = []
        trans.append(query)
        response, img_array = db.query(trans, blobs)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], 10)

        self.disconnect(db)

    def test_AddBatchAndFindKNN(self):

        db = self.create_connection()

        # Create and verify descriptor set
        trans_list = []
        trans_dict = {}
        desc_set = {}
        desc_set["engine"] = "FaissFlat"
        desc_set["metric"] = "L2"
        desc_set["name"] = "knn_batch_set"
        desc_set["dimensions"] = 128
        trans_dict["AddDescriptorSet"] = desc_set

        trans_list.append(trans_dict)

        response, img_array = db.query(trans_list)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        # Descriptor Set Created, now lets create a batch to insert
        # first lets make a big combined blob representing the inserted descriptor
        trans = []
        blobs = []
        nr_dims = 128
        batch_size = 5
        desc_blob = []
        x = np.ones(nr_dims * batch_size)
        for i in range(batch_size):
            x[2 + (i * nr_dims)] = 2.34 + i * 20

        x = x.astype("float32")
        desc_blob.append(x.tobytes())

        properties_list = []
        for x in range(batch_size):
            props = {"myid": x + 200}
            properties_list.append(props)

        descriptor = {}
        descriptor["set"] = "knn_batch_set"
        descriptor["batch_properties"] = properties_list

        query = {}
        query["AddDescriptor"] = descriptor
        trans.append(query)
        blobs.append(desc_blob)

        response, img_array = db.query(trans, blobs)
        self.assertEqual(response[0]["AddDescriptor"]["status"], 0)

        ### Now try to find a KNN
        kn = 3
        finddescriptor = {}
        finddescriptor["set"] = "knn_batch_set"

        results = {}
        results["list"] = ["myid", "_id", "_distance"]
        results["blob"] = True
        finddescriptor["results"] = results
        finddescriptor["k_neighbors"] = kn

        query = {}
        query["FindDescriptor"] = finddescriptor

        all_queries = []
        all_queries.append(query)

        descriptor_blob = []
        x = np.ones(128)
        x[2] = 2.34 + 1 * 20  # 2.34 + 1*20
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        response, blob_array = db.query(all_queries, [descriptor_blob])

        self.assertEqual(len(blob_array), kn)
        self.assertEqual(descriptor_blob[0], blob_array[0])

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], kn)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["_distance"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][1]["_distance"], 400)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][2]["_distance"], 400)
        self.disconnect(db)

    def test_classifyDescriptor(self):
        db = self.create_connection()
        set_name = "features_128d_4_classify"
        dims = 128
        all_queries = self.create_descriptor_set(set_name, dims)
        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        total = 30
        all_queries, descriptor_blob = self.create_descriptors(
            set_name, dims, total, labels=True
        )
        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0, total - 1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

        descriptor = {}
        descriptor["set"] = set_name
        query = {}
        query["ClassifyDescriptor"] = descriptor

        for i in range(2, total // 10, 4):
            all_queries = []
            descriptor_blob = []

            x = np.ones(dims)
            x[2] = 2.34 + i * 20  # Calculated to be of class0
            x = x.astype("float32")
            descriptor_blob.append(x.tobytes())

            all_queries.append(query)

            response, img_array = db.query(all_queries, [descriptor_blob])

            # Check success
            self.assertEqual(response[0]["ClassifyDescriptor"]["status"], 0)
            self.assertEqual(
                response[0]["ClassifyDescriptor"]["label"], "class" + str(int(i / 4))
            )
        self.disconnect(db)

    @TestCommand.TestCommand.shouldSkipRemotePythonTest()
    def test_addDifferentSets(self):
        engines = ["FaissFlat", "FaissIVFFlat", "Flinng", "TileDBDense", "TileDBSparse"]
        metrics = ["L2", "IP"]
        dimensions = [128, 4075]
        for eng in engines:
            for metric in metrics:
                for dim in dimensions:
                    self.addSet(f"{dim}-{metric}-{eng}", dim, metric, eng)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByConstraints(self):
        # Add Set
        set_name = "features_128d_4_findbyConst"
        dims = 128
        total = 5
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        all_queries = []
        constraints = {}
        constraints["myid"] = ["==", 202]
        results = {}
        results["list"] = [
            "myid",
        ]
        query = self.add_descriptor(
            "FindDescriptor", set_name, constraints=constraints, results=results
        )
        all_queries.append(query)
        response, img_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], 1)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["myid"], 202)
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescUnusedRef(self):
        # Add Set
        set_name = "features_128d_4_findunusedRef"
        dims = 128
        total = 5
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        all_queries = []
        constraints = {}
        constraints["myid"] = ["==", 202]
        results = {}
        results["list"] = ["myid"]
        query = self.add_descriptor(
            "FindDescriptor", set_name, ref=1, constraints=constraints, results=results
        )
        all_queries.append(query)

        response, blob_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], 1)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["myid"], 202)
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByConst_multiple_blobTrue(self):
        # Add Set
        set_name = "features_128d_4_findDescriptors_m_blob"
        dims = 128
        total = 5
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        all_queries = []
        constraints = {}
        constraints["myid"] = ["<=", 202]
        results = {}
        results["list"] = ["myid"]
        results["sort"] = "myid"
        results["blob"] = True
        query = self.add_descriptor(
            "FindDescriptor", set_name, constraints=constraints, results=results
        )
        all_queries.append(query)

        response, fv_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], 3)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["myid"], 200)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][1]["myid"], 201)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][2]["myid"], 202)
        self.assertEqual(len(fv_array), 3)
        self.assertEqual(len(fv_array[0]), dims * 4)
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByBlob(self):
        # Add Set
        set_name = "findwith_blob"
        dims = 128
        total = 5
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        kn = 3
        all_queries = []
        results = {}
        results["list"] = ["myid", "_id", "_distance"]
        results["blob"] = True
        query = self.add_descriptor(
            "FindDescriptor", set_name, k_neighbors=kn, results=results
        )
        all_queries.append(query)

        descriptor_blob = []
        x = np.ones(dims)
        x[2] = x[2] = 2.34 + 1 * 20  # 2.34 + 1*20
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        response, blob_array = db.query(all_queries, [descriptor_blob])

        self.assertEqual(len(blob_array), kn)
        self.assertEqual(descriptor_blob[0], blob_array[0])

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], kn)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["_distance"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][1]["_distance"], 400)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][2]["_distance"], 400)
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByBlobNoResults(self):
        # Add Set
        set_name = "findwith_blobNoResults"
        dims = 128
        total = 1
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        kn = 1

        all_queries = []
        results = {}
        results["blob"] = True
        query = self.add_descriptor(
            "FindDescriptor", set_name, k_neighbors=kn, results=results
        )
        all_queries.append(query)

        descriptor_blob = []
        x = np.ones(dims)
        x[2] = 2.34
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        response, blob_array = db.query(all_queries, [descriptor_blob])

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], 1)
        self.assertEqual(len(blob_array), kn)
        self.assertEqual(descriptor_blob[0], blob_array[0])
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByBlobUnusedRef(self):
        # Add Set
        set_name = "findwith_blobUnusedRef"
        dims = 50
        total = 3
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        kn = 3

        all_queries = []
        results = {}
        results["blob"] = True
        query = self.add_descriptor(
            "FindDescriptor", set_name, ref=1, k_neighbors=kn, results=results
        )
        all_queries.append(query)

        descriptor_blob = []
        x = np.ones(dims)
        x[2] = 2.34 + 1 * 20
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        response, blob_array = db.query(all_queries, [descriptor_blob])

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], kn)
        self.assertEqual(len(blob_array), kn)
        self.assertEqual(descriptor_blob[0], blob_array[0])
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByBlobAndConstraints(self):
        # Add Set
        set_name = "findwith_blob_const"
        dims = 128
        total = 5
        self.addSet_and_Insert(set_name, dims, total)

        db = self.create_connection()

        kn = 3

        all_queries = []
        results = {}
        results["list"] = ["myid", "_id", "_distance"]
        results["blob"] = True
        constraints = {}
        constraints["myid"] = ["==", 202]
        query = self.add_descriptor(
            "FindDescriptor",
            set_name,
            k_neighbors=kn,
            constraints=constraints,
            results=results,
        )
        all_queries.append(query)

        descriptor_blob = []
        x = np.ones(dims)
        x[2] = 2.34 + 2 * 20
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        response, blob_array = db.query(all_queries, [descriptor_blob])

        self.assertEqual(len(blob_array), 1)
        self.assertEqual(descriptor_blob[0], blob_array[0])

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], 1)

        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["_distance"], 0)
        self.disconnect(db)

    # @unittest.skip("Skipping class until fixed")
    def test_findDescByBlobWithLink(self):
        # Add Set
        set_name = "findwith_blob_link"
        dims = 128
        total = 3

        db = self.create_connection()
        all_queries = self.create_descriptor_set(set_name, dims)

        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        all_queries = []
        descriptor_blob = []

        class_counter = -1
        for i in range(0, total):
            if (i % 4) == 0:
                class_counter += 1

            reference = i + 2

            x = np.ones(dims)
            x[2] = 2.34 + i * 20
            x = x.astype("float32")
            descriptor_blob.append(x.tobytes())

            props = {}
            props["myid"] = i + 200
            query = self.add_descriptor(
                "AddDescriptor",
                set_name,
                label="class" + str(class_counter),
                ref=reference,
                props=props,
            )

            all_queries.append(query)

            props = {}
            props["entity_prop"] = i + 200
            link = {}
            link["ref"] = reference
            query = self.create_entity(
                "AddEntity", class_str="RandomEntity", props=props, link=link
            )
            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0, total - 1, 2):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)
            self.assertEqual(response[x + 1]["AddEntity"]["status"], 0)

        kn = 3
        reference = 102  # because I can

        all_queries = []
        results = {}
        results["list"] = ["myid", "_id", "_distance"]
        results["blob"] = True
        query = self.add_descriptor(
            "FindDescriptor", set_name, ref=reference, k_neighbors=kn, results=results
        )
        all_queries.append(query)

        descriptor_blob = []
        x = np.ones(dims)
        x[2] = 2.34 + 1 * 20
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        results = {}
        results["list"] = ["entity_prop"]
        results["sort"] = "entity_prop"
        link = {}
        link["ref"] = reference
        query = self.create_entity(
            "FindEntity", class_str="RandomEntity", results=results, link=link
        )

        all_queries.append(query)

        response, blob_array = db.query(all_queries, [descriptor_blob])

        self.assertEqual(len(blob_array), kn)
        # This checks that the received blobs is the same as the inserted.
        self.assertEqual(descriptor_blob[0], blob_array[0])

        # Check success
        self.assertEqual(response[0]["FindDescriptor"]["status"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["returned"], kn)

        self.assertEqual(response[0]["FindDescriptor"]["entities"][0]["_distance"], 0)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][1]["_distance"], 400)
        self.assertEqual(response[0]["FindDescriptor"]["entities"][2]["_distance"], 400)

        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["returned"], kn)

        self.assertEqual(response[1]["FindEntity"]["entities"][0]["entity_prop"], 200)
        self.assertEqual(response[1]["FindEntity"]["entities"][1]["entity_prop"], 201)
        self.assertEqual(response[1]["FindEntity"]["entities"][2]["entity_prop"], 202)
        self.disconnect(db)
