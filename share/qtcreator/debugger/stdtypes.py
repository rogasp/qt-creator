############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

from dumper import *

def qform__std__array():
    return arrayForms()

def qdump__std__array(d, value):
    size = value.type.templateArgument(1, numeric=True)
    d.putItemCount(size)
    if d.isExpanded():
        d.putPlotData(value.address(), size, value.type[0])


def qform__std____1__array():
    return arrayForms()

def qdump__std____1__array(d, value):
    qdump__std__array(d, value)


def qdump__std__complex(d, value):
    innerType = value.type[0]
    (real, imag) = value.split('{%s}{%s}' % (innerType.name, innerType.name))
    d.putValue("(%s, %s)" % (real.display(), imag.display()))
    if d.isExpanded():
        with Children(d, 2, childType=innerType):
            d.putSubItem("real", real)
            d.putSubItem("imag", imag)

def qdump__std____1__complex(d, value):
    qdump__std__complex(d, value)


def qdump__std__deque(d, value):
    if d.isQnxTarget():
        qdump__std__deque__QNX(d, value)
        return

    innerType = value.type[0]
    innerSize = innerType.size()
    bufsize = 1
    if innerSize < 512:
        bufsize = int(512 / innerSize)

    #(mapptr, mapsize, startCur, startFirst, startLast, startNode,
    #          finishCur, finishFirst, finishLast, finishNode) = d.split("pppppppppp", value)
#
#    numInBuf = bufsize * (int((finishNode - startNode) / innerSize) - 1)
#    numFirstItems  = int((startLast - startCur) / innerSize)
#    numLastItems = int((finishCur - finishFirst) / innerSize)

    impl = value["_M_impl"]
    start = impl["_M_start"]
    finish = impl["_M_finish"]
    size = bufsize * int((finish["_M_node"].integer() - start["_M_node"].integer()) / d.ptrSize() - 1)
    size += int((finish["_M_cur"].integer() - finish["_M_first"].integer()) / innerSize)
    size += int((start["_M_last"].integer() - start["_M_cur"].integer()) / innerSize)

    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    if d.isExpanded():
        with Children(d, size, maxNumChild=2000, childType=innerType):
            pcur = start["_M_cur"].integer()
            pfirst = start["_M_first"]
            plast = start["_M_last"].integer()
            pnode = start["_M_node"]
            for i in d.childRange():
                d.putSubItem(i, d.createValue(pcur, innerType))
                pcur += innerSize
                if pcur == plast:
                    # FIXME: Remove pointer operation.
                    newnode = pnode + 1 # Type is std::_Deque_iterator<Foo, Foo&, Foo*>::_Map_pointer\"} a.k.a 'Foo **'
                    #warn("TYPE: %s" % pnode.type)
                    #warn("PNODE: 0x%x %s" % (pnode.pointer(), pnode))
                    #warn("NEWNODE: 0x%x %s" % (newnode.pointer(), newnode))
                    pnode = newnode
                    #warn("PNODE 2: 0x%x %s" % (pnode.pointer(), pnode))
                    pfirst = newnode.dereference().integer()
                    plast = pfirst + bufsize * d.ptrSize()
                    pcur = pfirst

def qdump__std__deque__QNX(d, value):
    innerType = value.type[0]
    innerSize = innerType.size()
    if innerSize <= 1:
        bufsize = 16
    elif innerSize <= 2:
        bufsize = 8
    elif innerSize <= 4:
        bufsize = 4
    elif innerSize <= 8:
        bufsize = 2
    else:
        bufsize = 1

    myoff = value['_Myoff']
    mysize = value['_Mysize']
    mapsize = value['_Mapsize']

    d.check(0 <= mapsize and mapsize <= 1000 * 1000 * 1000)
    d.putItemCount(mysize)
    if d.isExpanded():
        with Children(d, mysize, maxNumChild=2000, childType=innerType):
            map = value['_Map']
            for i in d.childRange():
                block = myoff / bufsize
                offset = myoff - (block * bufsize)
                if mapsize <= block:
                    block -= mapsize
                d.putSubItem(i, map[block][offset])
                myoff += 1;

def qdump__std____debug__deque(d, value):
    qdump__std__deque(d, value)


