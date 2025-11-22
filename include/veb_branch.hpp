#pragma once

#include "veb_branch_dense.hpp"
#include "veb_branch_sparse.hpp"

// Dense 16-bit branch
using VebBranch16 = VebBranch<16, false>;
// Sparse 32-bit branch for use inside 64-bit tree
using VebBranch32 = VebBranch<32, true>;
// Dense top-level 32-bit tree
using VebTop32 = VebBranch<32, false>;
using VebBranch64 = VebBranch<64>;
using VebTop64 = VebBranch64;
