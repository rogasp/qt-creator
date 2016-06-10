/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "testtreeitem.h"
#include "itestparser.h"

namespace Autotest {
namespace Internal {

class ITestFramework
{
public:
    virtual ~ITestFramework() { }

    virtual const char *name() const = 0;
    virtual unsigned priority() const = 0;          // should this be modifyable?

    TestTreeItem *rootNode()
    {   if (!m_rootNode)
            m_rootNode = createRootNode();
        return m_rootNode;
    }

    ITestParser *testParser()
    {
        if (!m_testParser)
            m_testParser = createTestParser();
        return m_testParser;
    }

protected:
    virtual ITestParser *createTestParser() const = 0;
    virtual TestTreeItem *createRootNode() const = 0;

private:
    TestTreeItem *m_rootNode = 0;
    ITestParser *m_testParser = 0;
};

} // namespace Internal
} // namespace Autotest