def qdump__std__list(d, value):
    if d.isQnxTarget():
        qdump__std__list__QNX(d, value)
        return

    if value.type.size() == 3 * d.ptrSize():
        # C++11 only.
        (dummy1, dummy2, size) = value.split("ppp")
        d.putItemCount(size)
    else:
        # Need to count manually.
        p = d.extractPointer(value)
        head = value.address
        size = 0
        while head != p and size < 1001:
            size += 1
            p = d.extractPointer(p)
        d.putItemCount(size, 1000)

    if d.isExpanded():
        p = d.extractPointer(value)
        innerType = value.type[0]
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + 2 * d.ptrSize(), innerType))
                p = d.extractPointer(p)

def qdump__std__list__QNX(d, value):
    node = value["_Myhead"]
    size = value["_Mysize"]

    d.putItemCount(size, 1000)

    if d.isExpanded():
        p = node["_Next"]
        with Children(d, size, maxNumChild=1000, childType=value.type[0]):
            for i in d.childRange():
                d.putSubItem(i, p['_Myval'])
                p = p["_Next"]

def qdump__std____debug__list(d, value):
    qdump__std__list(d, value)

def qdump__std____cxx11__list(d, value):
    qdump__std__list(d, value)

def qdump__std____1__list(d, value):
    if value.type.size() == 3 * d.ptrSize():
        # C++11 only.
        (dummy1, dummy2, size) = value.split("ppp")
        d.putItemCount(size)
    else:
        # Need to count manually.
        p = d.extractPointer(value)
        head = value.address()
        size = 0
        while head != p and size < 1001:
            size += 1
            p = d.extractPointer(p)
        d.putItemCount(size, 1000)

    if d.isExpanded():
        (prev, p) = value.split("pp")
        innerType = value.type[0]
        typeCode = "pp{%s}" % innerType.name
        with Children(d, size, maxNumChild=1000, childType=innerType):
            for i in d.childRange():
                (prev, p, val) = d.split(typeCode, p)
                d.putSubItem(i, val)

def qform__std__map():
    return mapForms()

def qdump__std__map(d, value):
    if d.isQnxTarget():
        qdump__std__map__QNX(d, value)
        return

    # stuff is actually (color, pad) with 'I@', but we can save cycles/
    (compare, stuff, parent, left, right, size) = value.split('pppppp')
    impl = value["_M_t"]["_M_impl"]
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)

    if d.isExpanded():
        pairType = value.type[3][0]
        pairPointer = pairType.pointer()
        with PairedChildren(d, size, pairType=pairType, maxNumChild=1000):
            node = impl["_M_header"]["_M_left"]
            typeCode = "p@{%s}@{%s}" % (pairType[0].name, pairType[1].name)
            for i in d.childRange():
                pair = (node + 1).cast(pairPointer).dereference()
                d.putPairItem(i, pair)
                #(pp, pad1, key, pad2, value) = d.split(typeCode, node.integer())
                #d.putPairItem(i, (key, value))
                if node["_M_right"].integer() == 0:
                    parent = node["_M_parent"]
                    while True:
                        if node.integer() != parent["_M_right"].integer():
                            break
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while True:
                        if node["_M_left"].integer() == 0:
                            break
                        node = node["_M_left"]

def qdump__std__map__QNX(d, value):
    size = value['_Mysize']
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)

    if d.isExpanded():
        keyType = value.type[0]
        valueType = value.type[1]
        try:
            # Does not work on gcc 4.4, the allocator type
            # (fourth template argument) is not available.
            pairType = value.type[3][0]
            pairPointer = pairType.pointer()
        except:
            # So use this as workaround:
            pairType = impl.type[1]
            pairPointer = pairType.pointer()
        isCompact = d.isMapCompact(keyType, valueType)
        innerType = pairType
        if isCompact:
            innerType = valueType
        head = value['_Myhead']
        node = head['_Left']
        nodeType = head.type
        childType = innerType
        if size == 0:
            childType = pairType
        childNumChild = 2
        if isCompact:
            childNumChild = None
        with Children(d, size, maxNumChild=1000,
                childType=childType, childNumChild=childNumChild):
            for i in d.childRange():
                pair = node.cast(nodeType).dereference()['_Myval']
                d.putPairItem(i, pair)
                if not node['_Right']['_Isnil']:
                    node = node['_Right']
                    while not node['_Left']['_Isnil']:
                        node = node['_Left']
                else:
                    parent = node['_Parent']
                    while node == parent['_Right']['_Isnil']:
                        node = parent
                        parent = parent['_Parent']
                    if node['_Right'] != parent:
                        node = parent

