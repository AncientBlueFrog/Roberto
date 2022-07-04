#include "obj_stack.h"
#include <lvds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

stack *stack_init(int iniLength, int step)
{
    stack *pilha = malloc(sizeof(stack));

    pilha->length = iniLength;

    pilha->array = malloc(sizeof(void *) * pilha->length);

    pilha->step = step;

    pilha->index = 0;

    return pilha;
}

int stack_add(struct obj_stack *cs, void *obj)
{
    cs->array[cs->index] = obj;

    cs->index++;

    if (cs->index >= cs->length)
    {
        cs->length += cs->step;
        cs->array = realloc(cs->array, cs->length * 8);
    }

    if (cs->array == NULL)
    {
        return 0;
    }
    return 1;
}

void stack_remove(struct obj_stack *cs)
{
    cs->index--;
    free(cs->array[cs->index]);
}

void stack_erase(struct obj_stack *cs)
{
    free(cs->array);
    free(cs);
}

void *stack_get(struct obj_stack *cs, int index)
{
    if (index == STACK_TOP)
    {
        return cs->array[cs->index - 1];
    }

    return cs->array[index];
}

void stack_set(struct obj_stack *cs, void *obj, int index)
{
    if (index == STACK_TOP)
    {
        index = cs->index - 1;
    }

    free(cs->array[index]);
    cs->array[index] = obj;
}

void stack_close(struct obj_stack *cs)
{
    for (int i = 0; i < cs->index; i++)
    {
        free(cs->array[i]);
    }

    free(cs->array);
    free(cs);
}

void stack_clear(struct obj_stack *cs)
{
    while (cs->index)
    {
        stack_remove(cs);
    }
}

int stack_lstr_search(stack *cs, char *value)
{
    char *current_str;
    for (int i = 0; i < cs->index; i++)
    {
        current_str = (char *)stack_get(cs, i);
        if (!strcmp(value, current_str))
            return 1;
    }
    return 0;
}

int stack_str_append(stack *dest, stack *src)
{
    for (int i = 0; i < src->index; i++)
    {
        char *str = (char *)stack_get(src, i);
        if (!stack_lstr_search(dest, str))
        {
            char *strcarrier = strdup(str);

            if (!strcarrier)
                return 0;
            stack_add(dest, strcarrier);
        }
    }
    return 1;
}

int stack_str_copy(stack *dest, stack *src)
{
    for (int i = 0; i < src->index; i++)
    {
        char *str = (char *)stack_get(src, i);
        char *strcarrier = strdup(str);

        if (!strcarrier)
            return 0;
        stack_add(dest, strcarrier);
    }
    return 1;
}

int stack_share(stack *dest, stack *src)
{
    int ret = stack_add(dest, stack_get(src, STACK_TOP));

    return ret;
}

int stack_give(stack *dest, stack *src)
{
    int ret = stack_share(dest, src);
    src->index--;

    return ret;
}
