namespace GreenTiger
{

  template<typename Key>
  TreePtr<Key> insert(const Key & key, const TreePtr<Key> & tree)
  {
    if (!tree)
    {
      return std::make_unique<Tree<Key>>(TreePtr<Key>(), key, TreePtr<Key>());
    }

    if (key < tree->m_key)
    {
      return std::make_unique<Tree<Key>>(insert(key, tree->m_lhs), tree->m_key, tree->m_rhs);
    }

    if (key > tree->m_key)
    {
      return std::make_unique<Tree<Key>>(tree->m_lhs, tree->m_key, insert(key, tree->m_rhs));
    }

    return std::make_unique<Tree<Key>>(tree->m_lhs, tree->m_key, tree->m_rhs);
  }

  template<typename Key>
  bool member(const Key & key, const TreePtr<Key> & tree)
  {
    if (!tree)
    {
      return false;
    }

    if (key < tree->m_key)
    {
      return member(key, tree->m_lhs);
    }

    if (key > tree->m_key)
    {
      return member(key, tree->m_rhs);
    }

    return true;
  }

  template<typename Key, typename Payload>
  PayloadTreePtr<Key, Payload> insertPayload(const Key & key, const Payload & payload, const PayloadTreePtr<Key, Payload>& tree)
  {
    if (!tree)
    {
      return std::make_unique<PayloadTree<Key, Payload>>(PayloadTreePtr<Key, Payload>(), key, payload, PayloadTreePtr<Key, Payload>());
    }

    if (key < tree->m_key)
    {
      return std::make_unique<PayloadTree<Key, Payload>>(insertPayload(key, payload, tree->m_lhs), tree->m_key, tree->m_payload, tree->m_rhs);
    }

    if (key > tree->m_key)
    {
      return std::make_unique<PayloadTree<Key, Payload>>(tree->m_lhs, tree->m_key, tree->m_payload, insertPayload(key, payload, tree->m_rhs));
    }

    return std::make_unique<PayloadTree<Key, Payload>>(tree->m_lhs, tree->m_key, payload, tree->m_rhs);
  }

  template<typename Key, typename Payload>
  Payload lookup(const Key & key, const PayloadTreePtr<Key, Payload>& tree)
  {
    if (!tree)
    {
      return {};
    }

    if (key < tree->m_key)
    {
      return lookup(key, tree->m_lhs);
    }

    if (key > tree->m_key)
    {
      return lookup(key, tree->m_rhs);
    }

    return tree->m_payload;
  }

} // namespace GreenTiger