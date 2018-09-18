/*
* A general form of FSM (Finite State Machine), which can be used by any owner
*/

#ifndef	__CMN_FSM_H__
#define	__CMN_FSM_H__


/* states shared by all FSMs */
#define	STATE_CLOSING				-1
#define	STATE_CONTINUE			-2

/* events shared by all FSMs */
#define	EVENT_ERROR			-3
#define	EVENT_TIMEOUT			-2
#define	EVENT_UNKNOWN		-1
#define	EVENT_CLOSING			0


typedef	struct
{
	int					event;
	int					statusCode;

	void					*data;		/* */
	
	void					*ownerCtx;	/* owner of FSM which handle this event */
}EVENT;


typedef	struct _transition_t
{
	int				event;
	char				*name;

	int				(*handle)(void *ownerCtx, EVENT *event);
}TRANSITION;

typedef	struct _transition_t	transition_t;

struct	_transition_table_t
{
	int					state;
	char					*name;

	int					size;
	transition_t			*eventHandlers;
	
	void					(*enter_handle)(void *ownerCtx);
};

typedef	struct	_transition_table_t			statemachine_t;


typedef	struct	_SERVICE_FSM
{
	int					size;
	char					*name;
	statemachine_t		*states;
}SERVICE_FSM;


typedef	struct	_FSM_OWNER
{
	char					name[CMN_NAME_LENGTH];
	int					currentState;
	
	int					currentCmd;		/* pending command */
	void					*dataOfCurrentCmd;	
	
	long					timeout;

	int					errorCode;
	
	SERVICE_FSM			*fsm;
	
	void					*ctx;	/* other info used to extended by owner, eg. MUX_PLAY_T of this FSM */
}FSM_OWNER;


int cmnFsmHandle(void *ctx, void *_event);


#endif

