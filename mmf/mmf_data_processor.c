#include "lib_mmf.h"
#include <lvds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
Project: Roberto
Author: AncientBlueFrog
Version: 0.1
*/

enum file_type;
typedef struct c_file_buffer cf_buffer;
char *search_deps(stack *, char *);
cf_buffer *c_file_interpreter(char *, project *, mcp *);
void cf_buffer_clear(cf_buffer *b);
int accumulator(cf_buffer *, project *, mcp *);

struct c_file_buffer
{
    char *name;
    stack *function_declarations;
    stack *function_references;
    stack *function_calls;
    stack *headers;
    stack *external_files;
    stack *files;
};

enum text_mode
{
    code,
    double_slash,
    slash_asterisk,
    double_quote,
    less_greater,
};

enum block_mode
{
    no_block,
    hashtag,
    parenthesis,
};

enum operation
{
    no_op,
    inclusion,
    function,
};

int makefile_generator(project *p, mcp *profile)
{
    // Opens file.
    FILE *makefile = fopen(p->makefile, "w+");
    stack *execs = p->exec_stack;
    stack *fb_stack = stack_init(10, 5);
    cf_buffer *file_buffer;

    if (!makefile)
    {
        printf("Could not open makefile.");
        return 0;
    }

    // All
    fprintf(makefile, "all: ");
    for (int i = 0; i < execs->stack_index; i++)
    {
        file_buffer = c_file_interpreter((char *)stack_get(execs, i), p, profile);
        accumulator(file_buffer, p, profile);

        fprintf(makefile, "%s ", file_buffer->name);
        stack_add(fb_stack, file_buffer);
    }

    fprintf(makefile, "\n");

    // Projecs
    for (int i = 0; i < fb_stack->stack_index; i++)
    {
        file_buffer = stack_get(fb_stack, i);
        fprintf(makefile, "%s", file_buffer->name);
        fprintf(makefile, ": ");

        // File name.
        fprintf(makefile, "%s.c ", file_buffer->name);

        // Internal headers.
        stack *stack_i_headers = file_buffer->headers;
        for (int j = 0; j < stack_i_headers->stack_index; j++)
        {
            fprintf(makefile, "%s ", (char *)stack_get(stack_i_headers, j));
        }

        fprintf(makefile, "\n\t");

        // compiler.
        fprintf(makefile, "%s ", profile->compiler);

        // Options.
        stack *stack_options = profile->options;

        for (int j = 0; j < stack_options->stack_index; j++)
        {
            fprintf(makefile, "-%s ", (char *)stack_get(stack_options, j));
        }

        // Files.
        stack *stack_files = file_buffer->files;

        for (int j = 0; j < stack_files->stack_index; j++)
        {
            fprintf(makefile, "%s ", (char *)stack_get(stack_files, j));
        }

        // File name
        fprintf(makefile, "-o %s ", file_buffer->name);

        // External headers.
        stack *stack_e_headers = file_buffer->external_files;

        for (int j = 0; j < stack_e_headers->stack_index; j++)
        {
            fprintf(makefile, "-l%s ", (char *)stack_get(stack_e_headers, j));
        }

        fprintf(makefile, "\n");

        cf_buffer_clear(file_buffer);
    }

    // closing.
    fclose(makefile);
    close_project(p);

    return 1;
}

// search_deps
char *search_deps(stack *haystack, char *dep)
{
    // stack(libraries) > config_lib > char *
    char *current_str;
    for (int i = 0; i < haystack->stack_index; i++)
    {
        config_lib *cl_carrier = stack_get(haystack, i);
        current_str = cl_carrier->header;
        if (!strcmp(dep, current_str))
        {
            if (cl_carrier->implement)
            {
                return cl_carrier->implement;
            }
            else
            {
                int idenl = strlen(cl_carrier->header);
                if (current_str[idenl - 2] == '.')
                    current_str[idenl - 2] = 0;
                return current_str;
            }
        }
    }
    return NULL;
}

