# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# get the last release tag from git, and use it to find all merged PRs since
# that tag. extract their titles, labels and PR numbers and classify them into
# break changes, new # features, enhancements, bug fixes, and others based on
# their labels.
#
# The release version is generated based on the last release tag.  The tag
# should be in the format of "WAMR-major.minor.patch", where major, minor,
# and patch are numbers. If there is new feature in merged PRs, the minor
# version should be increased by 1, and the patch version should be reset to 0.
# If there is no new feature, the patch version should be increased by 1.
#
# new content should be inserted into the beginning of the RELEASE_NOTES.md file.
# in a form like:
#
# ``` markdown
# ## WAMR-major.minor.patch
#
# ### Breaking Changes
#
# ### New Features
#
# ### Bug Fixes
#
# ### Enhancements
#
# ### Others
# ```
# The path of RELEASE_NOTES.md is passed in as an command line argument.

import json
import os
import subprocess
import sys


def run_cmd(cmd):
    result = subprocess.run(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    )
    if result.returncode != 0:
        print(f"Error running command: {cmd}\n{result.stderr}")
        sys.exit(1)
    return result.stdout.strip()


def get_last_release_tag():
    tags = run_cmd("git tag --sort=-creatordate").splitlines()
    for tag in tags:
        if tag.startswith("WAMR-"):
            return tag
    return None


def get_merged_prs_since(tag):
    # Get commits  since the last release tag
    log_cmd = f'git log {tag}..HEAD --pretty=format:"%s"'
    logs = run_cmd(log_cmd).splitlines()

    print(f"Found {len(logs)} merge commits since last tag '{tag}'.")

    pr_numbers = []
    for line in logs:
        # assume the commit message ends with "(#PR_NUMBER)"
        if not line.endswith(")"):
            continue

        # Extract PR number
        parts = line.split("(#")
        if len(parts) < 2:
            continue

        # PR_NUMBER) -> PR_NUMBER
        pr_num = parts[1][:-1]
        pr_numbers.append(pr_num)
    return pr_numbers


def get_pr_info(pr_number):
    # Use GitHub CLI to get PR info
    pr_json = run_cmd(f"gh pr view {pr_number} --json title,labels,url")
    pr_data = json.loads(pr_json)
    title = pr_data.get("title", "")
    labels = [label["name"] for label in pr_data.get("labels", [])]
    url = pr_data.get("url", "")
    return title, labels, url


def classify_pr(title, labels, url):
    entry = f"- {title} (#{url.split('/')[-1]})"
    if "breaking-change" in labels:
        return "Breaking Changes", entry
    elif "new feature" in labels:
        return "New Features", entry
    elif "enhancement" in labels:
        return "Enhancements", entry
    elif "bug-fix" in labels:
        return "Bug Fixes", entry
    else:
        return "Others", entry


def generate_release_notes(pr_numbers):
    sections = {
        "Breaking Changes": [],
        "New Features": [],
        "Bug Fixes": [],
        "Enhancements": [],
        "Others": [],
    }
    for pr_num in pr_numbers:
        title, labels, url = get_pr_info(pr_num)
        section, entry = classify_pr(title, labels, url)
        sections[section].append(entry)
    return sections


def generate_version_string(last_tag, sections):
    last_tag_parts = last_tag.split("-")[-1]
    major, minor, patch = map(int, last_tag_parts.split("."))

    if sections["New Features"]:
        minor += 1
        patch = 0
    else:
        patch += 1

    return f"WAMR-{major}.{minor}.{patch}"


def format_release_notes(version, sections):
    notes = [f"## {version}\n"]
    for section in [
        "Breaking Changes",
        "New Features",
        "Bug Fixes",
        "Enhancements",
        "Others",
    ]:
        notes.append(f"### {section}\n")
        if sections[section]:
            notes.extend(sections[section])
        else:
            notes.append("")
        notes.append("")
    return "\n".join(notes)


def insert_release_notes(notes, RELEASE_NOTES_FILE):
    with open(RELEASE_NOTES_FILE, "r", encoding="utf-8") as f:
        old_content = f.read()
    with open(RELEASE_NOTES_FILE, "w", encoding="utf-8") as f:
        f.write(notes + old_content)


def set_action_output(name, value):
    """Set the output for GitHub Actions."""
    if not os.getenv("GITHUB_OUTPUT"):
        return

    print(f"{name}={value}")


def main(RELEASE_NOTES_FILE):
    last_tag = get_last_release_tag()
    if not last_tag:
        print("No release tag found.")
        sys.exit(1)

    print(f"Last release tag: {last_tag}")

    pr_numbers = get_merged_prs_since(last_tag)
    if not pr_numbers:
        print("No merged PRs since last release.")
        sys.exit(0)

    print(f"Found {len(pr_numbers)} merged PRs since last release.")
    print(f"PR numbers: {', '.join(pr_numbers)}")

    sections = generate_release_notes(pr_numbers)

    next_version = generate_version_string(last_tag, sections)
    print(f"Next version will be: {next_version}")

    notes = format_release_notes(next_version, sections)
    insert_release_notes(notes, RELEASE_NOTES_FILE)
    print(f"Release notes for {next_version} generated and inserted.")

    set_action_output("next_version", next_version)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        RELEASE_NOTES_FILE = sys.argv[1]
    else:
        RELEASE_NOTES_FILE = os.path.join(
            os.path.dirname(__file__), "../../RELEASE_NOTES.md"
        )

    main(RELEASE_NOTES_FILE)
