"""Checks README.md against the code.

* C++ blocks marked <!-- cpp --> are one continuous session: they are compiled and run together
* C++ blocks marked <!-- program --> are complete programs, compiled and run on their own
* Python blocks marked <!-- python --> run in order in one namespace
* <!-- file: NAME --> blocks are written to the working directory first
* <!-- output --> blocks: every line (except addresses) must be printed by the C++ session
* local links, images and table of contents anchors must exist

Usage:  PYTHONPATH=build python docs/check_readme.py [build_dir]
"""
import io
import contextlib
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
BUILD = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build"))
text = open(os.path.join(ROOT, "README.md")).read()
errors = []


def fail(msg):
    errors.append(msg)
    print("FAIL:", msg)


# ---- blocks -------------------------------------------------------------------------------------
blocks = []  # (marker, language, code)
for m in re.finditer(r"(?:<!-- (?P<marker>[^>]*?) -->\n)?```(?P<lang>\w*)\n(?P<code>.*?)```", text, re.S):
    blocks.append((m.group("marker"), m.group("lang"), m.group("code")))

work = tempfile.mkdtemp(prefix="readme_check_")
for marker, lang, code in blocks:
    if marker and marker.startswith("file:"):
        with open(os.path.join(work, marker.split(":")[1].strip()), "w") as f:
            f.write(code)

# ---- C++ ----------------------------------------------------------------------------------------
def compile_run(source, name):
    src = os.path.join(work, name + ".cpp")
    exe = os.path.join(work, name)
    open(src, "w").write(source)
    cc = subprocess.run(["clang++", "-std=c++17", "-O1", "-Wall", "-I", os.path.join(ROOT, "include"), src, "-L", BUILD, "-lsipm",
                         f"-Wl,-rpath,{BUILD}", "-o", exe], capture_output=True, text=True)
    if cc.returncode:
        fail(f"{name}: does not compile\n{cc.stderr[:2500]}")
        return ""
    run = subprocess.run([exe], capture_output=True, text=True, cwd=work)
    if run.returncode:
        fail(f"{name}: exit code {run.returncode}\n{run.stderr[:500]}")
    return run.stdout


includes, body = [], []
for marker, lang, code in blocks:
    if marker == "cpp":
        for line in code.split("\n"):
            if line.startswith("#include"):
                includes.append(line)
            elif line.strip() != "using namespace sipm;":
                body.append(line)
session_out = ""
if body:
    src = "\n".join(sorted(set(includes))) + "\nusing namespace sipm;\nint main() {\n" + "\n".join(body) + "\n}\n"
    session_out = compile_run(src, "session")
for i, (marker, lang, code) in enumerate(b for b in blocks if b[0] == "program"):
    out = compile_run(code, f"program{i}")
    if "integral" not in out:
        fail("program: unexpected output " + out[:200])

# ---- outputs ------------------------------------------------------------------------------------
printed = set(l.rstrip() for l in session_out.split("\n"))
for marker, lang, code in blocks:
    if marker == "output":
        for line in code.rstrip("\n").split("\n"):
            if line.startswith("Address:"):
                continue
            if line.rstrip() not in printed:
                fail(f"output line not printed by the code: {line!r}")

# ---- Python -------------------------------------------------------------------------------------
ns = {}
cwd = os.getcwd()
os.chdir(work)
for marker, lang, code in blocks:
    if marker == "python":
        buf = io.StringIO()
        try:
            with contextlib.redirect_stdout(buf):
                exec(compile(code, "<README python>", "exec"), ns)
        except Exception as e:  # noqa: BLE001
            fail(f"python block failed: {type(e).__name__}: {e}")
        print("python output:", buf.getvalue().strip().replace("\n", " | "))
os.chdir(cwd)

# ---- links, images, anchors ---------------------------------------------------------------------
def slug(h):
    s = re.sub(r"[^\w\- ]", "", h.strip().lower())
    return s.replace(" ", "-")


anchors = {slug(m.group(1)) for m in re.finditer(r"^#{1,6} (.+)$", text, re.M)}
for m in re.finditer(r"\]\(#([^)]+)\)", text):
    if m.group(1) not in anchors:
        fail(f"broken anchor #{m.group(1)}")
for m in re.finditer(r'(?:src="|\]\()((?!http|#|mailto)[^")\s]+)', text):
    if not os.path.exists(os.path.join(ROOT, m.group(1))):
        fail(f"missing file {m.group(1)}")

print(f"{len(blocks)} code blocks checked, {len(errors)} problems")
sys.exit(1 if errors else 0)
