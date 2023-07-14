#include "pocketpy/repl.h"

namespace pkpy {
    REPL::REPL(VM* vm) : vm(vm){
        vm->_stdout(vm, "pocketpy " PK_VERSION " (" __DATE__ ", " __TIME__ ") ");
        vm->_stdout(vm, fmt("[", sizeof(void*)*8, " bit] on ", PK_SYS_PLATFORM "\n"));
        vm->_stdout(vm, "https://github.com/blueloveTH/pocketpy" "\n");
        vm->_stdout(vm, "Type \"exit()\" to exit." "\n");
    }

    bool REPL::input(std::string line){
        CompileMode mode = REPL_MODE;
        if(need_more_lines){
            buffer += line;
            buffer += '\n';
            int n = buffer.size();
            if(n>=need_more_lines){
                for(int i=buffer.size()-need_more_lines; i<buffer.size(); i++){
                    // no enough lines
                    if(buffer[i] != '\n') return true;
                }
                need_more_lines = 0;
                line = buffer;
                buffer.clear();
                mode = CELL_MODE;
            }else{
                return true;
            }
        }
        
        try{
            vm->exec(line, "<stdin>", mode);
        }catch(NeedMoreLines& ne){
            buffer += line;
            buffer += '\n';
            need_more_lines = ne.is_compiling_class ? 3 : 2;
            if (need_more_lines) return true;
        }
        return false;
    }

}