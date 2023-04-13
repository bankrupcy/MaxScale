/*
 * Copyright (c) 2023 MariaDB plc
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

export const LINK_SHAPES = {
    ORTHO: 'Orthogonal',
    ENTITY_RELATION: 'Entity Relation',
    STRAIGHT: 'Straight',
}

export const TARGET_POS = {
    RIGHT: 'right',
    LEFT: 'left',
    INTERSECT: 'intersect',
}

export default () => ({
    link: {
        containerClass: 'link_container',
        pathClass: 'link_path',
        invisiblePathClass: 'link_path__invisible',
        markerClass: 'entity-marker__path',
        /**
         * Path attributes can also be a function
         * e.g. element.attr('stroke', color:(d) => d.linkStyles.color )
         */
        color: '#0e9bc0',
        strokeWidth: 2.5,
        invisibleStrokeWidth: 12,
        dashArr: '5',
        opacity: 0.5,
        hover: {
            strokeWidth: 2.5,
            dashArr: '0',
            opacity: 1,
        },
        dragging: {
            strokeWidth: 2.5,
            dashArr: '0',
            opacity: 1,
        },
    },
    linkShape: {
        type: LINK_SHAPES.ORTHO,
        entitySizeConfig: {
            rowHeight: 32,
            // Reserve 4 px to make sure point won't be at the top or bottom edge of the row
            rowOffset: 4,
            // Ensure that the marker remains visible while dragging a node by allocating a specific width.
            markerWidth: 18,
        },
    },
})

const optionalSymbol = 'M 0 0 a 4 4 0 1 0 8 0 a 4 4 0 1 0 -8 0'
const manySymbol = 'M 8 0 L 18 0 M 8 0 L 18 -5 M 8 0 L 18 5'
const straight = 'M 0 0 L 18 0' // straight line

export const MIN_MAX_CARDINALITY = {
    ONE: '1',
    ONLY_ONE: '1..1',
    ZERO_OR_ONE: '0..1',
    MANY: 'N',
    ONE_OR_MANY: '1..N',
    ZERO_OR_MANY: '0..N',
}

export const CARDINALITY_SYMBOLS = {
    [MIN_MAX_CARDINALITY.ONE]: `${straight} M 13 -5 L 13 5`,
    [MIN_MAX_CARDINALITY.ONLY_ONE]: `${straight} M 8 -5 L 8 5 M 13 -5 L 13 5`,
    [MIN_MAX_CARDINALITY.ZERO_OR_ONE]: `${optionalSymbol} M 8 0 L 18 0 M 13 -5 L 13 5`,
    [MIN_MAX_CARDINALITY.MANY]: `M 0 0 L 8 0 ${manySymbol}`,
    [MIN_MAX_CARDINALITY.ONE_OR_MANY]: `M 0 0 L 8 0 M 8 -5 L 8 5 ${manySymbol}`,
    [MIN_MAX_CARDINALITY.ZERO_OR_MANY]: `${optionalSymbol} ${manySymbol}`,
}
