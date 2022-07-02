#include "lib_mmf.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Project: Roberto
Author: AncientBlueFrog
Version: 1.1
*/

int add_project(stack *, char *);
int analyze_file(void);
int arg_switch(int, char **, stack *);
void help(void);
int initial_setup(void);
#define BUFFERSIZE 45

mcp *config_obj;
char *profile_name;
int err_code;
stack *pilha;
char *version;
int main(int argc, char *argv[])
{
    version = "1.1";
    profile_name = NULL;
    pilha = stack_init(15, 5);

    err_code = arg_switch(argc, argv, pilha);

    if (!err_code)
    {
        return 1;
    }
    else if (err_code == 3)
    {
        return 0;
    }

    err_code = initial_setup();
    if (!err_code)
    {
        return 1;
    }

    // iterate for creating makefiles.
    for (int i = 0; i < pilha->index; i++)
    {
        err_code = makefile_generator(stack_get(pilha, i), config_obj);

        if (!err_code)
        {
            return 0;
        }
    }

    close_profile(config_obj);

    stack_destroy(pilha);
}

int arg_switch(int argc, char **argv, stack *pilha)
{
    // Leia os argumentos passados pelo terminal.
    // Itere de acordo com o número de argumentos
    char switch_symbol = 'f';
    for (int i = 1; i < argc; i++)
    {
        // Se o inicio do argumento for -, leia os argumentos abreviados.
        if (argv[i][0] == '-')
        {
            // Caso o segundo argumento for -, leia os argumentos por extenso.
            if (argv[i][1] == '-')
            {
                // statement
            }
            else
            {
                for (int j = 1; argv[i][j]; j++)
                {
                    switch (argv[i][j])
                    {
                    case 'h':
                        help();
                        return 3;

                    case 'p':
                        switch_symbol = 'p';
                        break;
                    }
                }
            }
        }
        else
        {
            switch (switch_symbol)
            {
            case 'f':
                err_code = add_project(pilha, argv[i]);

                if (!err_code)
                {
                    return 0;
                }
                break;
            case 'p':
                profile_name = argv[i];
                break;
            }

            switch_symbol = 'f';
        }
    }

    return 1;
}

// Add the projec to stack pilha.
int add_project(stack *project_stack, char *file_name)
{
    int filel = 0;
    int last_slash = 0;

    while (file_name[filel])
    {
        filel++;
        // Save the index of last slash.
        if (file_name[filel] == '/')
            last_slash = filel;
    }

    // If it is a .c file, then...
    if ((file_name[filel - 2] == '.') && (file_name[filel - 1] == 'c'))
    {
        // name_length = i + 1;
        // if last slash > 0, null value is placed after last slash.

        if (last_slash > 0)
        {
            file_name[last_slash + 1] = 0;
        }
    }

    for (int i = 0; i < project_stack->index; i++)
    {
        // Puxa um projeto da pilha.
        project *aproject = stack_get(project_stack, i);

        if (!strcmp(file_name, aproject->path))
        {
            return 0;
        }
    }

    stack_add(project_stack, create_project(file_name));

    return 1;
}

void help()
{
    printf("ROBERTO_C - Makefile generator\n");
    printf("Version: %s\n\n", version);
    printf("See your configurations in ~/.robertoconfig\n\n");
    printf("COMMANDS:\n\n-h: Help screen\n-p: Profile\n\n--language: Learn to "
           "configure\n");
}

int initial_setup()
{
    if (!profile_name)
    {
        profile_name = "Default";
    }

    if (!pilha->index)
    {
        stack_add(pilha, create_project("."));
    }

    // Abre o stream.
    char config_path[BUFFERSIZE];
    snprintf(config_path, BUFFERSIZE, "%s/%s", getenv("HOME"), ".robertoconfig");
    FILE *config_file = fopen(config_path, "r+");
    config_obj = create_config_buffer(profile_name);

    // Se o arquivo de configuração não foi criado, criar o arquivo de
    // configuração.
    if (!config_file)
    {
        config_file = fopen(config_path, "w+");
        fprintf(config_file, "Default:\n\t>clang\n;\n<\n\tmath.h | m\n>");
        printf("See your configurations in ~/.robertoconfig\n");
    }

    mmf_config_loader(config_obj, config_file);
    if (!config_obj->compiler)
    {
        printf("There's no such profile in .mmfconfig\n");
        return 0;
    }

    fclose(config_file);
    return 1;
}

// Soli Deo Gloria.
