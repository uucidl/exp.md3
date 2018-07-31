set -e
[[ -d "output" ]] || mkdir -p "output"
cc -o output/ion deps/bitwise/ion/main.c
printf "PROGRAM\t%s\n" "$(pwd)"/output/ion

