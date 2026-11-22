"""Builds KiCad 9 schematics from Python: symbols copied verbatim from the installed KiCad libraries
(plus local project symbols), every pin connected by a net label placed exactly on the pin end.
No wires are drawn; ERC treats labels with the same name as one net.
"""
import copy
import os
import uuid as _uuid

from sexp import Q, dump, find, first, parse

SYMDIR = "/usr/share/kicad/symbols"


def uid(seed):
    return str(_uuid.uuid5(_uuid.NAMESPACE_URL, "seri-tool/" + seed))


class Library:
    def __init__(self, extra_dirs=()):
        self.dirs = [SYMDIR] + list(extra_dirs)
        self.cache = {}

    def _lib(self, lib):
        if lib not in self.cache:
            for d in self.dirs:
                p = os.path.join(d, lib + ".kicad_sym")
                if os.path.exists(p):
                    self.cache[lib] = parse(open(p, encoding="utf-8").read())
                    break
            else:
                raise FileNotFoundError(lib)
        return self.cache[lib]

    def symbol(self, lib, name):
        """Flattened symbol (derived 'extends' resolved), renamed lib:name for lib_symbols."""
        tree = self._lib(lib)
        sym = next(s for s in find(tree, "symbol") if s[1] == name)
        sym = copy.deepcopy(sym)
        ext = first(sym, "extends")
        if ext:
            parent = self.symbol(lib, ext[1])
            sym = [x for x in sym if not (isinstance(x, list) and x[0] == "extends")]
            props = {p[1]: p for p in find(sym, "property")}
            merged = [parent[0], Q(f"{lib}:{name}")]
            for x in parent[2:]:
                if isinstance(x, list) and x[0] == "property" and x[1] in props:
                    merged.append(props.pop(x[1]))
                elif isinstance(x, list) and x[0] == "symbol":
                    sub = copy.deepcopy(x)
                    sub[1] = Q(sub[1].replace(ext[1].split(":")[-1], name, 1))
                    merged.append(sub)
                else:
                    merged.append(x)
            merged += list(props.values())
            return merged
        sym[1] = Q(f"{lib}:{name}")
        return sym

    @staticmethod
    def pins(sym):
        out = {}
        for sub in find(sym, "symbol"):
            for p in find(sub, "pin"):
                at = first(p, "at")
                num = first(p, "number")[1]
                name = first(p, "name")[1]
                out[str(num)] = (float(at[1]), float(at[2]), p[1], str(name))
        return out


class Schematic:
    def __init__(self, project, title, library):
        self.project, self.lib = project, library
        self.sheet = uid(project + "/sheet")
        self.libsyms, self.items = {}, []
        self.title = title
        self.nets = {}  # net -> [(ref, pin)]
        self.parts = {}  # ref -> dict

    def add(self, lib, name, ref, value, at, nets, footprint="", fields=None):
        key = f"{lib}:{name}"
        if key not in self.libsyms:
            self.libsyms[key] = self.lib.symbol(lib, name)
        sym = self.libsyms[key]
        pins = Library.pins(sym)
        x, y = at
        missing = set(pins) - set(nets)
        if missing and not ref.startswith("#"):
            raise ValueError(f"{ref}: pins without a net: {sorted(missing)} (use 'NC' for no-connect)")
        node = ["symbol", ["lib_id", Q(key)], ["at", str(x), str(y), "0"], ["unit", "1"],
                ["exclude_from_sim", "no"], ["in_bom", "no" if ref.startswith("#") else "yes"], ["on_board", "yes"],
                ["dnp", "no"], ["uuid", uid(self.project + "/" + ref)]]
        props = [("Reference", ref, (x + 3, y - 3)), ("Value", value, (x + 3, y + 3)),
                 ("Footprint", footprint, (x, y)), ("Datasheet", "~", (x, y))]
        for k, v in (fields or {}).items():
            props.append((k, v, (x, y)))
        for k, v, (px, py) in props:
            eff = ["effects", ["font", ["size", "1.27", "1.27"]]]
            if k not in ("Reference", "Value"):
                eff.append(["hide", "yes"])
            node.append(["property", Q(k), Q(v), ["at", f"{px:g}", f"{py:g}", "0"], eff])
        for num in pins:
            node.append(["pin", Q(num), ["uuid", uid(f"{self.project}/{ref}/pin{num}")]])
        node.append(["instances", ["project", Q(self.project),
                                   ["path", Q("/" + self.sheet), ["reference", Q(ref)], ["unit", "1"]]]])
        self.items.append(node)
        self.parts[ref] = {"lib": key, "value": value, "footprint": footprint, "pins": pins}
        for num, net in nets.items():
            if num not in pins:
                raise ValueError(f"{ref}: no pin {num}")
            px, py = x + pins[num][0], y - pins[num][1]
            if net == "NC":
                self.items.append(["no_connect", ["at", f"{px:g}", f"{py:g}"], ["uuid", uid(f"{self.project}/{ref}/nc{num}")]])
                continue
            self.items.append(["label", Q(net), ["at", f"{px:g}", f"{py:g}", "0"],
                               ["effects", ["font", ["size", "1.0", "1.0"]], ["justify", "left", "bottom"]],
                               ["uuid", uid(f"{self.project}/{ref}/lab{num}")]])
            self.nets.setdefault(net, []).append((ref, num))

    def text(self, s, at):
        self.items.append(["text", Q(s), ["exclude_from_sim", "no"], ["at", str(at[0]), str(at[1]), "0"],
                           ["effects", ["font", ["size", "2", "2"]], ["justify", "left", "bottom"]],
                           ["uuid", uid(self.project + "/text/" + s[:40])]])

    def save(self, path):
        root = ["kicad_sch", ["version", "20250114"], ["generator", Q("seri-tool-schgen")], ["generator_version", Q("9.0")],
                ["uuid", self.sheet], ["paper", Q("A2")],
                ["title_block", ["title", Q(self.title)], ["company", Q("Seri-tool")], ["rev", Q("1")]],
                ["lib_symbols"] + list(self.libsyms.values())]
        root += self.items
        root.append(["sheet_instances", ["path", Q("/"), ["page", Q("1")]]])
        open(path, "w", encoding="utf-8").write(dump(root) + "\n")
        # project-local symbol library table, so ERC finds every library on any machine
        libs = sorted({k.split(":")[0] for k in self.libsyms})
        table = ["sym_lib_table", ["version", "7"]]
        for lib in libs:
            local = [d for d in self.lib.dirs[1:] if os.path.exists(os.path.join(d, lib + ".kicad_sym"))]
            uri = (os.path.relpath(os.path.join(local[0], lib + ".kicad_sym"), os.path.dirname(os.path.abspath(path)))
                   if local else "${KICAD9_SYMBOL_DIR}/" + lib + ".kicad_sym")
            if local:
                uri = "${KIPRJMOD}/" + uri
            table.append(["lib", ["name", Q(lib)], ["type", Q("KiCad")], ["uri", Q(uri)], ["options", Q("")], ["descr", Q("")]])
        open(os.path.join(os.path.dirname(os.path.abspath(path)), "sym-lib-table"), "w").write(dump(table) + "\n")
        pro = os.path.splitext(os.path.abspath(path))[0] + ".kicad_pro"
        if not os.path.exists(pro):  # a project file makes KiCad use the project-local tables
            import json
            json.dump({"meta": {"filename": os.path.basename(pro), "version": 1}}, open(pro, "w"), indent=2)
