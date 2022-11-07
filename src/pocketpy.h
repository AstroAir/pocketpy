#pragma once

#include "vm.h"
#include "compiler.h"

inline int _round(float f){
    if(f > 0) return (int)(f + 0.5);
    return (int)(f - 0.5);
}

#define BIND_NUM_ARITH_OPT(name, op)                                                                    \
    _vm->bindMethodMulti({"int","float"}, #name, [](VM* vm, PyVarList args){               \
        if(!vm->isIntOrFloat(args[0], args[1]))                                                         \
            vm->_error("TypeError", "unsupported operand type(s) for " #op );                        \
        if(args[0]->isType(vm->_tp_int) && args[1]->isType(vm->_tp_int)){                               \
            return vm->PyInt(vm->PyInt_AS_C(args[0]) op vm->PyInt_AS_C(args[1]));                       \
        }else{                                                                                          \
            return vm->PyFloat(vm->numToFloat(args[0]) op vm->numToFloat(args[1]));                     \
        }                                                                                               \
    });

#define BIND_NUM_LOGICAL_OPT(name, op, fallback)                                                        \
    _vm->bindMethodMulti({"int","float"}, #name, [](VM* vm, PyVarList args){               \
        if(!vm->isIntOrFloat(args[0], args[1])){                                                        \
            if constexpr(fallback) return vm->PyBool(args[0] op args[1]);                               \
            vm->_error("TypeError", "unsupported operand type(s) for " #op );                        \
        }                                                                                               \
        return vm->PyBool(vm->numToFloat(args[0]) op vm->numToFloat(args[1]));                          \
    });
    

