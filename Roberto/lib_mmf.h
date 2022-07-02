#include <dirent.h>
#include <obj_stack.h>
#include <stdio.h>

// Macros
enum text_mode
{
    code,
    double_slash,
    slash_asterisk,
    quote,
    double_quote,
    less_greater,
};

enum block_mode
{
    no_block,
    hashtag,
    parenthesis,
};

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
typedef struct makefile_config_profile
{
    char *compiler;
    stack *flags;
    stack *libraries;
    char *name;
    stack *parent;
} mcp;

// project.c
typedef struct Project project;
struct Project *create_project(char *);
void close_project(project *);

// mmf_config
int mmf_config_loader(mcp *, FILE *);
void close_profile(mcp *);
mcp *mmf_profile_init(char *, stack *);
mcp *create_config_buffer(char *);

// mmf_data_processor
int makefile_generator(project *, mcp *);
// Soli Deo Gloria.
