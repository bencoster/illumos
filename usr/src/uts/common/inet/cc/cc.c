/*-
 * Copyright (c) 2007-2008
 *	Swinburne University of Technology, Melbourne, Australia.
 * Copyright (c) 2009-2010 Lawrence Stewart <lstewart@freebsd.org>
 * Copyright (c) 2010 The FreeBSD Foundation
 * All rights reserved.
 * Copyright (c) 2014 by Delphix. All rights reserved.
 *
 * This software was developed at the Centre for Advanced Internet
 * Architectures, Swinburne University of Technology, by Lawrence Stewart and
 * James Healy, made possible in part by a grant from the Cisco University
 * Research Program Fund at Community Foundation Silicon Valley.
 *
 * Portions of this software were developed at the Centre for Advanced
 * Internet Architectures, Swinburne University of Technology, Melbourne,
 * Australia by David Hayes under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This software was first released in 2007 by James Healy and Lawrence Stewart
 * whilst working on the NewTCP research project at Swinburne University of
 * Technology's Centre for Advanced Internet Architectures, Melbourne,
 * Australia, which was made possible in part by a grant from the Cisco
 * University Research Program Fund at Community Foundation Silicon Valley.
 * More details are available at:
 *   http://caia.swin.edu.au/urp/newtcp/
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <inet/cc.h>
#include <inet/tcp.h>

#define	CC_KMODDIR	"cc"

/*
 * List of available cc algorithms on the current system. Access is
 * synchronized using cc_list_lock.
 */
static STAILQ_HEAD(cc_head, cc_algo) cc_list = STAILQ_HEAD_INITIALIZER(cc_list);
static kmutex_t cc_list_lock;

/*
 * Initialise CC subsystem on system boot.
 */
void
cc_init(void)
{
	STAILQ_INIT(&cc_list);
}

struct cc_algo *
cc_load_algo(const char *name)
{
	struct cc_algo *algo;
	boolean_t tried_modload = B_FALSE, found = B_FALSE;
	char modname[CC_ALGO_NAME_MAX + 3] = "cc_"; /* cc_<name> */

find_registered_algo:
	mutex_enter(&cc_list_lock);
	STAILQ_FOREACH(algo, &cc_list, entries) {
		if (strncmp(algo->name, name, CC_ALGO_NAME_MAX) == 0) {
			found = B_TRUE;
			break;
		}
	}
	mutex_exit(&cc_list_lock);

	if (!found) {
		algo = NULL;
		if (!tried_modload) {
			/*
			 * If the module has not yet been loaded, then attempt
			 * to load it now.  If modload() succeeds, the
			 * algorithm should have registered using
			 * cc_register_algo(), in which case we can go back and
			 * attempt to find it again.
			 */
			(void) strlcat(modname, name, sizeof (modname));
			if (modload(CC_KMODDIR, modname) != -1) {
				tried_modload = B_TRUE;
				goto find_registered_algo;
			}
		}
	}

	return (algo);
}

/*
 * Returns non-zero on success, 0 on failure.
 */
int
cc_deregister_algo(struct cc_algo *remove_cc)
{
	struct cc_algo *funcs, *tmpfuncs;
	int err = ENOENT;

	mutex_enter(&cc_list_lock);
        STAILQ_FOREACH_SAFE(funcs, &cc_list, entries, tmpfuncs) {
                if (funcs == remove_cc) {
                        STAILQ_REMOVE(&cc_list, funcs, cc_algo, entries);
                        err = 0;
                        break;
                }
        }
        mutex_exit(&cc_list_lock);
	return (err);
}

/*
 * Returns 0 on success, non-zero on failure.
 */
int
cc_register_algo(struct cc_algo *add_cc)
{
	struct cc_algo *funcs;
	int err = 0;

	/*
	 * Iterate over list of registered CC algorithms and make sure
	 * we're not trying to add a duplicate.
	 */
	mutex_enter(&cc_list_lock);
	STAILQ_FOREACH(funcs, &cc_list, entries) {
		if (strncmp(funcs->name, add_cc->name, CC_ALGO_NAME_MAX) == 0)
			err = EEXIST;
	}

	if (err == 0)
		STAILQ_INSERT_TAIL(&cc_list, add_cc, entries);

	mutex_exit(&cc_list_lock);

	return (err);
}
