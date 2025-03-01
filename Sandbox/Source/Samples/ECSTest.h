#include <cstdint>
#include <vector>
#include <memory>
#include <tuple>
#include <utility>
#include <algorithm>

using entity = uint32_t;
constexpr entity null_entity = -1;

// Component type management
struct component_family {
    static size_t counter;
    template<typename T> static size_t id() {
        static size_t id = counter++;
        return id;
    }
};
size_t component_family::counter = 0;

// Base sparse set interface
struct sparse_set_base {
    virtual ~sparse_set_base() = default;
    virtual bool contains(entity e) const = 0;
    virtual void remove(entity e) = 0;
    virtual void clear() = 0;
};

// Component storage using sparse sets
template<typename T>
class sparse_set : public sparse_set_base {
public:
    std::vector<entity> sparse;
    std::vector<entity> dense;
    std::vector<T> components;

public:
    void add(entity e, T&& component) {
        if (contains(e)) {
            components[sparse[e]] = std::move(component);
            return;
        }
        
        if (e >= sparse.size()) {
            sparse.resize(e + 1, null_entity);
        }
        
        sparse[e] = dense.size();
        dense.push_back(e);
        components.push_back(std::move(component));
    }

    T& get(entity e) { return components[sparse[e]]; }
    const T& get(entity e) const { return components[sparse[e]]; }

    bool contains(entity e) const override {
        return e < sparse.size() && 
               sparse[e] != null_entity && 
               sparse[e] < dense.size() && 
               dense[sparse[e]] == e;
    }

    void remove(entity e) override {
        if (!contains(e)) return;

        const size_t index = sparse[e];
        const entity last = dense.back();

        // Swap with last element
        dense[index] = last;
        components[index] = std::move(components.back());
        sparse[last] = index;

        // Remove last element
        dense.pop_back();
        components.pop_back();

        // Invalidate sparse entry
        sparse[e] = null_entity;
    }

    void clear() override {
        sparse.clear();
        dense.clear();
        components.clear();
    }

    auto size() const { return dense.size(); }
    auto begin() const { return dense.begin(); }
    auto end() const { return dense.end(); }
};

// Registry class
class registry {
    std::vector<std::unique_ptr<sparse_set_base>> sparse_sets;
    entity next_entity = 0;

public:
    entity create() {
        if (next_entity == null_entity) throw std::runtime_error("Entity overflow");
        return next_entity++;
    }

    void destroy(entity e) {
        for (auto& set : sparse_sets) {
            if (set) set->remove(e);
        }
    }

    template<typename T, typename... Args>
    T& add(entity e, Args&&... args) {
        auto& set = get_set<T>();
        set.add(e, T{std::forward<Args>(args)...});
        return set.get(e);
    }

    template<typename T>
    void remove(entity e) {
        get_set<T>().remove(e);
    }

    template<typename T>
    T& get(entity e) {
        return get_set<T>().get(e);
    }

    template<typename T>
    bool has(entity e) {
        return get_set<T>().contains(e);
    }

    template<typename... Ts>
    auto View() {
        return view<sizeof...(Ts), Ts...>(get_set<Ts>()...);
    }

private:
    template<typename T>
    sparse_set<T>& get_set() {
        const size_t id = component_family::id<T>();
        if (id >= sparse_sets.size()) {
            sparse_sets.resize(id + 1);
        }
        if (!sparse_sets[id]) {
            sparse_sets[id] = std::make_unique<sparse_set<T>>();
        }
        return static_cast<sparse_set<T>&>(*sparse_sets[id]);
    }

    template<size_t N, typename... Ts>
    class view {
        std::tuple<sparse_set<Ts>&...> sets;
        using first_set = std::tuple_element_t<0, decltype(sets)>;

    public:
        view(sparse_set<Ts>&... sets) : sets(sets...) {}

        class iterator {
            size_t index;
            const first_set& main_set;
            std::tuple<sparse_set<Ts>&...> sets;

        public:
            iterator(size_t index, const first_set& main_set, std::tuple<sparse_set<Ts>&...> sets)
                : index(index), main_set(main_set), sets(sets) {}

            bool operator!=(const iterator& other) const { return index != other.index; }
            
            entity operator*() const { 
                return main_set.dense[index];
            }

            iterator& operator++() {
                while (++index < main_set.dense.size()) {
                    if (check_components()) break;
                }
                return *this;
            }

        private:
            bool check_components() const {
                const entity e = main_set.dense[index];
                return (std::get<sparse_set<Ts>&>(sets).contains(e) && ...);
            }
        };

        iterator begin() {
            size_t start = 0;
            while (start < std::get<0>(sets).dense.size()) {
                if (check_components(start)) break;
                start++;
            }
            return iterator(start, std::get<0>(sets), sets);
        }

        iterator end() {
            return iterator(std::get<0>(sets).dense.size(), std::get<0>(sets), sets);
        }

    private:
        bool check_components(size_t index) {
            const entity e = std::get<0>(sets).dense[index];
            return (std::get<sparse_set<Ts>&>(sets).contains(e) && ...);
        }
    };
};