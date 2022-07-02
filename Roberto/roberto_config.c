#include "lib_mmf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum config_mode
{
    c_null,
    c_compiler,
    c_flags,
    c_libraries,
    c_comment,
};

int mmf_config_loader(mcp *profile, FILE *config_input);
mcp *search_profile(mcp *, char *);

int mmf_config_loader(mcp *profile, FILE *config_input)
{
    stack *profile_stack = profile->parent;

    enum config_mode cmode = c_null;

    char identifier[32] = {};
    int i = 0;
    char cb = 0;
    int alias = 0;
    config_lib *cl_carrier;

    do
    {
        cb = fgetc(config_input);

        if (cmode == c_comment)
        {
            if (cb == 10)
            {
                cmode = c_null;
            }
            else
            {
                continue;
            }
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
                profile = mmf_profile_init(identifier, profile_stack);
            }
            continue;
        case '<':
            cmode = c_libraries;
            continue;
        case '-':
            cmode = c_flags;
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

                if ((cmode) && (cb != ';') && (cmode != c_comment))
                {
                    address_carrier = strdup(identifier);
                }

                identifier[i] = 0;

                // Execute the due methots for each mode.
                switch ((int)cmode)
                {
                case c_compiler:
                    profile->compiler = address_carrier;
                    break;
                case c_flags:
                    stack_add(profile->flags, address_carrier);
                    break;
                case c_libraries:
                    if (alias)
                    {
                        cl_carrier->implement = address_carrier;
                        alias = 0;
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

mcp *mmf_profile_init(char *profile_name, stack *profile_parent)
{
    mcp *config_profile = malloc(sizeof(mcp));

    if (!config_profile)
    {
        return NULL;
    }

    if (profile_parent->index == 0)
    {
        config_profile->libraries = stack_init(10, 5);
    }
    else
    {
        mcp *mcp_carrier = stack_get(profile_parent, 0);
        config_profile->libraries = mcp_carrier->libraries;
    }
    config_profile->flags = stack_init(10, 5);
    config_profile->parent = profile_parent;
    config_profile->name = strdup(profile_name);
    config_profile->compiler = NULL;

    stack_add(profile_parent, config_profile);
    return config_profile;
}

mcp *create_config_buffer(char *profile_name)
{
    stack *profile_parent = stack_init(10, 5);

    if (!profile_parent)
    {
        return NULL;
    }

    return mmf_profile_init(profile_name, profile_parent);
}

mcp *search_profile(mcp *profile, char *profile_name)
{
    mcp *profile_ret;

    stack *profile_stack = profile->parent;

    for (int i = 0; i < profile_stack->index; i++)
    {
        profile_ret = stack_get(profile_stack, i);

        if (!strcmp(profile_ret->name, profile_name))
        {
            return profile_ret;
        }
    }
    return NULL;
}

void close_profile(mcp *p)
{ // Cleaning profiles.
    stack *profile_stack = p->parent;
    stack *libraries_stack = p->libraries;

    mcp *profile_carrier;
    for (int i = 0; i < profile_stack->index; i++)
    {
        profile_carrier = stack_get(profile_stack, i);

        free(profile_carrier->name);
        if (profile_carrier->compiler)
            free(profile_carrier->compiler);
        stack_close(profile_carrier->flags);
    }

    config_lib *library_carrier;
    for (int i = 0; i < libraries_stack->index; i++)
    {
        library_carrier = stack_get(libraries_stack, i);

        free(library_carrier->header);
        if (library_carrier->implement)
            free(library_carrier->implement);
    }

    stack_close(p->libraries);
    stack_close(profile_stack);
}
// Soli Deo Gloria.
