
#ifndef PYLIST_H
#define PYLIST_H

#include <iostream>
#include <vector>
#include <memory>
#include <variant>

class pylist {
private:
    struct ListNode;
    using Value = std::variant<int, std::shared_ptr<ListNode>>;
    
    struct ListNode {
        std::vector<Value> data;
        int ref_count;
        
        ListNode() : ref_count(1) {}
    };
    
    std::shared_ptr<ListNode> node;
    
    // Helper function for output with self-reference detection
    static void output_list(std::ostream &os, const ListNode *current, const ListNode *original, 
                           std::vector<const ListNode*> &visited) {
        os << "[";
        for (size_t i = 0; i < current->data.size(); ++i) {
            if (i > 0) os << ", ";
            
            if (std::holds_alternative<int>(current->data[i])) {
                os << std::get<int>(current->data[i]);
            } else {
                auto sublist_node = std::get<std::shared_ptr<ListNode>>(current->data[i]).get();
                
                // Check if we've already visited this node (self-reference detection)
                bool already_visited = false;
                for (const ListNode* visited_node : visited) {
                    if (visited_node == sublist_node) {
                        already_visited = true;
                        break;
                    }
                }
                
                if (already_visited) {
                    os << "[...]";
                } else {
                    visited.push_back(sublist_node);
                    output_list(os, sublist_node, original, visited);
                    visited.pop_back();
                }
            }
        }
        os << "]";
    }

public:
    pylist() : node(std::make_shared<ListNode>()) {}
    
    // Copy constructor - shares the same underlying data
    pylist(const pylist &other) : node(other.node) {}
    
    // Assignment operator - shares the same underlying data
    pylist &operator=(const pylist &other) {
        if (this != &other) {
            node = other.node;
        }
        return *this;
    }
    
    void append(const pylist &x) {
        node->data.push_back(x.node);
    }
    
    void append(int x) {
        node->data.push_back(x);
    }
    
    pylist pop() {
        if (node->data.empty()) {
            return pylist();
        }
        
        Value last = node->data.back();
        node->data.pop_back();
        
        if (std::holds_alternative<int>(last)) {
            // Return a pylist containing the int
            pylist result;
            result.append(std::get<int>(last));
            return result;
        } else {
            // Return the pylist - just return the shared_ptr as is
            pylist result;
            result.node = std::get<std::shared_ptr<ListNode>>(last);
            return result;
        }
    }
    
    // Proxy class for handling both int and pylist assignments
    class Proxy {
    private:
        Value &value_ref;
        
    public:
        Proxy(Value &ref) : value_ref(ref) {}
        
        // Conversion to int
        operator int() const {
            if (std::holds_alternative<int>(value_ref)) {
                return std::get<int>(value_ref);
            }
            return 0; // Default value, though this shouldn't happen in valid usage
        }
        
        // Assignment from int
        Proxy &operator=(int x) {
            value_ref = x;
            return *this;
        }
        
        // Assignment from pylist
        Proxy &operator=(const pylist &x) {
            value_ref = x.node;
            return *this;
        }
        
        // Assignment from another Proxy
        Proxy &operator=(const Proxy &other) {
            value_ref = other.value_ref;
            return *this;
        }
        
        // Address-of operator - always returns Proxy* to avoid type conflicts
        Proxy* operator&() {
            return this;
        }
        
        // Implicit conversion to pylist pointer
        operator pylist*() const {
            if (std::holds_alternative<std::shared_ptr<ListNode>>(value_ref)) {
                // Create a temporary pylist from the shared_ptr and return its address
                static pylist temp;
                temp.node = std::get<std::shared_ptr<ListNode>>(value_ref);
                return &temp;
            }
            return nullptr;
        }
        
        // Comparison operators between Proxy objects
        friend bool operator==(const Proxy &p1, const Proxy &p2) {
            return static_cast<int>(p1) == static_cast<int>(p2);
        }
        
        friend bool operator!=(const Proxy &p1, const Proxy &p2) {
            return static_cast<int>(p1) != static_cast<int>(p2);
        }
        
        friend bool operator<(const Proxy &p1, const Proxy &p2) {
            return static_cast<int>(p1) < static_cast<int>(p2);
        }
        
        friend bool operator>(const Proxy &p1, const Proxy &p2) {
            return static_cast<int>(p1) > static_cast<int>(p2);
        }
        
        friend bool operator<=(const Proxy &p1, const Proxy &p2) {
            return static_cast<int>(p1) <= static_cast<int>(p2);
        }
        
        friend bool operator>=(const Proxy &p1, const Proxy &p2) {
            return static_cast<int>(p1) >= static_cast<int>(p2);
        }
        
