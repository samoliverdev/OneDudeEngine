// <author>Mikhail Semenov</author>
// <date>2015-01-18</date>
// <summary>Contains implementation of various maps</summary>

#ifndef INDEX_MAPS_H
#define INDEX_MAPS_H
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <string>
#include <sstream>
#include <map>

template<class K>
std::out_of_range make_out_of_range(const std::string& message, const K& key)
{
    std::ostringstream ss;
    ss << message << key;
    return std::out_of_range(ss.str());
}

template<class K, class V>
class flat_map
{
public:

    typedef std::pair<K, V> value_type;
    typedef K key_type;
    typedef V mapped_type;

private:
    std::vector<value_type> m_data;

    struct compare_elems
    {
        bool operator()(const value_type& x, const value_type& y)
        {
            return x.first < y.first;
        }

        bool operator()(const value_type& x, key_type y)
        {
            return x.first < y;
        }

        bool operator()(key_type x, const value_type& y)
        {
            return x < y.first;
        }
    };

public:
    typedef typename std::vector<value_type>::iterator iterator;
    typedef typename std::vector<value_type>::const_iterator const_iterator;
    typedef typename std::vector<value_type>::reverse_iterator reverse_iterator;
    typedef typename std::vector<value_type>::const_reverse_iterator const_reverse_iterator;

    flat_map(std::size_t n = 64) { m_data.reserve(n); }


    std::pair<iterator, bool> insert(const value_type& value)//(const K& key, const V& v)
    {
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), value.first, compare_elems());
        if (it == m_data.end() || it->first != value.first)
        {
            iterator it2 = m_data.insert(it, value);
            return std::make_pair(it2, true);
        }
        else
        {
            //it->second = value.second;
            return std::make_pair(it, false);
        }
    }

    V& operator[](const K& key)
    {
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (it == m_data.end() || it->first != key)
        {
            iterator it2 = m_data.insert(it, value_type(key, V()));
            return it2->second;
        }
        else
        {
            return it->second;
        }
    }

    V& at(const K& key)
    {
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (it == m_data.end() || it->first != key)
        {
            throw make_out_of_range("flat_map invalid key: ", key);
        }
        else
        {
            return it->second;
        }
    }

    const V& at(const K& key) const
    {
        const_iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (it == m_data.end() || it->first != key)
        {
            throw make_out_of_range("flat_map invalid key: ", key);
        }
        else
        {
            return it->second;
        }
    }

    void swap(flat_map& m)
    {
        m_data.swap(m.m_data);
    }

    iterator find(const K& key)
    {
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (it == m_data.end() || it->first != key)
        {
            return m_data.end();
        }
        else
        {
            return it;
        }
    }

    const_iterator find(const K& key) const
    {
        const_iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (it == m_data.end() || it->first != key)
        {
            return m_data.end();
        }
        else
        {
            return it;
        }
    }

    void erase(const K& key)
    {
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (it != m_data.end() && it->first == key)
        {
            m_data.erase(it);
        }
    }

    void clear()
    {
        m_data.clear();
    }

    bool empty() const
    {
        return m_data.empty();
    }

    std::size_t size() const { return m_data.size(); }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    reverse_iterator rbegin() { return m_data.rbegin(); }
    reverse_iterator rend() { return m_data.rend(); }

    const_reverse_iterator rbegin() const { return m_data.rbegin(); }
    const_reverse_iterator rend() const { return m_data.rend(); }
};

template<class V, class K = unsigned>
class flat_index_map
{
public:

    typedef std::pair<K, V> value_type;
    typedef K key_type;
    typedef V mapped_type;

private:
    std::vector<value_type> m_data;
    std::vector<bool> m_bits;

    struct compare_elems
    {
        bool operator()(const value_type& x, const value_type& y)
        {
            return x.first < y.first;
        }

        bool operator()(const value_type& x, key_type y)
        {
            return x.first < y;
        }

        bool operator()(key_type x, const value_type& y)
        {
            return x < y.first;
        }
    };

public:
    typedef typename std::vector<value_type>::iterator iterator;
    typedef typename std::vector<value_type>::const_iterator const_iterator;
    typedef typename std::vector<value_type>::reverse_iterator reverse_iterator;
    typedef typename std::vector<value_type>::const_reverse_iterator const_reverse_iterator;

