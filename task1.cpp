#include <algorithm>
#include <initializer_list>
#include <list>
#include <stdexcept>
#include <tuple>
#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    static double rehash_enlarge_coaf;
    static double rehash_reduce_coaf;
    static size_t default_capacity;

private:
    using Node = std::pair<const KeyType, ValueType>;
    struct TableCell {
        std::list<size_t>::iterator it;
        std::vector<Node> nodes;
    };

public:
    class iterator {
        using CellIt = typename std::vector<TableCell>::iterator;
        using NodeIt = size_t;
    
    public:
        iterator() = default;
        iterator(CellIt cell_, NodeIt node_, HashMap* map)
                : cell(cell_), node(node_), connected_map(map) {
        }

        friend bool operator==(const iterator& lhs, const iterator& rhs) {
            return tie(lhs.cell, lhs.node) == tie(rhs.cell, rhs.node);
        }
        friend bool operator!=(const iterator& lhs, const iterator& rhs) {
            return !(lhs == rhs);
        }

        Node& operator*() {
            return cell->nodes[node];
        }
        typename std::vector<Node>::iterator operator->() {
            return cell->nodes.begin() + node;
        }

        iterator& operator++() {
            ++node;
            if (node == cell->nodes.size()) {
                while (next(cell->it) != connected_map->filled().end()) {
                    cell = connected_map->table().begin() + *next(cell->it);

                    node = cell->nodes.size();
                    if (!cell->nodes.empty()) {
                        node = 0;
                        break;
                    }
                }
            }

            return *this;
        }

        iterator operator++(int) {
            iterator old(*this);
            this->operator++();

            return old;
        }
    
    private:
        CellIt cell;
        NodeIt node;
        HashMap* connected_map;
    };

    class const_iterator {
        using CellIt = typename std::vector<TableCell>::const_iterator;
        using NodeIt = size_t;
    
    public:
        const_iterator() = default;
        const_iterator(CellIt cell_, NodeIt node_, const HashMap* map)
                : cell(cell_), node(node_), connected_map(map) {
        }

        friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) {
            return tie(lhs.cell, lhs.node) == tie(rhs.cell, rhs.node);
        }
        friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) {
            return !(lhs == rhs);
        }

        const Node& operator*() {
            return cell->nodes[node];
        }
        typename std::vector<Node>::const_iterator operator->() {
            return cell->nodes.cbegin() + node;
        }

        const_iterator& operator++() {
            ++node;
            if (node == cell->nodes.size()) {
                while (next(cell->it) != connected_map->filled().cend()) {
                    cell = connected_map->table().cbegin() + *next(cell->it);

                    node = cell->nodes.size();
                    if (!cell->nodes.empty()) {
                        node = 0;
                        break;
                    }
                }
            }

            return *this;
        }

        const_iterator operator++(int) {
            const_iterator old(*this);
            this->operator++();

            return old;
        }
    
    private:
        CellIt cell;
        NodeIt node;
        const HashMap* connected_map;
    };

public:
    iterator end() {
        if (size() > 0) {
            return iterator(table().begin() + filled_cells.back(), table()[filled_cells.back()].nodes.size(), this);
        }
        return iterator(prev(table().end()), 0, this);
    }
    iterator begin() {
        if (size() > 0) {
            return iterator(table().begin() + filled_cells.front(), 0, this);
        }
        return end();
    }

    const_iterator end() const {
        if (size() > 0) {
            return const_iterator(table().cbegin() + filled_cells.back(), table()[filled_cells.back()].nodes.size(), this);
        }
        return const_iterator(prev(table().cend()), 0, this);
    }
    const_iterator begin() const {
        if (size() > 0) {
            return const_iterator(table_.cbegin() + filled_cells.front(), 0, this);
        }
        return end();
    }

