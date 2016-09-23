#! /bin/ksh -p
#
# CDDL HEADER START
#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#
# CDDL HEADER END
#

#
# Copyright (c) 2014, 2015 by Delphix. All rights reserved.
#

. $STF_SUITE/include/libtest.shlib
. $STF_SUITE/tests/functional/removal/removal.kshlib

TMPDIR=${TMPDIR:-/tmp}
log_must $MKFILE $(($MINVDEVSIZE * 2)) $TMPDIR/dsk1
log_must $MKFILE $(($MINVDEVSIZE * 2)) $TMPDIR/dsk2
DISKS="$TMPDIR/dsk1 $TMPDIR/dsk2"
REMOVEDISK=$TMPDIR/dsk1

log_must default_setup_noexit "$DISKS"

function cleanup
{
	default_cleanup_noexit
	log_must $RM -f $DISKS
}

log_onexit cleanup

FILE_CONTENTS="Leeloo Dallas mul-ti-pass."

echo $FILE_CONTENTS  >$TESTDIR/$TESTFILE0
log_must [ "x$($CAT $TESTDIR/$TESTFILE0)" = "x$FILE_CONTENTS" ]

for i in {1..10}; do
	log_must $ZPOOL remove $TESTPOOL $TMPDIR/dsk1
	log_must wait_for_removal $TESTPOOL
	log_mustnot vdevs_in_pool $TESTPOOL $TMPDIR/dsk1
	log_must $ZPOOL add $TESTPOOL $TMPDIR/dsk1

	log_must $DD if=$TESTDIR/$TESTFILE0 of=/dev/null
	log_must [ "x$($CAT $TESTDIR/$TESTFILE0)" = "x$FILE_CONTENTS" ]

	log_must $ZPOOL remove $TESTPOOL $TMPDIR/dsk2
	log_must wait_for_removal $TESTPOOL
	log_mustnot vdevs_in_pool $TESTPOOL $TMPDIR/dsk2
	log_must $ZPOOL add $TESTPOOL $TMPDIR/dsk2

	log_must $DD if=$TESTDIR/$TESTFILE0 of=/dev/null
	log_must [ "x$($CAT $TESTDIR/$TESTFILE0)" = "x$FILE_CONTENTS" ]
done

log_pass "File contents transfered completely from one disk to another."