    flat_index_map(std::size_t n = 0) :m_bits(n) { }
    void resize(std::size_t  n) { m_bits.resize(n); }

    bool insert(const value_type& value)//(const K& key, const V& v)
    {
        if (m_bits[value.first])
            return false;
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), value.first, compare_elems());
        iterator it2 = m_data.insert(it, value);
        m_bits[value.first] = true;
        return true;
    }

    V& operator[](const K& key)
    {
        iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
        if (!m_bits[key])
        {
            iterator it2 = m_data.insert(it, value_type(key, V()));
            m_bits[key] = true;
            return it2->second;
        }
        else
        {
            return it->second;
        }
    }

    V& at(const K& key)
    {
        if (m_bits[key])
        {
            iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
            return it->second;
        }
        throw make_out_of_range("flat_index_map invalid key: ", key);
    }

    const V& at(const K& key) const
    {
        if (m_bits[key])
        {
            const_iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
            return it->second;
        }
        throw make_out_of_range("flat_index_map invalid key: ", key);
    }

    void swap(flat_index_map& m)
    {
        m_data.swap(m.m_data);
        m_bits(m.m_bits);
    }

    iterator find(const K& key)
    {
        if (m_bits[key])
        {
            iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
            return it;
        }
        else
        {
            return m_data.end();
        }
    }

    const_iterator find(const K& key) const
    {
        if (m_bits[key])
        {
            const_iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
            return it;
        }
        else
        {
            return m_data.cend();
        }
    }

    void erase(const K& key)
    {
        if (m_bits[key])
        {
            iterator it = std::lower_bound(m_data.begin(), m_data.end(), key, compare_elems());
            m_data.erase(it);
            m_bits[key] = false;
        }
    }

    void clear()
    {
        m_data.clear();
        std::fill(m_bits.begin(), m_bits.end(), false);
    }

    bool empty() const
    {
        return m_data.empty();
    }

    std::size_t size() const { return m_data.size(); }

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    reverse_iterator rbegin() { return m_data.rbegin(); }
    reverse_iterator rend() { return m_data.rend(); }

    const_reverse_iterator rbegin() const { return m_data.rbegin(); }
    const_reverse_iterator rend() const { return m_data.rend(); }
};


///
/// The unordered sparse set is slower than sparse set, except for iteration over the whole set of values.
/// It uses more memory than sparse set.
///
template <class V>
class unordered_index_map
{
public:
    typedef unsigned key_type;
    typedef V mapped_type;
    typedef std::pair<unsigned, V> value_type;

private:
    static constexpr unsigned EmptyElem = -1;
    typedef std::vector<unsigned> direct_access_sequence;
    typedef std::vector<value_type>  iteration_sequence;

    direct_access_sequence m_sparse;
    iteration_sequence m_dense;

    struct compare_elems
    {
        bool operator()(const value_type& x, const value_type& y)
        {
            return x.first < y.first;
        }

        bool operator()(const value_type& x, key_type y)
        {
            return x.first < y;
        }

        bool operator()(key_type x, const value_type& y)
        {
            return x < y.first;
        }
    };

    bool test(std::size_t i) const
    {
        return m_sparse[i] != EmptyElem;
    }

public:
    typedef std::size_t size_type;
    typedef typename iteration_sequence::iterator iterator;
    typedef typename iteration_sequence::const_iterator const_iterator;

    typedef typename iteration_sequence::reverse_iterator reverse_iterator;
    typedef typename iteration_sequence::const_reverse_iterator const_reverse_iterator;


    unordered_index_map(std::size_t size) :m_sparse(size), m_dense()
    {
        std::fill(m_sparse.begin(), m_sparse.end(), EmptyElem);
    }

    unordered_index_map() :m_sparse(), m_dense() {}

    void resize(std::size_t size)
    {
        m_sparse.resize(size);
        m_dense.clear();
        std::fill(m_sparse.begin(), m_sparse.end(), EmptyElem);
    }

    void swap(unordered_index_map& s)
    {
        m_sparse.swap(s.m_sparse);
        m_dense.swap(s.m_dense);
    }

    std::pair<iterator, bool> insert(const value_type& v)
    {
        if (m_sparse[v.first] != EmptyElem)
        {
            //m_dense[m_sparse[v.first]].second = v.second;
            return std::make_pair(m_dense.begin() + m_sparse[v.first], false);
        }
        m_sparse[v.first] = m_dense.size();
        m_dense.push_back(std::move(v));
        return std::make_pair(m_dense.end() - 1, true);
    }

