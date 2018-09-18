/*
* $Id: cmn_list.c,v 1.3 2007/03/20 21:19:12 lizhijie Exp $
*/

//#include <cmnList.h>
#include <cmnOsPort.h>

int cmn_list_init (cmn_list_t * li)
{
	li->nb_elt = 0;
	return 0;/* ok */
}

int cmn_list_iterate(cmn_list_t * li, int isContinue, int (*iterateFunc) (int, void *ele, void *priv), void *data)
{
	void *element;
	int res;
	int i = 0;

	if (li == NULL)
		return 0;
	for(i=0; i< cmn_list_size(li); i++)
	{
		element = (void *) cmn_list_get (li, i);
		res = (iterateFunc)(i, element, data);
		if( (res != 0) && (isContinue== 0) )
			return res;
	}

	return 0;
}

void cmn_list_special_free (cmn_list_t * li, void *(*free_func) (void *), int freeList)
{
	int pos = 0;
	void *element;

	if (li == NULL)
		return;
	while (!cmn_list_eol (li, pos))
	{
		element = (void *) cmn_list_get (li, pos);
		cmn_list_remove (li, pos);
		free_func (element);
	}
	
	if( freeList )
	{
		cmn_free (li);
	}
}

void cmn_list_ofchar_free (cmn_list_t * li, int freeList)
{
	int pos = 0;
	char *chain;
//	int i =0;

	if (li == NULL)
		return;
	while (!cmn_list_eol (li, pos))
	{
		chain = (char *) cmn_list_get (li, pos);
		if(chain==NULL)
			continue;
		
//		MUX_DEBUG("No.%d item is removed from list", ++i);
		cmn_list_remove (li, pos);
		cmn_free (chain);
	}

	if( freeList )
	{
		cmn_free (li);
	}
}

int cmn_list_size (const cmn_list_t * li)
{
	if (li != NULL)
		return li->nb_elt;
	else
		return -1;
}

int cmn_list_eol (const cmn_list_t * li, int i)
{
	if(li==NULL)
		return -1;
	if (i < li->nb_elt)
		return 0;			/* not end of list */
	return 1;			/* end of list */
}

/* index starts from 0; */
int cmn_list_add (cmn_list_t * li, void *el, int pos)
{
	__node_t *ntmp;
	int i = 0;

	if (pos == -1 || pos >= li->nb_elt)
	{/* insert at the end  */
		pos = li->nb_elt;
	}

	if (li->nb_elt == 0)
	{
		li->node = (__node_t *) cmn_malloc (sizeof (__node_t));
		li->node->element = el;
		li->nb_elt++;
		return li->nb_elt;
	}

	ntmp = li->node;		/* exist because nb_elt>0  */

	if (pos == 0)
	{
		li->node = (__node_t *) cmn_malloc (sizeof (__node_t));
		li->node->element = el;
		li->node->next = ntmp;
		li->nb_elt++;
		return li->nb_elt;
	}
	/* pos = 0 insert before first elt  */

	while (pos > i + 1)
	{
		i++;
		/* when pos>i next node exist  */
		ntmp = (__node_t *) ntmp->next;
	}

	/* if pos==nb_elt next node does not exist  */
	if (pos == li->nb_elt)
	{
		ntmp->next = (__node_t *) cmn_malloc (sizeof (__node_t));
		ntmp = (__node_t *) ntmp->next;
		ntmp->element = el;
		li->nb_elt++;
		return li->nb_elt;
	}

	/* here pos==i so next node is where we want to insert new node */
	{
		__node_t *nextnode = (__node_t *) ntmp->next;

		ntmp->next = (__node_t *) cmn_malloc (sizeof (__node_t));
		ntmp = (__node_t *) ntmp->next;
		ntmp->element = el;
		ntmp->next = nextnode;
		li->nb_elt++;
	}
	return li->nb_elt;
}

/* index starts from 0 */
void *cmn_list_get (const cmn_list_t * li, int pos)
{
	__node_t *ntmp;
	int i = 0;

	if (pos < 0 || pos >= li->nb_elt)/* element does not exist */
		return 0;

	ntmp = li->node;/* exist because nb_elt>0 */
	while (pos > i)
	{
		i++;
		ntmp = (__node_t *) ntmp->next;
	}
	return ntmp->element;
}

/* return -1 if failed */
int cmn_list_remove (cmn_list_t *li, int pos)
{
	__node_t *ntmp;
	int i = 0;

	if (pos < 0 || pos >= li->nb_elt)/* element does not exist */
		return -1;

	ntmp = li->node;		/* exist because nb_elt>0 */

	if ((pos == 0))
	{				/* special case  */
		li->node = (__node_t *) ntmp->next;
		li->nb_elt--;
		cmn_free (ntmp);
		return li->nb_elt;
	}

	while (pos > i + 1)
	{
		i++;
		ntmp = (__node_t *) ntmp->next;
	}

	/* insert new node */
	{
		__node_t *remnode;

		remnode = (__node_t *) ntmp->next;
		ntmp->next = ((__node_t *) ntmp->next)->next;
		cmn_free (remnode);
		li->nb_elt--;
	}
	return li->nb_elt;
}

