set -e
export IONHOME=$(pwd)/deps/bitwise/ion
output/ion -os osx -arch x64 -o output/docking_demo_program.c \
  docking_demo_program
cc -o output/docking_demo.elf output/docking_demo_program.c \
  -F./deps/SDL2_osx -framework SDL2 \
  -I./deps/SDL2_osx/SDL2.framework/Headers/ 
printf "PROGRAM\t%s\n" "$(pwd)"/output/docking_demo.elf
