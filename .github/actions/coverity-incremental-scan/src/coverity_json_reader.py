# SOURCE: https://github.com/intel-innersource/applications.security.pull-request-differential-analysis/blob/8e23baf63789d8ef115a3bc4eedce7c90f00e852/PRDA/src/coverity/parsers/coverity_json_reader.py
# MODIFIED: Lacewell 2023
# [INTEL CONFIDENTIAL]
#
# Copyright 2022 Intel Corporation
#
# This software and the related documents are Intel copyrighted materials,
# and your use of them is governed by the express license under which they
# were provided to you (License). Unless the License provides otherwise,
# you may not use, modify, copy, publish, distribute, disclose or transmit
# this software or the related documents without Intel's prior written
# permission.
#
# This software and the related documents are provided as is, with no
# express or implied warranties, other than those that are expressly stated
# in the License

import os
import json
import logging
from coverity_problem import CoverityProblem

COMMENT_PREFACE = (
    "<!-- Comment managed by coverity-incremental-scan action, do not modify!"
)
PRESENT = "PRESENT"
NOT_PRESENT = "NOT_PRESENT"
UNKNOWN_FILE = "Unknown File"

logger = logging.getLogger(__name__)


class CoverityJsonReaderException(Exception):
    pass


class CoverityJsonReader:
    def __init__(self, args):
        self.json_path = args.file
        self.github_repo = f"https://github.com/{args.owner}/{args.repo_name}"
        self.github_repo_dir = args.github_repo_dir
        self.github_ref = args.github_ref
        self._problems = []

    def get_problems(self):
        return self._problems

    def add_job_summary(self, list_of_issues):
        if "GITHUB_STEP_SUMMARY" in os.environ:
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary_h:
                # PREFACE
                print(f"{COMMENT_PREFACE}-->", file=summary_h)

                if len(list_of_issues) == 0:
                    print("NO COVERITY ISSUES", file=summary_h)
                else:
                    print(
                        "| cid | merge_key | issueName | Impact | Component | main Description | cwe | checkerName | How to Fix | Issue location |",
                        file=summary_h,
                    )
                    print(
                        "| --- | --------- | --------- | ------ | --------- | ---------------- | --- | ----------- | ---------- | -------------- |",
                        file=summary_h,
                    )

                for unparsed_problem in list_of_issues:
                    coverity_issue = CoverityProblem(
                        unparsed_problem, base_dir=self.github_repo_dir
                    )

                    componentString = ",".join(
                        unparsed_problem["stateOnServer"]["components"]
                    )

                    print(
                        "| {} | {} | {} | {} | {} | {} | {} | {} | {} | [{}:{}]({}/blob/{}/{}#L{}) |".format(
                            coverity_issue.cid,
                            coverity_issue.merge_key,
                            coverity_issue.issueName,
                            coverity_issue.impactString,
                            componentString,
                            coverity_issue.mainEventDescription,
                            coverity_issue.cweString,
                            coverity_issue.checkerNameString,
                            coverity_issue.remediationString,
                            coverity_issue.file_path,
                            coverity_issue.mainEventLineNumber,
                            self.github_repo,
                            self.github_ref,
                            coverity_issue.file_path_relative_to_repo,
                            coverity_issue.mainEventLineNumber,
                        ),
                        file=summary_h,
                    )

    def parse_json(self):
        logger.debug(
            f"Coverity Json will be parsed from the following path {self.json_path}"
        )

        json_content = self._open_and_load_json_file()
        try:
            list_of_issues = json_content["issues"]
        except KeyError:
            raise CoverityJsonReaderException(
                f"Failed to Parse Coverity JSON report file. "
                f"The issues key is missing in the file: {self.json_path}"
            )

        if len(list_of_issues) > 0:
            self.add_job_summary(list_of_issues)

    def _open_and_load_json_file(self):
        try:
            with open(self.json_path) as file:
                return json.load(file)
        except FileNotFoundError:
            raise CoverityJsonReaderException(
                f"Failed to read the Coverity JSON output file from: {self.json_path}"
            )