    V& operator[](key_type key)
    {
        if (m_sparse[key] != EmptyElem)
        {
            return m_dense[m_sparse[key]].second;
        }
        m_sparse[key] = m_dense.size();
        m_dense.push_back(value_type(key, V()));
        return m_dense.back().second;
    }

    V& at(key_type key)
    {
        if (m_sparse[key] != EmptyElem)
        {
            return m_dense[m_sparse[key]].second;
        }
        throw make_out_of_range("unordered_index_map invalid key: ", key);
    }

    const V& at(key_type key) const
    {
        if (m_sparse[key] != EmptyElem)
        {
            return m_dense[m_sparse[key]].second;
        }
        throw make_out_of_range("unordered_index_map invalid key: ", key);
    }

    void erase(key_type i)
    {
        if (m_sparse[i] == EmptyElem)
            return;
        unsigned p_index = m_sparse[i];
        value_type* p_last_element = &m_dense.back();
        unsigned j = p_last_element->first;
        if (i != j)
        {
            std::swap(m_dense[p_index], m_dense[m_sparse[j]]);
            m_sparse[j] = p_index;
        }
        m_sparse[i] = EmptyElem;
        m_dense.erase(m_dense.end() - 1);
    }

    void clear()
    {
        std::fill(m_sparse.begin(), m_sparse.end(), nullptr);
        m_dense.clear();
    }

    std::size_t capacity() const
    {
        return m_sparse.size();
    }

    std::size_t size() const
    {
        return m_dense.size();
    }

    bool empty() const
    {
        return m_dense.empty();
    }

    iterator begin()
    {
        return m_dense.begin();
    }

    iterator end()
    {
        return m_dense.end();
    }

    reverse_iterator rbegin()
    {
        return m_dense.rbegin();
    }

    reverse_iterator rend()
    {
        return m_dense.rend();
    }

    const_iterator begin() const
    {
        return m_dense.begin();
    }

    const_iterator end() const
    {
        return m_dense.end();
    }

    const_reverse_iterator rbegin() const
    {
        return m_dense.rbegin();
    }

    const_reverse_iterator rend() const
    {
        return m_dense.rend();
    }

    iterator find(key_type i)
    {
        unsigned j = m_sparse[i];
        if (j != EmptyElem)
        {
            return m_dense.begin() + j;
        }
        return m_dense.end();
    }

    iterator lower_bound(key_type i)
    {
        return std::lower_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }

    iterator upper_bound(key_type i)
    {
        return std::upper_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }

    const_iterator find(key_type i) const
    {
        unsigned j = m_sparse[i];
        if (j != EmptyElem)
        {
            return m_dense.begin() + j;
        }
        return m_dense.end();
    }

    const_iterator lower_bound(key_type i) const
    {
        return std::lower_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }

    const_iterator upper_bound(key_type i) const
    {
        return std::upper_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }
};

///
/// The unordered sparse set is slower than sparse set, except for iteration over the whole set of values.
/// It uses more memory than sparse set.
///
template <class V>
class sparse_map
{
public:
    typedef unsigned key_type;
    typedef V mapped_type;
    typedef std::pair<unsigned, V> value_type;

private:
    //static constexpr unsigned EmptyElem = -1;
    typedef typename std::list<value_type>::iterator dense_iterator;
    typedef std::vector<dense_iterator> direct_access_sequence;
    typedef std::list<value_type>  iteration_sequence;

    direct_access_sequence m_sparse;
    iteration_sequence m_dense;
    dense_iterator dense_null = m_dense.end();

    struct compare_elems
    {
        bool operator()(const value_type& x, const value_type& y)
        {
            return x.first < y.first;
        }

        bool operator()(const value_type& x, key_type y)
        {
            return x.first < y;
        }

        bool operator()(key_type x, const value_type& y)
        {
            return x < y.first;
        }
    };

    bool test(std::size_t i) const
    {
        return m_sparse[i] != dense_null;
    }

public:
    typedef std::size_t size_type;
    typedef typename iteration_sequence::iterator iterator;
    typedef typename iteration_sequence::const_iterator const_iterator;

    typedef typename iteration_sequence::reverse_iterator reverse_iterator;
    typedef typename iteration_sequence::const_reverse_iterator const_reverse_iterator;


