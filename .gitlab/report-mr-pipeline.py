#!/usr/bin/env python3

# report-mr-pipeline.py - utility for GIMP CI.
# Copyright (C) 2026 Jehan
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gitlab
import os
import sys

def main():
  token       = os.getenv('WILBOT_GUEST')
  api_url     = os.getenv("CI_SERVER_URL")
  project_id  = os.getenv("CI_PROJECT_ID")
  mr_iid      = os.getenv("CI_MERGE_REQUEST_IID")
  pipeline_id = os.getenv('CI_PIPELINE_ID')

  gl = gitlab.Gitlab(api_url, private_token=token, retry_transient_errors=True)
  try:
    gl.auth()
  except gitlab.GitlabAuthenticationError as exc:
    print(f"[ERROR] GitLab authentication failed: {exc}")
    print(f"        api_url: {api_url}")
    if token is None or len(token) == 0:
      # Do not leak the token in the CI, but warn if it's missing.
      print(f"        token missing.")
    sys.exit(1)

  project  = gl.projects.get(project_id)
  mr       = project.mergerequests.get(mr_iid)
  pipeline = project.pipelines.get(pipeline_id)

  artifacts = { }
  base_path = 'artifacts/browse/'
  for job in pipeline.jobs.list():
    if job.stage != 'distribution':
      continue
    if job.name == 'dist-appimage-weekly':
      url = os.path.join(job.web_url, base_path, 'build/linux/appimage/_Output/')
      artifacts['AppImages (x86_64 and Aarch64)'] = url
    elif job.name == 'dist-flatpak-weekly':
      url = os.path.join(job.web_url, base_path, 'build/linux/flatpak/_Output/')
      artifacts['Flatpak (x86_64 and Aarch64)'] = url
    elif job.name == 'dist-snap-weekly':
      url = os.path.join(job.web_url, base_path, 'build/linux/snap/_Output/')
      artifacts['Snap (x86_64 and Aarch64)'] = url
    elif job.name == 'dist-installer-weekly':
      url = os.path.join(job.web_url, base_path, 'build/windows/installer/_Output/')
      artifacts['Windows universal installer'] = url
    elif job.name == 'dist-store-weekly':
      url = os.path.join(job.web_url, base_path, 'build/windows/store/_Output/')
      artifacts['Windows MSIX'] = url
    elif job.name == 'dist-mac-weekly':
      url = os.path.join(job.web_url, base_path, 'build/macos/dmg/_Output/')
      artifacts['macOS DMG (x86_64 and Aarch64)'] = url

  comment = ''
  for artifact in artifacts:
    comment += f'* [{artifact}]({artifacts[artifact]})\n'

  if comment != '':
    comment = f'[Pipeline #{pipeline.id}]({pipeline.web_url}) created the following test binaries:\n\n{comment}'
    mr.notes.create({"body": comment})

if __name__ == "__main__":
  main()
