#include <iostream>

#include "octree.h"

#define Print(x) std::cout << x << std::endl;
#define Println(x) std::cout << x;

int main() {
    Print("Hello test");

    OcTree<int> tree(2);

    tree.insert(0, 0, 0, 1);
    tree.insert(1, 1, 2, 5);

    OcTreeResult<int> res;
    if (tree.get(0, 0, 0, res))
        Print(res.item);
    size_t size;
    uint8* data = tree.flatten(size);


    for (int i = 0; i < size; i++) {
        Println((int)data[i] << " ");
        if (i % 4 == 3)
            Print("");
        if (i % (4 * 9) == 4 * 9 - 1)
            Print("");
    }
    free(data);

    return 0;
}