    sparse_map(std::size_t size) :m_sparse(size), m_dense()
    {
        std::fill(m_sparse.begin(), m_sparse.end(), dense_null);
    }

    sparse_map() :m_sparse(), m_dense() {}

    void resize(std::size_t size)
    {
        m_sparse.resize(size);
        m_dense.clear();
        std::fill(m_sparse.begin(), m_sparse.end(), dense_null);
    }

    void swap(sparse_map& s)
    {
        m_sparse.swap(s.m_sparse);
        m_dense.swap(s.m_dense);
    }

    std::pair<dense_iterator, bool> insert(const value_type& v)
    {
        if (m_sparse[v.first] != dense_null)
        {
            return std::pair<dense_iterator, bool>(m_sparse[v.first], false);
        }

        auto it1 = std::find_if(m_sparse.begin() + v.first, m_sparse.end(), [this](const dense_iterator& x) { return x != dense_null; });
        if (it1 == m_sparse.end())
        {
            m_dense.push_back(v);
            m_sparse[v.first] = std::prev(m_dense.end());
        }
        else
        {
            auto it = *it1;
            auto it2 = m_dense.insert(it, v);
            m_sparse[v.first] = it2;
        }

        return std::pair<dense_iterator, bool>(m_sparse[v.first], true);
    }

    V& operator[](key_type key)
    {
        if (m_sparse[key] != dense_null)
        {
            return m_sparse[key]->second;
        }

        auto v = value_type(key, V());
        auto it1 = std::find_if(m_sparse.begin() + v.first, m_sparse.end(), [this](const dense_iterator& x) { return x != dense_null; });
        if (it1 == m_sparse.end())
        {
            m_dense.push_back(v);
            m_sparse[v.first] = std::prev(m_dense.end());
            return m_sparse[v.first]->second;
        }
        else
        {
            auto it = *it1;
            //auto it = std::find_if(m_dense.begin(), m_dense.end(), [&v](const value_type& v1)->bool { return v1.first == v.first; });
            auto it2 = m_dense.insert(it, v);
            m_sparse[v.first] = it2;
            return m_sparse[v.first]->second;
        }
    }

    V& at(key_type key)
    {
        if (m_sparse[key] != dense_null)
        {
            return m_sparse[key]->second;
        }
        throw make_out_of_range("sparse_map invalid key: ", key);
    }

    const V& at(key_type key) const
    {
        if (m_sparse[key] != dense_null)
        {
            return m_sparse[key]->second;
        }
        throw make_out_of_range("sparse_map invalid key: ", key);
    }

    void erase(key_type i)
    {
        if (m_sparse[i] == dense_null)
            return;
        m_dense.erase(m_sparse[i]);
        m_sparse[i] = dense_null;
    }

    void clear()
    {
        std::fill(m_sparse.begin(), m_sparse.end(), dense_null);
        m_dense.clear();
    }

    std::size_t capacity() const
    {
        return m_sparse.size();
    }

    std::size_t size() const
    {
        return m_dense.size();
    }

    bool empty() const
    {
        return m_dense.empty();
    }

    iterator begin()
    {
        return m_dense.begin();
    }

    iterator end()
    {
        return m_dense.end();
    }

    reverse_iterator rbegin()
    {
        return m_dense.rbegin();
    }

    reverse_iterator rend()
    {
        return m_dense.rend();
    }

    const_iterator begin() const
    {
        return m_dense.begin();
    }

    const_iterator end() const
    {
        return m_dense.end();
    }

    const_reverse_iterator rbegin() const
    {
        return m_dense.rbegin();
    }

    const_reverse_iterator rend() const
    {
        return m_dense.rend();
    }

    iterator find(key_type i)
    {
        return m_sparse[i];
    }

    iterator lower_bound(key_type i)
    {
        return std::lower_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }

    iterator upper_bound(key_type i)
    {
        return std::upper_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }


    const_iterator find(key_type i) const
    {
        return m_sparse[i];
    }


    const_iterator lower_bound(key_type i) const
    {
        return std::lower_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }

    const_iterator upper_bound(key_type i) const
    {
        return std::upper_bound(m_dense.begin(), m_dense.end(), i, compare_elems());
    }
};