def qdump__std____debug__map(d, value):
    qdump__std__map(d, value)

def qdump__std____debug__set(d, value):
    qdump__std__set(d, value)

def qdump__std__multiset(d, value):
    qdump__std__set(d, value)

def qdump__std____cxx1998__map(d, value):
    qdump__std__map(d, value)

def qform__std__multimap():
    return mapForms()

def qdump__std__multimap(d, value):
    return qdump__std__map(d, value)

def qdumpHelper__std__tree__iterator(d, value, isSet=False):
    if value.type.name.endswith("::iterator"):
        treeTypeName = value.type.name[:-len("::iterator")]
    elif value.type.name.endswith("::const_iterator"):
        treeTypeName = value.type.name[:-len("::const_iterator")]
    treeType = d.lookupType(treeTypeName)
    keyType = treeType[0]
    valueType = treeType[1]
    node = value["_M_node"].dereference()   # std::_Rb_tree_node_base
    d.putNumChild(1)
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            if isSet:
                typecode = 'pppp@{%s}' % keyType.name
                (color, parent, left, right, pad1, key) = d.split(typecode, node)
                d.putSubItem("value", key)
            else:
                typecode = 'pppp@{%s}@{%s}' % (keyType.name, valueType.name)
                (color, parent, left, right, pad1, key, pad2, value) = d.split(typecode, node)
                d.putSubItem("first", key)
                d.putSubItem("second", value)
            with SubItem(d, "[node]"):
                d.putNumChild(1)
                d.putEmptyValue()
                d.putType(" ")
                if d.isExpanded():
                    with Children(d):
                        #d.putSubItem("color", color)
                        nodeType = node.type.pointer()
                        d.putSubItem("left", d.createValue(left, nodeType))
                        d.putSubItem("right", d.createValue(right, nodeType))
                        d.putSubItem("parent", d.createValue(parent, nodeType))

def qdump__std___Rb_tree_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)

def qdump__std___Rb_tree_const_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)

def qdump__std__map__iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)

def qdump____gnu_debug___Safe_iterator(d, value):
    d.putItem(value["_M_current"])

def qdump__std__map__const_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value)

def qdump__std__set__iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value, True)

def qdump__std__set__const_iterator(d, value):
    qdumpHelper__std__tree__iterator(d, value, True)

def qdump__std____cxx1998__set(d, value):
    qdump__std__set(d, value)

def qdump__std__set(d, value):
    if d.isQnxTarget():
        qdump__std__set__QNX(d, value)
        return

    impl = value["_M_t"]["_M_impl"]
    size = impl["_M_node_count"].integer()
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    if d.isExpanded():
        valueType = value.type[0]
        node = impl["_M_header"]["_M_left"]
        with Children(d, size, maxNumChild=1000, childType=valueType):
            for i in d.childRange():
                d.putSubItem(i, (node + 1).cast(valueType.pointer()).dereference())
                if node["_M_right"].integer() == 0:
                    parent = node["_M_parent"]
                    while node == parent["_M_right"]:
                        node = parent
                        parent = parent["_M_parent"]
                    if node["_M_right"] != parent:
                        node = parent
                else:
                    node = node["_M_right"]
                    while node["_M_left"].integer() != 0:
                        node = node["_M_left"]

def qdump__std__set__QNX(d, value):
    size = value['_Mysize']
    d.check(0 <= size and size <= 100*1000*1000)
    d.putItemCount(size)
    if d.isExpanded():
        head = value['_Myhead']
        node = head['_Left']
        nodeType = head.type
        with Children(d, size, maxNumChild=1000, childType=value.type[0]):
            for i in d.childRange():
                d.putSubItem(i, node.cast(nodeType).dereference()['_Myval'])
                if not node['_Right']['_Isnil']:
                    node = node['_Right']
                    while not node['_Left']['_Isnil']:
                        node = node['_Left']
                else:
                    parent = node['_Parent']
                    while node == parent['_Right']['_Isnil']:
                        node = parent
                        parent = parent['_Parent']
                    if node['_Right'] != parent:
                        node = parent

def std1TreeMin(d, node):
    #_NodePtr __tree_min(_NodePtr __x):
    #    while (__x->__left_ != nullptr)
    #        __x = __x->__left_;
    #    return __x;
    #
    left = node['__left_']
    if left.integer():
        node = left
    return node

