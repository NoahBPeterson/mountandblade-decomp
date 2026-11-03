"""Windows-native VC7.1 toolchain and the few OS calls the tools share (the Mac runs cl.exe / link.exe under wine).

On sys.platform == "win32" the tools run work/toolchain/vc71tk/bin/cl.exe and link.exe directly, with the same INCLUDE and
LIB order as tools/matching/cl.sh and native paths (no "Z:" drive). On other platforms nothing here changes a command line:
winpath() is the wine path and CL is cl.sh. See docs/remote_worker.md.
"""
import os, sys, time

WIN = sys.platform == "win32"
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TC = os.path.join(ROOT, "work", "toolchain")
VC = os.path.join(TC, "vc71tk")
CL = os.path.join(VC, "bin", "cl.exe") if WIN else os.path.join(ROOT, "tools", "matching", "cl.sh")


def winpath(p):
    """p as the compiler sees it: wine's Z: view of a Unix path, or the native path itself."""
    return p.replace("/", "\\") if WIN else "Z:" + p.replace("/", "\\")


def include_dirs():
    """cl.sh's INCLUDE order (see the comments there for why each directory is where it is)."""
    return [os.path.join(ROOT, "src", "include"), os.path.join(ROOT, "match", "include"), os.path.join(VC, "include"),
            os.path.join(TC, "sal"), os.path.join(TC, "vc9sp1", "WinSDK", "Include"),
            os.path.join(ROOT, "work", "ext", "dxsdk_jun10", "DXSDK", "Include")]


def lib_dirs(link=False):
    """cl.sh's LIB; link.exe (trial_link) also searches the DirectX SDK libraries last."""
    d = [os.path.join(VC, "lib"), os.path.join(TC, "vc9sp1", "WinSDK", "Lib")]
    if link:
        d.append(os.path.join(ROOT, "work", "ext", "dxsdk_jun10", "DXSDK", "Lib", "x86"))
    return d


def toolchain_env(link=False):
    """Environment for a native cl.exe / link.exe run."""
    return dict(os.environ, INCLUDE=";".join(include_dirs()), LIB=";".join(lib_dirs(link)),
                PATH=os.path.join(VC, "bin") + os.pathsep + os.environ.get("PATH", ""))


if WIN:
    # every subprocess of a tool (cl.exe from slice.compile_obj, run_all, ...) inherits the compiler environment
    os.environ.update(toolchain_env())


def lock(f, wait=True):
    """Exclusive lock on an open file or file descriptor (flock on Unix, msvcrt.locking on Windows). Raises
    BlockingIOError if wait is False and another process holds it; released when it closes."""
    if WIN:
        import msvcrt
        fd = f if isinstance(f, int) else f.fileno()
        while True:
            try:
                os.lseek(fd, 0, 0)
                msvcrt.locking(fd, msvcrt.LK_NBLCK, 1)
                return
            except OSError:
                if not wait:
                    raise BlockingIOError
                time.sleep(0.5)
    import fcntl
    fcntl.flock(f, fcntl.LOCK_EX | (0 if wait else fcntl.LOCK_NB))
