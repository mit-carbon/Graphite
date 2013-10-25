#include <cassert>
#include <stdlib.h>

#include "interval_tree.h"
#include "utils.h"
#include "log.h"

IntervalTree::Node::Node():
   parent(NULL),
   left(NULL),
   right(NULL),
   height(0),
   key(0),
   interval(make_pair<UInt64,UInt64>(0,0))
{ }

IntervalTree::Node::Node(const Node& node_):
   parent(node_.parent),
   left(node_.left),
   right(node_.right),
   height(node_.height),
   key(node_.key),
   interval(node_.interval)
{ }

IntervalTree::Node::~Node()
{ }

void
IntervalTree::Node::initialize(pair<UInt64,UInt64> interval_)
{
   parent = NULL;
   left = NULL;
   right = NULL;
   height = 1;
   key = interval_.first;
   interval = interval_;
}

IntervalTree::IntervalTree(Node* root_tree):
   _root_tree(root_tree),
   _size(1)
{ }

IntervalTree::~IntervalTree()
{
   // inOrderTraversal();
}

void
IntervalTree::updateChildPointer(Node* node, Node* child_node, ChildDirection child_direction)
{
   if (node)
   {
      switch (child_direction)
      {
         case INVALID:
            assert(child_node);
            if (node->key < child_node->key)
               node->right = child_node;
            else
               node->left = child_node;
            break;

         case LEFT:
            node->left = child_node;
            break;

         case RIGHT:
            node->right = child_node;
            break;

         default:
            LOG_PRINT_ERROR("Undefined child_direction(%u)", child_direction);
            break;
      }
   }
}

void
IntervalTree::updateParentPointer(Node* node, Node* parent_node)
{
   if (node)
      node->parent = parent_node;
   if (parent_node == NULL)
      _root_tree = node;
}

inline SInt32
IntervalTree::getHeight(Node* root_subtree)
{
   return root_subtree ? (root_subtree->height) : 0;
}

bool
IntervalTree::heightBalanced(Node* node)
{
   assert(node);
   return ( ( abs( getHeight(node->left) - getHeight(node->right) ) <= 1) ? true : false );
}

void
IntervalTree::updateHeight(Node* node)
{
   assert(node);
   node->height = getMax<SInt32>( getHeight(node->left), getHeight(node->right) ) + 1;
}

void
IntervalTree::performRotation(Node* y, RotationDirection rotation_direction)
{
   LOG_PRINT("performRotation(%llu), direction(%u)", y->key, rotation_direction);
   Node* x;

   if (rotation_direction == CLOCKWISE)
   {
      x = y->left;
      
      updateParentPointer(x, y->parent);
      updateChildPointer(x->parent, x);

      y->left = x->right;
      updateParentPointer(y->left, y);

      x->right = y;
      updateParentPointer(y, x);
   }
   else // rotation_direction == ANTI_CLOCKWISE
   {
      x = y->right;
      
      updateParentPointer(x, y->parent);
      updateChildPointer(x->parent, x);

      y->right = x->left;
      updateParentPointer(y->right, y);

      x->left = y;
      updateParentPointer(y, x);
   }

   // Adjust Heights - Adjust y's height first
   updateHeight(y);
   updateHeight(x);
}

IntervalTree::Node*
IntervalTree::balanceHeight(Node* root_subtree)
{
   Node* z = root_subtree;

   assert(getHeight(z->left) != getHeight(z->right));
   Node* y = (getHeight(z->left) > getHeight(z->right)) ? z->left : z->right;
   ChildDirection y_direction = (getHeight(z->left) > getHeight(z->right)) ? LEFT : RIGHT;

   assert(y);

   Node* x;
   ChildDirection x_direction;
   if (getHeight(y->left) != getHeight(y->right))
   {
      x = (getHeight(y->left) > getHeight(y->right)) ? y->left : y->right;
      x_direction = (getHeight(y->left) > getHeight(y->right)) ? LEFT : RIGHT;
   }
   else if (y_direction == LEFT)
   {
      x = y->left;
      x_direction = LEFT;
   }
   else // y->direction == RIGHT
   {
      x = y->right;
      x_direction = RIGHT;
   }

   assert(x);

   // Perform single/double rotation
   if (y_direction == LEFT)
   {
      if (x_direction == RIGHT)
      {
         performRotation(y, ANTI_CLOCKWISE);
         performRotation(z, CLOCKWISE);
         return x;
      }
      else // x_direction == LEFT
      {
         performRotation(z, CLOCKWISE);
         return y;
      }
   }
   else // y_direction == RIGHT
   {
      if (x_direction == LEFT)
      {
         performRotation(y, CLOCKWISE);
         performRotation(z, ANTI_CLOCKWISE);
         return x;
      }
      else // x_direction == RIGHT
      {
         performRotation(z, ANTI_CLOCKWISE);
         return y;
      }
   }
}

