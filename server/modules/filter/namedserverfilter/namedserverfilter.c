/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-07-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include <stdio.h>
#include <maxscale/alloc.h>
#include <maxscale/filter.h>
#include <maxscale/modinfo.h>
#include <maxscale/modutil.h>
#include <maxscale/log_manager.h>
#include <string.h>
#include <regex.h>
#include <maxscale/hint.h>
#include <maxscale/alloc.h>

/**
 * @file namedserverfilter.c - a very simple regular expression based filter
 * that routes to a named server if a regular expression match is found.
 * @verbatim
 *
 * A simple regular expression based query routing filter.
 * Two parameters should be defined in the filter configuration
 *      match=<regular expression>
 *      server=<server to route statement to>
 * Two optional parameters
 *      source=<source address to limit filter>
 *      user=<username to limit filter>
 *
 * Date         Who             Description
 * 22/01/2015   Mark Riddoch    Written as example based on regex filter
 * @endverbatim
 */

static FILTER *createInstance(const char *name, char **options, FILTER_PARAMETER **params);
static void *newSession(FILTER *instance, SESSION *session);
static void closeSession(FILTER *instance, void *session);
static void freeSession(FILTER *instance, void *session);
static void setDownstream(FILTER *instance, void *fsession, DOWNSTREAM *downstream);
static int routeQuery(FILTER *instance, void *fsession, GWBUF *queue);
static void diagnostic(FILTER *instance, void *fsession, DCB *dcb);
static uint64_t getCapabilities(void);

/**
 * Instance structure
 */
typedef struct
{
    char *source; /* Source address to restrict matches */
    char *user; /* User name to restrict matches */
    char *match; /* Regular expression to match */
    char *server; /* Server to route to */
    regex_t re; /* Compiled regex text */
} REGEXHINT_INSTANCE;

/**
 * The session structuee for this regex filter
 */
typedef struct
{
    DOWNSTREAM down; /* The downstream filter */
    int n_diverted; /* No. of statements diverted */
    int n_undiverted; /* No. of statements not diverted */
    int active; /* Is filter active */
} REGEXHINT_SESSION;

/**
 * The module entry point routine. It is this routine that
 * must populate the structure that is referred to as the
 * "module object", this is a structure with the set of
 * external entry points for this module.
 *
 * @return The module object
 */
MODULE_INFO* GetModuleObject()
{
    static FILTER_OBJECT MyObject =
    {
        createInstance,
        newSession,
        closeSession,
        freeSession,
        setDownstream,
        NULL, // No Upstream requirement
        routeQuery,
        NULL, // No clientReply
        diagnostic,
        getCapabilities,
        NULL, // No destroyInstance
    };

    static MODULE_INFO info =
    {
        MODULE_API_FILTER,
        MODULE_GA,
        FILTER_VERSION,
        "A routing hint filter that uses regular expressions to direct queries",
        "V1.1.0",
        &MyObject
    };

    return &info;
}

/**
 * Create an instance of the filter for a particular service
 * within MaxScale.
 *
 * @param params    The array of name/value pair parameters for the filter
 * @param options   The options for this filter
 * @param params    The array of name/value pair parameters for the filter
 *
 * @return The instance data for this new instance
 */
static FILTER *
createInstance(const char *name, char **options, FILTER_PARAMETER **params)
{
    REGEXHINT_INSTANCE *my_instance = (REGEXHINT_INSTANCE*)MXS_MALLOC(sizeof(REGEXHINT_INSTANCE));

    if (my_instance)
    {
        my_instance->match = NULL;
        my_instance->server = NULL;
        my_instance->source = NULL;
        my_instance->user = NULL;
        bool error = false;

        for (int i = 0; params && params[i]; i++)
        {
            if (!strcmp(params[i]->name, "match"))
            {
                my_instance->match = MXS_STRDUP_A(params[i]->value);
            }
            else if (!strcmp(params[i]->name, "server"))
            {
                my_instance->server = MXS_STRDUP_A(params[i]->value);
            }
            else if (!strcmp(params[i]->name, "source"))
            {
                my_instance->source = MXS_STRDUP_A(params[i]->value);
            }
            else if (!strcmp(params[i]->name, "user"))
            {
                my_instance->user = MXS_STRDUP_A(params[i]->value);
            }
            else if (!filter_standard_parameter(params[i]->name))
            {
                MXS_ERROR("namedserverfilter: Unexpected parameter '%s'.",
                          params[i]->name);
                error = true;
            }
        }

        int cflags = REG_ICASE;

        if (options)
        {
            for (int i = 0; options[i]; i++)
            {
                if (!strcasecmp(options[i], "ignorecase"))
                {
                    cflags |= REG_ICASE;
                }
                else if (!strcasecmp(options[i], "case"))
                {
                    cflags &= ~REG_ICASE;
                }
                else if (!strcasecmp(options[i], "extended"))
                {
                    cflags |= REG_EXTENDED;
                }
                else
                {
                    MXS_ERROR("namedserverfilter: Unsupported option '%s'.",
                              options[i]);
                    error = true;
                }
            }
        }

        if (my_instance->match == NULL)
        {
            MXS_ERROR("namedserverfilter: Missing required parameters 'match'.");
            error = true;
        }

        if (my_instance->server == NULL)
        {
            MXS_ERROR("namedserverfilter: Missing required parameters 'server'.");
            error = true;
        }
        if (my_instance->server && my_instance->match &&
            regcomp(&my_instance->re, my_instance->match, cflags))
        {
            MXS_ERROR("namedserverfilter: Invalid regular expression '%s'.\n",
                      my_instance->match);
            MXS_FREE(my_instance->match);
            my_instance->match = NULL;
            error = true;
        }

        if (error)
        {
            if (my_instance->match)
            {
                regfree(&my_instance->re);
                MXS_FREE(my_instance->match);
            }
            MXS_FREE(my_instance->server);
            MXS_FREE(my_instance->source);
            MXS_FREE(my_instance->user);
            MXS_FREE(my_instance);
            my_instance = NULL;
        }

    }
    return (FILTER *) my_instance;
}

