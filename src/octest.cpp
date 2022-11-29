#include <iostream>

#include "octree.h"

#define Print(x) std::cout << x << std::endl;
#define Println(x) std::cout << x;

int main() {
    Print("Hello test");

    OcTree<Voxel> tree(2);

    tree.insert(0, 0, 0, Voxel());
    //tree.insert(1, 1, 2, 5);
    //tree.insert(4, 4, 2, 3);

    OcTreeResult<Voxel> res;
    if (tree.get(0, 0, 0, res))
        Print("Yeha");
    size_t size;
    uint8* data = tree.flatten(size,16);


    for (int i = 0; i < size; i++) {
        Println((int)data[i] << " ");
        if ((i-16) % 4 == 3)
            Print("");
        if ((i -16) % (4 * 9) == 4 * 9 - 1)
            Print("");
    }
    free(data);

    return 0;
}