def std1TreeIsLeftChild(d, node):
    # bool __tree_is_left_child(_NodePtr __x):
    #     return __x == __x->__parent_->__left_;
    #
    other = node['__parent_']['__left_']
    return node.integer() == other.integer()


def std1TreeNext(d, node):
    #_NodePtr __tree_next(_NodePtr __x):
    #    if (__x->__right_ != nullptr)
    #        return __tree_min(__x->__right_);
    #    while (!__tree_is_left_child(__x))
    #        __x = __x->__parent_;
    #    return __x->__parent_;
    #
    right = node['__right_']
    if right.integer():
        return std1TreeMin(d, right)
    while not std1TreeIsLeftChild(d, node):
        node = node['__parent_']
    return node['__parent_']

def qdump__std____1__set(d, value):
    tree = value["__tree_"]
    base3 = tree["__pair3_"].address()
    size = d.extractUInt(base3)
    d.check(size <= 100*1000*1000)
    d.putItemCount(size)
    if d.isExpanded():
        # type of node is std::__1::__tree_node<Foo, void *>::value_type
        valueType = value.type[0]
        d.putFields(tree)
        node = tree["__begin_node_"]
        nodeType = node.type
        with Children(d, size):
            for i in d.childRange():
                with SubItem(d, i):
                    d.putItem(node['__value_'])
                    d.putBetterType(valueType)
                node = std1TreeNext(d, node).cast(nodeType)

def qform__std____1__map():
    return mapForms()

def qdump__std____1__map(d, value):
    tree = value["__tree_"]
    base3 = tree["__pair3_"].address()
    size = d.extractUInt(base3)
    d.check(size <= 100*1000*1000)
    d.putItemCount(size)
    if d.isExpanded():
        # type of node is std::__1::__tree_node<Foo, Bar>::value_type
        valueType = value.type[0]
        node = tree["__begin_node_"]
        nodeType = node.type
        pairType = value.type[3][0]
        with PairedChildren(d, size, pairType=pairType, maxNumChild=1000):
            node = tree["__begin_node_"]
            nodeType = node.type
            for i in d.childRange():
                # There's possibly also:
                #pair = node['__value_']['__nc']
                pair = node['__value_']['__cc']
                d.putPairItem(i, pair)
                node = std1TreeNext(d, node).cast(nodeType)

def qform__std____1__multimap():
    return mapForms()

def qdump__std____1__multimap(d, value):
    qdump__std____1__map(d, value)

def qdump__std__stack(d, value):
    d.putItem(value["c"])
    d.putBetterType(value.type)

def qdump__std____debug__stack(d, value):
    qdump__std__stack(d, value)

def qform__std__string():
    return [Latin1StringFormat, SeparateLatin1StringFormat,
            Utf8StringFormat, SeparateUtf8StringFormat ]

def qdump__std__string(d, value):
    qdumpHelper_std__string(d, value, d.createType("char"), d.currentItemFormat())

def qdumpHelper_std__string(d, value, charType, format):
    if d.isQnxTarget():
        qdumpHelper__std__string__QNX(d, value, charType, format)
        return

    data = value.extractPointer()
    # We can't lookup the std::string::_Rep type without crashing LLDB,
    # so hard-code assumption on member position
    # struct { size_type _M_length, size_type _M_capacity, int _M_refcount; }
    (size, alloc, refcount) = d.split("ppp", data - 3 * d.ptrSize())
    refcount = refcount & 0xffffffff
    d.check(refcount >= -1) # Can be -1 according to docs.
    d.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    d.putCharArrayHelper(data, size, charType, format)

def qdumpHelper__std__string__QNX(d, value, charType, format):
    size = value['_Mysize']
    alloc = value['_Myres']
    _BUF_SIZE = int(16 / charType.size())
    if _BUF_SIZE <= alloc: #(_BUF_SIZE <= _Myres ? _Bx._Ptr : _Bx._Buf);
        data = value['_Bx']['_Ptr']
    else:
        data = value['_Bx']['_Buf']
    sizePtr = data.cast(d.charType().pointer())
    refcount = int(sizePtr[-1])
    d.check(refcount >= -1) # Can be -1 accoring to docs.
    d.check(0 <= size and size <= alloc and alloc <= 100*1000*1000)
    d.putCharArrayHelper(sizePtr, size, charType, format)


