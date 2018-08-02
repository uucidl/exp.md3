program_path=${1:?please pass program path}
DYLD_FRAMEWORK_PATH=./deps/SDL2_osx "${program_path}"


