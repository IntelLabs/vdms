import sys
import grpc
import entity_pb2
import entity_pb2_grpc
import json
import os
import importlib.util
import asyncio
import signal
from concurrent.futures import ProcessPoolExecutor
import multiprocessing

tmp_dir_path = None
UDF_MAP = {}

cpu_cores = multiprocessing.cpu_count()
max_workers = max(cpu_cores - 1, 1)
executor = ProcessPoolExecutor(max_workers=max_workers)


# Function to dynamically import a module given its full path
def import_module_from_path(module_name, path):
    try:
        # Create a module spec from the given path
        spec = importlib.util.spec_from_file_location(module_name, path)

        # Load the module from the created spec
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    except Exception as e:
        print("import_module_from_path() failed:", str(e))
        return None


def setup(tmp_path):
    global tmp_dir_path

    # Get the real directory where this Python file is
    currentDir = os.path.realpath(os.path.dirname(__file__))

    if tmp_path is None:
        tmp_path = currentDir
        print("Warning: Using temporary dir:", tmp_path, " as default.")

    if not os.path.exists(tmp_path):
        raise Exception(f"{tmp_path}: path to temporary dir is invalid")

    functions_path = os.path.join(currentDir, "functions")
    if not os.path.exists(functions_path):
        raise Exception(f"{functions_path}: path to functions dir is invalid")

    # Set path to temporary dir
    tmp_dir_path = tmp_path

    for entry in os.scandir(functions_path):
        if entry.is_file() and entry.path.endswith(".py"):
            module_name = entry.name[:-3]

            # Import the module from the given path
            module = import_module_from_path(module_name, entry)
            if module is None:
                raise Exception(
                    "setup() error: module '" + entry + "' could not be loaded"
                )
            UDF_MAP[module_name] = module


def run_udf(module_name, result, options):
    udf = UDF_MAP[module_name]
    return udf.run(result, options)


# gRPC Servicer
class OperatorServicer(entity_pb2_grpc.OperatorServicer):
    async def Operate(self, request, context):
        result = request.entity
        options = json.loads(request.options.decode("utf-8"))
        loop = asyncio.get_running_loop()
        ebytes, rdict = await loop.run_in_executor(
            executor, run_udf, options["id"], result, options
        )
        return entity_pb2.Entity(
            entity=ebytes, options=json.dumps(rdict).encode("utf-8")
        )


# Graceful shutdown handler
async def shutdown(server, executor):
    print("\nShutting down...")
    await server.stop(5)  # Allow 5 seconds to finish active RPCs
    server.wait_for_termination()
    executor.shutdown(wait=True)
    print("Shutdown complete.")


async def main(port):
    server = grpc.aio.server()
    entity_pb2_grpc.add_OperatorServicer_to_server(OperatorServicer(), server)
    server.add_insecure_port("[::]:{}".format(port))
    await server.start()
    print("Async gRPC server (multiprocessing) started on port 50051")

    stop_event = asyncio.Event()

    # Handle SIGINT and SIGTERM
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, stop_event.set)

    await stop_event.wait()
    await shutdown(server, executor)


if __name__ == "__main__":
    if sys.argv[1] is None:
        print("Port missing\n Correct Usage: python3 udf_server.py <port> [tmp_path]")
    elif len(sys.argv) < 3:
        print(
            "Warning: Path to the temporary directory is missing\nBy default the path will be the current directory"
        )
        setup(None)
        try:
            asyncio.run(main(int(sys.argv[1])))
        except KeyboardInterrupt:
            pass
    else:
        setup(sys.argv[2])
        try:
            asyncio.run(main(int(sys.argv[1])))
        except KeyboardInterrupt:
            pass
