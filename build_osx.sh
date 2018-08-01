set -e
[[ -d "output" ]] || mkdir -p "output"
export IONHOME=$(pwd)/deps/bitwise/ion
function build_program_module() {
	Module=$1
	Name=$2
	pushd src
	../output/ion -os osx -arch x64 -o ../output/"${Module}".c \
  	"$Module"
  	popd
	cc -o output/"${Name}".elf output/"${Module}".c \
  		-F./deps/SDL2_osx -framework SDL2 \
  		-I./deps/SDL2_osx/SDL2.framework/Headers/ \
  		-I./deps/nanovg/src \
  		-I./deps/GL3/include \
  		-I./
	printf "PROGRAM\t%s\n" "$(pwd)"/output/"${Name}".elf	
}

build_program_module docking_demo_program docking_demo
build_program_module kaon_scaffold_program kaon_scaffold

 