void __initializeBuiltinFunctions(VM* _vm) {
    BIND_NUM_ARITH_OPT(__add__, +)
    BIND_NUM_ARITH_OPT(__sub__, -)
    BIND_NUM_ARITH_OPT(__mul__, *)

    BIND_NUM_LOGICAL_OPT(__lt__, <, false)
    BIND_NUM_LOGICAL_OPT(__le__, <=, false)
    BIND_NUM_LOGICAL_OPT(__gt__, >, false)
    BIND_NUM_LOGICAL_OPT(__ge__, >=, false)
    BIND_NUM_LOGICAL_OPT(__eq__, ==, true)
    BIND_NUM_LOGICAL_OPT(__ne__, !=, true)

#undef BIND_NUM_ARITH_OPT
#undef BIND_NUM_LOGICAL_OPT

    _vm->bindBuiltinFunc("print", [](VM* vm, PyVarList args) {
        for (auto& arg : args) vm->printFn(vm->PyStr_AS_C(vm->asStr(arg)) + " ");
        vm->printFn("\n");
        return vm->None;
    });

    _vm->bindBuiltinFunc("hash", [](VM* vm, PyVarList args) {
        return vm->PyInt(vm->hash(args.at(0)));
    });

    _vm->bindBuiltinFunc("chr", [](VM* vm, PyVarList args) {
        int i = vm->PyInt_AS_C(args.at(0));
        if (i < 0 || i > 128) vm->_error("ValueError", "chr() arg not in range(128)");
        return vm->PyStr(_Str(1, (char)i));
    });

    _vm->bindBuiltinFunc("round", [](VM* vm, PyVarList args) {
        return vm->PyInt(_round(vm->numToFloat(args.at(0))));
    });

    _vm->bindBuiltinFunc("ord", [](VM* vm, PyVarList args) {
        _Str s = vm->PyStr_AS_C(args.at(0));
        if (s.size() != 1) vm->_error("TypeError", "ord() expected an ASCII character");
        return vm->PyInt((int)s[0]);
    });

    _vm->bindBuiltinFunc("dir", [](VM* vm, PyVarList args) {
        PyVarList ret;
        for (auto& [k, _] : args.at(0)->attribs) ret.push_back(vm->PyStr(k));
        return vm->PyList(ret);
    });

    _vm->bindMethod("object", "__new__", [](VM* vm, PyVarList args) {
        PyVar obj = vm->newObject(args.at(0), -1);
        args.erase(args.begin());
        PyVarOrNull init_fn = vm->getAttr(obj, __init__, false);
        if (init_fn != nullptr) vm->call(init_fn, args);
        return obj;
    });

    _vm->bindMethod("object", "__str__", [](VM* vm, PyVarList args) {
        PyVar _self = args[0];
        _Str s = "<" + _self->getTypeName() + " object at " + std::to_string((uintptr_t)_self.get()) + ">";
        return vm->PyStr(s);
    });

    _vm->bindMethod("range", "__new__", [](VM* vm, PyVarList args) {
        _Range r;
        if( args.size() == 0 ) vm->_error("TypeError", "range expected 1 arguments, got 0");
        else if (args.size() == 1+1) {
            r.stop = vm->PyInt_AS_C(args[1]);
        }
        else if (args.size() == 2+1) {
            r.start = vm->PyInt_AS_C(args[1]);
            r.stop = vm->PyInt_AS_C(args[2]);
        }
        else if (args.size() == 3+1) {
            r.start = vm->PyInt_AS_C(args[1]);
            r.stop = vm->PyInt_AS_C(args[2]);
            r.step = vm->PyInt_AS_C(args[3]);
        }
        else {
            vm->_error("TypeError", "range expected 1 to 3 arguments, got " + std::to_string(args.size()-1));
        }
        return vm->PyRange(r);
    });

    _vm->bindMethod("range", "__iter__", [](VM* vm, PyVarList args) {
        vm->__checkType(args.at(0), vm->_tp_range);
        auto iter = std::make_shared<RangeIterator>(args[0], [=](int val){return vm->PyInt(val);});
        return vm->PyIter(iter);
    });

    _vm->bindMethod("NoneType", "__str__", [](VM* vm, PyVarList args) {
        return vm->PyStr("None");
    });

    _vm->bindMethodMulti({"int", "float"}, "__truediv__", [](VM* vm, PyVarList args) {
        if(!vm->isIntOrFloat(args[0], args[1]))
            vm->_error("TypeError", "unsupported operand type(s) for " "/" );
        return vm->PyFloat(vm->numToFloat(args[0]) / vm->numToFloat(args[1]));
    });

    _vm->bindMethodMulti({"int", "float"}, "__pow__", [](VM* vm, PyVarList args) {
        if(!vm->isIntOrFloat(args[0], args[1]))
            vm->_error("TypeError", "unsupported operand type(s) for " "**" );
        if(args[0]->isType(vm->_tp_int) && args[1]->isType(vm->_tp_int)){
            return vm->PyInt(_round(pow(vm->PyInt_AS_C(args[0]), vm->PyInt_AS_C(args[1]))));
        }else{
            return vm->PyFloat((float)pow(vm->numToFloat(args[0]), vm->numToFloat(args[1])));
        }
    });

    /************ PyInt ************/
    _vm->bindMethod("int", "__floordiv__", [](VM* vm, PyVarList args) {
        if(!args[0]->isType(vm->_tp_int) || !args[1]->isType(vm->_tp_int))
            vm->_error("TypeError", "unsupported operand type(s) for " "//" );
        return vm->PyInt(vm->PyInt_AS_C(args[0]) / vm->PyInt_AS_C(args[1]));
    });

    _vm->bindMethod("int", "__mod__", [](VM* vm, PyVarList args) {
        if(!args[0]->isType(vm->_tp_int) || !args[1]->isType(vm->_tp_int))
            vm->_error("TypeError", "unsupported operand type(s) for " "%" );
        return vm->PyInt(vm->PyInt_AS_C(args[0]) % vm->PyInt_AS_C(args[1]));
    });

    _vm->bindMethod("int", "__neg__", [](VM* vm, PyVarList args) {
        if(!args[0]->isType(vm->_tp_int))
            vm->_error("TypeError", "unsupported operand type(s) for " "-" );
        return vm->PyInt(-1 * vm->PyInt_AS_C(args[0]));
    });

    _vm->bindMethod("int", "__str__", [](VM* vm, PyVarList args) {
        return vm->PyStr(std::to_string(vm->PyInt_AS_C(args[0])));
    });

    /************ PyFloat ************/
    _vm->bindMethod("float", "__neg__", [](VM* vm, PyVarList args) {
        return vm->PyFloat(-1.0f * vm->PyFloat_AS_C(args[0]));
    });

    _vm->bindMethod("float", "__str__", [](VM* vm, PyVarList args) {
        return vm->PyStr(std::to_string(vm->PyFloat_AS_C(args[0])));
    });

    /************ PyString ************/
    _vm->bindMethod("str", "__new__", [](VM* vm, PyVarList args) {
        vm->_assert(args[0] == vm->_tp_str, "str.__new__ must be called with str as first argument");
        vm->_assert(args.size() == 2, "str expected 1 argument");
        return vm->asStr(args[1]);
    });

    _vm->bindMethod("str", "__add__", [](VM* vm, PyVarList args) {
        if(!args[0]->isType(vm->_tp_str) || !args[1]->isType(vm->_tp_str))
            vm->_error("TypeError", "unsupported operand type(s) for " "+" );
        const _Str& lhs = vm->PyStr_AS_C(args[0]);
        const _Str& rhs = vm->PyStr_AS_C(args[1]);
        return vm->PyStr(lhs + rhs);
    });

    _vm->bindMethod("str", "__len__", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        return vm->PyInt(_self.u8_length());
    });

    _vm->bindMethod("str", "__contains__", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const _Str& _other = vm->PyStr_AS_C(args[1]);
        return vm->PyBool(_self.str().find(_other.str()) != _Str::npos);
    });

    _vm->bindMethod("str", "__str__", [](VM* vm, PyVarList args) {
        return args[0]; // str is immutable
    });

    _vm->bindMethod("str", "__eq__", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const _Str& _other = vm->PyStr_AS_C(args[1]);
        return vm->PyBool(_self == _other);
    });

    _vm->bindMethod("str", "__ne__", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const _Str& _other = vm->PyStr_AS_C(args[1]);
        return vm->PyBool(_self != _other);
    });

    _vm->bindMethod("str", "__getitem__", [](VM* vm, PyVarList args) {
        const _Str& _self (vm->PyStr_AS_C(args[0]));

        if(args[1]->isType(vm->_tp_slice)){
            _Slice s = vm->PySlice_AS_C(args[1]);
            s.normalize(_self.u8_length());
            return vm->PyStr(_self.u8_substr(s.start, s.stop));
        }

        int _index = vm->PyInt_AS_C(args[1]);
        _index = vm->normalizedIndex(_index, _self.u8_length());
        return vm->PyStr(_self.u8_getitem(_index));
    });

    _vm->bindMethod("str", "__gt__", [](VM* vm, PyVarList args) {
        const _Str& _self (vm->PyStr_AS_C(args[0]));
        const _Str& _obj (vm->PyStr_AS_C(args[1]));
        return vm->PyBool(_self > _obj);
    });

    _vm->bindMethod("str", "__lt__", [](VM* vm, PyVarList args) {
        const _Str& _self (vm->PyStr_AS_C(args[0]));
        const _Str& _obj (vm->PyStr_AS_C(args[1]));
        return vm->PyBool(_self < _obj);
    });

    _vm->bindMethod("str", "upper", [](VM* vm, PyVarList args) {
        const _Str& _self (vm->PyStr_AS_C(args[0]));
        _StrStream ss;
        for(auto c : _self.str()) ss << (char)toupper(c);
        return vm->PyStr(ss);
    });

    _vm->bindMethod("str", "lower", [](VM* vm, PyVarList args) {
        const _Str& _self (vm->PyStr_AS_C(args[0]));
        _StrStream ss;
        for(auto c : _self.str()) ss << (char)tolower(c);
        return vm->PyStr(ss);
    });

    _vm->bindMethod("str", "replace", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const _Str& _old = vm->PyStr_AS_C(args[1]);
        const _Str& _new = vm->PyStr_AS_C(args[2]);
        std::string _copy = _self.str();
        // replace all occurences of _old with _new in _copy
        size_t pos = 0;
        while ((pos = _copy.find(_old.str(), pos)) != std::string::npos) {
            _copy.replace(pos, _old.str().length(), _new.str());
            pos += _new.str().length();
        }
        return vm->PyStr(_copy);
    });

    _vm->bindMethod("str", "startswith", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const _Str& _prefix = vm->PyStr_AS_C(args[1]);
        return vm->PyBool(_self.str().find(_prefix.str()) == 0);
    });

    _vm->bindMethod("str", "endswith", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const _Str& _suffix = vm->PyStr_AS_C(args[1]);
        return vm->PyBool(_self.str().rfind(_suffix.str()) == _self.str().length() - _suffix.str().length());
    });

    _vm->bindMethod("str", "join", [](VM* vm, PyVarList args) {
        const _Str& _self = vm->PyStr_AS_C(args[0]);
        const PyVarList& _list = vm->PyList_AS_C(args[1]);
        _StrStream ss;
        for(int i = 0; i < _list.size(); i++){
            if(i > 0) ss << _self;
            ss << vm->PyStr_AS_C(vm->asStr(_list[i]));
        }
        return vm->PyStr(ss);
    });

    /************ PyList ************/
    _vm->bindMethod("list", "__iter__", [](VM* vm, PyVarList args) {
        vm->__checkType(args.at(0), vm->_tp_list);
        auto iter = std::make_shared<VectorIterator>(args[0]);
        return vm->PyIter(iter);
    });

    _vm->bindMethod("list", "append", [](VM* vm, PyVarList args) {
        PyVarList& _self = vm->PyList_AS_C(args[0]);
        _self.push_back(args[1]);
        return vm->None;
    });

    _vm->bindMethod("list", "insert", [](VM* vm, PyVarList args) {
        PyVarList& _self = vm->PyList_AS_C(args[0]);
        int _index = vm->PyInt_AS_C(args[1]);
        _index = vm->normalizedIndex(_index, _self.size());
        _self.insert(_self.begin() + _index, args[2]);
        return vm->None;
    });

    _vm->bindMethod("list", "clear", [](VM* vm, PyVarList args) {
        vm->PyList_AS_C(args[0]).clear();
        return vm->None;
    });

    _vm->bindMethod("list", "copy", [](VM* vm, PyVarList args) {
        return vm->PyList(vm->PyList_AS_C(args[0]));
    });

    _vm->bindMethod("list", "pop", [](VM* vm, PyVarList args) {
        PyVarList& _self = vm->PyList_AS_C(args[0]);
        if(_self.empty()) vm->_error("IndexError", "pop from empty list");
        PyVar ret = _self.back();
        _self.pop_back();
        return ret;
    });      

    _vm->bindMethod("list", "__add__", [](VM* vm, PyVarList args) {
        const PyVarList& _self = vm->PyList_AS_C(args[0]);
        const PyVarList& _obj = vm->PyList_AS_C(args[1]);
        PyVarList _new_list = _self;
        _new_list.insert(_new_list.end(), _obj.begin(), _obj.end());
        return vm->PyList(_new_list);
    });

    _vm->bindMethod("list", "__len__", [](VM* vm, PyVarList args) {
        const PyVarList& _self = vm->PyList_AS_C(args[0]);
        return vm->PyInt(_self.size());
    });

    _vm->bindMethod("list", "__getitem__", [](VM* vm, PyVarList args) {
        const PyVarList& _self = vm->PyList_AS_C(args[0]);

        if(args[1]->isType(vm->_tp_slice)){
            _Slice s = vm->PySlice_AS_C(args[1]);
            s.normalize(_self.size());
            PyVarList _new_list;
            for(int i = s.start; i < s.stop; i++)
                _new_list.push_back(_self[i]);
            return vm->PyList(_new_list);
        }

        int _index = vm->PyInt_AS_C(args[1]);
        _index = vm->normalizedIndex(_index, _self.size());
        return _self[_index];
    });

    _vm->bindMethod("list", "__setitem__", [](VM* vm, PyVarList args) {
        PyVarList& _self = vm->PyList_AS_C(args[0]);
        int _index = vm->PyInt_AS_C(args[1]);
        _index = vm->normalizedIndex(_index, _self.size());
        _self[_index] = args[2];
        return vm->None;
    });

    _vm->bindMethod("list", "__delitem__", [](VM* vm, PyVarList args) {
        PyVarList& _self = vm->PyList_AS_C(args[0]);
        int _index = vm->PyInt_AS_C(args[1]);
        _index = vm->normalizedIndex(_index, _self.size());
        _self.erase(_self.begin() + _index);
        return vm->None;
    });

    /************ PyTuple ************/
    _vm->bindMethod("tuple", "__iter__", [](VM* vm, PyVarList args) {
        vm->__checkType(args.at(0), vm->_tp_tuple);
        auto iter = std::make_shared<VectorIterator>(args[0]);
        return vm->PyIter(iter);
    });

    _vm->bindMethod("tuple", "__len__", [](VM* vm, PyVarList args) {
        const PyVarList& _self = vm->PyTuple_AS_C(args[0]);
        return vm->PyInt(_self.size());
    });

    _vm->bindMethod("tuple", "__getitem__", [](VM* vm, PyVarList args) {
        const PyVarList& _self = vm->PyTuple_AS_C(args[0]);
        int _index = vm->PyInt_AS_C(args[1]);
        _index = vm->normalizedIndex(_index, _self.size());
        return _self[_index];
    });

    /************ PyBool ************/
    _vm->bindMethod("bool", "__str__", [](VM* vm, PyVarList args) {
        bool val = vm->PyBool_AS_C(args[0]);
        return vm->PyStr(val ? "True" : "False");
    });

    _vm->bindMethod("bool", "__eq__", [](VM* vm, PyVarList args) {
        return vm->PyBool(args[0] == args[1]);
    });
}

