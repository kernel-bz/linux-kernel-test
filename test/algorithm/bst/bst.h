#ifndef TREE_H
#define TREE_H
#include <iostream>
#include <queue>
using namespace std;
template<class K, class E>
struct Node{
        Node(K ky, E el, Node<K, E> *left=0, Node<K, E> *right = 0)
        : key(ky), element(el), leftChild(left), rightChild(right){}
        Node<K,E> *leftChild;
        K key;
        E element;
        Node<K, E> *rightChild;
        int leftSize;
};
template<class K, class E>
class BST{
public:
        BST(){root = 0;}
        void Insert(K &newkey, E &el){Insert(root, newkey, el);}
        void Preorder() {Preorder(root);}
        void Inorder() {Inorder(root);}
        void Postorder() {Postorder(root);}
        void Levelorder();
        bool Get(const K&, E&);
        bool Print();
        bool RankGet(int r, K &k, E &e);
        void Delete(K &oldkey){Delete(root, oldkey);}
        void ThreeWayJoin(BST<K, E>& small, K midkey, E midel, BST<K, E>& big);
        void TwoWayJoin(BST<K, E>& small, BST<K, E>& big);
private: //helper 함수들
        void Visit(Node<K, E> *);
        void Insert(Node<K, E>* &, K &, E &);
        void Preorder(Node<K, E> *);
        void Inorder(Node<K, E> *);
        void Postorder(Node<K, E> *);
        void Delete(Node<K, E>* &, K &);
        Node<K, E> *root;
};
template <class K, class E>
void BST<K, E>::ThreeWayJoin(BST<K, E>& small,K midkey, E midel, BST<K, E>& big){
        root = new Node<K, E>(midkey, midel, small.root, big.root);
        small.root = big.root = 0;
}
template <class K, class E>
void BST<K, E>::TwoWayJoin(BST<K, E>& small, BST<K, E>& big){
        if(!small.root) {root = big.root; big.root = 0;}
        if(!big.root) {root = small.root; small.root = 0;}
        BST small2 = small;
        Node<K, E> *currentNode = small2.root->rightChild;
        while(currentNode->rightChild){
        currentNode = currentNode->rightChild;
        }
        Node<K, E> tmproot = *currentNode;
        small2.Delete(currentNode->key);
        small = small2;
        ThreeWayJoin(small, tmproot.key, tmproot.element, big);
        small.root = 0; big.root = 0;
}
template <class K, class E>
bool BST<K, E>::RankGet(int r, K &k, E &e){
        Node<K, E> *currentNode = root;
        while(currentNode)
                if(r<currentNode->leftSize){
                        currentNode = currentNode->leftChild;
                }
                else if(r>currentNode->leftSize){
                        r -= currentNode->leftSize;
                        currentNode = currentNode->rightChild;
                }
                else return k = currentNode->key, e = currentNode->element;
                return 0;
}
template <class K, class E>
void BST<K, E>::Delete(Node<K, E>* &ptr, K &oldkey){
        Node<K, E>*tmpptr; Node<K, E> *tmpdaddyptr;
        if(ptr == 0) return;//그런 노드가 없으면 그냥 return
        if(oldkey < ptr->key) Delete(ptr->leftChild, oldkey);
        else if(oldkey>ptr->key){//우측 아들을 루트로 하는 트리에서 지우시오
                Delete(ptr->rightChild, oldkey);
        }
        else{//ptr노드가 바로 지울 노드인 경우임
                if(!ptr->leftChild && !ptr->rightChild)//아들이 없다면
                        {delete ptr; ptr = 0; return;}
                else if(ptr->leftChild && !ptr->rightChild)
                //그 아들을 ptr가 가리키게 하고 현재 ptr가 가리키는 노드 지움
                { tmpptr = ptr; ptr = ptr->leftChild; delete tmpptr; return;}
                else if(ptr->rightChild && !ptr->leftChild)
                { tmpptr = ptr; ptr = ptr->rightChild; delete tmpptr; return;}
                else{//두 아들있음:루트가 rc인 우측트리에서 '제일 작은 노드'찾자
                        Node<K, E> *rc = ptr->rightChild;//rc가 루트인 subtree
                        if(!rc->leftChild)//rc가 왼쪽아들이 없으면 rc가 그 노드!
                        {ptr->key = rc->key; ptr->element = rc->element;
                        ptr->leftChild = rc->rightChild; delete rc;
                        }
                        else{
                                while(rc->leftChild){
                                        rc = rc->leftChild;
                                }
                        ptr->key = rc->key; ptr->element = rc->element;
                        ptr->leftChild = rc->rightChild; delete rc;
                        }
                }
        }
}
template <class K, class E>
bool BST<K, E>::Get(const K& k, E& e){//helper 대신 직접수행
        Node<K,E> *ptr = root;
        while(ptr)
                if(k < ptr->key) ptr = ptr->leftChild;
                else if(k > ptr->key) ptr = ptr->rightChild;
                else{ e = ptr->element; return true;}
        return false;
}
template <class K, class E>
bool BST<K, E>::Print(){//inorder, preorder와 postorder로 traversal함
        cout << endl << "Inorder traversal :    "; Inorder();
        cout << endl << "Preorder traversal :  "; Preorder();
        cout << endl << "Postorder traversal :  "; Postorder();
}
template <class K, class E>
void BST<K, E>::Visit(Node<K, E> *ptr){
        cout << ptr->key << ":" << ptr->element << " ";
}
template <class K, class E>
void BST<K, E>::Insert(Node<K, E>* &ptr, K &newkey, E &el){//Insert의 helpler 함수
        if(ptr==0) {ptr = new Node<K, E>(newkey, el); ptr->leftSize = 1;}//아무것도 없으면
        else if(newkey < ptr->key) {ptr->leftSize++; Insert(ptr->leftChild, newkey, el);}
        else if(newkey > ptr->key) Insert(ptr->rightChild, newkey, el);
        else if(newkey == ptr->key) ptr->element = el;//같은 key가 있으면
        else ptr->element = el;//Update element
}
template <class K, class E>
void BST<K, E>::Inorder(Node<K, E> *currentNode){//Inorder의 helper함수
        if(currentNode){
                Inorder(currentNode->leftChild);
                Visit(currentNode);
                Inorder(currentNode->rightChild);
        }
}
template <class K, class E>
void BST<K, E>::Postorder(Node<K, E> *currentNode){//Postorder의 helper함수
        if(currentNode){
                Postorder(currentNode->leftChild);
                Postorder(currentNode->rightChild);
                Visit(currentNode);
        }
}
template <class K, class E>
void BST<K, E>::Preorder(Node<K, E> *currentNode){//Postorder의 helper함수
        if(currentNode){
                Visit(currentNode);
                Postorder(currentNode->leftChild);
                Postorder(currentNode->rightChild);
        }
}
#endif