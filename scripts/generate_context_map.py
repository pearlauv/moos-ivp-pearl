#!/usr/bin/env python3
"""Generate a compact repo traversal map for moos-ivp-pearl."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MD_PATH = ROOT / "docs/context/repo_map.md"
JSON_PATH = ROOT / "docs/context/repo_map.json"
SCHEMA_VERSION = 1


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def git_files() -> list[str]:
    try:
        output = subprocess.check_output(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
            cwd=ROOT,
            text=True,
            stderr=subprocess.DEVNULL,
        )
    except (OSError, subprocess.CalledProcessError):
        return sorted(
            rel(path)
            for path in ROOT.rglob("*")
            if path.is_file() and ".git" not in path.parts
        )
    return sorted(line for line in output.splitlines() if line)


def strip_cmake_comments(text: str) -> str:
    lines = []
    for line in text.splitlines():
        lines.append(re.sub(r"\s*#.*$", "", line))
    return "\n".join(lines)


def strip_line_comment(line: str) -> str:
    return re.sub(r"//.*$", "", line).strip()


def parse_build_profiles(src_cmake: Path) -> dict[str, list[str]]:
    profiles: dict[str, set[str]] = defaultdict(set)
    profile_stack: list[str | None] = []

    for raw_line in read(src_cmake).splitlines():
        line = re.sub(r"\s*#.*$", "", raw_line).strip()
        if not line:
            continue

        if_match = re.match(r"IF\s*\(\s*PEARL-OPTIONS\s+STREQUAL\s+([^) ]+)", line, re.I)
        elseif_match = re.match(
            r"ELSEIF\s*\(\s*PEARL-OPTIONS\s+STREQUAL\s+([^) ]+)", line, re.I
        )
        endif_match = re.match(r"ENDIF\s*\(", line, re.I)

        if if_match:
            profile_stack.append(if_match.group(1))
            continue
        if re.match(r"IF\s*\(", line, re.I):
            profile_stack.append(None)
            continue
        if elseif_match:
            if profile_stack and profile_stack[-1] is not None:
                profile_stack[-1] = elseif_match.group(1)
            else:
                profile_stack.append(elseif_match.group(1))
            continue
        if endif_match:
            if profile_stack:
                profile_stack.pop()
            continue

        add_match = re.search(r"ADD_SUBDIRECTORY\s*\(\s*([^) \t]+)", line, re.I)
        if add_match:
            component = add_match.group(1).strip()
            profile = next((item for item in reversed(profile_stack) if item), "default")
            profiles[component].add(profile or "default")

    return {key: sorted(value) for key, value in sorted(profiles.items())}


def parse_component(component_dir: Path, build_profiles: dict[str, list[str]]) -> dict[str, Any]:
    cmake_path = component_dir / "CMakeLists.txt"
    text = strip_cmake_comments(read(cmake_path))
    component = component_dir.name

    target = component
    kind = "unknown"
    target_match = re.search(r"ADD_(EXECUTABLE|LIBRARY)\s*\(\s*([^\s)]+)", text, re.I)
    if target_match:
        kind = "executable" if target_match.group(1).upper() == "EXECUTABLE" else "library"
        target = target_match.group(2)

    link_libraries: list[str] = []
    link_match = re.search(
        r"TARGET_LINK_LIBRARIES\s*\(\s*" + re.escape(target) + r"\s+(.*?)\)",
        text,
        re.I | re.S,
    )
    if link_match:
        for token in re.split(r"\s+", link_match.group(1).strip()):
            if token and not token.startswith("${"):
                link_libraries.append(token)

    files = sorted(rel(path) for path in component_dir.iterdir() if path.is_file())
    source_files = [path for path in files if path.endswith((".cpp", ".c"))]
    header_files = [path for path in files if path.endswith((".h", ".hpp"))]
    main_files = [
        path
        for path in source_files
        if Path(path).name == "main.cpp" or Path(path).name.lower().endswith("main.cpp")
    ]
    info_files = [path for path in source_files if Path(path).name.endswith("_Info.cpp")]
    core_source_files = [
        path for path in source_files if path not in set(main_files) and path not in set(info_files)
    ]

    return {
        "name": component,
        "path": rel(component_dir),
        "cmake": rel(cmake_path),
        "kind": kind,
        "target": target,
        "build_profiles": build_profiles.get(component, []),
        "source_count": len(source_files),
        "header_count": len(header_files),
        "core_source_files": core_source_files,
        "main_files": main_files,
        "info_files": info_files,
        "link_libraries": sorted(link_libraries),
    }


def parse_moos_file(path: Path) -> dict[str, Any]:
    process_configs: set[str] = set()
    antler_runs: set[str] = set()
    includes: set[str] = set()
    behavior_files: set[str] = set()

    for raw_line in read(path).splitlines():
        stripped = raw_line.strip()
        include_match = re.match(r"#include\s+(.+)$", stripped)
        if include_match:
            includes.add(include_match.group(1).strip())
            continue

        line = strip_line_comment(raw_line)
        if not line:
            continue

        process_match = re.match(r"ProcessConfig\s*=\s*([^\s{]+)", line, re.I)
        if process_match:
            process_configs.add(process_match.group(1))

        run_match = re.match(r"Run\s*=\s*([^\s@]+)", line, re.I)
        if run_match:
            antler_runs.add(run_match.group(1))

        behavior_match = re.match(r"behaviors\s*=\s*(.+)$", line, re.I)
        if behavior_match:
            behavior_files.add(behavior_match.group(1).strip())

    return {
        "path": rel(path),
        "process_configs": sorted(process_configs),
        "antler_runs": sorted(antler_runs),
        "includes": sorted(includes),
        "behavior_files": sorted(behavior_files),
    }


def parse_bhv_file(path: Path) -> dict[str, Any]:
    behaviors: list[dict[str, str]] = []
    current: dict[str, str] | None = None

    for raw_line in read(path).splitlines():
        line = strip_line_comment(raw_line)
        if not line:
            continue
        behavior_match = re.match(r"Behavior\s*=\s*([^\s{]+)", line, re.I)
        if behavior_match:
            current = {"type": behavior_match.group(1), "name": ""}
            behaviors.append(current)
            continue
        name_match = re.match(r"name\s*=\s*(.+)$", line, re.I)
        if name_match and current is not None and not current["name"]:
            current["name"] = name_match.group(1).strip()

    return {"path": rel(path), "behaviors": behaviors}


def mission_dirs(files: list[str]) -> list[Path]:
    dirs: set[Path] = set()
    for file_name in files:
        if not file_name.startswith("missions/"):
            continue
        path = ROOT / file_name
        if path.suffix in {".moos", ".bhv"}:
            dirs.add(path.parent)
    return sorted(dirs, key=rel)


def parse_mission(path: Path) -> dict[str, Any]:
    moos_paths = sorted(path.glob("*.moos"))
    bhv_paths = sorted(path.glob("*.bhv"))
    launch_scripts = sorted(path.glob("launch*.sh"))
    readmes = sorted(child for child in path.iterdir() if child.is_file() and child.name == "README")
    map_assets = sorted(
        child
        for child in path.iterdir()
        if child.is_file() and child.suffix.lower() in {".info", ".tif", ".tiff"}
    )

    moos = [parse_moos_file(child) for child in moos_paths]
    bhv = [parse_bhv_file(child) for child in bhv_paths]

    process_configs = sorted({item for child in moos for item in child["process_configs"]})
    antler_runs = sorted({item for child in moos for item in child["antler_runs"]})
    includes = sorted({item for child in moos for item in child["includes"]})
    behavior_files = sorted({item for child in moos for item in child["behavior_files"]})
    behavior_types = sorted({item["type"] for child in bhv for item in child["behaviors"]})

    return {
        "name": rel(path),
        "path": rel(path),
        "readmes": [rel(child) for child in readmes],
        "launch_scripts": [rel(child) for child in launch_scripts],
        "moos_files": [child["path"] for child in moos],
        "bhv_files": [child["path"] for child in bhv],
        "map_assets": [rel(child) for child in map_assets],
        "process_configs": process_configs,
        "antler_runs": antler_runs,
        "includes": includes,
        "behavior_files": behavior_files,
        "behavior_types": behavior_types,
    }


def parse_script_dirs(files: list[str]) -> dict[str, dict[str, Any]]:
    script_files = [file_name for file_name in files if file_name.startswith("scripts/")]
    dirs: dict[str, list[str]] = defaultdict(list)
    for file_name in script_files:
        parent = Path(file_name).parent.as_posix()
        dirs[parent].append(file_name)

    result: dict[str, dict[str, Any]] = {}
    for path, dir_files in sorted(dirs.items()):
        direct_files = sorted(file_name for file_name in dir_files if Path(file_name).parent.as_posix() == path)
        result[path] = {
            "path": path,
            "files": direct_files,
            "readmes": [
                file_name
                for file_name in direct_files
                if Path(file_name).name == "README" or Path(file_name).name.startswith("README_")
            ],
            "arduino_sketches": [
                file_name for file_name in direct_files if file_name.endswith(".ino")
            ],
            "python_scripts": [
                file_name for file_name in direct_files if file_name.endswith(".py")
            ],
            "headers": [
                file_name for file_name in direct_files if file_name.endswith((".h", ".hpp"))
            ],
        }
    return result


def build_map() -> dict[str, Any]:
    files = git_files()
    build_profiles = parse_build_profiles(ROOT / "src/CMakeLists.txt")
    component_dirs = sorted(
        path.parent
        for path in (ROOT / "src").glob("*/CMakeLists.txt")
        if path.parent.name != "attic"
    )
    components = [parse_component(path, build_profiles) for path in component_dirs]
    missions = [parse_mission(path) for path in mission_dirs(files)]

    component_by_target = {component["target"]: component for component in components}
    local_targets = set(component_by_target)
    local_dirs = {component["name"] for component in components}
    target_consumers: dict[str, set[str]] = defaultdict(set)
    for component in components:
        for library in component["link_libraries"]:
            if library in local_targets:
                target_consumers[library].add(component["name"])

    app_usage: dict[str, set[str]] = defaultdict(set)
    external_app_usage: dict[str, set[str]] = defaultdict(set)
    behavior_usage: dict[str, set[str]] = defaultdict(set)
    missions_with_map_assets: dict[str, list[str]] = {}
    for mission in missions:
        configured_apps = set(mission["process_configs"]) | set(mission["antler_runs"])
        for app in configured_apps:
            if app in local_targets or app in local_dirs:
                app_usage[app].add(mission["path"])
            elif app not in {"ANTLER", "MOOSDB"}:
                external_app_usage[app].add(mission["path"])
        for behavior in mission["behavior_types"]:
            behavior_usage[behavior].add(mission["path"])
        if mission["map_assets"]:
            missions_with_map_assets[mission["path"]] = mission["map_assets"]

    source_exts = Counter(Path(file_name).suffix or "[none]" for file_name in files)
    generated_outputs = {
        "bin": "Runtime output directory for locally built executables.",
        "lib": "Library output directory for locally built archives/shared libraries.",
        "build": "CMake build tree; inspect source CMake files before editing generated build files.",
        "docs/context/repo_map.md": "Generated by scripts/generate_context_map.py.",
        "docs/context/repo_map.json": "Generated by scripts/generate_context_map.py.",
    }

    return {
        "schema_version": SCHEMA_VERSION,
        "generated_by": "scripts/generate_context_map.py",
        "summary": {
            "indexed_files": len(files),
            "src_components": len(components),
            "missions": len(missions),
            "mission_moos_files": sum(len(mission["moos_files"]) for mission in missions),
            "mission_bhv_files": sum(len(mission["bhv_files"]) for mission in missions),
        },
        "source_inputs": [
            "CMakeLists.txt",
            "src/CMakeLists.txt",
            "src/*/CMakeLists.txt",
            "missions/**/*.moos",
            "missions/**/*.bhv",
            "missions/**/launch*.sh",
            "missions/**/README",
            "scripts/**/*",
            "README",
            "build.sh",
        ],
        "validation": {
            "regenerate": "python3 scripts/generate_context_map.py",
            "freshness_check": "python3 scripts/generate_context_map.py --check",
            "build_profile_environment": "PEARLOPTIONS",
            "build_all": "./build.sh",
            "build_mac": "./build.sh --mac",
            "clean": "./clean.sh or ./build.sh --clean",
        },
        "generated_outputs": generated_outputs,
        "file_extension_counts": dict(sorted(source_exts.items())),
        "indices": {
            "components": {component["name"]: component for component in components},
            "components_by_target": {component["target"]: component["name"] for component in components},
            "missions": {mission["path"]: mission for mission in missions},
            "local_app_usage": {
                app: sorted(paths) for app, paths in sorted(app_usage.items())
            },
            "external_app_usage": {
                app: sorted(paths) for app, paths in sorted(external_app_usage.items())
            },
            "target_consumers": {
                target: sorted(consumers) for target, consumers in sorted(target_consumers.items())
            },
            "behavior_usage": {
                behavior: sorted(paths) for behavior, paths in sorted(behavior_usage.items())
            },
            "missions_with_map_assets": missions_with_map_assets,
            "script_dirs": parse_script_dirs(files),
            "build_profiles": build_profiles,
        },
    }


def markdown_table(headers: list[str], rows: list[list[str]]) -> list[str]:
    lines = [
        "| " + " | ".join(headers) + " |",
        "| " + " | ".join("---" for _ in headers) + " |",
    ]
    lines.extend("| " + " | ".join(row) + " |" for row in rows)
    return lines


def short_list(values: list[str], limit: int = 8) -> str:
    if not values:
        return "-"
    if len(values) <= limit:
        return ", ".join(f"`{value}`" for value in values)
    visible = ", ".join(f"`{value}`" for value in values[:limit])
    return f"{visible}, +{len(values) - limit} more"


def render_markdown(data: dict[str, Any]) -> str:
    summary = data["summary"]
    components = data["indices"]["components"]
    missions = data["indices"]["missions"]
    local_app_usage = data["indices"]["local_app_usage"]

    lines: list[str] = [
        "<!-- Generated by scripts/generate_context_map.py; do not edit by hand. -->",
        "",
        "# Repo Context Map",
        "",
        "This is a generated first-hop traversal map for `moos-ivp-pearl`. Use it to choose which source, mission, or validation file to open next; keep detailed behavior changes in the source files.",
        "",
        "## Summary",
        "",
        f"- Repository files indexed: `{summary['indexed_files']}`",
        f"- Source components: `{summary['src_components']}`",
        f"- Mission directories: `{summary['missions']}`",
        f"- Mission `.moos` files: `{summary['mission_moos_files']}`",
        f"- Mission `.bhv` files: `{summary['mission_bhv_files']}`",
        "",
        "## Source Inputs",
        "",
    ]

    lines.extend(f"- `{item}`" for item in data["source_inputs"])
    lines.extend(
        [
            "",
            "## Validation Routes",
            "",
            f"- Regenerate: `{data['validation']['regenerate']}`",
            f"- Freshness check: `{data['validation']['freshness_check']}`",
            f"- Build profile environment: `{data['validation']['build_profile_environment']}`",
            f"- Build all profile: `{data['validation']['build_all']}`",
            f"- Build Mac profile: `{data['validation']['build_mac']}`",
            f"- Clean build artifacts: `{data['validation']['clean']}`",
            "",
            "## Generated Outputs",
            "",
        ]
    )
    lines.extend(
        f"- `{path}`: {description}"
        for path, description in data["generated_outputs"].items()
    )

    lines.extend(["", "## Source Components", ""])
    component_rows = []
    for name, component in components.items():
        component_rows.append(
            [
                f"`{name}`",
                f"`{component['path']}`",
                component["kind"],
                f"`{component['target']}`",
                short_list(component["build_profiles"], 3),
                short_list(component["link_libraries"], 5),
            ]
        )
    lines.extend(
        markdown_table(
            ["Component", "Path", "Kind", "Target", "Build profiles", "Links"],
            component_rows,
        )
    )

    lines.extend(["", "## Shared Target Consumers", ""])
    consumer_rows = []
    for target, consumers in data["indices"]["target_consumers"].items():
        component_name = data["indices"]["components_by_target"].get(target, target)
        consumer_rows.append(
            [
                f"`{target}`",
                f"`{components[component_name]['path']}`" if component_name in components else "-",
                short_list(consumers, 8),
            ]
        )
    if consumer_rows:
        lines.extend(markdown_table(["Target", "Source", "Consumers"], consumer_rows))
    else:
        lines.append("No local target consumers found.")

    lines.extend(["", "## Local App Usage", ""])
    usage_rows = []
    for app, paths in local_app_usage.items():
        component_name = data["indices"]["components_by_target"].get(app, app)
        usage_rows.append(
            [
                f"`{app}`",
                f"`{components[component_name]['path']}`" if component_name in components else "-",
                str(len(paths)),
                short_list(paths, 6),
            ]
        )
    if usage_rows:
        lines.extend(markdown_table(["App", "Source", "Mission count", "Missions"], usage_rows))
    else:
        lines.append("No local app usage found in mission configs.")

    lines.extend(["", "## Mission Routes", ""])
    mission_rows = []
    for path, mission in missions.items():
        mission_rows.append(
            [
                f"`{path}`",
                str(len(mission["moos_files"])),
                str(len(mission["bhv_files"])),
                short_list(mission["launch_scripts"], 3),
                short_list(mission["process_configs"], 7),
                short_list(mission["behavior_types"], 5),
            ]
        )
    lines.extend(
        markdown_table(
            ["Mission", "MOOS", "BHV", "Launch", "ProcessConfig", "Behaviors"],
            mission_rows,
        )
    )

    lines.extend(["", "## Script Routes", ""])
    script_rows = []
    for path, script_dir in data["indices"]["script_dirs"].items():
        if path == "scripts":
            continue
        script_rows.append(
            [
                f"`{path}`",
                short_list(script_dir["arduino_sketches"], 3),
                short_list(script_dir["python_scripts"], 3),
                short_list(script_dir["readmes"], 2),
            ]
        )
    if script_rows:
        lines.extend(markdown_table(["Path", "Arduino", "Python", "Readme"], script_rows))
    else:
        lines.append("No script directories found.")

    lines.extend(
        [
            "",
            "## JSON Lookup Examples",
            "",
            "- Component source path: `jq -r '.indices.components.iPEARL.path' docs/context/repo_map.json`",
            "- Component build/link summary: `jq '.indices.components.iPEARL | {path,target,build_profiles,core_source_files,main_files,info_files,link_libraries}' docs/context/repo_map.json`",
            "- Missions using a local app: `jq -r '.indices.local_app_usage.pPearlPID[]' docs/context/repo_map.json`",
            "- Consumers of a local target: `jq -r '.indices.target_consumers.NMEAParse[]' docs/context/repo_map.json`",
            "- Missions using a behavior: `jq -r '.indices.behavior_usage.BHV_AvdColregsV19[]' docs/context/repo_map.json`",
            "- Mission route: `jq '.indices.missions[\"missions/auv_dock\"] | {launch_scripts,moos_files,bhv_files,process_configs,behavior_types}' docs/context/repo_map.json`",
            "- Script route: `jq '.indices.script_dirs[\"scripts/PEARL_frontseat_v2\"]' docs/context/repo_map.json`",
            "- Build profile membership: `jq '.indices.build_profiles' docs/context/repo_map.json`",
            "",
        ]
    )
    return "\n".join(lines)


def write_outputs(data: dict[str, Any]) -> None:
    MD_PATH.parent.mkdir(parents=True, exist_ok=True)
    json_text = json.dumps(data, indent=2, sort_keys=True) + "\n"
    markdown_text = render_markdown(data)
    if not markdown_text.endswith("\n"):
        markdown_text += "\n"
    JSON_PATH.write_text(json_text, encoding="utf-8")
    MD_PATH.write_text(markdown_text, encoding="utf-8")


def check_outputs(data: dict[str, Any]) -> int:
    expected_json = json.dumps(data, indent=2, sort_keys=True) + "\n"
    expected_md = render_markdown(data)
    if not expected_md.endswith("\n"):
        expected_md += "\n"

    stale = []
    if not JSON_PATH.exists() or JSON_PATH.read_text(encoding="utf-8") != expected_json:
        stale.append(rel(JSON_PATH))
    if not MD_PATH.exists() or MD_PATH.read_text(encoding="utf-8") != expected_md:
        stale.append(rel(MD_PATH))

    if stale:
        print("Context map is stale; regenerate with: python3 scripts/generate_context_map.py")
        for path in stale:
            print(f"stale: {path}")
        return 1

    print("Context map is up to date.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if generated outputs are stale")
    args = parser.parse_args()

    data = build_map()
    if args.check:
        return check_outputs(data)

    write_outputs(data)
    print(f"Wrote {rel(MD_PATH)}")
    print(f"Wrote {rel(JSON_PATH)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
