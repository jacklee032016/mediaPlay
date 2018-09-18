/*
* $Id$
*/
#include "libCmn.h"
#include "cmnFsm.h"

int _fsmStateHandle(statemachine_t *state, void *ctx, EVENT *event)
{
	int	i;
	int	res;
	transition_t *handle = state->eventHandlers;
	
	for(i=0; i< state->size; i++)
	{
		if(event->event == handle->event )
		{
//			MUX_DEBUG("Event '%s' is handled in state '%s'", handle->name, state->name);
			res = (handle->handle)(ctx, event );

			/* FSM is in shared library, so it does not manage memory allocated by the app: who allocate, then who free it */
			/*
			cmn_free(event);
			*/			
			return res;
		}
		
		handle++;
	}
	
	return STATE_CONTINUE;
}


/* ctx is object which will be send to event handler of FSM */
int cmnFsmHandle(void *ctx, void *_event)
{
	int		j;
	int		newState = STATE_CONTINUE;

	EVENT	*event = (EVENT *)_event;
	FSM_OWNER *owner = (FSM_OWNER *)event->ownerCtx;
	
	if(event->event <= EVENT_UNKNOWN )
	{
		MUX_WARN( "Unknown Event\n");
		return -EXIT_FAILURE;
	}

	if(owner == NULL )
	{
		MUX_WARN("FSM Owner is null\n");
		exit(1);
	}

//	MUX_DEBUG("FSM Owner is '%s'", owner->fsm->name );

	if(owner->fsm == NULL )
	{
		MUX_WARN("FSM is null\n");
		exit(1);
	}

	for(j=0; j< owner->fsm->size; j++)
	{
		if( owner->fsm->states[j].state == owner->currentState )
		{
			MUX_DEBUG("FSM '%s' handling in state '%s'", owner->name, owner->fsm->states[j].name);
			newState = _fsmStateHandle( &owner->fsm->states[j], ctx, event);
			break;
		}
	}
	
	if(newState != STATE_CONTINUE && newState != owner->currentState )
	{
		for(j=0; j< owner->fsm->size; j++)
		{
			if( owner->fsm->states[j].state == newState )
			{
				/* clean old timer for this device and new timer is set in enter_handle */
				owner->timeout = 0;
				
				if( owner->fsm->states[j].enter_handle!= NULL)
					( owner->fsm->states[j].enter_handle)( owner);
				
				MUX_DEBUG("Entered into new state '%s' of FSM '%s'", owner->fsm->states[j].name, owner->name );
				break;
			}
		}
		
		owner->currentState = newState;
	}

	return EXIT_SUCCESS;
}


