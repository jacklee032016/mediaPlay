/* 
* cgi-llist.c - Minimal linked list library for revised CGI C library
* $Id: cgi_llist.c,v 1.4 2007/03/14 18:57:47 lizhijie Exp $
*/

#include <stdlib.h>
#include <string.h>


#include "libCgi.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


static node *__list_newnode(char *key, char *value,char *type, char *rule)
{
	node *newnode = (node *)malloc(sizeof(node));

	if(newnode==NULL)
	{
		printf("%s : malloc error!\n", __FUNCTION__);
		return NULL;
	}

	memset(newnode, 0, sizeof(node) );

	newnode->entry.name = cgi_new_string( key);
	newnode->entry.value = cgi_new_string( value);
	newnode->entry.type = cgi_new_string( type);
	newnode->entry.rule = cgi_new_string( rule);

	return newnode;	
}

void list_free_node(node *current)
{
	if (current->entry.name)
	{
		free(current->entry.name);
		current->entry.name = NULL;
	}
	if (current->entry.value)
	{
		free(current->entry.value);
		current->entry.value = NULL;
	}
	if (current->entry.type)
	{
		free(current->entry.type);
		current->entry.type = NULL;
	}
	if (current->entry.rule)
	{
		free(current->entry.rule);
		current->entry.rule = NULL;
	}
	free(current);
	current = NULL;
	return;
}

node* list_next(node* w)
{
	return w->next;
}

short on_list_debug(llist *l, node* w)
{
	node* current;

	if (w == 0)
		return 0;
	else 
	{
		current = l->head;
		while ( (current != w) && (current != 0) )
			current = current->next;
		if (current == w)
			return 1;
		else
			return 0;
	}
}

//~tangr: for the purpose of modification in llist when traverse, a change done
void list_traverse(llist *l, void (*visit)(entrytype *, void *), void *object)
{
	node* current;

	current = l->head;
	while (current != 0) 
	{
//		(*visit)(current->entry);
		(*visit)(&current->entry,object);
		current = current->next;
	}
}

node* list_insbefore(llist* l, node* w, entrytype item)
{
	node *newnode =  __list_newnode(item.name, item.value, item.type, item.rule );
	node *ppre=l->head;

	// insert node before w,first search point before w;
	while(ppre->next!=w)
			ppre=ppre->next;
	if(w==NULL)
		return NULL;

	if(l->head==0)
	{
		l->head=newnode;
		newnode->next=NULL;
	}
	else if(l->head==w)
	{
		newnode->next=w;
		l->head=newnode;
	}
	else
	{
		newnode->next = w;
		ppre->next = newnode;
	}
			
	return newnode;
}

node* list_insafter(llist* l, node* w, entrytype item)
{
	node* newnode;

#if 0
	cgidebug("name %s\r\n", (item.name)?item.name:"NULL" );
	cgidebug("value %s\n", (item.value)?item.value:"NULL" );
	cgidebug("type %s\r\n", (item.type!=NULL)?item.type:"NULL" );
	cgidebug("rule %s\r\n", (item.rule!=NULL)?item.rule:"NULL" );
#endif	
	newnode = __list_newnode(item.name, item.value, item.type, item.rule );

	if (l->head == 0) 
	{
		newnode->next = 0;
		l->head = newnode;
	}
	else if (w == NULL )
	{
    /* ERROR: can't insert item after w since w is not on l */
		exit(1);
	}
	else 
	{
    /* insert node after */
		if (newnode == 0) /* can assume that w != NULL */
		{
			/* ERROR: nothing to insert after */
			exit(1);
		}
		else 
		{
			newnode->next = w->next;
			w->next = newnode;
		}
	}

	return newnode;
}

node *list_append_raw(llist* l,  char *key, char *value,char *type, char *rule)
{
	node *w;
	node* newnode = __list_newnode(key, value, type, rule);
	
	w=l->head;
	if (w == 0)
	{
		l->head = newnode;
		return newnode;
	}

	while(w->next!=0)
	{
		w = w->next;
	} ;

	w->next = newnode;

	return newnode;
}

node* list_append(llist* l,  entrytype *item)
{
	return list_append_raw(l, item->name, item->value, item->type, item->rule );
}

short list_delete_node(llist *l, node* w)
{
	node	*current;
	node	*next,*pre;

	if (w == 0)
	    return 0;

	current = l->head;
	if (current == w)
	{
		next = current->next;
		list_free_node(w);
		w = next;
		l->head = next;
		return 1;
	}

	pre = current;
	current = current->next;
	while(current)
	{
		next =current->next;
		if(current == w)
		{
			list_free_node(w);
			w = next;
			pre->next = next;
			return 1;
		}
		pre=current;
		current = next;
	};

	return 0;
}


short list_delete_by_name(llist *l, char  *name)
{
	node	*n = list_find_node_by_key(l, name);
	return list_delete_node(l, n);
}

void list_clear(llist* l)
{
	node* nextnode;
	node* current;

	current = l->head;
	while( current) 
	{
		nextnode = current->next;
		list_free_node(current);
		current = nextnode;
	}

	l->head = NULL;
}

//to extract cmd name from a cmdline 
void list_extract_cmdline(entrytype *pen)
{
	char *p;
	
	p=strrchr(pen->name,'/');
	if(p)
	{
		p=cgi_new_string(p+1);
		free(pen->name);
		pen->name=p;
	}
}

int list_add_config_items(llist *list, conf_item_t *items, int size)
{
	int i =0;
	conf_item_t *it = items;
	if(!it )
		return -1;
	
	for(i=0; i< size; i++)
	{
		list_append_raw(list, it->key, it->value, NULL, NULL);
		it++;
	}	

	return 0;
}

node *list_find_node_by_key(llist *list, char *key)
{
	node* current;

	if(!key || strlen(key)==0 )
		return NULL;
	current = list->head;
	while (current != 0) 
	{
		if(!strcasecmp(current->entry.name, key) )
			return current;
		
		current = current->next;
	}
	return NULL;
}

char *list_find_value_by_key(llist *list, char *key)
{
	node *current;

	current = list_find_node_by_key(list, key);
	if(current) 
	{
		return current->entry.value;
	}

	return "";
}

int list_replace_value_by_key(llist *list, char *key,char *newValue)
{
	node* current;

	current = list->head;
	while (current != 0) 
	{
		if(!strcasecmp(current->entry.name, key) )
		{
			free(current->entry.value);
			current->entry.value = cgi_new_string(newValue);
			sprintf(current->entry.value, "%s", newValue);
			return 1;
		}
		current = current->next;
	}

	return 0;
}

/* replace value in rawList with the value in newList */
int list_replace_with_new_list(llist *rawList, llist *newList)
{
	node	*current;
	node	*tmp ;
	int		res = 0;

	if(!rawList || !newList )
		return -1;
	
	current = newList->head;
	while (current != 0) 
	{
		tmp = rawList->head;

		while(tmp)
		{
			if(!strcasecmp(current->entry.name, tmp->entry.name ) )
			{
				sprintf(tmp->entry.value, "%s", current->entry.value);
				if( strlen(current->entry.type) )
					sprintf(tmp->entry.type, "%s", current->entry.type);
				if( strlen(current->entry.rule) )
					sprintf(tmp->entry.rule, "%s", current->entry.rule);

				res = 1;
				break;
			}

			tmp = tmp->next;
		}
		current = current->next;
	}

	return res;
}

int list_length(llist *list)
{
	node	*current;
	int		length = 0;

	current = list->head;
	while (current != 0) 
	{
		length++;
		current = current->next;
	}

	return length;
}

