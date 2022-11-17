#include "octree.h"
#include <iostream>

#define Print(x) std::cout << x << std::endl;
#define Println(x) std::cout << x;


int main(){
    Print("Hello test");

    OcTree<int> tree(2);

    tree.insert(0,0,0,1);
    tree.insert(1,1,2,1);

    OcTreeResult<int> res;
    if (tree.get(0,0,0,res)) Print(res.item);

    std::list<Grid> pool = tree.flatten();
    uint8* data = new uint8[4*9*pool.size()];
    {
        int i = 0;
        for (Grid grid : pool){
            memcpy(&data[i],&grid,9*4*sizeof(uint8));
            i += 9*4;
        }
    }



    for (int i = 0; i < 4*9*pool.size(); i++){
        Println((int)data[i] << " ");
        if (i%4==3) Print("");
        if (i%(4*9)==4*9-1) Print("");

    }
    free(data);

    return 0;
}