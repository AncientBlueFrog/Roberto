all: mkmakefile 
mkmakefile: mkmakefile.c lib_mmf.h 
	clang mkmakefile.c mmf_config.c mmf_data_processor.c project.c -o mkmakefile -llvds -lobj_stack 