        // Support for nested access
        Proxy operator[](size_t i) {
            if (std::holds_alternative<std::shared_ptr<ListNode>>(value_ref)) {
                auto sublist_node = std::get<std::shared_ptr<ListNode>>(value_ref);
                if (i < sublist_node->data.size()) {
                    return Proxy(sublist_node->data[i]);
                }
            }
            // Return a dummy proxy for invalid access
            static Value dummy_value = 0;
            return Proxy(dummy_value);
        }
        
        // Support for append method
        void append(int x) {
            if (std::holds_alternative<std::shared_ptr<ListNode>>(value_ref)) {
                auto sublist_node = std::get<std::shared_ptr<ListNode>>(value_ref);
                sublist_node->data.push_back(x);
            }
        }
        
        void append(const pylist &x) {
            if (std::holds_alternative<std::shared_ptr<ListNode>>(value_ref)) {
                auto sublist_node = std::get<std::shared_ptr<ListNode>>(value_ref);
                sublist_node->data.push_back(x.node);
            }
        }
        
        // Support for pop method
        pylist pop() {
            if (std::holds_alternative<std::shared_ptr<ListNode>>(value_ref)) {
                auto sublist_node = std::get<std::shared_ptr<ListNode>>(value_ref);
                if (!sublist_node->data.empty()) {
                    Value last = sublist_node->data.back();
                    sublist_node->data.pop_back();
                    
                    pylist result;
                    if (std::holds_alternative<int>(last)) {
                        result.append(std::get<int>(last));
                    } else {
                        result.node = std::get<std::shared_ptr<ListNode>>(last);
                    }
                    return result;
                }
            }
            return pylist();
        }
        
        // Support for arithmetic operations
        friend int operator+(const Proxy &p, int x) { return static_cast<int>(p) + x; }
        friend int operator-(const Proxy &p, int x) { return static_cast<int>(p) - x; }
        friend int operator*(const Proxy &p, int x) { return static_cast<int>(p) * x; }
        friend int operator/(const Proxy &p, int x) { return static_cast<int>(p) / x; }
        friend int operator%(const Proxy &p, int x) { return static_cast<int>(p) % x; }
        friend int operator&(const Proxy &p, int x) { return static_cast<int>(p) & x; }
        friend int operator|(const Proxy &p, int x) { return static_cast<int>(p) | x; }
        friend int operator^(const Proxy &p, int x) { return static_cast<int>(p) ^ x; }
        friend int operator<<(const Proxy &p, int x) { return static_cast<int>(p) << x; }
        friend int operator>>(const Proxy &p, int x) { return static_cast<int>(p) >> x; }
        
        friend int operator+(int x, const Proxy &p) { return x + static_cast<int>(p); }
        friend int operator-(int x, const Proxy &p) { return x - static_cast<int>(p); }
        friend int operator*(int x, const Proxy &p) { return x * static_cast<int>(p); }
        friend int operator/(int x, const Proxy &p) { return x / static_cast<int>(p); }
        friend int operator%(int x, const Proxy &p) { return x % static_cast<int>(p); }
        friend int operator&(int x, const Proxy &p) { return x & static_cast<int>(p); }
        friend int operator|(int x, const Proxy &p) { return x | static_cast<int>(p); }
        friend int operator^(int x, const Proxy &p) { return x ^ static_cast<int>(p); }
        friend int operator<<(int x, const Proxy &p) { return x << static_cast<int>(p); }
        friend int operator>>(int x, const Proxy &p) { return x >> static_cast<int>(p); }
        
        // Unary operators
        friend int operator+(const Proxy &p) { return +static_cast<int>(p); }
        friend int operator-(const Proxy &p) { return -static_cast<int>(p); }
        
        // Comparison operators
        friend bool operator<(const Proxy &p, int x) { return static_cast<int>(p) < x; }
        friend bool operator>(const Proxy &p, int x) { return static_cast<int>(p) > x; }
        friend bool operator<=(const Proxy &p, int x) { return static_cast<int>(p) <= x; }
        friend bool operator>=(const Proxy &p, int x) { return static_cast<int>(p) >= x; }
        friend bool operator==(const Proxy &p, int x) { return static_cast<int>(p) == x; }
        friend bool operator!=(const Proxy &p, int x) { return static_cast<int>(p) != x; }
        
        friend bool operator<(int x, const Proxy &p) { return x < static_cast<int>(p); }
        friend bool operator>(int x, const Proxy &p) { return x > static_cast<int>(p); }
        friend bool operator<=(int x, const Proxy &p) { return x <= static_cast<int>(p); }
        friend bool operator>=(int x, const Proxy &p) { return x >= static_cast<int>(p); }
        friend bool operator==(int x, const Proxy &p) { return x == static_cast<int>(p); }
        friend bool operator!=(int x, const Proxy &p) { return x != static_cast<int>(p); }
        
        // Output support
        friend std::ostream &operator<<(std::ostream &os, const Proxy &p) {
            os << static_cast<int>(p);
            return os;
        }
    };
    
