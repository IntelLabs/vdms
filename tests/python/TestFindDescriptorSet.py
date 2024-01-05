import TestCommand


class TestFindDescriptorSet(TestCommand.TestCommand):
    def addSet(self, name, dim, metric, engine):
        db = self.create_connection()

        all_queries = []
        descriptor_set = {}
        descriptor_set["name"] = name
        descriptor_set["dimensions"] = dim
        descriptor_set["metric"] = metric
        descriptor_set["engine"] = engine

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        # Execute the query
        response, img_array = db.query(all_queries)

        # Check if the query was successful (you can add your own checks here)
        if "AddDescriptorSet" in response[0]:
            status = response[0]["AddDescriptorSet"].get("status")
            self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

    def test_findDescriptorSet(self):
        db = self.create_connection()
        name = "testFindDescriptorSet-new"
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
