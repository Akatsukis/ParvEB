#pragma once

#include "veb_branch_dense.hpp"
#include "veb_branch_sparse.hpp"

// Dense 12-bit branch
using VebBranch12 = VebBranch<12, false>;
// Sparse 24-bit branch for use inside 64-bit tree
using VebBranch24 = VebBranch<24, true>;
// Dense top-level 24-bit tree
using VebTop24 = VebBranch<24, false>;
using VebBranch48 = VebBranch<48>;
using VebTop48 = VebBranch48;
