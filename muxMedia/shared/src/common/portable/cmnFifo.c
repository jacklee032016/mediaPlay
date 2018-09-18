/*
* $Id: port_fifo.c,v 1.4 2007/03/19 20:43:32 lizhijie Exp $
*/

#include <cmnOsPort.h>

/* always use this method to initiate cmn_fifo_t.*/
void cmn_fifo_init (int _maxSize, cmn_fifo_t **_ff)
{
	cmn_fifo_t *ff = NULL;
	ff = cmn_malloc(sizeof(cmn_fifo_t) );
	*_ff = ff;
	
	if(_maxSize < 0 )
	{
		_maxSize = 1;
	}
	
	ff->qislocked = cmn_mutex_init ();
	/*INIT SEMA TO BLOCK ON GET() WHEN QUEUE IS EMPTY */
	ff->qisempty = cmn_sem_init (0);

	ff->queue = (cmn_list_t *) cmn_malloc (sizeof (cmn_list_t));
	cmn_list_init (ff->queue);
	/* ff->nb_elt = 0; */
	ff->etat = vide;

	ff->maxSize = _maxSize;
}

int cmn_fifo_add (cmn_fifo_t * ff, void *el)
{
	cmn_mutex_lock (ff->qislocked);

	if (ff->etat != plein)
	{
		/* ff->nb_elt++; */
		cmn_list_add (ff->queue, el, -1);	/* insert at end of queue */
	}
	else
	{
		MUX_WARN("too much traffic in fifo.");

		cmn_mutex_unlock (ff->qislocked);
		return -1;		/* stack is full */
	}

	/* if (ff->nb_elt >= FIFO_MAX_SIZE) */
	if (cmn_list_size (ff->queue) >= ff->maxSize )
		ff->etat = plein;
	else
		ff->etat = ok;

	cmn_sem_post (ff->qisempty);
	cmn_mutex_unlock (ff->qislocked);

	return 0;
}


int cmn_fifo_insert (cmn_fifo_t * ff, void *el)
{
	cmn_mutex_lock (ff->qislocked);

	if (ff->etat != plein)
	{
		/* ff->nb_elt++; */
		cmn_list_add (ff->queue, el, 0);	/* insert at end of queue */
	}
	else
	{
		MUX_WARN("too much traffic in fifo.");

		cmn_mutex_unlock (ff->qislocked);
		return -1;		/* stack is full */
	}
	/* if (ff->nb_elt >= FIFO_MAX_SIZE) */
	if (cmn_list_size (ff->queue) >= ff->maxSize )
		ff->etat = plein;
	else
		ff->etat = ok;

	cmn_sem_post (ff->qisempty);
	cmn_mutex_unlock (ff->qislocked);

	return 0;
}


int cmn_fifo_size (cmn_fifo_t * ff)
{
	int i;

	cmn_mutex_lock (ff->qislocked);

	i = cmn_list_size (ff->queue);

	cmn_mutex_unlock (ff->qislocked);

	return i;
}


void *cmn_fifo_get (cmn_fifo_t * ff)
{
	void *el = NULL;

	int i = cmn_sem_wait (ff->qisempty);

	if (i != 0)
		return NULL;
	cmn_mutex_lock (ff->qislocked);

	if (ff->etat != vide)
	{
		el = cmn_list_get (ff->queue, 0);
		cmn_list_remove (ff->queue, 0);
		/* ff->nb_elt--; */
	}
	else
	{
		MUX_ERROR("no element in fifo.");
		cmn_mutex_unlock (ff->qislocked);
		return 0;			/* pile vide */
	}
	/* if (ff->nb_elt <= 0) */
	if (cmn_list_size (ff->queue) <= 0)
		ff->etat = vide;
	else
		ff->etat = ok;

	cmn_mutex_unlock (ff->qislocked);
	return el;
}

void *cmn_fifo_tryget (cmn_fifo_t * ff)
{
	void *el = NULL;

	if (0 != cmn_sem_trywait (ff->qisempty))
	{/* no elements... */
		return NULL;
	}

	cmn_mutex_lock (ff->qislocked);
	if (ff->etat == vide)
		return NULL;

	if (ff->etat != vide)
	{
		el = cmn_list_get (ff->queue, 0);
		cmn_list_remove (ff->queue, 0);
		/* ff->nb_elt--; */
	}
	else
	{/* this case MUST never happen... */
		MUX_ERROR("no element in fifo.");
		cmn_mutex_unlock (ff->qislocked);
		return 0;
	}

	/* if (ff->nb_elt <= 0) */
	if (cmn_list_size (ff->queue) <= 0)
		ff->etat = vide;
	else
		ff->etat = ok;

	cmn_mutex_unlock (ff->qislocked);

	return el;
}

void cmn_fifo_free (cmn_fifo_t * ff)
{
	if (ff == NULL)
		return;

	cmn_mutex_destroy (ff->qislocked);
	/* seems that pthread_mutex_destroy does not free space by itself */
	cmn_sem_destroy (ff->qisempty);

	cmn_list_ofchar_free(ff->queue, 0);
	
	cmn_free (ff->queue);
	cmn_free (ff);
}