    Proxy operator[](size_t i) {
        if (i >= node->data.size()) {
            // This shouldn't happen in valid usage, but let's handle it gracefully
            static Value dummy_value = 0;
            return Proxy(dummy_value);
        }
        return Proxy(node->data[i]);
    }
    
    // Support for arithmetic operations with pylist
    friend int operator+(int x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            return x + std::get<int>(ls.node->data[0]);
        }
        return x;
    }
    
    friend int operator-(int x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            return x - std::get<int>(ls.node->data[0]);
        }
        return x;
    }
    
    friend int operator*(int x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            return x * std::get<int>(ls.node->data[0]);
        }
        return x;
    }
    
    friend int operator/(int x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            return x / std::get<int>(ls.node->data[0]);
        }
        return x;
    }
    
    friend int operator%(int x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            return x % std::get<int>(ls.node->data[0]);
        }
        return x;
    }
    
    // Support for pylist + pylist operations
    friend int operator+(const pylist &ls1, const pylist &ls2) {
        int val1 = 0, val2 = 0;
        if (ls1.node->data.size() == 1 && std::holds_alternative<int>(ls1.node->data[0])) {
            val1 = std::get<int>(ls1.node->data[0]);
        }
        if (ls2.node->data.size() == 1 && std::holds_alternative<int>(ls2.node->data[0])) {
            val2 = std::get<int>(ls2.node->data[0]);
        }
        return val1 + val2;
    }
    
    friend int operator-(const pylist &ls1, const pylist &ls2) {
        int val1 = 0, val2 = 0;
        if (ls1.node->data.size() == 1 && std::holds_alternative<int>(ls1.node->data[0])) {
            val1 = std::get<int>(ls1.node->data[0]);
        }
        if (ls2.node->data.size() == 1 && std::holds_alternative<int>(ls2.node->data[0])) {
            val2 = std::get<int>(ls2.node->data[0]);
        }
        return val1 - val2;
    }
    
    friend int operator*(const pylist &ls1, const pylist &ls2) {
        int val1 = 0, val2 = 0;
        if (ls1.node->data.size() == 1 && std::holds_alternative<int>(ls1.node->data[0])) {
            val1 = std::get<int>(ls1.node->data[0]);
        }
        if (ls2.node->data.size() == 1 && std::holds_alternative<int>(ls2.node->data[0])) {
            val2 = std::get<int>(ls2.node->data[0]);
        }
        return val1 * val2;
    }
    
    friend int operator/(const pylist &ls1, const pylist &ls2) {
        int val1 = 0, val2 = 1;
        if (ls1.node->data.size() == 1 && std::holds_alternative<int>(ls1.node->data[0])) {
            val1 = std::get<int>(ls1.node->data[0]);
        }
        if (ls2.node->data.size() == 1 && std::holds_alternative<int>(ls2.node->data[0])) {
            val2 = std::get<int>(ls2.node->data[0]);
        }
        return val2 != 0 ? val1 / val2 : 0;
    }
    
    friend int operator%(const pylist &ls1, const pylist &ls2) {
        int val1 = 0, val2 = 1;
        if (ls1.node->data.size() == 1 && std::holds_alternative<int>(ls1.node->data[0])) {
            val1 = std::get<int>(ls1.node->data[0]);
        }
        if (ls2.node->data.size() == 1 && std::holds_alternative<int>(ls2.node->data[0])) {
            val2 = std::get<int>(ls2.node->data[0]);
        }
        return val2 != 0 ? val1 % val2 : 0;
    }
    
    // Support for compound assignment operators with template to handle different integer types
    template<typename T>
    friend T& operator+=(T &x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            x += static_cast<T>(std::get<int>(ls.node->data[0]));
        }
        return x;
    }
    
    template<typename T>
    friend T& operator-=(T &x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            x -= static_cast<T>(std::get<int>(ls.node->data[0]));
        }
        return x;
    }
    
    template<typename T>
    friend T& operator*=(T &x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            x *= static_cast<T>(std::get<int>(ls.node->data[0]));
        }
        return x;
    }
    
    template<typename T>
    friend T& operator/=(T &x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            x /= static_cast<T>(std::get<int>(ls.node->data[0]));
        }
        return x;
    }
    
    template<typename T>
    friend T& operator%=(T &x, const pylist &ls) {
        if (ls.node->data.size() == 1 && std::holds_alternative<int>(ls.node->data[0])) {
            x %= static_cast<T>(std::get<int>(ls.node->data[0]));
        }
        return x;
    }
    
    friend std::ostream &operator<<(std::ostream &os, const pylist &ls) {
        std::vector<const ListNode*> visited;
        visited.push_back(ls.node.get());  // Add current node to visited list first
        output_list(os, ls.node.get(), ls.node.get(), visited);
        return os;
    }
};

#endif //PYLIST_H
