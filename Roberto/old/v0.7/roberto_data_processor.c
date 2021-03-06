#include "lib_mmf.h"
#include <lvds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum file_type;
typedef struct c_file_buffer cf_buffer;
char *search_deps(stack *, char *);
cf_buffer *c_file_interpreter(char *, project *, mcp *, stack *);
void cf_buffer_clear(cf_buffer *b);

struct c_file_buffer
{
    char *name;
    stack *function_declarations;
    stack *function_references;
    stack *function_calls;
    stack *headers;
    stack *external_files;
    stack *files;
    stack *history;
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
        file_buffer = c_file_interpreter((char *)stack_get(execs, i), p, profile, NULL);

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
            if (!stack_lstr_search(stack_i_headers, (char *)stack_get(stack_files, j)))
            {
                // printf("%s,  ", (char *)stack_get(stack_files, j));
                fprintf(makefile, "%s ", (char *)stack_get(stack_files, j));
            }
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

// search for deps
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
                char *str_carrier = malloc(idenl);
                str_carrier = strcpy(str_carrier, current_str);

                if (str_carrier[idenl - 2] == '.')
                    str_carrier[idenl - 2] = 0;
                return str_carrier;
            }
        }
    }
    return NULL;
}

cf_buffer *c_file_interpreter(char *file_name, project *p, mcp *profile, stack *history)
{
    // printf("%s:\n", file_name);
    //   Get file name path to open file.
    int fnl = strlen(file_name);
    char *file_name_path = malloc(fnl + p->path_length + 1);
    file_name_path = strcpy(file_name_path, p->path);
    file_name_path = strcat(file_name_path, file_name);
    FILE *file = fopen(file_name_path, "r");
    free(file_name_path);

    if (!file)
    {
        return NULL;
    }

    // Get filename formated.
    char *filenamef = strcopy(file_name);
    filenamef[fnl - 2] = 0;

    // Function variables.
    char identifier[32] = {};
    int i = 0, cb = 0, pb = 0, pcount = 0;
    char interrupt = 1, infunc = 0;
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
    buffer_ret->history = stack_init(10, 5);
    stack_add(buffer_ret->files, strcopy(file_name));

    if (history)
    {
        stack_str_copy(buffer_ret->history, history);
    }

    stack_add(buffer_ret->history, strcopy(file_name));

    while (cb != EOF)
    {
        interrupt = 1;
        pb = cb;
        blockm = next_blockm;
        textm = next_textm;
        cb = fgetc(file);
        if (cb == EOF)
        {
            // printf("EOF\n");
        }
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
            }
            break;
        case '(':
            if (next_blockm == no_block)
                next_blockm = parenthesis;

            if (next_blockm == parenthesis)
                pcount++;
            break;
        case '#':
            if (next_blockm == no_block)
                next_blockm = hashtag;

            break;
        case ';':
            if (op == function)
                infunc++;

            break;
        case '=':
            if (op == function)
                infunc++;

            break;
        case '}':
            infunc = 0;
            break;
        case '/':
            cb = fgetc(file);
            if (cb == '/')
            {
                if (next_textm == code)
                    next_textm = double_slash;
            }
            else if (cb == '*')
            {
                if (next_textm == code)
                    next_textm = slash_asterisk;
            }
            break;
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
            break;
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
            {
                pcount--;
                if (pcount == 0)
                {
                    next_blockm = no_block;
                }
            }

            break;
        case '>':
            if (next_textm == less_greater)
                next_textm = code;

            break;
        case '{':
            if (op == function)
                infunc += 2;
            break;
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
                if ((strcmp(identifier, "if")) && (strcmp(identifier, "do")) && (strcmp(identifier, "else")) &&
                    (strcmp(identifier, "for")) && (strcmp(identifier, "while")))
                {

                    switch ((int)op)
                    {
                    case inclusion:
                        if ((textm == double_quote) && cb == '"')
                        {
                            if ((!stack_lstr_search(buffer_ret->headers, identifier)) &&
                                (!stack_lstr_search(buffer_ret->headers, identifier)) &&
                                (!stack_lstr_search(buffer_ret->history, identifier)))
                            {
                                // printf("\tHEADERS: %s from %s\n", identifier, file_name);

                                stack_add(buffer_ret->headers, strcopy(identifier));
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
                                    // printf("\tTO LINK: %s from %s\n", identifier, file_name);
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
                                // printf("\tFUNCTION DECLARATIONS: %s from %s\n", identifier, buffer_ret->name);
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
                                    // printf("\tFUNCTION REFERENCES: %s from %s\n", identifier, buffer_ret->name);
                                    stack_add(buffer_ret->function_references, strcopy(identifier));
                                }
                            }
                            op = no_op;
                        }
                        else if ((infunc >= 4) && (strcmp(identifier, "if"))) // 1 * '(' + 1 * '{' + 1 * '('+ 1 * ';'.
                        {
                            if (!stack_lstr_search(buffer_ret->function_calls, identifier))
                            {
                                // printf("\tFUNCTION CALLS: %s from %s\n", identifier, buffer_ret->name);
                                stack_add(buffer_ret->function_calls, strcopy(identifier));
                            }
                            infunc = 3;
                            op = no_op;
                        }
                        break;
                    }
                }
            }
            // Define the mode of next identifier.
            if ((blockm == hashtag) && !strcmp(identifier, "include"))
            {
                op = inclusion;
            }
            else if (cb == '(')
            {
                if (infunc != 1)
                {
                    infunc++;
                    op = function;
                }
            }

            // clear identifier.
            if ((cb != 32) && (cb != 10) && !((infunc == 1) || (infunc == 4)))
            {
                for (int j = i; j >= 0; j--)
                {
                    identifier[j] = 0;
                }
                i = 0;
            }

            if ((!interrupt) && (cb != 32) && (cb != '{') && (cb != '!') && (cb != 39) && (cb != '&') && (cb != '|') &&
                (i <= 30))
            {
                identifier[i] = (char)cb;
                i++;
            }
        }
        else if (blockm != parenthesis)
        {
            if ((i <= 30) && (cb != 32) && (cb != '{') && (cb != '!') && (cb != 39) && (cb != '&') && (cb != '|'))
            {
                identifier[i] = (char)cb;
                i++;
            }
        }
    }

    fclose(file);

    // Recursivelly adding files.
    // Lists lib files.
    stack *lib_name_stack = p->lib_stack;
    cf_buffer *lib_buffer;
    stack *lib_buffer_stack = stack_init(10, 5);
    char *lib_carrier;
    stack *stack_function = buffer_ret->function_calls;

    stack *header_name_stack = buffer_ret->headers;
    char *header_carrier;
    cf_buffer *header_buffer;

    for (int i = 0; i < lib_name_stack->stack_index; i++)
    {
        lib_carrier = (char *)stack_get(lib_name_stack, i);
        if (!strcmp(lib_carrier, file_name))
        {
            continue;
        }

        if (stack_lstr_search(buffer_ret->history, lib_carrier))
        {
            continue;
        }

        /* printf("History: ");
        for (int j = 0; j < buffer_ret->history->stack_index; j++)
        {
            printf("%s, ", (char *)stack_get(buffer_ret->history, j));
        }
        printf("\ngoto %s\n", lib_carrier);
        */
        // Get the buffer of lib.
        lib_buffer = c_file_interpreter(lib_carrier, p, profile, buffer_ret->history);
        if (!lib_buffer)
        {
            continue;
        }
        stack_add(lib_buffer_stack, lib_buffer);
    }

    for (int i = 0; i < header_name_stack->stack_index; i++)
    {
        header_carrier = stack_get(header_name_stack, i);

        if (!strcmp(lib_carrier, file_name))
        {
            continue;
        }

        if (stack_lstr_search(buffer_ret->history, lib_carrier))
        {
            continue;
        }

        // printf("goto %s\n", identifier);
        cf_buffer *header_buffer = c_file_interpreter(header_carrier, p, profile, buffer_ret->history);
        // printf("%s:\n", file_name);

        if (!header_buffer)
        {
            continue;
        }

        stack_str_append(buffer_ret->headers, header_buffer->headers);
        stack_str_append(buffer_ret->external_files, header_buffer->external_files);
        stack_str_append(buffer_ret->function_declarations, header_buffer->function_declarations);
        stack_str_append(buffer_ret->function_references, header_buffer->function_references);
        stack_str_append(buffer_ret->history, header_buffer->history);

        // cf_buffer_clear(header_buffer);
    }

    for (int i = 0; i < stack_function->stack_index; i++)
    {
        for (int j = 0; j < lib_buffer_stack->stack_index; j++)
        {
            lib_buffer = stack_get(lib_buffer_stack, j);
            /*
            if (!strcmp(lib_buffer->name, "roberto_data_processor"))
            {
                printf("%s {\n ", (char *)stack_get(stack_function, i));
                for (int k = 0; k < lib_buffer->function_references->stack_index; k++)
                {
                    printf("%s, ", (char *)stack_get(lib_buffer->function_references, k));
                }
                printf("\n");
            }
*/
            if (stack_lstr_search(lib_buffer->function_references, stack_get(stack_function, i)))
            {
                // printf("To append: %s in %s\n", lib_buffer->name, file_name);
                stack_str_append(buffer_ret->headers, lib_buffer->headers);
                stack_str_append(buffer_ret->external_files, lib_buffer->external_files);
                stack_str_append(buffer_ret->function_declarations, lib_buffer->function_declarations);
                stack_str_append(buffer_ret->function_references, lib_buffer->function_references);
                stack_str_append(buffer_ret->function_calls, lib_buffer->function_calls);
                // stack_str_append(buffer_ret->history, lib_buffer->history);
                stack_str_append(buffer_ret->files, lib_buffer->files);
                break;
            }
            // cf_buffer_clear(lib_buffer);
        }
    }

    for (int i = 0; i < lib_buffer_stack->stack_index; i++)
    {
        lib_buffer = stack_get(lib_buffer_stack, i);

        cf_buffer_clear(lib_buffer);
    }
    stack_close(lib_buffer_stack);

    /* printf("%s{\n", buffer_ret->name);
    for (int i = 0; i < buffer_ret->files->stack_index; i++)
    {
        printf("%s, ", (char *)stack_get(buffer_ret->files, i));
    }
*/
    // printf("return\n");
    return buffer_ret;
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
