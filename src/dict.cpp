#include "pocketpy/dict.h"

namespace pkpy{

    Dict::Dict(): _capacity(__Capacity),
            _mask(__Capacity-1),
            _size(0), _critical_size(__Capacity*__LoadFactor+0.5f), _head_idx(-1), _tail_idx(-1){
        _items = (Item*)pool128_alloc(_capacity * sizeof(Item));
        memset(_items, 0, _capacity * sizeof(Item));
        _nodes = (ItemNode*)pool64_alloc(_capacity * sizeof(ItemNode));
        memset(_nodes, -1, _capacity * sizeof(ItemNode));
    }

    Dict::Dict(Dict&& other){
        _capacity = other._capacity;
        _mask = other._mask;
        _size = other._size;
        _critical_size = other._critical_size;
        _head_idx = other._head_idx;
        _tail_idx = other._tail_idx;
        _items = other._items;
        _nodes = other._nodes;
        other._items = nullptr;
        other._nodes = nullptr;
    }

    Dict::Dict(const Dict& other){
        _capacity = other._capacity;
        _mask = other._mask;
        _size = other._size;
        _critical_size = other._critical_size;
        _head_idx = other._head_idx;
        _tail_idx = other._tail_idx;
        _items = (Item*)pool128_alloc(_capacity * sizeof(Item));
        memcpy(_items, other._items, _capacity * sizeof(Item));
        _nodes = (ItemNode*)pool64_alloc(_capacity * sizeof(ItemNode));
        memcpy(_nodes, other._nodes, _capacity * sizeof(ItemNode));
    }

    void Dict::set(VM* vm, PyVar key, PyVar val){
        // do possible rehash
        if(_size+1 > _critical_size) _rehash(vm);
        bool ok; int i;
        _probe_1(vm, key, ok, i);
        if(!ok) {
            _size++;
            _items[i].first = key;

            // append to tail
            if(_size == 0+1){
                _head_idx = i;
                _tail_idx = i;
            }else{
                _nodes[i].prev = _tail_idx;
                _nodes[_tail_idx].next = i;
                _tail_idx = i;
            }
        }
        _items[i].second = val;
    }

    void Dict::_rehash(VM* vm){
        Item* old_items = _items;
        ItemNode* old_nodes = _nodes;
        int old_head_idx = _head_idx;

        _capacity *= 4;
        _mask = _capacity - 1;
        _size = 0;
        _critical_size = _capacity*__LoadFactor+0.5f;
        _head_idx = -1;
        _tail_idx = -1;
        
        _items = (Item*)pool128_alloc(_capacity * sizeof(Item));
        memset(_items, 0, _capacity * sizeof(Item));
        _nodes = (ItemNode*)pool64_alloc(_capacity * sizeof(ItemNode));
        memset(_nodes, -1, _capacity * sizeof(ItemNode));

        // copy old items to new dict
        int i = old_head_idx;
        while(i != -1){
            set(vm, old_items[i].first, old_items[i].second);
            i = old_nodes[i].next;
        }
        pool128_dealloc(old_items);
        pool64_dealloc(old_nodes);
    }


    PyVar Dict::try_get(VM* vm, PyVar key) const{
        bool ok; int i;
        _probe_0(vm, key, ok, i);
        if(!ok) return nullptr;
        return _items[i].second;
    }

    bool Dict::contains(VM* vm, PyVar key) const{
        bool ok; int i;
        _probe_0(vm, key, ok, i);
        return ok;
    }

    bool Dict::erase(VM* vm, PyVar key){
        bool ok; int i;
        _probe_0(vm, key, ok, i);
        if(!ok) return false;
        _items[i].first = nullptr;
        // _items[i].second = PY_DELETED_SLOT;  // do not change .second if it is not NULL, it means the slot is occupied by a deleted item
        _size--;

        if(_size == 0){
            _head_idx = -1;
            _tail_idx = -1;
        }else{
            if(_head_idx == i){
                _head_idx = _nodes[i].next;
                _nodes[_head_idx].prev = -1;
            }else if(_tail_idx == i){
                _tail_idx = _nodes[i].prev;
                _nodes[_tail_idx].next = -1;
            }else{
                _nodes[_nodes[i].prev].next = _nodes[i].next;
                _nodes[_nodes[i].next].prev = _nodes[i].prev;
            }
        }
        _nodes[i].prev = -1;
        _nodes[i].next = -1;
        return true;
    }

    void Dict::update(VM* vm, const Dict& other){
        other.apply([&](PyVar k, PyVar v){ set(vm, k, v); });
    }

    Tuple Dict::keys() const{
        Tuple t(_size);
        int i = _head_idx;
        int j = 0;
        while(i != -1){
            t[j++] = _items[i].first;
            i = _nodes[i].next;
        }
        PK_ASSERT(j == _size);
        return t;
    }

    Tuple Dict::values() const{
        Tuple t(_size);
        int i = _head_idx;
        int j = 0;
        while(i != -1){
            t[j++] = _items[i].second;
            i = _nodes[i].next;
        }
        PK_ASSERT(j == _size);
        return t;
    }

    void Dict::clear(){
        _size = 0;
        _head_idx = -1;
        _tail_idx = -1;
        memset(_items, 0, _capacity * sizeof(Item));
        memset(_nodes, -1, _capacity * sizeof(ItemNode));
    }

    Dict::~Dict(){
        if(_items==nullptr) return;
        pool128_dealloc(_items);
        pool64_dealloc(_nodes);
    }
}   // namespace pkpy