cf_buffer *c_file_interpreter(char *file_name, project *p, mcp *profile)
{
    printf("%s:\n", file_name);
    // Get file name path to open file.
    int fnl = strlen(file_name);
    char *file_name_path = malloc(fnl + p->path_length + 1);

    file_name_path = strcpy(file_name_path, p->path);
    file_name_path = strcat(file_name_path, file_name);
    FILE *file = fopen(file_name_path, "r");
    free(file_name_path);

    char *filename = malloc(fnl);
    filename = strcpy(filename, file_name);

    // Get filename formated.
    char *filenamef = malloc(fnl);
    filenamef = strcpy(filenamef, file_name);
    filenamef[fnl - 2] = 0;

    // Function variables.
    char identifier[32] = {};
    int i = 0, elementc = 0;
    char cb = 0, pb = 0, interrupt = 1, infunc = 0;
    enum block_mode next_blockm = no_block, blockm = no_block;
    enum text_mode next_textm = code, textm = code;
    enum operation op = no_op;

    // initializing buffer.
    cf_buffer *buffer_ret = malloc(sizeof(cf_buffer));
    buffer_ret->name = filenamef;
    buffer_ret->files = stack_init(10, 5);
    buffer_ret->function_references = stack_init(10, 5);
    buffer_ret->function_declarations = stack_init(10, 5);
    buffer_ret->function_calls = stack_init(10, 5);
    buffer_ret->external_files = stack_init(10, 5);
    buffer_ret->headers = stack_init(10, 5);
    stack_add(buffer_ret->files, filename);

    while (cb != EOF)
    {
        interrupt = 1;
        pb = cb;
        blockm = next_blockm;
        textm = next_textm;
        cb = fgetc(file);

        // Switch for start modes.
        switch (cb)
        {
        case 32:
            if (next_textm == double_quote)
            {
                interrupt = 0;
            }
            break;
        case 9:
            break;
        case 10:
            if (next_textm == double_slash)
                next_textm = code;
            else if (next_blockm == hashtag)
            {
                next_blockm = no_block;
                op = no_op;
                elementc = 0;
            }
            break;
        case '(':
            if (next_blockm == no_block)
                next_blockm = parenthesis;

            break;
        case '#':
            if (next_blockm == no_block)
                next_blockm = hashtag;

            break;
        case ';':
            if (op == function)
                infunc++;

            elementc = 0;
            break;
        case '=':
            if (op == function)
                infunc++;

            elementc = 0;
            break;
        case '}':
            infunc = 0;
            break;
        case '/':
            if (identifier[0] == '/')
            {
                if (next_textm == code)
                    next_textm = double_slash;
            }
        case '*':
            if (next_textm == slash_asterisk)
            {
                identifier[i] = cb;
                i++;
                cb = fgetc(file);
                if (cb == '/')
                    next_textm = code;
            }
            if (identifier[0] == '/')
            {
                if (next_textm == code)
                    next_textm = slash_asterisk;
            }
        case '"':
            if (next_textm == double_quote)
            {
                next_textm = code;
            }
            else if (next_textm == code)
            {
                next_textm = double_quote;
            }
            break;
        case '<':

            if (next_textm == code)
                next_textm = less_greater;
            break;
        case ')':
            if (next_blockm == parenthesis)
                next_blockm = no_block;

            break;
        case '>':
            if (next_textm == less_greater)
                next_textm = code;

            break;
        case '{':
            if (op == function)
                infunc += 2;

        default:
            interrupt = 0;
            break;
        }

        // Continue to while if it is a comment.
        if ((next_textm == slash_asterisk) || (next_textm == double_slash) || (blockm == parenthesis))
        {
            continue;
        }

        if (interrupt || (pb == 32) || (pb == 10))
        {
            if (i > 0)
            {
                identifier[i] = 0;

                switch ((int)op)
                {
                case inclusion:
                    if ((textm == double_quote) && cb == '"')
                    {
                        if ((!stack_lstr_search(buffer_ret->headers, identifier)) &&
                            (stack_lstr_search(p->headers, identifier)))
                        {
                            if (!stack_lstr_search(buffer_ret->headers, identifier))
                            {
                                printf("\tHEADERS: %s from %s\n", identifier, file_name);

                                printf("goto %s\n", identifier);

                                cf_buffer *buffer_carrier = c_file_interpreter(identifier, p, profile);

                                printf("%s:\n", file_name);

                                stack_append(buffer_ret->headers, buffer_carrier->headers);
                                stack_append(buffer_ret->external_files, buffer_carrier->external_files);
                                stack_append(buffer_ret->function_declarations, buffer_carrier->function_declarations);
                                stack_append(buffer_ret->function_references, buffer_carrier->function_references);

                                stack_add(buffer_ret->headers, strcopy(identifier));

                                cf_buffer_clear(buffer_carrier);
                            }
                        }
                        op = no_op;
                    }
                    else if ((textm == less_greater) && cb == '>')
                    {
                        if (!stack_lstr_search(buffer_ret->headers, identifier))
                        {
                            char *ef_carrier = search_deps(profile->libraries, identifier);
                            if (ef_carrier)
                            {
                                printf("\tTO LINK: %s from %s\n", identifier, file_name);
                                stack_add(buffer_ret->external_files, strcopy(ef_carrier));
                            }
                        }
                        op = no_op;
                    }
                    break;
                case function:
                    // infunc 1 and infunc = 4 are invalid.
                    // ';' = 1, '(' = 1, '{' = 2
                    if (infunc == 2) // 1 * '(' + 1 * ';'.
                    {
                        if (!stack_lstr_search(buffer_ret->function_declarations, identifier))
                        {
                            printf("\tFUNCTION DECLARATIONS: %s from %s\n", identifier, buffer_ret->name);
                            stack_add(buffer_ret->function_declarations, strcopy(identifier));
                        }
                        infunc = 0;
                        op = no_op;
                    }
                    else if (infunc == 3) // 1 * '(' + 1 * '{'.
                    {
                        // If indentifier != main and identifier != function_references_stack.
                        if (strcmp(identifier, "main"))
                        {
                            if (!stack_lstr_search(buffer_ret->function_references, identifier))
                            {
                                printf("\tFUNCTION REFERENCES: %s from %s\n", identifier, buffer_ret->name);
                                stack_add(buffer_ret->function_references, strcopy(identifier));
                            }
                        }
                        op = no_op;
                    }
                    else if (infunc == 5) // 1 * '(' + 1 * '{' + 1 * '('+ 1 * ';'.
                    {
                        if (!stack_lstr_search(buffer_ret->function_calls, identifier))
                        {
                            printf("\tFUNCTION CALLS: %s from %s\n", identifier, buffer_ret->name);
                            stack_add(buffer_ret->function_calls, strcopy(identifier));
                        }

                        infunc = 3;
                        op = no_op;
                    }
                    break;
                }
            }
            // Define the mode of next identifier.
            if ((blockm == hashtag) && !strcmp(identifier, "include"))
            {
                op = inclusion;
            }
            else if ((cb == '(') && (elementc >= 1))
            {
                infunc++;
                op = function;
            }
            // clear identifier.
            if ((cb != 32) && (cb != 10) && !((infunc == 1) || (infunc == 4)))
            {
                for (int j = i; j >= 0; j--)
                {
                    identifier[j] = 0;
                }
                i = 0;
                elementc++;
            }

            if (!interrupt)
            {
                identifier[i] = cb;
                i++;
            }
        }
        else if (blockm != parenthesis)
        {
            if (i <= 30)
            {
                identifier[i] = cb;
                i++;
            }
        }
    }

    printf("return\n");
    return buffer_ret;
}

