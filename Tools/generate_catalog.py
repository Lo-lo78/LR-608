#!/usr/bin/env python3
"""Generate LR-608 VST3 parameter/page catalogues and migration audit data.

The JSFX is authoritative for parameter semantics.  The Speak FX Params
profile is authoritative for page membership, order, and grid height.
"""

from __future__ import annotations

import base64
import csv
import pathlib
import re
import shlex


ROOT = pathlib.Path(__file__).resolve().parents[1]
WORKSPACE = ROOT.parent
JSFX = WORKSPACE / "LR-608 JSFX" / "LR-608.jsfx"
PROFILE = WORKSPACE / "LR-608 JSFX" / "JS - LR - 608.txt"
RPL = WORKSPACE / "LR-608 JSFX" / "LR-608.jsfx.rpl"
SOURCE_DIR = ROOT / "Source"
DOC_DIR = ROOT / "Documentation"

SLIDER_RE = re.compile(
    r"^slider(?P<number>\d+):(?P<default>[-+0-9.eE]+)"
    r"<(?P<minimum>[-+0-9.eE]+),(?P<maximum>[-+0-9.eE]+),"
    r"(?P<step>[-+0-9.eE]+)(?:\{(?P<choices>[^}]*)\})?>"
    r"(?P<name>.+?)\s*$"
)

PAGE_NAMES = {
    1: "Kick",
    2: "Snare 1",
    3: "Snare 2",
    4: "Clap",
    5: "Rim",
    6: "Toms",
    7: "HiHat",
    8: "Cymbal",
    9: "Maracas",
    10: "Clave / Cowbell",
    11: "Zap",
    12: "Global",
}

ROUTING_PARAMETERS = [
    ("routeKick", "Kick Output"),
    ("routeSnare1", "Snare 1 Output"),
    ("routeSnare2", "Snare 2 Output"),
    ("routeClap", "Clap Output"),
    ("routeRim", "Rim Output"),
    ("routeLowTom", "Low Tom Output"),
    ("routeMidTom", "Mid Tom Output"),
    ("routeHighTom", "High Tom Output"),
    ("routeHiHat", "HiHat Output"),
    ("routeCrash", "Crash Output"),
    ("routeRide", "Ride Output"),
    ("routeMaracas", "Maracas Output"),
    ("routeCowbell", "Clave/Cowbell Output"),
    ("routeZap", "Zap Output"),
]

PAGE_ROUTING = {
    1: [("routeKick", "Kick Output")],
    2: [("routeSnare1", "Snare 1 Output")],
    3: [("routeSnare2", "Snare 2 Output")],
    4: [("routeClap", "Clap Output")],
    5: [("routeRim", "Rim Output")],
    6: [("routeLowTom", "Low Tom Output"), ("routeMidTom", "Mid Tom Output"),
        ("routeHighTom", "High Tom Output")],
    7: [("routeHiHat", "HiHat Output")],
    8: [("routeCrash", "Crash Output"), ("routeRide", "Ride Output")],
    9: [("routeMaracas", "Maracas Output")],
    10: [("routeCowbell", "Clave/Cowbell Output")],
    11: [("routeZap", "Zap Output")],
    12: [],
}


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def cpp_float(value: str) -> str:
    rendered = format(float(value), ".17g")
    if "." not in rendered and "e" not in rendered.lower():
        rendered += ".0"
    return rendered


def parse_parameters(source: str) -> list[dict[str, str]]:
    parameters: list[dict[str, str]] = []
    for line_number, line in enumerate(source.splitlines(), 1):
        if re.match(r"^slider\d+:", line) is None:
            continue
        match = SLIDER_RE.match(line)
        if match is None:
            raise RuntimeError(f"Unsupported slider declaration at line {line_number}: {line}")
        parameters.append(match.groupdict(default=""))

    numbers = [int(item["number"]) for item in parameters]
    if numbers != list(range(1, 257)):
        raise RuntimeError(f"Expected exactly slider1..slider256, got {numbers[:3]}..{numbers[-3:]}")
    return parameters


def parse_profile(text: str) -> tuple[dict[int, list[tuple[int, str]]], dict[int, int]]:
    section = ""
    groups: dict[int, list[tuple[int, str]]] = {}
    heights: dict[int, int] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line
            continue
        if section == "[GroupSlots]" and "=" in line:
            slot_text, encoded = line.split("=", 1)
            slot = int(slot_text)
            entries: list[tuple[int, str]] = []
            for item in encoded.split(r"\n"):
                index_text, name = item.split("\t", 1)
                index = int(index_text)
                if index >= 0:  # -300000 Compare is a host pseudo-parameter.
                    entries.append((index + 1, name))  # profile indices are zero based.
            groups[slot] = entries
        match = re.match(r"group_grid_size_v1::slot::(\d+)=(\d+)$", line)
        if match:
            heights[int(match.group(1))] = int(match.group(2))

    if sorted(groups) != list(range(1, 13)):
        raise RuntimeError(f"Expected 12 GroupSlots, got {sorted(groups)}")
    return groups, heights


def parse_presets(text: str) -> list[tuple[str, list[str]]]:
    presets: list[tuple[str, list[str]]] = []
    pattern = re.compile(r"<PRESET `([^`]*)`\s+(.*?)\s+>", re.DOTALL)
    for name, payload in pattern.findall(text):
        compact = "".join(payload.split())
        decoded = base64.b64decode(compact).decode("utf-8")
        tokens = shlex.split(decoded)
        presets.append((name, tokens))
    if not presets:
        raise RuntimeError("No presets decoded from RPL")
    return presets


