#!/bin/bash -f

# MBDyn (C) is a multibody analysis code.
# http://www.mbdyn.org
#
# Copyright (C) 1996-2023
#
# Pierangelo Masarati	<pierangelo.masarati@polimi.it>
# Paolo Mantegazza	<paolo.mantegazza@polimi.it>
#
# Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
# via La Masa, 34 - 20156 Milano, Italy
# http://www.aero.polimi.it
#
# Changing this copyright notice is forbidden.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (version 2 of the License).
#
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# AUTHOR: Reinhard Resch <mbdyn-user@a1.net>
# Copyright (C) 2023(-2023) all rights reserved.

# The copyright of this code is transferred
# to Pierangelo Masarati and Paolo Mantegazza
# for use in the software MBDyn as described
# in the GNU Public License version 2.1

## Octave package testsuite based on Octave's __run_test_suite__ function
## At the moment "mboct-mbdyn-pkg" is the only package which will perform unit tests on MBDyn

program_name="$0"
OCT_PKG_LIST="${OCT_PKG_LIST:-mboct-mbdyn-pkg}"
OCT_PKG_TESTS="${OCT_PKG_TESTS:-mboct-mbdyn-pkg:yes}"
OCT_PKG_TIMEOUT="${OCT_PKG_TIMEOUT:-mboct-mbdyn-pkg:unlimited}"
OCT_PKG_TEST_DIR="${OCT_PKG_TEST_DIR:-octave-pkg-testsuite}"
OCTAVE_EXEC="${OCTAVE_EXEC:-octave}"
OCT_PKG_TESTS_VERBOSE="${OCT_PKG_TESTS_VERBOSE:-no}"
OCT_PKG_PRINT_RES="${OCT_PKG_PRINT_RES:-no}"
OCT_PKG_TEST_MODE="${OCT_PKG_TEST_MODE:-pkg}"

echo $program_name

if test "$(basename ${program_name})" = "${program_name}" && ! test -z "$(which ${program_name})"; then
    ## Path of script was inside the environment variable PATH
    program_name="$(which ${program_name})"
    echo "${program_name} found on PATH"
fi

program_name=`realpath ${program_name}`

if ! test -z "${program_name}"; then
    AWKPATH="$(dirname "${program_name}"):${AWKPATH}" ## Needed for parse_test_suite_status.awk
fi

while ! test -z "$1"; do
    case "$1" in
        --octave-pkg-list)
            OCT_PKG_LIST="$2"
            shift
            ;;
        --octave-pkg-tests)
            OCT_PKG_TESTS="$2"
            shift
            ;;
        --octave-pkg-test-dir)
            OCT_PKG_TEST_DIR="$2"
            shift
            ;;
        --octave-exec)
            OCTAVE_EXEC="$2"
            shift
            ;;
        --octave-pkg-test-mode)
            OCT_PKG_TEST_MODE="$2"
            shift
            ;;
        --verbose)
            OCT_PKG_TESTS_VERBOSE="$2"
            shift
            ;;
        --timeout)
            OCT_PKG_TIMEOUT="$2"
            shift
            ;;
        --print-resources)
            OCT_PKG_PRINT_RES="$2"
            shift
            ;;
        --help|-h)
            printf "%s\n --octave-pkg-list <list-of-packages>\n --octave-pkg-tests <list-of-tests>\n --octave-pkg-test-dir <output-dir>\n --timeout <timeout-value>\n --help\n" "${program_name}"
            exit 1;
            ;;
        *)
            printf "%s: invalid argument \"%s\"\n" "${program_name}" "$1"
            exit 1
            ;;
    esac
    shift
done

if ! test -d "${OCT_PKG_TEST_DIR}"; then
    if ! mkdir -p "${OCT_PKG_TEST_DIR}"; then
        echo "Failed to create directory \"${OCT_PKG_TEST_DIR}\""
        exit 1
    fi
fi

if ! test -d "${OCT_PKG_TEST_DIR}"; then
    echo "Directory \"${OCT_PKG_TEST_DIR}\" does not exist"
    exit 1
