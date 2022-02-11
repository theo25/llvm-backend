#ifndef RUNTIME_MACROS_H
#define RUNTIME_MACROS_H

#define NOT_YOUNG_OBJECT_BIT @NOT_YOUNG_OBJECT_BIT@
#define AGE_MASK @AGE_MASK@
#define FWD_PTR_BIT @FWD_PTR_BIT@
#define VARIABLE_BIT @VARIABLE_BIT@
#define LAYOUT_OFFSET @LAYOUT_OFFSET@
#define AGE_OFFSET @AGE_OFFSET@
#define AGE_WIDTH @AGE_WIDTH@
#define HDR_MASK @HDR_MASK@
#define TAG_MASK @TAG_MASK@LL
#define LENGTH_MASK @LENGTH_MASK@

#define MAP_LAYOUT @MAP_LAYOUT@
#define LIST_LAYOUT @LIST_LAYOUT@
#define SET_LAYOUT @SET_LAYOUT@
#define INT_LAYOUT @INT_LAYOUT@
#define FLOAT_LAYOUT @FLOAT_LAYOUT@
#define STRINGBUFFER_LAYOUT @STRINGBUFFER_LAYOUT@
#define BOOL_LAYOUT @BOOL_LAYOUT@
#define SYMBOL_LAYOUT @SYMBOL_LAYOUT@
#define VARIABLE_LAYOUT @VARIABLE_LAYOUT@
#define SETITER_LAYOUT @SETITER_LAYOUT@
#define MAPITER_LAYOUT @MAPITER_LAYOUT@

#define STRINGIFY(x) #x
#define TOSTRING(X) STRINGIFY(X)
#define INSTALL_PREFIX TOSTRING(@INSTALL_DIR_ABS_PATH@)
#define GDB_SCRIPT_PATH TOSTRING(@GDB_SCRIPT_PATH@)
#define GDB_SCRIPT_NAME TOSTRING(@GDB_SCRIPT_NAME@)

#endif
