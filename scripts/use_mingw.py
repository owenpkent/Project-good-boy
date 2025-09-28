# Force PlatformIO native environment to use PlatformIO-provided MinGW GCC on Windows
# Referenced by platformio.ini -> [env:native] extra_scripts = scripts/use_mingw.py

import os
import shutil
from os.path import join, exists
from SCons.Script import (  # type: ignore
    DefaultEnvironment,
)

env = DefaultEnvironment()

pkg_dir = env.PioPlatform().get_package_dir("toolchain-gccmingw32")
if not pkg_dir:
    print("[use_mingw] toolchain-gccmingw32 not found; PlatformIO will attempt to install it.")
else:
    bin_dir = join(pkg_dir, "bin")
    # Prepend MinGW bin to PATH so gcc/g++ resolve
    env.PrependENVPath("PATH", bin_dir)
    # Explicitly set tools to avoid PATH issues
    env.Replace(
        CC=join(bin_dir, "gcc" + (".exe" if os.name == "nt" else "")),
        CXX=join(bin_dir, "g++" + (".exe" if os.name == "nt" else "")),
        AR=join(bin_dir, "ar" + (".exe" if os.name == "nt" else "")),
        RANLIB=join(bin_dir, "ranlib" + (".exe" if os.name == "nt" else "")),
        AS=join(bin_dir, "as" + (".exe" if os.name == "nt" else "")),
        LINK=join(bin_dir, "g++" + (".exe" if os.name == "nt" else "")),
    )

    # On Windows, ensure test runtime DLLs are alongside the executable so `pio test` can run it
    if os.name == "nt":
        def _find_dlls(root_dir):
            targets = {
                "libstdc++-6.dll": None,
                "libwinpthread-1.dll": None,
                # libgcc_s*.dll: pick first we find (dw2/sjlj/seh)
                "libgcc_s": None,
            }
            for base, _, files in os.walk(root_dir):
                for f in files:
                    fl = f.lower()
                    if fl == "libstdc++-6.dll" and not targets["libstdc++-6.dll"]:
                        targets["libstdc++-6.dll"] = join(base, f)
                    if fl == "libwinpthread-1.dll" and not targets["libwinpthread-1.dll"]:
                        targets["libwinpthread-1.dll"] = join(base, f)
                    if fl.startswith("libgcc_s") and fl.endswith('.dll') and not targets["libgcc_s"]:
                        targets["libgcc_s"] = join(base, f)
            return targets

        def _copy_dlls(target, source, env):
            build_dir = env.subst("$BUILD_DIR")
            # Ensure destination exists
            try:
                os.makedirs(build_dir, exist_ok=True)
            except Exception:
                pass
            copied = []
            # Discover DLLs recursively under the MinGW package dir for robustness
            found = _find_dlls(pkg_dir)
            for key, src in found.items():
                if not src or not exists(src):
                    continue
                dll = os.path.basename(src)
                dst = join(build_dir, dll)
                try:
                    shutil.copy2(src, dst)
                    copied.append(dll)
                except Exception as e:
                    print(f"[use_mingw] Warning: failed to copy {dll}: {e}")
            if copied:
                print("[use_mingw] Copied DLLs to build dir:", ", ".join(copied))

        # After the program is linked, copy the DLLs next to it (be explicit about the path)
        program_target = join("$BUILD_DIR", "${PROGNAME}${PROGSUFFIX}")
        env.AddPostAction(program_target, _copy_dlls)
        # Also hook generic path if available
        try:
            env.AddPostAction("$PROG_PATH", _copy_dlls)
        except Exception:
            pass

        # Also attempt to copy immediately (fallback when nothing is rebuilt)
        try:
            _copy_dlls(None, None, env)
        except Exception as e:
            print(f"[use_mingw] Immediate DLL copy attempt failed: {e}")

print("[use_mingw] Configured MinGW toolchain for native environment.")
