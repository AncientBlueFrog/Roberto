#include "lib_mmf.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int define_strings(project *p, char *);
int define_files(project *);
int is_exec(FILE *);

struct Project *create_project(char *path)
{
    struct Project *p = malloc(sizeof(struct Project));

    if (!p)
    {
        printf("Falha ao carregar projeto.");
        return NULL;
    }

    define_strings(p, path);
    define_files(p);

    return p;
}

int define_strings(project *p, char *path)
{

    int arrSize = strlen(path);

    if (path[arrSize - 1] != '/')
        arrSize++;
    // Alocating memory.
    p->path_length = arrSize + 1;
    char *filename = malloc(p->path_length + 8);
    p->path = malloc(p->path_length);

    // Copy the string path to project path and filename.
    p->path = strcpy(p->path, path);

    p->path[arrSize - 1] = '/';

    filename = strcpy(filename, p->path);

    // Append "/Makefile" to filename in order to create a file path.
    filename = strcat(filename, "Makefile");

    p->dir = opendir(p->path);

    p->makefile = filename;

    if (!p->dir || !p->makefile || !p->path)
    {
        printf("Falha ao carregar metadados.");
        return 0;
    }
    return 1;
}

// Start the streams for code files.
int define_files(project *p)
{
    // Initialize the stack.
    p->lib_stack = stack_init(10, 5);
    p->exec_stack = stack_init(5, 1);
    p->headers = stack_init(5, 5);

    struct dirent *dc;

    if (!p->dir)
    {
        return 0;
    }

    while ((dc = readdir(p->dir)) != NULL)
    {

        // count the length of file names.
        int name_length = strlen(dc->d_name);
        char *name_string = malloc(name_length + 1);
        name_string = strcpy(name_string, dc->d_name);
        // Add the file to project.code_files.
        if ((dc->d_name[name_length - 2] == '.') && (dc->d_name[name_length - 1] == 'c'))
        {
            // Get the complete path of the file.
            char *file_path = malloc(p->path_length + name_length);

            file_path = strcpy(file_path, p->path);

            file_path = strcat(file_path, dc->d_name);

            // Tries to open the code file.
            FILE *f = fopen(file_path, "r");
            free(file_path);
            // Returns error;
            if (!f)
            {
                printf("Falha ao abrir arquivo");
                free(name_string);
                return 0;
            }

            if (is_exec(f))
            {
                stack_add(p->exec_stack, name_string);
            }
            else
            {
                stack_add(p->lib_stack, name_string);
            }
        }
        else if ((dc->d_name[name_length - 2] == '.') && (dc->d_name[name_length - 1] == 'o'))
        {
            stack_add(p->lib_stack, name_string);
        }
        else if ((dc->d_name[name_length - 2] == '.') && (dc->d_name[name_length - 1] == 'h'))
        {
            stack_add(p->headers, name_string);
        }
        else
        {
            free(name_string);
        }
    }

    return 1;
}

void close_project(project *p)
{
    // Close Makefile.
    free(p->makefile);

    // Close dir.
    closedir(p->dir);

    // Close path.
    free(p->path);

    // Close project.
    free(p);

    // Close stack of execs.
    stack_close(p->exec_stack);

    // Close stack of libs.
    stack_close(p->lib_stack);

    // Close stack of headers.
    stack_close(p->headers);
}

int is_exec(FILE *f)
{
    char identifier[64] = {};
    int i = 0;
    char b = 0;
    char comment = 0;
    char block_comment = 0;

    while (b != EOF)
    {
        b = getc(f);

        // Verify whether the comment ends.
        if (b == 10)
        {
            comment = 0;
        }

        if ((b == '*' || block_comment == 1) && block_comment)
        {
            block_comment = 1;
            if (b == '/')
            {
                block_comment = 0;
            }
            else if (b != '*')
            {
                block_comment = 2;
            }
        }

        // Continue to while if it is a comment.
        if (comment || block_comment)
        {
            continue;
        }

        if ((b == 32) || (b == 10) || (b == 9) || (b == '(') || (b == ')') || (b == '{') || (b == '}'))
        {
            if (i > 0)
            {
                identifier[i] = 0;

                // compare identifier with "main".
                if (!strcmp(identifier, "main"))
                {
                    return 1;
                }

                // clear identifier.
                for (int j = 0; j <= i; j++)
                {
                    identifier[j] = 0;
                }

                // clear i and return.
                i = 0;
            }

            continue;
        }
        if (i < 63)
        {
            identifier[i] = b;
            i++;
        }

        if (!strcmp(identifier, "//"))
        {
            comment = 1;
        }
        else if (!strcmp(identifier, "/*"))
        {
            block_comment = 2;
        }
    }
    fclose(f);

    return 0;
}
// Soli Deo Gloria.
