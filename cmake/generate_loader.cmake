# Reads generated.h and produces generated_loader.h with a function
# that loads all bytecode modules in dependency order.
#
# Usage: cmake -DGENERATED_H=<path> -DOUTPUT=<path> -P generate_loader.cmake

file(READ "${GENERATED_H}" content)

# Find all "const uint32_t qjsc_XXX_size = NNN;" declarations (in order)
string(REGEX MATCHALL "const uint32_t (qjsc_[a-zA-Z0-9_]+)_size" matches "${content}")

set(modules "")
foreach(match IN LISTS matches)
    string(REGEX REPLACE "const uint32_t (qjsc_[a-zA-Z0-9_]+)_size" "\\1" name "${match}")
    list(APPEND modules "${name}")
endforeach()

# The last module is the entry point (load_only=0), all others are dependencies (load_only=1)
list(LENGTH modules count)
math(EXPR last "${count} - 1")

set(out "/* Auto-generated loader - do not edit */\n")
string(APPEND out "#pragma once\n")
string(APPEND out "#include \"generated.h\"\n\n")
string(APPEND out "static inline void qjsc_load_modules(JSContext* ctx) {\n")

set(idx 0)
foreach(mod IN LISTS modules)
    if(idx EQUAL last)
        string(APPEND out "    js_std_eval_binary(ctx, ${mod}, ${mod}_size, 0);\n")
    else()
        string(APPEND out "    js_std_eval_binary(ctx, ${mod}, ${mod}_size, 1);\n")
    endif()
    math(EXPR idx "${idx} + 1")
endforeach()

string(APPEND out "}\n")

file(WRITE "${OUTPUT}" "${out}")
