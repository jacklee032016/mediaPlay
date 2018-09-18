/*
* llist.h - Header file for llist.c
*/

#ifndef __LLIST_CGI_H__
#define __LLIST_CGI_H__ 

typedef struct 
{
	char		*name;
	char		*value;

	char		*type;	/* added for PBX and Record config file which the second item is type, 'string', 'int' etc */
	char		*rule;	/* dial rule for ua,xfg or pbx.cfg */
} entrytype;

typedef struct _node 
{
	entrytype entry;
	struct _node* next;
} node;

typedef struct 
{
	node* head;
} llist;


#define	LIST_CREATE( list)	( (list)->head = NULL)

node* list_next(node* w);
short on_list_debug(llist *l, node* w);
void list_traverse(llist *l, void (*visit)(entrytype *item, void *), void *object);
node* list_append(llist* l,  entrytype* item);
node *list_append_raw(llist* l,  char *key, char *value, char *type, char *rule);
node* list_insafter(llist* l, node* w, entrytype item);
short list_delete_node(llist *l, node* w);
void list_clear(llist* l);
void list_extract_cmdline(entrytype *pen);

node* list_insbefore(llist* l, node* w, entrytype item);


short list_delete_by_name(llist *l, char  *name);

node *list_find_node_by_key(llist *list, char *key);
char *list_find_value_by_key(llist *list, char *key);
int list_replace_value_by_key(llist *list, char *key,char *newValue);

int list_length(llist *list);

/* from cgi_string.h */
char *cgi_new_string(char *str);
char *cgi_replace_ltgt(char *str);
char *cgi_lower_case(char *buffer);

#define	GET_VALUE(list, key)		\
	list_find_value_by_key( (list), (key) )


#endif

