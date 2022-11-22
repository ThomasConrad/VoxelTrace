#pragma once

#include <bitset>
#include <list>
#include <queue>
#include <stdexcept>
#include <cstring>

#include "helpers.hpp"

typedef unsigned char uint8;
typedef unsigned int uint;

template <typename T>
struct OcTreeResult {
    uint depth;
    T item;
};

typedef struct Cell {
    uint8 data[4];  // 24 bit color/pointer + 8 bit data
	Cell() : data{0,0,0,0}{}
	Cell(glm::u8vec3 val, uint8 depth) : data{val.x, val.y, val.z, depth}{	}
} Cell;

typedef struct Grid {
    Cell val;  //"Mipmapped" value of cells
    Cell cells[8];
} Grid;

template <typename T>
class OcTree {
    template <typename D>
    struct Node {  // Doing a second template is unnecessary, but prevents complaining from VS about an ambiguous destructor
        Node<D>* parent;
        uint8 subindex_in_parent;
        uint depth;

        std::bitset<8> child_exists;  // A node either has nodes or data as children, depending on whether it is a leaf or branch node, never both!
        Node<D>* children[8];         // Array of pointers to child nodes
        D* data;                      // Dynamic array of type T

        Node(Node<D>* parent, uint8 subindex, uint depth) : parent{parent}, subindex_in_parent{subindex}, depth{depth} {
            for (int i = 0; i < 8; i++) {
                children[i] = nullptr;
            }
            data = nullptr;
        }
        ~Node() {
            delete[] data;
            for (auto node : children) {
                delete node;
            }
        }
    };

    Node<T> root;

    uint8 make_subindex_from_index(uint x, uint y, uint z, uint depth) {
        uint mask = 1 << (max_depth - depth);
        bool x_bit, y_bit, z_bit;
        x_bit = x & mask;
        y_bit = y & mask;
        z_bit = z & mask;
        return compose_subindex(x_bit, y_bit, z_bit);
    }
    static uint8 compose_subindex(bool x_bit, bool y_bit, bool z_bit) {
        uint out = 0;
        if (x_bit)
            out += 4;
        if (y_bit)
            out += 2;
        if (z_bit)
            out += 1;
        return out;
    }
    static void prune_branch(Node<T>* node) {
        if (node->child_exists.count() == 0 && node->depth > 0) {  // If node has no children and is not a root node
            Node<T>* parent = node->parent;
            parent->child_exists.reset(node->subindex_in_parent);  // Delete from parent
            parent->children[node->subindex_in_parent] = nullptr;  // When destructor is run later, this must be null to avoid access violation through the dangling pointer
            delete node;                                           // Delete node
            prune_branch(parent);                                  // Recursively prune parent node
        }
    }

    struct qElem {
        Node<T>* node;
        size_t idx;
        Cell* parent;
    };
    
    template<typename fake>
    Cell toPayload(Voxel data) {
        return Cell(glm::u8vec3(data.albedo * 255.0f), 1);
    }
    
    template<typename fake>
    Cell toPayload(int data) {
        return Cell{(uint8)(data),  // encode data in cell
                    (uint8)(data),
                    (uint8)(data),
                    1};
    }
    template<typename fake>
    Cell toPayload(float data) {
        return Cell{(uint8)(data * 255.0),  // encode data in cell
                    (uint8)(data * 255.0),
                    (uint8)(data * 255.0),
                    1};
    }

    template <typename D, typename fake = void>
    Cell toPayload(D data) {
        throw std::invalid_argument("No flattening implementation for template type.");
    }


