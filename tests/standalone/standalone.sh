#!/bin/bash

# get every run.sh under standalone sub-direcotry
#
#       for f in folders_include_run_sh
#           cd sub-direcoty
#           ./run.sh

# Usage:
# ./standalone.sh --aot|--classic-interp|--fast-interp|--fast-jit|--jit|--multi-tier-jit \
#                 [--sgx|--no-sgx] [--thread|--no-thread] [--simd|--no-simd]
#
# Note:
#  The order of the options can not be exchanged
#

if [[ $1 == "--classic-interp" ]]; then
    # long time to run gcc-loops, smallpt and tsf with classic interpreter
    IGNORE_LIST+=("gcc-loops" "smallpt" "tsf")
fi
if [[ $1 == "--classic-interp" || $1 == "--fast-interp" ]]; then
    # long time to run mandelbrot with interpreter
    IGNORE_LIST+=("mandelbrot")
fi
if [[ $1 == "--jit" ]]; then
    # long time to compile test-printf with llvm-jit
    IGNORE_LIST+=("test-printf")
fi
if [[ $2 == "--sgx" ]]; then
    # require to allocate/mmap large memory in sgx
    # need to modify Enclave.config.xml
    IGNORE_LIST+=("stream")
fi
if [[ $3 != "--thread" ]]; then
    # test-pthread requires thread support
    IGNORE_LIST+=("test-pthread")
fi
if [[ $4 != "--simd" || $1 == "--classic-interp" || $1 == "--fast-interp"
      || $1 == "--fast-jit" ]]; then
    # blake3 and c-wasm-simd128-example require SIMD support
    IGNORE_LIST+=("blake3" "c-wasm-simd128-example")
fi

if [[ -z $5 ]]; then
    TARGET="X86_64"
else
    TARGET=$5
fi

function contain()
{
    # [$1, $-1)
    local list=${@:0:${#}}
    # [$-1]
    local item=${@:${#}}
    [[ ${list} =~ (^| )${item}($| ) ]] && return 0 || return 1
}

total_num=0
failed_num=0
failed_list=()
passed_num=0
passed_list=()

echo "*-------------- start testing standalone test cases --------------*"
for f in $(find . -name "run.sh" -type f | sort -n | awk -F "/" '{print $2}')
do
    if contain "${IGNORE_LIST[@]}" ${f};then
        echo "ignore ${f} case"
        continue
    else
        cd $f
        if [[ $2 == "--sgx" ]]; then
           ./run.sh $1 --sgx ${TARGET}
        else
           ./run.sh $1 --no-sgx ${TARGET}
        fi

        retval=$?
        if [ ${retval} -ne 0 ] && [ ${retval} -ne 1 ]; then
            echo ""
            echo "run $f test failed, 'run.sh' returns ${retval}"
            echo ""
            failed_num=$((failed_num + 1))
            failed_list+=("$f")
        else
            echo ""
            echo "run $f test successfully, 'run.sh' returns ${retval}"
            echo ""
            passed_num=$((passed_num + 1))
            passed_list+=("$f")
        fi
        cd ..
    fi
done
total_num=$((failed_num+passed_num))
echo "*--------------    standalone test cases finish    --------------*"
echo ""
echo ""

report_file="./standalone_test_report.txt"
# Delete the last report file
if [ -f "${report_file}" ]; then
    rm "${report_file}"
fi

echo  "*================   Standalone Test Report Start   ==============*" | tee -a ${report_file}
echo  "" | tee -a ${report_file}
echo  "Total: ${total_num}" | tee -a ${report_file}
echo  "Passed: ${passed_num}" | tee -a ${report_file}
echo  "Failed: ${failed_num}" | tee -a ${report_file}


if [ ${passed_num} -gt 0 ]; then
    echo  "" >> ${report_file}
    echo  "******************************************************************" >> ${report_file}
    echo  "Passed cases list:" >> ${report_file}
    echo  "" >> ${report_file}
    for passed_case in "${passed_list[@]}"; do
        echo  "  $passed_case" >> ${report_file}
    done
fi

if [ ${failed_num} -gt 0 ]; then
    echo  "" | tee -a ${report_file}
    echo  "******************************************************************" | tee -a ${report_file}
    echo  "Failed cases list:" | tee -a ${report_file}
    for failed_case in "${failed_list[@]}"; do
        echo  "  $failed_case" | tee -a ${report_file}
    done
fi

echo  "" | tee -a ${report_file}
echo  "*================   Standalone Test Report End   ==============*" | tee -a ${report_file}
