#pragma once

#include "veb_branch.hpp"
#include "veb_leaf64.hpp"

using VebNode6 = Leaf64;
using VebNode12 = Branch64<VebNode6, VebNode6::SUBTREE_BITS>;
using VebNode18 = Branch64<VebNode12, VebNode12::SUBTREE_BITS>;
using VebNode24 = Branch64<VebNode18, VebNode18::SUBTREE_BITS>;
