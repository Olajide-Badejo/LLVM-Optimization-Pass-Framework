import os

import lit.formats

# The suite runs RUN lines as shell scripts, exactly like the in tree LLVM
# tests, so the FileCheck idioms transfer without translation.
config.name = "opf"
config.test_format = lit.formats.ShTest(execute_external=True)
config.suffixes = [".ll"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = getattr(config, "opf_obj_root", config.test_source_root)

# Put the pinned LLVM tool directory first on PATH so a bare "opt" or
# "FileCheck" in a RUN line resolves to the pinned distribution and nothing
# else on the system.
llvm_tools_dir = getattr(config, "llvm_tools_dir", "")
if llvm_tools_dir:
    config.environment["PATH"] = os.pathsep.join(
        [llvm_tools_dir, config.environment.get("PATH", "")]
    )

# %opflib expands to the freshly built plugin so every test loads the code under
# test rather than a stale copy.
config.substitutions.append(("%opflib", getattr(config, "opf_plugin", "")))
