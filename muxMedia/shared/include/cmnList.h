/*
* $Id: cmn_list.h,v 1.3 2007/03/20 21:19:12 lizhijie Exp $
*/

#ifndef __CMN_LIST_H__
#define __CMN_LIST_H__


/**
 * This is a very simple implementation of a linked list.
 * There is not much to say about it... Except that it could be a lot improved. Sadly, it would be difficult
 * to improve it without breaking the compatibility with older version!
 */
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __node __node_t;

struct __node
{
	void		*next;			/**< next __node_t containing element */
	void		*element;              /**< element in Current node */
};

typedef struct cmn_list cmn_list_t;

struct cmn_list
{
	int			nb_elt;         /**< Number of element in the list */
	__node_t		*node;     /**< Next node containing element  */
};

/**
 * NOTE: this element MUST be previously allocated. * @param li The element to initialise.
 */
int cmn_list_init (cmn_list_t * li);

/* iterate every element of list, when return value of iterateFunc is not 0, iteration ends and return that value
* If isContinue!=0, iteration go on to the last one no matter what the return value of iterateFun() is
* int (*iterateFunc) (int, void *ele, void *priv): int is index, ele is element, priv is private data for iterateFunc
*/
int cmn_list_iterate(cmn_list_t * li, int isContinue, int (*iterateFunc) (int, void *ele, void *priv), void *data);

/**
 * Free a list of element.
 * Each element will be free with the method given as the second parameter.
 * @param li The element to work on.
 * @param free_func The method that is able to release one element of the list.
 */
void cmn_list_special_free (cmn_list_t *li, void *(*free_func) (void *), int freeList);

/* Free a list of element where elements are pointer to 'char'.  @param li The element to work on. */
void cmn_list_ofchar_free (cmn_list_t * li, int freeList);

/* Get the size of a list of element. @param li The element to work on. */
int cmn_list_size (const cmn_list_t * li);

/**
 * Check if the end of list is detected .
 * @param li The element to work on.
 * @param pos The index of the possible element.
 */
int cmn_list_eol (const cmn_list_t * li, int pos);

/**
 * Add an element in a list.
 * @param li The element to work on.
 * @param element The pointer on the element to add.
 * @param pos the index of the element to add. (or -1 to append the element at the end)
 */
int cmn_list_add (cmn_list_t * li, void *element, int pos);

#define	cmn_list_append(li, element)	\
		cmn_list_add( (li), (element), -1 )

/*
 * Get an element from a list. @param li The element to work on. @param pos the index of the element to get.
 */
void *cmn_list_get (const cmn_list_t * li, int pos);

/**
 * Remove an element from a list.
 * @param li The element to work on.
 * @param pos the index of the element to remove.
 */
int cmn_list_remove (cmn_list_t * li, int pos);

#ifdef __cplusplus
}
#endif

#endif