fi

case "${OCT_PKG_TEST_MODE}" in
    pkg|single)
        echo "Test mode: ${OCT_PKG_TEST_MODE}"
        ;;
    *)
        echo "Invalid value for --octave-pkg-test-mode \"${OCT_PKG_TEST_MODE}\""
        exit 1
esac

OCT_PKG_TEST_DIR="$(realpath ${OCT_PKG_TEST_DIR})"

test_status="passed"
failed_packages=""

for pkgname in ${OCT_PKG_LIST}; do
    pkg_test_flag=$(echo ${OCT_PKG_TESTS} | awk -v RS=" " -F ":" "/^${pkgname}\>/{print \$2}")

    case "${pkg_test_flag}" in
        yes)
            printf "Test package \"%s\"\n" "${pkgname}"
            ;;
        *)
            printf "Tests of package \"%s\" skipped\n" "${pkgname}"
            continue
            ;;
    esac

    echo "Temporary directory: ${OCT_PKG_TEST_DIR}/${pkgname}"

    if ! test -d "${OCT_PKG_TEST_DIR}/${pkgname}"; then
        if ! mkdir "${OCT_PKG_TEST_DIR}/${pkgname}"; then
            echo "Failed to create directory ${OCT_PKG_TEST_DIR}/${pkgname}"
            exit 1
        fi
    fi

    ## Octave allows us to set TMPDIR in order to store all the temporary files in a single folder.
    ## This will make it easier to delete those files.
    export TMPDIR="${OCT_PKG_TEST_DIR}/${pkgname}"

    rm -f ${OCT_PKG_TEST_DIR}/${pkgname}/fntests.*

    case "${OCT_PKG_TEST_MODE}" in
        pkg)
            OCTAVE_CODE=`printf "pkg('load','%s');p=pkg('list','%s');__run_test_suite__({p{1}.dir},{});" "${pkgname}" "${pkgname}"`
            ;;
        single)
            OCTAVE_CMD_FUNCTIONS=$(printf "p=pkg('describe','-verbose','%s'); for i=1:numel(p{1}.provides) for j=1:numel(p{1}.provides{i}.functions) disp(p{1}.provides{i}.functions{j}); endfor; endfor" "${pkgname}")
            #echo ${OCTAVE_CMD_FUNCTIONS}
            OCTAVE_PKG_FUNCTIONS=`${OCTAVE_EXEC} --eval "${OCTAVE_CMD_FUNCTIONS}"`
            rc=$?
            if test ${rc} != 0; then
                echo "${OCTAVE_CMD_FUNCTIONS} failed with status ${rc}"
                exit 1
            fi
            OCTAVE_CODE=""
            for pkg_function_name in ${OCTAVE_PKG_FUNCTIONS}; do
                OCTAVE_CODE="${OCTAVE_CODE} $(printf "pkg('load','%s');test('%s');" "${pkgname}" "${pkg_function_name}")"
                #OCTAVE_CODE="${OCTAVE_CODE} $(printf "pkg('load','%s');addpath('~/work/mboct-git/mboct-octave-pkg/inst');test('%s');" "${pkgname}" "${pkg_function_name}")"
            done
            #echo ${OCTAVE_PKG_FUNCTIONS}
            #echo ${OCTAVE_CODE}
            ;;
        *)
            exit 1
            ;;
    esac
    #exit 0
    pkg_test_timeout=$(echo ${OCT_PKG_TIMEOUT} | awk -v RS=" " -F ":" "/^${pkgname}\>/{print \$2}")

    if test -z "${pkg_test_timeout}"; then
        pkg_test_timeout="unlimited"
    fi

    case "${pkg_test_timeout}" in
        unlimited)
            TIMEOUT_CMD=""
            ;;
        *)
            ## Octave will not dump a core file on SIGINT
            TIMEOUT_CMD="timeout --signal=SIGINT ${pkg_test_timeout} "
            ;;
    esac

    pkg_test_timing="${OCT_PKG_TEST_DIR}/${pkgname}/fntests.tm"
    pkg_test_timing_cmd="/usr/bin/time --verbose --output ${pkg_test_timing}"

    for octave_code_cmd in ${OCTAVE_CODE}; do
        OCTAVE_CMD=$(printf '%s%s %s -qfH --eval %s' "${TIMEOUT_CMD}" "${pkg_test_timing_cmd}" "${OCTAVE_EXEC}" "${octave_code_cmd}")

        if test ${OCT_PKG_PRINT_RES} = "yes"; then
            echo "Memory usage before test:"
            vmstat -S M
            echo "Temporary files before test:"
            ls -lhF "${OCT_PKG_TEST_DIR}/${pkgname}"
            echo "Disk usage before test:"
            df -h
        fi

        curr_test_status="failed"

        pkg_test_output_file="${OCT_PKG_TEST_DIR}/${pkgname}/fntests.out"

        echo "${OCTAVE_CMD}"

        curr_dir="`pwd`"

        if ! cd "${OCT_PKG_TEST_DIR}/${pkgname}"; then
            exit 1
        fi

        case "${OCT_PKG_TESTS_VERBOSE}" in
            yes)
                ${OCTAVE_CMD} 2>&1 | tee "${pkg_test_output_file}" 2>&1 | grep -i -E '^\!\!\!\!\! test failed$|/^PASSES\>/|[[:alnum:]]+/[[:alnum:]]+/[[:alnum:]]+\.m\>|pass|fail|^Summary|^Integrated test scripts|\.m files have no tests\.$'
                ;;
            *)
                ${OCTAVE_CMD} >& "${pkg_test_output_file}"
                ;;
        esac

        if ! cd "${curr_dir}"; then
            exit 1
        fi

        rc=$?

        case ${rc} in
            0)
                echo "${OCTAVE_CMD} completed with status 0"
                case ${OCT_PKG_TEST_MODE} in
                    pkg|single)
                        if awk -f parse_test_suite_status.awk "${pkg_test_output_file}"; then
                            curr_test_status="passed"
                        fi
                        ;;
                    single)
                        ## If we are using the test function, then an exit status of zero already indicates that all tests have passed
                        curr_test_status="passed"
                        ;;
                esac
                ;;
            124)
                echo "${OCTAVE_CMD} failed with timeout"
                ;;
        esac

        if test -f "${pkg_test_timing}"; then
            echo "Resources used by ${OCTAVE_CMD}"
            cat "${pkg_test_timing}"
        fi

        if test ${OCT_PKG_PRINT_RES} = "yes"; then
            echo "Memory usage after test:"
            vmstat -S M
            echo "Temporary files after test:"
            ls -lhF ${OCT_PKG_TEST_DIR}
            echo "Disk usage after test:"
            df -h
        fi

        case "${curr_test_status}" in
            passed)
                printf "octave testsuite for package \"%s\" passed\n" "${pkgname}"
                ;;
            *)
                printf "octave testsuite for package \"%s\" failed\n" "${pkgname}"
                if test -f ${OCT_PKG_TEST_DIR}/fntests.log; then
                    cat ${OCT_PKG_TEST_DIR}/fntests.log;
                else
                    echo "${OCT_PKG_TEST_DIR}/fntests.log not found";
                fi
                test_status="failed"
                failed_packages="${failed_packages} ${pkgname}"
                case "${OCT_PKG_TEST_MODE}" in
                    single)
                        failed_packages="${failed_packages}:${octave_code_cmd}"
                        ;;
                esac
        esac

        echo "Remove all temporary files after the test:"
        find "${OCT_PKG_TEST_DIR}/${pkgname}" '(' -type f -and '(' -name 'oct-*' -or -name 'fntests.*' ')' ')' -delete
    done
done

case "${test_status}" in
    passed)
        echo "all tests passed"
        ;;
    *)
        echo "the following packages did not pass:"
        for pkgname in ${failed_packages}; do
            printf "%s\n" "${pkgname}"
        done
        exit 1
        ;;
esac
