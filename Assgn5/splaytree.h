

// we decided to not use this 
#include <cstdio>
template <typename T>
struct Node
{
    Node<T> *l;
    Node<T> *r;
    Node<T> *p;
    T v;
};
template <typename T>
struct SplayTree
{
    Node<T> *root;

    SplayTree()
    {
        root = NULL;
    }
    void rightRotate(Node<T> *P)
    {
        Node<T> *T_ = P->l;
        Node<T> *B = T_->r;
        Node<T> *D = P->p;
        if (D)
        {
            if (D->r == P)
                D->r = T_;
            else
                D->l = T_;
        }
        if (B)
            B->p = P;
        T_->p = D;
        T_->r = P;

        P->p = T_;
        P->l = B;
    }
    void leftRotate(Node<T> *P)
    {
        Node<T> *T_ = P->r;
        Node<T> *B = T_->l;
        Node<T> *D = P->p;
        if (D)
        {
            if (D->r == P)
                D->r = T_;
            else
                D->l = T_;
        }
        if (B)
            B->p = P;
        T_->p = D;
        T_->l = P;

        P->p = T_;
        P->r = B;
    }
    void splay(Node<T> *P)
    {
        while (true)
        {
            Node<T> *p = P->p;
            if (!p)
                break;
            Node<T> *pp = p->p;
            if (!pp) // Zig
            {
                if (p->l == P)
                    rightRotate(p);
                else
                    leftRotate(p);
                break;
            }
            if (pp->l == p)
            {
                if (p->l == P)
                { // ZigZig
                    rightRotate(pp);
                    rightRotate(p);
                }
                else
                { // ZigZag
                    leftRotate(p);
                    rightRotate(pp);
                }
            }
            else
            {
                if (p->l == P)
                { // ZigZag
                    rightRotate(p);
                    leftRotate(pp);
                }
                else
                { // ZigZig
                    leftRotate(pp);
                    leftRotate(p);
                }
            }
        }
        this->root = P;
    }
    void insert(T v)
    {
        if (!this->root)
        {
            this->root = (Node<T> *)malloc(sizeof(Node<T>));
            this->root->l = NULL;
            this->root->r = NULL;
            this->root->p = NULL;
            this->root->v = v;
            return;
        }
        Node<T> *P = this->root;
        while (true)
        {
            if (P->v == v)
                break; // not multiset
            if (v < (P->v))
            {
                if (P->l)
                    P = P->l;
                else
                {
                    P->l = (Node<T> *)malloc(sizeof(Node<T>));
                    P->l->p = P;
                    P->l->r = NULL;
                    P->l->l = NULL;
                    P->l->v = v;
                    P = P->l;
                    break;
                }
            }
            else
            {
                if (P->r)
                    P = P->r;
                else
                {
                    P->r = (Node<T> *)malloc(sizeof(Node<T>));
                    P->r->p = P;
                    P->r->r = NULL;
                    P->r->l = NULL;
                    P->r->v = v;
                    P = P->r;
                    break;
                }
            }
        }
        Splay(P);
    }
    void Inorder(Node<T> *R)
    {
        if (!R)
            return;
        Inorder(R->l);
        // printf("v: %d ", R->v);
        if (R->l)
            ; // printf("l: %d ", R->l->v);
        if (R->r)
            ; // printf("r: %d ", R->r->v);
        // puts("");
        Inorder(R->r);
    }
    Node<T> *Find(T v)
    {
        if (!this->root)
            return NULL;
        Node<T> *P = this->root;
        while (P)
        {
            if (P->v == v)
                break;
            if (v < (P->v))
            {
                if (P->l)
                    P = P->l;
                else
                    break;
            }
            else
            {
                if (P->r)
                    P = P->r;
                else
                    break;
            }
        }
        Splay(P);
        if (P->v == v)
            return P;
        else
            return NULL;
    }
    bool Erase(T v)
    {
        Node<T> *N = this->Find(v);
        if (!N)
            return false;
        Splay(N);
        Node<T> *P = N->l;
        if (!P)
        {
            this->root = N->r;
            this->root->p = NULL;
            free(N);
            return true;
        }
        while (P->r)
            P = P->r;
        if (N->r)
        {
            P->r = N->r;
            N->r->p = P;
        }
        this->root = N->l;
        this->root->p = NULL;
        free(N);
        return true;
    }
};
int main()
{
    while (true)
    {
        int t;
        scanf("%d", &t);
        if (t != 0 && t != -1)
            Insert(t);
        else if (t == 0)
        {
            scanf("%d", &t);
            if (!Find(t))
                printf("Couldn't Find %d!\n", t);
            else
                printf("Found %d!\n", t);
        }
        else
        {
            scanf("%d", &t);
            if (Erase(t))
                printf("Deleted %d!\n", t);
            else
                printf("Couldn't Find %d!\n", t);
        }
        if (root)
            printf("root: %d\n", root->v);
        Inorder(root);
    }
}