    void flattenNodes(std::list<Grid>& pool,
                      std::queue<qElem>& queue) {
        struct qElem item = queue.front();
        queue.pop();
        Node<T>* node = item.node;
        size_t parent_idx = item.idx;
        Cell* parent = item.parent;

        size_t poolSize = pool.size();
        if (parent != NULL) {
            size_t offset = poolSize - parent_idx;
            assert(offset < 16'777'215); //Max 24 bit size
            glm::u8vec3 idx = glm::u8vec3((offset & 0x000000FF) >> 0,
                                          (offset & 0x0000FF00) >> 8,
                                          (offset & 0x00FF0000) >> 16);
            *parent = Cell(idx, 2);  // encode offset in cell
        }
        pool.push_back(Grid());  // Add a new grid array to the pool
        Grid* ptr = &(pool.back());
        for (uint i = 0; i < 8; i++) {  // Go through all children
            if (node->child_exists.test(i)) {
                if (node->depth != max_depth) {  // we can go deeper so add to queue
                    Node<T>* child = node->children[i];
                    assert(child != NULL);

                    queue.push(qElem{child, poolSize, &ptr->cells[i]});
                } else {  // node.depth == max_depth
                    T* data = node->data;
                    // Assume voxel payload
                    ptr->cells[i] = toPayload<T>(data[i]);
                }
            }
        }
    }

public:
    const uint max_depth;

    OcTree(uint _max_depth) : max_depth{_max_depth}, root{Node<T>(nullptr, 0, 0)} {}
    uint get_range() {
        return 1 << (max_depth + 1);  // 2^(max_depth+1)
    }

    void insert(uint x, uint y, uint z, T item) {
        Node<T>* current_node = &root;
        uint8 subindex;
        for (uint d = 0; d < max_depth; d++) {
            subindex = make_subindex_from_index(x, y, z, d);
            if (!current_node->child_exists.test(subindex)) {                    // If the next node doesn't exist yet
                Node<T>* new_node = new Node<T>(current_node, subindex, d + 1);  // Make it
                if (d + 1 == max_depth)
                    new_node->data = new T[8];  // If the new node is a leaf, allocate the data array
                current_node->children[subindex] = new_node;
                current_node->child_exists.set(subindex);
            }
            current_node = current_node->children[subindex];
        }
        subindex = make_subindex_from_index(x, y, z, max_depth);
        current_node->data[subindex] = item;
        current_node->child_exists.set(subindex);
    }

    void remove(uint x, uint y, uint z) {
        Node<T>* current_node = &root;
        uint8 subindex;
        for (uint d = 0; d < max_depth; d++) {
            subindex = make_subindex_from_index(x, y, z, d);
            if (!current_node->child_exists.test(subindex))
                return;  // If a branch node doesn't exist, the item doesn't exist, just return
            current_node = current_node->children[subindex];
        }
        subindex = make_subindex_from_index(x, y, z, max_depth);
        current_node->child_exists.reset(subindex);  // Currently, the data is just made invisible, not removed until the node itself is cleaned up, can be overwritten
        // current_node->data = T(); //Reset data to default value
        prune_branch(current_node);
    }

    // Returns true if an item exists at the given index. In either case, returns the depth of the recursion
    bool get(uint x, uint y, uint z, OcTreeResult<T>& result) {
        Node<T>* current_node = &root;
        uint8 subindex;
        for (uint d = 0; d < max_depth; d++) {
            subindex = make_subindex_from_index(x, y, z, d);
            if (!current_node->child_exists.test(subindex)) {
                result.depth = d;
                return false;
            }
            current_node = current_node->children[subindex];
        }
        subindex = make_subindex_from_index(x, y, z, max_depth);
        if (current_node->child_exists.test(subindex)) {
            result.depth = max_depth;
            result.item = current_node->data[subindex];
            return true;
        }
        result.depth = max_depth;
        return false;
    }

    uint8* flatten(size_t& size, size_t offset = 0) {
        std::list<Grid> pool;
        std::queue<qElem> queue;
        queue.push(qElem{&root, 0, NULL});
        while (!queue.empty()) {
            flattenNodes(pool, queue);
        }

        uint8* data = new uint8[offset + pool.size() * sizeof(Grid)];
        {
            int i = 0;
            for (Grid grid : pool) {
                std::memcpy(&data[i + offset], &grid, sizeof(Grid));
                i += sizeof(Grid) / sizeof(uint8);
            }
        }
        size = pool.size() * sizeof(Grid);  // size of pool in bytes

        return data;
    }
};