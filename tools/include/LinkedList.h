//
// Created by onioin on 13/03/25.
//

#ifndef ONIOIN_LINKEDLIST_H
#define ONIOIN_LINKEDLIST_H

#include <iterator>
#include <assert.h>
#include <cstddef>
#include <iostream>

namespace osl {

    template<typename T>
    class LinkedList {
        class Node {
        public:
            T data;
            Node *next;
            Node *prev;

            Node(T val) {
                data = val;
                next = nullptr;
                prev = nullptr;
            }
        };

    private:
        Node *head;
        Node *tail;
        size_t len;
    public:
        LinkedList() {
            head = nullptr;
            len = 0;
            tail = nullptr;
        }

        ~LinkedList();

        void append(T);

        void prepend(T);

        T removeHead();

        T removeTail();

        bool isEmpty() { return this->len == 0; }

        friend std::ostream &operator<<(std::ostream &OS, LinkedList &LL) {
            if (LL.isEmpty()) {
                OS << "List is empty.";
                return OS;
            }
            Node *curr = LL.head;
            OS << "[";
            while (nullptr != curr->next) {
                OS << curr->data << ", ";
                curr = curr->next;
            }
            OS << curr->data << "]";
            return OS;
        }

    public:
        class iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
        private:
            Node *curr;
        public:
            explicit iterator(Node *n) { curr = n; };

            explicit iterator(LinkedList<T> LL) { curr = LL.head; };

            iterator &operator++() {
                curr = curr->next;
                return *this;
            }

            iterator operator++(int) {
                iterator ret = *this;
                ++(*this);
                return ret;
            }

            iterator &operator--(){
                curr = curr->prev;
                return *this;
            }

            iterator operator--(int){
                iterator ret = *this;
                --(*this);
                return ret;
            }

            bool operator==(iterator other) const {
                return (this->curr == other.curr) || ((this->curr->data == other.curr->data) &&
                                                      (this->curr->next == other.curr->next));
            }

            bool operator!=(iterator other) const { return !(*this == other); };

            typename std::iterator<std::forward_iterator_tag, T>::reference operator*() const { return curr->data; }
        };

        iterator begin() { return iterator(head); }

        iterator end() { return iterator(tail); };

    };

    template<typename T>
    LinkedList<T>::~LinkedList() {
        Node *curr = head;
        while (nullptr != curr) {
            Node *nxt = curr->next;
            delete curr;
            curr = nxt;
        }
    }

    template<typename T>
    void LinkedList<T>::append(T val) {
        if (isEmpty()) {
            head = new Node(val);
            tail = head;
            len++;
            return;
        }
        Node *old = tail;
        tail = new Node(val);
        old->next = tail;
        tail->prev = old;
        len++;
    }

    template<typename T>
    void LinkedList<T>::prepend(T val) {
        if (isEmpty()) {
            head = new Node(val);
            tail = head;
            len++;
            return;
        }
        Node *nxt = head;
        head = new Node(val);
        head->next = nxt;
        nxt->prev = head;
        len++;
    }

    template<typename T>
    T LinkedList<T>::removeHead() {
        assert(!isEmpty() && "Cannot remove from an empty list");
        Node *toRet = head;
        head = toRet->next;
        head->prev = nullptr;
        T retVal = toRet->data;
        delete toRet;
        len--;
        return retVal;
    }

    template<typename T>
    T LinkedList<T>::removeTail() {
        assert(!isEmpty() && "Cannot remove from an empty list");
        Node *toRet = tail;
        tail = toRet->prev;
        tail->next = nullptr;
        T retVal = toRet->data;
        delete toRet;
        len--;
        return retVal;
    }

} //namespace
#endif //ONIOIN_LINKEDLIST_H
