/*
 * Copyright (c) 2020 MariaDB Corporation Ab
 * Copyright (c) 2023 MariaDB plc, Finnish Branch
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-03-14
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
import QueryConn from '@wsModels/QueryConn'
import QueryEditor from '@wsModels/QueryEditor'
import QueryTab from '@wsModels/QueryTab'
import QueryTabTmp from '@wsModels/QueryTabTmp'
import QueryResult from '@wsModels/QueryResult'
import Worksheet from '@wsModels/Worksheet'
import queries from '@wsSrc/api/queries'

export default {
    namespaced: true,
    actions: {
        /**
         * @param {String} param.qualified_name - Table id (database_name.table_name).
         * @param {String} param.query_mode - a key in QUERY_MODES. Either PRVW_DATA or PRVW_DATA_DETAILS
         */
        async fetchPrvw({ rootState, dispatch }, { qualified_name, query_mode }) {
            const config = Worksheet.getters('getActiveRequestConfig')
            const { id, meta: { name: connection_name } = {} } = QueryConn.getters(
                'getActiveQueryTabConn'
            )
            const activeQueryTabId = QueryEditor.getters('getActiveQueryTabId')
            const request_sent_time = new Date().valueOf()
            let field, sql, queryName
            const escapedQN = this.vue.$helpers.escapeIdentifiers(qualified_name)
            switch (query_mode) {
                case rootState.mxsWorkspace.config.QUERY_MODES.PRVW_DATA:
                    sql = `SELECT * FROM ${escapedQN} LIMIT 1000;`
                    queryName = `Preview ${escapedQN} data`
                    field = 'prvw_data'
                    break
                case rootState.mxsWorkspace.config.QUERY_MODES.PRVW_DATA_DETAILS:
                    sql = `DESCRIBE ${escapedQN};`
                    queryName = `View ${escapedQN} details`
                    field = 'prvw_data_details'
                    break
            }
            QueryTabTmp.update({
                where: activeQueryTabId,
                data(obj) {
                    obj[field].request_sent_time = request_sent_time
                    obj[field].total_duration = 0
                    obj[field].is_loading = true
                },
            })
            const [e, res] = await this.vue.$helpers.to(
                queries.post({
                    id,
                    body: { sql, max_rows: rootState.prefAndStorage.query_row_limit },
                    config,
                })
            )
            if (e)
                QueryTabTmp.update({
                    where: activeQueryTabId,
                    data(obj) {
                        obj[field].is_loading = false
                    },
                })
            else {
                const now = new Date().valueOf()
                const total_duration = ((now - request_sent_time) / 1000).toFixed(4)
                QueryTabTmp.update({
                    where: activeQueryTabId,
                    data(obj) {
                        obj[field].data = Object.freeze(res.data.data)
                        obj[field].total_duration = parseFloat(total_duration)
                        obj[field].is_loading = false
                    },
                })
                dispatch(
                    'prefAndStorage/pushQueryLog',
                    {
                        startTime: now,
                        name: queryName,
                        sql,
                        res,
                        connection_name,
                        queryType: rootState.mxsWorkspace.config.QUERY_LOG_TYPES.ACTION_LOGS,
                    },
                    { root: true }
                )
            }
        },
        /**
         * @param {String} sql - SQL string
         */
        async fetchUserQuery({ commit, dispatch, getters, rootState }, sql) {
            const config = Worksheet.getters('getActiveRequestConfig')
            const { id, meta: { name: connection_name } = {} } = QueryConn.getters(
                'getActiveQueryTabConn'
            )
            const request_sent_time = new Date().valueOf()
            const activeQueryTabId = QueryEditor.getters('getActiveQueryTabId')
            const abortController = new AbortController()
            commit(
                'queryResultsMem/UPDATE_ABORT_CONTROLLER_MAP',
                { id: activeQueryTabId, value: abortController },
                { root: true }
            )
            const {
                QUERY_CANCELED,
                QUERY_LOG_TYPES: { USER_LOGS },
            } = rootState.mxsWorkspace.config

            QueryTabTmp.update({
                where: activeQueryTabId,
                data(obj) {
                    obj.query_results.request_sent_time = request_sent_time
                    obj.query_results.total_duration = 0
                    obj.query_results.is_loading = true
                },
            })

            let [e, res] = await this.vue.$helpers.to(
                queries.post({
                    id,
                    body: { sql, max_rows: rootState.prefAndStorage.query_row_limit },
                    config: { ...config, signal: abortController.signal },
                })
            )
            const now = new Date().valueOf()
            const total_duration = ((now - request_sent_time) / 1000).toFixed(4)

            if (!e && res && sql.match(/(use|drop database)\s/i))
                await QueryConn.dispatch('updateActiveDb')

            if (getters.getHasKillFlag(activeQueryTabId)) {
                // If the KILL command was sent for the query is being run, the query request is aborted
                QueryTabTmp.update({ where: activeQueryTabId, data: { has_kill_flag: false } })
                /**
                 * This is done automatically in queryHttp.interceptors.response.
                 * However, because the request is aborted, is_busy needs to be set manually.
                 */
                QueryConn.update({ where: id, data: { is_busy: false } })
                res = {
                    data: { data: { attributes: { results: [{ message: QUERY_CANCELED }], sql } } },
                }
            }

            QueryTabTmp.update({
                where: activeQueryTabId,
                data(obj) {
                    obj.query_results.data = Object.freeze(res.data.data)
                    obj.query_results.total_duration = parseFloat(total_duration)
                    obj.query_results.is_loading = false
                },
            })

            dispatch(
                'prefAndStorage/pushQueryLog',
                {
                    startTime: request_sent_time,
                    sql,
                    res,
                    connection_name,
                    queryType: USER_LOGS,
                },
                { root: true }
            )
            commit('queryResultsMem/DELETE_ABORT_CONTROLLER', activeQueryTabId, { root: true })
        },
        /**
         * This action uses the current active QueryEditor connection to send
         * KILL QUERY thread_id
         */
        async stopUserQuery({ commit, rootGetters }) {
            const config = Worksheet.getters('getActiveRequestConfig')
            const activeQueryTabConn = QueryConn.getters('getActiveQueryTabConn')
            const activeQueryTabId = QueryEditor.getters('getActiveQueryTabId')
            const queryEditorConn = QueryConn.getters('getQueryEditorConn')

            // abort the query first then send kill flag
            const abort_controller = rootGetters['queryResultsMem/getAbortController'](
                activeQueryTabId
            )
            abort_controller.abort() // abort the running query

            QueryTabTmp.update({
                where: activeQueryTabId,
                data: { has_kill_flag: true },
            })
            const [e, res] = await this.vue.$helpers.to(
                queries.post({
                    id: queryEditorConn.id,
                    body: { sql: `KILL QUERY ${activeQueryTabConn.attributes.thread_id}` },
                    config,
                })
            )

            if (e) this.vue.$logger.error(e)
            else {
                const results = this.vue.$typy(res, 'data.data.attributes.results').safeArray
                if (this.vue.$typy(results, '[0].errno').isDefined)
                    commit(
                        'mxsApp/SET_SNACK_BAR_MESSAGE',
                        {
                            text: [
                                'Failed to stop the query',
                                ...Object.keys(results[0]).map(key => `${key}: ${results[0][key]}`),
                            ],
                            type: 'error',
                        },
                        { root: true }
                    )
            }
        },
        /**
         * This action clears prvw_data and prvw_data_details to empty object.
         * Call this action when user selects option in the sidebar.
         * This ensure sub-tabs in Data Preview tab are generated with fresh data
         */
        clearDataPreview() {
            QueryTabTmp.update({
                where: QueryEditor.getters('getActiveQueryTabId'),
                data(obj) {
                    obj.prvw_data = {}
                    obj.prvw_data_details = {}
                },
            })
        },
    },
    getters: {
        getActiveQueryMode: () => {
            const { query_mode = '' } =
                QueryResult.find(QueryEditor.getters('getActiveQueryTabId')) || {}
            return query_mode
        },
        // Getters for accessing query data stored in memory
        getActiveUserQueryRes: () => QueryTab.getters('getActiveQueryTabTmp').query_results || {},
        getActivePrvwData: (state, getters, rootState) => mode => {
            const { PRVW_DATA, PRVW_DATA_DETAILS } = rootState.mxsWorkspace.config.QUERY_MODES
            switch (mode) {
                case PRVW_DATA:
                    return QueryTab.getters('getActiveQueryTabTmp').prvw_data || {}
                case PRVW_DATA_DETAILS:
                    return QueryTab.getters('getActiveQueryTabTmp').prvw_data_details || {}
                default:
                    return {}
            }
        },
        // Getters by query_tab_id
        getIsLoading: () => query_tab_id => {
            const { query_results: { is_loading = false } = {} } =
                QueryTabTmp.find(query_tab_id) || {}

            return is_loading
        },
        getHasKillFlag: () => query_tab_id => QueryTabTmp.find(query_tab_id).has_kill_flag,
    },
}
