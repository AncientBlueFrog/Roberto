#ifndef OBJ_STACK_C
#define OBJ_STACK_C
#define STACK_TOP -1

struct obj_stack
{
    int step;
    void **array;
    int length;
    int index;
};

typedef struct obj_stack stack;
struct obj_stack *stack_init(int iniLength, int step);
int stack_add(struct obj_stack *cs, void *obj);
void stack_remove(struct obj_stack *cs);
void *stack_get(struct obj_stack *cs, int index);
void stack_set(struct obj_stack *cs, void *obj, int index); // Set NULL pointer to remove elemente by index.
void stack_close(struct obj_stack *cs);
void stack_erase(struct obj_stack *cs);
void stack_clear(struct obj_stack *cs);
int stack_lstr_search(stack *cs, char *value);
int stack_str_append(stack *, stack *);
int stack_share(stack *, stack *);
int stack_give(stack *, stack *);
int stack_str_copy(stack *, stack *);

#endif
