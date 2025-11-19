# Development Workflow

## clang-format enforcement

The repository now includes a `pre-push` Git hook (`.githooks/pre-push`) that runs
`clang-format` over every C/C++ file that is part of the push. If any file would
be reformatted, the push is rejected locally so the issue can be fixed before
the code ever leaves your machine.

### One-time setup

1. Make sure `clang-format` (version 10 or newer) is on your `PATH`.
2. Point Git at the shared hooks directory:
   ```bash
   git config core.hooksPath .githooks
   ```

That is it—every future `git push` will automatically run the formatter check.

### Fixing formatting issues

When the hook reports problems you can either run `clang-format -i <file>` on the
reported files or let the helper script handle it for you:

```bash
scripts/format.sh
```

The script formats every tracked C/C++ file we own (it skips `third_party/`). To
verify formatting without applying changes you can run:

```bash
CHECK_ONLY=1 scripts/format.sh
```

### Temporarily skipping the hook

If you have an exceptional case (for example, pushing an experiment or bisect),
set `SKIP_CLANG_FORMAT=1` when invoking `git push`. Please avoid skipping the
hook in regular workflows—consistent formatting makes code reviews much easier.
