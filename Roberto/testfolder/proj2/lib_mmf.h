#include <obj_stack.h>
#include <dirent.h>
#include <stdio.h>

typedef struct
{
    char *header;
    char *implement;
} config_lib;

struct Project
{
    char *path;
    int path_length;
    DIR *dir;
    char *makefile;
    stack *exec_stack;
    stack *lib_stack;
    stack *headers;
};
extern int teste;
typedef struct makefile_config_profile
{
    char *compiler;
    stack *options;
    stack *libraries;
    char *name;
    stack *parent;
} mcp;

// project.c
typedef struct Project project;
struct Project *create_project(char *);
void close_project(project *);

// mmf_config
int mmf_config_file_loader(mcp *, FILE *);
int mmf_config_string_loader(mcp *, char *);
void mmf_remove_profile(mcp *);
mcp *mmf_profile_init(char *, int, stack *);
mcp *create_makefile_buffer(char *);

// mmf_data_processor
int mmf_config_writer(mcp *, FILE *);
int makefile_generator(project *, mcp *);
// Soli Deo Gloria.
