#pragma once
#include <boost/variant.hpp>
#include <memory>
#include <string>

namespace GreenTiger {

template <typename Key> struct Tree;
template <typename Key> using TreePtr = std::unique_ptr<Tree<Key>>;
template <typename Key> struct Tree {
  TreePtr<Key> m_lhs;
  Key m_key;
  TreePtr<Key> m_rhs;

  Tree(const TreePtr<Key> &lhs, const Key &key, const TreePtr<Key> &rhs) :
      m_lhs(lhs ? std::make_unique<Tree>(*lhs) : nullptr), m_key(key),
      m_rhs(rhs ? std::make_unique<Tree>(*rhs) : nullptr) {}

  Tree(const Tree &other) : Tree(other.m_lhs, other.m_key, other.m_rhs) {}
};

template <typename Key>
TreePtr<Key> insert(const Key &key, const TreePtr<Key> &tree = {});

template <typename Key>
bool member(const Key &key, const TreePtr<Key> &tree = {});

template <typename Key>
TreePtr<Key> insertKeysHelper(const TreePtr<Key> &tree, const Key &key) {
  return insert(key, tree);
}

template <typename Key, typename... Keys>
TreePtr<Key> insertKeysHelper(const TreePtr<Key> &tree, const Key &key,
                              const Keys &... keys) {
  return insertKeysHelper(insert(key, tree), keys...);
}

template <typename Key, typename... Keys>
TreePtr<Key> insertKeys(const Key &key, const Keys &... keys) {
  return insertKeysHelper(TreePtr<Key>(), key, keys...);
}

template <typename Key, typename Payload> struct PayloadTree;
template <typename Key, typename Payload>
using PayloadTreePtr = std::unique_ptr<PayloadTree<Key, Payload>>;
template <typename Key, typename Payload> struct PayloadTree {
  PayloadTreePtr<Key, Payload> m_lhs;
  Key m_key;
  Payload m_payload;
  PayloadTreePtr<Key, Payload> m_rhs;

  PayloadTree(const PayloadTreePtr<Key, Payload> &lhs, const Key &key,
              const Payload &payload, const PayloadTreePtr<Key, Payload> &rhs) :
      m_lhs(lhs ? std::make_unique<PayloadTree>(*lhs) : nullptr),
      m_key(key), m_payload(payload),
      m_rhs(rhs ? std::make_unique<PayloadTree>(*rhs) : nullptr) {}

  PayloadTree(const PayloadTree &other) :
      PayloadTree(other.m_lhs, other.m_key, other.m_payload, other.m_rhs) {}
};

template <typename Key, typename Payload>
PayloadTreePtr<Key, Payload>
  insertPayload(const Key &key, const Payload &payload,
                const PayloadTreePtr<Key, Payload> &tree = {});

template <typename Key, typename Payload>
Payload lookup(const Key &key, const PayloadTreePtr<Key, Payload> &tree);

} // namespace GreenTiger

#include "Exercises.inl"