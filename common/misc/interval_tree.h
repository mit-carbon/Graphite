#pragma once

#include <string>
#include <utility>
#include <stdio.h>
using namespace std;

#include "fixed_types.h"

class IntervalTree
{
   public:
      class Node
      {
         public:
            Node();
            Node(const Node& node_);
            ~Node();

            void initialize(pair<UInt64,UInt64> interval_);

            Node* parent;
            Node* left;
            Node* right;
            SInt32 height;
            UInt64 key;
            pair<UInt64,UInt64> interval;
      };

      IntervalTree(Node* root_tree);
      ~IntervalTree();

      void insert(Node* node);
      Node* remove(Node* node);
      Node* search(pair<UInt64,UInt64> interval);
      UInt32 size() { return _size; }
      void inOrderTraversal();

   private:
      
      enum RotationDirection
      {
         CLOCKWISE = 0,
         ANTI_CLOCKWISE,
         NUM_ROTATION_DIRECTIONS
      };
      enum ChildDirection
      {
         INVALID = 0,
         LEFT,
         RIGHT,
         NUM_CHILD_DIRECTIONS = 2
      };

      void insertInTree(Node* node, Node* root_subtree);
      Node* removeFromTree(Node* node);
      Node* searchTree(pair<UInt64,UInt64> interval, Node* root_subtree);

      void rebalanceAVLTree(Node* root_subtree);
      Node* balanceHeight(Node* root_subtree);
      void performRotation(Node* y, RotationDirection rotation_direction);

      void updateHeight(Node* node);
      bool heightBalanced(Node* node);
      inline SInt32 getHeight(Node* root_subtree);

      void updateChildPointer(Node* node, Node* child_node, ChildDirection child_direction = INVALID);
      void updateParentPointer(Node* node, Node* parent_node);

      void swap(Node* node1, Node* node2);
      Node* findMinKeyNode(Node* root_subtree);

      void inOrderTraversalTree(Node* root_subtree, string prefix);

      Node* _root_tree;
      UInt32 _size;
};