void
IntervalTree::rebalanceAVLTree(Node* root_subtree)
{
   if (root_subtree == NULL)
      return;

   SInt32 old_height = root_subtree->height;
   Node* new_root_subtree = root_subtree;

   if (!heightBalanced(root_subtree))
   {
      // Do rotations for balancing (node)
      new_root_subtree = balanceHeight(root_subtree);
   }
   else
   {
      // Update height of node to reflect changes
      updateHeight(root_subtree);
   }

   // (node) should be height balanced by now
   if (!heightBalanced(new_root_subtree))
   {
      inOrderTraversal();
      LOG_PRINT_ERROR("Left Subtree Height(%i), Right Subtree Height(%i)", \
         getHeight(new_root_subtree->left), getHeight(new_root_subtree->right));
   }
   
   if (new_root_subtree->height != old_height)
   {
      // Propagate height changes up
      rebalanceAVLTree(new_root_subtree->parent);
   }
}

void
IntervalTree::swap(Node* node1, Node* node2)
{
   Node temp_node(*node1);

   node1->key = node2->key;
   node1->interval = node2->interval;

   node2->key = temp_node.key;
   node2->interval = temp_node.interval;
}

IntervalTree::Node*
IntervalTree::findMinKeyNode(Node* root_subtree)
{
   if (root_subtree->left)
      return findMinKeyNode(root_subtree->left);
   else
      return root_subtree;
}

void
IntervalTree::insert(Node* node)
{
   LOG_PRINT("Insert(%llu)", node->key);
   _size ++;
   return insertInTree(node, _root_tree);
}

void
IntervalTree::insertInTree(Node* node, Node* root_subtree)
{
   if (node->key < root_subtree->key)
   {
      if (root_subtree->left)
      {
         insertInTree(node, root_subtree->left);
      }
      else
      {
         root_subtree->left = node;
         node->parent = root_subtree;
         // Rebalance the AVL Tree
         rebalanceAVLTree(root_subtree);
      }
   }
   else if (node->key > root_subtree->key)
   {
      if (root_subtree->right)
      {
         insertInTree(node, root_subtree->right);
      }
      else
      {
         root_subtree->right = node;
         node->parent = root_subtree;
         // Rebalance the AVL Tree
         rebalanceAVLTree(root_subtree);
      }
   }
   else
   {
      inOrderTraversal();
      LOG_PRINT_ERROR("Found 2 nodes with same key(%llu)", node->key);
   }
}

IntervalTree::Node*
IntervalTree::remove(Node* node)
{
   LOG_PRINT("Remove(%llu)", node->key);
   _size --;
   return removeFromTree(node);
}

IntervalTree::Node*
IntervalTree::removeFromTree(Node* node)
{
   if (!node->left)
   {
      if (node->parent)
         updateChildPointer(node->parent, node->right, (node->parent->key < node->key) ? RIGHT : LEFT);
      updateParentPointer(node->right, node->parent);
      // Rebalance the AVL Tree
      rebalanceAVLTree(node->parent);

      return node;
   }
   else if (!node->right)
   {
      updateChildPointer(node->parent, node->left);
      updateParentPointer(node->left, node->parent);
      // Rebalance the AVL Tree
      rebalanceAVLTree(node->parent);

      return node;
   }
   else // ((node->left) && (node->right))
   {
      // Node has both children
      // Find Successor of node
      Node* successor_node = findMinKeyNode(node->right);
      __attribute__((unused)) Node* removed_node = removeFromTree(successor_node);
      assert(removed_node == successor_node);

      // Swap Node & Successor Node Parameters
      swap(node, successor_node);

      // Return the successor node
      return successor_node;
   }

}

IntervalTree::Node*
IntervalTree::search(pair<UInt64,UInt64> interval)
{
   LOG_PRINT("Search(%llu,%llu)", interval.first, interval.second);
   return searchTree(interval, _root_tree);
}

IntervalTree::Node*
IntervalTree::searchTree(pair<UInt64,UInt64> interval, Node* root_subtree)
{
   if (!root_subtree)
      return NULL;

   if ((interval.first >= root_subtree->interval.first) && (interval.second <= root_subtree->interval.second))
   {
      // Found interval in current node
      return root_subtree;
   }
   
   if (interval.second < root_subtree->interval.first)
   {
      // Explore left subtree
      Node* found_node = searchTree(interval, root_subtree->left);
      if (found_node)
         return found_node;
   }

   if ((interval.first < root_subtree->interval.first) && \
         ((root_subtree->interval.second - root_subtree->interval.first) >= (interval.second - interval.first)))
   {
      // Found interval in current node
      return root_subtree;
   }

   // Explore right subtree
   return searchTree(interval, root_subtree->right);
}

void
IntervalTree::inOrderTraversal()
{
   inOrderTraversalTree(_root_tree, "");
   fprintf(stderr, "Size(%i)\n", _size);
}

void
IntervalTree::inOrderTraversalTree(Node* root_subtree, string prefix)
{
   if (!root_subtree)
      return;
   inOrderTraversalTree(root_subtree->left, prefix + "LL ");
   fprintf(stderr, "%s(%llu, %llu)\n", \
         prefix.c_str(), \
         (long long unsigned int) root_subtree->interval.first, \
         (long long unsigned int) root_subtree->interval.second);
   inOrderTraversalTree(root_subtree->right, prefix + "RR ");
}
