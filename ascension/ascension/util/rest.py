"""
This file is part of Ascension.
Copyright (C) 2018-2022 GNUnet e.V.

Ascension is free software: you can redistribute it and/or modify it
under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Ascension is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

SPDX-License-Identifier: AGPL3.0-or-later

Author: rexxnor
"""

import logging
import subprocess
import os

import requests
import requests.adapters


# Namestore REST API Error Codes
NAMESTORE_REST_API_ERRORS = {
    "Unknown Error": "Error not specified",
    "No identity found": ("Identity was not found with given name, "
                          "this is combined with the HTTP Error Code 404 Not Found"),
    "No default zone specified": \
        "Identity name was not given and no identiy was added to the subsystem namestore",
    "Namestore action failed": "The task of the namestore API (not REST API) failed",
    "No data": "Missing data",
    "Data invalid": "Wrong data given",
    "Error storing records": "POST request failed",
    "Deleting record failed": "DELETE request failed",
    "No record found": ("Delete failed due to missing record, "
                        "this is combined with the HTTP Error Code 404 Not Found"),
}


class GNUnetRestSession:
    """
    Class wrapping the functionality of the REST session handler
    """
    def __init__(self):
        """Constructor"""
        self.logger = logging.getLogger(__name__)
        self.session = requests.Session()
        self.session.headers = {"Content-Type": "application/json"}

        # Find the URL to connect to automatically
        listen_address = subprocess.run(['gnunet-config', '-s', 'rest', '-o', 'BIND_TO'],
            capture_output=True,
            check=True
        ).stdout.strip().decode()
        listen_port = subprocess.run(['gnunet-config', '-s', 'rest', '-o', 'HTTP_PORT'],
            capture_output=True,
            check=True
        ).stdout.strip().decode()
        self.base_url = f"http://{listen_address}:{listen_port}"

        # Retry adapter
        self.retries = requests.adapters.Retry(
            total=5,
            backoff_factor=1,
            allowed_methods=["GET", "POST", "PUT"]
        )
        self.session.mount(
            f"{self.base_url}", requests.adapters.HTTPAdapter(max_retries=self.retries)
        )

        # Basic Auth implementation
        basic_auth_enabled = subprocess.run(
            ['gnunet-config', '-s', 'rest', '-o', 'BASIC_AUTH_ENABLED'],
            capture_output=True,
            check=True
        ).stdout.strip().decode()

        if basic_auth_enabled == "YES":
            secret_location = subprocess.run(
                ['gnunet-config', '-f', '-s', 'rest', '-o', 'BASIC_AUTH_SECRET_FILE'],
                capture_output=True,
                check=True
            ).stdout.strip()
            secret = subprocess.run(
                ['cat', secret_location],
                capture_output=True,
                check=True,
            ).stdout.decode()
            self.session.auth = (os.environ.get("USER"), secret)


    def get(self, path: str, **kwargs):
        """
        Wrapper around requests.Session.get() with subsystem integration
        """
        self.logger.debug("Sending GET request with data %s, headers %s, to %s",
              str(kwargs.get("data")),
              str(kwargs.get("headers")),
              f"{self.base_url}{path}"
        )
        return self.session.get(f"{self.base_url}{path}", **kwargs)


    def post(self, path: str, **kwargs):
        """
        Wrapper around requests.Session.post() with subsystem integration
        """
        self.logger.debug("Sending POST request with data %s, headers %s, to %s",
              str(kwargs.get("data")),
              str(kwargs.get("headers")),
              f"{self.base_url}{path}"
        )
        return self.session.post(f"{self.base_url}{path}", **kwargs)


    def put(self, path: str, **kwargs):
        """
        Wrapper around requests.Session.put() with subsystem integration
        """
        self.logger.debug("Sending PUT request with data %s, headers %s, to %s",
              str(kwargs.get("data")),
              str(kwargs.get("headers")),
              f"{self.base_url}{path}"
        )
        return self.session.put(f"{self.base_url}{path}", **kwargs)


    def is_running(self) -> bool:
        """
        Checks if the REST API is running and responding
        """
        try:
            _ = self.get("")
        except requests.exceptions.ConnectionError:
            return False
        return True
