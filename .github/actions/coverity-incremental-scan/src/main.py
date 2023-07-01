import logging.config
import sys, os
import argparse
from pathlib import Path
import requests
from utils import log_exception_stack, ExitCode, LOGGING_CONFIG
from coverity_json_reader import CoverityJsonReader
import json

logging.config.dictConfig(LOGGING_CONFIG)
logger = logging.getLogger(__name__)

parser = argparse.ArgumentParser()
parser.add_argument(
    "-f",
    "--file",
    type=Path,
    default="coverity-results.json",
    required=True,
    help="JSON Coverity Scan",
)
parser.add_argument(
    "--github_repo_dir",
    type=str,
    default=None,
    help="gitHub Repo Directory in Coverity Result [default: /vdms]",
)
parser.add_argument(
    "-c", "--github_ref", type=str, required=True, help="Commit checkout reference"
)
parser.add_argument(
    "-o",
    "--outfile",
    type=str,
    default=None,
    required=True,
    help="Path to write Coverity results as Markdown",
)

parser.add_argument(
    "--owner",
    type=str,
    default="intel-innersource",
    help="Github Repo Owner [default: intel-innersource]",
)
parser.add_argument(
    "--repo_name",
    type=str,
    default="libraries.databases.visual.vdms",
    help="GitHub Repo Name [default: libraries.databases.visual.vdms]",
)

args = parser.parse_args()


def main():
    try:
        cov_results = CoverityJsonReader(args)
        cov_results.parse_json()

    except Exception as error:
        log_exception_stack(logger, error)
        sys.exit(int(ExitCode.FAIL))


if __name__ == "__main__":
    main()
