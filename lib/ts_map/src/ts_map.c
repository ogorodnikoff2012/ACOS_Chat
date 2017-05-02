#include <ts_map.h>
#include <stdlib.h>
#include <stdbool.h>

#define LOCK pthread_mutex_lock((pthread_mutex_t *)&m->mutex)
#define UNLOCK pthread_mutex_unlock((pthread_mutex_t *)&m->mutex)

/* Static functions */

static void ts_map_node_destroy(ts_map_node_t *node, void (* destructor)(void *)) {
    if (node != NULL) {
        ts_map_node_destroy(node->left, destructor);
        ts_map_node_destroy(node->right, destructor);
        if (destructor != NULL) {
            destructor(node->val);
        }
        free(node);
    }
}

static size_t get_size(ts_map_node_t *node) {
    return node == NULL ? 0 : node->size;
}

static void update_size(ts_map_node_t *node) {
    if (node != NULL) {
        node->size = 1 + get_size(node->left) + get_size(node->right);
    }
}

static bool is_root(ts_map_node_t *node) {
    return node->parent == NULL;
}

static bool is_left_son(ts_map_node_t *node) {
    return node->parent != NULL && node->parent->left == node;
}

static bool is_right_son(ts_map_node_t *node) {
    return node->parent != NULL && node->parent->right == node;
}

static void connect_left(ts_map_node_t *parent, ts_map_node_t *son) {
    if (parent != NULL) {
        parent->left = son;
    }
    if (son != NULL) {
        son->parent = parent;
    }
}

static void connect_right(ts_map_node_t *parent, ts_map_node_t *son) {
    if (parent != NULL) {
        parent->right = son;
    }
    if (son != NULL) {
        son->parent = parent;
    }
}

static void left_turn(ts_map_node_t *node) {
    ts_map_node_t *new_root = node->right;
    if (new_root == NULL) {
        return;
    }

    ts_map_node_t *parent = node->parent;
    bool node_left = is_left_son(node);

    connect_right(node, new_root->left);
    connect_left(new_root, node);

    update_size(node);
    update_size(new_root);

    if (parent != NULL) {
        if (node_left) {
            connect_left(parent, new_root);
        } else {
            connect_right(parent, new_root);
        }
    } else {
        new_root->parent = NULL;
    }
}

static void right_turn(ts_map_node_t *node) {
    ts_map_node_t *new_root = node->left;
    if (new_root == NULL) {
        return;
    }

    ts_map_node_t *parent = node->parent;
    bool node_left = is_left_son(node);

    connect_left(node, new_root->right);
    connect_right(new_root, node);

    update_size(node);
    update_size(new_root);

    if (parent != NULL) {
        if (node_left) {
            connect_left(parent, new_root);
        } else {
            connect_right(parent, new_root);
        }
    } else {
        new_root->parent = NULL;
    }
}

static ts_map_node_t *splay(ts_map_node_t *node) {
    while (!is_root(node)) {
        if (is_root(node->parent)) {
            if (is_left_son(node)) {
                right_turn(node->parent);
            } else {
                left_turn(node->parent);
            }
            continue;
        }

        ts_map_node_t *p = node->parent, *g = node->parent->parent;
        bool node_left = is_left_son(node);
        bool p_left = is_left_son(p);

        if (node_left) {
            if (p_left) {
                right_turn(g);
                right_turn(p);
            } else {
                right_turn(p);
                left_turn(g);
            }
        } else {
            if (p_left) {
                left_turn(p);
                right_turn(g);
            } else {
                left_turn(g);
                left_turn(p);
            }
        }
    }
    return node;
}

static ts_map_node_t *find(ts_map_node_t *root, uint64_t key) {
    if (root == NULL) {
        return NULL;
    }
    if (key == root->key) {
        splay(root);
        return root;
    }
    if (key < root->key) {
        if (root->left == NULL) {
            splay(root);
            return root;
        }
        return find(root->left, key);
    } else {
        if (root->right == NULL) {
            splay(root);
            return root;
        }
        return find(root->right, key);
    }
}