public:
    size_t size() const {
        return size_;
    }
    size_t capacity() const {
        return cap_;
    }
    Hash hash_function() const {
        return hasher_;
    }
    bool empty() const {
        return size() == 0;
    }

    HashMap(Hash hasher = Hash()) : hasher_(hasher) {
        default_init();
    }
    template<typename It>
    HashMap(It first, It last, Hash hasher = Hash()) : hasher_(hasher) {
        default_init();

        while (first != last) {
            insert(*first);
            ++first;
        }
    }
    HashMap(std::initializer_list<Node> l) {
        default_init();

        for (const auto& p : l) {
            insert(p);
        }
    }

    HashMap(const HashMap& other) {
        if (this != &other) {
            hasher_ = other.hash_function();
            clear();

            for (const auto& p : other) {
                insert(p);
            }
        }
    }

    HashMap& operator=(const HashMap& other) {
        if (this != &other) {
            hasher_ = other.hash_function();
            clear();

            for (const auto& p : other) {
                insert(p);
            }
        }

        return *this;
    }
    HashMap& operator=(HashMap&& other) {
        if (this != &other) {
            hasher_ = other.hash_function();
            clear();

            for (const auto& p : other) {
                insert(p);
            }
        }

        return *this;
    }

    iterator find(const KeyType& key) {
        size_t h = get_hash(key);
        auto& cell = table_[h].nodes;
        size_t pos = 0;
        for (auto it = cell.begin(); it != cell.end(); ++it, ++pos) {
            if (it->first == key) {
                return iterator(table_.begin() + h, pos, this);
            }
        }

        return end();
    }
    const_iterator find(const KeyType& key) const {
        size_t h = get_hash(key);
        auto& cell = table_[h].nodes;
        size_t pos = 0;
        for (auto it = cell.cbegin(); it != cell.cend(); ++it, ++pos) {
            if (it->first == key) {
                return const_iterator(table_.cbegin() + h, pos, this);
            }
        }

        return end();
    }

    void insert(const Node& node) {
        if (counts(node.first)) {
            return;
        }

        size_t h = get_hash(node.first);
        if (table_[h].nodes.empty()) {
            filled_cells.push_back(h);
            table_[h].it = prev(filled_cells.end());
        }
        table_[h].nodes.push_back(node);
        ++size_;

        if (size() * rehash_enlarge_coaf >= capacity()) {
            rehash(2 * capacity());
        }
    }
    void erase(const KeyType& key) {
        if (!counts(key)) {
            return;
        }

        --size_;
        size_t h = get_hash(key);
        auto& cell = table_[h].nodes;
        std::vector<Node> new_cell; // cannot use vector::erase because elements are not MoveAssignable.
        for (const auto& p : cell) {
            if (p.first == key) {
                continue;
            }
            new_cell.emplace_back(p);
        }
        cell.swap(new_cell);
        if (cell.empty()) {
            filled_cells.erase(table_[h].it);
        }

        // it makes everything work slower.
        // if (size() * (2 * rehash_reduce_coaf) < capacity()) {
        //     rehash(capacity() / rehash_reduce_coaf);
        // }
    }

    ValueType& operator[](const KeyType& key) {
        auto it = find(key);
        if (it == end()) {
            insert({key, ValueType()});
            it = find(key);
        }

        return it->second;
    }
    const ValueType& at(const KeyType& key) const {
        auto it = find(key);
        if (it != end()) {
            return it->second;
        }
        throw std::out_of_range("key not found.");
    }

    void clear() {
        for (const auto& cell : filled_cells) {
            table_[cell].nodes.clear();
        }
        filled_cells.clear();
        default_init();
    }

private:
    size_t size_;
    size_t cap_;
    Hash hasher_;
    std::list<size_t> filled_cells;
    std::vector<TableCell> table_;

    std::vector<TableCell>& table() {
        return table_;
    }
    const std::vector<TableCell>& table() const {
        return table_;
    }

    std::list<size_t>& filled() {
        return filled_cells;
    }
    const std::list<size_t>& filled() const {
        return filled_cells;
    }

    void rehash(size_t new_cap) {
        std::vector<Node> tmp;
        tmp.reserve(size());
        for (const auto& p : *this) {
            tmp.push_back(p);
        }
        clear();

        cap_ = new_cap;
        table_.resize(new_cap);
        for (const auto& p : tmp) {
            insert(p);
        }
    }

    void default_init() {
        size_ = 0;
        cap_ = default_capacity;
        table_.resize(cap_);
    }

    bool counts(const KeyType& key) const {
        return find(key) != end();
    }

    size_t get_hash(const KeyType& key) const {
        return hasher_(key) % capacity();
    }
};

template<class KeyType, class ValueType, class Hash>
size_t HashMap<KeyType, ValueType, Hash>::default_capacity = 8;

template<class KeyType, class ValueType, class Hash>
double HashMap<KeyType, ValueType, Hash>::rehash_enlarge_coaf = 2;

template<class KeyType, class ValueType, class Hash>
double HashMap<KeyType, ValueType, Hash>::rehash_reduce_coaf = 4;
