"""Predeclared geometry, directory locations and cell observations from retail overlays.

The tuple values are fixed before executing these experiments; recovered C
constants are never imported as the expected numeric encodings.
"""

import struct

# Scene, sampler, axis count, vertical/horizontal cell units, sample Y/X/Z,
# sample value, directory address, selected layer address, absent Y, value at Y0.
RETAIL_GRIDS = [
    ('abra_shop', 'abraShopGetSpatialCell', 8, 20.0, 20.0, 0, 4, 3, 3, 2148824124, 2148823868, 1, 3),
    ('bedroom', 'bedroomGetSpatialCell', 16, 20.0, 10.0, 0, 1, 3, 46, 2148918360, 2148915288, 3, 46),
    ('cobalt_coast', 'cobaltCoastGetSpatialCell', 64, 5.0, 10.0, 0, 30, 16, 40, 2148857120, 2148840736, 1, 40),
    ('cobalt_fishing', 'cobaltFishingGetSpatialCell', 8, 20.0, 20.0, 0, 1, 1, 5, 2148865800, 2148865544, 1, 5),
    ('cobalt_island', 'cobaltIslandGetSpatialCell', 16, 15.0, 20.0, 0, 4, 1, 65, 2149098124, 2149095052, 3, 65),
    ('credits', 'creditsGetSpatialCell', 4, 20.0, 20.0, 0, 0, 1, 3, 2148811288, 2148811224, 1, 3),
    ('entryway', 'entrywayGetSpatialCell', 16, 21.0, 10.0, 0, 7, 3, 1, 2148780168, 2148778120, 2, 1),
    ('front_yard', 'frontYardGetSpatialCell', 32, 10.0, 10.0, 0, 8, 20, 93, 2148954280, 2148941992, 1, 93),
    ('ochre_deep', 'ochreDeepGetSpatialCell', 32, 10.0, 10.0, 0, 0, 2, 45, 2149105736, 2149085256, 5, 45),
    ('ochre_entrance', 'ochreEntranceGetSpatialCell', 16, 20.0, 20.0, 0, 4, 2, 7, 2149009012, 2149007988, 1, 7),
    ('ochre_fishing', 'ochreFishingGetSpatialCell', 8, 20.0, 20.0, 0, 3, 2, 6, 2148880816, 2148880560, 1, 6),
    ('oddish_meadow', 'oddishMeadowGetSpatialCell', 16, 10.0, 20.0, 0, 3, 1, 164, 2149094448, 2149093424, 1, 164),
    ('olivine_fishing', 'olivineFishingGetSpatialCell', 8, 20.0, 20.0, 0, 1, 1, 1, 2148848152, 2148847896, 1, 1),
    ('olivine_lake', 'olivineLakeGetSpatialCell', 32, 5.0, 10.0, 0, 0, 2, 94, 2148863700, 2148855508, 1, 94),
    ('olivine_lakeshore', 'lakeshoreGetSpatialCell', 32, 5.0, 10.0, 0, 0, 2, 112, 2148880628, 2148872436, 1, 112),
    ('playground', 'playgroundGetSpatialCell', 8, 20.0, 40.0, 0, 2, 2, 17, 2148900940, 2148900684, 1, 17),
    ('springleaf_entrance', 'springleafEntranceGetSpatialCell', 32, 5.0, 10.0, 0, 12, 19, 136, 2148976492, 2148947820, 5, 136),
    ('springleaf_field', 'springleafFieldGetSpatialCell', 32, 5.0, 10.0, 0, 2, 2, 66, 2149045848, 2149021272, 3, 66),
    ('viridian_deep', 'viridianDeepGetSpatialCell', 32, 5.0, 10.0, 0, 12, 4, 18, 2148916168, 2148912072, 1, 18),
    ('viridian_edge', 'viridianEdgeGetSpatialCell', 8, 20.0, 20.0, 0, 2, 2, 5, 2148908848, 2148908592, 1, 5),
    ('viridian_entrance', 'viridianEntranceGetSpatialCell', 32, 5.0, 10.0, 0, 12, 4, 25, 2149009020, 2149000828, 1, 25),
    ('viridian_forest', 'viridianForestGetSpatialCell', 32, 5.0, 10.0, 0, 10, 8, 34, 2148850464, 2148834080, 4, 34),
    ('wilderness_camp', 'wildernessCampGetSpatialCell', 16, 20.0, 20.0, 0, 5, 5, 1, 2148781624, 2148780600, 1, 1),
]


def experiments():
    """Check nonempty cells, absent layers, outer bounds and negative truncation."""
    from .cases import word

    cases = []
    for scene, symbol, n, height, width, layer, x, z, value, directory, pointer, empty, near in RETAIL_GRIDS:
        points = [
            ("interior", (x + 0.5) * width, (layer + 0.5) * height, (z + 0.5) * width, value),
            ("near-negative-y", (x + 0.5) * width, -0.5 * height, (z + 0.5) * width, near),
            ("below-x", -width, 0, 0, 0),
            ("below-y", 0, -height, 0, 0),
            ("below-z", 0, 0, -width, 0),
            ("upper-x", n * width, 0, 0, 0),
            ("upper-y", 0, n * height, 0, 0),
            ("upper-z", 0, 0, n * width, 0),
            ("absent-layer", (x + 0.5) * width, (empty + 0.5) * height, (z + 0.5) * width, 0),
        ]
        observations = [
            (directory + layer * 4, word(pointer)),
            (pointer + ((z * n) + x) * 4, word(value)),
            (directory + empty * 4, word(0)),
        ]
        for label, px, py, pz, expected in points:
            bits = [struct.unpack(">I", struct.pack(">f", coordinate))[0] for coordinate in (px, py, pz)]
            cases.append(dict(
                id=f"numeric.scene-grid.{scene}.{label}", symbol=symbol,
                overlay=".scene_" + scene,
                args=[0, 0, bits[2]], fpr_pairs={12: bits[0], 14: bits[1]},
                patches=[], observations=observations, expected_return=expected,
                scope="selected retail scene sampler with synthetic world coordinates",
                limitations=[
                    "Raw cell values do not establish collision/event meanings, gameplay reachability or resource lifetime.",
                    "Finite coordinates exercise explicit conversion boundaries; general floating rounding fidelity is unverified.",
                ],
            ))
    return cases