void __runCodeBuiltins(VM* vm, const char* src){
    _Code code = compile(vm, src, "builtins.py");
    vm->exec(code, {}, vm->builtins);
}

#include <cstdlib>
void __addModuleRandom(VM* vm){
    srand(time(NULL));
    PyVar random = vm->newModule("random");
    vm->bindFunc(random, "randint", [](VM* vm, PyVarList args) {
        int _min = vm->PyInt_AS_C(args[0]);
        int _max = vm->PyInt_AS_C(args[1]);
        return vm->PyInt(rand() % (_max - _min + 1) + _min);
    });
    vm->_modules["random"] = random;
}

#include "builtins.h"

#ifdef _WIN32
#define __EXPORT __declspec(dllexport)
#elif __APPLE__
#define __EXPORT __attribute__((visibility("default"))) __attribute__((used))
#else
#define __EXPORT
#endif

extern "C" {
    __EXPORT
    VM* createVM(PrintFn printFn){
        VM* vm = new VM();
        __initializeBuiltinFunctions(vm);
        //__runCodeBuiltins(vm, __BUILTINS_CODE);
        //__addModuleRandom(vm);
        vm->printFn = printFn;
        return vm;
    }

    __EXPORT
    void destroyVM(VM* vm){
        delete vm;
    }

    __EXPORT
    void exec(VM* vm, const char* source){
        try{
            _Code code = compile(vm, source, "main.py");
            vm->exec(code);
        }catch(std::exception& e){
            vm->printFn(e.what());
            vm->printFn("\n");
            vm->cleanError();
        }
    }

    __EXPORT
    void registerModule(VM* vm, const char* name, const char* source){
        _Code code = compile(vm, source, name + _Str(".py"));
        vm->registerCompiledModule(name, code);
    }
}