all: mmf

mmf: mkmakefile.c lib_mmf.h
	clang -ggdb -Wall -Wextra -g -Werror -Wformat=0 mkmakefile.c mmf_config.c mmf_data_processor.c project.c obj_stack.c -llvds -o mmf
