Import('env')
import shutil

# Wraps the compiler invocations in ccache so repeated builds (edit -> compile -> flash loop)
# reuse object files instead of recompiling unchanged translation units from scratch. Only
# wired into the mbi5153matrix env (see platformio.ini) rather than [env] globally, so it
# doesn't change behavior for anyone building an unrelated upstream WLED environment.
# No-ops cleanly if ccache isn't installed (see .devcontainer/Dockerfile).

ccache_path = shutil.which("ccache")
if ccache_path:
    env["CC"]  = "ccache " + env["CC"]
    env["CXX"] = "ccache " + env["CXX"]
    print("ccache.py: wrapping CC/CXX with ccache ({})".format(ccache_path))
else:
    print("ccache.py: ccache not found on PATH, skipping (apt-get install ccache)")
