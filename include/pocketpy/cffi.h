#pragma once

#include "common.h"
#include "vm.h"
#include "_generated.h"

namespace pkpy {

#define PY_CLASS(T, mod, name)                  \
    static Type _type(VM* vm) {                 \
        PK_LOCAL_STATIC const std::pair<StrName, StrName> _path(#mod, #name); \
        return PK_OBJ_GET(Type, vm->_modules[_path.first]->attr()[_path.second]); \
    }                                                                       \
    static void _check_type(VM* vm, PyObject* val){                         \
        if(!vm->isinstance(val, T::_type(vm))){                             \
            vm->TypeError("expected '" #mod "." #name "', got " + OBJ_NAME(vm->_t(val)).escape());  \
        }                                                                   \
    }                                                                       \
    static PyObject* register_class(VM* vm, PyObject* mod) {                \
        if(OBJ_NAME(mod) != #mod) {                                         \
            auto msg = fmt("register_class() failed: ", OBJ_NAME(mod), " != ", #mod); \
            throw std::runtime_error(msg);                                  \
        }                                                                   \
        PyObject* type = vm->new_type_object(mod, #name, vm->tp_object);    \
        T::_register(vm, mod, type);                                        \
        type->attr()._try_perfect_rehash();                                 \
        return type;                                                        \
    }                                                                       

#define VAR_T(T, ...) vm->heap.gcnew<T>(T::_type(vm), __VA_ARGS__)

inline PyObject* py_var(VM* vm, void* p);
inline PyObject* py_var(VM* vm, char* p);

struct VoidP{
    PY_CLASS(VoidP, c, void_p)

    void* ptr;
    VoidP(void* ptr): ptr(ptr){}

    bool operator==(const VoidP& other) const {
        return ptr == other.ptr;
    }
    bool operator!=(const VoidP& other) const {
        return ptr != other.ptr;
    }
    bool operator<(const VoidP& other) const { return ptr < other.ptr; }
    bool operator<=(const VoidP& other) const { return ptr <= other.ptr; }
    bool operator>(const VoidP& other) const { return ptr > other.ptr; }
    bool operator>=(const VoidP& other) const { return ptr >= other.ptr; }


    Str hex() const{
        std::stringstream ss;
        ss << std::hex << reinterpret_cast<intptr_t>(ptr);
        return "0x" + ss.str();
    }

    static void _register(VM* vm, PyObject* mod, PyObject* type);
};

struct C99Struct{
    PY_CLASS(C99Struct, c, struct)

    static constexpr int INLINE_SIZE = 24;

    char _inlined[INLINE_SIZE];
    char* p;
    int size;

    C99Struct(int new_size){
        this->size = new_size;
        if(size <= INLINE_SIZE){
            p = _inlined;
        }else{
            p = (char*)malloc(size);
        }
    }

    C99Struct(void* p, int size): C99Struct(size){
        if(p != nullptr) memcpy(this->p, p, size);
    }

    C99Struct(const C99Struct& other): C99Struct(other.p, other.size){}

    ~C99Struct(){ if(p!=_inlined) free(p); }

    static void _register(VM* vm, PyObject* mod, PyObject* type);
};

struct ReflField{
    std::string_view name;
    int offset;
    bool operator<(const ReflField& other) const{ return name < other.name; }
    bool operator==(const ReflField& other) const{ return name == other.name; }
    bool operator!=(const ReflField& other) const{ return name != other.name; }
    bool operator<(std::string_view other) const{ return name < other; }
    bool operator==(std::string_view other) const{ return name == other; }
    bool operator!=(std::string_view other) const{ return name != other; }
};

struct ReflType{
    std::string_view name;
    size_t size;
};
inline static std::map<std::string_view, ReflType> _refl_types;

inline void add_refl_type(std::string_view name, size_t size){
    ReflType type{name, size};
    _refl_types[name] = std::move(type);
}

static_assert(sizeof(Py_<C99Struct>) <= 64);
static_assert(sizeof(Py_<Tuple>) <= 64);

inline PyObject* py_var(VM* vm, void* p){
    return VAR_T(VoidP, p);
}

inline PyObject* py_var(VM* vm, char* p){
    return VAR_T(VoidP, p);
}
/***********************************************/
template<typename T>
T to_void_p(VM* vm, PyObject* var){
    static_assert(std::is_pointer_v<T>);
    if(var == vm->None) return nullptr;     // None can be casted to any pointer implicitly
    VoidP& p = CAST(VoidP&, var);
    return reinterpret_cast<T>(p.ptr);
}
/*****************************************************************/
void add_module_c(VM* vm);

}   // namespace pkpy