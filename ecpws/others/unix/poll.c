/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@boa.org>
 *  Some changes Copyright (C) 1996-2000 Jon Nelson <jnelson@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* $Id: poll.c,v 1.1.1.1 2006/11/29 17:51:53 lizhijie Exp $*/

#include "ecpws.h"

void update_blocked(struct pollfd pfd1[]);

struct pollfd *pfds;
unsigned int pfd_len;

void loop(int server_s)
{
    struct pollfd pfd1[2][MAX_FD];
    short which = 0, other = 1, temp;
    int server_pfd, watch_server;

    pfds = pfd1[which];
    pfd_len = server_pfd = 0;
    watch_server = 1;

    while (1) {
        int timeout;

        time(&current_time);

        if (sighup_flag)
            sighup_run();
        if (sigchld_flag)
            sigchld_run();
        if (sigalrm_flag)
            sigalrm_run();

        if (sigterm_flag) {
            if (sigterm_flag == 1) {
                sigterm_stage1_run();
                close(server_s);
                server_s = -1;
                /* remove server_pfd */
                {
                    unsigned int i, j;

                    for(i = 0, j = 0;i < pfd_len;++i) {
                        if (i == (unsigned) server_pfd)
                            continue;
                        pfd1[other][j].fd = pfd1[which][j].fd;
                        pfd1[other][j].events = pfd1[which][j].events;
                        ++j;
                    }
                    pfd_len = j;
                    pfds = pfd1[other];
                    temp = other;
                    other = which;
                    which = temp;
                }
                watch_server = 0;
            }
            if (sigterm_flag == 2 && !QUEUE_FIRST_READY(ecpws) && !QUEUE_FIRST_BLOCK(ecpws)) {
                sigterm_stage2_run();
            }
        } else {
            if (_ecpws.totalConnections < _ecpws.cfg->maxConnections) {
                server_pfd = pfd_len++;
                pfds[server_pfd].fd = server_s;
                pfds[server_pfd].events = BOA_READ;
                watch_server = 1;
            } else {
                watch_server = 0;
            }
        }

        /* If there are any requests ready, the timeout is 0.
         * If not, and there are any requests blocking, the
         *  timeout is ka_timeout ? ka_timeout * 1000, otherwise
         *  REQUEST_TIMEOUT * 1000.
         * -1 means forever
         */
        pending_requests = 0;
        if (pfd_len) {
            timeout = (QUEUE_FIRST_READY(ecpws) ? 0 :
                       (QUEUE_FIRST_BLOCK(ecpws) ?
                        (ka_timeout ? ka_timeout * 1000 :
                         REQUEST_TIMEOUT * 1000) : -1));

            if (poll(pfds, pfd_len, timeout) == -1) {
                if (errno == EINTR)
                    continue;       /* while(1) */
            }
            if (!sigterm_flag && watch_server &&
                (pfds[server_pfd].revents & BOA_READ))
                pending_requests = 1;
            time(&current_time);
            /* if pfd_len is 0, we didn't poll, so the current time
             * should be up-to-date, and we *won't* be accepting anyway
             */
        }

        /* go through blocked and unblock them if possible */
        /* also resets pfd_len and pfd to known blocked */
        pfd_len = 0;
        if (QUEUE_FIRST_BLOCK(ecpws)) {
            update_blocked(pfd1[other]);
        }

        /* swap pfd */
        pfds = pfd1[other];
        temp = other;
        other = which;
        which = temp;

        /* process any active requests */
        process_requests(server_s);
    }
}

/*
 * Name: update_blocked
 *
 * Description: iterate through the blocked requests, checking whether
 * that file descriptor has been set by select.  Update the fd_set to
 * reflect current status.
 *
 * Here, we need to do some things:
 *  - keepalive timeouts simply close
 *    (this is special:: a keepalive timeout is a timeout where
 *    keepalive is active but nothing has been read yet)
 *  - regular timeouts close + error
 *  - stuff in buffer and fd ready?  write it out
 *  - fd ready for other actions?  do them
 */

void update_blocked(struct pollfd pfd1[])
{
    HTTP_REQ *current, *next = NULL;
    time_t time_since;

    time(&current_time);
    for (current = QUEUE_FIRST_READY(ecpws); current; current = next) {
        time_since = current_time - current->time_last;
        next = current->next;

        // FIXME::  the first below has the chance of leaking memory!
        //  (setting status to DEAD not DONE....)
        /* hmm, what if we are in "the middle" of a request and not
         * just waiting for a new one... perhaps check to see if anything
         * has been read via header position, etc... */
        if (pfds[current->pollfd_id].revents) {
            ;
        } else if (time_since > REQUEST_TIMEOUT) {
            ecpwsLogErrorReq(current);
            fputs("connection timed out\n", stderr);
            current->status = DEAD;
        } else if (current->kacount < ka_max && /* we *are* in a keepalive */
            (time_since >= ka_timeout) && /* ka timeout has passed */
            !current->logline) { /* haven't read anything yet */
            current->status = DEAD; /* connection keepalive timed out */
        } else {                /* still blocked */
            pfd1[pfd_len].fd = pfds[current->pollfd_id].fd;
            pfd1[pfd_len].events = pfds[current->pollfd_id].events;
            current->pollfd_id = pfd_len++;
            continue;
        }
        ready_request(current);
    }
}
