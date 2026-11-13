@echo off
rem build the pinned angr venv under D:\mb (no system-wide changes: uv's Python in D:\mb\uvpy, no registry, no bin shims)
set UV_PYTHON_INSTALL_DIR=D:\mb\uvpy
set UV_CACHE_DIR=D:\mb\uvcache
set PIP_CACHE_DIR=D:\mb\uvcache\pip
set PIP_DISABLE_PIP_VERSION_CHECK=1
D:\mb\uvtool\Scripts\uv python install 3.12.15 --no-bin --no-registry 2>&1
for /f "delims=" %%p in ('D:\mb\uvtool\Scripts\uv python find 3.12.15') do set PY312=%%p
echo python: %PY312%
"%PY312%" D:\mb\eq\setup_angr_venv.py --dest D:\mb\angr-venv --python "%PY312%" 2>&1
