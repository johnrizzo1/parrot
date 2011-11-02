/*
Copyright (C) 2010-2011, Parrot Foundation.

=head1 NAME

src/alarm.c - Implements a mechanism for alarms, setting a flag after a delay.

=cut

*/

#include "parrot/parrot.h"
#include "parrot/alarm.h"

/* Some per-process state */
static volatile UINTVAL  alarm_serial = 0;
static volatile FLOATVAL alarm_set_to = 0.0;

/* This file relies on POSIX. Probably need two other versions of it:
 *  one for Windows and one for platforms with no signals or threads. */

#include <windows.h>
#include <time.h>
#include <errno.h>

/* HEADERIZER BEGIN: static */
/* HEADERIZER END: static */

/*

=over 4

=item C<void Parrot_alarm_init(void)>

Initialize the alarm queue. This function should only be called from the initial
pthread. Any other pthreads should make sure to mask out SIGALRM.

=cut

*/

VOID CALLBACK Parrot_alarm_callback(PVOID lparam, BOOLEAN TimerOrWaitFired);

void
Parrot_alarm_init(PARROT_INTERP, ARGIN(PMC * const scheduler))
{
    Parrot_Scheduler_attributes * const sched = PARROT_SCHEDULER(scheduler);
    sched->alarm_data = CreateTimerQueue();
}

void
Parrot_alarm_destroy(PARROT_INTERP, ARGIN(PMC * const scheduler))
{
    Parrot_Scheduler_attributes * const sched = PARROT_SCHEDULER(scheduler);
    DeleteTimerQueue(sched->alarm_data);
}


/*

=item C<void Parrot_alarm_callback(int sig_number)>

Callback for SIGALRM. When this is called, a timer should be ready to fire.

=cut

*/

static VOID CALLBACK
Parrot_alarm_callback(PVOID lparam, BOOLEAN TimerOrWaitFired);
{
    ASSERT_ARGS(Parrot_alarm_callback)

    /* Not atomic; only one thread ever writes this value */
    alarm_serial += 1;
}

/*

=item C<int Parrot_alarm_check(UINTVAL* last_serial)>

Has any alarm triggered since we last checked?

Possible design improvement: Alert only the thread that
set the alarm.

=cut

*/

PARROT_EXPORT
int
Parrot_alarm_check(ARGMOD(UINTVAL* last_serial))
{
    ASSERT_ARGS(Parrot_alarm_check)

    if (*last_serial == alarm_serial) {
        return 0;
    }
    else {
        *last_serial = alarm_serial;
        return 1;
    }
}

static void
windows_alarm_set(PARROT_INTERP, FLOATVAL time)
{
    ASSERT_ARGS(windows_alarm_set)
    Parrot_Scheduler_attributes * const sched = PARROT_SCHEDULER(scheduler);
    HANDLE hTimer;
    INTVAL ms = ((int)time) * 1000;
    CreateTimeQueueTimer(&hTimer, sched->alarm_data, (WAITORTIMERCALLBACK)Parrot_alarm_callback, interp, ms, 0, 0);

}

/*

=item C<void Parrot_alarm_set(FLOATVAL when)>

Sets an alarm to trigger at time 'when'.

=cut

*/

PARROT_EXPORT
void
Parrot_alarm_set(PARROT_INTERP, FLOATVAL when)
{
    ASSERT_ARGS(Parrot_alarm_set)
    FLOATVAL now = Parrot_floatval_time();

    /* Better late than early */
    when += 0.0001;

    if (alarm_set_to > now && alarm_set_to < when)
        return;

    alarm_set_to = when;
    windows_alarm_set(interp, when - now);
}

/*

=item C<void Parrot_alarm_now(void)>

Trigger an alarm wakeup.

=cut

*/

void
Parrot_alarm_now(void)
{
    ASSERT_ARGS(Parrot_alarm_now)
#ifdef _WIN32
    /* TODO: Implement on Windows */
#else
    kill(getpid(), SIGALRM);
#endif
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4 cinoptions='\:2=2' :
 */
