#include "file.hpp"
#include "spacemanager.hpp"
#include <string>
using namespace std;
int main() {

    InodeTable inode_table(50);
    for (int i = 0; i < 50; ++i) {
        uint64_t inum = inode_table.AllocateFileInode();
        cout << "inode table cur size: " << inode_table.GetCurSize() << endl;
    }

    for (int i = 49; i >= 0; --i) {
        inode_table.DeleteInode(i);
        cout << "inode table cur size: " << inode_table.GetCurSize() << endl;
    }


    Inode inode(FileType::REG_FILE, 0);
    for (int i = 0; i < 5; ++i) {
        string name = "file" + to_string(i);
        inode.AddDentry(name, i);
    }

    for (int i = 0; i < 5; ++i) {
        string name = "file" + to_string(i);
        auto dentry = inode.GetDentry(name);
        if (dentry != nullptr) {
            cout << "dentry name: " << dentry->GetName() << ", hash: " << dentry->GetHash() << " found." << endl;
        }
    }

    SpaceManager space_manager(123, FILE_BLOCK_SIZE << 8, FILE_BLOCK_SIZE);



    return 0;
}