static ts_map_node_t *leftest(ts_map_node_t *node) {
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

static ts_map_node_t *rightest(ts_map_node_t *node) {
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}

static ts_map_node_t *merge(ts_map_node_t *left, ts_map_node_t *right) {
    if (left == NULL) {
        return right;
    }
    if (right == NULL) {
        return left;
    }
    ts_map_node_t *tree = splay(rightest(left));
    connect_right(tree, right);
    update_size(tree);
    return tree;
}

static void split(ts_map_node_t *tree, uint64_t key,
                  ts_map_node_t **left, ts_map_node_t **right) {
    if (tree == NULL) {
        *left = *right = NULL;
        return;
    }
    tree = find(tree, key);
    if (tree->key < key) {
        *left = tree;
        *right = tree->right;
        if (tree->right != NULL) {
            tree->right->parent = NULL;
            tree->right = NULL;
        }
    } else {
        *right = tree;
        *left = tree->left;
        if (tree->left != NULL) {
            tree->left->parent = NULL;
            tree->left = NULL;
        }
    }
    update_size(*left);
    update_size(*right);
}

static ts_map_node_t *add(ts_map_node_t *tree, uint64_t key, void *val) {
    if (tree == NULL) {
        ts_map_node_t *node = calloc(1, sizeof(ts_map_node_t));
        node->left = node->right = node->parent = NULL;
        node->key = key;
        node->val = val;
        node->size = 1;
        return node;
    }

    if ((tree = find(tree, key))->key == key) {
        return tree;
    }

    ts_map_node_t *root = calloc(1, sizeof(ts_map_node_t));
    split(tree, key, &root->left, &root->right);
    
    root->key = key;
    root->parent = NULL;
    root->val = val;
    
    connect_left(root, root->left);
    connect_right(root, root->right);
    update_size(root);
    return root;
}

static ts_map_node_t *remove(ts_map_node_t *tree, uint64_t key, void **val) {
    if (tree == NULL) {
        *val = NULL;
        return NULL;
    }

    find(tree, key);
    if (tree->key != key) {
        *val = NULL;
        return tree;
    }

    ts_map_node_t *left = tree->left, *right = tree->right;
    *val = tree->val;
    free(tree);

    return merge(left, right);
}

/* Global functions */

void ts_map_init(volatile ts_map_t *m) {
    pthread_mutex_init((pthread_mutex_t *)&m->mutex, NULL);
    
    LOCK;
    m->root = NULL;
    m->frosen = false;
    UNLOCK;
}

void ts_map_destroy(volatile ts_map_t *m, void (* destructor)(void *)) {
    LOCK;
    ts_map_node_destroy(m->root, destructor);
    m->frosen = true;
    UNLOCK;

    pthread_mutex_destroy((pthread_mutex_t *)&m->mutex);
}

bool ts_map_insert(volatile ts_map_t *m, uint64_t key, void *val) {
    bool success;
    LOCK;
    if (m->frosen) {
        success = false;
    } else {
        m->root = add(m->root, key, val);
        success = true;
    }
    UNLOCK;
    return success;
}

void *ts_map_find(volatile ts_map_t *m, uint64_t key) {
    void *ans = NULL;
    LOCK;
    m->root = find(m->root, key);
    if (m->root != NULL && m->root->key == key) {
        ans = m->root->val;
    }
    UNLOCK;
    return ans;
}

void *ts_map_erase(volatile ts_map_t *m, uint64_t key) {
    void *val = NULL;
    LOCK;
    if (!m->frosen) {
        m->root = remove(m->root, key, &val);
    }
    UNLOCK;
    return val;
}

size_t ts_map_size(volatile ts_map_t *m) {
    size_t ans;
    LOCK;
    ans = get_size(m->root);
    UNLOCK;
    return ans;
}

static void ts_map_forall_helper(ts_map_node_t *m, void *ptr, void (* callback)(uint64_t key, void *value, void *ptr)) {
    if (m == NULL) {
        return;
    }
    ts_map_forall_helper(m->left, ptr, callback);
    callback(m->key, m->val, ptr);
    ts_map_forall_helper(m->right, ptr, callback);
} 

void ts_map_forall(volatile ts_map_t *m, void *ptr, void (* callback)(uint64_t key, void *value, void *ptr)) {
    LOCK;
    ts_map_forall_helper(m->root, ptr, callback);
    UNLOCK;
}