template <class V>
class two_level_map
{
public:
    typedef std::pair<unsigned, V> value_type;
    typedef unsigned key_type;
    typedef size_t size_type;
    typedef V mapped_type;
private:
    //static constexpr unsigned EmptyIndex = -1;
    //typedef std::size_t base_type;
    static constexpr unsigned shift = 12;
    static constexpr unsigned level2_max_size = 4096;
    static constexpr unsigned one_bit = 1;

    unsigned m_size;
    //typedef std::vector<value_type> level2_type;
    typedef flat_map<unsigned, V> level2_type;
    //typedef index_map<level2_type> level1_type;
    typedef flat_map<unsigned, level2_type> level1_type;
    level1_type m_level1;
    //std::vector<value_type> m_level2;

    struct compare_elems
    {
        bool operator()(const value_type& x, const value_type& y)
        {
            return x.first < y.first;
        }

        bool operator()(const value_type& x, key_type y)
        {
            return x.first < y;
        }

        bool operator()(key_type x, const value_type& y)
        {
            return x < y.first;
        }
    };

    bool test(unsigned i)
    {
        //return ((m_bit_array[i >> unsigned_bits_log2] >> (i & unsigned_bits_log2_mask)) & 1) != 0;
        unsigned level1_index = i >> shift;
        //const level2_type& level2 = m_level1.find(level1_index);
        auto it1 = m_level1.find(level1_index);
        if (it1 == m_level1.end())
        {
            return false;
        }
        level2_type& level2 = it1->second;
        auto it = level2.find(i);//std::lower_bound(level2.begin(), level2.end(), i, compare_elems());
        if (it != level2.end())
        {
            return true;
        }
        return false;
    }

public:


    two_level_map(std::size_t size) :m_size(size)//,
        //m_level1((m_size + level2_max_size - 1) >> shift)
    {
    }

    two_level_map() :m_size(0), m_level1(0)
    {
    }

    void swap(two_level_map& s)
    {
        m_level1.swap(s.m_level1);
        std::swap(m_size, s.m_size);
    }

    void resize(std::size_t size)
    {
        //m_level1.reserve((m_size + level2_max_size - 1) >> shift);
        m_size = size;
    }

    bool insert(const value_type& value)
    {
        unsigned i = value.first;
        unsigned level1_index = i >> shift;
        level2_type& level2 = m_level1[level1_index];
        /*
        auto it = std::lower_bound(level2.begin(), level2.end(), i, compare_elems());
        if (it != level2.end() && it->first == i)
        {
        return false;
        }
        */
        return level2.insert(value).second;
        //return true;
    }

    V& operator[](key_type i)
    {
        unsigned level1_index = i >> shift;
        level2_type& level2 = m_level1[level1_index];
        return level2[i];
    }

    V& at(key_type i)
    {
        unsigned level1_index = i >> shift;
        auto it = m_level1.find(level1_index);
        if (it != m_level1.end())
        {
            level2_type& level2 = it->second;
            auto it2 = level2.find(i);
            if (it2 != level2.end())
            {
                return it2->second;
            }
        }
        throw make_out_of_range("two_level_map invalid key: ", i);
    }

    const V& at(key_type i) const
    {
        unsigned level1_index = i >> shift;
        auto it = m_level1.find(level1_index);
        if (it != m_level1.end())
        {
            level2_type& level2 = it->second;
            auto it2 = level2.find(i);
            if (it2 != level2.end())
            {
                return it2->second;
            }
        }
        throw make_out_of_range("two_level_map invalid key: ", i);
    }

    void erase(key_type i)
    {
        unsigned level1_index = i >> shift;
        auto it = m_level1.find(level1_index);
        if (it != m_level1.end())
        {
            level2_type& level2 = it->second;
            auto it2 = level2.find(i);
            if (it2 != level2.end())
            {
                level2.erase(it2->first);
            }
            if (level2.empty())
            {
                m_level1.erase(level1_index);
            }
        }
    }

    struct iterator
    {
        friend two_level_map;
        typedef std::forward_iterator_tag
            iterator_category;
        //typedef  value_type;
        typedef std::ptrdiff_t
            difference_type;

    private:

        bool next()
        {
            ++m_level2_it;
            if (m_level2_it == m_level2_stopIt)
            {
                bool ok = false;
                if (++m_level1_it != m_level1_stopIt)
                {
                    //if (!m_level1_it->second.empty())
                    {
                        m_level2_it = m_level1_it->second.begin();
                        m_level2_stopIt = m_level1_it->second.end();
                        return true;
                    }
                }
                m_empty = true;
                return false;
            }
            return true;
        }

