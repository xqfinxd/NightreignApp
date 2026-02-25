#!/usr/bin/env python3

import os
import sys
import csvutil
import defines
from pathlib import Path

EventDefinitions = defines.EventDefinitionsCHS


def build_flag_filters(flagsortbase):
    """Build eventFlag -> list of filter conditions from LotBaseMapPatternFlag"""
    flag_filters = {}
    for row in flagsortbase:
        event_flag = int(row.get("eventFlag") or 0)
        if event_flag == 0:
            continue
        filters = flag_filters.setdefault(event_flag, [])
        filters.append({
            "require1":    int(row.get("requireModifier1") or 0),
            "require2":    int(row.get("requireModifier2") or 0),
            "exclude1":    int(row.get("excludeModifier1") or 0),
            "exclude2":    int(row.get("excludeModifier2") or 0),
            "modifierSet": int(row.get("modifierSet") or 0),
            "modifier":    int(row.get("modifier") or 0),
        })
    return flag_filters


def build_pattern_flags(flagdistbase):
    """Build patternId -> {id, modifiers, flags} from LotResultMapPatternFlag"""
    pattern_flags = {}
    for row in flagdistbase:
        pattern_id = int(row.get("patternId") or 0)
        pattern = pattern_flags.setdefault(pattern_id, {
            "id": pattern_id,
            "modifiers": {},  # modifier -> modifierSet
            "flags": [],      # unique list of eventFlags
        })

        modifier = int(row.get("modifier") or 0)
        if modifier != 0:
            pattern["modifiers"][modifier] = int(row.get("modifierSet") or 0)

        event_flag = int(row.get("eventFlag") or 0)
        if event_flag not in pattern["flags"]:
            pattern["flags"].append(event_flag)

    return pattern_flags


def resolve_pattern_events(pattern_flags, flag_filters):
    """Match event flags to patterns and resolve event names."""
    pattern_events = []
    invasion_flags = {7705, 7725}

    for pattern_id, data in pattern_flags.items():
        for event_flag in data["flags"]:
            filter_set = flag_filters.get(event_flag)
            if not filter_set:
                continue

            for f in filter_set:
                # Check require/exclude conditions
                match = True
                if f["require1"] != 0 and f["require1"] not in data["modifiers"]:
                    match = False
                if f["require2"] != 0 and f["require2"] not in data["modifiers"]:
                    match = False
                if f["exclude1"] != 0 and f["exclude1"] in data["modifiers"]:
                    match = False
                if f["exclude2"] != 0 and f["exclude2"] in data["modifiers"]:
                    match = False

                if not match:
                    continue

                if event_flag in invasion_flags:
                    # Invasion events also require modifierSet to match
                    if f["modifierSet"] != data["modifiers"].get(f["modifier"]):
                        continue

                # Resolve event name
                definition = EventDefinitions.get(event_flag)
                if definition is None:
                    break
                if callable(definition):
                    event_name = definition(
                        f["modifier"],
                        data["modifiers"].get(f["modifier"], f["modifierSet"])
                    )
                else:
                    event_name = definition

                if event_name:
                    pattern_events.append({
                        "id": pattern_id,
                        "event": event_name,
                    })
                break  # first matching filter wins

    return pattern_events


def main():
    paths = defines.PathDefinitions(__file__)

    flagsortbase = csvutil.load_csv(paths.get_metadata("LotBaseMapPatternFlag.csv"))
    flagdistbase = csvutil.load_csv(paths.get_metadata("LotResultMapPatternFlag.csv"))

    flag_filters   = build_flag_filters(flagsortbase)
    pattern_flags  = build_pattern_flags(flagdistbase)
    pattern_events = resolve_pattern_events(pattern_flags, flag_filters)

    header = {
        "id":        "int",
        "event":     "std::string",
    }
    csvutil.generate_csv(header, pattern_events, paths.get_output("manual_events.csv"))
    csvutil.generate_cpp_header(header, paths.get_cpp_header("EventRow"), "EventRow")
    print(f"Generated {len(pattern_events)} pattern events.")


if __name__ == "__main__":
    sys.exit(main())

