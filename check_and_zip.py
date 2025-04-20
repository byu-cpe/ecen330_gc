#!/usr/bin/python3

"""
This module is used to verify that your lab solution will build with the lab
submission system.
"""

import atexit
import pathlib
import argparse
import shutil
import subprocess
import sys
import zipfile
import getpass
import re
import os

STUDENT_REPO_PATH = pathlib.Path(__file__).absolute().parent.resolve()
TEST_REPO_PATH = (STUDENT_REPO_PATH / "test_repo").resolve()

USE_TEST_REPO = False # make test repo for build or run

class TermColors:
    """Terminal codes for printing in color"""

    # pylint: disable=too-few-public-methods

    PURPLE = "\033[95m"
    BLUE = "\033[94m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    END = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def print_color(color, *msg):
    """Print a message in color"""
    print(color + " ".join(str(item) for item in msg), TermColors.END)


def error(*msg, returncode=-1):
    """Print an error message and exit program"""

    print_color(TermColors.RED, "ERROR:", " ".join(str(item) for item in msg))
    sys.exit(returncode)


def clone_student_repo():
    """Clone a clean student repo into 'test_repo_path', deleting existing one if it exists"""

    # Delete existing repo
    shutil.rmtree(TEST_REPO_PATH, ignore_errors=True)

    if TEST_REPO_PATH.is_dir():
        error("Could not delete", TEST_REPO_PATH)

    print_color(TermColors.BLUE, "Cloning base repo into", TEST_REPO_PATH)
    proc = subprocess.run(
        [
            "git",
            "clone",
            "https://github.com/byu-cpe/ecen330_gc",
            str(TEST_REPO_PATH),
        ],
        cwd=STUDENT_REPO_PATH,
        check=False,
    )
    if proc.returncode:
        return False
    return True


def get_files_to_copy_and_zip(lab):
    """Build a list of (src,dest) files to copy into the temp repo given the lab"""

    print_color(TermColors.BLUE, "Enumerating files to copy/zip")

    src_lab_path = STUDENT_REPO_PATH / lab
    src_libs_path = STUDENT_REPO_PATH / "components"
    dest_lab_path = TEST_REPO_PATH / lab
    dest_libs_path = TEST_REPO_PATH / "components"

    # Build a list of files
    # Each entry in this list is a tuple in format (src - pathlib.Path, dest - pathlib.Path, include_in_zip? - boolean)
    # include_in_zip, False: for past submissions needed to compile that are not graded in the current lab and not included in the zip file.
    # include_in_zip, True : for new submissions that are graded in the current lab and included in the zip file.
    files = []
    # Examples:
        # files.append((src_libs_path / "pin/pin.c", dest_libs_path / "pin", True))
        # files.append((src_lab_path / "main/main.c", dest_lab_path / "main", True))
    if lab == "lab01":
        files.append((src_lab_path / "main/main.c", dest_lab_path / "main", True))
    elif lab == "lab02":
        files.append((src_libs_path / "pin/pin.c", dest_libs_path / "pin", True))
    elif lab == "lab03":
        files.append((src_lab_path / "main/main.c", dest_lab_path / "main", True))
    elif lab == "lab04":
        files.append((src_libs_path / "joy/joy.c", dest_libs_path / "joy", True))
        files.append((src_libs_path / "tone/tone.c", dest_libs_path / "tone", True))
    elif lab == "lab05":
        files.append((src_lab_path / "main/game.c", dest_lab_path / "main", True))
        files.append((src_lab_path / "main/com.c", dest_lab_path / "main", True))
    elif lab == "lab06":
        files.append((src_lab_path / "main/game.c", dest_lab_path / "main", True))
        files.append((src_lab_path / "main/missile.c", dest_lab_path / "main", True))
        files.append((src_lab_path / "main/plane.c", dest_lab_path / "main", True))
    elif lab == "lab07":
        # Add all files in path
        with os.scandir(src_lab_path) as it:
            for entry in it:
                if entry.name != "build": # skip the "build" directory
                    files.append((src_lab_path / entry.name, dest_lab_path / entry.name, True))

    if USE_TEST_REPO:
        print(
            len(files), "files to copy to the test repo."
        )
    print(
        len([f for f in files if f[2]]), "files to include in the submission zip archive."
    )
    return files


def copy_solution_files(files_to_copy):
    """Copy student files to the temp repo"""

    print_color(TermColors.BLUE, "Copying solution files to the test_repo")

    # files_to_copy provides a list of files in (src_path, dest_path, include_in_zip?) format
    for (src, dest, _) in files_to_copy:
        print(
            "Copying", src.relative_to(STUDENT_REPO_PATH), "to", dest.relative_to(STUDENT_REPO_PATH)
        )
        if src.is_file():
            shutil.copy(src, dest)
        elif src.is_dir():
            shutil.copytree(src, dest / src.name)
        else:
            error("Required file", src, "does not exist.")


