"""Shared strict parsing of the retained scene configuration catalog."""

import re


def configuration_locations(source, constants=None):
    """Read a masked explicit 23-entry scene catalog, rejecting unfamiliar expressions."""
    code = source
    table = re.search(r'\bgAreaConfigurations\[(23|SCENE_AREA_COUNT)\]\s*=\s*\{([^}]+)\}', code)
    if not table:
        raise ValueError('expected the retained 23-entry scene configuration catalog')
    if table[1] == 'SCENE_AREA_COUNT':
        definitions = re.findall(r'\bSCENE_AREA_COUNT\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)\b', constants or '')
        if len(definitions) != 1 or int(definitions[0], 0) != 23:
            raise ValueError('scene extent must independently resolve to the retail count 23')
    entries = [entry.strip() for entry in table[2].split(',') if entry.strip()]
    locations = []
    for entry in entries:
        match = re.fullmatch(r'\(SceneConfigurationPrefix\s*\*\)\s*\((scene_\w+_DATA_START)\s*\+\s*(0x[\dA-Fa-f]+)\)', entry)
        if not match:
            raise ValueError('unrecognized scene configuration expression')
        locations.append((match[1], int(match[2], 16)))
    if len(locations) != 23 or len({base for base, _ in locations}) != 23:
        raise ValueError('expected 23 distinct scene overlay configurations')
    return locations

