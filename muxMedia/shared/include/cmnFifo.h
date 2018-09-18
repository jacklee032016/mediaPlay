/*
* $Id: cmn_fifo.h,v 1.3 2007/04/03 17:55:11 lizhijie Exp $
*/

#ifndef __CMN_FIFO_H__
#define __CMN_FIFO_H__


/**
 * This is a very simple implementation of a fifo.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#define	DEFAULT_THREAD_QUEUE_LENGTH			100

typedef enum
{
	ok,
	plein,
	vide
}cmn_fifo_etat;


typedef struct cmn_fifo cmn_fifo_t;

struct cmn_fifo
{
	cmn_mutex_t			*qislocked;  /**@internal */
	cmn_sem_t			*qisempty;     /**@internal */

	cmn_list_t			*queue;            /**< list of nodes containing elements */
	int					nb_elt;                    /**< nb of elements */
	cmn_fifo_etat			etat;           /**@internal state of the fifo */

	int					maxSize;
};

/**
 * NOTE: this element MUST be previously allocated.
 * @param ff The element to initialise.
 */
void cmn_fifo_init (int _maxSize, cmn_fifo_t **ff);

void cmn_fifo_free (cmn_fifo_t * ff);

/**
 * Insert an element in a fifo (at the beginning).
 * @param ff The element to work on.
 * @param element The pointer on the element to insert.
 */
int cmn_fifo_insert (cmn_fifo_t * ff, void *element);

/**
 * Add an element in a fifo.
 * @param ff The element to work on.
 * @param element The pointer on the element to add.
 */
int cmn_fifo_add (cmn_fifo_t * ff, void *element);

/* Get the number of element in a fifo. * @param ff The element to work on. */
int cmn_fifo_size (cmn_fifo_t * ff);

/* Get an element from a fifo or block until one is added. * @param ff The element to work on. */
void *cmn_fifo_get (cmn_fifo_t * ff);


/**
 * Try to get an element from a fifo, but do not block if there is no element.
 * @param ff The element to work on.
 */
void *cmn_fifo_tryget (cmn_fifo_t * ff);

#ifdef __cplusplus
}
#endif


#endif