int accumulator(cf_buffer *buffer_ret, project *p, mcp *profile)
{
    // Recursivelly adding files.
    // Lists lib files.
    stack *lib_name_stack = p->lib_stack;
    cf_buffer *lib_buffer;
    stack *lib_buffer_stack = stack_init(10, 5);
    char *lib_carrier;
    stack *stack_function = buffer_ret->function_calls;

    for (int i = 0; i < lib_name_stack->stack_index; i++)
    {
        lib_carrier = (char *)stack_get(lib_name_stack, i);

        // Get the buffer of lib.
        lib_buffer = c_file_interpreter(lib_carrier, p, profile);
        stack_add(lib_buffer_stack, lib_buffer);
    }

    for (int i = 0; i < stack_function->stack_index; i++)
    {
        for (int j = 0; j < lib_buffer_stack->stack_index; j++)
        {
            lib_buffer = stack_get(lib_buffer_stack, j);
            if (stack_lstr_search(lib_buffer->function_references, stack_get(stack_function, i)))
            {
                stack_append(buffer_ret->headers, lib_buffer->headers);
                stack_append(buffer_ret->external_files, lib_buffer->external_files);
                stack_append(buffer_ret->function_declarations, lib_buffer->function_declarations);
                stack_append(buffer_ret->function_references, lib_buffer->function_references);
                stack_append(buffer_ret->files, lib_buffer->files);
                break;
            }
            else
            {
                // cf_buffer_clear(lib_buffer);
            }
        }
    }

    for (int i = 0; i < lib_buffer_stack->stack_index; i++)
    {
        cf_buffer_clear(stack_get(lib_buffer_stack, i));
    }

    stack_close(lib_buffer_stack);
    return 1;
}
void cf_buffer_clear(cf_buffer *b)
{
    stack_close(b->headers);
    stack_close(b->external_files);
    stack_close(b->files);
    stack_close(b->function_calls);
    stack_close(b->function_declarations);
    stack_close(b->function_references);
    free(b->name);
}

// Soli Deo Gloria.