def generate_parameters(parameters: list[dict[str, str]]) -> None:
    rows = []
    for parameter in parameters:
        number = int(parameter["number"])
        rows.append(
            '    { %d, "slider%03d", "%s", %s, %s, %s, %s, "%s" },'
            % (
                number,
                number,
                cpp_string(parameter["name"]),
                cpp_float(parameter["minimum"]),
                cpp_float(parameter["maximum"]),
                cpp_float(parameter["step"]),
                cpp_float(parameter["default"]),
                cpp_string(parameter["choices"].replace(",", "|")),
            )
        )
    output_choices = "|".join(
        ["Stereo 1/2"] + [f"Output {channel}/{channel + 1}" for channel in range(3, 30, 2)]
    )
    for parameter_id, name in ROUTING_PARAMETERS:
        rows.append(
            f'    {{ 0, "{parameter_id}", "{cpp_string(name)}", '
            f'0.0, 14.0, 1.0, 0.0, "{output_choices}" }},'
        )
    content = """// Generated by Tools/generate_catalog.py. Do not edit.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

namespace lr608::generated
{
struct ParameterDescriptor
{
    int sliderNumber;
    const char* id;
    const char* name;
    double minimum;
    double maximum;
    double step;
    double defaultValue;
    const char* choices;
};

inline constexpr ParameterDescriptor parameters[] = {
""" + "\n".join(rows) + "\n};\n}\n"
    (SOURCE_DIR / "GeneratedParameters.h").write_text(content, encoding="utf-8", newline="\n")


def generate_pages(
    groups: dict[int, list[tuple[int, str]]], heights: dict[int, int]
) -> None:
    arrays: list[str] = []
    rows: list[str] = []
    for slot in range(1, 13):
        entries = groups[slot]
        route_entries = PAGE_ROUTING[slot]
        ids = ", ".join(
            [f'"{parameter_id}"' for parameter_id, _ in route_entries]
            + [f'"slider{number:03d}"' for number, _ in entries]
        )
        names = ", ".join(
            [f'"{cpp_string(name)}"' for _, name in route_entries]
            + [f'"{cpp_string(name)}"' for _, name in entries]
        )
        arrays.append(f"inline constexpr const char* page{slot}Ids[] = {{ {ids} }};")
        arrays.append(f"inline constexpr const char* page{slot}Names[] = {{ {names} }};")
        rows.append(
            f'    {{ "{cpp_string(PAGE_NAMES[slot])}", page{slot}Ids, page{slot}Names, '
            f'std::size (page{slot}Ids), {heights[slot]} }},'
        )
    content = """// Generated by Tools/generate_catalog.py from GroupSlots. Do not edit.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <cstddef>
#include <iterator>

namespace lr608::generated
{
struct PageDescriptor
{
    const char* name;
    const char* const* parameterIds;
    const char* const* parameterNames;
    std::size_t parameterCount;
    int rowsPerColumn;
};

""" + "\n".join(arrays) + "\n\ninline constexpr PageDescriptor pages[] = {\n" + "\n".join(rows) + "\n};\n}\n"
    (SOURCE_DIR / "GeneratedPages.h").write_text(content, encoding="utf-8", newline="\n")


def write_audit(
    parameters: list[dict[str, str]],
    groups: dict[int, list[tuple[int, str]]],
    heights: dict[int, int],
    source: str,
    presets: list[tuple[str, list[str]]],
) -> None:
    page_for_slider = {
        number: PAGE_NAMES[slot]
        for slot, entries in groups.items()
        for number, _ in entries
    }
    profile_name_for_slider = {
        number: name for entries in groups.values() for number, name in entries
    }
    with (DOC_DIR / "parameter-map.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "jsfx_slider", "vst3_id", "jsfx_name", "ui_name", "default",
            "minimum", "maximum", "step", "choices", "page", "source_occurrences",
        ])
        for item in parameters:
            number = int(item["number"])
            occurrences = len(re.findall(rf"(?<![A-Za-z0-9_])slider{number}(?![0-9])", source))
            writer.writerow([
                number, f"slider{number:03d}", item["name"],
                profile_name_for_slider.get(number, item["name"]), item["default"],
                item["minimum"], item["maximum"], item["step"], item["choices"],
                page_for_slider.get(number, "Unexposed"), occurrences,
            ])

    with (DOC_DIR / "page-map.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["page", "rows_per_column", "position", "jsfx_slider", "vst3_id", "ui_name"])
        for slot in range(1, 13):
            for position, (number, name) in enumerate(groups[slot], 1):
                writer.writerow([PAGE_NAMES[slot], heights[slot], position, number,
                                 f"slider{number:03d}", name])

    with (DOC_DIR / "preset-audit.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["number", "name", "token_count", "has_256_slider_values"])
        for index, (name, tokens) in enumerate(presets, 1):
            writer.writerow([index, name, len(tokens), len(tokens) >= 257])


def main() -> None:
    SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    DOC_DIR.mkdir(parents=True, exist_ok=True)
    source = JSFX.read_text(encoding="utf-8-sig")
    parameters = parse_parameters(source)
    groups, heights = parse_profile(PROFILE.read_text(encoding="utf-8-sig"))
    presets = parse_presets(RPL.read_text(encoding="utf-8-sig"))
    generate_parameters(parameters)
    generate_pages(groups, heights)
    write_audit(parameters, groups, heights, source, presets)
    exposed = {number for entries in groups.values() for number, _ in entries}
    print(f"Generated {len(parameters)} JSFX + {len(ROUTING_PARAMETERS)} routing parameters, "
          f"{len(groups)} pages, {len(presets)} presets")
    print(f"Profile exposes {len(exposed)} unique sliders; {256 - len(exposed)} remain host-automatable")


if __name__ == "__main__":
    main()
