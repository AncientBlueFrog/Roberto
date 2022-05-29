#include "lib_mmf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum config_mode
{
    c_null,
    c_compiler,
    c_options,
    c_remove,
    c_libraries,
    c_comment,
};

enum input_mode
{
    finput,
    sinput
};

void mmf_opt_remove(mcp *, char *);
int mmf_config_loader(mcp *profile, void *config_input, enum input_mode i_mode);
mcp *search_profile(mcp *, char *);

int mmf_config_file_loader(mcp *profile, FILE *config_input)
{
    return mmf_config_loader(profile, config_input, finput);
}

int mmf_config_string_loader(mcp *profile, char *config_input)
{
    return mmf_config_loader(profile, config_input, sinput);
}

int mmf_config_loader(mcp *profile, void *config_input, enum input_mode imode)
{
    stack *profile_stack = profile->parent;

    enum config_mode cmode = c_null;

    char identifier[32] = {};
    int i = 0;
    char cb = 0;
    int index = 0, alias = 0;
    config_lib *cl_carrier;

    do
    {
        if (imode == finput)
        {
            cb = fgetc(config_input);
        }
        else
        {
            cb = *((char *)(config_input + index));
            index++;
        }

        switch (cb)
        {
        case 10:
            if (cmode == c_comment)
            {
                cmode = c_null;
                continue;
            }
            break;
        case '>':
            if (cmode == c_libraries)
            {
                cmode = c_null;
            }
            else
            {
                cmode = c_compiler;
            }
            continue;
        case ':':
            cmode = c_null;

            profile = search_profile(profile, identifier);

            if (!profile)
            {
                profile = mmf_profile_init(identifier, i, profile_stack);
            }
            continue;
        case '<':
            cmode = c_libraries;
            continue;
        case '-':
            cmode = c_options;
            continue;
        case '!':
            cmode = c_remove;
            continue;
        case '|':
            alias = 1;
            continue;
        case ';':
            break;
        case '#':
            cmode = c_comment;
            continue;
        }

        if ((cb == 32) || (cb == 10) || (cb == 9) || (cb == ';') || (cb == ',') || (cb == ':') || (cb == '>'))
        {
            if (i > 0)
            {
                char *address_carrier;

                // Execute the remove mode method and avoid allocating memory.
                if (cmode == c_remove)
                {
                    mmf_opt_remove(profile, identifier);
                }
                else if ((cmode) && (cb != ';') && (cmode != c_comment))
                {
                    address_carrier = malloc(i + 1);
                    address_carrier = strcpy(address_carrier, identifier);
                }

                identifier[i] = 0;

                // Execute the due methots for each mode.
                switch ((int)cmode)
                {
                case c_compiler:
                    profile->compiler = address_carrier;
                    break;
                case c_options:
                    stack_add(profile->options, address_carrier);
                    break;
                case c_libraries:
                    if (alias)
                    {
                        cl_carrier->implement = address_carrier;
                    }
                    else
                    {
                        cl_carrier = malloc(sizeof(config_lib));
                        cl_carrier->header = address_carrier;
                        cl_carrier->implement = NULL;

                        stack_add(profile->libraries, cl_carrier);
                    }
                    break;
                }

                if (cmode != c_libraries)
                    cmode = c_null;
            }

            if (cb == ';')
            {
                //
            }

            // Define the mode of next identifier.
            // clear identifier.
            for (int j = i; j >= 0; j--)
            {
                identifier[j] = 0;
            }

            // clear i and return.
            i = 0;
        }
        else
        {
            if (i < 30)
            {
                identifier[i] = cb;
                i++;
            }
        }
    } while ((cb != EOF) && (cb != 0));
    return 1;
}

void mmf_opt_remove(mcp *config_stack, char *toremove)
{
    stack *options_stack = config_stack->options;
    for (int i = 0; i < options_stack->stack_index; i++)
    {
        if (!strcmp((char *)stack_get(options_stack, i), toremove))
        {
            stack_set(options_stack, NULL, i);
        }
    }

    stack *libraries_stack = config_stack->libraries;
    for (int i = 0; i < libraries_stack->stack_index; i++)
    {
        if (!strcmp((char *)stack_get(libraries_stack, i), toremove))
        {
            stack_set(libraries_stack, NULL, i);
        }
    }

    stack *profile_stack = config_stack->parent;
    mcp *profile_buffer;

    for (int i = 0; i < options_stack->stack_index; i++)
    {
        profile_buffer = stack_get(profile_stack, i);

        if (!strcmp(profile_buffer->name, toremove))
        {
            mmf_remove_profile(profile_buffer);

            stack_set(profile_stack, NULL, i);
        }
    }
}

int mmf_config_writer(mcp *pb, FILE *config_file)
{
    stack *profile_stack = pb->parent;
    mcp *profile_buffer;
    stack *compiler_options;
    stack *profile_libs;

    for (int i = 0; i < profile_stack->stack_index; i++)
    {
        profile_buffer = stack_get(profile_stack, i);

        // Name
        fprintf(config_file, "%s:\n    ", profile_buffer->name);

        // Compiler.
        fprintf(config_file, ">%s ", profile_buffer->compiler);

        // Options.
        compiler_options = profile_buffer->options;
        for (int j = 0; j < compiler_options->stack_index; j++)
        {
            fprintf(config_file, "-%s ", (char *)stack_get(compiler_options, j));
        }
    }

    profile_buffer = stack_get(profile_stack, 0);
    profile_libs = profile_buffer->libraries;

    fprintf(config_file, "<\n");
    for (int i = 0; i < profile_libs->stack_index; i++)
    {
        fprintf(config_file, "%s, ", (char *)stack_get(profile_libs, i));
    }

    fprintf(config_file, "\n>");
    // Libs.

    return 1;
}

void mmf_remove_profile(mcp *config_profile)
{
    stack_close(config_profile->options);
    stack_close(config_profile->libraries);
    free(config_profile->compiler);
    free(config_profile->name);

    free(config_profile);
}

mcp *mmf_profile_init(char *profile_name, int profile_name_length, stack *profile_parent)
{
    char *address_carrier;
    mcp *config_profile = malloc(sizeof(mcp));

    if (!config_profile)
    {
        return NULL;
    }

    address_carrier = malloc(profile_name_length + 1);
    address_carrier = strcpy(address_carrier, profile_name);

    if (profile_parent->stack_index == 0)
    {
        config_profile->libraries = stack_init(10, 5);
    }
    else
    {
        mcp *mcp_carrier = stack_get(profile_parent, 0);
        config_profile->libraries = mcp_carrier->libraries;
    }
    config_profile->options = stack_init(10, 5);
    config_profile->parent = profile_parent;
    config_profile->name = profile_name;

    stack_add(profile_parent, config_profile);
    return config_profile;
}

mcp *create_makefile_buffer(char *profile_name)
{
    int profile_name_length = strlen(profile_name);
    stack *profile_parent = stack_init(10, 5);

    if (!profile_parent)
    {
        return NULL;
    }

    return mmf_profile_init(profile_name, profile_name_length, profile_parent);
}

mcp *search_profile(mcp *profile, char *profile_name)
{
    mcp *profile_ret;

    stack *profile_stack = profile->parent;

    for (int i = 0; i < profile_stack->stack_index; i++)
    {
        profile_ret = stack_get(profile_stack, i);

        if (!strcmp(profile_ret->name, profile_name))
        {
            return profile_ret;
        }
    }
    return NULL;
}
// Soli Deo Gloria.