def qdump__std____1__string(d, value):
    firstByte = value.split('b')[0]
    if int(firstByte & 1) == 0:
        # Short/internal.
        size = int(firstByte / 2)
        data = value.address() + 1
    else:
        # Long/external.
        (dummy, size, data) = value.split('ppp')
    d.putCharArrayHelper(data, size, d.charType(), d.currentItemFormat())
    d.putType("std::string")


def qdump__std____1__wstring(d, value):
    firstByte = value.split('b')[0]
    if int(firstByte & 1) == 0:
        # Short/internal.
        size = int(firstByte / 2)
        data = value.address() + 4
    else:
        # Long/external.
        (dummy, size, data) = value.split('ppp')
    d.putCharArrayHelper(data, size, d.createType('wchar_t'))
    d.putType("std::wstring")


def qdump__std__shared_ptr(d, value):
    i = value["_M_ptr"]
    if i.integer() == 0:
        d.putValue("(null)")
        d.putNumChild(0)
        return
    d.putValue(i.dereference().simpleDisplay())
    d.putNumChild(3)
    with Children(d, 3):
        d.putSubItem("data", i)
        refcount = value["_M_refcount"]["_M_pi"]
        d.putIntItem("usecount", refcount["_M_use_count"])
        d.putIntItem("weakcount", refcount["_M_weak_count"])

def qdump__std____1__shared_ptr(d, value):
    i = value["__ptr_"]
    if i.integer() == 0:
        d.putValue("(null)")
        d.putNumChild(0)
        return
    d.putValue(i.dereference().simpleDisplay())
    d.putNumChild(3)
    with Children(d, 3):
        d.putSubItem("data", i.dereference())
        d.putFields(value["__cntrl_"].dereference())
        #d.putIntItem("usecount", refcount["_M_use_count"])
        #d.putIntItem("weakcount", refcount["_M_weak_count"])

def qdump__std__unique_ptr(d, value):
    p = d.extractPointer(value)
    if p == 0:
        d.putValue("(null)")
        d.putNumChild(0)
        return
    val = d.createValue(p, value.type[0])
    d.putValue(val.simpleDisplay())
    d.putNumChild(1)
    with Children(d, 1):
        d.putSubItem("data", val)

def qdump__std____1__unique_ptr(d, value):
    qdump__std__unique_ptr(d, value)


def qform__std__unordered_map():
    return mapForms()

def qform__std____debug__unordered_map():
    return mapForms()

def qdump__std__unordered_map(d, value):
    keyType = value.type[0]
    valueType = value.type[1]
    allocatorType = value.type[4]
    pairType = allocatorType[0]
    ptrSize = d.ptrSize()
    try:
        # gcc ~= 4.7
        size = value["_M_element_count"].integer()
        start = value["_M_before_begin"]["_M_nxt"]
        offset = 0
    except:
        try:
            # libc++ (Mac)
            size = value["_M_h"]["_M_element_count"].integer()
            start = value["_M_h"]["_M_bbegin"]["_M_node"]["_M_nxt"]
            offset = 0
        except:
            try:
                # gcc 4.9.1
                size = value["_M_h"]["_M_element_count"].integer()
                start = value["_M_h"]["_M_before_begin"]["_M_nxt"]
                offset = 0
            except:
                # gcc 4.6.2
                size = value["_M_element_count"].integer()
                start = value["_M_buckets"].dereference()
                # FIXME: Pointer-aligned?
                offset = pairType.size()
                d.putItemCount(size)
                # We don't know where the data is
                d.putNumChild(0)
                return

    d.putItemCount(size)
    if d.isExpanded():
        p = start.integer()
        if d.isMapCompact(keyType, valueType):
            with PairedChildren(d, size, pairType=pairType):
                for i in d.childRange():
                    pair = d.createValue(p + ptrSize, pairType)
                    d.putPairItem(i, pair)
                    p = d.extractPointer(p)
        else:
            with Children(d, size, childType=pairType):
                for i in d.childRange():
                    d.putSubItem(i, d.createValue(p + ptrSize - offset, pairType))
                    p = d.extractPointer(p + offset)

def qdump__std____debug__unordered_map(d, value):
    qdump__std__unordered_map(d, value)

