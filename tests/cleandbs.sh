#!/bin/bash -e

rm -rf /tmp/tests_output_dir || true
rm -rf test_db_client || true
rm -rf test_db_1 || true
rm -rf db || true
rm -rf db_backup || true
rm -rf /tmp/rpathimage.jpg || true
rm -rf /tmp/kubeconfig || true
rm -rf remote_function_test/entity_pb2_grpc.py || true
rm -rf remote_function_test/entity_pb2.py || true