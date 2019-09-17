//===- DSGraphTraits.h - Provide generic graph interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides GraphTraits specializations for the DataStructure graph
// nodes, allowing datastructure graphs to be processed by generic graph
// algorithms.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_DSGRAPHTRAITS_H
#define LLVM_ANALYSIS_DSGRAPHTRAITS_H

#include "dsa/DSGraph.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/STLExtras.h"

#include <iterator>

namespace llvm {

template<typename NodeTy>
class DSNodeIterator : public std::iterator<std::forward_iterator_tag, const DSNode, ptrdiff_t> {
  friend class DSNode;

  DSNode::const_edge_iterator NH;

  typedef DSNodeIterator<NodeTy> _Self;

  DSNodeIterator(NodeTy *N) : NH(N->edge_begin()) {}   // begin iterator
  DSNodeIterator(NodeTy *N, bool) : NH(N->edge_end()) {}  // Create end iterator

public:
//  DSNodeIterator(const DSNodeHandle &NH)
//    : Node(NH.getNode()), Offset(NH.getOffset()) {}

  bool operator==(const _Self& x) const {
    return NH == x.NH;
  }
  bool operator!=(const _Self& x) const { return !operator==(x); }

  const _Self &operator=(const _Self &I) {
    NH = I.NH;
    return *this;
  }

  pointer operator*() const {
    return NH->second.getNode();
  }
  pointer operator->() const { return operator*(); }

  _Self& operator++() {                // Preincrement
    ++NH;
    return *this;
  }
  _Self operator++(int) { // Postincrement
    _Self tmp = *this; ++*this; return tmp;
  }

  unsigned getOffset() const { return NH->first; }
};

// Provide iterators for DSNode...
inline DSNode::iterator DSNode::begin() {
  return DSNode::iterator(this);
}
inline DSNode::iterator DSNode::end() {
  return DSNode::iterator(this, false);
}
inline DSNode::const_iterator DSNode::begin() const {
  return DSNode::const_iterator(this);
}
inline DSNode::const_iterator DSNode::end() const {
  return DSNode::const_iterator(this, false);
}

template <> struct GraphTraits<DSNode*> {
  typedef DSNode* NodeRef;
  typedef DSNode::iterator ChildIteratorType;

  static NodeRef getEntryNode(NodeRef N) { return N; }
  static ChildIteratorType child_begin(NodeRef N) { return N->begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->end(); }
};

template <> struct GraphTraits<const DSNode*> {
  typedef const DSNode* NodeRef;
  typedef DSNode::const_iterator ChildIteratorType;

  static NodeRef getEntryNode(NodeRef N) { return N; }
  static ChildIteratorType child_begin(NodeRef N) { return N->begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->end(); }
};

template <> struct GraphTraits<const DSGraph*> {
  typedef const DSNode* NodeRef;
  typedef DSNode::const_iterator ChildIteratorType;

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef pointer_iterator<DSGraph::node_const_iterator> nodes_iterator;
  static nodes_iterator nodes_begin(const DSGraph *G) {
    return nodes_iterator(G->node_begin());
  }
  static nodes_iterator nodes_end(const DSGraph *G) {
    return nodes_iterator(G->node_end());
  }

  static ChildIteratorType child_begin(NodeRef N) { return N->begin(); }
  static ChildIteratorType child_end(NodeRef N) { return N->end(); }
};

} // End llvm namespace

#endif
