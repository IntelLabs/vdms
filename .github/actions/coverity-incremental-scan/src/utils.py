import logging
import re

filename = __import__("datetime").datetime.now().strftime("%Y-%m-%d_%H-%M")
from enum import IntEnum


def log_exception_stack(logger, exception):
    """
    Recursively logs exception and its context messages.
    :param logger: Use this logger for logging exception messages.
    :param exception: Exception to log.

    SOURCE https://github.com/intel-innersource/applications.security.pull-request-differential-analysis/blob/8e23baf63789d8ef115a3bc4eedce7c90f00e852/PRDA/src/core/logging/log_exception_stack.py
    """
    if exception.__context__:
        log_exception_stack(logger, exception.__context__)
    logger.error(exception)


class ExitCode(IntEnum):
    SUCCESS = 0
    FAIL = 1
    ISSUE_OR_PROBLEM_FOUND = 2


class IssueFoundException(Exception):
    """
    Exception raised in case of failed issue classification found in github comments
    """


class InvalidConfigException(Exception):
    """
    Exception raised in case of invalid config file
    """


class TokenFilter(logging.Filter):
    def filter(self, record):
        message = record.getMessage()
        token = re.search("not-used:(.*)@", message)
        if token:
            token = token.group(1)
            record.msg = message.replace(token, "*****")
        return True


LOGGING_CONFIG = {
    "version": 1,
    "disable_existing_loggers": False,
    "loggers": {
        "": {"level": "DEBUG", "handlers": ["console_handler"]},  # , 'file_handler']
    },
    "handlers": {
        "console_handler": {
            "level": "INFO",
            "formatter": "basic_formatter",
            "class": "logging.StreamHandler",
            "stream": "ext://sys.stdout",
            "filters": ["token_filter"],
        },
        "file_handler": {
            "level": "DEBUG",
            "formatter": "basic_formatter",
            "class": "logging.handlers.RotatingFileHandler",
            "mode": "a",
            "filename": f"{filename}.log",
            "maxBytes": 24 * 1024 * 1024,
            "backupCount": 20,
            "filters": ["token_filter"],
        },
    },
    "formatters": {
        "basic_formatter": {
            "format": "[%(asctime)s][%(processName)s][%(name)s][%(levelname)s][%(message)s]",
            "datefmt": "%Y-%m-%d %H:%M:%S",
        },
    },
    "filters": {
        "token_filter": {
            "()": TokenFilter,
        }
    },
}
