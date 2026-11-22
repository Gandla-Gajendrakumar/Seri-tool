"""Minimal S-expression reader/writer for KiCad files (tokens kept as strings; quoted strings marked)."""
import re

TOKEN = re.compile(r'\s*(?:(\()|(\))|"((?:[^"\\]|\\.)*)"|([^\s()"]+))', re.S)


class Q(str):
    """A quoted string token."""


def parse(text):
    stack, cur = [], []
    pos = 0
    while True:
        m = TOKEN.match(text, pos)
        if not m or m.end() == pos:
            break
        pos = m.end()
        if m.group(1):
            stack.append(cur)
            cur = []
        elif m.group(2):
            done = cur
            cur = stack.pop()
            cur.append(done)
        elif m.group(3) is not None:
            cur.append(Q(m.group(3).replace('\\"', '"').replace("\\\\", "\\")))
        else:
            cur.append(m.group(4))
    return cur[0] if len(cur) == 1 else cur


def dump(node, indent=0):
    if isinstance(node, Q):
        return '"' + node.replace("\\", "\\\\").replace('"', '\\"') + '"'
    if isinstance(node, str):
        return node
    simple = all(not isinstance(x, list) for x in node)
    if simple:
        return "(" + " ".join(dump(x) for x in node) + ")"
    pad = "  " * (indent + 1)
    parts = [dump(node[0])] + [("\n" + pad + dump(x, indent + 1)) if isinstance(x, list) else dump(x) for x in node[1:]]
    out = "(" + parts[0]
    for p in parts[1:]:
        out += p if p.startswith("\n") else " " + p
    return out + ")"


def find(node, head):
    return [x for x in node if isinstance(x, list) and x and x[0] == head]


def first(node, head):
    r = find(node, head)
    return r[0] if r else None