/**
 * Associate a new session with this instance of the filter.
 *
 * @param instance  The filter instance data
 * @param session   The session itself
 * @return Session specific data for this session
 */
static void *
newSession(FILTER *instance, SESSION *session)
{
    REGEXHINT_INSTANCE *my_instance = (REGEXHINT_INSTANCE *) instance;
    REGEXHINT_SESSION *my_session;
    const char *remote, *user;

    if ((my_session = MXS_CALLOC(1, sizeof(REGEXHINT_SESSION))) != NULL)
    {
        my_session->n_diverted = 0;
        my_session->n_undiverted = 0;
        my_session->active = 1;
        if (my_instance->source
            && (remote = session_get_remote(session)) != NULL)
        {
            if (strcmp(remote, my_instance->source))
            {
                my_session->active = 0;
            }
        }

        if (my_instance->user && (user = session_get_user(session))
            && strcmp(user, my_instance->user))
        {
            my_session->active = 0;
        }
    }

    return my_session;
}

/**
 * Close a session with the filter, this is the mechanism
 * by which a filter may cleanup data structure etc.
 *
 * @param instance  The filter instance data
 * @param session   The session being closed
 */
static void
closeSession(FILTER *instance, void *session)
{
}

/**
 * Free the memory associated with this filter session.
 *
 * @param instance  The filter instance data
 * @param session   The session being closed
 */
static void
freeSession(FILTER *instance, void *session)
{
    MXS_FREE(session);
    return;
}

/**
 * Set the downstream component for this filter.
 *
 * @param instance  The filter instance data
 * @param session   The session being closed
 * @param downstream    The downstream filter or router
 */
static void
setDownstream(FILTER *instance, void *session, DOWNSTREAM *downstream)
{
    REGEXHINT_SESSION *my_session = (REGEXHINT_SESSION *) session;
    my_session->down = *downstream;
}

/**
 * The routeQuery entry point. This is passed the query buffer
 * to which the filter should be applied. Once applied the
 * query should normally be passed to the downstream component
 * (filter or router) in the filter chain.
 *
 * If the regular expressed configured in the match parameter of the
 * filter definition matches the SQL text then add the hint
 * "Route to named server" with the name defined in the server parameter
 *
 * @param instance  The filter instance data
 * @param session   The filter session
 * @param queue     The query data
 */
static int
routeQuery(FILTER *instance, void *session, GWBUF *queue)
{
    REGEXHINT_INSTANCE *my_instance = (REGEXHINT_INSTANCE *) instance;
    REGEXHINT_SESSION *my_session = (REGEXHINT_SESSION *) session;
    char *sql;

    if (modutil_is_SQL(queue) && my_session->active)
    {
        if ((sql = modutil_get_SQL(queue)) != NULL)
        {
            if (regexec(&my_instance->re, sql, 0, NULL, 0) == 0)
            {
                queue->hint = hint_create_route(queue->hint,
                                                HINT_ROUTE_TO_NAMED_SERVER,
                                                my_instance->server);
                my_session->n_diverted++;
            }
            else
            {
                my_session->n_undiverted++;
            }
            MXS_FREE(sql);
        }
    }
    return my_session->down.routeQuery(my_session->down.instance,
                                       my_session->down.session, queue);
}

/**
 * Diagnostics routine
 *
 * If fsession is NULL then print diagnostics on the filter
 * instance as a whole, otherwise print diagnostics for the
 * particular session.
 *
 * @param   instance    The filter instance
 * @param   fsession    Filter session, may be NULL
 * @param   dcb     The DCB for diagnostic output
 */
static void
diagnostic(FILTER *instance, void *fsession, DCB *dcb)
{
    REGEXHINT_INSTANCE *my_instance = (REGEXHINT_INSTANCE *) instance;
    REGEXHINT_SESSION *my_session = (REGEXHINT_SESSION *) fsession;

    dcb_printf(dcb, "\t\tMatch and route:           /%s/ -> %s\n",
               my_instance->match, my_instance->server);
    if (my_session)
    {
        dcb_printf(dcb, "\t\tNo. of queries diverted by filter: %d\n",
                   my_session->n_diverted);
        dcb_printf(dcb, "\t\tNo. of queries not diverted by filter:     %d\n",
                   my_session->n_undiverted);
    }
    if (my_instance->source)
    {
        dcb_printf(dcb,
                   "\t\tReplacement limited to connections from     %s\n",
                   my_instance->source);
    }
    if (my_instance->user)
    {
        dcb_printf(dcb,
                   "\t\tReplacement limit to user           %s\n",
                   my_instance->user);
    }
}

/**
 * Capability routine.
 *
 * @return The capabilities of the filter.
 */
static uint64_t getCapabilities(void)
{
    return RCAP_TYPE_CONTIGUOUS_INPUT;
}
