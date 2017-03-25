#include <ts_map/ts_map.h>
#include <stdio.h>
#include <stdlib.h>

void print_tree(ts_map_node_t *node) {
    static int indent = 0;
    if (node == NULL) {
        return;
    }
    ++indent;
    print_tree(node->left);
    
    fprintf(stderr, "%*c%p:\n", indent, ' ', node);
    fprintf(stderr, "%*cleft = %p:\n", indent, ' ', node->left);
    fprintf(stderr, "%*cright = %p:\n", indent, ' ', node->right);
    fprintf(stderr, "%*cparent = %p:\n", indent, ' ', node->parent);
    fprintf(stderr, "%*ckey = %d:\n", indent, ' ', node->key);
    fprintf(stderr, "%*cvalue = %d:\n", indent, ' ', *(int *)node->val);
    
    print_tree(node->right);
    --indent;
}

int main() {
    ts_map_t map;
    ts_map_init(&map);

    char ch;
    while (scanf("%c", &ch) == 1) {
        int key, *val;
        switch (ch) {
            case 'a':
                val = malloc(sizeof(int));
                scanf("%d %d", &key, val);
                ts_map_insert(&map, key, val);
                break;
            case 'd':
                scanf("%d", &key);
                val = ts_map_find(&map, key);
                if (val != NULL) {
                    free(val);
                }
                ts_map_erase(&map, key);
                break;
            case '?':
                scanf("%d", &key);
                val = ts_map_find(&map, key);
                if (val != NULL) {
                    printf("%d\n", *val);
                } else {
                    printf("<NULL>\n");
                }
                break;
            default:
                if (ch > ' ') {
                    printf("Unknown action\n");
                } else {
                    continue;
                }
                break;
        }  
        print_tree(map.root);
        fprintf(stderr, "%*c\n", 40, ' ');
/*        fprintf(stderr, "%d\n", ch); */
    }

    ts_map_destroy(&map, free);
    return 0;
}