def qdump__std__unordered_set(d, value):
    try:
        # gcc ~= 4.7
        size = value["_M_element_count"].integer()
        start = value["_M_before_begin"]["_M_nxt"]
        offset = 0
    except:
        try:
            # libc++ (Mac)
            size = value["_M_h"]["_M_element_count"].integer()
            start = value["_M_h"]["_M_bbegin"]["_M_node"]["_M_nxt"]
            offset = 0
        except:
            try:
                # gcc 4.6.2
                size = value["_M_element_count"].integer()
                start = value["_M_buckets"].dereference()
                offset = d.ptrSize()
            except:
                # gcc 4.9.1
                size = value["_M_h"]["_M_element_count"].integer()
                start = value["_M_h"]["_M_before_begin"]["_M_nxt"]
                offset = 0

    d.putItemCount(size)
    if d.isExpanded():
        p = start.integer()
        valueType = value.type[0]
        with Children(d, size, childType=valueType):
            ptrSize = d.ptrSize()
            for i in d.childRange():
                d.putSubItem(i, d.createValue(p + ptrSize - offset, valueType))
                p = d.extractPointer(p + offset)

def qform__std____1__unordered_map():
    return mapForms()

def qdump__std____1__unordered_map(d, value):
    size = value["__table_"]["__p2_"]["__first_"].integer()
    d.putItemCount(size)
    if d.isExpanded():
        # There seem to be several versions of the implementation.
        def valueCCorNot(val):
            try:
                return val["__cc"]
            except:
                return val

        node = value["__table_"]["__p1_"]["__first_"]["__next_"]
        with PairedChildren(d, size, pairType=valueCCorNot(node["__value_"]).type):
            for i in d.childRange():
                d.putPairItem(i, valueCCorNot(node["__value_"]))
                node = node["__next_"]


def qdump__std____1__unordered_set(d, value):
    size = int(value["__table_"]["__p2_"]["__first_"])
    d.putItemCount(size)
    if d.isExpanded():
        node = value["__table_"]["__p1_"]["__first_"]["__next_"]
        with Children(d, size, childType=value.type[0], maxNumChild=1000):
            for i in d.childRange():
                d.putSubItem(i, node["__value_"])
                node = node["__next_"]


def qdump__std____debug__unordered_set(d, value):
    qdump__std__unordered_set(d, value)


def qform__std__valarray():
    return arrayForms()

def qdump__std__valarray(d, value):
    (size, data) = value.split('pp')
    d.putItemCount(size)
    d.putPlotData(data, size, value.type[0])


def qform__std____1__valarray():
    return arrayForms()

def qdump__std____1__valarray(d, value):
    innerType = value.type[0]
    (begin, end) = value.split('pp')
    size = int((end - begin) / innerType.size())
    d.putItemCount(size)
    d.putPlotData(begin, size, innerType)


def qform__std__vector():
    return arrayForms()

def qedit__std__vector(d, value, data):
    import gdb
    values = data.split(',')
    n = len(values)
    innerType = value.type[0]
    cmd = "set $d = (%s*)calloc(sizeof(%s)*%s,1)" % (innerType, innerType, n)
    gdb.execute(cmd)
    cmd = "set {void*[3]}%s = {$d, $d+%s, $d+%s}" % (value.address, n, n)
    gdb.execute(cmd)
    cmd = "set (%s[%d])*$d={%s}" % (innerType, n, data)
    gdb.execute(cmd)

def qdump__std__vector(d, value):
    if d.isQnxTarget():
        qdumpHelper__std__vector__QNX(d, value)
    else:
        qdumpHelper__std__vector(d, value, False)

def qdumpHelper__std__vector(d, value, isLibCpp):
    innerType = value.type[0]
    isBool = innerType.name == 'bool'
    if isBool:
        if isLibCpp:
            (start, size) = value.split("pp")  # start is 'unsigned long *'
            alloc = size
        else:
            (start, soffset, pad, finish, foffset, pad, alloc) = value.split("pI@pI@p")
            size = (finish - start) * 8 + foffset - soffset # 8 is CHAR_BIT.
    else:
        (start, finish, alloc) = value.split("ppp")
        size = int((finish - start) / innerType.size())
        d.check(finish <= alloc)
        if size > 0:
            d.checkPointer(start)
            d.checkPointer(finish)
            d.checkPointer(alloc)

    d.check(0 <= size and size <= 1000 * 1000 * 1000)

    d.putItemCount(size)
    if isBool:
        if d.isExpanded():
            with Children(d, size, maxNumChild=10000, childType=innerType):
                for i in d.childRange():
                    q = start + int(i / 8)
                    with SubItem(d, i):
                        d.putValue((int(d.extractPointer(q)) >> (i % 8)) & 1)
                        d.putType("bool")
                        d.putNumChild(0)
    else:
        d.putPlotData(start, size, innerType)

