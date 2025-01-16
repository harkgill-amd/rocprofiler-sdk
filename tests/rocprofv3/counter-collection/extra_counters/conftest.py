#!/usr/bin/env python3

import json
import pytest
import pandas as pd

from rocprofiler_sdk.pytest_utils.dotdict import dotdict
from rocprofiler_sdk.pytest_utils import collapse_dict_list


def pytest_addoption(parser):
    parser.addoption("--input", action="store", help="Path to csv file.")
    parser.addoption(
        "--json-input",
        action="store",
        help="Path to JSON file.",
    )


@pytest.fixture
def input_data(request):
    filename = request.config.getoption("--input")
    with open(filename, "r") as inp:
        return pd.read_csv(inp)


@pytest.fixture
def json_data(request):
    filename = request.config.getoption("--json-input")
    with open(filename, "r") as inp:
        return dotdict(collapse_dict_list(json.load(inp)))
