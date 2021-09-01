#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "process.h"
#include "global.h"

PUBLIC int sys_get_ticks(){
    return ticks;
}