#include <iostream>

#include "octree.h"

#define Print(x) std::cout << x << std::endl;
#define Println(x) std::cout << x;

void printBits(unsigned int num) {
    for (int bit = 0; bit < (sizeof(unsigned int) * 8); bit++) {
        printf("%i", num & 0x01);
        num = num >> 1;
    }
}

int main() {
    Print("Hello test");

    OcTree<Voxel> tree(0);

    tree.insert(0, 0, 0, Voxel(glm::vec3(1.0,1.0,0.0)));
    tree.insert(0, 0, 1, Voxel());

    //tree.insert(1, 1, 2, 5);
    //tree.insert(4, 4, 2, 3);

    OcTreeResult<Voxel> res;
    if (tree.get(0, 0, 0, res))
        Print("Yeha");
    size_t size;
    uint8* data = tree.flatten(size,16);


    for (int i = 16; i < size; i++) {
        Println((int)data[i] << " ");
        if ((i-16) % 4 == 3)
            Print("");
        if ((i -16) % (4 * 9) == 4 * 9 - 1)
            Print("");
    }
    /*for (int i = 4; i < 12; i++) {
        printBits(data[i]);
        Print("");
        Print(data[i]);
        Print("");
    }*/
    free(data);

    return 0;
}