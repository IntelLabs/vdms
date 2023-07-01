# SOURCE: https://github.com/intel-innersource/applications.security.pull-request-differential-analysis/blob/8e23baf63789d8ef115a3bc4eedce7c90f00e852/PRDA/src/coverity/model/coverity_problem.py
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
# in the License.

import logging
import os
import platform
from enum import Enum, unique
from pathlib import Path

logger = logging.getLogger(__name__)


class CoverityDataParserException(Exception):
    pass


class CoverityProblemParserException(Exception):
    pass


class CoverityDataParser:
    def __init__(self, data):
        self.data = data

    def parse_by_key(self, key):
        try:
            return self.data[key]
        except KeyError:
            raise CoverityDataParserException(
                f"Failed to parse Coverity Data. The {key} is missing."
            )


@unique
class Classification(Enum):
    """
    Class holding classification statuses for coverity
    """

    UNCLASSIFIED = "Unclassified"
    PENDING = "Pending"
    FALSE_POSITIVE = "False Positive"
    INTENTIONAL = "Intentional"
    BUG = "Bug"

    @classmethod
    def allowed_option(cls, value: str):
        """
        Checks if classification is one of available
        :param value: classification status
        :return: True if value is a correct classification, False value is incorrect
        """
        return value in [classification.value for classification in cls]


class CoverityProblemId:
    def __init__(self, cid, merge_key):
        self.cid = cid
        self.merge_key = merge_key

    def get_cid(self):
        return self.cid

    def __str__(self):
        return f"cid: {self.cid} merge_key: {self.merge_key}"

    def __eq__(self, other):
        return self.cid == other.cid and self.merge_key == other.merge_key

    def __hash__(self):
        return hash((self.cid, self.merge_key))


class CoverityProblem:
    def __init__(self, unparsed_problem, base_dir=None):
        parser = CoverityProblemParser(unparsed_problem)
        self.classification = parser.parse_classification()
        self.cid = parser.parse_cid()
        self.merge_key = parser.parse_by_key("mergeKey")
        self.checker_name = parser.parse_by_key("checkerName")
        self.file_path = parser.parse_by_key("mainEventFilePathname")
        if base_dir is None:
            base_dir = str(Path(self.file_path).parents[-2])
        self.file_path_relative_to_repo = self.file_path.replace(f"{base_dir}/", "")
        self.file_path_unix_style = self.file_path.replace("\\", "/")
        self.event_description = parser.parse_event_description()
        self.line = parser.parse_line_number()
        self.mainEventLineNumber = parser.parse_by_key("mainEventLineNumber")
        self.position = None

        self.issueName = (
            parser.parse_by_key("checkerProperties")["subcategoryShortDescription"]
            if parser.parse_by_key("checkerProperties")
            else self.checker_name
        )
        self.checkerNameString = (
            f"{self.checker_name}" if parser.parse_by_key("checkerProperties") else ""
        )
        self.impactString = (
            parser.parse_by_key("checkerProperties")["impact"]
            if parser.parse_by_key("checkerProperties")
            else "Unknown"
        )
        self.cweString = (
            f"CWE-{parser.parse_by_key('checkerProperties')['cweCategory']}"
            if parser.parse_by_key("checkerProperties")
            else ""
        )
        mainEvent = [
            event for event in parser.parse_by_key("events") if event["main"]
        ]  # issue.events.find(event => event.main === true)
        self.mainEventDescription = (
            mainEvent[0]["eventDescription"] if len(mainEvent) > 0 else ""
        )
        remediationEvent = [
            event for event in parser.parse_by_key("events") if event["remediation"]
        ]  # issue.events.find(event => event.remediation === true)
        self.remediationString = (
            f"{remediationEvent[0]['eventDescription']}"
            if len(remediationEvent) > 0
            else ""
        )

    def set_file_path(self, file_path):
        self.file_path = file_path
        self.file_path_unix_style = file_path.replace("\\", "/")

    def get_status_as_str(self):
        return self.classification.value

    def get_problem_id(self):
        return CoverityProblemId(self.cid, self.merge_key)

    def get_description_data(self):
        return {
            "os": platform.system(),
            "file_path": self.file_path,
            "line": self.line,
            "status": self.get_status_as_str(),
            "code": self.checker_name,
            "message": self.event_description,
            "cid": self.cid,
            "merge_key": self.merge_key,
        }

    def get_problem_id_on_server(self):
        return self.cid

    def __eq__(self, other):
        return (
            self.cid == other.cid
            and self.merge_key == other.merge_key
            and os.path.basename(self.file_path) == os.path.basename(other.file_path)
        )

    def __hash__(self):
        return hash(
            (
                self.classification,
                self.cid,
                self.merge_key,
                self.checker_name,
                self.file_path,
                self.event_description,
                self.line,
            )
        )

    def __repr__(self):
        return (
            f"CoverityProblem(CID={self.cid}, merge_key={self.merge_key}, "
            f"classification={self.classification}, file_path={self.file_path_unix_style}, line={self.line}, "
            f"checker_name={self.checker_name})"
        )

    def __str__(self):
        return (
            f"CoverityProblem: merge_key: {self.merge_key}"
            f"cid: {self.cid}"
            f"checker_name: {self.checker_name}"
            f"file_path: {self.file_path}"
            f"classification: {self.get_status_as_str()}"
            f"line: {self.line}"
        )


