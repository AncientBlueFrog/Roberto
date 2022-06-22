clang -fPIC -o obj_stack.o -c obj_stack.c
clang -shared -fPIC -o libobj_stack.so obj_stack.o
ar -rcs libobj_stack.a obj_stack.o
cp libobj_stack.a ~/lib/
cp obj_stack.h ~/include/
