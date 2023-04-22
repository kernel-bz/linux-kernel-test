#include <fstream>
#include "bst.h"

void File2Tree(istream& fin, BST<string, int>& tree){
        string command, key; int elt; int r;
        while(fin>>command)
                if(command == "print") tree.Print();
                else if(command == "insert")
                        {fin >> key >> elt; tree.Insert(key, elt);}
                else if(command == "get"){
                        fin >> key;
                        if(tree.Get(key,elt))
                                cout << endl << "The value of " << key << " is " << elt;
                        else cout << endl << "No such key: " << key;
                }
                else if(command == "rankget"){
                        fin >> r; //read in ranking
                        if(tree.RankGet(r, key, elt))
                                cout << endl << "The " << r << "-th element is "
                                << key << ":" << elt << endl;
                        else cout << "No such ranking:" << r << endl;
                }
                else if(command == "delete"){
                        fin >> key; tree.Delete(key);
                }
                else cout << "Invalid command : " << command << endl;
}
int main(int argc, char* argv[]){
        if(argc < 2){
                cerr << "Usage: " << argv[0] << " infile[infile2]\n"; return 1;
        }
        ifstream fin(argv[1]);
        if(!fin){
                cerr << argv[1] << " open failed\n"; return 1;
        }
        BST<string, int> small;
        File2Tree(fin, small); fin.close();
        cout << endl;
        if(argc == 2) return 0;//argc[2]가 없으면 종료
        ifstream fin2(argv[2]);
        if(!fin2){ cerr << argv[2] << " open failed\n"; return 1;}
        cout << endl;
        BST<string, int> big;
        cout << "\n2nd tree follows\n";
        File2Tree(fin2, big); fin2.close();
        cout << endl;
        BST<string ,int> finaltree; finaltree.TwoWayJoin(small, big);
        cout << "\n<two way joined tree final print>"; finaltree.Print();
        cout << endl;
}
/*
<bst1.dat>
insert boy 23
insert emerald 70
insert cola 30
insert dog 40
insert ace 10
insert bug 27
insert boy 90
print
get boy
get emerald
get dog
get hohoho

<bst2.dat>
insert xboy 23
insert xemerald 70
insert xcola 30
insert xdog 40
insert xace 10
insert xboy 90
print
rankget 4
delete xcola
delete xemerald
print
get xboy
get xemerald
get xdog
get xhohoho

<hw9a bst1.dat result>
Inorder traversal :     ace:10 boy:90 bug:27 cola:30 dog:40 emerald:70
Postorder traversal :   ace:10 bug:27 dog:40 cola:30 emerald:70 boy:90
The value of boy is 90
The value of emerald is 70
The value of dog is 40
No such key: hohoho

<hw9b bst2.dat result>
Inorder traversal :     xace:10 xboy:90 xcola:30 xdog:40 xemerald:70
Postorder traversal :   xace:10 xdog:40 xcola:30 xemerald:70 xboy:90
The 4-th element is xdog:40

Inorder traversal :     xace:10 xboy:90 xdog:40
Postorder traversal :   xace:10 xdog:40 xboy:90
The value of xboy is 90
No such key: xemerald
The value of xdog is 40
No such key: xhohoho
*/