        iterator(level1_type& level1, unsigned i, bool low_bnd) : m_empty(false)
        {
            unsigned level1_index = i >> shift;
            auto it1 = level1.find(level1_index);
            level2_type* p_level2 = nullptr;
            if (it1 != level1.end())
            {
                p_level2 = &(it1->second);
                
                typename level2_type::iterator it;
                
                if (low_bnd)
                    it = std::lower_bound(p_level2->begin(), p_level2->end(), i, compare_elems());
                else
                    it = std::upper_bound(p_level2->begin(), p_level2->end(), i, compare_elems());

                if (it != p_level2->end())
                {
                    m_level2_it = it;
                    m_level2_stopIt = p_level2->end();
                }
                else
                {
                    if (++m_level1_it != m_level1_stopIt)
                    {
                        m_level2_it = m_level1_it->second.begin();
                        m_level2_stopIt = m_level1_it->second.end();
                        return;
                    }
                    m_empty = true;
                }
            }
            else
            {
                m_empty = true;
            }
        }

        iterator(level1_type& level1, unsigned i) : m_empty(false) // used for find!!! 
        {
            unsigned level1_index = i >> shift;
            m_level1_it = level1.find(level1_index);
            if (m_level1_it != level1.end())
            {                
                level2_type& level2 = m_level1_it->second;
                auto it = level2.find(i);
                if (it != level2.end())
                {
                    m_level1_stopIt = level1.end();
                    m_level2_it = it;
                    m_level2_stopIt = level2.end();
                    return;
                }
            }
            m_empty = true;
        }

    public:

        iterator(level1_type& level1) :m_empty(false)
        {
            m_level1_it = level1.begin();
            m_level1_stopIt = level1.end();
            if (!m_level1_it->second.empty())
            {
                m_level2_it = m_level1_it->second.begin();
                m_level2_stopIt = m_level1_it->second.end();
            }
            else
            {
                if (++m_level1_it != m_level1_stopIt)
                {
                    //if (!m_level1_it->second.empty())
                    {
                        m_level2_it = m_level1_it->second.begin();
                        m_level2_stopIt = m_level1_it->second.end();
                        return;
                    }
                }
                m_empty = true;
            }
        }

        iterator() : m_empty(true)
        {}

        value_type& operator*() const
        {
            return *m_level2_it;
        }

        value_type* operator->() const
        {
            return &*m_level2_it;
        }

        iterator& operator++()
        {
            next();
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp(*this);
            ++*this;
            return tmp;
        }

        bool operator==(const iterator& y)
        {
            if (m_empty || y.m_empty)
                return m_empty && y.m_empty;
            return (m_level1_it == y.m_level1_it) && (m_level2_it == y.m_level2_it);
        }

        bool operator!=(const iterator& y)
        {
            if (m_empty || y.m_empty)
                return m_empty != y.m_empty;

            return (m_level1_it != y.m_level1_it) || (m_level2_it != y.m_level2_it);
        }

    private:
        bool m_empty;
        typename level1_type::iterator m_level1_it;
        typename level1_type::iterator m_level1_stopIt;
        typename level2_type::iterator m_level2_it;
        typename level2_type::iterator m_level2_stopIt;


    };

    //typedef iterator const_iterator;

    iterator begin()
    {
        return iterator(m_level1);
    }

    iterator end()
    {
        return iterator();
    }

    iterator find(unsigned i)
    {        
        return iterator(m_level1, i);     
    }

    iterator lower_bound(unsigned i)
    {
        return iterator(m_level1, i, true);
    }

    iterator upper_bound(unsigned i)
    {
        if (i + 1 >= m_size)
            return iterator();
        return iterator(m_level1, i, false);
    }

    void erase(const iterator& it)
    {
        erase(*it);
    }

    bool empty() const
    {
        for (auto& x : m_level1)
        {
            if (!x.empty())
                return false;
        }
        return true;
    }

    void clear()
    {
        for (auto& x : m_level1)
        {
            x.clear();
        }
    }

    std::size_t capacity() const
    {
        return m_size;
    }

    std::size_t size() const
    {
        std::size_t count = 0;
        for (auto& x : m_level1)
        {
            count += x.second.size();
        }
        return count;
    }
}; // two_level_map
#endif