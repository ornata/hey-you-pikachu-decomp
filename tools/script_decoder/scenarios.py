"""Read explicit scenario selector returns without inferring table sizes from neighbors.

Literal returns establish indices to inspect, not gameplay reachability or branch
conditions. Unknown expressions stop the inventory rather than extending a table.
"""

import re


def function_body(code, name):
    """Extract a balanced function body from comment/string-masked game C."""
    match = re.search(r'\bs32\s+' + re.escape(name) + r'\([^{};]*\)\s*\{', code)
    if not match:
        match = re.search(r'\bvoid\s+' + re.escape(name) + r'\([^{};]*\)\s*\{', code)
    if not match:
        raise ValueError(f'missing function definition: {name}')
    start, depth = match.end(), 1
    for index in range(start, len(code)):
        depth += (code[index] == '{') - (code[index] == '}')
        if not depth:
            return code[start:index]
    raise ValueError(f'unclosed function: {name}')


def scenario_domains(code, area_ids=None):
    """Resolve the 23 explicit area cases to literal scenario return indices."""
    body = function_body(code, 'presentationInitializeForCurrentArea')
    switch = re.search(r'\bswitch\s*\(gRuntimeCurrentArea\)\s*\{', body)
    if not switch:
        raise ValueError('missing explicit area scenario selection')
    start, depth = switch.end(), 1
    end = start
    while end < len(body) and depth:
        depth += (body[end] == '{') - (body[end] == '}')
        end += 1
    if depth:
        raise ValueError('unclosed area scenario switch')
    selection = body[start:end - 1]
    labels = list(re.finditer(r'\bcase\s+(\w+)\s*:|\bdefault\s*:', selection))
    pending, result = [], {}
    for index, label in enumerate(labels):
        if label[1] is None:
            block = selection[label.end():]
            if pending or index != len(labels) - 1 or re.search(r'\bgAreaScenarioIndex\s*=', block):
                raise ValueError('area cases fall through into the default')
            continue
        if label[1].isdigit():
            area = int(label[1])
        elif label[1] in (area_ids or {}):
            area = area_ids[label[1]]
        else:
            raise ValueError('unresolved area case constant')
        pending.append(area)
        block = selection[label.end():labels[index + 1].start() if index + 1 < len(labels) else len(selection)].strip()
        if not block:
            continue
        assignments = re.findall(r'\bgAreaScenarioIndex\s*=\s*([^;]+);', block)
        if len(assignments) != 1 or not re.search(r'\bbreak\s*;\s*$', block):
            raise ValueError('unrecognized area scenario assignment')
        expression = assignments[0].strip()
        selector = re.fullmatch(r'(presentationSelect\w+)\s*\(\s*&gCardPresentationActionScript\s*,\s*&gCardPresentationAudioCue\s*\)', expression)
        if selector:
            returns = re.findall(r'\breturn\b([^;]*);', function_body(code, selector[1]))
            if not returns or any(not re.fullmatch(r'\s*(0[xX][0-9a-fA-F]+|\d+)\s*', value) for value in returns):
                raise ValueError(f'{selector[1]}: scenario return requires further analysis')
            values = sorted({int(value.strip(), 0) for value in returns})
            name = selector[1]
        elif re.fullmatch(r'0[xX][0-9a-fA-F]+|\d+', expression):
            values, name = [int(expression, 0)], None
        else:
            raise ValueError('scenario index is not a known selector or literal')
        for area in pending:
            if area in result:
                raise ValueError('duplicate area scenario case')
            result[area] = dict(indices=values, selector=name)
        pending.clear()
    if pending or set(result) != set(range(23)):
        raise ValueError('scenario selection must cover exactly the 23 retained areas')
    return result