def build(lab):
    """Run ESP-IDF"""

    build_path = TEST_REPO_PATH / lab / "build"

    print_color(TermColors.BLUE, "Removing build directory (" + str(build_path) + ")")
    shutil.rmtree(build_path, ignore_errors=True)

    # Run idf.py
    print_color(TermColors.BLUE, "Trying to build")
    proc = subprocess.run(
        ["idf.py", "build"],
        cwd=TEST_REPO_PATH / lab,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    build_output = proc.stdout.decode()
    print(build_output)
    if proc.returncode:
        return False

    # If user code has warnings, ask if they still want to compile.
    matches = re.findall(" warning: ", build_output)
    if matches:
        input_txt = ""
        while input_txt not in ["y", "n"]:
            input_txt = input(
                TermColors.YELLOW
                + "Your code has "
                + str(len(matches))
                + " warning(s). You will lose a coding standard point for each warning. Are you sure you want to continue? (y/n) "
                + TermColors.END
            ).lower()
        if input_txt == "n":
            error("User canceled zip process.")

    return True


def run(lab, elf_name=None):
    """Run the lab program (flash and monitor)"""

    try:
        proc = subprocess.run(
            ["idf.py", "flash", "monitor"],
            cwd=TEST_REPO_PATH / lab,
            check=False
        )
    except KeyboardInterrupt:
        print()


def zip(lab, files):
    """Zip the lab files"""

    zip_path = STUDENT_REPO_PATH / (getpass.getuser() + "_" + lab + ".zip")
    print_color(TermColors.BLUE, "Creating zip file", zip_path.relative_to(STUDENT_REPO_PATH))
    if zip_path.is_file():
        print("Deleting existing file.")
        zip_path.unlink()
    with zipfile.ZipFile(zip_path, "w") as zf:
        print("Created new zip file")
        # Loop through files that are marked for zip (f[2] == True)
        for f in (f for f in files if f[2]):
            if f[0].is_file():
                # Write file to zip file
                print("Adding", f[0].relative_to(STUDENT_REPO_PATH))
                zf.write(f[0], arcname=f[0].name)
            elif f[0].is_dir():
                # Directory -- do a glob search and write all files to zip file
                for sub_f in f[0].rglob("*"):
                    print("Adding", sub_f.relative_to(STUDENT_REPO_PATH))
                    zf.write(sub_f, arcname=sub_f.relative_to(f[0].parent))
            else:
                error(f[0].relative_to(STUDENT_REPO_PATH), "does not exist")

    return zip_path.relative_to(STUDENT_REPO_PATH)


def get_milestones(lab):
    """Return the different milestones for the lab."""
    return [("main", None)]


def exit_handler():
    # Delete test repo
    print_color(TermColors.BLUE, "Removing", TEST_REPO_PATH.name)
    shutil.rmtree(TEST_REPO_PATH, ignore_errors=True)


def main():
    """Copy files into temp repo, build and run lab"""

    atexit.register(exit_handler)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "lab",
        choices=[
            "lab01",
            "lab02",
            "lab03",
            "lab04",
            "lab05",
            "lab06",
            "lab07",
        ],
    )
    parser.add_argument(
        "--build", action="store_true", help="build code"
    )
    parser.add_argument(
        "--run", action="store_true", help="run code after building"
    )
    args = parser.parse_args()
    USE_TEST_REPO = args.build or args.run

    # Get a list of files needed to build and zip
    files = get_files_to_copy_and_zip(args.lab)

    if USE_TEST_REPO:
        # Clone/clean student repo
        if not clone_student_repo():
            input_txt = ""
            while input_txt not in ["y", "n"]:
                input_txt = input(
                    TermColors.YELLOW
                    + "Could not clone GitHub repo. Perhaps you are not connected to the internet. "
                    "It is recommended that you cancel the process, connect to the internet, and retry. "
                    "If you proceed, the generated zip file will be untested, and may not build properly on the TA's evaluation system. "
                    "Are you sure you want to proceed? (y/n) " + TermColors.END
                ).lower()
            if input_txt == "n":
                error("User canceled zip process.")

        else:
            # Copy over necessary files to test repo
            copy_solution_files(files)

            # See if the code builds
            build_success = build(args.lab)

            if not build_success:
                s = ""
                while s not in ("y", "n"):
                    s = input(
                        TermColors.RED + "Build failed. Continue? (y/n)" + TermColors.END
                    ).lower()
                if s == "n":
                    sys.exit(0)

            # Run it
            if args.run:
                # Loop through executable files
                for (milestone_name, elf_name) in get_milestones(args.lab):
                    input(
                        TermColors.BLUE
                        + "Ready to run "
                        + milestone_name
                        + ". Hit <Enter> to continue."
                        + TermColors.END
                    )
                    print_color(TermColors.BLUE, "Running", args.lab, milestone_name)
                    print_color(
                        TermColors.BLUE,
                        "If the emulator won't close, press Ctrl+C in this terminal.",
                    )
                    run(args.lab, elf_name)

    # Zip it
    zip_relpath = zip(args.lab, files)

    print_color(TermColors.BLUE, "Created", zip_relpath, "\nDone.")


if __name__ == "__main__":
    main()