class CoverityProblemParser(CoverityDataParser):
    def __init__(self, problem_data):
        super().__init__(problem_data)
        self.events = self._parse_events()
        self.triage = self._parse_triage()

    def _parse_events(self):
        events = []
        for unparsed_event in self.parse_by_key("events"):
            coverity_event = CoverityEvent(unparsed_event)
            events.append(coverity_event)
        if len(events) < 1:
            raise CoverityProblemParserException(
                "There are no events provided in Coverity problem data"
            )
        return events

    def _parse_triage(self):
        try:
            triage_data = self.data["stateOnServer"]["triage"]
        except KeyError as e:
            missing_key = e.args[0]
            raise CoverityProblemParserException(
                f"{missing_key} key not present in coverity problem data"
            )
        return CoverityTriage(triage_data)

    def parse_cid(self):
        try:
            cid = self.data["stateOnServer"]["cid"]
        except KeyError as e:
            missing_key = e.args[0]
            raise CoverityProblemParserException(
                f"{missing_key} key not present in coverity problem data"
            )
        if not cid:
            raise CoverityProblemParserException(
                "CID not present in coverity problem data"
            )
        return cid

    def parse_event_description(self):
        return self.events[0].event_description

    def parse_line_number(self):
        return self.events[0].line_number

    def parse_classification(self):
        return self.triage.get_classification()


class CoverityTriageDataParser(CoverityDataParser):
    def __init__(self, data):
        super().__init__(data)

    def parse_by_key(self, key):
        if not self.data:
            return None
        return super().parse_by_key(key)


class CoverityTriage:
    """
    Class responsible for Mapping Coverity Triage. Not mapping all coverity json fields
    """

    def __init__(self, unparsed_triage):
        parser = CoverityTriageDataParser(unparsed_triage)
        self.classification = Classification(parser.parse_by_key("classification"))

    def get_classification(self) -> Classification:
        return self.classification


class CoverityEvent:
    """
    class mapping Coverity Event, not all fields are mapped
    """

    def __init__(self, unparsed_event):
        parser = CoverityDataParser(unparsed_event)
        self.event_description = parser.parse_by_key("eventDescription")
        self.event_number = parser.parse_by_key("eventNumber")
        self.line_number = parser.parse_by_key("lineNumber")
        self.file_pathname = parser.parse_by_key("filePathname")

    def __eq__(self, other):
        return (
            self.event_description == other.event_description
            and self.event_number == other.event_number
            and self.line_number == other.line_number
            and self.file_pathname == other.file_pathname
        )
