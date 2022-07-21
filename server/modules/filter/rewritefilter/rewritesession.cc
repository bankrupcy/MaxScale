/*
 * Copyright (c) 2022 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2026-06-06
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#define MXB_MODULE_NAME "rewitefilter"
#include "rewritesession.hh"
#include "rewritefilter.hh"

#include <maxscale/modutil.hh>

RewriteFilterSession::RewriteFilterSession(MXS_SESSION* pSession,
                                           SERVICE* pService,
                                           const std::shared_ptr<const SessionData>& sSettings)
    : maxscale::FilterSession(pSession, pService)
    , m_sSession_data(sSettings)
{
}

void RewriteFilterSession::log_replacement(const std::string& from,
                                           const std::string& to,
                                           bool what_if)
{
    std::ostringstream os;
    if (what_if)
    {
        os << "what_if is set. Would r";
    }
    else
    {
        os << 'R';
    }
    os << "eplace \"" << from << "\" with \"" << to << '"';
    MXB_NOTICE("%s", os.str().c_str());
}

RewriteFilterSession::~RewriteFilterSession()
{
}

// static
RewriteFilterSession* RewriteFilterSession::create(MXS_SESSION* pSession,
                                                   SERVICE* pService,
                                                   const std::shared_ptr<const SessionData>& sSettings)
{
    return new RewriteFilterSession(pSession, pService, sSettings);
}

bool RewriteFilterSession::routeQuery(GWBUF* pBuffer)
{
    auto& session_data = *m_sSession_data.get();
    const auto& sql = pBuffer->get_sql();

    for (const auto& r : session_data.rewriters)
    {
        std::string new_sql;
        if (r->replace(sql, &new_sql))
        {
            if (session_data.settings.log_replacement || r->template_def().what_if)
            {
                log_replacement(sql, new_sql, r->template_def().what_if);
            }

            if (!r->template_def().what_if)
            {
                gwbuf_free(pBuffer);
                pBuffer = modutil_create_query(new_sql.c_str());
            }
            break;
        }
    }

    return mxs::FilterSession::routeQuery(pBuffer);
}
