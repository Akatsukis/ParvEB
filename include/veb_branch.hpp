#pragma once

#include "veb_branch_dense.hpp"
#include "veb_branch_sparse.hpp"

// Dense 12-bit branch
using VebBranch12 = VebBranch<12, false>;
// Sparse 24-bit branch for use inside 64-bit tree
using VebBranch24 = VebBranch<24, true>;
// Dense top-level 24-bit tree
using VebTop24 = VebBranch<24, false>;
// Sparse 32-bit branch for use inside 64-bit tree
using VebBranch32 = VebBranch<32, true>;
// Dense top-level 32-bit tree
using VebTop32 = VebBranch<32, false>;
using VebBranch48 = VebBranch<48>;
using VebTop48 = VebBranch48;
using VebBranch64 = VebBranch<64>;
using VebTop64 = VebBranch64;