def qdumpHelper__std__vector__QNX(d, value):
    innerType = value.type[0]
    isBool = str(innerType) == 'bool'
    if isBool:
        impl = value['_Myvec']
        start = impl['_Myfirst']
        last = impl['_Mylast']
        end = impl['_Myend']
        size = value['_Mysize']
        storagesize = start.dereference().type.size() * 8
    else:
        start = value['_Myfirst']
        last = value['_Mylast']
        end = value['_Myend']
        size = (last.integer() - start.integer()) / innerType.size()
        alloc = (end.integer() - start.integer()) / innerType.size()

    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.check(last <= end)
    d.checkPointer(start)
    d.checkPointer(last)
    d.checkPointer(end)

    d.putItemCount(size)
    if d.isExpanded():
        if isBool:
            with Children(d, size, maxNumChild=10000, childType=innerType):
                for i in d.childRange():
                    q = start + int(i / storagesize)
                    with SubItem(d, i):
                        d.putValue((q.dereference() >> (i % storagesize)) & 1)
                        d.putType("bool")
                        d.putNumChild(0)
        else:
            d.putArrayData(start, size, innerType)

def qdump__std____1__vector(d, value):
    qdumpHelper__std__vector(d, value, True)

def qform__std____debug__vector():
    return arrayForms()

def qdump__std____debug__vector(d, value):
    qdump__std__vector(d, value)


def qedit__std__string(d, value, data):
    d.call(value, "assign", '"%s"' % data.replace('"', '\\"'))

def qedit__string(d, expr, value):
    qedit__std__string(d, expr, value)

def qdump__string(d, value):
    qdump__std__string(d, value)

def qform__std__wstring():
    return [SimpleFormat, SeparateFormat]

def qdump__std__wstring(d, value):
    qdumpHelper_std__string(d, value, d.createType('wchar_t'), d.currentItemFormat())

def qdump__std__basic_string(d, value):
    innerType = value.type[0]
    qdumpHelper_std__string(d, value, innerType, d.currentItemFormat())

def qdump__std____cxx11__basic_string(d, value):
    innerType = value.type[0]
    (data, size) = value.split("pI")
    d.check(0 <= size) #and size <= alloc and alloc <= 100*1000*1000)
    d.putCharArrayHelper(data, size, innerType, d.currentItemFormat())

def qform__std____cxx11__string(d, value):
    qform__std__string(d, value)

def qdump__std____cxx11__string(d, value):
    (data, size) = value.split("pI")
    d.check(0 <= size) #and size <= alloc and alloc <= 100*1000*1000)
    d.putCharArrayHelper(data, size, d.charType(), d.currentItemFormat())

# Needed only to trigger the form report above.
def qform__std____cxx11__string():
    return qform__std__string()

def qform__std____cxx11__wstring():
    return qform__std__wstring()

def qdump__std____1__basic_string(d, value):
    innerType = value.type[0].name
    if innerType == "char":
        qdump__std____1__string(d, value)
    elif innerType == "wchar_t":
        qdump__std____1__wstring(d, value)
    else:
        warn("UNKNOWN INNER TYPE %s" % innerType)

def qdump__wstring(d, value):
    qdump__std__wstring(d, value)


def qdump____gnu_cxx__hash_set(d, value):
    ht = value["_M_ht"]
    size = ht["_M_num_elements"].integer()
    d.check(0 <= size and size <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    innerType = value.type[0]
    d.putType("__gnu__cxx::hash_set<%s>" % innerType.name)
    if d.isExpanded():
        with Children(d, size, maxNumChild=1000, childType=innerType):
            buckets = ht["_M_buckets"]["_M_impl"]
            bucketStart = buckets["_M_start"]
            bucketFinish = buckets["_M_finish"]
            p = bucketStart
            itemCount = 0
            for i in xrange(int((bucketFinish.integer() - bucketStart.integer()) / d.ptrSize())):
                if p.dereference().integer():
                    cur = p.dereference()
                    while cur.integer():
                        d.putSubItem(itemCount, cur["_M_val"])
                        cur = cur["_M_next"]
                        itemCount += 1
                p = p + 1


def qdump__uint8_t(d, value):
    d.putNumChild(0)
    d.putValue(value.integer())

def qdump__int8_t(d, value):
    d.putNumChild(0)
    d.putValue(value.integer())

