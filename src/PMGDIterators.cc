/**
 * @file   PMGDIterators.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "PMGDIterators.h"

using namespace VDMS;

bool PMGDQueryHandler::ReusableNodeIterator::_next()
{
    if (_it != _traversed.end()) {
        ++_it;
        if (_it != _traversed.end())
            return true;
    }
    if (bool(_ni)) {
        _it = _traversed.insert(_traversed.end(), &*_ni);
        _ni.next();
        return true;
    }
    return false;
}

// TODO It might be possible to avoid this if the first iterator
// was build out of an index sorted on the same key been sought here.
// Hopefully that won't happen.
void PMGDQueryHandler::ReusableNodeIterator::sort(PMGD::StringID sortkey)
{
    // First finish traversal
    traverse_all();
    _traversed.sort(compare_propkey{sortkey});
    _it = _traversed.begin();
}

bool PMGDQueryHandler::MultiNeighborIteratorImpl::_next()
{
    while (_start_ni != NULL && bool(*_start_ni)) {
        delete _neighb_i;

        // TODO Maybe unique can have a default value of false.
        // TODO No support in case unique is true but get it from LDBC.
        // Eventually need to add a get_union(NodeIterator, vector<Constraints>)
        // call to PMGD.
        // TODO Any way to skip new?
        _neighb_i = new PMGD::NodeIterator(_search_neighbors.eval_nodes(**_start_ni,
                    _dir, _edge_tag));
        _start_ni->next();
        if (bool(*_neighb_i))
            return true;
    }
    _start_ni = NULL;
    return false;
}

bool PMGDQueryHandler::MultiNeighborIteratorImpl::next()
{
    if (_neighb_i != NULL && bool(*_neighb_i)) {
        _neighb_i->next();
        if (bool(*_neighb_i))
            return true;
    }
    return _next();
}

bool PMGDQueryHandler::ReusableEdgeIterator::_next()
{
    if (_it != _traversed.end()) {
        ++_it;
        if (_it != _traversed.end())
            return true;
    }
    if (bool(_ei)) {
        _it = _traversed.insert(_traversed.end(), &static_cast<PMGD::Edge &>(*_ei));
        _ei.next();
        return true;
    }
    return false;
}

// TODO It might be possible to avoid this if the first iterator
// was build out of an index sorted on the same key been sought here.
// Hopefully that won't happen.
void PMGDQueryHandler::ReusableEdgeIterator::sort(PMGD::StringID sortkey)
{
    // First finish traversal
    traverse_all();
    _traversed.sort(compare_propkey{sortkey});
    _it = _traversed.begin();
}

bool PMGDQueryHandler::NodeEdgeIteratorImpl::next()
{
    _edge_it->next();
    while (_edge_it != NULL && bool(*_edge_it)) {
        if (check_predicates())
            return true;
        _edge_it->next();
    }
    return _next();
}

bool PMGDQueryHandler::NodeEdgeIteratorImpl::_next()
{
    while (_src_ni != NULL && bool(*_src_ni)) {
        delete _edge_it;
        _src_ni->next();
        if (bool(*_src_ni)) {
            _edge_it = new PMGD::EdgeIterator((*_src_ni)->get_edges(_dir, _expr.tag()));
            while (_edge_it != NULL && bool(*_edge_it)) {
                if (check_predicates())
                    return true;
                _edge_it->next();
            }
        }
        else
            break;
    }
    return false;
}

bool PMGDQueryHandler::NodeEdgeIteratorImpl::check_predicates()
{
    PMGD::Edge *e = get_edge();
    for (std::size_t i = _pred_start; i < _num_predicates; i++) {
        PMGD::PropertyFilter<PMGD::Edge> pf(_expr.predicate(i));
        if (pf(*e) == PMGD::DontPass)
            return false;
    }
    if (_check_dest &&
            _dest_nodes.find(&(e->get_destination()) ) == _dest_nodes.end())
        return false;;
    return true;
}


